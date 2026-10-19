// Loading MUGEN sprite files (.sff) and palettes (.act).
//
// A screenpack's entire look -- title art, menu cursors, lifebars, font
// glyphs -- lives in one or two of these. There are two formats in the wild:
//
//   v1  a singly-linked list of 32-byte subheaders, each followed by an
//       inline PCX image. Some of those images carry a 768-byte palette and
//       the rest share the last one seen -- see gfx/pcx_codec.h.
//   v2  flat arrays of 28-byte sprite nodes and 16-byte palette nodes, with
//       pixels held in shared `ldata` (literal) and `tdata` (compressed)
//       blocks and referenced by offset.
//
// They share nothing but the 512-byte signature, so they are decoded
// separately below rather than behind a common abstraction. The version is
// the fourth byte of the version word at 0x0C.
//
// Only the index is kept resident. Pixel data is fetched back out of the file
// with a range read when a sprite is first drawn, then handed to the GPU and
// dropped. MUGEN files compress well and there are tens of thousands of
// sprites in a full roster; holding every decoded RGBA image is multiple
// gigabytes and is what used to kill the app on a phone. See texture().
//
// Reference: Elecbyte's "MUGEN File Format Documentation" and the SFF 2.00
// spec, cross-checked byte-for-byte against a real Elecbyte-produced file.
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/file_source.h"
#include "gfx/renderer.h"

namespace af {

// A sprite's identity and geometry. `width` and `height` come straight out of
// the file's header (v2) or the PCX header (v1), so they are available without
// touching the pixels.
//
// `axis` is the sprite's own origin, expressed as an offset *from* its
// top-left corner. This is the single most important number in the whole
// format: MUGEN positions every sprite by its axis, so a sprite drawn at
// (x, y) has its top-left at (x - axisX, y - axisY).
struct Sprite {
    int16_t group = 0;
    int16_t image = 0;
    int16_t axisX = 0;
    int16_t axisY = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] bool valid() const { return width > 0 && height > 0; }
};

// One sprite's index within a bank. Sprites are addressed the way MUGEN
// addresses them: by a (group, image) pair, not by ordinal.
using SpriteKey = uint32_t;

[[nodiscard]] inline SpriteKey spriteKey(int group, int image) {
    return (static_cast<uint32_t>(static_cast<uint16_t>(group)) << 16) |
           static_cast<uint16_t>(image);
}

class SpriteBank {
public:
    // A palette node's MUGEN identity, the `(group, item)` pair a `RemapPal`
    // controller names. SFF v2 carries one on every palette node; SFF v1 has no
    // palette nodes at all, and the .act files a character adds are addressed by
    // item alone in group 1, so their table entries keep the default {-1,-1}.
    struct PaletteKey {
        int group = -1;
        int item = -1;
    };

    SpriteBank() = default;
    ~SpriteBank();
    SpriteBank(const SpriteBank&) = delete;
    SpriteBank& operator=(const SpriteBank&) = delete;

    // Parses the file's index. The file is read once here and then released;
    // individual sprites are fetched back through `files` as they are drawn, so
    // `files` must outlive the bank.
    //
    // Returns false and fills `errorOut` on a malformed file or a missing one.
    // Individual sprites that fail to decode are skipped and logged rather than
    // failing the load, because a screenpack with one bad sprite is still
    // usable.
    bool load(FileSource& files, const std::string& path, std::string* errorOut);

    // Returns nullptr when the sprite is absent.
    [[nodiscard]] const Sprite* find(int group, int image) const;

    // Decodes and uploads on first use, so this needs a current GL context.
    // Returns nullptr when the sprite is absent or its pixels cannot be read.
    // The returned pointer is owned by the bank and stays valid until the next
    // load().
    //
    // An 8-bit paletted sprite uploads as *indices* (Texture::indexed(): one
    // R8 byte per texel, 4x less VRAM than RGBA) plus a 256x1 LUT per palette
    // -- the whole of MUGEN's colour story (palno, RemapPal, PalFX) then works
    // by re-pointing or grading tables instead of re-uploading pixels. True-
    // colour sprites (PNG24/32, 24/32-bit raw) still upload as RGBA.
    const Texture* texture(int group, int image);

    // ------------------------------------------------------------ palettes
    //
    // The colour tables, one per palette node (v2) or distinct palette seen
    // (v1), plus whatever a character's .def adds from its .act files. Index 0
    // is the character's *main* palette by both formats' conventions, which is
    // the slot palno replaces.
    //
    // Appends a 768-byte RGB palette (an .act file's whole payload) and returns
    // its table index, or -1 on a null argument.
    int addPalette(const uint8_t* rgb768);
    [[nodiscard]] int paletteCount() const { return static_cast<int>(palettes_.size()); }

    // Resolves an SFF v2 palette node's `(group, item)` to its table entry.
    // Pure so the mapping can be checked without a loaded file; the bank's own
    // table is searched in `paletteNodeIndex`. A non-positive coordinate names
    // no node, and a bank whose entries carry no key answers -1.
    [[nodiscard]] static int findPaletteKey(const std::vector<PaletteKey>& keys, int group,
                                            int item) {
        if (group <= 0 || item <= 0) return -1;
        for (size_t i = 0; i < keys.size(); ++i) {
            if (keys[i].group == group && keys[i].item == item) return static_cast<int>(i);
        }
        return -1;
    }
    [[nodiscard]] int paletteNodeIndex(int group, int item) const {
        return findPaletteKey(paletteKeys_, group, item);
    }

    // The `(group, item)` identity of palette table entry `index`, or {-1,-1}
    // for an entry with none (an .act append, the grey ramp, or a v1 palette).
    // The draw uses this to ask whether a sprite's *own* palette is the source
    // of a RemapPal entry.
    [[nodiscard]] PaletteKey paletteKeyOf(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= paletteKeys_.size()) return {};
        return paletteKeys_[static_cast<size_t>(index)];
    }

    // The 256x1 LUT texture for palette table entry `index`, uploaded on first
    // use -- needs a GL context like texture(). Returns nullptr for an index
    // out of range (or out of GL memory).
    const Texture* lutTexture(int index);

    // Which palette table entry a sprite's pixels were decoded against, or -1
    // before its texture() has been uploaded (PNG8 sprites register their PLTE
    // palette, and palette-less v1 sprites a grey ramp, at decode time). A
    // positive answer is the sprite's *own* palette: fighter palette selection
    // only replaces slot 0.
    [[nodiscard]] int spritePalette(int group, int image) const;

    [[nodiscard]] size_t size() const { return sprites_.size(); }
    [[nodiscard]] bool empty() const { return sprites_.empty(); }
    [[nodiscard]] int version() const { return version_; }

    // How many sprites the last load skipped because their pixels would not
    // decode. Non-zero is not fatal but is worth reporting, since the symptom
    // is otherwise just some sprites silently missing.
    [[nodiscard]] int decodeFailures() const { return decodeFailures_; }

    // Every sprite in file order. Used by the font loader, which needs to walk
    // a whole (group) range rather than look up individual keys.
    [[nodiscard]] const std::vector<Sprite>& sprites() const { return sprites_; }

private:
    // Where one sprite's pixels live, and how to turn them back into RGBA.
    // `palette` indexes palettes_; -1 means the sprite has no palette and is
    // drawn through the grey ramp (see indexedToRgba in the .cpp).
    struct Payload {
        size_t offset = 0;
        size_t length = 0;
        int format = 0;
        int colorDepth = 8;
        int palette = -1;
        bool pcx = false;
        bool failed = false;
    };

    struct Palette {
        std::array<uint8_t, 768> rgb{};
        int colors = 0;  // 0 means the node exists but carries no colour data
    };

    bool loadV1(const uint8_t* data, size_t size, std::string* errorOut);
    bool loadV2(const uint8_t* data, size_t size, std::string* errorOut);

    void addSprite(Sprite sprite, Payload payload);

    // Reads this one sprite's bytes back out of the file and expands them to
    // RGBA -- for the true-colour payloads only (PNG24/32, 24/32-bit raw).
    bool decode(const Payload& payload, int width, int height,
                std::vector<uint8_t>& rgba) const;

    // Whether the payload is 8-bit paletted: PCX, the RLE8/RLE5/LZ5 family,
    // PNG8, or 8-bit raw. Those upload as index textures rather than RGBA.
    [[nodiscard]] static bool isIndexed(const Payload& payload) {
        return payload.pcx || payload.format == 2 || payload.format == 3 || payload.format == 4 ||
               payload.format == 10 || (payload.format == 0 && payload.colorDepth == 8);
    }

    // Reads this sprite's bytes back and expands any compression into raw
    // 8-bit palette indices. Non-const: a PNG8 sprite registers its PLTE
    // palette in the table here, and a sprite with no usable palette entry is
    // pointed at the shared grey ramp so every indexed upload has exactly one
    // LUT. Both write payload.palette.
    bool decodeIndices(Payload& payload, int width, int height,
                       std::vector<uint8_t>& indices);

    // The palette a sprite with no palette data falls back to: a grey ramp
    // (index i -> (i, i, i)). Created once so every palette-less sprite shares
    // it; drawing in grey is a better failure than drawing invisible.
    int ensureGreyRamp();

    [[nodiscard]] const uint8_t* paletteData(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= palettes_.size()) return nullptr;
        if (palettes_[static_cast<size_t>(index)].colors <= 0) return nullptr;
        return palettes_[static_cast<size_t>(index)].rgb.data();
    }

    std::vector<Sprite> sprites_;
    std::vector<Payload> payloads_;
    std::unordered_map<SpriteKey, int> byKey_;

    // Parallel to sprites_; null until uploaded. unique_ptr because Texture is
    // move-only and we need stable addresses for the returned pointers.
    std::vector<std::unique_ptr<Texture>> textures_;

    // One entry per palette node (v2) or distinct palette seen (v1), so the
    // colour tables survive after the file itself is released.
    std::vector<Palette> palettes_;

    // Parallel to palettes_: the node's MUGEN identity for a v2 node, {-1,-1}
    // for anything that has none. Kept beside the table rather than folded into
    // Palette because only the palette path reads it.
    std::vector<PaletteKey> paletteKeys_;

    // Parallel to palettes_; null until lutTexture() uploads the entry. The LUT
    // side of the indexed path: one 256x1 RGB8 texture per palette, shared by
    // every sprite that draws with it.
    std::vector<std::unique_ptr<Texture>> luts_;
    int greyRamp_ = -1;  // palette table index of the fallback ramp, or -1

    FileSource* source_ = nullptr;
    std::string path_;

    int version_ = 0;
    int decodeFailures_ = 0;
};

}  // namespace af

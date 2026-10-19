#include "gfx/sprite_bank.h"

#include <algorithm>
#include <cstring>

#include "core/log.h"
#include "gfx/pcx_codec.h"
#include "gfx/sff_codecs.h"

namespace af {

namespace {

constexpr size_t kHeaderSize = 512;
constexpr size_t kV1SubheaderSize = 32;
constexpr size_t kV2SpriteNodeSize = 28;
constexpr size_t kV2PaletteNodeSize = 16;
constexpr size_t kPaletteBytes = 768;  // 256 RGB triples, in .act files

// The signature is 12 bytes and the twelfth is a NUL. Files written by
// non-Elecbyte tools sometimes drop the NUL, so accept both rather than
// rejecting an otherwise-fine screenpack over one byte.
bool hasSffSignature(const uint8_t* data) {
    return std::memcmp(data, "ElecbyteSpr\0", 12) == 0 ||
           std::memcmp(data, "ElecbyteSpr", 11) == 0;
}

// All multi-byte fields in both SFF revisions are little-endian, regardless of
// host byte order.
inline uint16_t rdU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
inline int16_t rdI16(const uint8_t* p) { return static_cast<int16_t>(rdU16(p)); }
inline uint32_t rdU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
inline int32_t rdI32(const uint8_t* p) { return static_cast<int32_t>(rdU32(p)); }

// 8-bit paletted pixels upload as indices and are coloured by a LUT at draw
// time, so the RGBA expansion helper the old path used is gone. Index 0 as the
// transparent key is a convention of the format, not something the file
// records, and it is applied unconditionally in the fragment shader.
void rawToRgba(const uint8_t* src, int width, int height, int bytesPerPixel,
                std::vector<uint8_t>& out) {
    out.assign(static_cast<size_t>(width) * height * 4, 0);
    for (int i = 0; i < width * height; ++i) {
        const uint8_t* p = src + static_cast<size_t>(i) * bytesPerPixel;
        out[i * 4 + 0] = p[0];
        out[i * 4 + 1] = p[1];
        out[i * 4 + 2] = p[2];
        out[i * 4 + 3] = (bytesPerPixel >= 4) ? p[3] : 255;
    }
}

// ---------------------------------------------------------------- RLE8 (SFF v2)
// See gfx/sff_codecs.h -- the two v2 pixel codecs live there, extracted so a
// host build can test them against real character files without a GLES
// context.

}  // namespace

SpriteBank::~SpriteBank() = default;

void SpriteBank::addSprite(Sprite sprite, Payload payload) {
    const SpriteKey key = spriteKey(sprite.group, sprite.image);
    byKey_[key] = static_cast<int>(sprites_.size());
    sprites_.push_back(std::move(sprite));
    payloads_.push_back(payload);
}

bool SpriteBank::load(FileSource& files, const std::string& path, std::string* errorOut) {
    sprites_.clear();
    payloads_.clear();
    byKey_.clear();
    textures_.clear();
    palettes_.clear();
    paletteKeys_.clear();
    luts_.clear();
    greyRamp_ = -1;
    decodeFailures_ = 0;
    version_ = 0;

    source_ = &files;
    path_ = path;

    std::unique_ptr<FileData> data = files.read(path);
    if (!data) {
        if (errorOut) *errorOut = "sprite file not found: " + path;
        source_ = nullptr;
        path_.clear();
        return false;
    }
    if (data->size() < kHeaderSize) {
        if (errorOut) *errorOut = "sprite file is smaller than a 512-byte header";
        source_ = nullptr;
        path_.clear();
        return false;
    }
    const uint8_t* bytes = data->data();
    if (!hasSffSignature(bytes)) {
        if (errorOut) *errorOut = "not an SFF file (bad signature)";
        source_ = nullptr;
        path_.clear();
        return false;
    }

    // The version is the high byte of the 4-byte version word at 0x0C. 1 and 2
    // are the only revisions that exist; anything else is a file we do not
    // understand and should say so rather than guess.
    const uint8_t versionHigh = bytes[0x0F];
    version_ = versionHigh;

    bool ok = false;
    if (versionHigh == 1) {
        ok = loadV1(bytes, data->size(), errorOut);
    } else if (versionHigh == 2) {
        ok = loadV2(bytes, data->size(), errorOut);
    } else if (errorOut) {
        *errorOut = "unsupported SFF version " + std::to_string(versionHigh);
    }

    if (ok) {
        textures_.resize(sprites_.size());
        AF_LOGI("sprite bank '%s': SFF v%u, %zu sprites, %d undecodable", path.c_str(),
                versionHigh, sprites_.size(), decodeFailures_);
    } else {
        // A partially-parsed bank is not a usable bank; keep the invariant that
        // payloads_ is parallel to sprites_ for the next load.
        sprites_.clear();
        payloads_.clear();
        byKey_.clear();
        palettes_.clear();
        paletteKeys_.clear();
        source_ = nullptr;
        path_.clear();
    }
    return ok;
}

// ---------------------------------------------------------------------- SFF v1

bool SpriteBank::loadV1(const uint8_t* bytes, size_t size, std::string* errorOut) {
    const uint32_t imageCount = rdU32(bytes + 0x14);
    uint32_t offset = rdU32(bytes + 0x18);
    if (offset == 0 || offset >= size) {
        if (errorOut) *errorOut = "SFF v1 first subheader offset is out of range";
        return false;
    }

    // Walk the singly-linked list of subheaders first, collecting each one's
    // file offset, so that linked sprites (which reference a *sprite index*, not
    // a file offset) can be resolved afterwards.
    struct Node {
        uint32_t offset;
        uint32_t dataLength;
        int16_t axisX, axisY;
        uint16_t group, image;
        uint16_t linkedIndex;
    };
    std::vector<Node> nodes;
    nodes.reserve(imageCount);

    // Guard against a cycle as well as against running off the end: a corrupt
    // file should not spin forever.
    const size_t maxNodes = size / kV1SubheaderSize + 1;
    while (offset != 0 && offset + kV1SubheaderSize <= size && nodes.size() < maxNodes) {
        const uint8_t* h = bytes + offset;
        Node node{};
        node.offset = offset;
        node.dataLength = rdU32(h + 0x04);
        node.axisX = rdI16(h + 0x08);
        node.axisY = rdI16(h + 0x0A);
        node.group = rdU16(h + 0x0C);
        node.image = rdU16(h + 0x0E);
        node.linkedIndex = rdU16(h + 0x10);
        nodes.push_back(node);
        const uint32_t next = rdU32(h + 0x00);
        if (next <= offset) break;  // non-advancing => cycle or terminator
        offset = next;
    }

    if (nodes.empty()) {
        if (errorOut) *errorOut = "SFF v1 contains no sprites";
        return false;
    }

    // V1 sprites share a palette: the author stores it once, on whichever
    // sprite comes first, and the rest are written without one. The PCX
    // decoder tracks that state for us while the file is in memory; every
    // distinct palette is copied out here so the sprite can be decoded later,
    // after the file itself is gone.
    const uint8_t* sharedPalette = nullptr;
    std::unordered_map<std::string, int> paletteIds;

    for (const Node& node : nodes) {
        const Node* source = &node;
        // A zero data length marks a linked sprite: it reuses the pixels of an
        // earlier sprite, given by index. Follow the chain, with a hop limit so a
        // malformed file cannot loop.
        for (int hops = 0; source->dataLength == 0 && hops < 64; ++hops) {
            if (source->linkedIndex >= nodes.size()) break;
            const Node* next = &nodes[source->linkedIndex];
            if (next == source) break;
            source = next;
        }

        Sprite sprite;
        sprite.group = static_cast<int16_t>(node.group);
        sprite.image = static_cast<int16_t>(node.image);
        sprite.axisX = node.axisX;
        sprite.axisY = node.axisY;

        Payload payload;
        if (source->dataLength == 0) {
            payload.failed = true;
            ++decodeFailures_;
            addSprite(std::move(sprite), payload);
            continue;
        }

        const size_t dataOffset = source->offset + kV1SubheaderSize;
        if (dataOffset >= size) {
            payload.failed = true;
            ++decodeFailures_;
            addSprite(std::move(sprite), payload);
            continue;
        }
        const size_t available = size - dataOffset;
        const size_t blockLength = std::min<size_t>(source->dataLength, available);

        // The block is decoded once here only to learn the image size and to
        // advance the shared palette. The indices are dropped immediately.
        PcxBlock block;
        if (decodePcxBlock(bytes + dataOffset, blockLength, block, sharedPalette)) {
            sprite.width = block.width;
            sprite.height = block.height;
            payload.offset = dataOffset;
            payload.length = blockLength;
            payload.pcx = true;

            if (block.palette != nullptr) {
                const std::string key(reinterpret_cast<const char*>(block.palette), kPaletteBytes);
                const auto it = paletteIds.find(key);
                if (it != paletteIds.end()) {
                    payload.palette = it->second;
                } else {
                    Palette palette;
                    std::memcpy(palette.rgb.data(), block.palette, kPaletteBytes);
                    palette.colors = 256;
                    const int index = static_cast<int>(palettes_.size());
                    palettes_.push_back(palette);
                    // A v1 PCX palette has no MUGEN palette-node identity.
                    paletteKeys_.emplace_back();
                    paletteIds.emplace(key, index);
                    payload.palette = index;
                }
            }
        } else {
            payload.failed = true;
            ++decodeFailures_;
        }
        addSprite(std::move(sprite), payload);
    }
    return true;
}

// ---------------------------------------------------------------------- SFF v2

bool SpriteBank::loadV2(const uint8_t* bytes, size_t size, std::string* errorOut) {
    const int32_t spriteNodesOffset = rdI32(bytes + 0x24);
    const int32_t spriteCount = rdI32(bytes + 0x28);
    const int32_t paletteNodesOffset = rdI32(bytes + 0x2C);
    const int32_t paletteCount = rdI32(bytes + 0x30);
    const int32_t ldataOffset = rdI32(bytes + 0x34);
    const int32_t tdataOffset = rdI32(bytes + 0x3C);

    if (spriteCount <= 0 || spriteNodesOffset < 0 ||
        static_cast<size_t>(spriteNodesOffset) >= size) {
        if (errorOut) *errorOut = "SFF v2 sprite node table is out of range";
        return false;
    }

    // Resolve the palettes up front and convert them to the RGB triplets the
    // rest of the engine uses. Sprites refer to them by index; the raw RGBA
    // entries in the file are not needed again.
    if (paletteCount > 0 && paletteNodesOffset > 0) {
        palettes_.resize(static_cast<size_t>(paletteCount));
        paletteKeys_.resize(static_cast<size_t>(paletteCount));
        std::vector<uint8_t> resolved(static_cast<size_t>(paletteCount), 0);

        auto fill = [&](int index, const uint8_t* data, size_t colors) {
            Palette& palette = palettes_[static_cast<size_t>(index)];
            const size_t n = std::min<size_t>(colors, 256);
            palette.colors = static_cast<int>(n);
            for (size_t c = 0; c < n; ++c) {
                palette.rgb[c * 3 + 0] = data[c * 4 + 0];
                palette.rgb[c * 3 + 1] = data[c * 4 + 1];
                palette.rgb[c * 3 + 2] = data[c * 4 + 2];
            }
        };

        for (int i = 0; i < paletteCount; ++i) {
            const size_t nodeAt =
                static_cast<size_t>(paletteNodesOffset) + static_cast<size_t>(i) * kV2PaletteNodeSize;
            if (nodeAt + kV2PaletteNodeSize > size) break;
            const uint8_t* h = bytes + nodeAt;
            // The node's own identity, which is what a `RemapPal` names. It is
            // kept even when the colours are linked from another node: the
            // linked node still exists under its own `(group, item)`.
            paletteKeys_[static_cast<size_t>(i)] = PaletteKey{rdU16(h + 0x00), rdU16(h + 0x02)};
            uint16_t linked = rdU16(h + 0x06);
            const uint32_t dataOffset = rdU32(h + 0x08);
            const uint32_t dataLength = rdU32(h + 0x0C);

            // Zero length means this palette node shares another node's colours.
            if (dataLength == 0) {
                for (int hops = 0; linked < palettes_.size() && hops < 64; ++hops) {
                    if (resolved[linked] != 0) {
                        palettes_[i] = palettes_[linked];
                        resolved[i] = 1;
                        break;
                    }
                    const size_t linkedAt = static_cast<size_t>(paletteNodesOffset) +
                                            static_cast<size_t>(linked) * kV2PaletteNodeSize;
                    if (linkedAt + kV2PaletteNodeSize > size) break;
                    const uint8_t* lh = bytes + linkedAt;
                    const uint32_t ll = rdU32(lh + 0x0C);
                    if (ll != 0) {
                        const size_t at = static_cast<size_t>(ldataOffset) + rdU32(lh + 0x08);
                        if (at + ll <= size) {
                            fill(i, bytes + at, ll / 4);
                            resolved[i] = 1;
                        }
                        break;
                    }
                    linked = rdU16(lh + 0x06);
                }
                continue;
            }

            const size_t at = static_cast<size_t>(ldataOffset) + dataOffset;
            if (at + dataLength <= size) {
                fill(i, bytes + at, dataLength / 4);
                resolved[i] = 1;
            }
        }
    }

    for (int i = 0; i < spriteCount; ++i) {
        const size_t nodeAt =
            static_cast<size_t>(spriteNodesOffset) + static_cast<size_t>(i) * kV2SpriteNodeSize;
        if (nodeAt + kV2SpriteNodeSize > size) break;
        const uint8_t* h = bytes + nodeAt;

        Sprite sprite;
        sprite.group = rdI16(h + 0x00);
        sprite.image = rdI16(h + 0x02);
        const int width = rdI16(h + 0x04);
        const int height = rdI16(h + 0x06);
        sprite.axisX = rdI16(h + 0x08);
        sprite.axisY = rdI16(h + 0x0A);
        int16_t linkedIndex = rdI16(h + 0x0C);
        uint8_t format = h[0x0E];
        const uint8_t colorDepth = h[0x0F];
        const int32_t dataOffset = rdI32(h + 0x10);
        int32_t dataLength = rdI32(h + 0x14);
        int16_t paletteIndex = rdI16(h + 0x18);
        const int16_t flags = rdI16(h + 0x1A);

        Payload payload;

        // A linked sprite carries no pixels of its own. `linkedIndex` is a sprite
        // index, and only the axis is genuinely per-sprite, so resolve the whole
        // header from the target and keep our own axis.
        if (dataLength == 0) {
            bool found = false;
            for (int hops = 0; hops < 64 && linkedIndex >= 0 && linkedIndex < spriteCount; ++hops) {
                const size_t at = static_cast<size_t>(spriteNodesOffset) +
                                  static_cast<size_t>(linkedIndex) * kV2SpriteNodeSize;
                if (at + kV2SpriteNodeSize > size) break;
                const uint8_t* lh = bytes + at;
                if (rdI32(lh + 0x14) != 0) {
                    format = lh[0x0E];
                    dataLength = rdI32(lh + 0x14);
                    paletteIndex = rdI16(lh + 0x18);
                    const int32_t linkedDataOffset = rdI32(lh + 0x10);
                    const int16_t linkedFlags = rdI16(lh + 0x1A);
                    const size_t base = (linkedFlags & 1) ? static_cast<size_t>(tdataOffset)
                                                          : static_cast<size_t>(ldataOffset);
                    const size_t at2 = base + static_cast<size_t>(linkedDataOffset);
                    if (at2 + dataLength <= size) {
                        payload.offset = at2;
                        payload.length = static_cast<size_t>(dataLength);
                        payload.format = format;
                        payload.colorDepth = colorDepth;
                        payload.palette = paletteIndex;
                        found = true;
                    }
                    break;
                }
                linkedIndex = rdI16(lh + 0x0C);
            }
            if (found) {
                sprite.width = width;
                sprite.height = height;
            } else {
                payload.failed = true;
                ++decodeFailures_;
            }
            addSprite(std::move(sprite), payload);
            continue;
        }

        // `flags` bit 0 selects which block the offset is relative to: clear
        // means the literal block, set means the compressed one. Getting this
        // backwards reads unrelated bytes and produces noise.
        const size_t base = (flags & 1) ? static_cast<size_t>(tdataOffset)
                                        : static_cast<size_t>(ldataOffset);
        const size_t at = base + static_cast<size_t>(dataOffset);

        bool ok = false;
        if (at >= size || width <= 0 || height <= 0) {
            ok = false;
        } else {
            const size_t srcLen = std::min<size_t>(static_cast<size_t>(dataLength), size - at);
            payload.offset = at;
            payload.length = srcLen;
            payload.format = format;
            payload.colorDepth = colorDepth;
            payload.palette = paletteIndex;

            switch (format) {
                case 0:  // raw
                    if (colorDepth == 8) {
                        ok = srcLen >= static_cast<size_t>(width) * height;
                    } else if (colorDepth == 24 || colorDepth == 32) {
                        ok = srcLen >= static_cast<size_t>(width) * height * (colorDepth / 8);
                    }
                    break;

                case 2:   // RLE8
                case 3:   // RLE5
                case 4:   // LZ5
                case 10:  // PNG8
                case 11:  // PNG24
                case 12:  // PNG32
                    // These blocks all start with the decompressed length,
                    // which we do not need -- we know it from width * height.
                    // For the PNG formats the bytes after it are a complete
                    // PNG file.
                    ok = srcLen > 4;
                    break;

                default:
                    break;
            }
        }

        if (ok) {
            sprite.width = width;
            sprite.height = height;
        } else {
            payload.failed = true;
            ++decodeFailures_;
        }
        addSprite(std::move(sprite), payload);
    }
    return true;
}

// ----------------------------------------------------------------------- pixel access

// True-colour payloads only: PNG24/32 and 24/32-bit raw. Paletted payloads go
// through decodeIndices() and never produce RGBA at all.
bool SpriteBank::decode(const Payload& payload, int width, int height,
                        std::vector<uint8_t>& rgba) const {
    if (payload.failed || payload.length == 0 || source_ == nullptr ||
        width <= 0 || height <= 0) {
        return false;
    }

    // Fetch just this sprite's compressed block. The file itself is not kept
    // in memory: a full roster is hundreds of megabytes of SFF, and the whole
    // point of this class is not to hold all of it at once.
    std::vector<uint8_t> bytes;
    if (!source_->readRange(path_, payload.offset, payload.length, bytes)) return false;

    switch (payload.format) {
        case 0:  // raw
            if (payload.colorDepth == 24 || payload.colorDepth == 32) {
                const size_t needed =
                    static_cast<size_t>(width) * height * (payload.colorDepth / 8);
                if (bytes.size() < needed) return false;
                rawToRgba(bytes.data(), width, height, payload.colorDepth / 8, rgba);
                return true;
            }
            return false;

        case 11:   // PNG24
        case 12: {  // PNG32 -- decode straight to RGBA, no palette involved
            if (bytes.size() <= 4) return false;
            rgba.assign(static_cast<size_t>(width) * height * 4, 0);
            if (!decodePngToRgba(bytes.data() + 4, bytes.size() - 4,
                                 static_cast<size_t>(width) * height, rgba.data())) {
                AF_LOGW("SFF v2 sprite fmt %d (PNG%s) could not be decoded", payload.format,
                        payload.format == 11 ? "24" : "32");
                return false;
            }
            return true;
        }

        default:
            return false;
    }
}

// 8-bit paletted payloads. The output is the raw index buffer -- one byte per
// texel, 0 the transparent key -- which is uploaded as R8.
//
// Where the colour table comes from depends on the format, and this is also
// where last-minute palette bookkeeping happens, because some payload kinds do
// not know their palette from the index pass alone:
//
//   v1/PCX   palette was registered during load (the shared-palette rule means
//            the index pass already decoded every block once).
//   v2 node  payload.palette names the node table entry directly.
//   PNG8     the palette is the PNG's own PLTE chunk, known only after the
//            decode -- registered here, deduped against the table, so a 200-
//            glyph font still ends up with one LUT rather than 200.
//   nothing  a sprite no palette ever attached to draws through the grey ramp
//            (ensureGreyRamp) rather than invisibly.
bool SpriteBank::decodeIndices(Payload& payload, int width, int height,
                               std::vector<uint8_t>& indices) {
    if (payload.failed || payload.length == 0 || source_ == nullptr ||
        width <= 0 || height <= 0) {
        return false;
    }
    const size_t count = static_cast<size_t>(width) * height;

    std::vector<uint8_t> bytes;
    if (!source_->readRange(path_, payload.offset, payload.length, bytes)) return false;

    if (payload.pcx) {
        PcxBlock block;
        // The palette state is irrelevant here (indices only), but the decoder
        // insists on a variable to carry.
        const uint8_t* shared = nullptr;
        if (!decodePcxBlock(bytes.data(), bytes.size(), block, shared)) return false;
        if (block.indices.size() < count) return false;
        indices = std::move(block.indices);
    } else {
        switch (payload.format) {
            case 0:  // raw, 8-bit: the block IS the indices
                if (payload.colorDepth != 8 || bytes.size() < count) return false;
                indices.assign(bytes.begin(), bytes.begin() + static_cast<ptrdiff_t>(count));
                break;

            case 2:  // RLE8
            case 3:  // RLE5
            case 4:  // LZ5
                // The first four bytes are the decompressed length; we already
                // know it from the sprite's dimensions.
                if (bytes.size() <= 4) return false;
                indices.assign(count, 0);
                if (payload.format == 2) {
                    decodeRle8(bytes.data() + 4, bytes.size() - 4, count, indices.data());
                } else if (payload.format == 3) {
                    decodeRle5(bytes.data() + 4, bytes.size() - 4, count, indices.data());
                } else {
                    decodeLz5(bytes.data() + 4, bytes.size() - 4, count, indices.data());
                }
                break;

            case 10: {  // PNG8 -- indices stay indexed; the palette comes from PLTE
                if (bytes.size() <= 4) return false;
                indices.assign(count, 0);
                std::array<uint8_t, kPaletteBytes> pngPalette{};
                if (!decodePngToIndices(bytes.data() + 4, bytes.size() - 4, count,
                                        indices.data(), pngPalette.data())) {
                    AF_LOGW("SFF v2 sprite fmt 10 (PNG8) could not be decoded");
                    return false;
                }
                // Register the PNG's own palette. A sprite whose palette lives in
                // its pixels' container is the PNG family's whole point: the
                // character's palno does not reach it, which matches MUGEN.
                int found = -1;
                for (size_t i = 0; i < palettes_.size(); ++i) {
                    if (palettes_[i].colors == 256 &&
                        std::memcmp(palettes_[i].rgb.data(), pngPalette.data(), kPaletteBytes) == 0) {
                        found = static_cast<int>(i);
                        break;
                    }
                }
                if (found >= 0) {
                    payload.palette = found;
                } else {
                    payload.palette = addPalette(pngPalette.data());
                }
                break;
            }

            default:
                return false;
        }
    }

    if (paletteData(payload.palette) == nullptr) payload.palette = ensureGreyRamp();
    return indices.size() >= count;
}

int SpriteBank::addPalette(const uint8_t* rgb768) {
    if (rgb768 == nullptr) return -1;
    Palette palette;
    std::memcpy(palette.rgb.data(), rgb768, kPaletteBytes);
    palette.colors = 256;
    palettes_.push_back(palette);
    // An .act is addressed as group 1 item N by the character's own table, not
    // by a palette node; this entry has no node identity of its own.
    paletteKeys_.emplace_back();
    return static_cast<int>(palettes_.size()) - 1;
}

int SpriteBank::ensureGreyRamp() {
    if (greyRamp_ >= 0) return greyRamp_;
    Palette palette;
    for (int i = 0; i < 256; ++i) {
        palette.rgb[static_cast<size_t>(i) * 3 + 0] = static_cast<uint8_t>(i);
        palette.rgb[static_cast<size_t>(i) * 3 + 1] = static_cast<uint8_t>(i);
        palette.rgb[static_cast<size_t>(i) * 3 + 2] = static_cast<uint8_t>(i);
    }
    palette.colors = 256;
    palettes_.push_back(palette);
    paletteKeys_.emplace_back();
    greyRamp_ = static_cast<int>(palettes_.size()) - 1;
    return greyRamp_;
}

const Texture* SpriteBank::lutTexture(int index) {
    if (index < 0 || static_cast<size_t>(index) >= palettes_.size()) return nullptr;
    if (palettes_[static_cast<size_t>(index)].colors <= 0) index = ensureGreyRamp();
    if (luts_.size() < palettes_.size()) luts_.resize(palettes_.size());
    if (!luts_[static_cast<size_t>(index)]) {
        auto lut = std::make_unique<Texture>();
        if (!lut->uploadLut256(palettes_[static_cast<size_t>(index)].rgb.data())) {
            AF_LOGW("palette %d failed to upload as a LUT", index);
            return nullptr;
        }
        luts_[static_cast<size_t>(index)] = std::move(lut);
    }
    return luts_[static_cast<size_t>(index)].get();
}

int SpriteBank::spritePalette(int group, int image) const {
    const auto it = byKey_.find(spriteKey(group, image));
    if (it == byKey_.end()) return -1;
    const size_t index = static_cast<size_t>(it->second);
    if (index >= payloads_.size()) return -1;
    return payloads_[index].palette;
}

// ----------------------------------------------------------------------- access

const Sprite* SpriteBank::find(int group, int image) const {
    const auto it = byKey_.find(spriteKey(group, image));
    if (it == byKey_.end()) return nullptr;
    return &sprites_[static_cast<size_t>(it->second)];
}

const Texture* SpriteBank::texture(int group, int image) {
    const auto it = byKey_.find(spriteKey(group, image));
    if (it == byKey_.end()) return nullptr;
    const size_t index = static_cast<size_t>(it->second);

    if (index >= textures_.size()) textures_.resize(sprites_.size());
    if (!textures_[index]) {
        const Sprite& sprite = sprites_[index];
        if (!sprite.valid() || index >= payloads_.size()) return nullptr;
        Payload& payload = payloads_[index];

        auto tex = std::make_unique<Texture>();
        if (isIndexed(payload)) {
            // The palette path: one index byte per texel, colours from the LUT.
            std::vector<uint8_t> indices;
            if (!decodeIndices(payload, sprite.width, sprite.height, indices)) {
                // Do not retry a sprite whose block the file will not give back;
                // it would mean a failed range read on every frame it is drawn.
                payload.failed = true;
                AF_LOGW("sprite %d,%d could not be decoded", sprite.group, sprite.image);
                return nullptr;
            }
            if (!tex->uploadRed8(indices.data(), sprite.width, sprite.height)) {
                AF_LOGW("sprite %d,%d failed to upload as a texture", sprite.group, sprite.image);
                return nullptr;
            }
            tex->setDefaultLut(lutTexture(payload.palette));
        } else {
            std::vector<uint8_t> rgba;
            if (!decode(payload, sprite.width, sprite.height, rgba)) {
                payload.failed = true;
                AF_LOGW("sprite %d,%d could not be decoded", sprite.group, sprite.image);
                return nullptr;
            }
            // A failed upload is not fatal: the caller draws nothing and the rest of
            // the screenpack still renders.
            if (!tex->uploadRgba8(rgba.data(), sprite.width, sprite.height)) {
                AF_LOGW("sprite %d,%d failed to upload as a texture", sprite.group, sprite.image);
                return nullptr;
            }
        }
        textures_[index] = std::move(tex);
    }
    return textures_[index].get();
}

}  // namespace af

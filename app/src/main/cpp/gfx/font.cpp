#include "gfx/font.h"

#include <algorithm>
#include <cstring>

#include "core/ini.h"
#include "core/log.h"

namespace af {

namespace {

constexpr size_t kFntHeaderSize = 64;
constexpr size_t kPcxHeaderSize = 128;
constexpr size_t kPaletteBytes = 768;

bool hasFntSignature(const uint8_t* data, size_t size) {
    if (size < kFntHeaderSize) return false;
    return std::memcmp(data, "ElecbyteFnt\0", 12) == 0 ||
           std::memcmp(data, "ElecbyteFnt", 11) == 0;
}

inline uint16_t rdU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
inline int16_t rdI16(const uint8_t* p) { return static_cast<int16_t>(rdU16(p)); }
inline uint32_t rdU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

// The directory part of a path, including the trailing '/', or empty.
std::string directoryOf(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return {};
    return path.substr(0, slash + 1);
}

// Resolves `name` against `baseDir` unless it is already rooted, and normalises
// "./" so that MUGEN's habit of writing `file = ./font.sff` still resolves.
std::string resolveRelative(const std::string& baseDir, std::string_view name) {
    std::string out(name);
    while (out.rfind("./", 0) == 0) out.erase(0, 2);
    if (!out.empty() && out[0] == '/') return out;
    return baseDir + out;
}

// Parses a `[map]` key, which is either a literal character or an `0xNN` escape.
// A literal is read as a single byte: MUGEN bitmap fonts predate any notion of
// multibyte characters, and upstream implementations take only the first byte
// too. Returns -1 when the token is not a character at all.
int parseMapChar(const std::string& token) {
    if (token.empty()) return -1;
    if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
        return static_cast<int>(std::strtol(token.c_str() + 2, nullptr, 16));
    }
    return static_cast<unsigned char>(token[0]);
}

// Decodes the 8-bit PCX that makes up a v1 font's glyph strip. Mirrors the PCX
// decoder in sprite_bank.cpp -- SFF v1 and FNT v1 use the same image format, but
// the two files locate the palette differently enough that sharing the code
// would obscure more than it saves.
std::vector<uint8_t> decodePcxStrip(const uint8_t* pcx, size_t pcxSize, int& width, int& height,
                                    std::vector<uint8_t>& paletteOut) {
    width = height = 0;
    if (pcxSize < kPcxHeaderSize + kPaletteBytes) return {};
    if (pcx[0] != 0x0A || pcx[3] != 8) return {};

    width = rdI16(pcx + 8) - rdI16(pcx + 4) + 1;
    height = rdI16(pcx + 10) - rdI16(pcx + 6) + 1;
    if (width <= 0 || height <= 0) return {};
    const int bytesPerLine = rdU16(pcx + 66);
    if (bytesPerLine <= 0) return {};

    // The pixel stream runs from just past the header to just before the
    // trailing palette.
    const size_t pixelsEnd = pcxSize - kPaletteBytes;
    if (pixelsEnd <= kPcxHeaderSize) return {};

    std::vector<uint8_t> indices(static_cast<size_t>(width) * height, 0);
    const uint8_t* p = pcx + kPcxHeaderSize;
    const uint8_t* const end = pcx + pixelsEnd;

    int row = 0, col = 0;
    size_t produced = 0;
    const size_t total = indices.size();

    while (produced < total && p < end && row < height) {
        const uint8_t d = *p++;
        int count = 1;
        uint8_t value = d;
        if ((d & 0xC0) == 0xC0) {
            count = d & 0x3F;
            if (p >= end) break;
            value = *p++;
            if (count == 0) continue;
        }
        for (int i = 0; i < count; ++i) {
            if (col < width) indices[static_cast<size_t>(row) * width + col] = value;
            ++col;
            ++produced;
            if (col >= bytesPerLine) {
                col = 0;
                if (++row >= height) break;
            }
        }
    }

    paletteOut.assign(pcx + pcxSize - kPaletteBytes, pcx + pcxSize);
    return indices;
}

}  // namespace

bool Font::load(FileSource& files, const std::string& path, std::string* errorOut) {
    glyphs_.clear();
    glyphRects_.clear();
    bank_.reset();
    stripTexture_.release();
    loaded_ = false;
    version_ = 0;

    std::unique_ptr<FileData> data = files.read(path);
    if (!data) {
        if (errorOut) *errorOut = "font file not found: " + path;
        return false;
    }

    // v1 announces itself with a signature; v2 is text, so anything without the
    // signature is parsed as a text def.
    const bool binary = hasFntSignature(data->data(), data->size());
    version_ = binary ? 1 : 2;
    const bool ok = binary ? loadV1(*data, errorOut) : loadV2(files, path, *data, errorOut);
    if (ok) {
        loaded_ = true;
        AF_LOGI("font '%s': v%d, %dx%d cell, %s", path.c_str(), version_, cellWidth_, cellHeight_,
                binary ? "strip" : "sprite");
    }
    return ok;
}

// ----------------------------------------------------------------------- FNT v1

bool Font::loadV1(const FileData& data, std::string* errorOut) {
    const uint8_t* bytes = data.data();
    const size_t size = data.size();

    const uint32_t pcxOffset = rdU32(bytes + 0x10);
    const uint32_t pcxLength = rdU32(bytes + 0x14);
    const uint32_t txtOffset = rdU32(bytes + 0x18);
    const uint32_t txtLength = rdU32(bytes + 0x1C);

    if (pcxOffset >= size || txtOffset >= size) {
        if (errorOut) *errorOut = "font v1: data offsets are out of range";
        return false;
    }

    const size_t pcxAvailable = size - pcxOffset;
    const size_t pcxBlock = std::min<size_t>(pcxLength == 0 ? pcxAvailable : pcxLength, pcxAvailable);

    int stripWidth = 0, stripHeight = 0;
    std::vector<uint8_t> palette;
    const std::vector<uint8_t> indices =
        decodePcxStrip(bytes + pcxOffset, pcxBlock, stripWidth, stripHeight, palette);
    if (indices.empty()) {
        if (errorOut) *errorOut = "font v1: glyph strip failed to decode";
        return false;
    }

    // The text blob is a normal INI document.
    const size_t txtAvailable = size - txtOffset;
    const size_t txtBlock = std::min<size_t>(txtLength == 0 ? txtAvailable : txtLength, txtAvailable);
    const IniFile text = IniFile::parse(std::string_view(
        reinterpret_cast<const char*>(bytes + txtOffset), txtBlock));

    const IniSection* def = text.section("def");
    if (def != nullptr) {
        const std::vector<std::string> cell = splitCommas(IniFile::get(*def, "size"));
        if (cell.size() >= 1) tryParseInt(cell[0], cellWidth_);
        if (cell.size() >= 2) tryParseInt(cell[1], cellHeight_);
        const std::vector<std::string> spacing = splitCommas(IniFile::get(*def, "spacing"));
        if (spacing.size() >= 1) tryParseInt(spacing[0], spacingX_);
        if (spacing.size() >= 2) tryParseInt(spacing[1], spacingY_);
    }
    // `size` is mandatory in a well-formed font; fall back to the strip so a
    // slightly-broken file still renders something sensible.
    if (cellWidth_ <= 0) cellWidth_ = stripWidth;
    if (cellHeight_ <= 0) cellHeight_ = stripHeight;

    const IniSection* map = text.section("map");
    if (map == nullptr) {
        if (errorOut) *errorOut = "font v1: no [map] section";
        return false;
    }

    // The whole strip becomes one texture, and each [map] record carves its
    // glyph out of it as a source rectangle. Sharing one texture across every
    // character keeps a font to a single upload.
    std::vector<uint8_t> rgba(static_cast<size_t>(stripWidth) * stripHeight * 4, 0);
    for (int i = 0; i < stripWidth * stripHeight; ++i) {
        const uint8_t index = indices[static_cast<size_t>(i)];
        const uint8_t* rgb = palette.data() + static_cast<size_t>(index) * 3;
        rgba[i * 4 + 0] = rgb[0];
        rgba[i * 4 + 1] = rgb[1];
        rgba[i * 4 + 2] = rgb[2];
        rgba[i * 4 + 3] = (index == 0) ? 0 : 255;
    }
    if (!stripTexture_.uploadRgba8(rgba.data(), stripWidth, stripHeight)) {
        if (errorOut) *errorOut = "font v1: glyph strip failed to upload";
        return false;
    }

    glyphs_.assign(256, GlyphSlot{});
    glyphRects_.assign(256, Rect{});

    // Records advance a running x cursor, so a `[map]` line may omit its offset
    // and simply take the next free position in the strip.
    int cursor = 0;
    for (const IniLine& line : map->lines) {
        const std::vector<std::string> parts = splitWhitespace(line.raw);
        if (parts.empty()) continue;

        const int code = parseMapChar(parts[0]);
        if (code < 0 || code > 255) continue;

        int offset = cursor;
        int advance = cellWidth_;
        if (parts.size() >= 2) {
            int parsed = 0;
            if (tryParseInt(parts[1], parsed)) offset = parsed;
        }
        if (parts.size() >= 3) {
            int parsed = 0;
            if (tryParseInt(parts[2], parsed)) {
                // A negative width is how MUGEN encodes a per-character nudge:
                // it shifts the glyph along and then means the same width.
                if (parsed < 0) {
                    offset += offset - parsed;
                    parsed = -parsed;
                }
                advance = parsed;
            }
        }

        const size_t slot = static_cast<size_t>(code);
        glyphs_[slot].present = true;
        glyphs_[slot].advance = advance;
        glyphRects_[slot] = Rect{static_cast<float>(offset), 0.0f, static_cast<float>(advance),
                                 static_cast<float>(stripHeight)};
        cursor = offset + advance;
    }

    return true;
}

// ----------------------------------------------------------------------- FNT v2

bool Font::loadV2(FileSource& files, const std::string& path, const FileData& data,
                  std::string* errorOut) {
    const IniFile text = IniFile::parse(
        std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    const IniSection* def = text.section("def");
    if (def == nullptr) {
        if (errorOut) *errorOut = "font v2: no [def] section";
        return false;
    }

    const std::vector<std::string> cell = splitCommas(IniFile::get(*def, "size"));
    if (cell.size() >= 1) tryParseInt(cell[0], cellWidth_);
    if (cell.size() >= 2) tryParseInt(cell[1], cellHeight_);
    const std::vector<std::string> spacing = splitCommas(IniFile::get(*def, "spacing"));
    if (spacing.size() >= 1) tryParseInt(spacing[0], spacingX_);
    if (spacing.size() >= 2) tryParseInt(spacing[1], spacingY_);

    const std::string file = unquote(IniFile::get(*def, "file"));
    if (file.empty()) {
        if (errorOut) *errorOut = "font v2: [def] has no `file`";
        return false;
    }

    // MUGEN resolves the SFF relative to the font file, not the game root.
    const std::string sffPath = resolveRelative(directoryOf(path), file);
    bank_ = std::make_unique<SpriteBank>();
    std::string error;
    if (!bank_->load(files, sffPath, &error)) {
        if (errorOut) *errorOut = "font v2: " + error;
        return false;
    }

    // In a sprite font the bank *is* the sprite group, and the image index is
    // the character code. Nothing is enumerated up front: glyphs are looked up
    // on demand so that a font covering a large range costs nothing extra.
    return true;
}

// ----------------------------------------------------------------- glyph access

bool Font::glyph(unsigned char c, int bankIndex, Glyph& out) const {
    if (!loaded_) return false;

    if (version_ == 1) {
        if (!stripTexture_.valid()) return false;
        const GlyphSlot& slot = glyphs_[c];
        if (!slot.present) return false;
        out.texture = &stripTexture_;
        out.width = static_cast<int>(glyphRects_[c].w);
        out.height = static_cast<int>(glyphRects_[c].h);
        out.advance = slot.advance + spacingX_;
        return true;
    }

    if (!bank_) return false;
    const Sprite* sprite = bank_->find(bankIndex, c);
    if (sprite == nullptr || !sprite->valid()) return false;
    out.texture = bank_->texture(bankIndex, c);
    if (out.texture == nullptr) return false;
    out.width = sprite->width;
    out.height = sprite->height;
    out.advance = sprite->width + spacingX_;
    return true;
}

int Font::textWidth(std::string_view text, int bankIndex) const {
    int total = 0;
    Glyph g;
    for (const char ch : text) {
        if (glyph(static_cast<unsigned char>(ch), bankIndex, g)) {
            total += g.advance;
        } else {
            // An uncovered character still occupies space, so that a missing
            // glyph shows up as a gap rather than silently closing up and
            // shifting everything after it.
            total += cellWidth_ + spacingX_;
        }
    }
    return total;
}

void Font::draw(Renderer& renderer, float x, float y, std::string_view text, int bankIndex,
                TextAlign align, Color tint, float scale) const {
    if (!loaded_ || text.empty()) return;

    const float width = static_cast<float>(textWidth(text, bankIndex)) * scale;
    float penX = x;
    if (align == TextAlign::Center) {
        penX = x - width * 0.5f;
    } else if (align == TextAlign::Right) {
        penX = x - width;
    }

    for (const char ch : text) {
        Glyph g;
        if (!glyph(static_cast<unsigned char>(ch), bankIndex, g)) {
            penX += static_cast<float>(cellWidth_ + spacingX_) * scale;
            continue;
        }

        if (version_ == 1) {
            const Rect src = glyphRects_[static_cast<unsigned char>(ch)];
            renderer.drawSprite(*g.texture, penX, y, src.w * scale, src.h * scale, src, tint,
                                false, false, Blend::Normal);
        } else {
            // The glyph's own axis positions it relative to the baseline cell.
            const Sprite* sprite = bank_->find(bankIndex, static_cast<unsigned char>(ch));
            const float ox = penX - static_cast<float>(sprite->axisX) * scale;
            const float oy = y - static_cast<float>(sprite->axisY) * scale;
            const Rect src{0, 0, static_cast<float>(sprite->width),
                           static_cast<float>(sprite->height)};
            renderer.drawSprite(*g.texture, ox, oy, src.w * scale, src.h * scale, src, tint, false,
                                false, Blend::Normal);
        }
        penX += static_cast<float>(g.advance) * scale;
    }
}

}  // namespace af

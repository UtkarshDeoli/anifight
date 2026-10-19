// Loading MUGEN bitmap fonts.
//
// Two revisions exist and they have nothing in common:
//
//   v1  a binary file with an `ElecbyteFnt` signature, holding an 8-bit PCX
//       glyph strip plus a text blob of `[map]` records saying where each
//       character sits in that strip.
//   v2  not a binary format at all -- a plain text `.def` naming an SFF whose
//       glyphs are sprites, one group per bank and the character code as the
//       image index.
//
// Both end up as "a glyph is a rectangle of pixels with a width and an
// advance", which is all the text layout below needs.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/file_source.h"
#include "gfx/renderer.h"
#include "gfx/sprite_bank.h"

namespace af {

// Horizontal anchoring, matching MUGEN's third `font = slot, bank, align` field.
enum class TextAlign {
    Left = -1,
    Center = 0,
    Right = 1,
};

[[nodiscard]] inline TextAlign textAlignFromInt(int value) {
    if (value < 0) return TextAlign::Left;
    if (value > 0) return TextAlign::Right;
    return TextAlign::Center;
}

class Font {
public:
    Font() = default;
    ~Font() = default;
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // `path` is relative to the game's data root. Resolves any SFF the font
    // references relative to the same directory, so a screenpack that keeps its
    // fonts in a subfolder still works.
    bool load(FileSource& files, const std::string& path, std::string* errorOut);

    // A laid-out glyph: where to draw it and how far to advance.
    struct Glyph {
        const Texture* texture = nullptr;
        int width = 0;
        int height = 0;
        int advance = 0;
    };

    // `bank` selects a sprite group in v2 sprite fonts; v1 fonts ignore it
    // because our generated fonts have a single bank. Returns false for a
    // character the font does not cover, which is not an error -- the caller
    // simply advances past it.
    bool glyph(unsigned char c, int bank, Glyph& out) const;

    // Width in font pixels, before any scaling.
    [[nodiscard]] int textWidth(std::string_view text, int bank) const;
    [[nodiscard]] int lineHeight() const { return cellHeight_; }
    [[nodiscard]] int cellWidth() const { return cellWidth_; }
    [[nodiscard]] bool loaded() const { return loaded_; }

    // Draws `text` with its left edge / centre / right edge at `x` according to
    // `align`, with the top of the line at `y`. `scale` enlarges the font
    // uniformly, which is how the HUD scales MUGEN's fixed-size fonts.
    void draw(Renderer& renderer, float x, float y, std::string_view text, int bank,
              TextAlign align, Color tint, float scale = 1.0f) const;

private:
    bool loadV1(const FileData& data, std::string* errorOut);
    bool loadV2(FileSource& files, const std::string& path, const FileData& data,
                std::string* errorOut);

    struct GlyphSlot {
        int advance = 0;
        bool present = false;
    };

    std::unique_ptr<SpriteBank> bank_;
    // Indexed by character code directly: a 256-entry table is smaller than a
    // hash map and every MUGEN bitmap font is ASCII-only in practice.
    std::vector<GlyphSlot> glyphs_;
    // v1 glyphs are sub-rectangles of one shared glyph strip, so the strip is a
    // single texture and each glyph just remembers its slice of it.
    std::vector<Rect> glyphRects_;
    Texture stripTexture_;

    int cellWidth_ = 0;
    int cellHeight_ = 0;
    int spacingX_ = 0;
    int spacingY_ = 0;
    bool loaded_ = false;
    int version_ = 0;
};

}  // namespace af

// The SFF v1 pixel decoder, standing alone.
//
// Every sprite in a v1 file is a small 8-bit PCX preceded by nothing but a
// 32-byte subheader, so this one function is the whole v1 image story. It
// lives apart from SpriteBank so a host build can compile and test it against
// real character files without a GLES context -- and because the rule it
// encodes about palettes is the kind that fails silently, painting a fighter
// in the wrong colours while every other part of the loader reports success.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace af {

// A sprite's optional trailing palette, and the byte that introduces it. A
// block holding its own palette ends with 0x0C followed by 768 bytes of RGB;
// a block that borrows one ends after the pixel stream with the 0x0C alone.
//
// Those are the only two shapes -- there is no block with a palette and no
// marker, and none with anything between the stream and the marker. Both
// readings depend on knowing exactly where the stream ends, which is why the
// decoder consumes whole padded scanlines and not just the visible pixels.
constexpr size_t kPcxPaletteBytes = 768;
constexpr uint8_t kPcxPaletteMarker = 0x0C;

struct PcxBlock {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> indices;  // width * height palette indices

    // The palette these indices should be read against: the sprite's own when
    // it carries one, otherwise the one an earlier sprite in the same file
    // left in `sharedPalette`. Null when nothing in the file has carried a
    // palette yet, which every consumer has to tolerate.
    const uint8_t* palette = nullptr;
};

// Decodes one sprite block. Returns false when the block is not a usable 8-bit
// PCX, in which case `out` is untouched.
//
// `sharedPalette` carries palette state between calls and must be the same
// variable for every sprite in a file. V1 sprites share a palette: the author
// stores it once, on whichever sprite comes first, and the rest are written
// without one. This function advances the pointer when the block in front of
// it carries a palette and otherwise reads through it, which is the whole of
// the sharing rule.
bool decodePcxBlock(const uint8_t* block, size_t size, PcxBlock& out,
                    const uint8_t*& sharedPalette);

}  // namespace af

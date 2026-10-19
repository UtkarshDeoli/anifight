// Checks the SFF v1 palette-sharing rule (gfx/pcx_codec).
//
// This is the failure mode worth a test: a v1 file stores its palette on one
// sprite and leaves the rest without one, and a decoder that reads the last
// 768 bytes of every block regardless hands those sprites part of their own
// pixel stream as a colour table. Nothing fails -- the sprite decodes, the
// dimensions are right, the silhouette is right -- and the fighter simply
// comes out in colours that are not its own. It is invisible to every check
// except looking at the picture.
//
// The blocks below are synthetic but sized like the real ones, because the
// mistake depends on the block being longer than a palette: a sprite of a few
// hundred bytes cannot produce it.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_pcx_codec \
//         tools/hosttest/test_pcx_codec.cpp app/src/main/cpp/gfx/pcx_codec.cpp
//     /tmp/test_pcx_codec

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "gfx/pcx_codec.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++failures;
}

constexpr int kW = 40;
constexpr int kH = 40;  // 1600 pixels: a literal stream of 1600 bytes

// A PCX header plus one literal byte per pixel, so the pixel stream lands at a
// known offset. `bytesPerLine` wider than kW adds padding to every scanline,
// the way an encoder aligning its rows would.
std::vector<uint8_t> pcxBlock(uint8_t pixelValue, const std::vector<uint8_t>* palette,
                              int bytesPerLine = kW) {
    std::vector<uint8_t> block(128, 0);
    block[0] = 0x0A;  // manufacturer
    block[1] = 0x05;  // version
    block[2] = 0x01;  // RLE encoded
    block[3] = 0x08;  // 8bpp
    block[4] = 0;
    block[5] = 0;  // xmin
    block[6] = 0;
    block[7] = 0;  // ymin
    block[8] = static_cast<uint8_t>(kW - 1);
    block[9] = 0;  // xmax
    block[10] = static_cast<uint8_t>(kH - 1);
    block[11] = 0;  // ymax
    block[66] = static_cast<uint8_t>(bytesPerLine);  // bytes per line
    block[67] = 0;

    // The padding is a literal byte, not an RLE opcode, so it decodes cleanly
    // and is discarded on the way out.
    for (int y = 0; y < kH; ++y) {
        block.insert(block.end(), kW, pixelValue);
        block.insert(block.end(), bytesPerLine - kW, 0x11);
    }
    block.push_back(af::kPcxPaletteMarker);
    if (palette != nullptr) block.insert(block.end(), palette->begin(), palette->end());
    return block;
}

}  // namespace

int main() {
    // A palette whose entries are all distinct, so a wrong entry is visible as
    // a wrong number rather than a coincidence.
    std::vector<uint8_t> marker(af::kPcxPaletteBytes, 0);
    for (size_t i = 0; i < af::kPcxPaletteBytes; ++i) marker[i] = static_cast<uint8_t>(i * 7);

    const std::vector<uint8_t> carried = pcxBlock(7, &marker);
    const std::vector<uint8_t> borrowed = pcxBlock(9, nullptr);

    // Both are comfortably longer than a palette, which is what makes reading
    // the tail dangerous.
    check(carried.size() > af::kPcxPaletteBytes + 128, "a carrying block is longer than a palette");
    check(borrowed.size() > af::kPcxPaletteBytes + 128, "a borrowing block is longer than a palette");
    check(borrowed.size() == 128 + static_cast<size_t>(kW) * kH + 1,
          "a borrowing block is the pixel stream and the marker, and nothing else");

    const uint8_t* shared = nullptr;

    // The first sprite carries its palette for the ones after it.
    af::PcxBlock first;
    check(af::decodePcxBlock(carried.data(), carried.size(), first, shared),
          "the carrying block decodes");
    check(shared != nullptr, "decoding a carrying block sets the shared palette");
    check(first.palette == shared, "a carrying sprite uses the palette it carries");
    check(first.palette == carried.data() + 128 + static_cast<size_t>(kW) * kH + 1,
          "that palette is the one right after the marker");

    // The bug: this sprite has no palette of its own, so it has to use the one
    // the previous sprite carried -- not the last 768 bytes of its own block.
    af::PcxBlock second;
    check(af::decodePcxBlock(borrowed.data(), borrowed.size(), second, shared),
          "the borrowing block decodes");
    check(second.palette == first.palette,
          "a borrowing sprite uses the palette the previous sprite carried");
    check(second.palette != borrowed.data() + borrowed.size() - af::kPcxPaletteBytes,
          "a borrowing sprite does NOT use the tail of its own pixel stream");
    check(second.palette != nullptr && second.palette[1 * 3 + 0] == marker[1 * 3 + 0],
          "its colours read out of the carried palette, not out of pixel data");

    // And its pixels still decode: the palette rule must not disturb the image.
    check(second.width == kW && second.height == kH, "a borrowing sprite keeps its dimensions");
    bool allNine = second.indices.size() == static_cast<size_t>(kW) * kH;
    for (uint8_t v : second.indices) allNine = allNine && v == 9;
    check(allNine, "a borrowing sprite keeps its pixels");

    // Scanlines are `bytesPerLine` wide, which is the width rounded up, so the
    // stream is longer than the image and ends later than width*height suggests.
    // A decoder that stops at the visible pixels stops short of the palette it
    // is standing on, and this sprite quietly borrows instead of carrying --
    // the same wrong colours, arrived at from the other side.
    const std::vector<uint8_t> padded = pcxBlock(13, &marker, kW + 2);
    check(padded.size() == 128 + static_cast<size_t>(kW + 2) * kH + 1 + af::kPcxPaletteBytes,
          "a padded block is as long as its whole stream, padding included");

    const uint8_t* paddedShared = nullptr;
    af::PcxBlock paddedSprite;
    check(af::decodePcxBlock(padded.data(), padded.size(), paddedSprite, paddedShared),
          "a padded block decodes");
    check(paddedShared != nullptr, "a padded block sets the shared palette");
    check(paddedSprite.palette == paddedShared, "a padded sprite uses the palette it carries");
    check(paddedSprite.palette == padded.data() + 128 + static_cast<size_t>(kW + 2) * kH + 1,
          "it finds that palette past the row padding, right after the marker");
    check(paddedSprite.palette != nullptr && paddedSprite.palette[1 * 3 + 0] == marker[1 * 3 + 0],
          "and the colours it finds are the carried palette's");

    // The padding is not part of the image: it must not leak into the pixels.
    bool noPad = paddedSprite.indices.size() == static_cast<size_t>(kW) * kH;
    for (uint8_t v : paddedSprite.indices) noPad = noPad && v == 13;
    check(noPad, "a padded sprite keeps its pixels and drops the padding");

    // A padded block that borrows still borrows: the padding must not be
    // mistaken for a palette sitting where the stream appeared to stop.
    const std::vector<uint8_t> paddedBorrow = pcxBlock(17, nullptr, kW + 2);
    const uint8_t* carry = &marker[0];
    af::PcxBlock borrowedPadded;
    check(af::decodePcxBlock(paddedBorrow.data(), paddedBorrow.size(), borrowedPadded, carry),
          "a padded borrowing block decodes");
    check(borrowedPadded.palette == &marker[0], "a padded borrowing sprite uses the shared palette");

    // A file whose first sprite borrows: nothing has carried a palette yet, so
    // there is nothing to read through and the caller has to cope with null.
    const uint8_t* empty = nullptr;
    af::PcxBlock orphan;
    check(af::decodePcxBlock(borrowed.data(), borrowed.size(), orphan, empty),
          "a borrowing first block decodes");
    check(orphan.palette == nullptr,
          "with nothing carried yet a borrowing sprite reports no palette rather than a wrong one");

    // The shared palette follows the file forward: a later sprite that carries
    // its own replaces it for the sprites after that.
    std::vector<uint8_t> other(af::kPcxPaletteBytes, 0);
    other[3] = 200;
    const std::vector<uint8_t> third = pcxBlock(11, &other);
    af::PcxBlock thirdDecoded;
    check(af::decodePcxBlock(third.data(), third.size(), thirdDecoded, empty),
          "a second carrying block decodes");
    check(thirdDecoded.palette[3] == 200, "a later carrying sprite replaces the shared palette");
    check(thirdDecoded.palette == empty, "and that is what is now shared");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

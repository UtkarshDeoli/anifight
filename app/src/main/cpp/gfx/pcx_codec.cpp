#include "gfx/pcx_codec.h"

namespace af {

namespace {

constexpr size_t kPcxHeaderSize = 128;

inline uint16_t rdU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
inline int16_t rdI16(const uint8_t* p) { return static_cast<int16_t>(rdU16(p)); }

}  // namespace

bool decodePcxBlock(const uint8_t* block, size_t size, PcxBlock& out,
                    const uint8_t*& sharedPalette) {
    if (size < kPcxHeaderSize) return false;
    if (block[0] != 0x0A) return false;  // PCX manufacturer id
    if (block[3] != 8) return false;     // must be 8bpp; SFF v1 has no other depth

    const int xmin = rdI16(block + 4);
    const int ymin = rdI16(block + 6);
    const int xmax = rdI16(block + 8);
    const int ymax = rdI16(block + 10);
    const int width = xmax - xmin + 1;
    const int height = ymax - ymin + 1;
    if (width <= 0 || height <= 0) return false;

    // bytes-per-line lives at offset 66 in a PCX header, not adjacent to the
    // fields above.
    const int bytesPerLine = rdU16(block + 66);
    if (bytesPerLine <= 0) return false;

    const size_t pixelCount = static_cast<size_t>(width) * height;
    std::vector<uint8_t> indices(pixelCount, 0);

    const uint8_t* p = block + kPcxHeaderSize;
    const uint8_t* const end = block + size;

    // A PCX scanline is `bytesPerLine` bytes wide, which the encoder rounds up
    // for alignment, so the stream holds more values than the image has pixels
    // and only ends once every padded row has been read. Stopping at
    // width*height instead leaves those pad bytes in front of whatever follows
    // the image -- and the palette follows the image, so the block then looks
    // like it has a handful of stray bytes between its pixels and its colours
    // and the sprite loses them.
    //
    // The padding decodes like any other pixel and is simply dropped: `col`
    // counts against bytesPerLine and only the first `width` columns are kept.
    const size_t streamCount = static_cast<size_t>(bytesPerLine) * height;

    int row = 0, col = 0;
    size_t produced = 0;

    while (produced < streamCount && p < end && row < height) {
        const uint8_t d = *p++;
        int count = 1;
        uint8_t value = d;
        if ((d & 0xC0) == 0xC0) {
            count = d & 0x3F;
            if (p >= end) break;
            value = *p++;
            if (count == 0) continue;  // a zero-length run is legal and means nothing
        }

        for (int i = 0; i < count; ++i) {
            if (col < width) indices[static_cast<size_t>(row) * width + col] = value;
            ++col;
            ++produced;
            if (col >= bytesPerLine) {
                col = 0;
                ++row;
                // A run that straddles the row boundary is truncated here, which
                // is what PCX encoders assume.
                if (row >= height) break;
            }
        }
    }

    out.width = width;
    out.height = height;
    out.indices = std::move(indices);

    // Where the palette is, now that the pixel stream has been consumed. The
    // two shapes a v1 block comes in:
    //
    //   [pixels][0x0C][768 bytes]   the sprite carries its own palette
    //   [pixels][0x0C]              the sprite borrows the last one seen
    //
    // So the marker is exactly where the stream ended in both, and the only
    // question is whether 768 bytes follow it. That check is what makes this
    // exact rather than a guess -- reading the trailing 768 bytes
    // unconditionally, as if the second shape did not exist, hands such a
    // sprite 767 bytes of its own pixel stream as a colour table. The picture
    // keeps its shape and every colour in it is noise, and it is not obvious
    // from the image that the palette is the part that is broken.
    out.palette = sharedPalette;
    if (static_cast<size_t>(end - p) >= 1 + kPcxPaletteBytes && p[0] == kPcxPaletteMarker) {
        out.palette = sharedPalette = p + 1;
    }
    return true;
}

}  // namespace af

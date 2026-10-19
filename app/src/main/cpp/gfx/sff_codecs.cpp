#include "gfx/sff_codecs.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <zlib.h>

namespace af {

void decodeRle8(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out) {
    size_t i = 0, produced = 0;
    while (i < size && produced < pixelCount) {
        const uint8_t d = src[i++];
        if ((d & 0xC0) == 0x40) {
            const size_t run = d & 0x3F;
            if (i >= size) break;
            const uint8_t value = src[i++];
            for (size_t n = 0; n < run && produced < pixelCount; ++n) out[produced++] = value;
        } else {
            out[produced++] = d;
        }
    }
}

void decodeLz5(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out) {
    size_t i = 0, produced = 0;
    uint8_t control = 0;
    int controlBits = 8;      // forces a reload on the first token
    uint32_t highBits = 0;    // high bits of a back-reference offset
    int highBitCount = 0;

    auto copyBackRef = [&](size_t offset, size_t length) {
        if (offset == 0 || offset > produced) return;  // malformed; nothing sane to copy
        for (size_t n = 0; n < length && produced < pixelCount; ++n) {
            out[produced] = out[produced - offset];
            ++produced;
        }
    };

    while (i < size && produced < pixelCount) {
        if (controlBits >= 8) {
            control = src[i++];
            controlBits = 0;
        }
        const bool isBackRef = (control & (1 << controlBits)) != 0;
        ++controlBits;

        if (isBackRef) {
            if (i >= size) break;
            const uint8_t d = src[i++];
            if ((d & 0x3F) == 0) {
                // Extended form: a 14-bit offset and an explicit length follow.
                // The length counts the way the reference implementations
                // count it, n+1 copies where n = lastByte + 2, so an exact
                // counter wants lastByte + 3.
                if (i + 1 >= size) break;
                const uint8_t next = src[i];
                const size_t offset = ((static_cast<size_t>(d) << 2) | next) + 1;
                const size_t length = src[i + 1] + 3;
                i += 2;
                copyBackRef(offset, length);
            } else {
                // The two offset high bits: mask the byte FIRST, then shift
                // the two extracted bits down. Masking AFTER shifting pulls
                // in the run-length bits (the low 6 bits of `d`) as offset
                // high bits, which misaligns every back-reference that
                // follows -- the bug a real character file exposed.
                highBits |= static_cast<uint32_t>(d & 0xC0) >> highBitCount;
                highBitCount += 2;
                const size_t length = (d & 0x3F) + 1;
                size_t offset = 0;
                if (highBitCount < 8) {
                    if (i >= size) break;
                    offset = static_cast<size_t>(src[i++]) + 1;
                } else {
                    offset = highBits + 1;
                    highBits = 0;
                    highBitCount = 0;
                }
                copyBackRef(offset, length);
            }
        } else {
            if (i >= size) break;
            const uint8_t d = src[i++];
            if ((d & 0xE0) == 0) {
                // d < 0x20: d is itself the value and a second byte extends the run.
                if (i >= size) break;
                const size_t length = static_cast<size_t>(src[i++]) + 8;
                for (size_t n = 0; n < length && produced < pixelCount; ++n) out[produced++] = d;
            } else {
                const size_t length = d >> 5;          // 1..7
                const uint8_t value = d & 0x1F;
                for (size_t n = 0; n < length && produced < pixelCount; ++n) {
                    out[produced++] = value;
                }
            }
        }
    }
}

void decodeRle5(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out) {
    size_t i = 0, produced = 0;
    while (i < size && produced < pixelCount) {
        const uint8_t control = src[i++];
        if ((control & 0x80) != 0) {
            // Run: replicate the one following byte (ctrl & 0x7F) + 1 times.
            const size_t run = (control & 0x7F) + 1;
            if (i >= size) break;
            const uint8_t value = src[i++];
            for (size_t n = 0; n < run && produced < pixelCount; ++n) out[produced++] = value;
        } else {
            // Literal run of ctrl + 1 bytes (bit 7 is clear, so ctrl is the
            // whole low field). A truncated stream says more than it holds:
            // take what exists rather than reading past the end.
            const size_t run = std::min({static_cast<size_t>(control) + 1, size - i,
                                         pixelCount - produced});
            std::memcpy(out + produced, src + i, run);
            produced += run;
            i += run;
        }
    }
}

// ------------------------------------------------------------ PNG (formats 10-12)
namespace {

// No sprite is remotely this large; the caps exist so a corrupt length field
// cannot turn into a multi-gigabyte allocation.
constexpr uint32_t kPngMaxDimension = 16384;
constexpr size_t kPngMaxInflated = 256u << 20;

inline uint32_t rdBe32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

int paethPredictor(int left, int up, int upperLeft) {
    const int p = left + up - upperLeft;
    const int pa = std::abs(p - left);
    const int pb = std::abs(p - up);
    const int pc = std::abs(p - upperLeft);
    return (pa <= pb && pa <= pc) ? left : (pb <= pc) ? up : upperLeft;
}

struct PngImage {
    int width = 0;
    int height = 0;
    int colourType = 0;      // 3 = indexed, 2 = RGB, 6 = RGBA
    int plteColors = 0;
    std::array<uint8_t, 768> plte{};
    std::vector<uint8_t> pixels;  // unfiltered scanlines, width * bpp per row
};

// The output buffer is sized exactly from IHDR, so Z_FINISH can only succeed
// if the stream produced everything the header promised -- a silent short
// decode is impossible.
bool inflateAll(const std::vector<uint8_t>& compressed, uint8_t* out, size_t outSize) {
    z_stream zs{};
    zs.next_in = const_cast<Bytef*>(compressed.data());
    zs.avail_in = static_cast<uInt>(compressed.size());
    zs.next_out = out;
    zs.avail_out = static_cast<uInt>(outSize);
    if (inflateInit(&zs) != Z_OK) return false;
    const int rc = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    return rc == Z_STREAM_END && zs.total_out == outSize;
}

// Parses the chunk stream, inflates the accumulated IDAT and reverses the
// per-scanline filter. Returns the raw samples in `image.pixels`; expanding
// indexed or RGB to the caller's target layout is the wrappers' job.
bool decodePng(const uint8_t* src, size_t size, PngImage& image) {
    static const uint8_t kSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (size < 8 || std::memcmp(src, kSignature, 8) != 0) return false;

    bool haveIhdr = false, haveIdat = false;
    uint32_t width = 0, height = 0;
    int colourType = 0;
    std::vector<uint8_t> idat;

    size_t pos = 8;
    while (pos + 12 <= size) {
        const uint32_t length = rdBe32(src + pos);
        const uint8_t* tag = src + pos + 4;
        if (static_cast<size_t>(length) > size - pos - 12) return false;
        const uint8_t* data = src + pos + 8;

        if (std::memcmp(tag, "IHDR", 4) == 0) {
            if (haveIhdr || length != 13) return false;
            width = rdBe32(data);
            height = rdBe32(data + 4);
            if (width == 0 || height == 0 || width > kPngMaxDimension ||
                height > kPngMaxDimension) {
                return false;
            }
            colourType = data[9];
            // 8-bit depth, the standard zlib method, adaptive filtering and
            // no interlace: everything a packed sprite PNG ever uses.
            if (data[8] != 8 || data[10] != 0 || data[11] != 0 || data[12] != 0) return false;
            if (colourType != 3 && colourType != 2 && colourType != 6) return false;
            haveIhdr = true;
        } else if (std::memcmp(tag, "PLTE", 4) == 0) {
            if (!haveIhdr || colourType != 3 || length == 0 || length % 3 != 0 ||
                length / 3 > 256) {
                return false;
            }
            std::memcpy(image.plte.data(), data, length);
            image.plteColors = static_cast<int>(length / 3);
        } else if (std::memcmp(tag, "IDAT", 4) == 0) {
            if (!haveIhdr) return false;
            // Encoders are allowed to split the deflate stream across several
            // IDAT chunks, so they are concatenated and inflated once.
            idat.insert(idat.end(), data, data + length);
            haveIdat = true;
        } else if (std::memcmp(tag, "IEND", 4) == 0) {
            break;
        }
        // Anything else (tEXt, tRNS, gAMA, ...) is auxiliary for the colour
        // types we support and is skipped. Chunk CRCs go unverified on
        // purpose: zlib's own Adler-32 over the IDAT payload plus the exact
        // inflated size is enough assurance for game data.
        pos += 12 + length;
    }
    if (!haveIhdr || !haveIdat) return false;

    const int bpp = (colourType == 3) ? 1 : (colourType == 2) ? 3 : 4;
    const size_t stride = static_cast<size_t>(width) * static_cast<size_t>(bpp);
    const size_t inflatedSize = static_cast<size_t>(height) * (stride + 1);
    if (inflatedSize > kPngMaxInflated) return false;

    std::vector<uint8_t> raw(inflatedSize);
    if (!inflateAll(idat, raw.data(), raw.size())) return false;

    // Each inflated scanline is a filter-type byte followed by `stride` bytes
    // of filtered samples; reconstruction adds a prediction of the byte back
    // in, modulo 256.
    image.pixels.assign(static_cast<size_t>(height) * stride, 0);
    const size_t bppSize = static_cast<size_t>(bpp);
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t filter = raw[static_cast<size_t>(y) * (stride + 1)];
        const uint8_t* row = raw.data() + static_cast<size_t>(y) * (stride + 1) + 1;
        uint8_t* dst = image.pixels.data() + static_cast<size_t>(y) * stride;
        const uint8_t* up = (y > 0) ? dst - stride : nullptr;
        switch (filter) {
            case 0:  // None
                std::memcpy(dst, row, stride);
                break;
            case 1:  // Sub: add the sample one pixel to the left
                for (size_t x = 0; x < stride; ++x) {
                    const uint8_t left = (x >= bppSize) ? dst[x - bppSize] : 0;
                    dst[x] = static_cast<uint8_t>(row[x] + left);
                }
                break;
            case 2:  // Up: add the sample from the row above
                for (size_t x = 0; x < stride; ++x) {
                    dst[x] = static_cast<uint8_t>(row[x] + (up != nullptr ? up[x] : 0));
                }
                break;
            case 3:  // Average: add the floor of the mean of left and up
                for (size_t x = 0; x < stride; ++x) {
                    const int left = (x >= bppSize) ? dst[x - bppSize] : 0;
                    const int above = (up != nullptr) ? up[x] : 0;
                    dst[x] = static_cast<uint8_t>(row[x] + (left + above) / 2);
                }
                break;
            case 4:  // Paeth: add whichever of left/up/upper-left is nearest
                for (size_t x = 0; x < stride; ++x) {
                    const int left = (x >= bppSize) ? dst[x - bppSize] : 0;
                    const int above = (up != nullptr) ? up[x] : 0;
                    const int upperLeft = (up != nullptr && x >= bppSize) ? up[x - bppSize] : 0;
                    dst[x] = static_cast<uint8_t>(row[x] + paethPredictor(left, above, upperLeft));
                }
                break;
            default:
                return false;
        }
    }

    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.colourType = colourType;
    return true;
}

}  // namespace

bool decodePngToIndices(const uint8_t* src, size_t size, size_t pixelCount,
                        uint8_t* out, uint8_t* palOut) {
    PngImage image;
    if (!decodePng(src, size, image)) return false;
    if (image.colourType != 3 || image.plteColors == 0) return false;
    if (static_cast<size_t>(image.width) * static_cast<size_t>(image.height) != pixelCount) {
        return false;
    }
    std::memcpy(out, image.pixels.data(), pixelCount);
    std::memcpy(palOut, image.plte.data(), image.plte.size());
    return true;
}

bool decodePngToRgba(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out) {
    PngImage image;
    if (!decodePng(src, size, image)) return false;
    if (image.colourType != 2 && image.colourType != 6) return false;
    if (static_cast<size_t>(image.width) * static_cast<size_t>(image.height) != pixelCount) {
        return false;
    }
    const size_t bpp = (image.colourType == 6) ? 4 : 3;
    for (size_t i = 0; i < pixelCount; ++i) {
        out[i * 4 + 0] = image.pixels[i * bpp + 0];
        out[i * 4 + 1] = image.pixels[i * bpp + 1];
        out[i * 4 + 2] = image.pixels[i * bpp + 2];
        out[i * 4 + 3] = (bpp == 4) ? image.pixels[i * bpp + 3] : 255;
    }
    return true;
}

}  // namespace af
// The SFF v2 pixel decoders, standing alone.
//
// These are the whole compression story MUGEN 1.1 sprites use (besides raw):
// two run codecs and a back-reference codec, each little more than a page of
// code, plus -- for formats 10-12, which store a complete PNG file -- a
// minimal PNG reader that borrows zlib only for the inflate step. They live
// apart from SpriteBank so they can be compiled and tested on a host -- a
// desktop build needs no GLES at all -- and a broken decoder is exactly the
// kind of bug a test against real character data pins down.
#pragma once

#include <cstddef>
#include <cstdint>

namespace af {

// v2's RLE8 is not the PCX scheme. Its run marker is bits 7..6 == 01, so
// 0x40-0x7F introduce a run while 0x80-0xFF are literals -- the opposite of
// PCX for the high half of the range. There is no scanline padding.
void decodeRle8(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out);

// Elecbyte never finished documenting this; the algorithm below is the one
// every working implementation converged on. It interleaves a literal/back-
// reference stream with a control byte whose bits say which token kind comes
// next.
void decodeLz5(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out);

// v2's RLE5 (format 3) is the plainest of the three: a token is one control
// byte, and bit 7 alone decides what it means. Set means a run -- (ctrl &
// 0x7F) + 1 copies of the single byte that follows it. Clear means a literal
// run -- the next (ctrl) + 1 bytes (bit 7 is 0, so masking is a no-op) are
// used as-is. The control byte comes first in both cases, the same ordering
// RLE8 uses; that is the layout the SFF 2.0 spec describes and the one real
// format-3 files are written in, so no second candidate order is implemented.
void decodeRle5(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out);

// Formats 10-12 store a complete PNG file after the shared 4-byte length
// prefix. The reader is deliberately minimal -- 8-bit depth, no interlace,
// filter types 0-4, chunk CRCs unverified -- which covers every PNG the MUGEN
// tools ever packed into an SFF.
//
// PNG8 (colour type 3) decodes to palette indices in `out`, with the PLTE
// chunk copied into `palOut` as 768 bytes of RGB triples, so the caller's
// existing indexed-to-RGBA pipeline applies unchanged.
bool decodePngToIndices(const uint8_t* src, size_t size, size_t pixelCount,
                        uint8_t* out, uint8_t* palOut);

// PNG24/32 (colour types 2 and 6) decode straight to RGBA8 in `out`
// (pixelCount * 4 bytes, RGBA byte order). A type-2 image gets alpha 255
// everywhere; the tRNS colour key is ignored.
//
// Both return false on anything unsupported or malformed, and both require
// the PNG's IHDR dimensions to multiply out to exactly `pixelCount` -- a
// mismatch means the SFF header and the embedded PNG disagree, which is the
// file's problem, not ours to paper over.
bool decodePngToRgba(const uint8_t* src, size_t size, size_t pixelCount, uint8_t* out);

}  // namespace af
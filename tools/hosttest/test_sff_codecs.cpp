// Host test for the SFF v2 pixel decoders (gfx/sff_codecs).
//
// The fixture is a real third-party SFF v2.01 file -- Elecbyte's Kung Fu Man,
// the reference character that ships with MUGEN itself -- because that is
// exactly the data the decoders must survive in the wild. Our own generated
// files share whatever the reader believes, so they can never catch its
// mistakes; a community file can and does.
//
// The expected index streams were produced by an independent decoder reading
// the published format behaviour; the checksums pin the decoders to them.
//
// Build and run:
//   clang++ -std=c++20 -O1 -o test_sff_codecs \
//       tools/hosttest/test_sff_codecs.cpp app/src/main/cpp/gfx/sff_codecs.cpp
//   ./test_sff_codecs <path>/local_chars/kfm/kfm.sff

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "gfx/sff_codecs.h"

namespace {

// A compact SHA-256: the test's only need is to compare long byte streams
// without embedding them.
struct Sha256 {
    uint32_t h[8];
    uint64_t bits = 0;
    uint8_t buffer[64];
    size_t buffered = 0;

    Sha256() {
        h[0] = 0x6a09e667; h[1] = 0xbb67ae85; h[2] = 0x3c6ef372; h[3] = 0xa54ff53a;
        h[4] = 0x510e527f; h[5] = 0x9b05688c; h[6] = 0x1f83d9ab; h[7] = 0x5be0cd19;
    }
    static uint32_t rotr(uint32_t v, int n) { return (v >> n) | (v << (32 - n)); }

    void block(const uint8_t* p) {
        static const uint32_t k[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
            0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
            0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
            0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
            0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
            0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t)p[i * 4] << 24 | (uint32_t)p[i * 4 + 1] << 16 |
                   (uint32_t)p[i * 4 + 2] << 8 | (uint32_t)p[i * 4 + 3];
        }
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], e = h[4], f = h[5], g = h[6];
        uint32_t d = h[3], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t t1 = hh + s1 + ch + k[i] + w[i];
            const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = s0 + maj;
            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    void update(const uint8_t* p, size_t n) {
        bits += static_cast<uint64_t>(n) * 8;
        while (n > 0) {
            const size_t take = (64 - buffered < n) ? 64 - buffered : n;
            std::memcpy(buffer + buffered, p, take);
            buffered += take;
            p += take;
            n -= take;
            if (buffered == 64) {
                block(buffer);
                buffered = 0;
            }
        }
    }

    std::string hexdigest() {
        const uint8_t pad = 0x80;
        const uint64_t bitSnapshot = bits;
        update(&pad, 1);
        const uint8_t zero = 0;
        while (buffered != 56) update(&zero, 1);
        uint8_t lenBytes[8];
        for (int i = 0; i < 8; ++i) lenBytes[i] = static_cast<uint8_t>(bitSnapshot >> (56 - i * 8));
        update(lenBytes, 8);
        std::string out(64, '0');
        for (int i = 0; i < 8; ++i) {
            for (int b = 0; b < 4; ++b) {
                const uint8_t byte = static_cast<uint8_t>(h[i] >> (24 - b * 8));
                static const char* digits = "0123456789abcdef";
                out[i * 8 + b * 2] = digits[byte >> 4];
                out[i * 8 + b * 2 + 1] = digits[byte & 0xF];
            }
        }
        return out;
    }
};

std::string sha256(const uint8_t* p, size_t n) {
    Sha256 s;
    s.update(p, n);
    return s.hexdigest();
}

std::vector<uint8_t> readFile(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == nullptr) return {};
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    if (fread(data.data(), 1, data.size(), f) != data.size()) {
        fclose(f);
        return {};
    }
    fclose(f);
    return data;
}

// Minimal SFF v2 header walk, just enough to reach a sprite's data.
uint32_t rdU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
int16_t rdI16(const uint8_t* p) {
    return static_cast<int16_t>(p[0] | (p[1] << 8));
}

struct SpriteNodeRef {
    int16_t group = 0, image = 0;
    int w = 0, h = 0;
    uint8_t format = 0;
    uint32_t dataOffset = 0, dataLength = 0;
};

SpriteNodeRef spriteNode(const std::vector<uint8_t>& d, int index) {
    const size_t spriteNodes = rdU32(d.data() + 0x24);
    const size_t nodeAt = spriteNodes + static_cast<size_t>(index) * 28;
    SpriteNodeRef s;
    s.group = rdI16(d.data() + nodeAt);
    s.image = rdI16(d.data() + nodeAt + 2);
    s.w = rdI16(d.data() + nodeAt + 4);
    s.h = rdI16(d.data() + nodeAt + 6);
    s.format = d[nodeAt + 0x0E];
    s.dataOffset = rdU32(d.data() + nodeAt + 0x10);
    s.dataLength = rdU32(d.data() + nodeAt + 0x14);
    return s;
}

bool decodeAndCheck(const std::vector<uint8_t>& d, int index, const std::string& expected,
                    const char* label) {
    const SpriteNodeRef s = spriteNode(d, index);
    const size_t ldata = rdU32(d.data() + 0x34);
    // Both v2 codecs expect `src` past the 4-byte uncompressed-length prefix.
    std::vector<uint8_t> out(static_cast<size_t>(s.w) * s.h, 0);
    const uint8_t* src = d.data() + ldata + s.dataOffset;
    if (s.format == 2) {
        af::decodeRle8(src + 4, s.dataLength - 4, out.size(), out.data());
    } else if (s.format == 4) {
        af::decodeLz5(src + 4, s.dataLength - 4, out.size(), out.data());
    } else {
        std::printf("FAIL %s: fixture uses unexpected format %u\n", label, s.format);
        return false;
    }
    const std::string got = sha256(out.data(), out.size());
    const bool ok = got == expected;
    std::printf("%s %s: sprite (%d,%d) %dx%d fmt %u\n", ok ? "PASS" : "FAIL", label, s.group,
                s.image, s.w, s.h, s.format);
    if (!ok) std::printf("    expected %s\n    got      %s\n", expected.c_str(), got.c_str());
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: test_sff_codecs <path-to-kfm.sff>\n");
        return 2;
    }
    const std::vector<uint8_t> d = readFile(argv[1]);
    if (d.size() < 512) {
        std::printf("FAIL: could not read fixture\n");
        return 2;
    }

    // Kung Fu Man's sprite nodes, in file order: the 120x140 palette-5 portrait,
    // the 25x25 select portrait, and the 47x106 standing pose. The standing pose
    // is the load-bearing one for LZ5: its stream leans on back-references
    // whose offset high bits are nonzero (plus extended forms), which is
    // precisely where a mis-assembled offset stops producing sane art.
    bool ok = true;
    ok = decodeAndCheck(d, 0, "7c2d72d9f14782a4591e0e040e7be51c61baa96adc669b5ef467d8501a05f427",
                        "rle8 portrait") && ok;
    ok = decodeAndCheck(d, 1, "510104495a73db92083b53d358530f6e860c7b12001a3aa3d0bd167afe3a1a12",
                        "lz5 small portrait") && ok;
    ok = decodeAndCheck(d, 2, "d497fd178f10c6b74ab4fc6b68987adb9f8549f4b6496d9f0cfdb43c5becf8ed",
                        "lz5 standing pose") && ok;
    return ok ? 0 : 1;
}
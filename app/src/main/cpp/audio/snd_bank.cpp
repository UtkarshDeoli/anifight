#include "audio/snd_bank.h"

#include <cstring>

#include "core/log.h"

namespace af {

namespace {

constexpr char kSignature[12] = {'E', 'l', 'e', 'c', 'b', 'y', 't', 'e',
                                 'S', 'n', 'd', '\0'};
constexpr size_t kHeaderBytes = 0x18;
constexpr size_t kNodeBytes = 16;

uint32_t rdU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t rdU16(const uint8_t* p) {
    return static_cast<uint16_t>(static_cast<uint16_t>(p[0]) |
                                 (static_cast<uint16_t>(p[1]) << 8));
}

bool tagIs(const uint8_t* p, const char* tag) {
    return std::memcmp(p, tag, 4) == 0;
}

// Reads a little-endian 32-bit value from `p` when there are 4 bytes there.
bool chunkU32(const uint8_t* p, size_t size, size_t at, uint32_t& out) {
    if (at + 4 > size) return false;
    out = rdU32(p + at);
    return true;
}

}  // namespace

bool decodeWav(const uint8_t* data, size_t size, SoundSample& out, std::string* errorOut) {
    if (size < 12 || !tagIs(data, "RIFF") || !tagIs(data + 8, "WAVE")) {
        if (errorOut != nullptr) *errorOut = "not a RIFF/WAVE payload";
        return false;
    }

    // Walk the chunks rather than assuming a 44-byte header. Files in the wild
    // carry `LIST`/`INFO` blocks, `fact` chunks and padding, and a reader that
    // starts at a fixed offset gets noise or a crash.
    int channels = 0;
    int sampleRate = 0;
    int bitsPerSample = 0;
    int formatTag = 1;  // 1 = PCM, 0xFFFE = extensible, 3 = float
    const uint8_t* audio = nullptr;
    size_t audioBytes = 0;

    size_t at = 12;
    while (at + 8 <= size) {
        const uint8_t* tag = data + at;
        uint32_t length = 0;
        if (!chunkU32(data, size, at + 4, length)) break;

        const size_t body = at + 8;
        if (tagIs(tag, "fmt ")) {
            if (body + 16 > size) break;
            formatTag = rdU16(data + body);
            channels = rdU16(data + body + 2);
            sampleRate = static_cast<int>(rdU32(data + body + 4));
            bitsPerSample = rdU16(data + body + 14);
        } else if (tagIs(tag, "data")) {
            // A `data` length larger than the file is what a truncated download
            // looks like, and it is worth taking what is there rather than
            // refusing the sound outright.
            const size_t available = size > body ? size - body : 0;
            audio = data + body;
            audioBytes = length < available ? length : available;
            break;
        }

        // Chunks are word-aligned, so an odd length is followed by a pad byte.
        at = body + length + (length & 1u);
    }

    if (audio == nullptr || channels <= 0 || sampleRate <= 0) {
        if (errorOut != nullptr) *errorOut = "no usable fmt/data pair";
        return false;
    }
    // 8- and 16-bit integer PCM are what MUGEN files carry. Anything else --
    // float, ADPCM, 24-bit -- is rare enough that converting it badly would be
    // worse than saying so.
    if (formatTag != 1 && formatTag != 0xFFFE) {
        if (errorOut != nullptr) *errorOut = "unsupported WAVE format tag";
        return false;
    }
    if (bitsPerSample != 8 && bitsPerSample != 16) {
        if (errorOut != nullptr) *errorOut = "unsupported bit depth";
        return false;
    }

    const int bytesPerSample = bitsPerSample / 8;
    const size_t count = audioBytes / static_cast<size_t>(bytesPerSample);
    if (count == 0) {
        if (errorOut != nullptr) *errorOut = "empty audio";
        return false;
    }

    out.channels = channels;
    out.sampleRate = sampleRate;
    out.pcm.resize(count);

    if (bitsPerSample == 16) {
        for (size_t i = 0; i < count; ++i) {
            // Unaligned by construction: a chunk length is not required to be
            // even, so the samples cannot be read through an int16_t*.
            out.pcm[i] = static_cast<int16_t>(rdU16(audio + i * 2));
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            // 8-bit WAVE is *unsigned*, centred on 128 -- the one place in the
            // format where the convention flips.
            out.pcm[i] = static_cast<int16_t>((static_cast<int>(audio[i]) - 128) << 8);
        }
    }
    return true;
}

bool SndBank::load(const FileData& data, std::string* errorOut) {
    // Deliberately not cleared: a bank accumulates, because a screenpack's
    // sounds are spread over two files and a character's are not.
    const uint8_t* bytes = data.data();
    const size_t size = data.size();
    if (size < kHeaderBytes || std::memcmp(bytes, kSignature, sizeof(kSignature)) != 0) {
        if (errorOut != nullptr) *errorOut = "not an ElecbyteSnd file";
        return false;
    }

    // The version is recorded from the file being read, so a bank assembled
    // from two reports the last one -- which is all the log needs it for.
    version_ = rdU16(bytes + 0x0C);
    const uint32_t count = rdU32(bytes + 0x10);
    uint32_t node = rdU32(bytes + 0x14);

    // A count larger than the file could possibly hold is a corrupt header; the
    // walk below also stops on a node it cannot read, so this is only a bound.
    if (count > size / kNodeBytes + 1) {
        if (errorOut != nullptr) *errorOut = "implausible sound count";
        return false;
    }

    for (uint32_t i = 0; i < count; ++i) {
        if (node == 0 || node + kNodeBytes > size) break;

        const uint32_t next = rdU32(bytes + node);
        const uint32_t length = rdU32(bytes + node + 4);
        const int group = static_cast<int>(rdU32(bytes + node + 8));
        const int number = static_cast<int>(rdU32(bytes + node + 12));

        const size_t body = node + kNodeBytes;
        const size_t available = size > body ? size - body : 0;
        const size_t payload = length < available ? length : available;

        SoundSample sample;
        sample.group = group;
        sample.number = number;
        std::string error;
        if (payload > 0 && decodeWav(bytes + body, payload, sample, &error)) {
            frames_ += sample.pcm.size();
            samples_.push_back(std::move(sample));
        } else {
            ++failed_;
            AF_LOGW("snd: group %d sample %d skipped: %s", group, number,
                    error.empty() ? "empty payload" : error.c_str());
        }

        // A node that points at or before itself is a loop, and following it
        // would be an infinite one.
        if (next <= node) break;
        node = next;
    }

    return true;
}

const SoundSample* SndBank::find(int group, int sample) const {
    for (const SoundSample& entry : samples_) {
        if (entry.group == group && entry.number == sample) return &entry;
    }
    return nullptr;
}

}  // namespace af

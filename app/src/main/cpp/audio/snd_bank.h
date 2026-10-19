// MUGEN's sound file (.snd), decoded.
//
// The format is the simplest of the three MUGEN containers, because it does not
// compress anything:
//
//     0x00  "ElecbyteSnd\0"        12 bytes
//     0x0C  version                2 bytes, 0 or 1
//     0x10  number of sounds       4 bytes
//     0x14  offset of the first    4 bytes
//
// and then a singly-linked list of 16-byte nodes, each immediately followed by
// its audio as a complete RIFF/WAVE file:
//
//     0x00  offset of the next node, 0 for the last
//     0x04  length of the WAVE payload in bytes
//     0x08  group number
//     0x0C  sample number
//
// The pair (group, sample) is the name a character uses: a .cns says
// `type = PlaySnd` / `value = 5, 0`, and that means group 5 sample 0. Groups
// are MUGEN's own buckets -- 0 is "common", 5 and 6 are the hit sounds, higher
// numbers are a character's own -- and nothing here assigns any meaning to
// them; the numbers are simply what the files use to talk to each other.
//
// Decoding is a real decode: the payloads are WAVs from every era of every tool
// that ever made one, so the parser walks the chunks rather than assuming a
// 44-byte header, takes the `fmt ` chunk's word rather than trusting a
// convention, and converts 8-bit unsigned samples to the signed 16-bit the
// mixer wants. There is no resampling here: each sample keeps the rate it was
// recorded at, and it is the mixer's job to reconcile that with the output.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/file_source.h"

namespace af {

// One sound: where it sits in the file's index, and its audio.
struct SoundSample {
    int group = 0;
    int number = 0;

    // Signed 16-bit PCM, interleaved at `channels` and `sampleRate`. Mono is
    // the common case and stereo is normal enough that both are kept.
    std::vector<int16_t> pcm;
    int channels = 1;
    int sampleRate = 44100;

    [[nodiscard]] bool empty() const { return pcm.empty(); }
    // How long it plays for, which is what the mixer needs to know when a clip
    // is still running.
    [[nodiscard]] double seconds() const {
        if (sampleRate <= 0 || channels <= 0) return 0.0;
        return static_cast<double>(pcm.size()) /
               (static_cast<double>(sampleRate) * static_cast<double>(channels));
    }
};

// Every sound in one .snd.
class SndBank {
public:
    // Decodes the file and *adds* what it holds to whatever the bank already
    // has.
    //
    // Adding rather than replacing is what lets one bank be assembled from
    // several files, and the screenpack needs exactly that: MUGEN puts the fight
    // screen's own sounds and the hit sounds every character shares in two
    // different files, and a lookup does not care which of them answered. A
    // character's bank is built from its one .snd, so for it the distinction
    // never comes up.
    //
    // A node whose WAVE payload will not parse is skipped rather than fatal -- a
    // character with one bad sound should still have the other twenty, and
    // silence is the right failure for audio.
    bool load(const FileData& data, std::string* errorOut);

    // The sound a character names as `value = group, sample`, or nullptr.
    [[nodiscard]] const SoundSample* find(int group, int sample) const;

    [[nodiscard]] size_t size() const { return samples_.size(); }
    [[nodiscard]] bool empty() const { return samples_.empty(); }
    // Total PCM frames held, which is the number that matters for memory.
    [[nodiscard]] size_t frames() const { return frames_; }
    [[nodiscard]] int version() const { return version_; }
    // Sounds whose payload failed to parse. Reported rather than hidden.
    [[nodiscard]] int failed() const { return failed_; }

private:
    std::vector<SoundSample> samples_;
    size_t frames_ = 0;
    int version_ = 0;
    int failed_ = 0;
};

// Decodes one RIFF/WAVE payload into 16-bit PCM. Exposed because the screenpack's
// sound files and a character's are the same bytes, and because a host test can
// check it against a hand-built WAVE without standing up a .snd.
bool decodeWav(const uint8_t* data, size_t size, SoundSample& out, std::string* errorOut);

}  // namespace af

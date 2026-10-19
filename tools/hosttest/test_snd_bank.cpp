// Host test for the .snd decoder (audio/snd_bank).
//
// The fixtures are real third-party .snd files, because that is the only thing
// that can catch a reader's mistakes here: our own files would share whatever
// the reader believes, and a MUGEN sound file is a WAVE from whatever tool the
// author had in 2004. The checks are the ones a wrong reader fails quietly:
//
//   * every sound the file indexes decodes, so a chunk walk that gives up early
//     shows up as a count;
//   * the sample rate and channel count come out of the `fmt ` chunk rather
//     than being assumed, which is what catches a fixed-offset reader;
//   * the (group, sample) pairs are the ones a character's .cns asks for;
//   * a truncated file loses sounds rather than crashing or inventing them.
//
// Build and run:
//   clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//       -o test_snd_bank tools/hosttest/test_snd_bank.cpp \
//       app/src/main/cpp/audio/snd_bank.cpp app/src/main/cpp/core/ini.cpp
//   ./test_snd_bank local_chars/kfm/kfm.snd [more.snd ...]

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "audio/snd_bank.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++failures;
}

std::vector<uint8_t> readFile(const std::string& path, bool* ok) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        *ok = false;
        return {};
    }
    std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    *ok = true;
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
}

// A hand-built 8-bit mono WAVE, so the unsigned-to-signed conversion and the
// chunk walk are checked against bytes rather than against a file that happens
// to work.
std::vector<uint8_t> makeWav8(int sampleCount) {
    const int rate = 8000;
    const int dataBytes = sampleCount;
    std::vector<uint8_t> out;
    auto u32 = [&out](uint32_t v) {
        out.push_back(static_cast<uint8_t>(v & 0xFF));
        out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    };
    auto u16 = [&out](uint16_t v) {
        out.push_back(static_cast<uint8_t>(v & 0xFF));
        out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    };
    auto tag = [&out](const char* t) {
        for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(t[i]));
    };

    tag("RIFF");
    u32(static_cast<uint32_t>(4 + 8 + 16 + 8 + dataBytes + (dataBytes & 1)));
    tag("WAVE");
    // A `LIST` chunk before `fmt ` is exactly the sort of thing a fixed-offset
    // reader trips over, so the fixture has one.
    tag("LIST");
    u32(4);
    tag("INFO");
    tag("fmt ");
    u32(16);  // the chunk size is four bytes, like every other chunk size
    u16(1);   // PCM
    u16(1);  // mono
    u32(static_cast<uint32_t>(rate));
    u32(static_cast<uint32_t>(rate));  // byte rate
    u16(1);                            // block align
    u16(8);                            // bits
    tag("data");
    u32(static_cast<uint32_t>(dataBytes));
    // Silence at the unsigned midpoint, so a correct decode is all zeros and a
    // missing -128 offset is a loud buzz.
    for (int i = 0; i < sampleCount; ++i) out.push_back(128);
    if (dataBytes & 1) out.push_back(0);  // the pad byte
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: %s <file.snd> [more.snd ...]\n", argv[0]);
        return 2;
    }

    // ------------------------------------------------- the WAVE decoder first
    {
        const std::vector<uint8_t> wav = makeWav8(64);
        af::SoundSample sample;
        std::string error;
        check(af::decodeWav(wav.data(), wav.size(), sample, &error),
              "a WAVE with a LIST chunk before fmt decodes");
        check(sample.channels == 1, "the channel count comes from the fmt chunk");
        check(sample.sampleRate == 8000, "the sample rate comes from the fmt chunk");
        check(sample.pcm.size() == 64, "every frame is decoded");
        bool silent = true;
        for (const int16_t value : sample.pcm) {
            if (value != 0) silent = false;
        }
        check(silent, "8-bit unsigned midpoint decodes to digital silence, not a buzz");

        af::SoundSample truncated;
        std::string truncErr;
        check(af::decodeWav(wav.data(), 20, truncated, &truncErr) == false,
              "a payload cut off before its data chunk is refused rather than guessed at");
        af::SoundSample notWav;
        std::string notWavErr;
        const uint8_t junk[16] = {'n', 'o', 'p', 'e', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        check(af::decodeWav(junk, sizeof(junk), notWav, &notWavErr) == false,
              "a payload that is not a WAVE is refused");
    }

    // ------------------------------------------------------------- real files
    for (int i = 1; i < argc; ++i) {
        const std::string path = argv[i];
        bool ok = false;
        const std::vector<uint8_t> bytes = readFile(path, &ok);
        if (!ok) {
            std::printf("FAIL: cannot read %s\n", path.c_str());
            ++failures;
            continue;
        }

        af::FileData data(path, bytes);
        af::SndBank bank;
        std::string error;
        if (!bank.load(data, &error)) {
            std::printf("FAIL: %s: %s\n", path.c_str(), error.c_str());
            ++failures;
            continue;
        }

        std::printf("%-40s %zu sounds, %d failed, %zu frames, v%d\n", path.c_str(), bank.size(),
                    bank.failed(), bank.frames(), bank.version());
        check(bank.size() > 0, path + ": every sound in the file should decode");
        check(bank.failed() == 0, path + ": no sound in the file should fail to decode");
        check(bank.frames() > 0, path + ": the file should hold some audio");

        // Every sound has to be usable: a rate, a channel count, and something
        // to play. A zero-length clip is what a mis-read `data` length looks
        // like, and it would be silent in a way nobody would trace back here.
        // Walk the pairs a character could ask for. The bank has no iteration
        // of its own -- a lookup is the only way in, which is what a character
        // does anyway -- so this sweeps the ranges the files actually use:
        // MUGEN's common groups are 0 (the character's own), 5 and 6 (hits),
        // 11 (voice) and a few screenpack ones above that.
        size_t shortest = SIZE_MAX;
        int zeroLength = 0;
        for (int group = 0; group <= 900; ++group) {
            for (int number = 0; number < 8; ++number) {
                const af::SoundSample* sample = bank.find(group, number);
                if (sample == nullptr) continue;
                if (sample->pcm.size() < shortest) shortest = sample->pcm.size();
                if (sample->pcm.empty()) ++zeroLength;
                check(sample->sampleRate > 0, path + ": a sound with no sample rate");
                check(sample->channels == 1 || sample->channels == 2,
                      path + ": a sound with an impossible channel count");
                check(sample->seconds() > 0.0, path + ": a sound that is zero seconds long");
            }
        }
        check(zeroLength == 0, path + ": no sound should decode to zero frames");
        check(shortest != SIZE_MAX, path + ": at least one (group, sample) should resolve");
        // A bank has to carry one of the two things a .cns actually names:
        // group 0 sample 0, which is a character's own first sound, or group 5
        // sample 0, the light hit every HitDef in the format asks for. A file
        // with neither is a file that would never be heard.
        check(bank.find(0, 0) != nullptr || bank.find(5, 0) != nullptr,
              path + ": should hold either the character's own sounds or the hit sounds");

        // ------------------------------------------------------ truncated file
        //
        // Cutting the file in half should lose sounds, not crash and not invent
        // them. This is the check that a walk reading past the end of the
        // buffer fails.
        af::FileData cut(path, std::vector<uint8_t>(bytes.begin(), bytes.begin() + bytes.size() / 2));
        af::SndBank partial;
        std::string partialError;
        if (!partial.load(cut, &partialError)) {
            std::printf("  (a truncated file was refused outright: %s)\n", partialError.c_str());
            check(false, path + ": a truncated file should still load what it holds");
        } else {
            std::printf("  truncated to %zu bytes: %zu of %zu sounds survived\n", bytes.size() / 2,
                        partial.size(), bank.size());
            check(partial.size() <= bank.size(),
                  path + ": a truncated file cannot have more sounds than the whole one");
        }
    }

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

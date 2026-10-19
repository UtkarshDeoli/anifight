// Host test for the mixer's bank-scoped full command and the storyboard unit
// conversions.
//
// The piece being pinned is the one the storyboard screen needs and could not
// previously express: a sound that must come from *this* bank (so a storyboard's
// group 0 sample 0 is not answered by the screenpack's menu blip) and that
// carries volume and pan at the same time. `playFrom` had no room for the
// parameters; `playFull` had the parameters but searched every registered bank
// in priority order. `playFromFull` is both.
//
// The checks are behavioural rather than structural: a hand-built .snd goes
// through the real decoder, the mixer is driven by `fill()` exactly as the audio
// thread drives it, and the rendered PCM is measured. That catches a command
// that is queued with the right fields but dropped, or one whose bank slot is
// set but ignored.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//         -o /tmp/test_mixer tools/hosttest/test_mixer.cpp \
//         app/src/main/cpp/audio/mixer.cpp \
//         app/src/main/cpp/audio/snd_bank.cpp
//     /tmp/test_mixer

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "audio/mixer.h"
#include "audio/snd_bank.h"
#include "core/file_source.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

void checkNear(double actual, double expected, double tolerance, const char* what) {
    if (std::fabs(actual - expected) <= tolerance) return;
    std::fprintf(stderr, "FAIL: %s (expected %.5f, got %.5f)\n", what, expected, actual);
    ++failures;
}

void putU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void putU32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void putTag(std::vector<uint8_t>& out, const char* tag) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(tag[i]));
}

// A 16-bit mono WAVE of a constant value, so the rendered RMS is exactly the
// amplitude times every gain in the path and a comparison between two runs is
// meaningful.
std::vector<uint8_t> makeWav16(int frames, int16_t amplitude, int sampleRate) {
    const uint32_t dataBytes = static_cast<uint32_t>(frames) * 2;
    std::vector<uint8_t> out;
    putTag(out, "RIFF");
    putU32(out, 36 + dataBytes);
    putTag(out, "WAVE");
    putTag(out, "fmt ");
    putU32(out, 16);
    putU16(out, 1);                                  // PCM
    putU16(out, 1);                                  // mono
    putU32(out, static_cast<uint32_t>(sampleRate));  // sample rate
    putU32(out, static_cast<uint32_t>(sampleRate) * 2);  // byte rate
    putU16(out, 2);                                  // block align
    putU16(out, 16);                                 // bits
    putTag(out, "data");
    putU32(out, dataBytes);
    for (int i = 0; i < frames; ++i) putU16(out, static_cast<uint16_t>(amplitude));
    return out;
}

// One-sound ElecbyteSnd container, so the test drives the real bank decoder
// instead of reaching into SndBank's private sample list.
std::vector<uint8_t> makeSnd(int group, int sample, const std::vector<uint8_t>& wav) {
    constexpr uint32_t kFirstNode = 0x18;
    std::vector<uint8_t> out;
    const char signature[12] = {'E', 'l', 'e', 'c', 'b', 'y', 't', 'e',
                                'S', 'n', 'd', '\0'};
    for (char c : signature) out.push_back(static_cast<uint8_t>(c));
    putU16(out, 0);                       // version at 0x0C
    putU16(out, 0);                       // two bytes of padding: count is at 0x10
    putU32(out, 1);                       // sound count
    putU32(out, kFirstNode);              // first node
    putU32(out, 0);                       // next node
    putU32(out, static_cast<uint32_t>(wav.size()));
    putU32(out, static_cast<uint32_t>(group));
    putU32(out, static_cast<uint32_t>(sample));
    out.insert(out.end(), wav.begin(), wav.end());
    return out;
}

bool loadBank(af::SndBank& bank, int group, int sample, int16_t amplitude, int sampleRate) {
    const std::vector<uint8_t> bytes =
        makeSnd(group, sample, makeWav16(2000, amplitude, sampleRate));
    af::FileData data("synthetic.snd", bytes);
    std::string error;
    if (bank.load(data, &error)) return true;
    std::fprintf(stderr, "FAIL: synthetic bank did not load: %s\n", error.c_str());
    ++failures;
    return false;
}

double rms(const std::vector<int16_t>& pcm, int channel) {
    double sum = 0.0;
    const size_t frames = pcm.size() / 2;
    for (size_t i = 0; i < frames; ++i) {
        const double value = pcm[channel == 0 ? i * 2 : i * 2 + 1];
        sum += value * value;
    }
    return frames == 0 ? 0.0 : std::sqrt(sum / static_cast<double>(frames));
}

struct Rendered {
    double left = 0.0;
    double right = 0.0;
};

Rendered render(af::Mixer& mixer, int frames, bool* started) {
    std::vector<int16_t> pcm(static_cast<size_t>(frames) * 2, 0);
    mixer.fill(pcm.data(), frames);
    if (started != nullptr) *started = mixer.started() > 0;
    return Rendered{rms(pcm, 0), rms(pcm, 1)};
}

af::SndCommand commandFor(int group, int sample, float volume, float pan) {
    af::SndCommand command;
    command.group = group;
    command.sample = sample;
    command.volume = volume;
    command.pan = pan;
    return command;
}

}  // namespace

int main() {
    constexpr int kRate = 8000;
    constexpr int kFrames = 200;

    // ------------------------------------------------------ unit conversions
    //
    // The reference's storyboard section: volumescale is a percentage (100 =
    // authored, 50 = half) with no support for making a sound louder, and pan is
    // an integer in -127..127 with 0 centre.
    checkNear(af::volumeScaleToGain(100.0f), 1.0, 1e-6, "volumescale 100 is unity gain");
    checkNear(af::volumeScaleToGain(50.0f), 0.5, 1e-6, "volumescale 50 is half gain");
    checkNear(af::volumeScaleToGain(0.0f), 0.0, 1e-6, "volumescale 0 is silence");
    checkNear(af::volumeScaleToGain(150.0f), 1.0, 1e-6, "volumescale above 100 clamps");
    checkNear(af::volumeScaleToGain(-20.0f), 0.0, 1e-6, "a negative volumescale clamps to 0");

    checkNear(af::storyboardPanToMixer(0), 0.0, 1e-6, "pan 0 is centre");
    checkNear(af::storyboardPanToMixer(127), 1.0, 1e-6, "pan 127 is full right");
    checkNear(af::storyboardPanToMixer(-127), -1.0, 1e-6, "pan -127 is full left");
    checkNear(af::storyboardPanToMixer(64), 64.0 / 127.0, 1e-6, "pan scales on 127");
    checkNear(af::storyboardPanToMixer(400), 1.0, 1e-6, "out-of-range pan clamps right");
    checkNear(af::storyboardPanToMixer(-400), -1.0, 1e-6, "out-of-range pan clamps left");

    // ------------------------------------------------- volume reaches a voice
    af::SndBank bank;
    if (!loadBank(bank, 0, 0, 20000, kRate)) return 1;
    check(bank.find(0, 0) != nullptr, "the synthetic bank resolves its sound");

    af::Mixer full;
    full.setOutputRate(kRate);
    bool started = false;
    full.playFromFull(bank, commandFor(0, 0, 1.0f, 0.0f));
    const Rendered loud = render(full, kFrames, &started);
    check(started, "playFromFull queues a command the audio thread starts");
    check(full.misses() == 0, "a sound that resolves is not a miss");
    check(loud.left > 1000.0, "the rendered sound is not silent");

    af::Mixer half;
    half.setOutputRate(kRate);
    half.playFromFull(bank, commandFor(0, 0, 0.5f, 0.0f));
    const Rendered quiet = render(half, kFrames, nullptr);
    checkNear(quiet.left / loud.left, 0.5, 0.01, "volume 0.5 halves the rendered level");
    checkNear(quiet.right / loud.right, 0.5, 0.01, "volume 0.5 halves both channels");

    af::Mixer silent;
    silent.setOutputRate(kRate);
    silent.playFromFull(bank, commandFor(0, 0, 0.0f, 0.0f));
    const Rendered mute = render(silent, kFrames, nullptr);
    checkNear(mute.left, 0.0, 1e-9, "volume 0 renders silence");

    // ---------------------------------------------------- pan reaches a voice
    //
    // The mixer's pan law keeps the near ear at unity and halves the far ear at
    // full deflection, so a hard-left sound has a 2:1 left/right ratio.
    af::Mixer left;
    left.setOutputRate(kRate);
    left.playFromFull(bank, commandFor(0, 0, 1.0f, -1.0f));
    const Rendered pannedLeft = render(left, kFrames, nullptr);
    checkNear(pannedLeft.left / pannedLeft.right, 2.0, 0.02, "pan -1 attenuates the right ear");

    af::Mixer right;
    right.setOutputRate(kRate);
    right.playFromFull(bank, commandFor(0, 0, 1.0f, 1.0f));
    const Rendered pannedRight = render(right, kFrames, nullptr);
    checkNear(pannedRight.right / pannedRight.left, 2.0, 0.02, "pan +1 attenuates the left ear");

    // ------------------------------------------------------- bank scoping
    //
    // Two banks answer the same (group, sample) with different amplitudes. A
    // plain playFull would take the first one in registration order; the
    // bank-scoped command has to take the named one even though the other was
    // registered first.
    af::SndBank firstBank;   // registered first, amplitude 20000
    af::SndBank namedBank;   // amplitude 4000, and the one we ask for
    if (!loadBank(firstBank, 5, 0, 20000, kRate)) return 1;
    if (!loadBank(namedBank, 5, 0, 4000, kRate)) return 1;

    af::Mixer scoped;
    scoped.setOutputRate(kRate);
    scoped.addBank(&firstBank);
    scoped.playFromFull(namedBank, commandFor(5, 0, 1.0f, 0.0f));
    const Rendered scopedRender = render(scoped, kFrames, nullptr);
    check(scoped.bankCount() == 2, "playFromFull registers a bank it has not seen");
    checkNear(scopedRender.left / loud.left, 4000.0 / 20000.0, 0.02,
              "playFromFull plays the named bank, not the first registered one");

    // A plain playFull with the same two banks resolves to the first one, which
    // is the behaviour the scoped command exists to override.
    af::Mixer unscoped;
    unscoped.setOutputRate(kRate);
    unscoped.addBank(&firstBank);
    unscoped.addBank(&namedBank);
    unscoped.playFull(commandFor(5, 0, 1.0f, 0.0f));
    const Rendered unscopedRender = render(unscoped, kFrames, nullptr);
    checkNear(unscopedRender.left / loud.left, 1.0, 0.02,
              "playFull still resolves in registration priority order");

    // ------------------------------------------------------- frequency too
    //
    // freqMul rides in the same command. The synthetic sample is 2000 frames
    // long, so at the output rate a voice at 1.0x is finished after 2000 output
    // frames and a voice at 0.5x is only halfway through. Rendering past that
    // point separates the two: the fast voice is silent and the slowed one is
    // not, which proves the multiplier survived the bank-scoped queue.
    af::Mixer normalSpeed;
    normalSpeed.setOutputRate(kRate);
    normalSpeed.playFromFull(bank, commandFor(0, 0, 1.0f, 0.0f));
    render(normalSpeed, 2000, nullptr);  // consume the whole clip
    const Rendered afterNormal = render(normalSpeed, 400, nullptr);
    checkNear(afterNormal.left, 0.0, 1e-9, "a 1.0x voice is done after its clip length");

    af::Mixer halfSpeed;
    halfSpeed.setOutputRate(kRate);
    af::SndCommand slow = commandFor(0, 0, 1.0f, 0.0f);
    slow.freqMul = 0.5f;
    halfSpeed.playFromFull(bank, slow);
    render(halfSpeed, 2000, nullptr);
    const Rendered afterSlow = render(halfSpeed, 400, nullptr);
    check(afterSlow.left > 1000.0, "a bank-scoped command keeps its frequency multiplier");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

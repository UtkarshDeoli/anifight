// The device end of the audio: a buffer queue that pulls from the mixer.
//
// OpenSL ES rather than AAudio, and that is one line of reasoning: AAudio is
// API 26 and this app's `minSdk` is 24. OpenSL ES is deprecated and has been
// since Android 8, but it is present on every device the game can run on, and a
// buffer queue player is the same forty lines of setup it always was.
//
// The shape is a pull: the platform asks for a block whenever it is short of
// one, on its own thread, and the callback hands it whatever the mixer has. Four
// blocks are kept in flight, which is about ninety milliseconds of slack --
// enough that a frame of the game taking twice as long as it should cannot empty
// the queue and be heard as a click.
#pragma once

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <array>
#include <cstdint>

namespace af {

class Mixer;

class AudioOut {
public:
    AudioOut() = default;
    ~AudioOut();
    AudioOut(const AudioOut&) = delete;
    AudioOut& operator=(const AudioOut&) = delete;

    // Opens the device and starts pulling. Returns false when there is no audio
    // device or it refuses to start -- one whose audio HAL is busy, or an
    // emulator built without sound. The game runs either way; it is just silent,
    // and the log says why.
    bool start(Mixer& mixer, int sampleRate);
    void stop();

    [[nodiscard]] bool running() const { return playerObject_ != nullptr; }
    // Blocks the platform has asked for and been given. Reported in the log,
    // because a count that stands still is the difference between "silent" and
    // "not playing at all".
    [[nodiscard]] uint64_t blocksServed() const { return served_; }

private:
    // 1024 frames is ~23ms at 44.1kHz: small enough that a sound starts without
    // being noticed as late, large enough not to wake the platform constantly.
    static constexpr int kBlockFrames = 1024;
    static constexpr int kBlocksInFlight = 4;

    static void onBufferQueue(SLAndroidSimpleBufferQueueItf queue, void* context);
    void serveBlock();

    Mixer* mixer_ = nullptr;
    // Interleaved stereo, one block per slot.
    std::array<std::array<int16_t, kBlockFrames * 2>, kBlocksInFlight> buffers_{};
    int nextBlock_ = 0;
    uint64_t served_ = 0;

    SLObjectItf engineObject_ = nullptr;
    SLEngineItf engine_ = nullptr;
    SLObjectItf outputMixObject_ = nullptr;
    SLObjectItf playerObject_ = nullptr;
    SLPlayItf play_ = nullptr;
    SLAndroidSimpleBufferQueueItf queue_ = nullptr;
};

}  // namespace af

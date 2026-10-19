// Playing a sound: the voice pool, and the device under it.
//
// There are two threads involved and they meet in exactly one place. The game
// thread says "play group 5 sample 0" from inside a state machine; the audio
// thread asks for the next block of samples from inside a device callback. What
// passes between them is a fixed-size ring of two integers and nothing else --
// no allocation, no locking, and no call back into the game from the audio
// thread, which is the rule that keeps an audio callback from stuttering.
//
// The voices live on the audio thread's side of that ring, so only one thread
// ever touches them and there is no race to reason about: `play` writes a
// command, the audio thread reads it and starts a voice. The banks are
// registered before the device starts and never written afterwards, which is why
// the audio thread can read that list without a lock.
#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace af {

class SndBank;

// Where a state machine asks for a sound.
//
// An interface rather than a call straight into the mixer, so that a machine can
// be built with no audio at all -- the host tests do exactly that, and a null
// sink turns "play this" into a no-op instead of a link error.
//
// The full controller shape (channel/loop/volume/pan/rate) rides in `playFull`;
// sinks that predate it degrade to plain play() because a sound played slightly
// wrong beats a sound not played.
struct SndCommand {
    int group = 0;
    int sample = 0;
    // MUGEN's channel: one exclusive slot per character. -1 is "any free voice".
    int channel = -1;
    bool loop = false;
    float volume = 1.0f;   // 1.0 = as authored
    float pan = 0.0f;      // -1 full left .. +1 full right
    float freqMul = 1.0f;  // playback rate multiplier
};

// The authored units a MUGEN file writes, converted to the units above.
//
// `volumescale` is a percentage: 100 is the level the sample was recorded at,
// 50 is half. The reference's storyboard section says there is currently no
// support for making a sound louder, and PlaySnd's documented range is 0..100,
// so values above 100 clamp rather than amplifying past the authored level.
[[nodiscard]] inline float volumeScaleToGain(float volumeScale) {
    return std::clamp(volumeScale / 100.0f, 0.0f, 1.0f);
}

// `soundN.pan` is an integer in -127..127, 0 centre, negative left; the
// reference's storyboard documentation gives that range explicitly. MUGEN's
// PlaySnd `pan` is a position in pixels and is a different unit, so this
// conversion is named for the storyboard key rather than for pan in general.
[[nodiscard]] inline float storyboardPanToMixer(int pan) {
    return std::clamp(static_cast<float>(pan) / 127.0f, -1.0f, 1.0f);
}

class SoundSink {
public:
    virtual ~SoundSink() = default;
    // The pair a .cns names as `value = 5, 0`.
    virtual void play(int group, int sample) = 0;
    virtual void playFull(const SndCommand& command) { play(command.group, command.sample); }
    // `StopSnd`: silence everything the caller has sounding on that channel.
    virtual void stopChannel(int channel) { (void)channel; }
    // `SndPan`: retune a playing channel.
    virtual void panChannel(int channel, float pan) {
        (void)channel;
        (void)pan;
    }
};

// The voice pool and the mixing, with no device attached.
class Mixer : public SoundSink {
public:
    // Enough for a busy exchange plus the hit sounds on top of it. A voice
    // stolen by a newer sound is the oldest one, which is what a fighting game
    // wants: the newest hit is the one that matters.
    static constexpr int kVoices = 24;
    // Commands the game thread may queue ahead of the audio thread. One
    // frame of game time is 1/60s, so this is seconds of slack.
    static constexpr int kCommandSlots = 64;

    // Distinct (group, sample) pairs reported as missing, so a bank that is
    // simply absent does not fill the log with the same line.
    static constexpr int kReportedMisses = 8;

    Mixer();
    ~Mixer();
    Mixer(const Mixer&) = delete;
    Mixer& operator=(const Mixer&) = delete;

    // Registers a bank to look sounds up in, in priority order: a fighter's own
    // sound file first, then the screenpack's, which is where MUGEN keeps the
    // hit and guard sounds every character shares.
    //
    // Safe to call while the device is running, which is what lets the banks be
    // registered when a match *starts* rather than for the whole roster up
    // front. The slot is written before the count is published, so the audio
    // thread sees either the old list or a list with a fully-formed entry in it
    // -- never a half-written one.
    void addBank(const SndBank* bank);
    [[nodiscard]] int bankCount() const { return banksUsed_.load(std::memory_order_acquire); }
    // How many lookups found nothing. Reported rather than silent, because a
    // character whose sounds never play looks exactly like a mixer that does
    // not work.
    [[nodiscard]] uint64_t misses() const { return misses_.load(); }
    [[nodiscard]] uint64_t started() const { return started_.load(); }

    void play(int group, int sample) override;
    void playFull(const SndCommand& command) override;
    void stopChannel(int channel) override;
    void panChannel(int channel, float pan) override;

    // Plays from one bank and no other.
    //
    // The screen's own sounds and a character's are two different things that use
    // the same numbers: group 0 sample 0 is a menu blip in fight.snd and a
    // character's first swing in its own .snd. A single priority list would
    // answer the second with the first -- a punch that clicks like a menu -- so
    // the UI asks for its own bank by name and the character path never sees it.
    void playFrom(const SndBank& bank, int group, int sample);

    // Plays from one bank with the full controller shape. `playFrom` is the
    // group/sample-only form and `playFull` searches every registered bank in
    // priority order; a caller that knows exactly which bank a sound belongs to
    // and also needs volume/pan/frequency uses this one.
    void playFromFull(const SndBank& bank, const SndCommand& command);

    // Fills `frames` stereo frames of signed 16-bit PCM at the output rate.
    // Called on the audio thread; safe to call with nothing playing.
    void fill(int16_t* out, int frames);

    // The rate the mixer renders at. Voices recorded at another rate are
    // resampled to this one, which is why it is set once and not per sound.
    void setOutputRate(int rate) { outputRate_ = rate > 0 ? rate : 44100; }
    [[nodiscard]] int outputRate() const { return outputRate_; }

    // How many frames have been rendered in total, for a log line that says the
    // audio path is alive.
    [[nodiscard]] uint64_t rendered() const { return rendered_.load(); }

private:
    struct Command {
        int group = 0;
        int sample = 0;
        // Which bank to use, or -1 to search the list in priority order.
        int bankSlot = -1;
        int channel = -1;      // -1 = any voice; >=0 steals that channel's voice
        bool loop = false;
        float volume = 1.0f;
        float pan = 0.0f;
        float freqMul = 1.0f;
        // The StopSnd/SndPan commands write these instead of a play.
        bool stop = false;
        bool repan = false;
    };

    // A voice is only ever touched by the audio thread, so it needs no
    // synchronisation of its own.
    struct Voice {
        const int16_t* pcm = nullptr;
        size_t frames = 0;      // frames in the clip, not samples
        int channels = 1;
        double position = 0.0;  // in source frames
        double step = 1.0;      // source frames per output frame
        float gain = 1.0f;
        bool active = false;
        int channel = -1;
        bool loop = false;
        float pan = 0.0f;
    };

    // Pops one command, if there is one. Consumer side of the ring.
    bool takeCommand(Command& out);
    // Registers `bank` if it is not already there, and reports its slot, or -1.
    int slotFor(const SndBank& bank);
    void startVoice(const Command& command);
    const class SndBank* findBank(int group, int sample) const;

    Voice voices_[kVoices];
    int nextVoice_ = 0;

    const SndBank* banks_[8] = {};
    std::atomic<int> banksUsed_{0};

    // Single producer: the game thread. Single consumer: the audio thread.
    Command commands_[kCommandSlots];
    std::atomic<uint32_t> commandWrite_{0};
    std::atomic<uint32_t> commandRead_{0};

    std::atomic<uint64_t> misses_{0};
    // Only touched by the audio thread, which is why it needs no lock.
    int reportedMisses_ = 0;
    int reportedPairs_[kReportedMisses][2] = {};
    std::atomic<uint64_t> started_{0};
    std::atomic<uint64_t> rendered_{0};
    int outputRate_ = 44100;
    float masterGain_ = 0.7f;
};

}  // namespace af

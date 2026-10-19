#include "audio/mixer.h"

#include <algorithm>
#include <cmath>

#include "audio/snd_bank.h"
#include "core/log.h"

namespace af {

Mixer::Mixer() = default;

Mixer::~Mixer() = default;

void Mixer::addBank(const SndBank* bank) {
    if (bank == nullptr || bank->empty()) return;
    const int used = banksUsed_.load(std::memory_order_relaxed);
    if (used >= static_cast<int>(std::size(banks_))) return;
    // A bank with nothing in it is not registered: a character whose archive
    // dropped its .snd would otherwise take a slot and answer no lookups.
    for (int i = 0; i < used; ++i) {
        if (banks_[i] == bank) return;  // registering the same bank twice
    }
    banks_[used] = bank;
    banksUsed_.store(used + 1, std::memory_order_release);
}

const SndBank* Mixer::findBank(int group, int sample) const {
    const int used = banksUsed_.load(std::memory_order_acquire);
    for (int i = 0; i < used; ++i) {
        if (banks_[i] != nullptr && banks_[i]->find(group, sample) != nullptr) return banks_[i];
    }
    return nullptr;
}

int Mixer::slotFor(const SndBank& bank) {
    const int used = banksUsed_.load(std::memory_order_acquire);
    for (int i = 0; i < used; ++i) {
        if (banks_[i] == &bank) return i;
    }
    return -1;
}

void Mixer::playFrom(const SndBank& bank, int group, int sample) {
    addBank(&bank);
    const int slot = slotFor(bank);
    if (slot < 0) return;  // no room for another bank

    // The producer side of the ring. The slot is written first and the index
    // published after it, so the audio thread can never read a half-written
    // command. A full ring drops the command rather than blocking: a lost
    // sound is a missed noise, and a game thread that waits for the audio
    // thread is a frame that stutters.
    const uint32_t write = commandWrite_.load(std::memory_order_relaxed);
    const uint32_t read = commandRead_.load(std::memory_order_acquire);
    if (write - read >= static_cast<uint32_t>(kCommandSlots)) {
        misses_.fetch_add(1);
        return;
    }
    commands_[write % kCommandSlots] = Command{group, sample, slot};
    commandWrite_.store(write + 1, std::memory_order_release);
}

void Mixer::play(int group, int sample) {
    SndCommand command;
    command.group = group;
    command.sample = sample;
    command.channel = -1;
    playFull(command);
}

void Mixer::playFromFull(const SndBank& bank, const SndCommand& command) {
    addBank(&bank);
    const int slot = slotFor(bank);
    if (slot < 0) return;  // no room for another bank

    // Same producer side of the ring as playFull and playFrom: the whole
    // command is written first and the index published after it, so the audio
    // thread never sees a half-written entry.
    const uint32_t write = commandWrite_.load(std::memory_order_relaxed);
    const uint32_t read = commandRead_.load(std::memory_order_acquire);
    if (write - read >= static_cast<uint32_t>(kCommandSlots)) {
        misses_.fetch_add(1);
        return;
    }
    Command wire;
    wire.group = command.group;
    wire.sample = command.sample;
    wire.bankSlot = slot;
    wire.channel = command.channel;
    wire.loop = command.loop;
    wire.volume = command.volume;
    wire.pan = command.pan;
    wire.freqMul = command.freqMul;
    commands_[write % kCommandSlots] = wire;
    commandWrite_.store(write + 1, std::memory_order_release);
}

void Mixer::playFull(const SndCommand& command) {
    const uint32_t write = commandWrite_.load(std::memory_order_relaxed);
    const uint32_t read = commandRead_.load(std::memory_order_acquire);
    if (write - read >= static_cast<uint32_t>(kCommandSlots)) {
        misses_.fetch_add(1);
        return;
    }
    Command wire;
    wire.group = command.group;
    wire.sample = command.sample;
    wire.bankSlot = -1;
    wire.channel = command.channel;
    wire.loop = command.loop;
    wire.volume = command.volume;
    wire.pan = command.pan;
    wire.freqMul = command.freqMul;
    commands_[write % kCommandSlots] = wire;
    commandWrite_.store(write + 1, std::memory_order_release);
}

void Mixer::stopChannel(int channel) {
    const uint32_t write = commandWrite_.load(std::memory_order_relaxed);
    const uint32_t read = commandRead_.load(std::memory_order_acquire);
    if (write - read >= static_cast<uint32_t>(kCommandSlots)) return;
    Command wire;
    wire.channel = channel;
    wire.stop = true;
    commands_[write % kCommandSlots] = wire;
    commandWrite_.store(write + 1, std::memory_order_release);
}

void Mixer::panChannel(int channel, float pan) {
    const uint32_t write = commandWrite_.load(std::memory_order_relaxed);
    const uint32_t read = commandRead_.load(std::memory_order_acquire);
    if (write - read >= static_cast<uint32_t>(kCommandSlots)) return;
    Command wire;
    wire.channel = channel;
    wire.pan = pan;
    wire.repan = true;
    commands_[write % kCommandSlots] = wire;
    commandWrite_.store(write + 1, std::memory_order_release);
}

bool Mixer::takeCommand(Command& out) {
    const uint32_t read = commandRead_.load(std::memory_order_relaxed);
    if (read == commandWrite_.load(std::memory_order_acquire)) return false;
    out = commands_[read % kCommandSlots];
    commandRead_.store(read + 1, std::memory_order_release);
    return true;
}

void Mixer::startVoice(const Command& command) {
    // Channel commands first: StopSnd silences the slot, SndPan retunes it, and
    // a play onto an occupied channel replaces it -- one channel, one sound, as
    // MUGEN's per-player channel slots read.
    if (command.stop) {
        for (Voice& voice : voices_) {
            if (voice.active && voice.channel == command.channel) voice.active = false;
        }
        return;
    }
    if (command.repan) {
        for (Voice& voice : voices_) {
            if (voice.active && voice.channel == command.channel) voice.pan = command.pan;
        }
        return;
    }

    // A command that named a bank uses it and nothing else; one that did not
    // searches the list in priority order, which is how a character's own sounds
    // win over the shared ones.
    const SndBank* bank = command.bankSlot >= 0 && command.bankSlot < banksUsed_.load()
                              ? banks_[command.bankSlot]
                              : findBank(command.group, command.sample);
    if (bank == nullptr) {
        misses_.fetch_add(1);
        // Named once each. Most of this roster is packaged without its .snd --
        // the sound file is the first thing an archive drops -- so a character's
        // own sounds are missing rather than broken, and the group tells you
        // which of the two it is: 5 and 6 come from the screenpack and *should*
        // resolve, anything else is the character's own and may legitimately not
        // exist.
        bool seen = false;
        for (int i = 0; i < reportedMisses_; ++i) {
            if (reportedPairs_[i][0] == command.group && reportedPairs_[i][1] == command.sample) {
                seen = true;
                break;
            }
        }
        if (!seen && reportedMisses_ < kReportedMisses) {
            reportedPairs_[reportedMisses_][0] = command.group;
            reportedPairs_[reportedMisses_][1] = command.sample;
            ++reportedMisses_;
            AF_LOGI("audio: no sound for group %d sample %d", command.group, command.sample);
        }
        return;
    }
    const SoundSample* sample = bank->find(command.group, command.sample);
    if (sample == nullptr || sample->pcm.empty() || sample->channels <= 0) return;

    // A play onto a named channel takes that channel's voice; otherwise the pool
    // rotates, and a looping voice is only ever stolen by the same rule -- the
    // newest hit is the one that matters.
    Voice* voice = nullptr;
    if (command.channel >= 0) {
        for (Voice& v : voices_) {
            if (v.channel == command.channel) {
                voice = &v;
                break;
            }
        }
    }
    if (voice == nullptr) {
        voice = &voices_[nextVoice_];
        nextVoice_ = (nextVoice_ + 1) % kVoices;
    }

    const size_t totalFrames = sample->pcm.size() / static_cast<size_t>(sample->channels);
    voice->pcm = sample->pcm.data();
    voice->frames = totalFrames;
    voice->channels = sample->channels;
    voice->position = 0.0;
    // One step of the reader per frame of the output, adjusted for a clip
    // recorded at another rate. Nothing here resamples for quality: a game
    // sound is a tenth of a second long and the difference between linear and
    // nearest is not audible on one.
    voice->step = static_cast<double>(sample->sampleRate) / static_cast<double>(outputRate_) *
                  static_cast<double>(std::clamp(command.freqMul, 0.01f, 100.0f));
    voice->gain = std::clamp(command.volume, 0.0f, 1.5f);
    voice->channel = command.channel;
    voice->loop = command.loop;
    voice->pan = std::clamp(command.pan, -1.0f, 1.0f);
    voice->active = totalFrames > 0;
    started_.fetch_add(1);
}

void Mixer::fill(int16_t* out, int frames) {
    if (out == nullptr || frames <= 0) return;

    // Take whatever the game thread has queued since the last block. A block is
    // a few milliseconds, so this is normally one or two commands.
    Command command;
    int budget = kCommandSlots;
    while (budget-- > 0 && takeCommand(command)) startVoice(command);

    std::fill(out, out + static_cast<size_t>(frames) * 2, static_cast<int16_t>(0));

    for (Voice& voice : voices_) {
        if (!voice.active) continue;

        // Pan, MUGEN-style: full left is half the sound's volume in the right
        // ear, centre keeps both. Nothing more exotic is audible on a phone.
        const float panLeft = voice.pan <= 0.0f ? 1.0f : 1.0f - voice.pan * 0.5f;
        const float panRight = voice.pan >= 0.0f ? 1.0f : 1.0f + voice.pan * 0.5f;

        for (int i = 0; i < frames; ++i) {
            if (voice.position >= static_cast<double>(voice.frames)) {
                // A looping sound wraps; anything else retires.
                if (voice.loop) {
                    voice.position = std::fmod(voice.position, static_cast<double>(voice.frames));
                } else {
                    voice.active = false;
                    break;
                }
            }
            const size_t frame = static_cast<size_t>(voice.position);
            // Mono plays in both ears; stereo keeps its two channels. A mono
            // clip panned by copying is what every MUGEN sound expects.
            const int16_t left = voice.pcm[frame * static_cast<size_t>(voice.channels)];
            const int16_t right = voice.channels > 1
                                      ? voice.pcm[frame * static_cast<size_t>(voice.channels) + 1]
                                      : left;
            // Sum with saturation. A hard clip is what a mixing desk does and
            // sounds like a louder hit rather than like a fault.
            const int l = out[i * 2] +
                          static_cast<int>(left * voice.gain * panLeft * masterGain_);
            const int r = out[i * 2 + 1] +
                          static_cast<int>(right * voice.gain * panRight * masterGain_);
            out[i * 2] = static_cast<int16_t>(std::clamp(l, -32768, 32767));
            out[i * 2 + 1] = static_cast<int16_t>(std::clamp(r, -32768, 32767));
            voice.position += voice.step;
        }
    }

    rendered_.fetch_add(static_cast<uint64_t>(frames));
}

}  // namespace af

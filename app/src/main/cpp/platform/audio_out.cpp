#include "platform/audio_out.h"

#include "audio/mixer.h"
#include "core/log.h"

namespace af {

AudioOut::~AudioOut() { stop(); }

void AudioOut::onBufferQueue(SLAndroidSimpleBufferQueueItf /*queue*/, void* context) {
    static_cast<AudioOut*>(context)->serveBlock();
}

void AudioOut::serveBlock() {
    if (mixer_ == nullptr || queue_ == nullptr) return;

    std::array<int16_t, kBlockFrames * 2>& block = buffers_[static_cast<size_t>(nextBlock_)];
    nextBlock_ = (nextBlock_ + 1) % kBlocksInFlight;

    // Filled here, on the platform's audio thread, and never anywhere else: the
    // game thread's only part in this is the command ring inside the mixer.
    mixer_->fill(block.data(), kBlockFrames);
    const SLresult result = (*queue_)->Enqueue(queue_, block.data(), block.size() * sizeof(int16_t));
    if (result == SL_RESULT_SUCCESS) {
        ++served_;
    } else if (served_ == 0) {
        // Only worth reporting while nothing has worked yet; a queue that fails
        // once and recovers would otherwise fill the log.
        AF_LOGW("audio: enqueue failed (%u)", static_cast<unsigned>(result));
    }
}

bool AudioOut::start(Mixer& mixer, int sampleRate) {
    stop();
    mixer_ = &mixer;
    mixer_->setOutputRate(sampleRate);

    SLresult result = slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS || engineObject_ == nullptr) {
        AF_LOGE("audio: no OpenSL engine (%u); the game will be silent",
                static_cast<unsigned>(result));
        return false;
    }
    if ((*engineObject_)->Realize(engineObject_, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
        AF_LOGE("audio: engine will not realise");
        stop();
        return false;
    }
    if ((*engineObject_)->GetInterface(engineObject_, SL_IID_ENGINE, &engine_) !=
        SL_RESULT_SUCCESS) {
        AF_LOGE("audio: no engine interface");
        stop();
        return false;
    }

    if ((*engine_)->CreateOutputMix(engine_, &outputMixObject_, 0, nullptr, nullptr) !=
        SL_RESULT_SUCCESS) {
        AF_LOGE("audio: no output mix");
        stop();
        return false;
    }
    (*outputMixObject_)->Realize(outputMixObject_, SL_BOOLEAN_FALSE);

    // Stereo, 16-bit, at the rate the mixer renders at. The format has to be
    // declared in the Android-specific PCM struct, which is the one piece of
    // this that is not portable OpenSL.
    SLDataLocator_AndroidSimpleBufferQueue queueLocator = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, kBlocksInFlight};
    SLDataFormat_PCM format = {SL_DATAFORMAT_PCM,
                               2,  // channels
                               static_cast<SLuint32>(sampleRate * 1000),
                               SL_PCMSAMPLEFORMAT_FIXED_16,
                               SL_PCMSAMPLEFORMAT_FIXED_16,
                               SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                               SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource source = {&queueLocator, &format};
    SLDataLocator_OutputMix outputLocator = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject_};
    SLDataSink sink = {&outputLocator, nullptr};

    const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean required[] = {SL_BOOLEAN_TRUE};
    result = (*engine_)->CreateAudioPlayer(engine_, &playerObject_, &source, &sink, 1, ids, required);
    if (result != SL_RESULT_SUCCESS || playerObject_ == nullptr) {
        AF_LOGE("audio: no player (%u)", static_cast<unsigned>(result));
        stop();
        return false;
    }
    (*playerObject_)->Realize(playerObject_, SL_BOOLEAN_FALSE);
    if ((*playerObject_)->GetInterface(playerObject_, SL_IID_PLAY, &play_) !=
        SL_RESULT_SUCCESS) {
        AF_LOGE("audio: no play interface");
        stop();
        return false;
    }
    if ((*playerObject_)
            ->GetInterface(playerObject_, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &queue_) !=
        SL_RESULT_SUCCESS) {
        AF_LOGE("audio: no buffer queue");
        stop();
        return false;
    }
    (*queue_)->RegisterCallback(queue_, onBufferQueue, this);

    // Prime the queue: the player starts pulling as soon as it is playing, and a
    // queue with nothing in it would start by underrunning.
    for (int i = 0; i < kBlocksInFlight; ++i) serveBlock();

    (*play_)->SetPlayState(play_, SL_PLAYSTATE_PLAYING);
    AF_LOGI("audio: %d Hz stereo, %d blocks of %d frames", sampleRate, kBlocksInFlight,
            kBlockFrames);
    return true;
}

void AudioOut::stop() {
    if (play_ != nullptr) (*play_)->SetPlayState(play_, SL_PLAYSTATE_STOPPED);
    // Order matters: the player owns the callback, so it has to be destroyed
    // before the object the callback refers to is forgotten.
    if (playerObject_ != nullptr) {
        (*playerObject_)->Destroy(playerObject_);
        playerObject_ = nullptr;
    }
    play_ = nullptr;
    queue_ = nullptr;
    if (outputMixObject_ != nullptr) {
        (*outputMixObject_)->Destroy(outputMixObject_);
        outputMixObject_ = nullptr;
    }
    if (engineObject_ != nullptr) {
        (*engineObject_)->Destroy(engineObject_);
        engineObject_ = nullptr;
    }
    engine_ = nullptr;
    mixer_ = nullptr;
}

}  // namespace af

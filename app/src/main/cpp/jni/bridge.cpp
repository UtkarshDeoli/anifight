// JNI surface. Kept deliberately thin: it translates Android types into engine
// types and nothing else, so all real logic stays in portable C++.
#include <jni.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <memory>

#include "core/log.h"
#include "engine/engine.h"
#include "platform/asset_file_source.h"

namespace {

// One game instance per process, matching the single-Activity design.
std::unique_ptr<af::AssetFileSource> gFileSource;
std::unique_ptr<af::Engine> gEngine;

// Maps the index Kotlin sends to an engine button. The order is the physical
// layout of the on-screen pad, not MUGEN's bit order.
uint32_t buttonForIndex(jint index) {
    switch (index) {
        case 0: return af::Button::kUp;
        case 1: return af::Button::kDown;
        case 2: return af::Button::kLeft;
        case 3: return af::Button::kRight;
        case 4: return af::Button::kA;
        case 5: return af::Button::kB;
        case 6: return af::Button::kC;
        case 7: return af::Button::kX;
        case 8: return af::Button::kY;
        case 9: return af::Button::kZ;
        case 10: return af::Button::kStart;
        default: return 0;
    }
}

}  // namespace

extern "C" {

JNIEXPORT jboolean JNICALL
Java_dev_anifight_NativeBridge_create(JNIEnv* env, jobject /*thiz*/, jobject assetManager,
                                       jint width, jint height, jstring settingsPath) {
    if (gEngine != nullptr) {
        AF_LOGW("create() called while an engine already exists; ignoring");
        return JNI_TRUE;
    }

    AAssetManager* manager = AAssetManager_fromJava(env, assetManager);
    if (manager == nullptr) {
        AF_LOGE("could not obtain an AAssetManager from the AssetManager passed by Java");
        return JNI_FALSE;
    }

    // Everything the game ships with lives under assets/mugen/, mirroring the
    // layout of a real MUGEN installation so that dropping in a screenpack or a
    // character later is a file copy.
    gFileSource = std::make_unique<af::AssetFileSource>(manager, "mugen");

    gEngine = std::make_unique<af::Engine>();
    // Where settings may be written. Android owns this decision, so the path arrives
    // from Java; the engine only ever reads and writes the one file it is given.
    if (settingsPath != nullptr) {
        const char* utf8 = env->GetStringUTFChars(settingsPath, nullptr);
        if (utf8 != nullptr) {
            gEngine->setSettingsPath(utf8);
            env->ReleaseStringUTFChars(settingsPath, utf8);
        }
    }
    if (!gEngine->create(gFileSource.get(), width, height)) {
        AF_LOGE("engine failed to start");
        gEngine.reset();
        gFileSource.reset();
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_dev_anifight_NativeBridge_resize(JNIEnv* /*env*/, jobject /*thiz*/, jint width, jint height) {
    if (gEngine != nullptr) gEngine->resize(width, height);
}

JNIEXPORT void JNICALL
Java_dev_anifight_NativeBridge_frame(JNIEnv* /*env*/, jobject /*thiz*/, jfloat deltaSeconds) {
    if (gEngine != nullptr) gEngine->frame(static_cast<double>(deltaSeconds));
}

JNIEXPORT void JNICALL
Java_dev_anifight_NativeBridge_pointer(JNIEnv* /*env*/, jobject /*thiz*/, jint id, jfloat x,
                                       jfloat y, jboolean down) {
    if (gEngine != nullptr) gEngine->setPointer(id, x, y, down == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_dev_anifight_NativeBridge_button(JNIEnv* /*env*/, jobject /*thiz*/, jint index,
                                      jboolean down) {
    const uint32_t button = buttonForIndex(index);
    if (gEngine != nullptr && button != 0) {
        gEngine->pushInput(button, down == JNI_TRUE);
    }
}

JNIEXPORT void JNICALL
Java_dev_anifight_NativeBridge_destroy(JNIEnv* /*env*/, jobject /*thiz*/) {
    if (gEngine != nullptr) {
        gEngine->shutdown();
        gEngine.reset();
    }
    gFileSource.reset();
}

// Called when the Activity dies but the GL thread is already gone, so there is
// no context to release GL objects against. Dropping the handles is correct:
// the driver frees everything when the context is destroyed.
JNIEXPORT void JNICALL
Java_dev_anifight_NativeBridge_detach(JNIEnv* /*env*/, jobject /*thiz*/) {
    af::setGlContextAlive(false);
    gEngine.reset();
    gFileSource.reset();
    AF_LOGI("engine detached (GL context already gone)");
}

}  // extern "C"

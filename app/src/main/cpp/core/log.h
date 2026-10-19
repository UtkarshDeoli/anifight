// Logging. Header-only so any translation unit can log without a link dep.
#pragma once

#include <android/log.h>

#define AF_TAG "Anifight"

#define AF_LOGI(...) __android_log_print(ANDROID_LOG_INFO, AF_TAG, __VA_ARGS__)
#define AF_LOGW(...) __android_log_print(ANDROID_LOG_WARN, AF_TAG, __VA_ARGS__)
#define AF_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, AF_TAG, __VA_ARGS__)
#define AF_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, AF_TAG, __VA_ARGS__)

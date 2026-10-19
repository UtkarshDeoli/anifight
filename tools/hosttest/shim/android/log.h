// A stand-in for the NDK's <android/log.h>, so the engine's parsers can be
// compiled and run on the host.
//
// core/log.h is header-only and every translation unit that logs includes it,
// which means anything host-compilable transitively needs this. The engine
// logs a fair amount while parsing -- the parsers' own diagnostics are half of
// what the host tests assert on -- so this forwards to stderr rather than
// swallowing the output.
//
// Use with `-I tools/hosttest/shim`.
#pragma once

#include <cstdarg>
#include <cstdio>

enum {
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
};

inline int __android_log_print(int, const char* tag, const char* format, ...) {
    std::fprintf(stderr, "[%s] ", tag);
    va_list args;
    va_start(args, format);
    const int written = std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
    return written;
}

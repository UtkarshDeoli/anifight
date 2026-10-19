#include "engine/win_flow.h"

#include <algorithm>

namespace af {

namespace {

long long nonNegative(int value) { return value > 0 ? static_cast<long long>(value) : 0; }

}  // namespace

WinScreenTiming::WinScreenTiming(int fadeInTime, int poseTime, int fadeOutTime)
    : fadeIn_(nonNegative(fadeInTime)),
      pose_(nonNegative(poseTime)),
      fadeOut_(nonNegative(fadeOutTime)) {
    total_ = fadeIn_ + pose_ + fadeOut_;
}

float WinScreenTiming::fadeAlpha() const {
    if (fadeIn_ > 0 && tick_ < fadeIn_) {
        return 1.0f - static_cast<float>(tick_) / static_cast<float>(fadeIn_);
    }
    if (fadeOut_ > 0 && tick_ >= fadeIn_ + pose_) {
        const float t = static_cast<float>(tick_ - fadeIn_ - pose_) /
                        static_cast<float>(fadeOut_);
        return t > 1.0f ? 1.0f : t;
    }
    return 0.0f;
}

bool winTextVisible(int displayTime, long long tick) {
    // Negative is MUGEN's "show for the whole screen"; every other value is a
    // count of ticks from the start, so `tick == displayTime` is already gone.
    return displayTime < 0 || tick < static_cast<long long>(displayTime);
}

}  // namespace af

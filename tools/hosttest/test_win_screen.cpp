// Checks the two pieces of the Win screen that a device cannot show you: the
// fade-in / pose / fade-out clock and the wintext displaytime rule.
//
// The drawing itself is deliberately not covered. What is worth pinning is that
// the three `[Win Screen]` durations are additive (the pose is held *after* the
// fade-in, not instead of it), that the screen leaves on exactly
// `fadein + pose + fadeout`, that the overlay is black at both ends and clear
// through the pose, and that `displaytime = -1` really is persistent while a
// finite value stops the text on its own tick.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_win_screen \
//         tools/hosttest/test_win_screen.cpp app/src/main/cpp/engine/win_flow.cpp
//     /tmp/test_win_screen

#include <cmath>
#include <cstdio>

#include "engine/win_flow.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

void checkNear(float actual, float expected, const char* what) {
    if (std::fabs(actual - expected) < 1e-5f) return;
    std::fprintf(stderr, "FAIL: %s (expected %.4f, got %.4f)\n", what, expected, actual);
    ++failures;
}

}  // namespace

int main() {
    // ------------------------------------------------------------------- clock
    //
    // The distribution's shapes: 32 ticks in, a 300-tick pose, 64 ticks out
    // (SCREEN-FLOW-RE §2.5). Unlike VictoryTiming's `time`, `pose.time` is a
    // duration, so the screen is up for 396 ticks.
    constexpr int kIn = 32;
    constexpr int kPose = 300;
    constexpr int kOut = 64;

    af::WinScreenTiming timing(kIn, kPose, kOut);
    check(timing.tick() == 0, "a fresh win screen starts at tick 0");
    check(timing.totalTicks() == kIn + kPose + kOut,
          "the screen lasts fadein + pose + fadeout");
    check(!timing.fadeInDone(), "the fade-in is not done at tick 0");
    check(!timing.finished(), "a fresh win screen is not finished");
    checkNear(timing.fadeAlpha(), 1.0f, "the screen starts fully black");

    timing.advance();
    checkNear(timing.fadeAlpha(), 1.0f - 1.0f / static_cast<float>(kIn),
              "the fade-in lifts one step per tick");

    for (int i = 1; i < kIn; ++i) timing.advance();
    check(timing.tick() == kIn, "the fade-in ends on fadein.time");
    check(timing.fadeInDone(), "the fade-in is done on fadein.time");
    checkNear(timing.fadeAlpha(), 0.0f, "the pose is fully visible after the fade-in");

    // The pose is held after the fade-in, not before it: the last held tick and
    // the tick the fade-out starts on are both clear.
    while (timing.tick() < kIn + kPose - 1) timing.advance();
    checkNear(timing.fadeAlpha(), 0.0f, "the pose is clear on its last held tick");
    check(!timing.finished(), "the screen has not finished while the pose is held");
    timing.advance();
    check(timing.tick() == kIn + kPose, "the fade-out starts after the pose");
    checkNear(timing.fadeAlpha(), 0.0f, "the fade-out starts from fully visible");

    timing.advance();
    checkNear(timing.fadeAlpha(), 1.0f / static_cast<float>(kOut),
              "the fade-out darkens one step per tick");

    while (timing.tick() < kIn + kPose + kOut - 1) timing.advance();
    check(!timing.finished(), "the screen is still up one tick before the end");
    checkNear(timing.fadeAlpha(), 1.0f - 1.0f / static_cast<float>(kOut),
              "the screen is almost black on its last visible tick");

    timing.advance();
    check(timing.tick() == kIn + kPose + kOut,
          "the screen finishes on fadein + pose + fadeout");
    check(timing.finished(), "the screen finishes on fadein + pose + fadeout");
    checkNear(timing.fadeAlpha(), 1.0f, "the screen is black when it finishes");

    // It must not overshoot if a slow frame calls advance again before the pop
    // is applied; finishing has to stay stable.
    timing.advance();
    check(timing.finished(), "the screen stays finished after its end tick");
    checkNear(timing.fadeAlpha(), 1.0f, "the overlay does not go past fully black");

    // A screenpack that writes zeros gets a zero-length card rather than one
    // that can never leave. Negative values collapse to zero too.
    af::WinScreenTiming zeroed(0, 0, 0);
    check(zeroed.totalTicks() == 0, "all-zero durations total zero ticks");
    check(zeroed.fadeInDone(), "a zero-tick fade-in is done immediately");
    checkNear(zeroed.fadeAlpha(), 0.0f, "a zero-tick fade-in starts visible");
    check(zeroed.finished(), "an all-zero card is finished at tick 0");

    af::WinScreenTiming negative(-4, -10, -2);
    check(negative.totalTicks() == 0, "negative durations collapse to zero");
    check(negative.fadeInDone(), "a negative fade-in is treated as zero");
    checkNear(negative.fadeAlpha(), 0.0f, "a negative fade-in leaves the screen visible");

    // A zero-length pose is allowed: the fade-out follows the fade-in directly.
    af::WinScreenTiming noPose(10, 0, 5);
    check(noPose.totalTicks() == 15, "a zero pose still totals fadein + fadeout");
    for (int i = 0; i < 10; ++i) noPose.advance();
    checkNear(noPose.fadeAlpha(), 0.0f, "the fade-out starts when a zero pose ends");
    noPose.advance();
    checkNear(noPose.fadeAlpha(), 1.0f / 5.0f, "the zero-pose fade-out still steps");

    // A zero-length fade-out cuts rather than lingering.
    af::WinScreenTiming noFade(5, 3, 0);
    for (int i = 0; i < 8; ++i) noFade.advance();
    check(noFade.finished(), "a zero fade-out ends the card at fadein + pose");
    checkNear(noFade.fadeAlpha(), 0.0f, "a zero fade-out does not add a black step");

    // -------------------------------------------------------------- wintext
    //
    // The distribution writes displaytime = -1 for this screen, and the parser
    // supplies that default for an omitted key. Any other value counts from the
    // start of the card.
    check(af::winTextVisible(-1, 0), "-1 is persistent from the first tick");
    check(af::winTextVisible(-1, 100000), "-1 stays visible for the whole screen");
    check(af::winTextVisible(-2, 100000), "any negative displaytime is persistent");
    check(af::winTextVisible(10, 0), "a finite displaytime starts visible");
    check(af::winTextVisible(10, 9), "the text is visible on its last tick");
    check(!af::winTextVisible(10, 10), "the text is gone on the displaytime tick");
    check(!af::winTextVisible(0, 0), "displaytime 0 draws nothing");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

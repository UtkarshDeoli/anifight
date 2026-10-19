// Checks the pure half of the Continue screen: the countdown and the YES/NO
// selection. The drawing is deliberately not covered, as with the victory and
// Win cards; what matters here is that the clock expires on exactly the tick
// the interpretation says, and that the result the screen hands back follows
// the selection (or the timeout).
//
// SCREEN-FLOW-RE §3.3 leaves the reference's countdown duration unresolved and
// names no system.def key for it, so `kContinueCountdownTicks` is this engine's
// documented interpretation (10 seconds at the fixed 60 Hz tick), not a traced
// value. These tests pin the arithmetic, not the number's provenance.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_continue_screen \
//         tools/hosttest/test_continue_screen.cpp app/src/main/cpp/engine/continue_flow.cpp
//     /tmp/test_continue_screen

#include <cstdio>

#include "engine/continue_flow.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

}  // namespace

int main() {
    // -------------------------------------------------------------- duration
    //
    // The interpretation: ten wall-clock seconds at the engine's fixed 60 Hz
    // logic tick. Nothing in the reference pins this; the comment on the
    // constant says so as well.
    check(af::kContinueCountdownTicks == 600,
          "the interpreted countdown is 600 ticks (10 s at 60 Hz)");

    // --------------------------------------------------------------- countdown
    //
    // A fresh prompt starts at the full duration with YES selected. Advance is
    // a plain decrement with a floor at zero.
    af::ContinuePrompt prompt;
    check(prompt.ticksLeft() == af::kContinueCountdownTicks,
          "a fresh prompt starts at the full countdown");
    check(!prompt.expired(), "a fresh prompt has not expired");
    check(prompt.yesSelected(), "YES is selected by default");

    prompt.advance();
    check(prompt.ticksLeft() == af::kContinueCountdownTicks - 1,
          "advance removes exactly one tick");

    // Boundary: one tick left is not expired; the tick that reaches zero is.
    // The screen advances *before* it reads input, so the tick that hits zero
    // is already too late to answer -- ten seconds means ten seconds.
    while (prompt.ticksLeft() > 1) prompt.advance();
    check(prompt.ticksLeft() == 1, "the countdown reaches its last tick");
    check(!prompt.expired(), "one tick left is not expired");
    prompt.advance();
    check(prompt.ticksLeft() == 0, "the final advance reaches zero");
    check(prompt.expired(), "zero ticks means expired");

    // Expiry is stable: a slow frame or an extra update before the deferred pop
    // is applied must not underflow the clock.
    prompt.advance();
    check(prompt.ticksLeft() == 0, "the countdown does not go below zero");
    check(prompt.expired(), "an expired prompt stays expired");

    // A zero or negative duration is a prompt that is already out of time rather
    // than one that can never leave. The screenpack cannot write this (the
    // duration is not a key), but the constructor is public and the clamp keeps
    // it honest.
    af::ContinuePrompt zero(0);
    check(zero.ticksLeft() == 0, "a zero duration starts expired");
    check(zero.expired(), "a zero duration is expired at once");

    af::ContinuePrompt negative(-30);
    check(negative.ticksLeft() == 0, "a negative duration collapses to zero");
    check(negative.expired(), "a negative duration is expired at once");

    af::ContinuePrompt oneTick(1);
    check(!oneTick.expired(), "a one-tick duration is alive before its tick");
    oneTick.advance();
    check(oneTick.expired(), "a one-tick duration expires on its tick");

    // A custom duration is honoured as given, which is what lets the clock stay
    // testable without waiting ten seconds.
    af::ContinuePrompt shortPrompt(5);
    check(shortPrompt.ticksLeft() == 5, "a custom duration is used as given");
    for (int i = 0; i < 4; ++i) shortPrompt.advance();
    check(!shortPrompt.expired(), "a custom countdown is not early");
    shortPrompt.advance();
    check(shortPrompt.expired(), "a custom countdown expires on its tick");

    // --------------------------------------------------------------- selection
    //
    // YES is the left block and NO the right one. Movement clamps rather than
    // wrapping: MUGEN's two-option prompt does not cycle between them, and a
    // held direction at an end must not flip the selection back and forth.
    af::ContinuePrompt select;
    check(select.yesSelected(), "selection starts on YES");

    select.moveSelection(-1);
    check(select.yesSelected(), "left at YES clamps to YES");

    select.moveSelection(+1);
    check(!select.yesSelected(), "right moves YES to NO");

    select.moveSelection(+1);
    check(!select.yesSelected(), "right at NO clamps to NO");

    select.moveSelection(0);
    check(!select.yesSelected(), "a zero move leaves the selection alone");

    select.moveSelection(-1);
    check(select.yesSelected(), "left moves NO back to YES");

    // A multi-step delta still lands on the nearest end, which is what a
    // caller that folded a long press into one move should get.
    select.moveSelection(+7);
    check(!select.yesSelected(), "a large right move lands on NO");
    select.moveSelection(-7);
    check(select.yesSelected(), "a large left move lands on YES");

    // ---------------------------------------------------------------- results
    //
    // The result mapping is the whole contract with FightScreen: accepting
    // resumes the match, declining or timing out ends the run.
    af::ContinuePrompt accepted;
    check(accepted.chosen() == af::ContinueResult::Accepted,
          "YES maps to Accepted");

    accepted.moveSelection(+1);
    check(accepted.chosen() == af::ContinueResult::Declined,
          "NO maps to Declined");

    check(af::ContinuePrompt::timedOut() == af::ContinueResult::Declined,
          "a timeout takes the NO path");

    // Selection does not stop the clock, and the clock does not change the
    // selection: a player who moved to NO and then let time run out still ends
    // the run, and one who moved back to YES is still accepting on the last
    // tick.
    af::ContinuePrompt late(2);
    late.moveSelection(+1);
    late.advance();
    check(late.ticksLeft() == 1 && !late.expired(), "moving does not spend a tick");
    late.moveSelection(-1);
    check(late.chosen() == af::ContinueResult::Accepted,
          "the selection survives to the last tick");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

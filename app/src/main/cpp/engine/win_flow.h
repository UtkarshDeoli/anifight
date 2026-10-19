// The pure half of the Win screen: its clock and its text-visibility rule.
//
// `[Win Screen]` is the one card shown per *cleared arcade run* (SCREEN-FLOW-RE
// §2.5, §3.2 steps 7-9), not per match. Its shape is fade-in / hold the pose /
// fade-out over three screenpack numbers, and its win text has MUGEN's
// `displaytime` (the distribution's motifs all write -1, "persistent").
//
// Both pieces are separated from the screen for the same reason victory_flow
// exists: neither needs a renderer, a character or a GL context, so the host
// suite can pin the arithmetic a device test cannot see. The screen's drawing
// is deliberately not host-tested.
#pragma once

namespace af {

// The Win screen's fade-in / pose / fade-out clock.
//
// SCREEN-FLOW-RE §2.5 gives three separate durations, not one hold: the pose is
// held for `pose.time` *after* the fade-in completes. So the shape is
//
//     [0, fadeIn)                          fading in from black
//     [fadeIn, fadeIn + pose)              fully visible
//     [fadeIn + pose, fadeIn + pose + out) fading back to black
//
// and the screen is finished on the tick `fadeIn + pose + out`. This differs
// from VictoryTiming, where the screenpack's `time` is the tick the fade-out
// starts at; the two records mean different things and are kept apart rather
// than sharing a class with a renamed field.
//
// Negative values collapse to zero. A screenpack that writes `pose.time = 0`
// gets a card that goes straight through: the reference's 300 is the compiled
// default for an omitted key, not a floor for an explicit zero.
class WinScreenTiming {
public:
    WinScreenTiming(int fadeInTime, int poseTime, int fadeOutTime);

    void advance() { ++tick_; }

    [[nodiscard]] long long tick() const { return tick_; }
    // The tick at which the screen pops. Exposed so a caller can log the shape
    // it will play without duplicating the sum.
    [[nodiscard]] long long totalTicks() const { return total_; }
    [[nodiscard]] bool fadeInDone() const { return tick_ >= fadeIn_; }
    [[nodiscard]] bool finished() const { return tick_ >= total_; }

    // The alpha of the black overlay: 1 at the start of the fade-in, 0 through
    // the held pose, and 1 again at the end of the fade-out.
    [[nodiscard]] float fadeAlpha() const;

private:
    long long tick_ = 0;
    long long fadeIn_ = 0;
    long long pose_ = 0;
    long long fadeOut_ = 0;
    long long total_ = 0;
};

// Whether a wintext element with `displayTime` is still visible on `tick`.
//
// MUGEN's `displaytime = -1` means "persistent", and the distribution's motifs
// all use it for this screen. Any other value is the number of ticks the text
// is shown from the start of the screen; zero draws nothing. (DisplayElement's
// own default is 1, but the `[Win Screen]` parser supplies -1 for an omitted
// key, so a section that writes nothing still gets the persistent banner.)
[[nodiscard]] bool winTextVisible(int displayTime, long long tick);

}  // namespace af

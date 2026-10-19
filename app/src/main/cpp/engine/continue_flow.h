// The pure half of the Continue screen: its countdown, its two-way selection,
// and the result the fight flow acts on.
//
// SCREEN-FLOW-RE.md §2.6 and §3.3 establish the shape of this screen from the
// reference: `[Continue Screen]` draws five text blocks, YES is one side and NO
// the other, an accepted continue resumes the fight loop without advancing
// `matchno`, and a declined or expired one reaches the Game Over gate. The two
// things the reference does *not* expose are the countdown duration and the
// exact timeout path; both are recorded as unresolved there.
//
// Separating this from the screen is the same choice victory_flow and win_flow
// make: none of this needs a renderer, a character or a GL context, so the host
// suite can pin arithmetic that a device test cannot see. The drawing and the
// input routing are deliberately left to the screen.
#pragma once

namespace af {

// How many ticks the continue prompt waits for a decision.
//
// **This is an interpretation, not a traced value.** SCREEN-FLOW-RE §3.3
// records that no `system.def` key exposes the duration and that the UI
// routine `0x44f470` contains no immediate compare against a plausible length,
// so the reference's countdown could not be measured. MUGEN's arcade continue
// screen traditionally gives the player ten seconds; at this engine's fixed
// 60 Hz that is 600 ticks. It is one named constant so a future measurement
// changes one line. Nothing draws it: the section has no timer element, so the
// countdown expires silently.
inline constexpr int kContinueCountdownTicks = 600;

// What the fight flow does after the prompt closes.
enum class ContinueResult {
    Accepted,  // YES was confirmed: resume this match, without advancing matchno
    Declined,  // NO was confirmed, or the countdown ran out: end the run
};

// The prompt's state, with no drawing and no input dependency.
class ContinuePrompt {
public:
    // A duration of zero or less is already expired. Negative file values are
    // not possible here (there is no key), but a caller may still ask for one.
    explicit ContinuePrompt(int durationTicks = kContinueCountdownTicks);

    // One tick, matching the engine's 60 Hz logic step. A prompt that has
    // already expired stays expired; its value is stable for a caller that
    // reads it again on the tick the pop is applied.
    void advance();

    // Left (-1) moves to YES and right (+1) to NO, matching the layout: the
    // `yes.*` text sits at a negative offset and `no.*` at a positive one.
    // Two options do not wrap, so pushing past an edge keeps the selection
    // there. A zero delta does nothing.
    void moveSelection(int delta);

    [[nodiscard]] bool yesSelected() const { return yes_; }
    [[nodiscard]] int ticksLeft() const { return ticksLeft_; }
    [[nodiscard]] bool expired() const { return ticksLeft_ <= 0; }

    // Confirming the current selection.
    [[nodiscard]] ContinueResult chosen() const {
        return yes_ ? ContinueResult::Accepted : ContinueResult::Declined;
    }
    // Letting the clock run out follows the NO path (SCREEN-FLOW-RE §3.3: both
    // reach the Game Over gate; the intermediate flag was not named).
    [[nodiscard]] static constexpr ContinueResult timedOut() {
        return ContinueResult::Declined;
    }

private:
    int ticksLeft_ = 0;
    bool yes_ = true;
};

}  // namespace af

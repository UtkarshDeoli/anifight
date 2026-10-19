// The Continue screen: the YES/NO prompt shown after a lost arcade match.
//
// This class is `ContinueScreenView` and not `ContinueScreen` because that name
// already belongs to the parsed `[Continue Screen]` record in engine/defs.h
// (`af::ContinueScreen`). The victory and Win screens hit the same collision and
// solved it the same way.
//
// SCREEN-FLOW-RE.md §2.6, §3.2 step 5b and §3.3 describe the reference: the
// screenpack's five text blocks are drawn at `pos`, YES starts selected, the
// prompt takes a yes/no decision, accepting resumes the fight loop without
// advancing `matchno`, and declining or letting the countdown expire ends the
// run. The section names no background and no fade times, so the card is black
// behind the screenpack's own text.
//
// The screen holds no flow decision of its own. It writes its result into a
// caller-owned value and pops; `FightScreen` is what restarts or ends the run.
// That keeps the arcade flow in one place and lets this class be pushed anywhere
// a prompt is wanted.
#pragma once

#include "engine/continue_flow.h"
#include "engine/defs.h"
#include "engine/screen.h"

namespace af {

class Game;

class ContinueScreenView : public Screen {
public:
    // `result` is written exactly once, immediately before the screen pops.
    // It may be null, for a caller that only wants the prompt's timing.
    explicit ContinueScreenView(Game& game, ContinueResult* result = nullptr);

    void enter() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

    // The on-screen pad is the decision surface on a phone: its d-pad moves the
    // selection and its attack buttons (and Start) confirm. Unlike the timed
    // cards, this screen has something to navigate, so the pad is drawn.
    [[nodiscard]] bool wantsTouchPad() const override { return true; }
    [[nodiscard]] bool acceptsTouchInput() const override { return true; }

private:
    // Writes the result and leaves. Guarded so repeated calls in one tick
    // cannot pop twice.
    void decide(ContinueResult result);

    Game& game_;
    ContinueResult* result_ = nullptr;
    ContinuePrompt prompt_;
    // Set by decide(). The pop is deferred to the end of the tick, so without
    // this a second input read or an expiry in the same tick could queue a
    // second pop and take the screen underneath with it.
    bool decided_ = false;
    long long tick_ = 0;
};

}  // namespace af

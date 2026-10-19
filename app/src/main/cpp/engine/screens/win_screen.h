// The Win Screen: the card shown once after an arcade run is cleared.
//
// This class is `WinScreenView` and not `WinScreen` because that name already
// belongs to the parsed `[Win Screen]` record in engine/defs.h
// (`af::WinScreen`). The victory slice ran into the same collision and solved
// it the same way; the two names sit in one translation unit otherwise.
//
// It is deliberately not a per-match card. `[Victory Screen]` is shown after a
// won *match*; `[Win Screen]` is shown after the *last* match of a cleared run,
// once, and is skipped for a character with an ending (SCREEN-FLOW-RE §2.5,
// §3.2 steps 7-9). This engine has no ending detection yet, so the gate here
// is exactly what the flow can honestly ask: the run was cleared and
// `[Win Screen] enabled` is on.
//
// The reference draws the winner's win pose. This engine has no forced
// state-180 win pose, so the honest equivalent is the frame the winner was
// frozen in when the match ended -- the same fallback VictoryScreenView uses
// for a missing portrait. The pose is drawn without input or decisions; the
// screen stack holds the fight underneath, so nothing ticks while it is up.
#pragma once

#include "engine/defs.h"
#include "engine/screen.h"
#include "engine/win_flow.h"

namespace af {

class Character;
class Fighter;
class Game;

class WinScreenView : public Screen {
public:
    // `winner` is the character whose pose is drawn and `winnerFighter` is the
    // fighter frozen at the end of the match. Either may be null; the card then
    // draws only what it can, never an empty pose over the text.
    WinScreenView(Game& game, Character* winner, const Fighter* winnerFighter);

    void enter() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

    // The reference's Win Screen takes no input and has no skip: it is once per
    // run and it says the run is over. So neither the pad nor a tap reaches it.
    [[nodiscard]] bool wantsTouchPad() const override { return false; }
    [[nodiscard]] bool acceptsTouchInput() const override { return false; }

private:
    void drawPose(Renderer& renderer);
    void drawWinText(Renderer& renderer);

    Game& game_;
    Character* winner_ = nullptr;
    const Fighter* winnerFighter_ = nullptr;
    WinScreenTiming timing_;
};

}  // namespace af

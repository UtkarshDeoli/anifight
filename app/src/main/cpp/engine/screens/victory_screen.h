// The Victory card: the screen shown after a won arcade match.
//
// This class is `VictoryScreenView` and not `VictoryScreen` because that name
// already belongs to the parsed `[Victory Screen]` record in engine/defs.h
// (`af::VictoryScreen`). One is a screenpack section, the other is a screen;
// both are visible in the same translation unit, and a shared name would make
// every mention ambiguous.
//
// MUGEN's arcade flow is VS before a match and Victory after a won one, with
// the winner's portrait, name and win quote. It is a timed card and takes no
// decisions: the screen stack already holds the fight while it is up, so the
// round does not tick and no input reaches the fighters underneath. That is the
// same reason VersusScreen is a screen rather than an overlay.
//
// The card reads `[Victory Screen]` through `SystemDef::victory` and
// `[VictoryBGdef]` through `SystemDef::victoryBg`; there is no art in this
// class. The quote comes from the winner character's `[Quotes]` (chosen by
// `VictoryQuote`, or random when the character did not choose), with the
// screenpack's `winquote.text` as the fallback when the character has none.
// SCREEN-FLOW-RE.md §2.4 and §3.4 are the reference for that order.
#pragma once

#include <string>

#include "engine/defs.h"
#include "engine/screen.h"
#include "engine/victory_flow.h"

namespace af {

class Character;
class Fighter;
class Game;

class VictoryScreenView : public Screen {
public:
    // `winner` is the character whose portrait and name the card shows, and
    // `winnerFighter` is its live fighter, used only when the character has no
    // group 9000, image 2 portrait. Either may be null; the card then says what
    // it can and never draws an empty frame on top of the background.
    VictoryScreenView(Game& game, Character* winner, const Fighter* winnerFighter, int matchNo);

    void enter() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

    // Nothing to navigate, so the pad is not drawn over the screenpack's own
    // layout -- but a tap or any button still skips the card, exactly as the VS
    // screen does.
    [[nodiscard]] bool wantsTouchPad() const override { return false; }
    [[nodiscard]] bool acceptsTouchInput() const override { return true; }

private:
    void drawPortrait(Renderer& renderer);
    void drawName(Renderer& renderer);
    void drawWinQuote(Renderer& renderer);
    // Used when the character's own victory portrait is absent: draws the frame
    // the winner is showing, fitted into the portrait window.
    void drawAnimationFallback(Renderer& renderer, const DisplayElement& element);

    Game& game_;
    Character* winner_ = nullptr;
    const Fighter* winnerFighter_ = nullptr;
    int matchNo_ = 1;
    // The quote is resolved once, in the constructor, and not per draw: the
    // choice must not change between frames, and drawing must not advance the
    // fight's random stream.
    int quoteIndex_ = -1;
    std::string quoteText_;
    VictoryTiming timing_;
};

}  // namespace af

// The VS screen: the card that names the two fighters before a match.
//
// MUGEN's flow is title, select, versus, fight, and this is the third of those.
// It is a timed card and nothing else -- it takes no input, decides nothing, and
// its only job is to say who is about to fight and to give the screenpack's own
// `[VS Screen]` elements somewhere to be drawn.
//
// It exists as a screen rather than as an overlay inside the fight screen
// because the engine only updates and draws the top of the stack. That is what
// makes the card free: while it is up the round is not ticking, the fighters are
// not moving and no input is being read, without the fight screen needing a
// paused flag or the round needing to know a card is in front of it.
#pragma once

#include <string>

#include "engine/defs.h"
#include "engine/screen.h"

namespace af {

class Character;
class Game;

class VersusScreen : public Screen {
public:
    // Either fighter may be null, which is what an engine-test match hands over;
    // the card then shows the name it is given and no portrait.
    VersusScreen(Game& game, Character* p1, Character* p2, int matchNumber);

    void enter() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

    // A card with nothing to navigate, so no pad is drawn across the
    // screenpack's own layout. It still takes input, though: a tap or any
    // button skips the card, which is what MUGEN does and what a player who has
    // seen it before expects.
    [[nodiscard]] bool wantsTouchPad() const override { return false; }
    [[nodiscard]] bool acceptsTouchInput() const override { return true; }

private:
    // One side of the card: the portrait and the name, which the screenpack
    // declares as two separate elements that this screen has to pair up.
    void drawSide(Renderer& renderer, const DisplayElement& portrait,
                  const DisplayElement& nameElement, Character* fighter,
                  const std::string& fallbackName);

    Game& game_;
    Character* fighters_[2] = {nullptr, nullptr};
    int matchNumber_ = 1;
    long long tick_ = 0;
};

}  // namespace af

// The stage select screen.
//
// MUGEN 1.0 has no stage-select screen of its own: a match's stage comes from
// the stage list beside the character in select.def, falling back to
// [ExtraStages]. This game ships a much larger stage pool than its roster has
// entries for, and none of the roster lines name a stage, so every match used
// to land in the same arena -- the first extra stage. This screen is the
// missing choice: it lists every stage that actually loaded, under the
// `[Info] displayname` MUGEN documents for exactly this list, and hands the
// picked one to the fight.
//
// It sits above the character select screen and replaces itself with the
// fight, so the stack stays title -> select -> fight and the fight's
// "return to the title" pops still mean what they meant before.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/screen.h"

namespace af {

class Game;
class Stage;

class StageSelectScreen : public Screen {
public:
    // Builds the screen a confirmed stage leads to. A null stage means "no
    // stage loaded at all", which the fight already handles by drawing its
    // procedural test arena.
    using FightFactory = std::function<std::unique_ptr<Screen>(Stage*)>;
    // Called when the player backs out, before this screen pops. The select
    // screen uses it to reopen the pick that led here -- in the non-versus
    // modes the character was already locked by the time we were pushed, and
    // backing out should not leave the grid believing a second player is
    // choosing.
    using CancelHandler = std::function<void()>;

    StageSelectScreen(Game& game, FightFactory onChoose, CancelHandler onCancel = {},
                      int initialIndex = 0);

    void enter() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

private:
    [[nodiscard]] int stageCount() const { return static_cast<int>(stages_.size()); }
    [[nodiscard]] Stage* stageAt(int index) const;
    [[nodiscard]] std::string labelFor(int index) const;

    // Pops this screen and pushes the chosen fight in the same tick. Deferred
    // by the engine exactly like any other stack change.
    void choose(Stage* stage);

    Game& game_;
    FightFactory onChoose_;
    CancelHandler onCancel_;
    std::vector<Stage*> stages_;
    int selected_ = 0;
    int firstVisible_ = 0;
    bool launched_ = false;
    long long tick_ = 0;
};

}  // namespace af

// The title screen: the screenpack's background, and the menu it declares.
//
// The menu is built from `menu.itemname.*`, and an entry left empty simply does
// not appear, which is how a screenpack hides a mode it does not implement. The
// one entry that is ours rather than MUGEN's is `training`, relabelled "ENGINE
// TEST": it drops straight into the fight screen with placeholder fighters,
// which is the only way to see the fight HUD in a game that has no characters.
#pragma once

#include <string>
#include <vector>

#include "engine/screen.h"

namespace af {

class Game;

class TitleScreen : public Screen {
public:
    explicit TitleScreen(Game& game);

    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

private:
    struct MenuItem {
        std::string key;    // "arcade", "exit", ...
        std::string label;  // what the font draws
    };

    void buildMenu();
    void activate();
    [[nodiscard]] int visibleItemCount() const;

    Game& game_;
    std::vector<MenuItem> items_;
    int selected_ = 0;
    int firstVisible_ = 0;
    long long tick_ = 0;
    int fade_ = 0;  // ticks elapsed since entering, for the fade in
    // Choosing an item starts `fadeout.time` rather than switching screen on the
    // same frame: MUGEN fades the title out before the next screen's own fade
    // in, so the two never cut against each other.
    bool leaving_ = false;
    int leaveTick_ = 0;
};

}  // namespace af

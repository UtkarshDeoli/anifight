// The splash card, and the loading screen behind it.
//
// Both exist because of what the game does before it can show a menu: it reads
// a screenpack, two sprite banks, its fonts, a fight definition, and then every
// character and stage the roster names. That is minutes of work on a real
// roster -- a single character with a thousand sprites takes seconds -- and
// until it is done there is nothing to draw. Without these two screens the game
// is a black rectangle for all of it.
//
// They are split because they are doing different jobs. The splash is a card
// with the game's name on it, up for a couple of seconds and skippable, which is
// what tells a player the app is alive. The loading screen is the real one: a
// bar that moves, the name of the thing being read, and a spinner that keeps
// turning while the roster is decoded on the worker thread.
//
// Neither draws the on-screen pad. There is nothing to steer and everything to
// tap.
#pragma once

#include <string>

#include "engine/screen.h"

namespace af {

class Game;

class SplashScreen : public Screen {
public:
    explicit SplashScreen(Game& game);

    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;
    [[nodiscard]] bool wantsTouchPad() const override { return false; }

private:
    // How long the card stays up before the loading screen replaces it. Short
    // enough not to be in the way, long enough to be read.
    static constexpr long long kSplashTicks = 110;

    Game& game_;
    long long tick_ = 0;
};

class LoadingScreen : public Screen {
public:
    explicit LoadingScreen(Game& game);

    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;
    [[nodiscard]] bool wantsTouchPad() const override { return false; }

private:
    Game& game_;
    long long tick_ = 0;
    int done_ = 0;
    int total_ = 0;
    // What the game is reading right now, so a slow character can be named
    // rather than leaving the player wondering what it is stuck on.
    std::string item_;
};

}  // namespace af

// The storyboard player: the screen behind the screenpack's `storyboard = ...`
// switches.
//
// A storyboard is not a screenpack screen with elements at fixed positions; it
// is a tiny timeline of its own (see engine/storyboard.h). This class is the
// half that needs a renderer: it loads the storyboard's own `spr`, `snd` and
// `font0..font9`, draws each scene's layers and fades, and pops when the
// timeline ends.
//
// The screen owns the storyboard's sprite bank. The sound bank is cached for
// the process instead: the mixer registers a bank by raw pointer and keeps it
// forever, so a screen-owned bank would dangle after the pop. See the cache
// helper in storyboard_screen.cpp.
//
// There is deliberately no skip input. The reference documents no key or button
// that ends a storyboard early, the flow notes do not mention one, and a screen
// that invented one would change how long Game Over lasts from the screenpack's
// point of view. Time is the only way out.
//
// BGM is not playable: this engine has no music decoder (`MusicDef` is parsed
// and explicitly inert), so a scene that names `bgm` is reported once and then
// passed over. `bg.name` background objects are reported the same way. Nothing
// is faked.
#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <string>

#include "audio/snd_bank.h"
#include "engine/screen.h"
#include "engine/storyboard.h"
#include "gfx/sprite_bank.h"

namespace af {

class FileSource;
class Font;
class Game;

class StoryboardScreen : public Screen {
public:
    // Loads eagerly, because a screen has no engine until it is pushed and the
    // caller wants to know *before* pushing whether the file is usable.
    // `storyboardPath` is the value from system.def's `storyboard` key.
    StoryboardScreen(FileSource& files, Game& game, std::string storyboardPath);

    // False when the storyboard file is missing, unreadable, or has no scenes.
    // The caller keeps its old fallback in that case.
    [[nodiscard]] bool ready() const { return ready_; }
    [[nodiscard]] const std::string& error() const { return error_; }

    void enter() override;
    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

    // Nothing to navigate: the pad would only cover the storyboard.
    [[nodiscard]] bool wantsTouchPad() const override { return false; }
    [[nodiscard]] bool acceptsTouchInput() const override { return false; }

private:
    void load(FileSource& files, Game& game, const std::string& storyboardPath);
    void finish();

    // Starts the sounds whose `starttime` this scene has reached.
    void playDueSounds(size_t scene, long long localTick);

    [[nodiscard]] Font* fontFor(int index) const;

    Game& game_;
    std::string path_;
    bool ready_ = false;
    std::string error_;

    StoryboardDef def_;
    StoryboardTiming timing_;
    SpriteBank sprites_;
    // Shared, not owned. The mixer registers a bank by raw pointer and keeps it
    // for the life of the engine, so an ephemeral storyboard bank would dangle
    // the moment this screen pops. storyboard_screen.cpp caches banks by path
    // for the process lifetime, which bounds them to the handful of storyboard
    // files a screenpack can name.
    std::shared_ptr<SndBank> sounds_;
    std::array<std::unique_ptr<Font>, 10> fonts_;

    // The scene whose per-scene sound flags are live, and which of the 100
    // samples have already been started for it.
    size_t currentScene_ = static_cast<size_t>(-1);
    std::array<bool, 100> soundStarted_{};

    bool popping_ = false;
    bool bgmReported_ = false;
    bool backgroundReported_ = false;
    bool audioLimitsReported_ = false;
    bool missingSoundReported_ = false;
};

}  // namespace af

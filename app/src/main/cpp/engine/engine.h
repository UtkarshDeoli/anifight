// The top-level engine: owns the renderer, the screen stack and the clock.
//
// Threading contract: `create`, `resize`, `frame` and `shutdown` are called on
// the GL thread only. `pushInput` is called from the UI thread and is the one
// thread-safe entry point.
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/game.h"
#include "engine/input.h"
#include "engine/screen.h"
#include "engine/touch_controls.h"
#include "audio/mixer.h"
#include "platform/audio_out.h"
#include "gfx/renderer.h"

namespace af {

class FileSource;

class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool create(FileSource* files, int framebufferWidth, int framebufferHeight);
    void resize(int framebufferWidth, int framebufferHeight);
    void frame(double deltaSeconds);
    void shutdown();

    // Called from the UI thread.
    void pushInput(uint32_t button, bool isDown);
    // `id` is Android's pointer id, which is stable for the life of a contact.
    // Tracking them separately is what lets a player hold a direction with one
    // thumb and press an attack with the other.
    void setPointer(int id, float framebufferX, float framebufferY, bool touching);

    void pushScreen(std::unique_ptr<Screen> screen);
    void popScreen();
    void replaceScreen(std::unique_ptr<Screen> screen);
    void requestQuit() { quit_ = true; }
    [[nodiscard]] bool quitting() const { return quit_; }

    [[nodiscard]] FileSource& files() { return *files_; }
    [[nodiscard]] Renderer& renderer() { return renderer_; }
    [[nodiscard]] Game& game() { return game_; }
    // The on-screen pad, so that the screen that edits its layout can reach it.
    [[nodiscard]] touchpad::Pad& pad() { return pad_; }

    // Where the pad's arrangement is saved, and reloaded from at start-up. An empty
    // path means nowhere: the pad still works, it just forgets where the player put
    // things when the app closes. Android hands this over from the Java side, which
    // is the only part of the app that knows where its own writable directory is.
    void setSettingsPath(std::string path) { settingsPath_ = std::move(path); }
    // Reads the saved layout, if there is one. Called once when the engine starts.
    void loadSettings();
    // Writes the current layout out. Called by the layout editor, and by anything
    // else that changes a setting.
    void saveSettings() const;

    // Where screens send their sounds. It exists whether or not a device opened:
    // a mixer with no output still mixes, and a `PlaySnd` with no mixer would be
    // a null check in every call site instead of one place.
    [[nodiscard]] Mixer& mixer() { return mixer_; }

private:
    void step(double fixedDelta);
    void applyPendingScreenChanges();
    void drawDiagnosticPattern();

    Renderer renderer_;
    Mixer mixer_;
    AudioOut audio_;
    FileSource* files_ = nullptr;
    Game game_;

    std::vector<std::unique_ptr<Screen>> stack_;
    std::vector<std::unique_ptr<Screen>> pendingPush_;
    int pendingPop_ = 0;

    // Written on the UI thread, read on the GL thread.
    std::mutex inputMutex_;
    uint32_t pendingHeld_ = 0;
    uint32_t pendingPressed_ = 0;
    uint32_t pendingReleased_ = 0;
    // A finger the UI thread has reported, plus whether it has already lifted.
    //
    // The engine samples fingers once a tick, so a contact that goes down and up
    // between two ticks would otherwise never be seen at all -- a quick tap
    // would simply vanish. Keeping a lifted finger in the map until the next tick
    // samples it guarantees every contact is seen at least once, so touch latches
    // the same way the keyboard does.
    struct Finger {
        touchpad::Contact contact;
        bool lifted = false;
    };
    std::unordered_map<int, Finger> pointers_;

    // The on-screen pad: its layout, the fingers it is holding, and how bright it
    // has faded to. It lives entirely on the GL thread.
    touchpad::Pad pad_;
    uint32_t touchHeld_ = 0;   // this tick, for drawing
    uint32_t lastTouch_ = 0;   // last tick, so contacts produce press/release edges
    Screen* lastTop_ = nullptr;

    InputState input_;
    bool quit_ = false;
    bool created_ = false;
    std::string settingsPath_;

    double accumulator_ = 0.0;
    double fpsWindow_ = 0.0;
    int fpsFrames_ = 0;
};

}  // namespace af

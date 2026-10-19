#include "engine/engine.h"

#include <fstream>
#include <sstream>

#include <cmath>
#include <cstdio>
#include <iterator>
#include <utility>

#include "core/file_source.h"
#include "core/log.h"
#include "engine/screens/splash_screen.h"
#include "engine/screens/title_screen.h"

namespace af {

namespace {

// MUGEN's logic tick is exactly 60 Hz. Screens count frames, not seconds, so
// the engine converts real time into whole ticks and never hands a screen a
// partial one.
constexpr double kFixedStep = 1.0 / 60.0;
// If the app is backgrounded or the device stalls, do not try to catch up more
// than this many ticks in a single rendered frame.
constexpr int kMaxStepsPerFrame = 5;

// Files the engine expects to find in a complete game folder, checked on boot
// so a broken screenpack produces a clear log line instead of a blank screen.
constexpr const char* kExpectedFiles[] = {
    "data/mugen.cfg",
    "data/system.def",
    "data/fight.def",
    "data/select.def",
};

}  // namespace

Engine::Engine() = default;
Engine::~Engine() = default;

bool Engine::create(FileSource* files, int framebufferWidth, int framebufferHeight) {
    files_ = files;

    const char* error = nullptr;
    if (!renderer_.init(&error)) {
        AF_LOGE("engine create failed: %s", error != nullptr ? error : "unknown");
        return false;
    }

    renderer_.setFramebufferSize(framebufferWidth, framebufferHeight);
    // MUGEN's canonical coordinate space. A screenpack can override it via
    // `localcoord` in system.def; until system.def is parsed this is the default.
    renderer_.setVirtualSize(320.0f, 240.0f);

    created_ = true;

    if (files_ != nullptr) {
        for (const char* path : kExpectedFiles) {
            AF_LOGI("data check: %-20s %s", path, files_->exists(path) ? "present" : "MISSING");
        }

        // Loading needs a current GL context, because fonts upload their glyph
        // strip as they are read. That is why this happens here and not in the
        // engine's constructor.
        //
        // Only the *first* half happens here. The roster and the stages are
        // minutes of work and are stepped by the loading screen, one item a
        // frame, so the game has something on screen while it does them.
        if (!game_.beginLoad(*files_)) {
            AF_LOGE("no usable screenpack; falling back to the diagnostic view");
        } else {
            // The game's coordinate space is the screenpack's, not ours: a
            // screenpack authored for 640x480 must not be squashed into 320x240.
            const Vec2 local = game_.system().localCoord;
            if (local.x > 0.0f && local.y > 0.0f) {
                renderer_.setVirtualSize(local.x, local.y);
            }
            renderer_.setAspectMode(game_.config().aspect);
            // The pad's saved arrangement, which has to be read before the first
            // frame draws it.
            loadSettings();
            AF_LOGI("game ready: virtual %.0fx%.0f, %d Hz", renderer_.virtualWidth(),
                    renderer_.virtualHeight(), game_.config().gameSpeed);

            // The shared bank, before anything can ask for one: it is what
            // answers a character that was packaged without its own .snd, which
            // on this roster is all but one of them. A character's own bank is
            // added ahead of it when a match *starts*, because that is when the
            // two fighters are known.
            mixer_.addBank(&game_.commonSounds());
            if (audio_.start(mixer_, 44100)) {
                AF_LOGI("audio: ready, %zu shared + %zu screen sounds",
                        game_.commonSounds().size(), game_.fightSounds().size());
            }

            pushScreen(std::make_unique<SplashScreen>(game_));
        }
    } else {
        AF_LOGW("no file source: the engine has no game data to read");
    }

    AF_LOGI("engine created (%dx%d framebuffer)", framebufferWidth, framebufferHeight);
    return true;
}

void Engine::shutdown() {
    audio_.stop();
    while (!stack_.empty()) {
        stack_.back()->leave();
        stack_.pop_back();
    }
    renderer_.shutdown();
    created_ = false;
    AF_LOGI("engine shut down");
}

void Engine::resize(int framebufferWidth, int framebufferHeight) {
    if (!created_) return;
    renderer_.setFramebufferSize(framebufferWidth, framebufferHeight);
}

void Engine::pushScreen(std::unique_ptr<Screen> screen) {
    pendingPush_.push_back(std::move(screen));
}

void Engine::popScreen() { ++pendingPop_; }

void Engine::replaceScreen(std::unique_ptr<Screen> screen) {
    // Deferred exactly like a push so it is safe to call from inside update().
    pendingPop_ += 1;
    pendingPush_.push_back(std::move(screen));
}

void Engine::applyPendingScreenChanges() {
    for (int i = 0; i < pendingPop_; ++i) {
        if (stack_.empty()) break;
        stack_.back()->leave();
        stack_.pop_back();
    }
    pendingPop_ = 0;

    for (auto& screen : pendingPush_) {
        stack_.push_back(std::move(screen));
        // Hand the screen its way back into the flow before letting it look at
        // anything, so enter() can already move along the stack if it wants to.
        stack_.back()->setEngine(this);
        stack_.back()->enter();
    }
    pendingPush_.clear();

    // A finger held across a screen change must not read as a fresh press on the
    // screen that arrives: pressing START to leave a menu would otherwise confirm
    // whatever the next screen was sitting on, and -- because that screen's
    // confirm changes the screen again -- the two would trade places for as long
    // as the finger stayed down.
    //
    // Pretending the buttons were already held last tick suppresses the press
    // edge while still letting the release through when the finger lifts.
    Screen* top = stack_.empty() ? nullptr : stack_.back().get();
    if (top != lastTop_) {
        lastTouch_ = touchHeld_;
        lastTop_ = top;
    }
}

void Engine::step(double fixedDelta) {
    // Snapshot input under the lock, then convert the accumulated event masks
    // into this tick's edges.
    std::vector<touchpad::Contact> contacts;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        input_.held = pendingHeld_;
        input_.pressed = pendingPressed_;
        input_.released = pendingReleased_;
        pendingPressed_ = 0;
        pendingReleased_ = 0;

        contacts.reserve(pointers_.size());
        for (const auto& entry : pointers_) contacts.push_back(entry.second.contact);
        // A lifted finger is dropped only now that it has been sampled, so a tap
        // shorter than a tick still reaches the pad below.
        for (auto it = pointers_.begin(); it != pointers_.end();) {
            it = it->second.lifted ? pointers_.erase(it) : std::next(it);
        }
    }

    // The fingers stay in device pixels: the pad is laid out as fractions of the
    // screen, not in the game's own coordinate space, so that a control is the same
    // size under the thumb however the game's screen happens to be letterboxed.
    const bool pad = !stack_.empty() && stack_.back()->acceptsTouchInput();
    touchHeld_ = pad ? pad_.buttonsFor(contacts, static_cast<float>(renderer_.framebufferWidth()),
                                      static_cast<float>(renderer_.framebufferHeight()))
                     : 0u;
    input_.touched = pad && !contacts.empty();

    // The pad has to synthesise its own edges: a finger is a level, and a screen
    // that reacts to `wasPressed` would otherwise never see one.
    input_.held |= touchHeld_;
    input_.pressed |= touchHeld_ & ~lastTouch_;
    input_.released |= lastTouch_ & ~touchHeld_;
    lastTouch_ = touchHeld_;

    if (!stack_.empty()) {
        // The fingers themselves first, for the one screen that wants them: the pad's
        // layout editor, which edits the pad and so cannot be handed a button mask.
        stack_.back()->touch(contacts);
        stack_.back()->update(fixedDelta, input_);
    }

    applyPendingScreenChanges();
}

void Engine::frame(double deltaSeconds) {
    if (!created_) return;

    if (!(deltaSeconds > 0.0) || deltaSeconds > 0.25) {
        // First frame, or the app was paused: advance exactly one tick rather
        // than fast-forwarding through hundreds.
        deltaSeconds = kFixedStep;
    }

    accumulator_ += deltaSeconds;
    int steps = 0;
    while (accumulator_ >= kFixedStep && steps < kMaxStepsPerFrame) {
        step(kFixedStep);
        accumulator_ -= kFixedStep;
        ++steps;
    }
    if (steps == kMaxStepsPerFrame) {
        // Give up on the backlog; the device cannot keep up at 60 Hz.
        accumulator_ = 0.0;
    }

    renderer_.beginFrame(Color::rgba8(0, 0, 0));
    if (!stack_.empty()) {
        Screen* top = stack_.back().get();
        top->draw(renderer_);
        // Last, so the pad is never hidden by a screen's own overlays. The frame
        // time goes with it: the pad fades itself down while nothing is being
        // touched, and how far that has got depends on how long it has been.
        if (top->wantsTouchPad()) {
            pad_.draw(renderer_, game_.fontTable(true), touchHeld_,
                      static_cast<float>(renderer_.framebufferWidth()),
                      static_cast<float>(renderer_.framebufferHeight()), deltaSeconds,
                      top->fadesTouchPad());
        }
    } else {
        drawDiagnosticPattern();
    }
    renderer_.endFrame();

    fpsWindow_ += deltaSeconds;
    ++fpsFrames_;
    if (fpsWindow_ >= 5.0) {
        // Audio goes in the once-a-second line with the frame rate because the
        // two are the same question: "is the engine actually running?" A mixer
        // whose block count is not moving is silent for a reason the log can
        // name, and a miss count says a sound was asked for and not found.
        AF_LOGI("%.1f fps (fb %dx%d, virtual %.0fx%.0f, audio %llu blocks / %llu played / %llu missed)",
                fpsFrames_ / fpsWindow_,
                 renderer_.framebufferWidth(), renderer_.framebufferHeight(),
                 renderer_.virtualWidth(), renderer_.virtualHeight(),
                 static_cast<unsigned long long>(audio_.blocksServed()),
                 static_cast<unsigned long long>(mixer_.started()),
                 static_cast<unsigned long long>(mixer_.misses()));
        fpsWindow_ = 0.0;
        fpsFrames_ = 0;
    }
}

void Engine::pushInput(uint32_t button, bool isDown) {
    std::lock_guard<std::mutex> lock(inputMutex_);
    if (isDown) {
        if ((pendingHeld_ & button) == 0) pendingPressed_ |= button;
        pendingHeld_ |= button;
    } else {
        if ((pendingHeld_ & button) != 0) pendingReleased_ |= button;
        pendingHeld_ &= ~button;
    }
}

// `id` is Android's pointer id, which is stable for the life of a contact.
// Tracking them separately is what lets a player hold a direction with one
// thumb and press an attack with the other.
void Engine::setPointer(int id, float framebufferX, float framebufferY, bool touching) {
    std::lock_guard<std::mutex> lock(inputMutex_);
    if (touching) {
        pointers_[id] = Finger{touchpad::Contact{id, framebufferX, framebufferY}, false};
    } else if (auto it = pointers_.find(id); it != pointers_.end()) {
        // Flagged rather than erased; step() clears it once the contact has been
        // sampled. See Engine::Finger.
        it->second.lifted = true;
    }
}

// Drawn when the screen stack is empty. Doubles as a rendering self-test: if
// the border below is not a crisp axis-aligned rectangle inset from the screen
// edges, the virtual-coordinate transform is wrong.
void Engine::drawDiagnosticPattern() {
    Renderer& r = renderer_;
    const float vw = r.virtualWidth();
    const float vh = r.virtualHeight();

    // Sky-ish vertical gradient, so banding and channel order are both obvious.
    constexpr int kBands = 32;
    for (int i = 0; i < kBands; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kBands - 1);
        const float y = vh * static_cast<float>(i) / static_cast<float>(kBands);
        const float h = vh / static_cast<float>(kBands) + 1.0f;
        r.fillRect(0.0f, y, vw, h,
                   Color{0.06f + 0.18f * t, 0.07f + 0.10f * t, 0.16f + 0.30f * t, 1.0f});
    }

    // 16-unit grid, so the scale of the virtual space is readable at a glance.
    for (float x = 0.0f; x <= vw; x += 16.0f) {
        r.fillRect(x, 0.0f, 1.0f, vh, Color{1.0f, 1.0f, 1.0f, 0.07f});
    }
    for (float y = 0.0f; y <= vh; y += 16.0f) {
        r.fillRect(0.0f, y, vw, 1.0f, Color{1.0f, 1.0f, 1.0f, 0.07f});
    }

    // The reference rectangle: exactly the virtual space.
    const Color edge = Color::rgba8(90, 220, 160);
    r.fillRect(0.0f, 0.0f, vw, 2.0f, edge);
    r.fillRect(0.0f, vh - 2.0f, vw, 2.0f, edge);
    r.fillRect(0.0f, 0.0f, 2.0f, vh, edge);
    r.fillRect(vw - 2.0f, 0.0f, 2.0f, vh, edge);

    // Corner ticks mark the safe area MUGEN keeps HUD inside.
    const Color tick = Color::rgba8(240, 200, 60);
    const float t = 4.0f;
    const float len = 18.0f;
    r.fillRect(t, t, len, 3.0f, tick);
    r.fillRect(t, t, 3.0f, len, tick);
    r.fillRect(vw - t - len, t, len, 3.0f, tick);
    r.fillRect(vw - t - 3.0f, t, 3.0f, len, tick);
    r.fillRect(t, vh - t - 3.0f, len, 3.0f, tick);
    r.fillRect(t, vh - t - len, 3.0f, len, tick);
    r.fillRect(vw - t - len, vh - t - 3.0f, len, 3.0f, tick);
    r.fillRect(vw - t - 3.0f, vh - t - len, 3.0f, len, tick);

    // Centre crosshair, to confirm the origin is the top-left of the virtual space.
    const Color cross = Color::rgba8(230, 80, 110);
    r.fillRect(vw * 0.5f - 1.0f, 0.0f, 2.0f, vh, cross);
    r.fillRect(0.0f, vh * 0.5f - 1.0f, vw, 2.0f, cross);

    // Markers at the live touch points, in virtual coordinates. Proves that touch
    // input reaches the engine, maps into game space correctly, and keeps each
    // finger separate.
    std::vector<touchpad::Contact> contacts;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        for (const auto& entry : pointers_) contacts.push_back(entry.second.contact);
    }
    for (touchpad::Contact& contact : contacts) {
        r.framebufferToVirtual(contact.x, contact.y, &contact.x, &contact.y);
        const Color marker = Color::rgba8(120, 220, 255);
        r.fillRect(contact.x - 9.0f, contact.y - 1.5f, 18.0f, 3.0f, marker);
        r.fillRect(contact.x - 1.5f, contact.y - 9.0f, 3.0f, 18.0f, marker);
        r.drawSprite(r.dot(), contact.x - 6.0f, contact.y - 6.0f, Rect{0, 0, 12, 12},
                     Color{1, 1, 1, 1});
    }
}

// ------------------------------------------------------------------- settings

void Engine::loadSettings() {
    if (settingsPath_.empty()) {
        AF_LOGI("no settings path; the on-screen pad will use its shipped layout");
        return;
    }
    // A missing settings file is the normal state of a fresh install, not an error,
    // so this reads quietly and says what it did.
    std::ifstream file(settingsPath_, std::ios::binary);
    if (!file) {
        AF_LOGI("no saved settings at %s; the on-screen pad will use its shipped layout",
                settingsPath_.c_str());
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();
    if (text.empty()) return;

    const IniFile document = IniFile::parse(text);
    pad_.load(document);
    AF_LOGI("settings loaded from %s (%zu bytes): pad at %.3f,%.3f", settingsPath_.c_str(),
            text.size(), pad_.layout().attacks.x, pad_.layout().attacks.y);
}

void Engine::saveSettings() const {
    if (settingsPath_.empty()) return;
    // Written whole rather than edited in place: a settings file is a few lines, and a
    // half-written one is worse than a missing one.
    std::ofstream file(settingsPath_, std::ios::binary | std::ios::trunc);
    if (!file) {
        AF_LOGW("could not write settings to %s", settingsPath_.c_str());
        return;
    }
    const std::string text = pad_.settingsText();
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    AF_LOGI("settings saved to %s (%zu bytes)", settingsPath_.c_str(), text.size());
}

}  // namespace af

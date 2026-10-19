#include "engine/screens/splash_screen.h"

#include <algorithm>
#include <cstdio>
#include <memory>

#include "core/log.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "engine/screens/title_screen.h"
#include "gfx/font.h"

namespace af {

namespace {

constexpr Color kNameColor = Color{1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color kDimColor = Color::rgba8(150, 160, 185);
constexpr Color kBarBackColor = Color{0.05f, 0.06f, 0.10f, 0.85f};
constexpr Color kBarFillColor = Color::rgba8(120, 220, 255);
constexpr Color kBarEdgeColor = Color::rgba8(60, 110, 150);

// How many ticks of the fade each end of the splash takes.
constexpr long long kFadeTicks = 20;

// A screenpack may name no fonts, or name a slot that failed to load; the
// smallest font always exists in the generated screenpack, so it is the
// fallback every read goes through.
Font* anyFont(const FontTable& fonts, int wanted) {
    if (Font* font = fonts.get(wanted)) return font;
    for (int i = 1; i <= 10; ++i) {
        if (Font* font = fonts.get(i)) return font;
    }
    return nullptr;
}

// Whether anything at all was pressed. A splash the player cannot skip is an
// obstacle once they have seen it, and every pad in the game has at least six
// buttons to press.
bool anyInput(const InputState& input) {
    return input.touched || input.pressed != 0;
}

// A file path turned into something worth reading on a loading screen.
//
// `dbz_piccolo/dbz_piccolo.def` is what the loader knows, and it says the same
// word twice and ends in an extension. Stripping the extension and then keeping
// only the last path component leaves the name the game itself uses: the folder
// for a character, and the file for a stage.
std::string friendlyName(std::string path) {
    if (const size_t dot = path.rfind('.'); dot != std::string::npos && dot > 0) {
        path = path.substr(0, dot);
    }
    if (const size_t slash = path.rfind('/'); slash != std::string::npos) {
        path = path.substr(slash + 1);
    }
    return path;
}

}  // namespace

SplashScreen::SplashScreen(Game& game) : game_(game) {}

void SplashScreen::update(double dt, const InputState& input) {
    (void)dt;
    ++tick_;
    if (tick_ >= kSplashTicks || anyInput(input)) {
        AF_LOGI("splash: %s after %lld ticks", tick_ >= kSplashTicks ? "done" : "skipped", tick_);
        engine()->replaceScreen(std::make_unique<LoadingScreen>(game_));
    }
}

void SplashScreen::draw(Renderer& renderer) {
    const SystemDef& system = game_.system();
    drawBackground(renderer, system.titleBg, game_.systemSprites(), system.animations, tick_);

    const float width = renderer.virtualWidth();
    const float height = renderer.virtualHeight();
    const FontTable fonts = game_.fontTable(false);

    if (Font* font = anyFont(fonts, system.title.menuItemActiveFont.font)) {
        font->draw(renderer, width * 0.5f, height * 0.42f, "ANIFIGHT", 0, TextAlign::Center,
                   kNameColor);
    }
    if (Font* small = anyFont(fonts, 1)) {
        // The screenpack is named because the art behind this card is its, not
        // ours, and a player who installed one should see which.
        if (!system.name.empty()) {
            small->draw(renderer, width * 0.5f, height * 0.52f, system.name.c_str(), 0,
                        TextAlign::Center, kDimColor);
        }
        small->draw(renderer, width * 0.5f, height * 0.78f, "TAP TO CONTINUE", 0, TextAlign::Center,
                    kDimColor);
    }

    // Fade up from black and back down again, so the card does not appear and
    // vanish mid-picture.
    float alpha = 0.0f;
    if (tick_ < kFadeTicks) {
        alpha = 1.0f - static_cast<float>(tick_) / static_cast<float>(kFadeTicks);
    } else if (tick_ > kSplashTicks - kFadeTicks) {
        alpha = static_cast<float>(tick_ - (kSplashTicks - kFadeTicks)) /
                static_cast<float>(kFadeTicks);
    }
    if (alpha > 0.0f) {
        renderer.fillRect(0.0f, 0.0f, width, height, Color{0.0f, 0.0f, 0.0f, alpha * 0.9f});
    }
}

// ----------------------------------------------------------- loading screen

LoadingScreen::LoadingScreen(Game& game) : game_(game) {
    done_ = game_.loadDoneCount();
    total_ = game_.loadTotalCount();
}

void LoadingScreen::update(double dt, const InputState& input) {
    (void)dt;
    (void)input;
    ++tick_;

    // Collect whatever the worker has finished since the last frame and return
    // immediately, so this frame gets drawn. The work itself is not here -- it
    // is on the load worker -- which is what lets the bar move every frame
    // instead of jumping ten seconds at a time.
    game_.stepLoad();

    done_ = game_.loadDoneCount();
    total_ = game_.loadTotalCount();
    item_ = game_.loadItem();

    if (game_.loadDone()) {
        AF_LOGI("loading: finished after %lld ticks (%d items)", tick_, done_);
        engine()->replaceScreen(std::make_unique<TitleScreen>(game_));
    }
}

void LoadingScreen::draw(Renderer& renderer) {
    const float width = renderer.virtualWidth();
    const float height = renderer.virtualHeight();
    const FontTable fonts = game_.fontTable(false);

    // The screenpack's own title art, dimmed almost to black. Loading screens
    // that show nothing are the ones players think have crashed; showing the
    // art behind a dark wash says "this is the game, it is getting ready".
    drawBackground(renderer, game_.system().titleBg, game_.systemSprites(),
                   game_.system().animations, tick_);
    renderer.fillRect(0.0f, 0.0f, width, height, Color{0.0f, 0.0f, 0.0f, 0.82f});

    if (Font* font = anyFont(fonts, game_.system().title.menuItemActiveFont.font)) {
        font->draw(renderer, width * 0.5f, height * 0.34f, "LOADING", 0, TextAlign::Center,
                   kNameColor);
    }

    // The bar. Its width is a fraction of the screen rather than a fixed count
    // of pixels, so a screenpack authored for 640x480 gets the same bar as one
    // authored for 320x240.
    const float barWidth = width * 0.62f;
    const float barHeight = 8.0f;
    const float barX = (width - barWidth) * 0.5f;
    const float barY = height * 0.50f;
    renderer.fillRect(barX, barY, barWidth, barHeight, kBarBackColor);
    renderer.fillRect(barX, barY, barWidth, 1.0f, kBarEdgeColor);
    renderer.fillRect(barX, barY + barHeight - 1.0f, barWidth, 1.0f, kBarEdgeColor);

    const float fraction = total_ > 0 ? static_cast<float>(done_) / static_cast<float>(total_) : 1.0f;
    const float filled = barWidth * std::clamp(fraction, 0.0f, 1.0f);
    if (filled > 0.0f) renderer.fillRect(barX, barY + 1.0f, filled, barHeight - 2.0f, kBarFillColor);

    if (Font* small = anyFont(fonts, 1)) {
        char line[160];
        // The count sits above the bar's right end and the name below it, so
        // the two lines cannot collide however long a name is.
        if (total_ > 0) {
            std::snprintf(line, sizeof(line), "%d / %d", done_, total_);
        } else {
            std::snprintf(line, sizeof(line), "PREPARING");
        }
        small->draw(renderer, barX + barWidth, barY - 11.0f, line, 0, TextAlign::Right, kDimColor);

        // The name of the thing being read. A slow character takes seconds and
        // the bar cannot move during it, so the spinner beside the name is what
        // says the app is still working rather than stuck.
        if (!item_.empty()) {
            static const char kSpin[] = "|/-\\";
            const char spin = kSpin[static_cast<size_t>(tick_ / 4) % 4];
            const std::string name = friendlyName(item_);
            std::snprintf(line, sizeof(line), "%c  %s", spin, name.c_str());
            small->draw(renderer, width * 0.5f, barY + barHeight + 8.0f, line, 0,
                        TextAlign::Center, kDimColor);
        }
    }
}

}  // namespace af

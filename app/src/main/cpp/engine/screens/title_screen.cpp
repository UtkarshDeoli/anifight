#include "engine/screens/title_screen.h"

#include <algorithm>
#include <memory>

#include "core/log.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "engine/screens/fight_screen.h"
#include "engine/screens/layout_screen.h"
#include "engine/screens/select_screen.h"
#include "gfx/font.h"

namespace af {

namespace {

// MUGEN's title menu order. An entry absent from `menu.itemname.*`, or present
// with an empty value, is skipped -- that is the screenpack saying "no".
constexpr const char* kMenuOrder[] = {
    "arcade", "versus",  "teamarcade",  "teamversus", "teamcoop", "survival",
    "survivalcoop", "training", "watch", "options", "exit",
};

constexpr Color kCursorColor = Color::rgba8(120, 220, 255);
constexpr Color kTextColor = Color{1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color kHintColor = Color::rgba8(140, 150, 175);

// Buttons that mean "yes". MUGEN's title menu accepts any of the attack buttons
// plus Start, because a pad wired for fighting has no "confirm" as such.
bool confirmPressed(const InputState& input) {
    return input.wasPressed(Button::kStart) || input.wasPressed(Button::kA) ||
           input.wasPressed(Button::kB) || input.wasPressed(Button::kC) ||
           input.wasPressed(Button::kX) || input.wasPressed(Button::kY) ||
           input.wasPressed(Button::kZ);
}

}  // namespace

TitleScreen::TitleScreen(Game& game) : game_(game) { buildMenu(); }

void TitleScreen::buildMenu() {
    items_.clear();
    const std::map<std::string, std::string>& names = game_.system().title.itemNames;
    for (const char* key : kMenuOrder) {
        const auto it = names.find(key);
        if (it == names.end() || it->second.empty()) continue;
        items_.push_back(MenuItem{key, it->second});
    }

    AF_LOGI("title menu: %zu entries", items_.size());
    if (items_.empty()) {
        // Better to leave the player something to press than to sit on a screen
        // with no way forward.
        items_.push_back(MenuItem{"exit", "EXIT"});
        AF_LOGW("screenpack defines no menu items; offering only EXIT");
    }
}

int TitleScreen::visibleItemCount() const {
    const int wanted = game_.system().title.menuWindowVisibleItems;
    if (wanted <= 0) return static_cast<int>(items_.size());
    return std::min(wanted, static_cast<int>(items_.size()));
}

void TitleScreen::update(double dt, const InputState& input) {
    (void)dt;
    ++tick_;
    if (fade_ < game_.system().title.fadeInTime) ++fade_;

    if (items_.empty()) return;

    // Once an item has been chosen the menu stops responding and the screen
    // fades out; `activate` runs when the fade is over.
    if (leaving_) {
        ++leaveTick_;
        if (leaveTick_ >= game_.system().title.fadeOutTime) activate();
        return;
    }

    const int last = static_cast<int>(items_.size()) - 1;
    const int before = selected_;
    // MUGEN's title menu does not wrap: pushing past the end does nothing.
    if (input.wasPressed(Button::kUp)) selected_ = std::max(0, selected_ - 1);
    if (input.wasPressed(Button::kDown)) selected_ = std::min(last, selected_ + 1);

    if (engine() != nullptr && selected_ != before) {
        engine()->mixer().playFrom(game_.systemSounds(), 100, 0);
    }

    // Scroll the window only as far as it takes to keep the cursor inside it.
    const int visible = visibleItemCount();
    if (selected_ < firstVisible_) firstVisible_ = selected_;
    if (selected_ >= firstVisible_ + visible) firstVisible_ = selected_ - visible + 1;

    if (confirmPressed(input)) {
        // A zero fade means the screenpack wants the switch on the same frame.
        if (game_.system().title.fadeOutTime <= 0) {
            activate();
            return;
        }
        leaving_ = true;
        leaveTick_ = 0;
    }
}

void TitleScreen::activate() {
    if (selected_ < 0 || selected_ >= static_cast<int>(items_.size())) return;
    if (engine() == nullptr) return;

    // The title is not popped when it pushes its child screen, so it can become
    // visible again later: the end of an arcade run returns here by popping the
    // fight and select screens. Clearing the leave state is what makes that
    // return a fresh menu rather than the tail of the last fade-out -- without
    // it, the first tick after the return re-runs `activate()` and throws the
    // player straight back into the mode they just lost.
    leaving_ = false;
    leaveTick_ = 0;

    const std::string& key = items_[static_cast<size_t>(selected_)].key;
    AF_LOGI("title menu: chose '%s'", key.c_str());

    if (key == "exit") {
        engine()->requestQuit();
        return;
    }
    if (key == "options") {
        // The options screen proper is a MUGEN `options.def` menu this screenpack does
        // not ship, so what is behind the entry is the one setting the game actually
        // has: where the on-screen controls sit. That is a real screen rather than a
        // refusal, which is the difference between a menu item and a dead end.
        engine()->pushScreen(std::make_unique<LayoutScreen>());
        return;
    }
    if (key == "training" || key == "watch") {
        // Both are "put some fighters on screen without a roster", which is what
        // the engine test exists for.
        engine()->pushScreen(std::make_unique<FightScreen>(game_, true));
        return;
    }

    // arcade, versus and the team modes all begin at character select.
    engine()->pushScreen(std::make_unique<SelectScreen>(game_, key));
}

void TitleScreen::draw(Renderer& renderer) {
    const SystemDef& system = game_.system();
    const TitleInfo& title = system.title;

    drawBackground(renderer, system.titleBg, game_.systemSprites(), system.animations, tick_);

    const FontTable fonts = game_.fontTable(false);
    Font* menuFont = fonts.get(title.menuItemFont.font);
    if (menuFont == nullptr) menuFont = fonts.get(title.menuItemActiveFont.font);

    const int visible = visibleItemCount();
    const float lineStep = title.menuItemSpacing.y > 0.0f ? title.menuItemSpacing.y : 12.0f;
    const float lineHeight =
        menuFont != nullptr ? static_cast<float>(menuFont->lineHeight()) : lineStep;

    // The window is the band of the background the screenpack left clear for the
    // menu. Anything outside it is clipped rather than drawn over the art.
    const float windowTop = title.menuPos.y - title.menuWindowMargins.x;
    const float windowBottom =
        title.menuPos.y + static_cast<float>(visible - 1) * lineStep + lineHeight +
        title.menuWindowMargins.y;
    const bool scrolls = static_cast<int>(items_.size()) > visible;
    if (scrolls) {
        renderer.pushClip(0.0f, windowTop, renderer.virtualWidth(), windowBottom);
    }

    for (int i = 0; i < visible; ++i) {
        const int index = firstVisible_ + i;
        if (index >= static_cast<int>(items_.size())) break;

        const float y = title.menuPos.y + static_cast<float>(i) * lineStep;
        const bool active = index == selected_;

        // MUGEN's menu item position is the text *baseline*, not the top of the
        // glyph. The distribution motif's `menu.boxcursor.coords = -58,-10,57,2`
        // frames a ten-unit ascent exactly when those coordinates are measured
        // from the baseline, and the big motif's `-35,+6` does the same for its
        // larger font. This engine's font draws from the top, so the label is
        // lifted by its ascent; the box cursor below stays on the baseline
        // anchor. Without the lift the cursor sat a whole line above the item it
        // was marking.
        const float ascent =
            menuFont != nullptr ? static_cast<float>(menuFont->lineHeight()) : 0.0f;

        if (menuFont != nullptr) {
            Font* font = active ? menuFont : fonts.get(title.menuItemFont.font);
            if (font == nullptr) font = menuFont;
            font->draw(renderer, title.menuPos.x, y - ascent, items_[static_cast<size_t>(index)].label,
                       active ? title.menuItemActiveFont.bank : title.menuItemFont.bank,
                       textAlignFromInt(title.menuItemFont.align), kTextColor);
        }

        // The box cursor marks the active item. Screenpacks that use one draw a
        // frame around the text instead of a second colour bank, because a font
        // is not required to have more than one bank.
        if (active && title.boxCursorVisible) {
            const Rect4& coords = title.boxCursorCoords;
            const float left = title.menuPos.x + coords[0];
            const float top = y + coords[1];
            const float right = title.menuPos.x + coords[2];
            const float bottom = y + coords[3];
            const float thickness = 1.0f;
            renderer.fillRect(left, top, right - left, thickness, kCursorColor);
            renderer.fillRect(left, bottom - thickness, right - left, thickness, kCursorColor);
            renderer.fillRect(left, top, thickness, bottom - top, kCursorColor);
            renderer.fillRect(right - thickness, top, thickness, bottom - top, kCursorColor);
        }
    }

    if (scrolls) renderer.popClip();

    // A hint line, which is a small addition of ours: with no characters there
    // is nothing else on screen to suggest what the game is waiting for.
    if (Font* hint = fonts.get(1)) {
        hint->draw(renderer, renderer.virtualWidth() * 0.5f, 96.0f,
                   "UP/DOWN TO CHOOSE - START TO CONFIRM", 0, TextAlign::Center, kHintColor);
    }

    // Fade in from black, as `fadein.time` asks for.
    if (title.fadeInTime > 0 && fade_ < title.fadeInTime) {
        const float alpha = 1.0f - static_cast<float>(fade_) / static_cast<float>(title.fadeInTime);
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, alpha});
    }

    // And back out, as `fadeout.time` asks for, once an item has been chosen.
    if (leaving_ && title.fadeOutTime > 0) {
        const float alpha = std::min(1.0f, static_cast<float>(leaveTick_) /
                                            static_cast<float>(title.fadeOutTime));
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, alpha});
    }
}

}  // namespace af

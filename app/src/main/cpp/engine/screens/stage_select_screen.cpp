#include "engine/screens/stage_select_screen.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "core/log.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "engine/stage.h"
#include "gfx/font.h"

namespace af {

namespace {

// Rows the list shows at once. The screens are authored for 320x240 and the
// on-screen pad covers the bottom band, so the list keeps to the same space the
// character grid uses above the player panels.
constexpr int kVisibleRows = 9;
constexpr float kHeadingY = 22.0f;
constexpr float kListTop = 46.0f;
constexpr float kRowStep = 14.0f;

constexpr Color kSelectedColor = Color::rgba8(120, 220, 255);
constexpr Color kTextColor = Color{1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color kHintColor = Color::rgba8(140, 150, 175);

}  // namespace

StageSelectScreen::StageSelectScreen(Game& game, FightFactory onChoose,
                                     CancelHandler onCancel, int initialIndex)
    : game_(game), onChoose_(std::move(onChoose)), onCancel_(std::move(onCancel)) {
    // Only stages that loaded. A stage named in select.def but missing from the
    // install is dropped at load time, and offering a row that starts a fight
    // nowhere is worse than not offering it.
    for (const std::unique_ptr<Stage>& stage : game_.stages()) {
        if (stage != nullptr && stage->loaded()) stages_.push_back(stage.get());
    }
    if (stageCount() > 0) {
        selected_ = std::clamp(initialIndex, 0, stageCount() - 1);
    }
}

void StageSelectScreen::enter() {
    AF_LOGI("stage select: %d stage(s) available", stageCount());
}

Stage* StageSelectScreen::stageAt(int index) const {
    if (index < 0 || index >= stageCount()) return nullptr;
    return stages_[static_cast<size_t>(index)];
}

std::string StageSelectScreen::labelFor(int index) const {
    const Stage* stage = stageAt(index);
    if (stage == nullptr) return {};
    const StageDef& def = stage->def();
    // `displayname` already falls back to `name` in the parser, but a stage
    // that leaves both empty still deserves a row rather than a blank line.
    if (!def.displayName.empty()) return def.displayName;
    return stage->defPath();
}

void StageSelectScreen::choose(Stage* stage) {
    if (launched_) return;
    launched_ = true;

    if (engine() == nullptr) return;
    if (onChoose_) {
        std::unique_ptr<Screen> next = onChoose_(stage);
        if (next != nullptr) {
            // Replace rather than pop-then-push: both are deferred, but
            // replaceScreen is one stack operation and leaves no window in
            // which the select screen would briefly be the top.
            engine()->replaceScreen(std::move(next));
            return;
        }
    }
    // No fight was built (no stages loaded, or a factory that declined): going
    // back is the only honest move, and it never leaves a dead screen up.
    engine()->popScreen();
}

void StageSelectScreen::update(double dt, const InputState& input) {
    (void)dt;
    ++tick_;

    if (launched_) return;

    if (stageCount() == 0) {
        // Nothing to choose. The fight screen still has its procedural arena,
        // so the run continues rather than dead-ending here.
        AF_LOGW("stage select: no stages loaded; using the fight's own arena");
        choose(nullptr);
        return;
    }

    if (input.wasPressed(Button::kUp)) {
        selected_ = std::max(0, selected_ - 1);
    }
    if (input.wasPressed(Button::kDown)) {
        selected_ = std::min(stageCount() - 1, selected_ + 1);
    }

    // Keep the cursor inside the visible window.
    if (selected_ < firstVisible_) firstVisible_ = selected_;
    if (selected_ >= firstVisible_ + kVisibleRows) firstVisible_ = selected_ - kVisibleRows + 1;
    firstVisible_ = std::clamp(firstVisible_, 0, std::max(0, stageCount() - kVisibleRows));

    if (input.wasPressed(Button::kLeft)) {
        selected_ = std::max(0, selected_ - kVisibleRows);
    }
    if (input.wasPressed(Button::kRight)) {
        selected_ = std::min(stageCount() - 1, selected_ + kVisibleRows);
    }

    // B backs out, the same button the character select screen uses. This
    // screen has no palette chord, so Start can confirm on the press.
    if (input.wasPressed(Button::kB)) {
        if (onCancel_) onCancel_();
        if (engine() != nullptr) engine()->popScreen();
        return;
    }

    const bool confirm = input.wasPressed(Button::kStart) || input.wasPressed(Button::kA) ||
                         input.wasPressed(Button::kC) || input.wasPressed(Button::kX) ||
                         input.wasPressed(Button::kY) || input.wasPressed(Button::kZ);
    if (confirm) {
        Stage* stage = stageAt(selected_);
        AF_LOGI("stage select: chose \"%s\" (%s)",
                stage != nullptr ? stage->def().name.c_str() : "(none)",
                stage != nullptr ? stage->defPath().c_str() : "no stage");
        choose(stage);
    }
}

void StageSelectScreen::draw(Renderer& renderer) {
    if (launched_) return;

    const SystemDef& system = game_.system();
    SpriteBank& sprites = game_.systemSprites();
    const FontTable fonts = game_.fontTable(false);

    // The same background as the character select screen: this screen is the
    // second half of that choice, not a separate place.
    drawBackground(renderer, system.selectBg, sprites, system.animations, tick_);

    Font* font = fonts.get(system.title.menuItemFont.font);
    if (font == nullptr) font = fonts.get(1);
    const float middleX = renderer.virtualWidth() * 0.5f;

    if (font != nullptr) {
        font->draw(renderer, middleX, kHeadingY, "SELECT STAGE", 0, TextAlign::Center,
                   kTextColor);

        for (int row = 0; row < kVisibleRows; ++row) {
            const int index = firstVisible_ + row;
            if (index >= stageCount()) break;

            const float y = kListTop + static_cast<float>(row) * kRowStep;
            const bool active = index == selected_;
            const std::string label = labelFor(index);
            font->draw(renderer, middleX, y, label, 0, TextAlign::Center,
                       active ? kSelectedColor : kTextColor);

            if (active) {
                // An underline reads as a cursor without borrowing a sprite the
                // screenpack may not define for a screen MUGEN does not have.
                const float width = static_cast<float>(font->textWidth(label, 0));
                renderer.fillRect(middleX - width * 0.5f, y + static_cast<float>(font->lineHeight()),
                                  width, 1.0f, kSelectedColor);
            }
        }

        if (stageCount() > kVisibleRows) {
            char counter[32];
            std::snprintf(counter, sizeof(counter), "%d / %d", selected_ + 1, stageCount());
            font->draw(renderer, middleX, kListTop + kVisibleRows * kRowStep + 2.0f, counter, 0,
                       TextAlign::Center, kHintColor);
        }

        font->draw(renderer, middleX, 214.0f,
                   "UP/DOWN TO CHOOSE - A TO FIGHT - B TO GO BACK", 0, TextAlign::Center,
                   kHintColor);
    }
}

}  // namespace af

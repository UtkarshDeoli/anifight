#include "engine/screens/select_screen.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/log.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/motif.h"
#include "engine/screens/fight_screen.h"
#include "engine/screens/stage_select_screen.h"
#include "engine/select_grid.h"
#include "engine/touch_controls.h"
#include "gfx/font.h"

namespace af {

namespace {

constexpr Color kEmptyColor = Color::rgba8(235, 170, 90);
constexpr Color kHintColor = Color::rgba8(140, 150, 175);

std::string upperCase(std::string text) {
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

}  // namespace

SelectScreen::SelectScreen(Game& game, std::string mode) : game_(game), mode_(std::move(mode)) {
    const SelectInfo& info = game_.system().select;
    // startcell is written column,row. Clamp it, because a screenpack is free to
    // name a cell the grid does not have.
    column_ = std::clamp(static_cast<int>(info.p1StartCell.x), 0, std::max(0, info.columns - 1));
    row_ = std::clamp(static_cast<int>(info.p1StartCell.y), 0, std::max(0, info.rows - 1));

    // The starting cell comes from the screenpack, which cannot know how short
    // this game's roster is. If it points at an empty box, walk to the first
    // cell the player could actually have meant.
    if (!canLandOn(row_ * std::max(1, info.columns) + column_)) {
        const int columns = std::max(1, info.columns);
        for (int cell = 0; cell < cellCount(); ++cell) {
            if (!canLandOn(cell)) continue;
            row_ = cell / columns;
            column_ = cell % columns;
            break;
        }
    }

    // The cursor arrives on a fighter without having moved there, so the palette
    // it opens in is set here rather than only in moveCursor.
    if (const Character* start = cursorCharacter()) p1Palette_ = start->defaultPaletteChoice();
}

int SelectScreen::cellCount() const {
    return static_cast<int>(game_.roster().size());
}

bool SelectScreen::canLandOn(int cell) const {
    const int count = cellCount();
    if (cell < 0 || cell >= count) return false;

    const SelectInfo& info = game_.system().select;
    if (info.moveOverEmptyBoxes) return true;

    const RosterCell& entry = game_.roster()[static_cast<size_t>(cell)];
    return entry.character != nullptr || entry.random;
}

void SelectScreen::moveCursor(int deltaRow, int deltaColumn) {
    const int before = row_ * 100 + column_;
    const SelectInfo& info = game_.system().select;
    const int rows = std::max(1, info.rows);
    const int columns = std::max(1, info.columns);

    // With nothing to land on, staying put is the only honest answer.
    const int cells = cellCount();
    if (cells == 0) return;

    // The traversal itself lives in select_grid.cpp, where a host test can
    // reach it: getting it wrong leaves the cursor unable to reach part of the
    // grid without anything else in the build noticing.
    std::vector<uint8_t> landable(static_cast<size_t>(cells), 0);
    for (int cell = 0; cell < cells; ++cell) {
        landable[static_cast<size_t>(cell)] = canLandOn(cell) ? 1 : 0;
    }

    const int landed =
        stepSelectGrid(row_ * columns + column_, deltaRow, deltaColumn, rows, columns,
                       info.wrapping, landable);
    row_ = landed / columns;
    column_ = landed % columns;

    // Moving to a different fighter drops the colour back to their own default.
    // The choice is about *this* cell, and carrying the last one's index across
    // is how a player ends up fighting in a colour they never picked. It opens
    // on the colour the *author* picked rather than on the first alternate,
    // which for a character whose pal.defaults names the sixth palette is the
    // difference between arriving as itself and arriving as someone else.
    if (row_ * 100 + column_ != before) {
        const Character* landed = cursorCharacter();
        activePalette() = landed != nullptr ? landed->defaultPaletteChoice() : 0;
    }

    // A blip as the cursor moves. A menu with no feedback reads as broken, and
    // this is the cheapest honest way to give it some: group 0 of the
    // screenpack's fight.snd is exactly "the screen's own sounds", which is what
    // MUGEN keeps in that group.
    if (engine() != nullptr && row_ * 100 + column_ != before) {
        engine()->mixer().playFrom(game_.systemSounds(), 100, 0);
    }
}

void SelectScreen::update(double dt, const InputState& input) {
    (void)dt;
    ++tick_;
    if (fade_ < game_.system().select.fadeInTime) ++fade_;

    // Start doubles as the palette modifier. Held, it turns the d-pad into a
    // palette picker; tapped on its own, it confirms exactly as it always did.
    //
    // That split costs Start its press-to-confirm, and it has to: if Start
    // confirmed the moment it went down, a player could never get to the d-pad
    // before the pick was already locked. So Start's confirm lands on the
    // release instead, and is dropped when a direction arrived while it was
    // held. The six attack buttons are untouched and still confirm on the
    // press, which is what every pad with buttons actually uses -- the comment
    // below this one already treats Start as the fallback for a pad without
    // them, and that is still exactly what it is.
    const bool startHeld = input.down(Button::kStart);
    if (startHeld != startHeld_) {
        startHeld_ = startHeld;
        if (startHeld) startChord_ = false;
    }
    if (startHeld && (input.wasPressed(Button::kLeft) || input.wasPressed(Button::kRight))) {
        startChord_ = true;
        cyclePalette(input.wasPressed(Button::kRight) ? 1 : -1);
        return;
    }

    // B backs out of the current pick, or out of the screen if nothing is
    // picked yet. Start is not "back" during a pick: an attack button pad has
    // no other confirm, and Start is how a pad without buttons confirms.
    if (input.wasPressed(Button::kB) && p1Cell_ < 0) {
        if (engine() != nullptr) engine()->popScreen();
        return;
    }
    if (input.wasPressed(Button::kB)) {
        cancel();
        return;
    }

    const bool wantsConfirm =
        (input.wasReleased(Button::kStart) && !startChord_) ||
        input.wasPressed(Button::kA) || input.wasPressed(Button::kC) ||
        input.wasPressed(Button::kX) || input.wasPressed(Button::kY) ||
        input.wasPressed(Button::kZ);
    if (wantsConfirm) {
        confirm();
        return;
    }

    // While Start is down the d-pad belongs to the palette, so the cursor
    // holds still -- a chord that half-moved the cursor would be worse than one
    // that does nothing.
    if (startHeld) return;

    if (input.wasPressed(Button::kLeft)) moveCursor(0, -1);
    if (input.wasPressed(Button::kRight)) moveCursor(0, 1);
    if (input.wasPressed(Button::kUp)) moveCursor(-1, 0);
    if (input.wasPressed(Button::kDown)) moveCursor(1, 0);
}

const Character* SelectScreen::cursorCharacter() const {
    const int columns = std::max(1, game_.system().select.columns);
    const int cell = row_ * columns + column_;
    if (cell < 0 || cell >= cellCount()) return nullptr;
    return game_.roster()[static_cast<size_t>(cell)].character.get();
}

int& SelectScreen::activePalette() { return p1Cell_ < 0 ? p1Palette_ : p2Palette_; }

int SelectScreen::activePalette() const { return p1Cell_ < 0 ? p1Palette_ : p2Palette_; }

void SelectScreen::cyclePalette(int delta) {
    const Character* character = cursorCharacter();
    if (character == nullptr) return;
    const int choices = character->paletteChoices();
    if (choices <= 1) return;
    // The choice can be stale -- the cursor may have moved onto a character with
    // fewer alternates than the last one, or player two may have been left on an
    // index that only made sense for the cell player one locked.
    int& choice = activePalette();
    if (choice < 0 || choice >= choices) choice = 0;
    choice = ((choice + delta) % choices + choices) % choices;
    if (engine() != nullptr) engine()->mixer().playFrom(game_.systemSounds(), 100, 0);
}

void SelectScreen::confirm() {
    if (engine() == nullptr) return;
    if (cellCount() == 0) return;

    const int cell = row_ * std::max(1, game_.system().select.columns) + column_;
    if (!canLandOn(cell)) return;

    // A confirmed cell may be empty of characters (a trailing empty box); a
    // confirm on it does nothing, because there is nothing there to fight as.
    const RosterCell& entry = game_.roster()[static_cast<size_t>(cell)];
    if (!entry.random && entry.character == nullptr) return;

    if (p1Cell_ < 0) {
        p1Cell_ = rollCell(cell);
        p1Row_ = row_;
        p1Column_ = column_;
        // A roll lands on a fighter the player never saw, so whatever they were
        // pointing at says nothing about it: it arrives in the colour its author
        // gave it. Without this, a choice made against the random cell's own
        // (absent) alternates would be reinterpreted against the rolled
        // character and hand out an alternate nobody asked for.
        if (p1Cell_ != cell) {
            if (const Character* rolled = game_.rosterCharacter(p1Cell_)) {
                p1Palette_ = rolled->defaultPaletteChoice();
            }
        }
        // Logged because the two ways to get a character that is not the one
        // under the cursor -- a `randomselect` cell, and a cursor that moved
        // without anyone noticing -- look identical from the fight that
        // follows, and only one of them is intended.
        if (engine() != nullptr) engine()->mixer().playFrom(game_.systemSounds(), 100, 1);
        AF_LOGI("select: cursor on cell %d (row %d, column %d) -> player one is cell %d%s",
                cell, row_, column_, p1Cell_,
                p1Cell_ == cell ? "" : " (a random cell rolls a different one)");

        if (mode_ != "versus") {
            // Every non-versus mode fills the second slot itself, from the
            // rest of the roster -- arcade's CPU ladder starts here.
            launch(p1Cell_, -1);
            return;
        }

        // Versus: player two picks with the same pad, from wherever the
        // screenpack started their cursor.
        const SelectInfo& info = game_.system().select;
        column_ = std::clamp(static_cast<int>(info.p2StartCell.x), 0,
                             std::max(0, info.columns - 1));
        row_ = std::clamp(static_cast<int>(info.p2StartCell.y), 0, std::max(0, info.rows - 1));
        if (!canLandOn(row_ * std::max(1, info.columns) + column_)) {
            for (int probe = 0; probe < cellCount(); ++probe) {
                if (!canLandOn(probe)) continue;
                row_ = probe / std::max(1, info.columns);
                column_ = probe % std::max(1, info.columns);
                break;
            }
        }
        // Same as the constructor's: this cursor was placed rather than moved,
        // so it opens on its own fighter's default colour.
        if (const Character* start = cursorCharacter()) p2Palette_ = start->defaultPaletteChoice();
        return;
    }

    launch(p1Cell_, rollCell(cell));
}

void SelectScreen::cancel() {
    if (p1Cell_ >= 0) {
        // Back to player one's pick: the two cursors return to where they
        // were, which is simply player one's own cells.
        row_ = p1Row_;
        column_ = p1Column_;
        p1Cell_ = p1Row_ = p1Column_ = -1;
    }
}

int SelectScreen::rollCell(int cell) const {
    // A randomselect cell resolves to a random playable character at confirm
    // time, which keeps a grid with one random cell honest -- the fighter is
    // not decided until it is locked in.
    if (cell < 0 || cell >= cellCount()) return -1;
    if (!game_.roster()[static_cast<size_t>(cell)].random) return cell;

    const int cells = cellCount();
    int firstPlayable = -1;
    int playable = 0;
    for (int probe = 0; probe < cells; ++probe) {
        const RosterCell& entry = game_.roster()[static_cast<size_t>(probe)];
        if (entry.character == nullptr || probe == cell) continue;
        if (firstPlayable < 0) firstPlayable = probe;
        ++playable;
    }
    if (playable == 0) return firstPlayable;  // may still be -1
    uint32_t seed = static_cast<uint32_t>(tick_ * 2654435761u + 1u);
    seed ^= seed >> 13;
    seed = seed * 1103515245u + 12345u;
    const int pick = static_cast<int>((seed >> 8) % static_cast<uint32_t>(playable));
    int seen = 0;
    for (int probe = 0; probe < cells; ++probe) {
        const RosterCell& entry = game_.roster()[static_cast<size_t>(probe)];
        if (entry.character == nullptr || probe == cell) continue;
        if (seen++ == pick) return probe;
    }
    return firstPlayable;
}

void SelectScreen::launch(int p1Cell, int p2Cell) {
    if (engine() == nullptr) return;
    if (p1Cell < 0) return;

    // Which row of the stage list the cursor should open on: the stage the
    // roster entry would have chosen for this fighter, so a player who does not
    // care keeps exactly the match MUGEN would have given them.
    const auto stageIndex = [this](const Stage* wanted) {
        const std::vector<std::unique_ptr<Stage>>& stages = game_.stages();
        for (size_t i = 0; i < stages.size(); ++i) {
            if (stages[i].get() == wanted) return static_cast<int>(i);
        }
        return 0;
    };
    // The stage list is the second half of this choice, so backing out of it
    // must reopen the choice. In the modes with one pick, that means releasing
    // the lock the confirm took; in versus player one stays locked and player
    // two picks again, which is where their cursor already is.
    StageSelectScreen::CancelHandler onStageCancel;
    if (mode_ != "versus") {
        onStageCancel = [this] {
            p1Cell_ = p1Row_ = p1Column_ = -1;
            p2Palette_ = 0;
        };
    }

    // The colour each side locked in, turned from a *choice index* into the
    // palNo the fight speaks. That conversion has to happen here, against the
    // cell, because the choice counts only the alternates a character actually
    // has -- a character shipping pal1/pal3/pal7 has three choices whose palNos
    // are 1, 3 and 7, not 1, 2 and 3.
    //
    // A cell with nobody on it resolves to 0, which asks the fight for nothing
    // and leaves that fighter in its own `pal.defaults`.
    const auto chosenPalNo = [this](int cell, int choice) {
        const Character* character = game_.rosterCharacter(cell);
        return character != nullptr ? character->palNoForChoice(choice) : 0;
    };

    // Arcade is a ladder, not a match. The player climbs the rest of the roster
    // in grid order, one opponent per win, which is all MUGEN's arcade mode is;
    // every other mode is the single match below.
    if (mode_ == "arcade") {
        ArcadeLadder ladder;
        ladder.player = game_.rosterCharacter(p1Cell);
        ladder.playerPalNo = chosenPalNo(p1Cell, p1Palette_);

        // The rest of the roster, in grid order, is the pool the run draws from.
        std::vector<Character*> pool;
        const int cells = cellCount();
        for (int probe = 0; probe < cells; ++probe) {
            if (probe == p1Cell) continue;
            if (Character* other = game_.rosterCharacter(probe)) pool.push_back(other);
        }
        // A roster of one still gets a run rather than a refusal: it fights
        // itself, which is a mirror match and perfectly playable.
        if (pool.empty()) pool.push_back(ladder.player);

        // `arcade.maxmatches` writes one value per difficulty tier: `6,1,1` is
        // six fights at tier 0, one at tier 1 and one at tier 2. Reading the
        // tiers rather than their total is the difference between a ladder that
        // gets harder and one that is merely long, and MUGEN's arcade mode is
        // the first of those.
        //
        // A run longer than the pool repeats it, which is what MUGEN's own
        // default ladder does against a small roster.
        const std::vector<int>& tiers = game_.select().arcadeMaxMatches;
        for (size_t tier = 0; tier < tiers.size(); ++tier) {
            for (int fight = 0; fight < tiers[tier]; ++fight) {
                ladder.opponents.push_back(pool[ladder.opponents.size() % pool.size()]);
                ladder.difficulty.push_back(static_cast<int>(tier));
            }
        }

        // Nothing configured: one fight per opponent at a steady middling
        // difficulty, rather than a ladder that claims a shape it does not have.
        if (ladder.opponents.empty()) {
            for (Character* other : pool) {
                ladder.opponents.push_back(other);
                ladder.difficulty.push_back(5);
            }
        }

        // The arena is the second half of the choice. The stage screen is only
        // skipped when nothing loaded at all, where the fight falls back to its
        // procedural arena and there is genuinely nothing to pick.
        Stage* preferred = game_.stageForMatch(ladder.player);
        if (game_.stages().empty()) {
            engine()->pushScreen(std::make_unique<FightScreen>(game_, std::move(ladder)));
            return;
        }
        auto onChoose = [&game = game_, ladder = std::move(ladder)](Stage* stage) mutable {
            return std::make_unique<FightScreen>(game, std::move(ladder), stage);
        };
        engine()->pushScreen(std::make_unique<StageSelectScreen>(
            game_, std::move(onChoose), std::move(onStageCancel), stageIndex(preferred)));
        return;
    }

    // The CPU opponent is a different fighter from the player where the roster
    // allows one. A roster with a single playable fighter leaves p2Cell at -1
    // here and the fight falls back to the placeholder box -- arcade is the
    // exception, and returns above: it mirrors a solo roster on purpose,
    // because a run of one fighter beating itself is a match, not a refusal.
    if (p2Cell < 0) {
        const int cells = cellCount();
        for (int probe = 0; probe < cells; ++probe) {
            if (probe == p1Cell) continue;
            if (game_.roster()[static_cast<size_t>(probe)].character != nullptr) {
                p2Cell = probe;
                break;
            }
        }
    }

    Character* p1 = p1Cell >= 0 ? game_.rosterCharacter(p1Cell) : nullptr;
    Character* p2 = p2Cell >= 0 ? game_.rosterCharacter(p2Cell) : nullptr;
    const int p1PalNo = chosenPalNo(p1Cell, p1Palette_);
    const int p2PalNo = chosenPalNo(p2Cell, p2Palette_);

    Stage* preferred = game_.stageForMatch(p1);
    if (game_.stages().empty()) {
        engine()->pushScreen(std::make_unique<FightScreen>(game_, p1, p2, p1PalNo, p2PalNo));
        return;
    }
    auto onChoose = [&game = game_, p1, p2, p1PalNo, p2PalNo](Stage* stage) {
        return std::make_unique<FightScreen>(game, p1, p2, p1PalNo, p2PalNo, stage);
    };
    engine()->pushScreen(std::make_unique<StageSelectScreen>(
        game_, std::move(onChoose), std::move(onStageCancel), stageIndex(preferred)));
}

void SelectScreen::draw(Renderer& renderer) {
    const SystemDef& system = game_.system();
    const SelectInfo& info = system.select;
    SpriteBank& sprites = game_.systemSprites();
    const FontTable fonts = game_.fontTable(false);

    drawBackground(renderer, system.selectBg, sprites, system.animations, tick_);

    // Everything below the roster grid lands in the band the on-screen pad
    // covers, so it is centred on the gap between the pad's thumb clusters
    // rather than on the screen. The screenpack puts the player panels there for
    // the same reason.
    const float middleX = touchpad::screenCentreX(renderer.virtualWidth());

    const int rows = std::max(1, info.rows);
    const int columns = std::max(1, info.columns);
    const float stepX = info.cellSize.x + static_cast<float>(info.cellSpacing);
    const float stepY = info.cellSize.y + static_cast<float>(info.cellSpacing);
    const int cells = cellCount();
    int playable = 0;
    for (int cell = 0; cell < cells; ++cell) {
        const RosterCell& entry = game_.roster()[static_cast<size_t>(cell)];
        if (entry.character != nullptr) ++playable;
    }

    // One pass over the grid, drawing whatever each cell holds. MUGEN fills the
    // boxes first and the portraits on top, but a cell only ever holds one of
    // the three, so a single pass draws the same thing without the overdraw.
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const int cell = row * columns + column;
            const float x = info.pos.x + static_cast<float>(column) * stepX;
            const float y = info.pos.y + static_cast<float>(row) * stepY;

            const RosterCell* entry =
                cell < cells ? &game_.roster()[static_cast<size_t>(cell)] : nullptr;

            if (entry != nullptr && entry->character != nullptr) {
                // The portrait comes out of the character's own sprite file,
                // which is why the roster had to be loaded before this screen
                // could draw anything. A screenpack that sets no portrait.spr
                // gets the grid without portraits rather than a null deref.
                Character& character = *entry->character;
                if (info.portrait.drawsSomething() && info.portrait.sprite.has_value()) {
                    drawSpriteAt(renderer, character.sprites(), info.portrait.sprite->first,
                                 info.portrait.sprite->second, x + info.portrait.offset.x,
                                 y + info.portrait.offset.y, info.portrait.facing,
                                 info.portraitScale.x, info.portraitScale.y);
                }
                continue;
            }

            if (entry != nullptr && entry->random) {
                drawElement(renderer, info.cellRandom, sprites, system.animations, fonts, tick_, x,
                            y);
                continue;
            }

            // A box for a character that is reserved but not installed.
            //
            // `showemptyboxes` is MUGEN's own setting for this, and MUGEN's
            // default is to draw them -- but only inside the roster. A grid is
            // four rows of ten whatever the roster holds, so the cells *past*
            // the last entry are not reserved for anything and a plate there
            // says the opposite: it looks like a row of characters that failed
            // to load. A roster of fourteen on a forty-cell grid drew
            // twenty-six of those, which is a screen two thirds full of black
            // boxes and no way to tell them from a broken install.
            //
            // So the rule is the plate's meaning: it marks a line in select.def
            // whose folder is missing.
            if (entry != nullptr && info.showEmptyBoxes) {
                drawElement(renderer, info.cellBg, sprites, system.animations, fonts, tick_, x, y);
            }
        }
    }

    // Cursors sit at the centre of a cell rather than its corner, which is why
    // the cursor sprites in the screenpack carry a centred axis. A locked pick
    // swaps the active cursor for the done one, so the grid shows which choice
    // belongs to which player.
    if (playable > 0) {
        auto cursorAt = [&](int row, int column, bool done) {
            const float cellCenterX =
                info.pos.x + static_cast<float>(column) * stepX + info.cellSize.x * 0.5f;
            const float cellCenterY =
                info.pos.y + static_cast<float>(row) * stepY + info.cellSize.y * 0.5f;
            const DisplayElement& cursor = done ? info.p1CursorDone : info.p1CursorActive;
            drawElement(renderer, cursor, sprites, system.animations, fonts, tick_, cellCenterX,
                        cellCenterY);
        };

        if (p1Cell_ >= 0) cursorAt(p1Row_, p1Column_, true);
        if (mode_ == "versus" || p1Cell_ < 0) cursorAt(row_, column_, false);

        // The highlighted fighter's name, under the grid where the panels
        // live. While choosing it names the cell under the cursor; once
        // locked, it names the locked fighter.
        int namedCell = p1Cell_ >= 0 ? p1Cell_ : (row_ * columns + column_);
        if (namedCell >= 0 && namedCell < cells) {
            const Character* named =
                game_.roster()[static_cast<size_t>(namedCell)].character.get();
            if (Font* font = fonts.get(1)) {
                if (named != nullptr) {
                    font->draw(renderer, middleX, 176.0f, named->displayName().c_str(), 0,
                               TextAlign::Center, Color{1.0f, 1.0f, 1.0f, 1.0f});
                } else {
                    font->draw(renderer, middleX, 176.0f, "RANDOM", 0, TextAlign::Center,
                               Color::rgba8(120, 220, 255));
                }
            }
        }
    }

    if (info.title.present) {
        drawElement(renderer, info.title, sprites, system.animations, fonts, tick_, 0.0f, 0.0f,
                    upperCase(mode_) + " MODE");
    }

    if (playable == 0 && info.emptyMessage.present) {
        // Our own extension: MUGEN would have refused to start rather than reach
        // this screen with nothing in it.
        drawElement(renderer, info.emptyMessage, sprites, system.animations, fonts, tick_, 0.0f,
                    0.0f);
        if (Font* font = fonts.get(1)) {
            font->draw(renderer, middleX, info.emptyMessage.offset.y + 14.0f,
                       "EDIT data/select.def", 0, TextAlign::Center, kEmptyColor);
        }
    } else if (playable == 0) {
        if (Font* font = fonts.get(1)) {
            font->draw(renderer, middleX, renderer.virtualHeight() * 0.5f,
                       "NO CHARACTERS INSTALLED", 0, TextAlign::Center, kEmptyColor);
        }
    }

    // The colour readout, on its own line between the name below the grid and
    // the hint under it. It is the only feedback the palette picker has: the
    // grid draws each cell's own art, which carries no palette, so nothing on
    // the grid changes when the colour does. It is skipped for the many
    // characters with no alternates, where it could only ever read "1/1".
    const Character* active = cursorCharacter();
    const int activeChoices = active != nullptr ? active->paletteChoices() : 1;
    if (Font* font = fonts.get(1); font != nullptr && activeChoices > 1) {
        const int choice = std::clamp(activePalette(), 0, activeChoices - 1);
        const std::string readout =
            "COLOUR " + std::to_string(choice + 1) + "/" + std::to_string(activeChoices);
        font->draw(renderer, middleX,
                   198.0f - static_cast<float>(font->lineHeight()) - 2.0f, readout, 0,
                   TextAlign::Center, kHintColor);
    }

    if (Font* hint = fonts.get(1)) {
        // The chip teaches the chord only on a fighter that has a colour to
        // choose. Telling a player to hold Start and press left on the twenty
        // characters where that does nothing is worse than saying nothing.
        const char* const text =
            activeChoices > 1 ? (p1Cell_ >= 0 ? "START+L/R COLOUR   A: LOCK IN   B: UNDO"
                                              : "START+L/R COLOUR   A: FIGHT   B: BACK")
                              : (p1Cell_ >= 0 ? "A: LOCK IN FOR P2 PICK - B: UNDO"
                                              : "A: FIGHT - B: BACK");
        hint->draw(renderer, middleX, 198.0f, text, 0, TextAlign::Center, kHintColor);
    }

    if (info.fadeInTime > 0 && fade_ < info.fadeInTime) {
        const float alpha =
            1.0f - static_cast<float>(fade_) / static_cast<float>(info.fadeInTime);
        renderer.fillRect(0.0f, 0.0f, renderer.virtualWidth(), renderer.virtualHeight(),
                          Color{0.0f, 0.0f, 0.0f, alpha});
    }
}

}  // namespace af

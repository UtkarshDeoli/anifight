// The character select screen.
//
// The grid has one cell per select.def roster line, and the cells that are not
// characters -- `randomselect` lines, and the empty tail of the grid -- are
// drawn as such rather than being quietly skipped. MUGEN refuses to start when
// select.def lists nothing, which makes an empty game impossible to look at;
// here the grid draws its empty cells and says so in the middle.
#pragma once

#include <string>

#include "engine/screen.h"
#include "engine/defs.h"

namespace af {

class Character;
class Game;

class SelectScreen : public Screen {
public:
    // `mode` is the title-menu key that opened this screen ("arcade", "versus",
    // ...). It only affects the heading for now; the modes diverge once there
    // are characters to fight with.
    SelectScreen(Game& game, std::string mode);

    void update(double dt, const InputState& input) override;
    void draw(Renderer& renderer) override;

private:
    // Cells in the grid: one per select.def roster line, random-select cells
    // included. Not the same as the number of characters, because a cell with
    // nothing in it is still a cell and still takes up a slot in the grid.
    [[nodiscard]] int cellCount() const;

    // Whether the cursor is allowed to stop on a cell. Every cell is fair game
    // when the screenpack says so; otherwise only the ones with something in
    // them, which is what keeps a two-character roster from making the player
    // walk a cursor across thirty empty boxes.
    [[nodiscard]] bool canLandOn(int cell) const;

    void moveCursor(int deltaRow, int deltaColumn);

    // The character the active cursor is sitting on, or null on a random or
    // empty cell. The palette picker works on whoever this is.
    [[nodiscard]] const Character* cursorCharacter() const;

    // Which alternate the player currently choosing is showing, as a choice
    // index into Character::paletteChoices(). There is one cursor and it changes
    // owner -- player one's until their cell is locked, player two's after --
    // so there is one live choice and one already-locked one, not two of each.
    [[nodiscard]] int& activePalette();
    [[nodiscard]] int activePalette() const;

    // Steps the active player's palette by `delta`, wrapping. Does nothing for
    // the many characters with no alternates at all, so the chord that drives it
    // is harmless everywhere on the grid.
    void cyclePalette(int delta);

    // Confirms the cell under the active cursor. In versus this locks player
    // one's pick and lets the second cursor choose; elsewhere it goes straight
    // to the fight with a CPU opponent drawn from the roster.
    void confirm();
    // Backs out of the current pick, or the screen if nothing is picked.
    void cancel();
    void launch(int p1Cell, int p2Cell);

    // Resolves a confirmed cell to a character index, rolling the dice for a
    // randomselect cell.
    [[nodiscard]] int rollCell(int cell) const;

    Game& game_;
    std::string mode_;
    int row_ = 0;
    int column_ = 0;
    // Player one's locked pick, held while the second cursor chooses. -1.0 for
    // row/column means nothing is picked yet.
    int p1Cell_ = -1;
    int p1Row_ = -1;
    int p1Column_ = -1;
    // The alternates the two players picked, as choice indices rather than
    // palNo values -- see paletteChoices, which is what knows whether a
    // character's alternates are .act files or the sprite bank's own table.
    int p1Palette_ = 0;
    int p2Palette_ = 0;
    // Whether Start was down on the previous frame, and whether a direction
    // arrived while it was. Together these are the palette chord: `start` held
    // turns the d-pad into a palette picker, and a Start that was chording does
    // not also confirm when it comes back up.
    bool startHeld_ = false;
    bool startChord_ = false;
    long long tick_ = 0;
    int fade_ = 0;
};

}  // namespace af

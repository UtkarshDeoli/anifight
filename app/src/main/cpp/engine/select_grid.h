// Where a cursor press lands on the character-select grid.
//
// This lives apart from SelectScreen because it is the one piece of that
// screen with a wrong answer available that nothing else would notice: the
// roster still loads, the grid still draws, and the cursor simply cannot reach
// part of it. Extracted so a host test can drive it without a Game.
#pragma once

#include <cstdint>
#include <vector>

namespace af {

// Steps the cursor once and returns the cell it lands on, or `start` when there
// is nowhere to go.
//
// `landable[cell]` is non-zero for a cell the cursor may stop on; cells past
// the end of the vector are not landable, which is how the empty tail of a grid
// bigger than the roster is handled. The cursor steps one cell at a time rather
// than jumping, because "the next cell the cursor may stop on" is not the same
// as "the next cell" -- with empty boxes skipped, one press can cross several.
//
// A horizontal press walks the grid in *reading order*: crossing an edge
// carries into the next row, or the previous one going left. Wrapping the
// column back into the row it started in instead would circle that row forever,
// and since a character folder that is not installed is a blank cell, a single
// all-blank row would then put every character below it out of reach.
int stepSelectGrid(int start, int deltaRow, int deltaColumn, int rows, int columns, bool wrapping,
                   const std::vector<uint8_t>& landable);

}  // namespace af

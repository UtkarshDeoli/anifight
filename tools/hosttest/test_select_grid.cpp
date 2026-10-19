// Checks that the select-screen cursor can actually reach the roster.
//
// This is the failure mode worth a test: the roster loads, the grid draws, and
// the cursor simply cannot travel past a row that has no character in it -- so
// every fighter below that row is unselectable and nothing anywhere says so.
// It is invisible until someone plays it.
//
// The layout below is the shipped one at the time of writing: a 10x4 grid, 40
// roster lines, of which 19 name a character present on disk (Kung Fu Man, the
// five amarimono characters and thirteen Dragon Ball ones) and 21 are slots
// whose folders are not installed. Those 21 are the empty boxes the cursor has
// to cross, and they sit in one unbroken block in the middle of the grid.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_select_grid \
//         tools/hosttest/test_select_grid.cpp app/src/main/cpp/engine/select_grid.cpp
//     /tmp/test_select_grid

#include <cstdio>
#include <string>
#include <vector>

#include "engine/select_grid.h"

namespace {

int failures = 0;

void check(int actual, int expected, const std::string& what) {
    if (actual == expected) return;
    std::fprintf(stderr, "FAIL: %s (expected cell %d, landed on %d)\n", what.c_str(), expected,
                 actual);
    ++failures;
}

constexpr int kRows = 4;
constexpr int kColumns = 10;
constexpr int kCells = kRows * kColumns;  // 40, the grid's capacity

// The cells that hold a character, in roster order: Kung Fu Man first, then the
// 21 empty slots of the famous-name block, then everything that is installed.
std::vector<int> playableCells() {
    std::vector<int> cells = {0, 22, 23, 24, 25, 26};
    for (int cell = 27; cell < kCells; ++cell) cells.push_back(cell);
    return cells;
}

std::vector<uint8_t> rosterGrid() {
    std::vector<uint8_t> landable(kCells, 0);
    for (int cell : playableCells()) landable[static_cast<size_t>(cell)] = 1;
    return landable;
}

}  // namespace

int main() {
    const std::vector<uint8_t> landable = rosterGrid();
    const std::vector<int> kPlayable = playableCells();
    const int first = kPlayable.front();
    const int last = kPlayable.back();

    std::printf("grid: %dx%d = %d cells, %zu landable\n", kRows, kColumns, kCells,
                kPlayable.size());

    // The bug: Kung Fu Man sits alone in row 0 and the next character is two
    // rows down. A right press has to cross the end of the row to reach him.
    check(af::stepSelectGrid(first, 0, 1, kRows, kColumns, true, landable), 22,
          "one right press from Kung Fu Man reaches the next character, two rows down");

    // And the rest of the row it crossed into.
    check(af::stepSelectGrid(22, 0, 1, kRows, kColumns, true, landable), 23,
          "a right press walks along the row it landed in");

    // Past the last character the remaining cells of the grid are empty, so the
    // cursor carries on out of the roster and wraps to its start.
    check(af::stepSelectGrid(last, 0, 1, kRows, kColumns, true, landable), first,
          "a right press past the last character wraps to the first");

    // The same journey backwards, which has to cross the row boundary the other
    // way: from Kung Fu Man, left goes to the last character, not nowhere.
    check(af::stepSelectGrid(first, 0, -1, kRows, kColumns, true, landable), last,
          "a left press from the first character wraps to the last");

    // With wrapping off, an edge is a wall rather than a seam.
    check(af::stepSelectGrid(first, 0, -1, kRows, kColumns, false, landable), first,
          "without wrapping a left press at the start of the grid stays put");

    // A vertical press stays in its own column, so from Kung Fu Man a down press
    // crosses three blank rows and lands on the character in column 0 of row 3.
    check(af::stepSelectGrid(first, 1, 0, kRows, kColumns, true, landable), 30,
          "a down press crosses blank rows to the next character in the same column");

    // A grid with everything filled is the ordinary case and must still step by
    // one: the carry only matters when the row is empty.
    const std::vector<uint8_t> full(kCells, 1);
    check(af::stepSelectGrid(0, 0, 1, kRows, kColumns, true, full), 1,
          "with a full grid a right press moves exactly one cell");
    check(af::stepSelectGrid(kColumns - 1, 0, 1, kRows, kColumns, true, full), kColumns,
          "with a full grid a right press at a row's end carries into the next row");
    check(af::stepSelectGrid(kColumns, 0, -1, kRows, kColumns, true, full), kColumns - 1,
          "with a full grid a left press at a row's start carries up a row");
    check(af::stepSelectGrid(0, 0, -1, kRows, kColumns, true, full), kCells - 1,
          "with a full grid a left press at the very start wraps to the last cell");
    check(af::stepSelectGrid(0, 1, 0, kRows, kColumns, true, full), kColumns,
          "with a full grid a down press moves one row");

    // Nothing landable at all: the cursor must not leave the cell it is on.
    const std::vector<uint8_t> none(kCells, 0);
    check(af::stepSelectGrid(5, 0, 1, kRows, kColumns, true, none), 5,
          "with nothing landable the cursor stays put");

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

#include "engine/select_grid.h"

namespace af {

namespace {

// A positive remainder, so stepping off the front of the grid wraps to its far
// end rather than staying negative.
int wrapIndex(int value, int span) {
    if (span <= 0) return 0;
    return ((value % span) + span) % span;
}

}  // namespace

int stepSelectGrid(int start, int deltaRow, int deltaColumn, int rows, int columns, bool wrapping,
                   const std::vector<uint8_t>& landable) {
    if (rows < 1 || columns < 1) return start;
    const int total = rows * columns;
    if (start < 0 || start >= total) return start;

    const int startRow = start / columns;
    const int startColumn = start % columns;

    const auto isLandable = [&](int cell) {
        return cell >= 0 && cell < static_cast<int>(landable.size()) && landable[cell] != 0;
    };

    for (int step = 1; step <= total; ++step) {
        int cell = 0;

        if (deltaColumn != 0) {
            const int linear = start + deltaColumn * step;
            if (linear < 0 || linear >= total) {
                if (!wrapping) break;
                cell = wrapIndex(linear, total);
            } else {
                cell = linear;
            }
        } else {
            const int row = startRow + deltaRow * step;
            if (row < 0 || row >= rows) {
                if (!wrapping) break;
            }
            cell = wrapIndex(row, rows) * columns + startColumn;
        }

        if (isLandable(cell)) return cell;
    }

    // Nowhere to land: the cursor stays where it was rather than moving to a
    // cell it may not occupy.
    return start;
}

}  // namespace af

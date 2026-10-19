#include "engine/continue_flow.h"

namespace af {

ContinuePrompt::ContinuePrompt(int durationTicks)
    : ticksLeft_(durationTicks > 0 ? durationTicks : 0) {}

void ContinuePrompt::advance() {
    if (ticksLeft_ > 0) --ticksLeft_;
}

void ContinuePrompt::moveSelection(int delta) {
    if (delta < 0) {
        yes_ = true;
    } else if (delta > 0) {
        yes_ = false;
    }
}

}  // namespace af

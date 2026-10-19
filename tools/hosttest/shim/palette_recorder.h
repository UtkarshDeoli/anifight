// Watching what a `Character` hands its sprite bank's palette table.
//
// The sprite-bank shim swallows `addPalette` -- a behaviour-only test loads no
// art, so the real table never exists. The palette-flip regression test needs
// the opposite: it must see the exact bytes `Character::load` passes, because
// the bug being pinned is an ordering one and the ordering is only visible in
// those bytes. So the shim keeps a recording of every 768-byte palette it is
// offered.
//
// Recording is opt-in and off by default, so a test that only steps a fighter
// -- test_statemachine -- keeps the shim's old no-op behaviour and its memory
// profile exactly. Turn it on with record(true), then read recorded().
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace af {
namespace palette_stub {

// Start or stop recording. Clearing is separate, so a test can turn recording
// on, load several characters, and read every palette they offered.
void record(bool enabled);

// The palettes `SpriteBank::addPalette` has been handed since the last clear(),
// in call order, each exactly 768 bytes of RGB triples.
const std::vector<std::array<uint8_t, 768>>& recorded();

void clear();

}  // namespace palette_stub
}  // namespace af

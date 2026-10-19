// Host tests for the .cns state reader (engine/cns): what a `[Statedef n]`
// header leaves on the StateDef, and what a controller leaves on itself.
//
// The parser is pure data -- it never runs a state -- so this needs no fighter,
// no sprite bank and no display: just cns.cpp, expr.cpp and their one
// dependency, core/ini.cpp. That is the point of testing it here rather than
// through the state machine, where a header bug and a runtime bug would be
// indistinguishable.
//
// The centre of gravity is the three `*persist` header flags. They are the one
// part of a statedef whose whole meaning is "which of the combat books survives
// entering this state", and getting one wrong is invisible until a combo
// behaves differently from how its author wrote it: MUGEN clears the books by
// default, and a state that means to keep them says so on this line.
//
// The last group covers the other half of reading a character's constants:
// `Character::readConstants` / `Character::seedConstantDefaults`, which live in
// engine/character.h precisely so this pure-logic build can reach them. A real
// .cns omits constants freely -- `const()` used to answer 0 for every one it
// left out -- so the tests pin what a gap is filled with, that a written value
// still wins, and that a default converts with the character's localcoord by
// the *width* ratio rather than the height ratio.
//
// Scenarios are named so a failure line says which rule it pins.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_cns_states \
//         tools/hosttest/test_cns_states.cpp app/src/main/cpp/engine/cns.cpp \
//         app/src/main/cpp/engine/expr.cpp app/src/main/cpp/core/ini.cpp
//     /tmp/test_cns_states

#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>

#include "engine/character.h"
#include "engine/cns.h"

namespace {

int gChecks = 0;
int gFailures = 0;
std::string gScenario;

void scenario(const char* name) { gScenario = name; }

#define CHECK(expr)                                                          \
    do {                                                                     \
        ++gChecks;                                                           \
        if (!(expr)) {                                                       \
            ++gFailures;                                                     \
            std::fprintf(stderr, "FAIL [%s] %s:%d: %s\n", gScenario.c_str(), \
                         __FILE__, __LINE__, #expr);                         \
        }                                                                    \
    } while (0)

// ------------------------------------------------------------ persist flags

void testPersistDefaults() {
    scenario("cns/persist-defaults");
    af::StateLibrary library;
    library.parse("[Statedef 100]\ntype = S\n");

    const af::StateDef* state = library.find(100);
    CHECK(state != nullptr);
    CHECK(state != nullptr && state->defined);
    // MUGEN resets the move books on entering a state unless the header says
    // otherwise, so the absence of a `*persist` line has to read as false and
    // not as "unset but truthy".
    CHECK(state != nullptr && !state->moveHitPersist);
    CHECK(state != nullptr && !state->hitCountPersist);
    CHECK(state != nullptr && !state->hitDefPersist);
}

void testPersistFlagsIndependent() {
    scenario("cns/persist-flags");
    af::StateLibrary library;
    library.parse(
        "[Statedef 100]\ntype = S\nmovehitpersist = 1\n"
        "[Statedef 101]\ntype = S\nhitcountpersist = 1\n"
        "[Statedef 102]\ntype = S\nhitdefpersist = 1\n");

    const af::StateDef* moveHit = library.find(100);
    const af::StateDef* hitCount = library.find(101);
    const af::StateDef* hitDef = library.find(102);
    CHECK(moveHit != nullptr && hitCount != nullptr && hitDef != nullptr);

    // Each flag turns on only itself; naming one must not set the other two.
    CHECK(moveHit != nullptr && moveHit->moveHitPersist);
    CHECK(moveHit != nullptr && !moveHit->hitCountPersist);
    CHECK(moveHit != nullptr && !moveHit->hitDefPersist);

    CHECK(hitCount != nullptr && hitCount->hitCountPersist);
    CHECK(hitCount != nullptr && !hitCount->moveHitPersist);
    CHECK(hitCount != nullptr && !hitCount->hitDefPersist);

    CHECK(hitDef != nullptr && hitDef->hitDefPersist);
    CHECK(hitDef != nullptr && !hitDef->moveHitPersist);
    CHECK(hitDef != nullptr && !hitDef->hitCountPersist);
}

void testPersistValueForms() {
    scenario("cns/persist-value-forms");
    af::StateLibrary library;
    library.parse(
        "[Statedef 200]\ntype = S\nmovehitpersist = yes\n"
        "[Statedef 201]\ntype = S\nhitcountpersist = on\n"
        "[Statedef 202]\ntype = S\nhitdefpersist = TRUE\n"
        "[Statedef 203]\ntype = S\nmovehitpersist = 0\n"
        "[Statedef 204]\ntype = S\nhitcountpersist = no\n"
        "[Statedef 205]\ntype = S\nHITDEFPERSIST = 1\n");

    // The boolean reader's word forms all mean true; a reader that only knew
    // "1" would silently drop `yes`/`on`/`TRUE`, which real files use.
    CHECK(library.find(200) != nullptr && library.find(200)->moveHitPersist);
    CHECK(library.find(201) != nullptr && library.find(201)->hitCountPersist);
    CHECK(library.find(202) != nullptr && library.find(202)->hitDefPersist);
    // An explicit `0`/`no` is false, same as omitting the line.
    CHECK(library.find(203) != nullptr && !library.find(203)->moveHitPersist);
    CHECK(library.find(204) != nullptr && !library.find(204)->hitCountPersist);
    // Keys are folded before they are matched, so the spelling's case is free.
    CHECK(library.find(205) != nullptr && library.find(205)->hitDefPersist);
}

void testPersistSurvivesControllers() {
    scenario("cns/persist-survives-controllers");
    af::StateLibrary library;
    library.parse(
        "[Statedef 300]\ntype = S\nmovehitpersist = 1\n\n"
        // A controller after the header must not clear what the header set:
        // the flags live on the StateDef, and the controller sections are
        // attached to that same StateDef rather than opening a new one.
        "[State 300, later]\ntype = VelSet\ntrigger1 = 1\nx = 1\n");

    const af::StateDef* state = library.find(300);
    CHECK(state != nullptr && state->moveHitPersist);
    CHECK(state != nullptr && state->controllers.size() == 1);
    // `later` names no persist flag, so it must leave the header's alone.
    CHECK(state != nullptr && !state->hitCountPersist);
}

void testPersistNoLeak() {
    scenario("cns/persist-no-leak");
    af::StateLibrary library;
    library.parse(
        "[Statedef 400]\ntype = S\nmovehitpersist = 1\nhitcountpersist = 1\n\n"
        "[Statedef 401]\ntype = S\n");

    // The next state starts from a fresh record; a flag set on 400 must not
    // bleed onto 401, which would keep books the author asked to reset.
    CHECK(library.find(401) != nullptr && !library.find(401)->moveHitPersist);
    CHECK(library.find(401) != nullptr && !library.find(401)->hitCountPersist);
}

void testPersistMergeAcrossHeaders() {
    scenario("cns/persist-merge");
    af::StateLibrary library;
    library.parse(
        "[Statedef 500]\ntype = S\nmovehitpersist = 1\n\n"
        "[Statedef 500]\ntype = S\n");

    const af::StateDef* state = library.find(500);
    // A second `[Statedef 500]` merges into the same record and only overwrites
    // the keys it names: the first header's flag survives the second header,
    // which is how a state split across two files keeps its books.
    CHECK(state != nullptr && state->moveHitPersist);
}

// --------------------------------------------------------- the other header

void testHeaderFields() {
    scenario("cns/statedef-fields");
    af::StateLibrary library;
    library.parse(
        "[Statedef 600]\n"
        "type = U\n"
        "movetype = A\n"
        "physics = N\n"
        "ctrl = 0\n"
        "anim = 40+var(49)\n"
        "facep2 = 1\n"
        "juggle = 5\n"
        "poweradd = 20\n"
        "sprpriority = 2\n");

    const af::StateDef* state = library.find(600);
    CHECK(state != nullptr);
    if (state == nullptr) return;
    CHECK(state->defined);
    // `U` means "unchanged", which is not the same as the fallback value the
    // letter parses to: the runtime has to see the difference.
    CHECK(state->typeUnchanged);
    CHECK(state->moveType == af::MoveType::Attack);
    CHECK(!state->moveTypeUnchanged);
    CHECK(state->physics == af::Physics::None);
    // `ctrl = 0` is present-and-false, which must be distinguishable from an
    // absent `ctrl` line -- a state that leaves it out inherits control.
    CHECK(state->hasCtrl && !state->ctrl);
    // A non-numeric anim keeps its text for the runtime to evaluate. Pinned as
    // current behaviour: the int fast path is whatever leading integer the text
    // begins with, so `40+var(49)` leaves anim at 40 rather than at the -1 "keep
    // the current animation" sentinel. A reader that assumed the fast path is
    // only set for a plain number would be surprised here; documented rather
    // than asserted as correct, because which one the runtime should use is a
    // question this parser test is not the place to answer.
    CHECK(state->hasAnim);
    CHECK(state->animRaw == "40+var(49)");
    CHECK(state->anim == 40);
    CHECK(state->hasFaceP2 && state->faceP2);
    CHECK(state->hasJuggle && state->juggle == 5);
    CHECK(state->hasPowerAdd && state->powerAdd == 20);
    CHECK(state->sprPriority == 2);
}

void testControllerPersistent() {
    scenario("cns/controller-persistent");
    af::StateLibrary library;
    library.parse(
        "[Statedef 700]\ntype = S\n\n"
        "[State 700, throttled]\n"
        "type = VelSet\ntrigger1 = 1\nx = 1\n"
        "persistent = 3\n"
        "ignorehitpause = 1\n\n"
        "[State 700, plain]\n"
        "type = VelSet\ntrigger1 = 1\nx = 2\n");

    const af::StateDef* state = library.find(700);
    CHECK(state != nullptr && state->controllers.size() == 2);
    if (state == nullptr || state->controllers.size() != 2) return;

    const af::Controller& throttled = state->controllers[0];
    CHECK(throttled.persistent() == 3);
    CHECK(throttled.ignoresHitPause());
    CHECK(throttled.triggerCount() == 1);

    // The defaults are the common path: once a tick, and frozen with the rest
    // of the fighter.
    const af::Controller& plain = state->controllers[1];
    CHECK(plain.persistent() == 1);
    CHECK(!plain.ignoresHitPause());
}

// ------------------------------------------------ constant seeds and aliases

bool close(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// Reads `cns` the way Character::loadCns does -- the file's own constants
// first, then MUGEN's defaults into the gaps -- and hands back the map
// `const()` would read.
std::unordered_map<std::string, float> seeded(const char* cns,
                                              af::Vec2 localCoord = {320.0f, 240.0f}) {
    const af::IniFile document = af::IniFile::parse(cns);
    std::unordered_map<std::string, float> consts;
    af::Character::readConstants(document, consts);
    af::Character::seedConstantDefaults(localCoord, consts);
    return consts;
}

float valueOf(const std::unordered_map<std::string, float>& consts, const char* key) {
    const auto it = consts.find(key);
    return it == consts.end() ? 0.0f : it->second;
}

bool declares(const std::unordered_map<std::string, float>& consts, const char* key) {
    return consts.find(key) != consts.end();
}

void testConstDefaultsFillGaps() {
    scenario("cns/defaults-fill-gaps");
    // A character that writes two of its [Data] numbers and nothing at all in
    // [Size]/[Velocity]/[Movement]: every name it left out has to answer with
    // MUGEN's default rather than 0, which is what `const()` returned for an
    // undeclared key.
    const auto consts = seeded(
        "[Data]\nlife = 1000\nsparkno = 7\n\n"
        "[Statedef 0]\ntype = S\n");

    // The two it wrote stay its own.
    CHECK(close(valueOf(consts, "data.life"), 1000.0f));
    CHECK(close(valueOf(consts, "data.sparkno"), 7.0f));
    // The combat numbers it left out come from the template.
    CHECK(close(valueOf(consts, "data.attack"), 100.0f));
    CHECK(close(valueOf(consts, "data.airjuggle"), 15.0f));
    CHECK(close(valueOf(consts, "data.power"), 3000.0f));
    CHECK(close(valueOf(consts, "movement.yaccel"), 0.44f));
    CHECK(close(valueOf(consts, "movement.airjump.num"), 1.0f));
    // A single-valued [Velocity] name is read by its `.x` spelling.
    CHECK(close(valueOf(consts, "velocity.walk.fwd.x"), 2.4f));
    CHECK(close(valueOf(consts, "size.height"), 60.0f));
    // The `.y` half of a pair constant only resolves because the bare name and
    // its halves are written together, through the loader's own storage path.
    CHECK(close(valueOf(consts, "size.head.pos"), -5.0f));
    CHECK(close(valueOf(consts, "size.head.pos.x"), -5.0f));
    CHECK(close(valueOf(consts, "size.head.pos.y"), -90.0f));
    CHECK(close(valueOf(consts, "movement.down.bounce.offset.y"), 20.0f));
    // The two jump aliases no file writes and the loader never synthesised, so
    // every character read 0 for them.
    CHECK(close(valueOf(consts, "velocity.jump.y"), -8.4f));
    CHECK(close(valueOf(consts, "velocity.airjump.y"), -8.1f));
    // data.fall.defence_mul is trigger.html's arithmetic, 100/(f+100).
    CHECK(close(valueOf(consts, "data.fall.defence_mul"), 100.0f / 150.0f));
    // A name defaulted *and* one that reads 0 both resolve; the difference is
    // that this one is now declared, which is what the log and the map can see.
    CHECK(declares(consts, "data.ko.echo"));
    CHECK(close(valueOf(consts, "data.ko.echo"), 0.0f));
}

void testConstDefaultsYieldToFile() {
    scenario("cns/defaults-yield-to-file");
    // The same names, all written by the character: a default must never
    // overwrite what the author tuned, and the derived aliases must follow the
    // file's own numbers rather than the template's.
    const auto consts = seeded(
        "[Data]\nlife = 3000\nfall.defence_up = 0\n\n"
        "[Velocity]\njump.neu = 0, -9.5\nwalk.fwd = 5.0\n\n"
        "[Movement]\nyaccel = 1.2\n\n"
        "[Statedef 0]\ntype = S\n");

    CHECK(close(valueOf(consts, "data.life"), 3000.0f));  // not the 1000 default
    CHECK(close(valueOf(consts, "velocity.walk.fwd.x"), 5.0f));
    CHECK(close(valueOf(consts, "movement.yaccel"), 1.2f));
    CHECK(close(valueOf(consts, "velocity.jump.neu.y"), -9.5f));
    CHECK(close(valueOf(consts, "velocity.jump.y"), -9.5f));  // derived from the file
    // Computed from the file's own fall.defence_up: 100/(0+100).
    CHECK(close(valueOf(consts, "data.fall.defence_mul"), 1.0f));
    // A name the file still left out is defaulted as usual.
    CHECK(close(valueOf(consts, "movement.airjump.num"), 1.0f));
    CHECK(close(valueOf(consts, "velocity.airjump.y"), -8.1f));
}

void testConstDefaultsScaleByWidth() {
    scenario("cns/defaults-scale-by-width");
    // 1280x720 against MUGEN's 320x240: the conversion factor is the *width*
    // ratio, 1280/320 = 4, and Elecbyte's own kfm720.cns is the proof (every
    // length and speed in it is exactly four times kfm.cns's, y halves
    // included). The height ratio, 720/240 = 3, appears nowhere in that pair.
    const auto consts = seeded("[Statedef 0]\ntype = S\n", {1280.0f, 720.0f});

    CHECK(close(valueOf(consts, "size.height"), 240.0f));       // 60 * 4, not * 3
    CHECK(close(valueOf(consts, "size.head.pos.y"), -360.0f));  // -90 * 4
    CHECK(close(valueOf(consts, "movement.yaccel"), 1.76f));    // .44 * 4
    CHECK(close(valueOf(consts, "velocity.walk.fwd.x"), 9.6f)); // 2.4 * 4
    CHECK(close(valueOf(consts, "velocity.jump.y"), -33.6f));   // -8.4 * 4
    // Dimensionless names do not convert.
    CHECK(close(valueOf(consts, "data.life"), 1000.0f));
    CHECK(close(valueOf(consts, "data.airjuggle"), 15.0f));
    CHECK(close(valueOf(consts, "size.xscale"), 1.0f));
    CHECK(close(valueOf(consts, "movement.airjump.num"), 1.0f));
    CHECK(close(valueOf(consts, "movement.stand.friction"), 0.85f));
    CHECK(close(valueOf(consts, "velocity.air.gethit.airrecover.mul.x"), 0.5f));
}

void testJumpAliasDeferAndChangeanim() {
    scenario("cns/jump-alias-defer-and-changeanim");
    // A file that writes only the x half of jump.neu leaves no y for the alias
    // to copy, so velocity.jump.y stays undeclared -- which is what leaves
    // statemachine.cpp's own fallback to jumpUp() in charge of that jump rather
    // than this seeding inventing a height.
    const auto partial = seeded("[Velocity]\njump.neu = 0\n\n[Statedef 0]\ntype = S\n");
    CHECK(!declares(partial, "velocity.jump.y"));
    CHECK(close(valueOf(partial, "velocity.jump.fwd.x"), 2.5f));  // the rest still seeds

    // movement.jump.changeanim.threshold is documented but deliberately not
    // seeded: no shipped file defines it and its engine default is unknown, so
    // it must stay absent rather than take a number nobody can justify.
    const auto consts = seeded("[Statedef 0]\ntype = S\n");
    CHECK(!declares(consts, "movement.jump.changeanim.threshold"));
}

}  // namespace

int main() {
    testPersistDefaults();
    testPersistFlagsIndependent();
    testPersistValueForms();
    testPersistSurvivesControllers();
    testPersistNoLeak();
    testPersistMergeAcrossHeaders();
    testHeaderFields();
    testControllerPersistent();
    testConstDefaultsFillGaps();
    testConstDefaultsYieldToFile();
    testConstDefaultsScaleByWidth();
    testJumpAliasDeferAndChangeanim();

    std::printf("%d checks, %d failed\n", gChecks, gFailures);
    if (gFailures == 0) std::printf("ok\n");
    return gFailures == 0 ? 0 : 1;
}

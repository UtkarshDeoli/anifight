// Host test for the state machine (engine/statemachine) against a real
// character.
//
// The claim being tested is the one the whole engine rests on: a character's
// moves come out of its own files. So the fixture is a real community
// character's .cns and .cmd -- not anything generated here, and not a stub --
// and the test drives it the way a player would:
//
//   * read a command out of the .cmd,
//   * feed the input that command is written for,
//   * assert the character left its standing state for a move,
//   * and assert the same command works facing the other way, from the mirrored
//     input, because `F` in a command means "forward" and not "right".
//
// It also checks the two things a character's file format gets wrong quietly:
// that the .cns's states and the .cmd's states end up in one library, and that
// a command the character declares is reachable at all.
//
// There are two ways in, because a character's behaviour is not all in one
// file:
//
//   ./test_statemachine <name>.cns <name>.cmd
//       Reads the two documents directly. Fast, and enough to exercise the
//       machine, but it does not see the `st`, `st1`..`st9` and `stcommon` keys
//       that hold most characters' states.
//   ./test_statemachine <name>.def
//       Loads the character the way the game does, through its .def, so every
//       state file it names is read -- and so a *missing* `stcommon`, which is
//       how most characters in the wild are packaged, is exercised too.
//
// Build and run:
//   clang++ -std=c++20 -O1 -I app/src/main/cpp -I tools/hosttest/shim \
//       -o test_statemachine tools/hosttest/test_statemachine.cpp \
//       tools/hosttest/shim/sprite_bank_stub.cpp \
//       app/src/main/cpp/engine/statemachine.cpp app/src/main/cpp/engine/cns.cpp \
//       app/src/main/cpp/engine/cmd.cpp app/src/main/cpp/engine/expr.cpp \
//       app/src/main/cpp/engine/character.cpp app/src/main/cpp/engine/victory_flow.cpp \
//       app/src/main/cpp/engine/anim.cpp app/src/main/cpp/engine/fighter.cpp \
//       app/src/main/cpp/engine/entities.cpp app/src/main/cpp/audio/snd_bank.cpp \
//       app/src/main/cpp/core/ini.cpp
//
// The gfx half (sprite_bank.cpp, renderer.cpp) is replaced by the stub under
// tools/hosttest/shim: the fighter is linked whole, so the linker asks for the
// drawing symbols even though nothing here draws. snd_bank.cpp is the real
// decoder -- it is pure arithmetic -- but no .snd fixture is needed, because
// the test's own RecordingSink is what the machine talks to.
//
// tools/hosttest/run_all_tests.sh builds and runs this along with the rest of
// the suite, against whichever character it can find.

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

// The sprite-bank shim's `drawSpriteAt` writes down what it was asked to draw, so
// a test with no display can still assert on which layer a thing would have been
// drawn on and how it would have been mirrored. Recording is opt-in; see the
// header for why it is a fact about the *call* and nothing else.
#include "draw_recorder.h"
#include "engine/character.h"
#include "engine/defs.h"
#include "engine/entities.h"
#include "engine/fighter.h"
#include "engine/statemachine.h"

// `parseTransType` is the translator from MUGEN's six trans types to the pair the
// draw path wants. It lives at namespace scope in statemachine.cpp rather than in
// a header, because the state machine is its only engine caller; the test reaches
// it through this declaration so that the six types can be checked one at a time,
// without a controller and a character standing between the string and the answer.
namespace af {
std::optional<TransParams> parseTransType(std::string_view text, float alphaSrc, float alphaDst);
}  // namespace af

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    ++failures;
    std::printf("FAIL: %s\n", what.c_str());
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::ostringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

// A sound sink that writes down what it was asked for.
//
// The alternative -- counting how many sounds played on a device -- cannot tell
// a swing sound from a hit sound from a menu blip, and cannot tell either from a
// sound that never arrived. Recording the pairs is exact.
class RecordingSink : public af::SoundSink {
public:
    void play(int group, int sample) override { pairs.emplace_back(group, sample); }

    [[nodiscard]] bool has(int group, int sample) const {
        for (const auto& pair : pairs) {
            if (pair.first == group && pair.second == sample) return true;
        }
        return false;
    }
    [[nodiscard]] size_t count() const { return pairs.size(); }
    void clear() { pairs.clear(); }

    std::vector<std::pair<int, int>> pairs;
};


// A sink that writes down what a state machine asked for, so that the controllers which
// create entities can be tested without any of the fight existing.
class RecordingEntities : public af::EntitySink {
public:
    void spawnExplod(const af::Explod& explod) override {
        explods.push_back(explod);
        ++spawnExplodCalls;
    }
    void spawnProjectile(const af::Projectile& projectile) override {
        projectiles.push_back(projectile);
    }
    void shake(int time, float frequency, float amplitude) override {
        shakes.push_back({time, frequency, amplitude});
    }
    // The two global palette grades. They are separate slots on the fight, not the
    // fighter's own -- a `PalFX` must never arrive here -- so the test records each
    // slot's last state and how many times each was written.
    void setBgPalFx(const af::StateMachine::PalFxState& state) override {
        bgPalFx = state;
        ++bgPalFxCalls;
    }
    void setAllPalFx(const af::StateMachine::PalFxState& state) override {
        allPalFx = state;
        ++allPalFxCalls;
    }
    // The explod mutations. Both are recorded one for one, because what these
    // controllers are worth testing for is *which* fields they chose to write:
    // `ModifyExplod` is a controller that is almost entirely omission, and the
    // interesting assertions are all about the fields it left alone.
    void modifyExplod(const af::ExplodModify& modify) override {
        modifications.push_back(modify);
    }
    void removeExplod(const af::Character* owner, int id, bool all) override {
        removals.push_back({owner, id, all});
    }
    [[nodiscard]] int countExplods(const af::Character* owner, const af::StateMachine* ownerMachine,
                                   int id) const override {
        int count = 0;
        for (const af::Explod& explod : explods) {
            const bool matches = ownerMachine != nullptr ? explod.ownerMachine == ownerMachine
                                                         : explod.owner == owner;
            if (matches && (id < 0 || explod.id == id)) ++count;
        }
        return count;
    }
    [[nodiscard]] int countProjectiles(const af::Character* owner,
                                       const af::StateMachine* ownerMachine, int id) const override {
        int count = 0;
        for (const af::Projectile& projectile : projectiles) {
            const bool matches = ownerMachine != nullptr
                                     ? projectile.ownerMachine == ownerMachine
                                     : projectile.owner == owner;
            if (matches && (id < 0 || projectile.id == id)) ++count;
        }
        return count;
    }
    // The player-side projectile contact triple, so the Proj* triggers can be
    // driven without a fight screen.
    [[nodiscard]] af::ProjectileContactRecord projectileContact(int ownerSide) const override {
        if (ownerSide < 0 || ownerSide > 1) return af::ProjectileContactRecord{};
        return contacts[ownerSide];
    }

    af::ProjectileContactRecord contacts[2];
    std::vector<af::Explod> explods;
    std::vector<af::Projectile> projectiles;
    std::vector<af::ExplodModify> modifications;
    struct Removal { const af::Character* owner; int id; bool all; };
    std::vector<Removal> removals;
    struct Shake { int time; float frequency; float amplitude; };
    std::vector<Shake> shakes;

    // The last grade each global slot was handed, and how many times it was
    // written. `time == 0` no-op and `-1` permanent are read off these.
    af::StateMachine::PalFxState bgPalFx;
    af::StateMachine::PalFxState allPalFx;
    int bgPalFxCalls = 0;
    int allPalFxCalls = 0;
    // How many explods the machine asked for, alongside the list of what they were.
    // The pair is what lets a test tell "asked for one, and it was animation 72" from
    // "asked for none at all".
    int spawnExplodCalls = 0;
};

// Reads a character's files straight off the filesystem, rooted at the folder
// holding its .def. The engine's own FileSource is backed by the APK's asset
// manager, which is not available here; the interface is three lines, so the
// host test brings its own.
class DirectorySource : public af::FileSource {
public:
    explicit DirectorySource(std::string root) : root_(std::move(root)) {}

    std::unique_ptr<af::FileData> read(const std::string& path) override {
        std::ifstream file(root_ + "/" + path, std::ios::binary);
        if (!file) return nullptr;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        const std::string bytes = buffer.str();
        std::vector<uint8_t> data(bytes.begin(), bytes.end());
        return std::make_unique<af::FileData>(path, std::move(data));
    }

    bool exists(const std::string& path) override {
        struct stat info {};
        return stat((root_ + "/" + path).c_str(), &info) == 0;
    }

private:
    std::string root_;
};

// The command symbol bits, mirrored from engine/cmd.h deliberately: the test
// has to build its input from the command's own notation, and reusing the
// machine's converter would only prove the converter agrees with itself.
namespace Sym {
constexpr uint32_t kUp = 1u << 0;
constexpr uint32_t kDown = 1u << 1;
constexpr uint32_t kBack = 1u << 2;
constexpr uint32_t kForward = 1u << 3;
constexpr uint32_t kButtonBase = 1u << 4;
uint32_t button(char c) { return kButtonBase << (c - 'a'); }
}  // namespace Sym

// Turns a command's symbols into this engine's button mask, resolving back and
// forward against facing -- the direction the command means, not the way the
// key happens to point.
uint32_t heldFrom(uint32_t symbols, int facing) {
    uint32_t held = 0;
    if ((symbols & Sym::kUp) != 0) held |= af::Button::kUp;
    if ((symbols & Sym::kDown) != 0) held |= af::Button::kDown;
    const bool backIsLeft = facing >= 0;
    if ((symbols & Sym::kBack) != 0) held |= backIsLeft ? af::Button::kLeft : af::Button::kRight;
    if ((symbols & Sym::kForward) != 0) held |= backIsLeft ? af::Button::kRight : af::Button::kLeft;

    const char letters[6] = {'x', 'y', 'z', 'a', 'b', 'c'};
    const uint32_t buttons[6] = {af::Button::kX, af::Button::kY, af::Button::kZ,
                                 af::Button::kA, af::Button::kB, af::Button::kC};
    for (int i = 0; i < 6; ++i) {
        if ((symbols & Sym::button(letters[i])) != 0) held |= buttons[i];
    }
    return held;
}

bool hasButton(const af::CommandDef& command) {
    for (const af::CmdStep& step : command.steps) {
        if ((step.symbols & ~(Sym::kUp | Sym::kDown | Sym::kBack | Sym::kForward)) != 0) {
            return true;
        }
    }
    return false;
}

// Plays a command's own notation back as input, one element a tick, and reports
// the states the character passed through.
struct Run {
    std::vector<int> states;
    bool leftStand = false;
    int finalState = 0;
};

Run playCommand(af::StateMachine& machine, const af::CommandDef& command, int facing,
                int settleTicks) {
    Run run;
    af::InputState input;
    input.held = 0;
    machine.control(input);
    machine.step();

    for (const af::CmdStep& step : command.steps) {
        // Two ticks an element: one to go down, one for the next element to be
        // able to see it as a press rather than as a hold that was already
        // there.
        for (int i = 0; i < 2; ++i) {
            af::InputState tick;
            tick.held = step.release ? 0u : heldFrom(step.symbols, facing);
            // A released element only reads as released once it is off, which
            // is what the second tick of a two-tick element gives it.
            machine.control(tick);
            machine.step();
            run.states.push_back(machine.stateNumber());
            if (machine.stateNumber() != 0) run.leftStand = true;
        }
    }
    for (int i = 0; i < settleTicks; ++i) {
        af::InputState tick;
        machine.control(tick);
        machine.step();
        run.states.push_back(machine.stateNumber());
    }
    run.finalState = machine.stateNumber();
    return run;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: %s <character.cns> <character.cmd>\n"
                    "       %s <character.def>\n",
                    argv[0], argv[0]);
        return 2;
    }

    af::Character character;
    if (argc >= 3) {
        std::string cnsText, cmdText;
        if (!readFile(argv[1], cnsText)) {
            std::printf("FAIL: cannot read %s\n", argv[1]);
            return 1;
        }
        if (!readFile(argv[2], cmdText)) {
            std::printf("FAIL: cannot read %s\n", argv[2]);
            return 1;
        }
        character.loadBehaviour(cnsText, cmdText);
    } else {
        const std::string defPath = argv[1];
        const size_t slash = defPath.find_last_of('/');
        const std::string folder = slash == std::string::npos ? "." : defPath.substr(0, slash);
        const std::string leaf = slash == std::string::npos ? defPath : defPath.substr(slash + 1);

        DirectorySource files(folder);
        std::string error;
        if (!character.load(files, leaf, &error)) {
            std::printf("FAIL: %s: %s\n", defPath.c_str(), error.c_str());
            return 1;
        }
    }

    const af::StateLibrary& states = character.states();
    const af::CommandList& commands = character.commands();
    std::printf("loaded %zu states, %zu controllers, %zu commands, %zu constants\n", states.size(),
                states.controllerCount(), commands.size(), character.constants().size());

    check(states.size() > 20, "the character should define many states");
    check(commands.size() > 5, "the .cmd should declare several commands");
    // States -1, -2 and -3 live in the .cmd by convention, so a library with
    // only the .cns's contents means the two documents did not merge.
    check(states.has(-1), "state -1 should be read, and it is normally in the .cmd");

    // ---------------------------------------------------- the command state
    af::StateMachine machine;
    af::StateMachine dummy;
    machine.setup(&character, 1);
    dummy.setup(nullptr, -1);
    machine.setOpponent(&dummy);
    machine.setPosition(320.0f);
    machine.setStageBounds(90.0f, 550.0f);
    check(machine.stateNumber() == 0, "a fighter should start in state 0");

    af::InputState idle;
    std::vector<int> idleTrace;
    for (int i = 0; i < 20; ++i) {
        machine.control(idle);
        machine.step();
        idleTrace.push_back(machine.stateNumber());
    }
    if (machine.stateNumber() != 0) {
        std::printf("  idle trace:");
        for (const int state : idleTrace) std::printf(" %d", state);
        std::printf("\n");
    }
    // A character with no common state file of its own -- Kung Fu Man is one --
    // has no `[Statedef 0]` in any of its files, and the engine's built-in
    // standing state is what it stands in. That is the point: the roster is not
    // required to carry MUGEN's own states around with it.
    check(machine.stateNumber() == 0, "an idle fighter should stay in its standing state");
    check(machine.life() > 0, "a fighter should start with life");
    check(machine.ctrl(), "a standing fighter with nothing happening should have control");

    // ------------------------------------------ every attack command fires
    //
    // Not "one of them": the complaint this machine exists to answer is that
    // only two moves worked, so the test walks the character's whole move list
    // and counts how many are reachable from standing.
    int attackCommands = 0;
    int reached = 0;
    std::vector<std::string> dead;
    for (const af::CommandDef& command : commands.commands()) {
        if (!hasButton(command) || command.steps.size() > 4) continue;
        ++attackCommands;

        af::StateMachine fresh;
        fresh.setup(&character, 1);
        fresh.setOpponent(&dummy);
        fresh.setPosition(320.0f);
        fresh.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 10; ++i) {
            af::InputState idleTick;
            fresh.control(idleTick);
            fresh.step();
        }

        const Run run = playCommand(fresh, command, 1, 12);
        if (run.leftStand) {
            ++reached;
        } else if (dead.size() < 12) {
            dead.push_back(command.name);
        }
    }
    std::printf("commands that name a button: %d, of which %d left the standing state\n",
                attackCommands, reached);
    check(attackCommands > 0, "the character should declare attack commands");
    check(reached > 0, "at least one declared attack command should be reachable");
    // Not every declared command has to move a fighter standing still: a
    // character declares commands it only ever tests from inside a state
    // ("hold_x" while charging, "recovery" after a knockdown), and several
    // gate their specials on power the fighter does not have yet. What is worth
    // pinning is that the move list is genuinely live -- so the floor is a
    // third, the count is printed either way, and the never-fired names are
    // listed for a human to judge.
    check(reached * 3 >= attackCommands,
          "at least a third of the declared attack commands should be reachable");
    if (!dead.empty()) {
        std::printf("  never fired: ");
        for (const std::string& name : dead) std::printf("%s ", name.c_str());
        std::printf("\n");
    }

    // A motion move -- three elements or more -- is the thing that only works
    // if the command strings, the input history and the facing all agree, and
    // it is the thing the whole exercise is for.
    int motionCommands = 0;
    int motionReached = 0;
    for (const af::CommandDef& command : commands.commands()) {
        if (!hasButton(command) || command.steps.size() < 3) continue;
        ++motionCommands;
        af::StateMachine fresh;
        fresh.setup(&character, 1);
        fresh.setOpponent(&dummy);
        fresh.setPosition(320.0f);
        fresh.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 10; ++i) {
            af::InputState idleTick;
            fresh.control(idleTick);
            fresh.step();
        }
        if (playCommand(fresh, command, 1, 12).leftStand) ++motionReached;
    }
    std::printf("motion commands (three elements or more): %d, of which %d fired\n",
                motionCommands, motionReached);
    if (motionCommands > 0) {
        check(motionReached * 3 >= motionCommands,
              "a character with motion moves should be able to perform some of them");
    }

    // ---------------------------------------------- commands are relative to
    //                                                facing, not to the screen
    //
    // The same command, played with mirrored input from a fighter looking the
    // other way, has to do the same thing. This is what makes one set of states
    // serve both players.
    int forwardCommands = 0;
    int mirrored = 0;
    for (const af::CommandDef& command : commands.commands()) {
        if (!hasButton(command) || command.steps.size() > 3) continue;

        af::StateMachine right;
        right.setup(&character, 1);
        right.setOpponent(&dummy);
        right.setPosition(320.0f);
        right.setStageBounds(90.0f, 550.0f);

        af::StateMachine left;
        left.setup(&character, -1);
        left.setOpponent(&dummy);
        left.setPosition(320.0f);
        left.setStageBounds(90.0f, 550.0f);

        const Run a = playCommand(right, command, 1, 8);
        const Run b = playCommand(left, command, -1, 8);
        if (!a.leftStand) continue;
        ++forwardCommands;
        if (b.leftStand) ++mirrored;
    }
    std::printf("commands that fired facing right: %d, of which %d also fired facing left\n",
                forwardCommands, mirrored);
    check(forwardCommands > 0, "some command should fire facing right");
    check(mirrored * 4 >= forwardCommands * 3,
          "a command that fires one way should fire the other way from mirrored input");

    // ------------------------------------------------------- taking a hit
    af::StateMachine victim;
    victim.setup(&character, -1);
    victim.setOpponent(&machine);
    victim.setPosition(360.0f);
    victim.setStageBounds(90.0f, 550.0f);
    const int lifeBefore = victim.life();

    af::StateMachine::HitInfo hit;
    hit.damage = 80;
    hit.attackerFacing = 1;
    hit.pushX = -4.0f;
    hit.hitTime = 12;
    hit.stateNumber = af::CommonState::kHitHigh;
    victim.applyHit(hit);

    check(victim.life() == lifeBefore - 80, "a hit should take its damage");
    // Which hit state, and its move type, are the character's own to choose --
    // what the machine owes is that the hit landed and the fighter is no longer
    // standing in its neutral state, facing the attacker.
    check(victim.stateNumber() != 0, "a hit should put the fighter in a hit state");
    check(victim.moveType() == af::MoveType::Hit || victim.stateType() != af::StateType::Stand,
          "a hit should show in the fighter's state type or move type");
    check(victim.facing() == -1, "the victim should be turned to face the attacker");

    std::vector<int> hurtTrace;
    // MUGEN's knockdown recovery is slow on purpose: a liedown alone is a
    // couple of seconds. The window has to cover the fall, the liedown and the
    // get-up, or the assertion tests the test rather than the character.
    for (int i = 0; i < 400; ++i) {
        af::InputState idleTick;
        victim.control(idleTick);
        victim.step();
        hurtTrace.push_back(victim.stateNumber());
    }
    check(victim.life() > 0, "the victim should survive 80 damage");
    if (victim.stateNumber() != 0) {
        std::printf("  hit trace:");
        for (const int state : hurtTrace) std::printf(" %d", state);
        std::printf("\n");
    }
    check(victim.stateNumber() == 0, "the victim should return to standing after the hitstun");

    // ------------------------------------------- a controller with no label
    //
    // The number in `[State n, ...]` is documentation, and the label is optional
    // too: MUGEN accepts a bare `[State]`. A file that heads every one of its
    // controllers that way is not asking for them to be skipped, and treating the
    // missing number as a malformed section loses the whole file's worth of
    // controllers at once -- which is how a character ends up defining a state it
    // has twenty controllers for, and standing in it doing nothing.
    {
        af::StateLibrary library;
        library.parse("[Statedef 200]\n"
                      "type = S\n"
                      "\n"
                      "[State]\n"
                      "type = VelSet\n"
                      "trigger1 = 1\n"
                      "x = 3\n"
                      "\n"
                      "[State ]\n"
                      "type = CtrlSet\n"
                      "trigger1 = 1\n"
                      "value = 1\n");
        const af::StateDef* state = library.find(200);
        check(state != nullptr, "a state with an unlabelled controller should exist");
        if (state != nullptr) {
            check(state->controllers.size() == 2,
                  "a `[State]` and a `[State ]` should both be attached to it");
        }
    }

    // --------------------------------------- an axis is a key, not a pair
    //
    // `PosAdd x = 10` and `PosAdd y = -30` are two separate single-valued keys.
    // Reading the second number of the `y` list returned the fallback for the
    // one-number form, so a y-only move silently did nothing. The corpus writes
    // the keys separately throughout.
    {
        af::Character mover;
        mover.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = N\nctrl = 0\n\n"
            "[State 0, up]\ntype = PosAdd\ntrigger1 = !time\ny = -30\n\n"
            "[State 0, side]\ntype = PosAdd\ntrigger1 = time = 1\nx = 10\n\n",
            "[Statedef -1]\n");

        af::StateMachine raiser;
        raiser.setup(&mover, 1);
        raiser.setPosition(320.0f);
        raiser.setStageBounds(90.0f, 550.0f);
        af::InputState still;
        raiser.control(still);
        raiser.step();
        check(std::fabs(raiser.y() - 30.0f) < 0.01f,
              "`posadd y = -30` should lift the fighter 30 units");
        raiser.control(still);
        raiser.step();
        check(std::fabs(std::fabs(raiser.x() - 320.0f) - 10.0f) < 0.01f,
              "and `posadd x = 10` should move it ten units, whichever way it faces");
    }

    // ------------------------------------------------- NumProjID counts an id
    //
    // `NumProjID` is `NumProj`'s state filter plus an ID equality
    // (`PROJECTILE-PARAMS-RE.md` §4.10). The count lives in the fight, so the
    // sink is asked directly with projectiles parked in it.
    {
        af::Character counter;
        counter.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, two]\ntype = ChangeState\ntrigger1 = numprojid(5) = 2\nvalue = 910\n\n"
            "[State 0, none]\ntype = ChangeState\ntrigger1 = numprojid(-1) = 0\nvalue = 911\n\n"
            "[Statedef 910]\ntype = S\nphysics = S\nctrl = 0\n\n"
            "[Statedef 911]\ntype = S\nphysics = S\nctrl = 0\n\n",
            "[Statedef -1]\n");

        RecordingEntities counted;
        // Counting is per state owner, so the machines that spawned these are
        // named. They are the same fighter here, as a fighter's own projectiles
        // would be.
        af::StateMachine ids;
        af::Projectile first;
        af::Projectile second;
        af::Projectile other;
        first.owner = &counter;
        first.ownerMachine = &ids;
        first.id = 5;
        second.owner = &counter;
        second.ownerMachine = &ids;
        second.id = 5;
        other.owner = &counter;
        other.ownerMachine = &ids;
        other.id = 6;
        counted.projectiles = {first, second, other};

        ids.setup(&counter, 1);
        ids.setEntitySink(&counted);
        af::InputState tick;
        ids.control(tick);
        ids.step();
        check(ids.stateNumber() == 910,
              "`numprojid(5)` should count the two projectiles carrying that id");

        // The negative form is the docs' "treated as 0" claim, which the
        // reference does not clamp. Either reading matches no `projid`, so it
        // must not turn into "count them all". A second character is needed
        // because, with the same file, the positive trigger would match first.
        af::Character strict;
        strict.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, none]\ntype = ChangeState\ntrigger1 = numprojid(-1) = 0\nvalue = 911\n\n"
            "[Statedef 911]\ntype = S\nphysics = S\nctrl = 0\n\n",
            "[Statedef -1]\n");
        af::StateMachine negative;
        negative.setup(&strict, 1);
        negative.setEntitySink(&counted);
        negative.control(tick);
        negative.step();
        check(negative.stateNumber() == 911,
              "and a negative id should match nothing rather than everything");
    }

    // ----------------------------------------------- the y axis has a sign
    //
    // A character's files measure height with +y *down*: a jump is a negative
    // y-velocity, an airborne fighter has a negative `pos y`, and a launch is
    // `air.velocity = -3, -3.5`. This engine does its own arithmetic the other way
    // up, so every y that crosses between the two has to change sign -- and a
    // miss is silent until a character decides something with it. The one that
    // bites hardest is a super move that lifts the fighter off the top of the
    // screen and waits to come back down with `trigger1 = pos y > 0`: with the
    // sign the wrong way the fighter falls *upwards* and never comes back, and the
    // round ends with it in the sky.
    {
        af::Character synthetic;
        synthetic.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = 1\nvalue = 900\n\n"
            "[Statedef 900]\ntype = A\nphysics = N\nctrl = 0\n\n"
            "[State 900, lift]\ntype = PosSet\ntrigger1 = !time\ny = -500\n\n"
            "[State 900, fall]\ntype = VelAdd\ntrigger1 = 1\ny = 7\n\n"
            "[State 900, down]\ntype = ChangeState\ntrigger1 = vel y > 0\n"
            "trigger1 = pos y > 0\nvalue = 901\n\n"
            "[Statedef 901]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 901, ground]\ntype = PosSet\ntrigger1 = !time\ny = 0\n\n"
            "[State 901, done]\ntype = ChangeState\ntrigger1 = time > 2\nvalue = 0\n",
            "[Statedef -1]\n");

        af::StateMachine faller;
        faller.setup(&synthetic, 1);
        faller.setOpponent(&dummy);
        faller.setPosition(320.0f);
        faller.setStageBounds(90.0f, 550.0f);

        bool reachedGround = false;
        float highest = 0.0f;
        for (int tick = 0; tick < 120; ++tick) {
            af::InputState tickInput;
            faller.control(tickInput);
            faller.step();
            if (faller.stateNumber() == 901) reachedGround = true;
            if (faller.y() > highest) highest = faller.y();
        }
        std::printf("  fall: reached the ground: %s, highest %0.1f\n", reachedGround ? "yes" : "no",
                    highest);
        check(reachedGround,
              "`veladd y` in a character's files should move the fighter downward, as MUGEN does");
        // The lift put it 500 up. A fighter that never comes down climbs through
        // that and keeps going, which is the shape of the bug this pins.
        check(highest < 600.0f, "and it should come down again rather than climb away");

        // The other half of the same convention: a jump has to leave the ground.
        // Nothing in a synthetic character's files says how high it jumps, so this
        // is the engine's own fallback velocity -- which has to point upwards in
        // this engine's frame for the fighter to leave the floor at all. A separate
        // character, with no state 0 of its own, so that what is being measured is
        // the engine's jump and not the faller above.
        af::Character plain;
        plain.loadBehaviour("[Statedef 900]\ntype = S\nphysics = S\n", "[Statedef -1]\n");
        af::StateMachine jumper;
        jumper.setup(&plain, 1);
        jumper.setOpponent(&dummy);
        jumper.setPosition(320.0f);
        jumper.setStageBounds(90.0f, 550.0f);
        int airborne = 0;
        float peak = 0.0f;
        for (int tick = 0; tick < 80; ++tick) {
            af::InputState tickInput;
            if (tick < 4) tickInput.held = af::Button::kUp;
            jumper.control(tickInput);
            jumper.step();
            if (jumper.stateType() == af::StateType::Air) ++airborne;
            if (jumper.y() > peak) peak = jumper.y();
        }
        std::printf("  jump: %d of 80 ticks airborne, %0.1f units up\n", airborne, peak);
        check(airborne > 15, "holding up should put the fighter in the air and keep it there");
        check(peak > 10.0f, "and a jump should gain some height before it comes down");
    }

    // ------------------------------------------------- LoopStart, and looping
    //
    // An action loops. All that is needed is the *absence* of a `LoopStart` line,
    // which is the common case -- so an animation that plays once and holds its
    // last element is the exception, written by giving that element a duration
    // of -1. `LoopStart` does not turn looping on; it moves the point the action
    // resumes from, for an action whose opening elements play once on entry.
    //
    // This matters far more than it looks, because it is what `AnimTime` counts
    // down to: every hand-over in the format is written `trigger1 = AnimTime = 0`,
    // and a looping animation reaches zero at the end of each time round.
    {
        af::AnimationSet plain;
        af::parseAirText("[Begin Action 900]\n0,0, 0,0, 2\n0,1, 0,0, 2\n", plain);
        const af::Animation* once = plain.find(900);
        check(once != nullptr, "an action with no LoopStart should still parse");
        if (once != nullptr) {
            check(once->loopStart == 0, "with no LoopStart an action resumes at its first element");
            check(once->infinite == false, "and it is not the holding kind");
        }

        af::AnimationSet looped;
        af::parseAirText(
            "[Begin Action 901]\n0,0, 0,0, 2\nLoopStart\n0,1, 0,0, 2\n0,2, 0,0, 2\n", looped);
        const af::Animation* cycle = looped.find(901);
        check(cycle != nullptr, "an action with a LoopStart should parse");
        if (cycle != nullptr) {
            check(cycle->loopStart == 1, "LoopStart should set the element to resume at");
        }

        // A last element with a duration of -1 is the "no loop" case, and the
        // one an action holds on: a lying-down pose, a win pose.
        af::AnimationSet held;
        af::parseAirText("[Begin Action 902]\n0,0, 0,0, 2\n0,1, 0,0, -1\n", held);
        const af::Animation* hold = held.find(902);
        check(hold != nullptr, "an action holding on a frame should parse");
        if (hold != nullptr) {
            check(hold->infinite, "a duration of -1 should be reported as holding forever");
            check(hold->frameAt(999) == 1, "and the action should stay on that element");
        }
    }

    // -------------------------------------------------------------- crouching
    //
    // Crouching is the one movement a player cannot avoid -- every character has
    // crouching attacks -- and letting go of down has to stand the fighter back
    // up. It is also a test that spans the whole engine: entering the crouch and
    // leaving it are the engine's own transitions (Elecbyte's common1.cns has no
    // entry into the crouch and no way out of it either, so the engine must own
    // both), and each of those waits for an animation to end.
    {
        af::StateMachine croucher;
        croucher.setup(&character, 1);
        croucher.setOpponent(&dummy);
        croucher.setPosition(320.0f);
        croucher.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 8; ++i) {
            af::InputState idleTick;
            croucher.control(idleTick);
            croucher.step();
        }

        af::InputState heldDown;
        heldDown.held = af::Button::kDown;
        for (int i = 0; i < 20; ++i) {
            croucher.control(heldDown);
            croucher.step();
        }
        if (croucher.stateType() != af::StateType::Crouch) {
            std::printf("  crouch attempt ended in state %d\n", croucher.stateNumber());
        }
        check(croucher.stateType() == af::StateType::Crouch,
              "holding down should put the fighter in a crouching state");

        std::vector<int> upTrace;
        for (int i = 0; i < 90; ++i) {
            af::InputState idleTick;
            croucher.control(idleTick);
            croucher.step();
            upTrace.push_back(croucher.stateNumber());
        }
        if (croucher.stateNumber() != 0) {
            std::printf("  after releasing down:");
            for (size_t i = 0; i < upTrace.size() && i < 30; ++i) {
                std::printf(" %d", upTrace[i]);
            }
            std::printf("\n");
        }
        check(croucher.stateNumber() == 0,
              "letting go of down should stand the fighter back up");

        // And the same on the way down: a fighter crouches with control, so the
        // crouch is a place it can act from rather than a trap.
        af::StateMachine again;
        again.setup(&character, 1);
        again.setOpponent(&dummy);
        again.setPosition(320.0f);
        again.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 8; ++i) {
            af::InputState idleTick;
            again.control(idleTick);
            again.step();
        }
        for (int i = 0; i < 40; ++i) {
            again.control(heldDown);
            again.step();
        }
        check(again.ctrl(), "a crouching fighter should still have control");
    }

    // ---------------------------------------------------------------- sounds
    //
    // A character's `type = PlaySnd` controllers and the engine's own hit sound
    // are two different mechanisms, and only one of them is in the character's
    // files: the swing is a controller, and the *impact* is the engine playing
    // the HitDef's `hitsound`. Both are checked here, because a fight with one
    // and not the other is a fight that sounds wrong rather than silent, which
    // is much harder to notice.
    RecordingSink sink;
    af::StateMachine armed;
    armed.setup(&character, 1);
    armed.setOpponent(&dummy);
    armed.setPosition(320.0f);
    armed.setStageBounds(90.0f, 550.0f);
    armed.setSoundSink(&sink);

    // Find a declared command that reaches an attack state and play it, which is
    // the path a swing sound travels: command -> state -1 -> state -> PlaySnd.
    // A fresh machine per candidate: a command that moved the fighter has left
    // it in an attack state, and replaying the next command from there would be
    // testing the wrong thing entirely.
    bool heardSwing = false;
    for (const af::CommandDef& command : commands.commands()) {
        if (!hasButton(command) || command.steps.size() > 3) continue;
        af::StateMachine fresh;
        fresh.setup(&character, 1);
        fresh.setOpponent(&dummy);
        fresh.setPosition(320.0f);
        fresh.setStageBounds(90.0f, 550.0f);
        fresh.setSoundSink(&sink);
        sink.clear();
        for (int i = 0; i < 10; ++i) {
            af::InputState idleTick;
            fresh.control(idleTick);
            fresh.step();
        }
        const Run run = playCommand(fresh, command, 1, 14);
        if (run.leftStand && sink.count() > 0) {
            heardSwing = true;
            break;
        }
    }
    check(heardSwing, "an attack should ask for at least one of its own sounds");

    // And the impact: a HitDef that has landed plays its `hitsound`, which for
    // nearly every character in the format is group 5.
    sink.clear();
    armed.markHitLanded();
    const af::StateMachine::HitDef& landed = armed.armedHitDef();
    std::printf("  hit sound: group %d sample %d\n", landed.hitSoundGroup, landed.hitSoundSample);
    check(sink.count() > 0, "a landed hit should ask for the HitDef's hit sound");
    check(sink.has(landed.hitSoundGroup, landed.hitSoundSample),
          "and the pair it asks for should be the one the HitDef names");

    // ------------------------------------------------------------ knock out
    //
    // One hit that empties the life bar, rather than a long series of small
    // ones: a character is free to regenerate life or to take reduced damage
    // from a repeated hit -- several do -- and neither is what this is testing.
    af::StateMachine::HitInfo finisher = hit;
    finisher.damage = victim.life();
    finisher.hitTime = 30;
    victim.applyHit(finisher);

    check(victim.life() == 0, "a hit that empties the life bar leaves no life");
    check(victim.knockedOut(), "a fighter at zero life is knocked out");
    // A death is not a state of its own: the fighter falls like any other victim
    // and the route it takes is the reference's -- state 0's own `!alive`
    // controller sends it to 5050, the *falling* get-hit state, and 5110's
    // `!alive` controller sends it from the floor into the lying-dead state 5150.
    // This engine's built-in states take the same route, which is what makes a
    // character with no common state file die the same way as one that ships
    // one.
    check(victim.stateNumber() == af::CommonState::kHitFall,
          "a knocked-out fighter should be in the falling get-hit state, on its way down");
    check(victim.moveType() == af::MoveType::Hit,
          "and should be in a hit state rather than standing up");

    // Being knocked out has to stick: a fighter that stood up again would make
    // the knock-out a cosmetic event. Left alone it falls, lands, lies down and
    // then -- because it is dead -- stops getting up, in 5150, for as long as
    // the round lasts.
    for (int tick = 0; tick < 200; ++tick) {
        af::InputState idleTick;
        victim.control(idleTick);
        victim.step();
    }
    check(victim.life() == 0, "a knocked-out fighter stays knocked out");
    check(victim.stateNumber() == af::CommonState::kDead,
          "and lies dead in the knocked-out state rather than getting up again");
    check(victim.moveType() == af::MoveType::Hit,
          "and is still in a hit state, not back on its feet");

    // ------------------------------------------------- the knock-out stand-up
    //
    // The one thing that ends that lying-dead state is the engine's round clock,
    // and it is the engine's because nothing else can be: 5150 is the one get-hit
    // state written with no exit at all. The reference watches its round phase
    // and, once the round is over and the loser has been down for ninety ticks,
    // writes the get-up state and gives the fighter its life back. Round state 3
    // is this engine's name for that phase -- the fight screen sets it on a
    // knock-out or a time-over -- so the stand-up must happen at ninety ticks of
    // it and not one tick before.
    {
        af::Character koChar;
        koChar.loadBehaviour(
            "[Data]\nlife = 1000\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
            "[Statedef -1]\n");
        af::StateMachine downed;
        downed.setup(&koChar, 1);

        af::InputState idleTick;
        downed.control(idleTick);
        downed.step();

        af::StateMachine::HitInfo kill = hit;
        kill.damage = downed.life();
        kill.stateNumber = af::CommonState::kHitHigh;
        downed.applyHit(kill);
        check(downed.stateNumber() == af::CommonState::kHitFall,
              "the stand-up needs a fighter that is on its way down first");

        // The round is still being fought: however long it lies there, it stays
        // dead. This is the knock-out being a knock-out.
        downed.setRoundState(2);
        for (int tick = 0; tick < 150; ++tick) {
            downed.control(idleTick);
            downed.step();
        }
        check(downed.stateNumber() == af::CommonState::kDead,
              "a fighter knocked out during the round does not get up");

        // Now the round ends. It has already been down far longer than ninety
        // ticks, so the stand-up is due on the first tick of the over phase.
        downed.setRoundState(3);
        downed.control(idleTick);
        downed.step();
        check(downed.stateNumber() == af::CommonState::kDownedGetUp,
              "a fighter whose round has ended gets up once it has been down long enough");
        check(downed.life() == downed.maxLife(),
              "and gets its life back to square up for the next round");
        check(downed.knockedOut() == false,
              "and is no longer knocked out, having been stood up and healed");
    }

    // The clock is ninety ticks of the over phase, not "as soon as the round
    // ends": a fighter knocked out on the last tick of the round still gets its
    // second and a half face down. Same setup as above, but the round state is
    // switched the moment the hit lands and the ticks are counted.
    {
        af::Character koChar;
        koChar.loadBehaviour(
            "[Data]\nlife = 1000\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
            "[Statedef -1]\n");
        af::StateMachine downed;
        downed.setup(&koChar, 1);

        af::InputState idleTick;
        downed.control(idleTick);
        downed.step();

        af::StateMachine::HitInfo kill = hit;
        kill.damage = downed.life();
        kill.stateNumber = af::CommonState::kHitHigh;
        downed.applyHit(kill);
        downed.setRoundState(3);

        int ticksDown = 0;
        while (ticksDown < 300 && downed.stateNumber() != af::CommonState::kDownedGetUp) {
            downed.control(idleTick);
            downed.step();
            ++ticksDown;
        }
        check(downed.stateNumber() == af::CommonState::kDownedGetUp,
              "the fighter should be stood up by the engine's own clock");
        // The count includes the fall and the liedown before 5150 is reached, so
        // ninety is a floor on it rather than the exact number of ticks; what is
        // being pinned down is that the stand-up is neither immediate nor
        // unbounded.
        check(ticksDown >= 90, "and not before it has been down for the full ninety ticks");
        std::printf("  knock-out stand-up after %d ticks of the over phase\n", ticksDown);
    }

    // --------------------------------------------------- the clock on a knockdown
    //
    // How long a fighter stays on the floor is `[Data] liedown.time`, and it is
    // the *engine's* clock rather than the animation's -- the reference's own
    // common1.cns defines 5110 with no hand-over at all and leaves the timing to
    // the engine. That distinction is the whole of this block, and it needs a
    // character with no art for the pose to show it: an animation-driven rule
    // calls a character with no animation "finished" on the frame it enters the
    // state, so the fighter is on its feet one tick after being knocked down, no
    // matter what its `liedown.time` says. Seven is chosen over the 60 default so
    // that "the number in the file" and "the number of ticks observed" cannot be
    // confused with each other or with the animation's length.
    //
    // `GetHitVar(recovertime)` is read from the same clock, and it answers both
    // ways round on purpose: a fighter on its feet reads the file's whole number
    // back -- which is what a state that asks the question *before* a knockdown
    // wants -- and one on the floor reads what is left of it.
    {
        af::Character downedChar;
        downedChar.loadBehaviour(
            "[Data]\nliedown.time = 7\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
            "[Statedef -1]\n");
        af::StateMachine downed;
        downed.setup(&downedChar, 1);
        const auto recoverTime = [&] {
            return downed.trigger("gethitvar", {af::Value::text("recovertime")}).toFloat();
        };

        check(recoverTime() == 7.0f,
              "a fighter on its feet reports its own liedown time as the time to recover");
        af::InputState idleTick;
        downed.control(idleTick);
        downed.step();
        check(recoverTime() == 7.0f,
              "and goes on reporting it for as long as it is not in a hit reaction");

        af::StateMachine::HitInfo down;
        down.damage = 40;
        down.hitTime = 1;
        down.pushY = 0.0f;
        down.stateNumber = af::CommonState::kDowned;
        check(downed.applyHit(down), "the knockdown lands");
        check(downed.stateNumber() == af::CommonState::kDowned, "and puts the fighter on the floor");

        // Seven ticks of lying there, and the clock behind `recovertime` counts
        // down one per tick as it goes.
        for (int tick = 1; tick <= 7; ++tick) {
            check(downed.stateNumber() == af::CommonState::kDowned,
                  "the fighter should still be on the floor inside its liedown time");
            check(recoverTime() == static_cast<float>(8 - tick),
                  "the recover time should count down one tick a tick while the fighter lies there");
            downed.control(idleTick);
            downed.step();
        }
        check(downed.stateNumber() == af::CommonState::kDowned,
              "a fighter with no art for the pose lies down for the whole liedown time, not a tick");
        check(recoverTime() == 0.0f, "and its recover time is spent");
        downed.control(idleTick);
        downed.step();
        check(downed.stateNumber() != af::CommonState::kDowned,
              "and it gets up on the next tick rather than lying there for ever");
    }

    // ------------------------------------------------------- the rules of a hit
    //
    // Four rules that the engine owns and no character file can express, checked
    // against a synthetic attacker so that what is being measured is the rule and
    // not somebody's animation:
    //
    //   * a hit freezes *both* fighters for its `pausetime`, which is the
    //     difference between an impact and a gap in an animation;
    //   * holding back blocks, if the attack's `guardflag` admits the height the
    //     defender is at, and a block costs the guard component of `damage` rather
    //     than the hit;
    //   * a hit lands only if the victim's air-combo allowance can pay for it;
    //   * and a refused hit leaves nothing behind -- no freeze, no shake, no state
    //     change -- because the attacker can hold the hitbox out indefinitely.
    {
        af::Character boxer;
        boxer.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[Statedef 200]\ntype = S\nmovetype = A\nphysics = S\nctrl = 0\n\n"
            "[State 200, hit]\ntype = HitDef\ntrigger1 = time = 0\n"
            "attr = S, NA\ndamage = 100, 10\nguardflag = MA\n"
            "pausetime = 6, 9\nground.hittime = 20\nground.velocity = -4\n"
            "air.juggle = 10\n\n"
            "[State 200, end]\ntype = ChangeState\ntrigger1 = time > 40\nvalue = 0\n",
            "[Command]\nname = \"x\"\ncommand = x\n\n"
            // The second command has to be declared too: a trigger naming a command nobody
            // declared is false for ever, which is what the first run of this test proved
            // by never entering the state.
            "[Command]\nname = \"y\"\ncommand = y\n\n"
            "[Statedef -1]\n"
            "[State -1, attack]\ntype = ChangeState\ntrigger1 = command = \"x\"\n"
            "trigger1 = ctrl\nvalue = 200\n");

        // The attacker, armed: state 200 is entered by pressing x, and its HitDef
        // runs on the first tick of the state.
        auto armAttacker = [&](af::StateMachine& attacker) {
            attacker.setup(&boxer, 1);
            attacker.setOpponent(&dummy);
            attacker.setPosition(320.0f);
            attacker.setStageBounds(90.0f, 550.0f);
            af::InputState idleTick;
            attacker.control(idleTick);
            attacker.step();
            af::InputState press;
            press.held = af::Button::kX;
            press.pressed = af::Button::kX;
            attacker.control(press);
            attacker.step();
            af::InputState held;
            held.held = af::Button::kX;
            attacker.control(held);
            attacker.step();
            for (int i = 0; i < 3; ++i) {
                af::InputState tick;
                attacker.control(tick);
                attacker.step();
            }
        };

        af::StateMachine attacker;
        armAttacker(attacker);
        check(attacker.stateNumber() == 200, "the synthetic character should be in its attack state");
        check(attacker.hasArmedHitDef(), "and should have armed a HitDef");
        check(attacker.attackPriority() == 4, "whose priority is the one it declared");

        // A defender standing there with its guard up, and one with its thumb on
        // back. The only difference between them is the input.
        auto makeDefender = [&](af::StateMachine& victim, uint32_t held) {
            victim.setup(&boxer, -1);
            victim.setOpponent(&attacker);
            victim.setPosition(360.0f);
            victim.setStageBounds(90.0f, 550.0f);
            for (int i = 0; i < 4; ++i) {
                af::InputState tick;
                tick.held = held;
                victim.control(tick);
                victim.step();
            }
        };

        af::StateMachine open;
        makeDefender(open, 0);
        const af::StateMachine::HitInfo unguarded = attacker.hitInfoFor(open);
        check(!unguarded.guarded, "a defender that is not holding back does not block");
        check(unguarded.attackerPause == 6 && unguarded.victimPause == 9,
              "and the hit carries the freeze frames the HitDef asked for");

        af::StateMachine braced;
        // The defender faces left, so "back" is the right-hand key: back is the way
        // the fighter is *not* looking.
        makeDefender(braced, af::Button::kRight);
        check(braced.holdingBack(), "a defender facing left holds back with the right key");
        const af::StateMachine::HitInfo guarded = attacker.hitInfoFor(braced);
        check(guarded.guarded, "and holding back blocks the hit");
        check(guarded.damage == 10, "a block costs the guard damage rather than the hit");
        check(guarded.stateNumber == af::CommonState::kGuarded,
              "and puts the blocker in the guard-hit state, not a hit state");

        // Landing them: the damage, the freeze on both sides, and the shake.
        const int bracedLife = braced.life();
        check(braced.applyHit(guarded), "a block is taken");
        check(braced.life() == bracedLife - 10, "the blocker loses exactly the chip damage");
        check(braced.moveGuarded(), "and the engine reports the hit as guarded");
        check(braced.pauseTicks() == 9, "the blocker freezes for the guard pause");
        check(braced.shakeTicks() == 9, "and shakes for it too");

        const int openLife = open.life();
        check(open.applyHit(unguarded), "the unguarded hit is taken");
        check(open.life() == openLife - 100, "for the full damage");
        check(open.pauseTicks() == 9, "the victim freezes for the victim's pause");
        check(open.stateNumber() != 0, "and is put into a hit state");

        // A third defender, airborne with its allowance already spent. The same hit
        // has to be refused outright, and a refusal has to leave no trace at all.
        af::StateMachine spent;
        makeDefender(spent, 0);
        spent.setPosition(360.0f);
        // Lift it off the ground by giving it an air state and some height.
        af::StateMachine::HitInfo lift;
        lift.damage = 0;
        lift.attackerFacing = 1;
        lift.pushX = 0.0f;
        lift.pushY = 8.0f;
        lift.stateNumber = af::CommonState::kHitAir;
        // Long hitstun, deliberately: a one-tick hitstun would end before the next
        // step and the engine would put the fighter back on the ground, which is a
        // different thing from what this is testing.
        lift.hitTime = 120;
        lift.juggleCost = 0;
        spent.applyHit(lift);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            spent.control(tick);
            spent.step();
        }
        check(spent.y() > 0.5f, "the third defender should be off the ground");

        // Spend its whole allowance, then ask for more than is left.
        af::StateMachine::HitInfo drain;
        drain.damage = 0;
        drain.attackerFacing = 1;
        drain.stateNumber = af::CommonState::kHitAir;
        drain.hitTime = 120;
        drain.juggleCost = boxer.airJuggle();
        spent.applyHit(drain);
        const int spentLife = spent.life();
        af::StateMachine::HitInfo tooMuch;
        tooMuch.damage = 100;
        tooMuch.attackerFacing = 1;
        tooMuch.stateNumber = af::CommonState::kHitAir;
        tooMuch.victimPause = 9;
        tooMuch.juggleCost = 10;
        check(!spent.applyHit(tooMuch), "a hit the juggle allowance cannot pay for is refused");
        check(spent.life() == spentLife, "a refused hit does no damage");
        check(spent.pauseTicks() == 0, "and freezes nobody, which is the trap it would otherwise be");
        check(spent.shakeTicks() == 0, "and shakes nobody");
    }

    // ------------------------------------------------- recovery out of a fall
    //
    // The one way out of a fall that is not the floor. `data/common1.cns` hangs it
    // off two controllers inside 5050 -- `[State 5050, 4] ;Recover near ground`
    // and `[State 5050, 5] ;Recover in mid air` -- both gated on `alive`, on
    // `CanRecover` and on the `recovery` command, and both landing the victim in a
    // state the reference defines further down the file: 5200 (which lands in
    // 5201) for the near-ground one and 5210 for the mid-air one. The two
    // controllers are in all 42 pack `common1.cns` files under local_chars, and
    // 41 of those carry 5200's own `SelfState 5201` as well. 5071, the trip's
    // fall, carries neither -- a tripped fighter stays tripped -- which is why
    // nothing here leaves from there.
    //
    // What is pinned here is the engine's half of it, because a character with no
    // common state file of its own has nothing to run those controllers. That
    // half is: the two exits exist at all, the near-ground one is preferred when
    // both conditions hold (the reference resolves that by writing it as
    // controller 4), the window they are gated on is the *hit's* clock rather
    // than the fall state's, and 5201's entry -- the hop back out of the floor the
    // recovery sank into -- is supplied.
    {
        // A victim with a file, but no common states in it: exactly the case the
        // built-in table is for. A machine with no character at all would not do --
        // `character_ == nullptr` skips the whole state logic in `step()` and the
        // dummy would never reach a fall.
        af::Character faller;
        faller.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n", "[Statedef -1]\n");

        // A launch, by hand. The victim goes straight into 5030, the knocked-away
        // state, which is a legitimate entry -- a character's own controller can
        // `ChangeState 5030` and this is that -- but the state's *exit* is written
        // as a split on `GetHitVar(fall)`, so the hit has to carry the fall flag as
        // well as the state number. It is the flag `hitfall` reads and the one
        // `[State 5000, 6]` and `[State 5080, 5]` split on, and 5030 hands over to
        // the fall (5050) when it is set and to the air recovery (5040) when it is
        // not. Everything below is a fall, so it is set here; the test that a plain
        // air hit is *not* a fall is the one that pins the other side of it.
        //
        // The push is 9 units a tick straight up, which at the engine's .35 gravity
        // is about 26 ticks up and 26 back down -- long enough to have a
        // conversation with.
        const auto launch = [&](af::StateMachine& victim, int stun, int recoverTicks,
                                bool recoverFlag) {
            victim.setup(&faller, 1);
            victim.setStageBounds(90.0f, 550.0f);
            victim.setPosition(320.0f);
            af::StateMachine::HitInfo hit;
            hit.damage = 1;
            hit.attackerFacing = 1;
            hit.hitTime = stun;
            hit.pushX = 0.0f;
            hit.pushY = 9.0f;
            hit.stateNumber = af::CommonState::kHitAirFall;
            hit.falls = true;
            hit.fallRecover = recoverFlag;
            hit.fallRecoverTime = recoverTicks;
            hit.fallRecoverTicks = recoverTicks;
            hit.victimPause = 1;
            check(victim.applyHit(hit), "the launch is taken");
        };

        // x+y is what the fallback uses -- it is the binding the reference's own
        // kfm.cmd gives `recovery` -- and the character here has no command list for
        // the engine to ask instead. The buttons are *released* by default: holding
        // them for a whole descent would recover the victim the moment its window
        // opened, and then there would be no window left to measure.
        const auto tickOf = [&](af::StateMachine& victim, uint32_t extra) {
            af::InputState tick;
            tick.held = extra;
            victim.control(tick);
            victim.step();
        };
        const auto idleTick = [&](af::StateMachine& victim) { tickOf(victim, 0); };
        const auto recoveryTick = [&](af::StateMachine& victim) {
            tickOf(victim, af::Button::kX | af::Button::kY);
        };
        // Falls the victim until it is in the fall *and* `ready` says so of it,
        // checking before each tick so the moment is the one the test named.
        const auto fallUntil = [&](af::StateMachine& victim, const auto& ready) {
            for (int guard = 0; guard < 400; ++guard) {
                if (victim.stateNumber() == af::CommonState::kHitFall && ready(victim)) return;
                idleTick(victim);
            }
        };

        // --- the hitstun is not the fall
        //
        // A launched victim spends its hitstun in the reaction before the table
        // hands it to the fall, and that reaction is not a state that listens for
        // a recovery. With no 5030 art the reference's `AnimTime = 0` exit fires
        // at once, so the reaction is 5035 -- the transition -- which still waits
        // for the stun gate before it hands on. Pressing the buttons there must
        // do nothing -- and `CanRecover` is *already* true while it does nothing,
        // because the flag is the hit's. Worth pinning, because it is the one
        // place where the trigger says yes and the engine says no, and the reason
        // is the stun rather than the window.
        af::StateMachine stunned;
        launch(stunned, 12, 0, true);
        for (int i = 0; i < 4; ++i) recoveryTick(stunned);
        check(stunned.stateNumber() == af::CommonState::kHitAirTransition,
              "a recovery pressed during the hitstun a launch begins with changes nothing");
        check(stunned.trigger("canrecover", {}).toFloat() != 0.0f,
              "though `canrecover` is already true there: the engine is holding the built-in "
              "table still for the stun, not refusing the recovery");

        // --- mid air: 5210, the roll
        af::StateMachine high;
        launch(high, 1, 0, true);
        // The top of the arc, which is where the reference's own condition for the
        // mid-air recovery -- `Vel Y > -1`, i.e. no longer rising -- is met. It is
        // also 100-odd units of height away from the near-ground one, so the two
        // cannot be confused for each other here.
        fallUntil(high, [](const af::StateMachine& v) { return v.velY() < 1.0f && v.y() > 40.0f; });
        check(high.stateNumber() == af::CommonState::kHitFall && high.y() > 40.0f,
              "the launch becomes a fall, and it has height on it");
        check(high.velY() < 1.0f,
              "and it is at the top of the arc, where only the mid-air recovery can reach it");

        const float beforePress = high.y();
        recoveryTick(high);
        check(high.stateNumber() == af::CommonState::kHitAirFallRecover,
              "and a recovery pressed there takes the mid-air roll, not the near-ground one");
        check(high.y() == beforePress,
              "with the position frozen from the first tick, which is its `PosFreeze value = 4`");

        const float frozen = high.y();
        for (int i = 0; i < 3; ++i) idleTick(high);
        check(high.stateNumber() == af::CommonState::kHitAirFallRecover, "the roll lasts four ticks");
        check(high.y() == frozen, "with the position frozen for all of them");

        const float beforeLift = high.velY();
        idleTick(high);
        check(high.stateNumber() == af::CommonState::kHitAirFallRecover,
              "and the roll outlasts its own lift");
        check(high.velY() > beforeLift && high.velY() > 0.0f,
              "which is the reference's `Time = 4` lift: a falling victim comes out of the roll "
              "rising, which is the whole of what a tech is");

        // And the rise bleeds off at the reference's own rate rather than the
        // fighter's: 5210's gravity is `.35`, which the engine supplies because the
        // state is `physics = N` and would otherwise hang there. It is a different
        // number from the `.44` a fall of the same fighter's runs at, so this is two
        // separate constants and not one reused.
        const float rising = high.velY();
        idleTick(high);
        check(std::fabs((high.velY() - rising) + 0.35f) < 0.001f,
              "and the rise bleeds off at the reference's `.35` a tick, which is a roll's gravity "
              "and not the arc's");

        // --- the two exits are ordered, and the order is a distance
        //
        // The reference writes the near-ground controller first (`[State 5050, 4]`
        // before `[State 5050, 5]`), and its condition is `Pos Y >= -20` -- a
        // distance from the floor, not "past the top of the arc". A victim falling
        // *from* the top is still above that floor-window and takes the roll, which
        // is what carries it the rest of the way down to try again.
        af::StateMachine boundary;
        launch(boundary, 1, 0, true);
        fallUntil(boundary, [](const af::StateMachine& v) {
            return v.velY() < 0.0f && v.y() > 20.0f && v.y() < 30.0f;
        });
        check(boundary.stateNumber() == af::CommonState::kHitFall && boundary.velY() < 0.0f &&
                  boundary.y() > 20.0f,
              "a victim falling hard, 20-odd units off the floor and not yet inside it");
        recoveryTick(boundary);
        check(boundary.stateNumber() == af::CommonState::kHitAirFallRecover,
              "takes the roll rather than the near-ground recovery -- the window the near-ground "
              "one tests is a distance from the floor, not a moment after the apex");
        // And the roll still does its job on a fast fall: this victim was dropping
        // at seven-odd units a tick, and the `VelAdd y = -4.5` at `Time = 4` is what
        // turns that into a rise -- the two conditional nudges alone leave it
        // falling.
        for (int i = 0; i < 4; ++i) idleTick(boundary);
        check(boundary.stateNumber() == af::CommonState::kHitAirFallRecover && boundary.velY() > 0.0f,
              "and a fall caught this low comes out of the roll rising as well");

        // The mid-air window has a *near* edge too: a victim still on the way up is
        // not recovering, it is being launched. The reference's `Vel Y > -1` is a
        // speed, not a direction, and reading it as "any fall at all" would let a
        // rising victim tech out of the launch itself.
        af::StateMachine launched;
        launch(launched, 1, 0, true);
        fallUntil(launched, [](const af::StateMachine& v) {
            return v.velY() > 1.0f && v.velY() < 4.0f && v.y() > 20.0f;
        });
        check(launched.stateNumber() == af::CommonState::kHitFall && launched.velY() > 1.0f,
              "a victim still rising");
        for (int i = 0; i < 3; ++i) recoveryTick(launched);
        check(launched.stateNumber() == af::CommonState::kHitFall,
              "cannot recover from it: the window is the top of the arc, not anywhere above the "
              "floor on the way up");

        // --- the stick aims the roll
        //
        // Same launch, same tick, one machine holding up through it -- the
        // reference's `VelAdd y = -2` behind `Command = "holdup"`.
        af::StateMachine plain;
        af::StateMachine steered;
        launch(plain, 1, 0, true);
        launch(steered, 1, 0, true);
        const auto bothAtTheTop = [](const af::StateMachine& v) {
            return v.y() > 20.0f && v.velY() < 1.0f;
        };
        for (int i = 0; i < 400; ++i) {
            if (plain.stateNumber() == af::CommonState::kHitFall && bothAtTheTop(plain) &&
                steered.stateNumber() == af::CommonState::kHitFall && bothAtTheTop(steered)) {
                break;
            }
            idleTick(plain);
            idleTick(steered);
        }
        recoveryTick(plain);
        recoveryTick(steered);
        check(plain.stateNumber() == af::CommonState::kHitAirFallRecover &&
                  steered.stateNumber() == af::CommonState::kHitAirFallRecover,
              "two victims launched and pressed together both take the roll");
        for (int i = 0; i < 5; ++i) {
            idleTick(plain);
            tickOf(steered, af::Button::kUp);
        }
        check(plain.stateNumber() == af::CommonState::kHitAirFallRecover &&
                  steered.stateNumber() == af::CommonState::kHitAirFallRecover,
              "and both are still inside it five ticks later");
        check(steered.velY() > plain.velY(),
              "but the one holding up comes out of the roll rising faster, which is how a roll is "
              "aimed");

        // --- near the ground: 5200, and the hop out of the floor it lands in
        af::StateMachine low;
        launch(low, 1, 0, true);
        fallUntil(low, [](const af::StateMachine& v) { return v.velY() < 0.0f && v.y() <= 20.0f; });
        check(low.stateNumber() == af::CommonState::kHitFall && low.y() <= 20.0f &&
                  low.velY() < 0.0f,
              "the fall reaches the near-ground window the reference tests for -- falling, "
              "and inside 20 units of the floor");
        recoveryTick(low);
        check(low.stateNumber() == af::CommonState::kHitFallRecover,
              "and a recovery pressed there takes the near-ground recovery, not the roll");

        int guard = 0;
        while (guard++ < 300 && low.stateNumber() == af::CommonState::kHitFallRecover) {
            idleTick(low);
        }
        check(low.stateNumber() == af::CommonState::kHitFallRecoverGround,
              "which lands in 5201, the recovery on the ground");
        check(low.y() == 0.0f, "put back on the floor");
        check(low.velY() > 0.0f, "and rising off it, which is that state's `VelSet y = -3.5`");
        // `x` in a state's controller parameters is local, so "backwards" is the
        // side the fighter is not facing -- `.15` a tick, against `facing`.
        check(std::fabs(low.velX()) > 0.1f && std::fabs(low.velX()) < 0.2f &&
                  low.velX() * static_cast<float>(low.facing()) < 0.0f,
              "and drifting backwards, which is its `VelSet x = -.15`");

        guard = 0;
        while (guard++ < 300 && low.stateNumber() == af::CommonState::kHitFallRecoverGround) {
            idleTick(low);
        }
        check(low.stateNumber() == af::CommonState::kLand || low.stateNumber() == af::CommonState::kStand,
              "and the hop ends with the fighter on its feet");
        check(low.moveType() == af::MoveType::Idle, "out of the hit reaction");

        // The engine's fallback binding for a character with no command list is the
        // *pair* of buttons, which is what the reference's own kfm.cmd puts behind
        // the `recovery` command. One of them is a punch, and pressing it mid-fall
        // is not a tech.
        af::StateMachine halfPressed;
        launch(halfPressed, 1, 0, true);
        fallUntil(halfPressed, [](const af::StateMachine& v) {
            return v.velY() < 1.0f && v.y() > 40.0f;
        });
        for (int i = 0; i < 3; ++i) tickOf(halfPressed, af::Button::kX);
        check(halfPressed.stateNumber() == af::CommonState::kHitFall,
              "and half the fallback binding is not a recovery: x alone leaves the victim falling");

        // --- what closes the window
        //
        // `fall.recover = 0` on the hit, which is the one thing a launcher can write
        // to make a fall untechable. Pressed at the top of the arc, where the
        // mid-air branch would otherwise take it -- and where there are two dozen
        // ticks of air left, so a victim that stays in the fall is refusing rather
        // than merely running out of height.
        af::StateMachine unrecoverable;
        launch(unrecoverable, 1, 0, false);
        fallUntil(unrecoverable, [](const af::StateMachine& v) {
            return v.velY() < 1.0f && v.y() > 40.0f;
        });
        check(unrecoverable.stateNumber() == af::CommonState::kHitFall,
              "the unrecoverable launch still falls");
        check(unrecoverable.trigger("canrecover", {}).toFloat() == 0.0f,
              "but `fall.recover = 0` closes `canrecover` on it");
        for (int i = 0; i < 5; ++i) recoveryTick(unrecoverable);
        check(unrecoverable.stateNumber() == af::CommonState::kHitFall,
              "and five recoveries pressed at the top of it leave the victim falling");

        // `nohardcodedkeys` is the character taking its own input away, and every
        // key the engine supplies on its behalf goes with it. Raised from state -1,
        // which runs every tick before the current state, so the flag is up by the
        // time the fall is asked.
        af::Character keyless;
        keyless.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                              "[Statedef -1]\n[State -1, keys off]\ntype = AssertSpecial\n"
                              "trigger1 = 1\nflag = nohardcodedkeys\n");
        af::StateMachine deaf;
        deaf.setup(&keyless, 1);
        deaf.setStageBounds(90.0f, 550.0f);
        deaf.setPosition(320.0f);
        {
            af::StateMachine::HitInfo hit;
            hit.damage = 1;
            hit.attackerFacing = 1;
            hit.hitTime = 1;
            hit.pushY = 9.0f;
            hit.stateNumber = af::CommonState::kHitAirFall;
            hit.falls = true;
            hit.fallRecoverTicks = 0;
            hit.victimPause = 1;
            check(deaf.applyHit(hit), "the second launch is taken");
        }
        fallUntil(deaf, [](const af::StateMachine& v) {
            return v.velY() < 1.0f && v.y() > 40.0f;
        });
        check(deaf.stateNumber() == af::CommonState::kHitFall, "and it falls as well");
        for (int i = 0; i < 5; ++i) recoveryTick(deaf);
        check(deaf.stateNumber() == af::CommonState::kHitFall,
              "but a character that has asserted `nohardcodedkeys` is not listening for a "
              "recovery the engine supplies");

        // --- the window is the hit's clock, not the fall state's
        //
        // `fall.recovertime` is "the minimum time before the player can recover from
        // a fall", and the clock on it starts at the *hit*: a launched victim spends
        // its hitstun in the shake before the table hands it to 5050, so a window
        // measured from the fall state would open that much later than the
        // reference's and put a default hit's recovery out of reach altogether.
        af::Character launcher;
        launcher.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, hit]\ntype = HitDef\ntrigger1 = time = 0\n"
            "attr = S, NA\ndamage = 10\npausetime = 2, 4\n"
            "fall = 1\nfall.recover = 1\nfall.recovertime = 3\nair.velocity = 0,-9\n",
            "[Statedef -1]\n");
        af::StateMachine winder;
        winder.setup(&launcher, 1);
        winder.setPosition(360.0f);
        idleTick(winder);
        check(winder.hasArmedHitDef(), "the synthetic launcher arms its HitDef");
        const af::StateMachine::HitInfo windowed = winder.hitInfoFor(stunned);
        check(windowed.fallRecoverTime == 3 && windowed.fallRecoverTicks == 3,
              "and `fall.recovertime = 3` arrives both as the number the trigger reports and as "
              "the clock that counts it down");

        af::StateMachine ticking;
        launch(ticking, 120, 3, true);
        int openTick = -1;
        for (int i = 1; i <= 8; ++i) {
            idleTick(ticking);
            if (openTick < 0 && ticking.trigger("canrecover", {}).toFloat() != 0.0f) openTick = i;
        }
        check(openTick == 4,
              "and it runs down one tick a tick from the hit: three ticks of window behind one "
              "tick of the hit's own hitpause, which the freeze does not spend");
    }

    // ---------------------------------------------------- the persist flags
    //
    // A statedef's `*persist` lines are the one place a character chooses
    // whether entering a state keeps the combat books or drops them. MUGEN
    // resets by default, and the engine pays that out in enterState; the flags
    // themselves are read in cns.cpp. What is pinned here is that the parsed
    // flag actually reaches the reset decision -- a parse-only test can see the
    // field but not whether the runtime honours it.
    {
        // Two characters, identical but for the destination state's header: one
        // asks for the books to persist, the other leaves the default reset.
        auto makeCharacter = [](bool persist) {
            std::string cns =
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = 1\nvalue = 200\n\n"
                "[Statedef 200]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 200, arm]\ntype = HitDef\ntrigger1 = time = 0\n"
                "damage = 10\nattr = S, NA\n\n"
                // The books are spent while 200 is the current state, and the
                // change happens a few ticks later so the test can see them
                // before and after.
                "[State 200, next]\ntype = ChangeState\ntrigger1 = time = 5\nvalue = 300\n\n";
            cns += "[Statedef 300]\ntype = S\nphysics = N\nctrl = 0\n";
            if (persist) {
                cns += "movehitpersist = 1\nhitdefpersist = 1\n";
            }
            cns += "\n[State 300, hold]\ntype = VelAdd\ntrigger1 = 1\nx = 0\n";
            return cns;
        };

        auto runTo = [&](af::Character& character, af::StateMachine& machine) {
            machine.setup(&character, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(320.0f);
            machine.setStageBounds(90.0f, 550.0f);
            for (int i = 0; i < 40 && machine.stateNumber() != 200; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
            for (int i = 0; i < 3; ++i) {  // settle into 200; the HitDef arms at time 0
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
        };

        af::Character keeper;
        keeper.loadBehaviour(makeCharacter(true), "[Statedef -1]\n");

        // The armed HitDef and the move-hit flag are tested on separate runs,
        // because landing the hit is itself what spends the HitDef: one run
        // would clobber the book the other is watching.
        af::StateMachine keepingHitDef;
        runTo(keeper, keepingHitDef);
        check(keepingHitDef.stateNumber() == 200,
              "the persist character should reach its attack state");
        check(keepingHitDef.hasArmedHitDef(), "with a HitDef armed");
        for (int i = 0; i < 20 && keepingHitDef.stateNumber() == 200; ++i) {
            af::InputState tick;
            keepingHitDef.control(tick);
            keepingHitDef.step();
        }
        check(keepingHitDef.stateNumber() == 300, "then change into the persist state");
        check(keepingHitDef.hasArmedHitDef(), "an armed HitDef survives a state that says hitdefpersist");

        af::StateMachine keepingMoveHit;
        runTo(keeper, keepingMoveHit);
        keepingMoveHit.markHitLanded();
        check(keepingMoveHit.moveHit(), "a landed hit is recorded");
        for (int i = 0; i < 20 && keepingMoveHit.stateNumber() == 200; ++i) {
            af::InputState tick;
            keepingMoveHit.control(tick);
            keepingMoveHit.step();
        }
        check(keepingMoveHit.stateNumber() == 300, "and reaches the persist state");
        check(keepingMoveHit.moveHit(), "the move-hit flag survives movehitpersist");

        af::Character resetter;
        resetter.loadBehaviour(makeCharacter(false), "[Statedef -1]\n");

        af::StateMachine resettingHitDef;
        runTo(resetter, resettingHitDef);
        for (int i = 0; i < 20 && resettingHitDef.stateNumber() == 200; ++i) {
            af::InputState tick;
            resettingHitDef.control(tick);
            resettingHitDef.step();
        }
        check(resettingHitDef.stateNumber() == 300, "the default character reaches the same destination");
        check(!resettingHitDef.hasArmedHitDef(), "a state without hitdefpersist drops the armed HitDef");

        af::StateMachine resettingMoveHit;
        runTo(resetter, resettingMoveHit);
        resettingMoveHit.markHitLanded();
        for (int i = 0; i < 20 && resettingMoveHit.stateNumber() == 200; ++i) {
            af::InputState tick;
            resettingMoveHit.control(tick);
            resettingMoveHit.step();
        }
        check(!resettingMoveHit.moveHit(), "and one without movehitpersist drops the move-hit flag");
    }

    // ------------------------------------------- the move-contact tick counters
    //
    // MOVECONTACT et al are not flags in the reference: they are tick counters
    // that a contact sets to one -- the blow connects between one tick's step and
    // the next -- and that every later tick counts on from there, until a state
    // change without `movehitpersist` drops them. A file asks `movecontact = 3`
    // exactly three ticks after a connect, and `movecontact >= 2 && <= 4` opens
    // a *window*; a boolean can only ever answer the first tick.
    {
        af::Character contactTicker;
        contactTicker.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = 1\nvalue = 200\n\n"
            "[Statedef 200]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
            "[State 200, arm]\ntype = HitDef\ntrigger1 = time = 0\ndamage = 10\n"
            "attr = S, NA\n\n"
            "[State 200, snap]\ntype = VarSet\ntrigger1 = 1\nv = 0\nvalue = movecontact\n"
            "ignorehitpause = 1\n\n"
            "[State 200, snap hit]\ntype = VarSet\ntrigger1 = 1\nv = 1\nvalue = movehit\n"
            "ignorehitpause = 1\n\n"
            "[State 200, next]\ntype = ChangeState\ntrigger1 = time = 8\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nphysics = N\nctrl = 0\n\n"
            "[State 300, hold]\ntype = VelAdd\ntrigger1 = 1\nx = 0\n\n"
            "[Statedef -1]\n",
            "[Statedef -1]\n");
        auto tickOnce = [](af::StateMachine& machine) {
            af::InputState tick;
            machine.control(tick);
            machine.step();
        };
        af::StateMachine striker;
        striker.setup(&contactTicker, 1);
        striker.setOpponent(&dummy);
        striker.setPosition(320.0f);
        striker.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 40 && striker.stateNumber() != 200; ++i) tickOnce(striker);
        for (int i = 0; i < 3; ++i) tickOnce(striker);
        check(striker.stateNumber() == 200, "the striker reached its attack state");
        auto seenContact = [&striker] {
            return static_cast<int>(striker.trigger("var", {af::Value::number(0.0f)}).toFloat());
        };
        auto seenHit = [&striker] {
            return static_cast<int>(striker.trigger("var", {af::Value::number(1.0f)}).toFloat());
        };
        check(seenContact() == 0, "an attack with no contact reads `movecontact = 0`");
        striker.markHitLanded();
        // The contact fell between the previous tick's step and this one: the
        // first tick's controllers read one, and every later tick counts on --
        // read here through a var the state itself snapshots mid-tick, which is
        // what a file's own `trigger1 = movecontact = n` sees.
        tickOnce(striker);
        check(seenContact() == 1, "the first tick after a connect reads `movecontact = 1`");
        check(seenHit() == 1, "and `movehit` counts the same way");
        tickOnce(striker);
        check(seenContact() == 2, "the next tick reads two");
        tickOnce(striker);
        check(seenContact() == 3, "then three");
        tickOnce(striker);
        check(seenContact() == 4, "then four");
        check(striker.trigger("movecontact", {}).toInt() == 5,
              "and the trigger reports the number, not a flag");
        // A state entry that does not say `movehitpersist` spends the books,
        // counters included.
        for (int i = 0; i < 12 && striker.stateNumber() == 200; ++i) tickOnce(striker);
        check(striker.stateNumber() == 300, "the striker changed into its next state");
        check(striker.trigger("movecontact", {}).toInt() == 0,
              "and the counter dropped at the state change");
    }


    // ------------------------------------------ the fall gravity is the hit's
    //
    // `GetHitVar(yaccel)` is the one number a launched victim cannot do without:
    // the whole falling family (5030, 5035, 5040, 5050, 5071, 5200) writes
    // `VelAdd y = GetHitVar(yaccel)` into the air, and `physics = N` in their
    // headers applies nothing of its own -- so the fall a *hit* built falls by
    // the hit's own gravity, not by the fighter's movement gravity. The
    // reference's default for the former is .35 in the machine constants, a
    // good deal lighter than the .44 a fighter's own jump carries; a `HitDef`
    // writing `yaccel = .7` falls visibly heavier, and one that keeps the
    // default falls more slowly than the jump. The engine dropped both of
    // those: the fall used the fighter's `yaccel` whatever hit built it.
    {
        af::Character fallerBook;
        fallerBook.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n",
                                 "[Statedef -1]\n");
        auto fallTick = [](af::StateMachine& victim) {
            af::InputState tick;
            victim.control(tick);
            victim.step();
        };
        af::StateMachine gravity;
        gravity.setup(&fallerBook, 1);
        gravity.setOpponent(&dummy);
        gravity.setPosition(360.0f);
        gravity.setStageBounds(90.0f, 550.0f);
        af::StateMachine::HitInfo hit;
        hit.damage = 1;
        hit.attackerFacing = 1;
        hit.pushX = 0.0f;
        hit.pushY = 9.0f;
        hit.hitTime = 12;
        hit.stateNumber = af::CommonState::kHitAirFall;
        hit.falls = true;
        // The number the character's own falling states would use.
        hit.yaccel = 0.5f;
        check(gravity.applyHit(hit), "the launch is taken");
        check(gravity.trigger("gethitvar", {af::Value::text("yaccel")}).toFloat() == 0.5f,
              "`GetHitVar(yaccel)` reports the hit's own number");
        // In the fall itself, one tick of descent loses exactly the hit's
        // gravity. It is measured only well above the floor and clearly
        // falling, where the landing clamp and the float around the arc's top
        // cannot touch the reading.
        int measured = 0;
        float previous = 0.0f;
        bool ok = true;
        for (int i = 0; i < 160; ++i) {
            fallTick(gravity);
            if (gravity.stateNumber() != af::CommonState::kHitFall || gravity.velY() >= -1.0f)
                continue;
            if (measured > 0) {
                const float drop = previous - gravity.velY();
                if (std::fabs(drop - 0.5f) > 1e-3f) {
                    if (measured == 1) {
                        std::printf("  fall gravity: expected .5 a tick, measured %f\n",
                                    static_cast<double>(drop));
                    }
                    ok = false;
                }
            }
            previous = gravity.velY();
            ++measured;
        }
        check(measured >= 6, "the fall was airborne long enough to measure");
        check(ok, "and it descended by the hit's gravity, not the fighter's");
    }

    // --------------------------------------------- the air recovery roll (5210)
    //
    // The mid-air recovery is the reference's own state, and its four beats
    // are the file's: `PosFreeze Time < 4`, the lift `VelMul` and `VelAdd`
    // off `velocity.air.gethit.airrecover` at `Time = 4`, the steer off the
    // stick held then, and `NotHitBy SCA` for 15 ticks covering all of it.
    // It ends on its own "Land on ground" (`Vel Y > 0 && Pos Y >= 0`) into
    // the jump-land state, with the fighter's control back, and its kill
    // clock is `CtrlSet value = 1` at `Time = 20`, not sooner.
    {
        af::Character rollerBook;
        rollerBook.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n",
                                 "[Statedef -1]\n");
        af::StateMachine roller;
        roller.setup(&rollerBook, 1);
        roller.setOpponent(&dummy);
        roller.setPosition(360.0f);
        roller.setStageBounds(90.0f, 550.0f);
        af::StateMachine::HitInfo hit;
        hit.damage = 1;
        hit.attackerFacing = 1;
        hit.pushX = -2.0f;  // a little side carry the roll keeps through the lift
        hit.pushY = 9.0f;
        hit.hitTime = 12;
        hit.stateNumber = af::CommonState::kHitAirFall;
        hit.falls = true;
        hit.fallRecover = true;
        hit.fallRecoverTime = 3;
        hit.fallRecoverTicks = 3;
        hit.victimPause = 2;
        check(roller.applyHit(hit), "the launch is taken");
        // Recovery is x+y, the binding the fallback itself uses.
        const uint32_t recovery = af::Button::kX | af::Button::kY;
        af::InputState recoveryTick;
        recoveryTick.held = recovery;
        bool rolled = false;
        for (int i = 0; i < 60 && roller.stateNumber() != af::CommonState::kHitAirFallRecover; ++i) {
            roller.control(recoveryTick);
            roller.step();
            rolled = true;
        }
        check(rolled && roller.stateNumber() == af::CommonState::kHitAirFallRecover,
              "the recovery press rolled out of the fall");

        // --- the roll itself: four frozen ticks, then the lift
        const float entryY = roller.y();
        const float entryVelY = roller.velY();
        const float entryVelX = roller.velX();
        for (int debug = 0; debug < 200 && roller.stateNumber() == af::CommonState::kHitAirFallRecover;
             ++debug) {
            const int t = roller.stateTime();
            roller.control(recoveryTick);
            roller.step();
            if (t < 4) {
                check(roller.y() == entryY, "the roll holds its position for four ticks");
                check(roller.velY() == entryVelY && roller.velX() == entryVelX,
                      "and its velocity carries the same numbers through");
            } else if (t == 4) {
                check(roller.velY() ==
                          entryVelY * 0.2f + 4.5f - 0.35f,
                      "the lift is the airrecover pair: mul .2, lift 4.5, gravity .35");
                check(roller.velX() == entryVelX * 0.5f,
                      "and the x carry slows by the airrecover mul.x");
                break;
            }
        }
        check(roller.velY() > entryVelY, "and the roll is in fact rising");
        // Gravity runs at the airrecover number from tick 4 on, one tick
        // at a time.
        const float liftVelY = roller.velY();
        roller.control(recoveryTick);
        roller.step();
        check(std::fabs(roller.velY() - (liftVelY - 0.35f)) < 1e-3f,
              "and gravity afterwards is `airrecover.yaccel`, a tick a tick");
        // --- the immunity and the control clock, on a fresh roll: the
        // probes above spent ticks of the one 15-tick window, and the
        // window's whole length is its own question.
        auto launchRoller = [](af::StateMachine& machine) {
            af::StateMachine::HitInfo hit;
            hit.damage = 1;
            hit.attackerFacing = 1;
            hit.pushX = -2.0f;
            hit.pushY = 9.0f;
            hit.hitTime = 12;
            hit.stateNumber = af::CommonState::kHitAirFall;
            hit.falls = true;
            hit.fallRecover = true;
            hit.fallRecoverTime = 3;
            hit.fallRecoverTicks = 3;
            hit.victimPause = 2;
            check(machine.applyHit(hit), "the launch is taken");
        };
        auto runToRoll = [](af::StateMachine& machine, const af::InputState& tick,
                            int cap = 60) {
            for (int i = 0; i < cap && machine.stateNumber() != af::CommonState::kHitAirFallRecover;
                 ++i) {
                machine.control(tick);
                machine.step();
            }
            check(machine.stateNumber() == af::CommonState::kHitAirFallRecover,
                  "the probed roll reached the recovery");
        };
        af::InputState freshRecoveryTick;
        freshRecoveryTick.held = af::Button::kX | af::Button::kY;
        af::StateMachine immuneRoller;
        immuneRoller.setup(&rollerBook, 1);
        immuneRoller.setOpponent(&dummy);
        immuneRoller.setPosition(360.0f);
        immuneRoller.setStageBounds(90.0f, 550.0f);
        launchRoller(immuneRoller);
        runToRoll(immuneRoller, freshRecoveryTick);
        int immune = 0;
        while (immuneRoller.ignoringHits() && immune < 30) {
            ++immune;
            immuneRoller.control(freshRecoveryTick);
            immuneRoller.step();
            if (immuneRoller.stateNumber() != af::CommonState::kHitAirFallRecover) break;
        }
        check(immune == 15, "the roll's `NotHitBy` covered its whole length");
        // The kill clock on the same fresh timeline: control stays off through
        // `Time = 20` (read here as the state's own tick numbers, which run
        // one behind step()'s post-step counter) and is back on the tick after.
        af::StateMachine clockRoller;
        clockRoller.setup(&rollerBook, 1);
        clockRoller.setOpponent(&dummy);
        clockRoller.setPosition(360.0f);
        clockRoller.setStageBounds(90.0f, 550.0f);
        launchRoller(clockRoller);
        runToRoll(clockRoller, freshRecoveryTick);
        bool clockOk = false;
        for (int guard = 0; guard < 60 && clockRoller.stateNumber() == af::CommonState::kHitAirFallRecover;
             ++guard) {
            const int t = clockRoller.stateTime();
            if (t <= 20 && clockRoller.ctrl()) {
                check(false, "control came back before `Time = 20`");
                break;
            }
            if (t > 20) {
                clockOk = clockRoller.ctrl();
                break;
            }
            clockRoller.control(freshRecoveryTick);
            clockRoller.step();
        }
        // --- the landing: the reference's own "Land on ground" exit
        for (int guard = 0; guard < 300 && roller.stateNumber() == af::CommonState::kHitAirFallRecover;
             ++guard) {
            roller.control(freshRecoveryTick);
            roller.step();
        }
        // The reference's `ChangeState 52 ;JUMP_LAND` is the exit, but 52 is a
        // pose: this fighter ships no header and no art for it, so the same tick
        // that enters it finds nothing to play and hands it to standing -- the
        // in-tick chain `5210 -> 52 -> 0`. That is observable in `prevstateno`,
        // which is 52 once the chain is done, and it is the same allowance the
        // near-ground hop's landing makes above. A character with its own 52
        // stays there instead, so both answers are the state 5210 exits into.
        const bool landed = roller.stateNumber() == af::CommonState::kLand ||
                            (roller.stateNumber() == af::CommonState::kStand &&
                             roller.prevStateNumber() == af::CommonState::kLand);
        check(landed && roller.y() <= 0.0f && roller.ctrl(),
              "the roll ends on the floor in the jump-land state");
    }


    // ------------------------------------------------- the entity controllers
    //
    // Explods and projectiles are what a Dragon Ball roster is made of: sparks, dust,
    // charge auras and every beam. A state machine does not own them -- they belong to
    // the fight and outlive the state that made them -- so what is tested here is that
    // the controllers *ask* for the right thing, in the right place, with the properties
    // the format says.
    {
        af::Character gunner;
        gunner.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            // An aura bound to the fighter, and a spark that is not.
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = S\nctrl = 0\n\n"
            "[State 300, aura]\ntype = Explod\ntrigger1 = time = 0\nanim = 900\n"
            "id = 77\npos = 12, -30\nbindtime = -1\nsprpriority = -2\n"
            "scale = 1.5, 1.5\nremove\ntrans = add\n\n"
            "[State 300, spark]\ntype = Explod\ntrigger1 = time = 0\nanim = 901\n"
            "pos = -8, -40\nremovetime = 6\n\n"
            "[State 300, beam]\ntype = HitDef\ntrigger1 = time = 0\ndamage = 55\n"
            "ground.velocity = -6\nattr = S, NA\n\n"
            "[State 300, fire]\ntype = Projectile\ntrigger1 = time = 0\nprojanim = 902\n"
            "projhitanim = 903\nprojhits = 3\nprojmisstime = 8\nprojremove = 0\n"
            "velocity = 5, -1\noffset = 20, -30\n\n"
            // A move that says it cannot be hit for a while, and one that takes its
            // movement away from the engine.
            "[State 300, hold]\ntype = NotHitBy\ntrigger1 = time = 0\nvalue = SCA\ntime = 20\n\n"
            "[Statedef 400]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 400, frozen]\ntype = AssertSpecial\ntrigger1 = 1\nflag = nohardcodedkeys\n",
            "[Command]\nname = \"x\"\ncommand = x\n\n"
            "[Command]\nname = \"y\"\ncommand = y\n\n"
            "[Statedef -1]\n"
            "[State -1, attack]\ntype = ChangeState\ntrigger1 = command = \"x\"\n"
            "trigger1 = ctrl\nvalue = 300\n"
            "[State -1, frozen]\ntype = ChangeState\ntrigger1 = command = \"y\"\n"
            "trigger1 = ctrl\nvalue = 400\n");

        RecordingEntities recorder;
        af::StateMachine machine;
        machine.setup(&gunner, 1);
        machine.setOpponent(&dummy);
        // The dummy is put to the right, because a fighter turns to face its opponent
        // between ticks and "forward" is whichever way it is looking: without this the
        // gunner faces left and every direction in this block reads backwards.
        dummy.setPosition(420.0f);
        dummy.setStageBounds(90.0f, 550.0f);
        machine.setPosition(200.0f);
        machine.setStageBounds(90.0f, 550.0f);
        machine.setEntitySink(&recorder);
        machine.setSide(0);

        af::InputState idleTick;
        for (int i = 0; i < 2; ++i) {
            machine.control(idleTick);
            machine.step();
        }
        af::InputState press;
        press.held = af::Button::kX;
        press.pressed = af::Button::kX;
        machine.control(press);
        machine.step();
        af::InputState held;
        for (int i = 0; i < 2; ++i) {
            machine.control(held);
            machine.step();
        }

        check(machine.stateNumber() == 300, "the gunner should be in its firing state");
        check(recorder.explods.size() == 2, "and should have asked for two explods");
        if (recorder.explods.size() == 2) {
            const af::Explod& aura = recorder.explods[0];
            check(aura.anim == 900, "the first is the aura's animation");
            check(aura.id == 77, "with the id it was given");
            check(aura.bindTime == -1, "bound to the fighter for ever");
            check(aura.trans.has_value() && aura.trans->blend == af::Blend::Additive,
                  "and drawn additively");
            check(std::fabs(aura.scaleX - 1.5f) < 0.01f, "at the scale it asked for");
            // `pos = 12, -30` is forward and *up*, and this engine keeps height above the
            // floor the other way up: the sign crosses once, as it does everywhere else
            // a character's y meets the engine's.
            check(aura.x > machine.x(), "placed in front of the fighter");
            check(aura.y > 0.0f, "and above it, not below");

            const af::Explod& spark = recorder.explods[1];
            check(spark.anim == 901, "the second is the spark's");
            check(spark.removeTime == 6, "which lasts the six ticks it was given");
            // A spark that wrote no `trans` must not have one invented for it: the
            // absence is what lets the animation's own `.air` code draw.
            check(!spark.trans.has_value(),
                  "and carries no trans, so its animation's own code draws it");
        }

        check(recorder.projectiles.size() == 1, "and should have fired one projectile");
        if (!recorder.projectiles.empty()) {
            const af::Projectile& beam = recorder.projectiles[0];
            check(beam.anim == 902 && beam.hitAnim == 903, "with its flight and hit animations");
            check(beam.hitsLeft == 3, "and the three hits it was allowed");
            check(beam.missTime == 8, "waiting eight ticks between them");
            check(!beam.removeOnHit,
                  "`projremove = 0` leaves it in the world after its hits are spent");
            check(beam.velX > 0.0f, "travelling forward");
            check(beam.y > machine.y(), "from a point above the fighter");
            // The reaction is the HitDef that was armed when it was fired: by the time a
            // beam arrives the fighter is in some other state with nothing armed at all.
            check(beam.hit.damage == 55, "carrying the damage of the HitDef that armed it");
            check(std::fabs(beam.hit.pushX + 6.0f) < 0.01f, "and its velocity");
        }
        check(machine.ignoringHits(), "a `NotHitBy` makes the fighter un-hittable");
        check(recorder.countExplods(&gunner, nullptr, 77) == 1, "and the explod count finds it by id");
        check(recorder.countExplods(&gunner, nullptr, -1) == 2, "and by any id");

        // The engine's own movement stands down when a character says so. The fighter is
        // put into the state that asserts it first, and *then* told to walk: the flag is
        // raised by a controller in that state, so entering it is what turns the engine's
        // movement off.
        af::StateMachine frozen;
        frozen.setup(&gunner, 1);
        frozen.setOpponent(&dummy);
        dummy.setPosition(420.0f);
        frozen.setPosition(200.0f);
        frozen.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 2; ++i) {
            frozen.control(idleTick);
            frozen.step();
        }
        af::InputState enter;
        enter.held = af::Button::kY;
        enter.pressed = af::Button::kY;
        frozen.control(enter);
        frozen.step();
        check(frozen.stateNumber() == 400, "the frozen state should be entered by its command");

        af::InputState forward;
        forward.held = af::Button::kRight;
        const float beforeX = frozen.x();
        for (int i = 0; i < 20; ++i) {
            frozen.control(forward);
            frozen.step();
        }
        check(std::fabs(frozen.x() - beforeX) < 0.5f,
              "`nohardcodedkeys` should stop the engine walking the fighter");

        // And the same state without the flag walks, which is what makes the assertion
        // above about the flag rather than about the fighter being unable to move.
        af::StateMachine walking;
        walking.setup(&gunner, 1);
        walking.setOpponent(&dummy);
        dummy.setPosition(420.0f);
        walking.setPosition(200.0f);
        walking.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 2; ++i) {
            walking.control(idleTick);
            walking.step();
        }
        const float walkBefore = walking.x();
        for (int i = 0; i < 20; ++i) {
            walking.control(forward);
            walking.step();
        }
        check(std::fabs(walking.x() - walkBefore) > 1.0f,
              "a fighter without that flag does walk when a direction is held");
    }

    // -------------------------------------------------------- `projremanim`
    //
    // A projectile that goes does not always simply cease to exist. `projremanim`
    // names an animation for the occasion -- the reference's examples are hitting a
    // wall and leaving the screen -- and the fallback when it is not written is
    // `projhitanim`, which is the pose the blast already strikes in. Reading a flat -1
    // there instead made every blast blink out of the world instead of playing the
    // animation its author had given it, and the corpus writes `projhitanim` far more
    // often than `projremanim`: eleven characters name a removal animation, four of
    // the six in this roster name only a hit one.
    //
    // The parse first, four projectiles in one state, one an element.
    {
        af::Character leaver;
        leaver.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 500\n\n"
            "[Statedef 500]\ntype = S\nmovetype = A\nphysics = S\nctrl = 0\n\n"
            // Only a hit animation: the removal animation falls back on it.
            "[State 500, a]\ntype = Projectile\ntrigger1 = time = 0\nprojanim = 900\n"
            "projhitanim = 903\n\n"
            // Neither of the two: nothing to play, and the flat -1 stands.
            "[State 500, b]\ntype = Projectile\ntrigger1 = time = 1\nprojanim = 900\n\n"
            // Both: the removal animation is the one that wins.
            "[State 500, c]\ntype = Projectile\ntrigger1 = time = 2\nprojanim = 900\n"
            "projhitanim = 903\nprojremanim = 904\n\n"
            // The removal animation written as -1 says so on purpose, and it means the
            // blast vanishes rather than falling back on the hit animation it names.
            // Four fifths of the corpus' written removal animations are exactly this.
            "[State 500, d]\ntype = Projectile\ntrigger1 = time = 3\nprojanim = 900\n"
            "projhitanim = 903\nprojremanim = -1\n\n"
            // And now the one-letter prefix that says which *file* each of the three
            // animations is in: bare is the character's own .air, `F` is the
            // screenpack's `fightfx.air`. Left unread, `projanim = F60` reaches the
            // expression parser as an unknown identifier, comes back 0, and draws the
            // fighter's stand sprite as its own fireball.
            "[State 500, e]\ntype = Projectile\ntrigger1 = time = 4\nprojanim = F60\n\n"
            "[State 500, f]\ntype = Projectile\ntrigger1 = time = 5\nprojanim = F60\n"
            "projhitanim = 903\n\n"
            "[State 500, g]\ntype = Projectile\ntrigger1 = time = 6\nprojanim = 60\n"
            "projhitanim = F903\nprojremanim = F904\n\n"
            // The fallback has to carry the file with it: naming only a prefixed hit
            // animation means the removal animation is that same fightfx one, not
            // animation 903 of the character's own file.
            "[State 500, h]\ntype = Projectile\ntrigger1 = time = 7\nprojanim = F60\n"
            "projhitanim = F903\n"
            // The collision-only parameters: no hits still exists and still cancels;
            // priority is raw (negative included); `remvelocity` crosses the y axis
            // like `velocity`; and `projcancelanim` is kept separately from
            // `projremanim` so the state-3 fallback chain can be resolved later.
            "[State 500, i]\ntype = Projectile\ntrigger1 = time = 8\nprojanim = 900\n"
            "projhits = 0\nprojpriority = -2\nremvelocity = 3, -2\n"
            "projcancelanim = 905\n",
            "[Statedef -1]\n");

        RecordingEntities leavingRecorder;
        af::StateMachine leaverMachine;
        leaverMachine.setup(&leaver, 1);
        leaverMachine.setEntitySink(&leavingRecorder);
        for (int i = 0; i < 12; ++i) {
            af::InputState tick;
            leaverMachine.control(tick);
            leaverMachine.step();
        }

        check(leavingRecorder.projectiles.size() == 9,
              "nine firing projectiles should have been recorded");
        if (leavingRecorder.projectiles.size() == 9) {
            check(leavingRecorder.projectiles[0].removeAnim == 903,
                  "with no `projremanim` a blast plays its hit animation on the way out");
            check(leavingRecorder.projectiles[0].hitAnim == 903,
                  "which is still the hit animation it was also given");
            check(leavingRecorder.projectiles[1].removeAnim == -1,
                  "a blast that names neither has nothing to play");
            check(leavingRecorder.projectiles[2].removeAnim == 904,
                  "and one that names `projremanim` uses it");
            check(leavingRecorder.projectiles[3].removeAnim == -1,
                  "while an explicit -1 means none, not the hit animation");

            const af::Projectile& plainFly = leavingRecorder.projectiles[4];
            check(plainFly.anim == 60 && plainFly.animFromFightFx,
                  "`projanim = F60` should be fightfx's 60 and not the character's");
            check(!plainFly.hitAnimFromFightFx && !plainFly.removeAnimFromFightFx,
                  "with the other two animations, which name no file of their own, on the "
                  "character's");

            const af::Projectile& mixed = leavingRecorder.projectiles[5];
            check(mixed.animFromFightFx && mixed.anim == 60,
                  "a prefixed flight animation is fightfx's");
            check(!mixed.hitAnimFromFightFx && mixed.hitAnim == 903,
                  "while a bare hit animation beside it is still the character's own");
            check(!mixed.removeAnimFromFightFx && mixed.removeAnim == 903,
                  "and the removal animation that falls back on it follows it");

            const af::Projectile& allBare = leavingRecorder.projectiles[6];
            check(!allBare.animFromFightFx && allBare.anim == 60,
                  "a bare flight animation is the character's own");
            check(allBare.hitAnimFromFightFx && allBare.hitAnim == 903,
                  "and a prefixed hit animation is fightfx's");
            check(allBare.removeAnimFromFightFx && allBare.removeAnim == 904,
                  "as is the removal animation written after it");

            const af::Projectile& inherited = leavingRecorder.projectiles[7];
            check(inherited.removeAnim == 903 && inherited.removeAnimFromFightFx,
                  "and a removal animation that only falls back on `projhitanim` "
                  "inherits its file as well as its number");

            const af::Projectile& duel = leavingRecorder.projectiles[8];
            check(duel.hitsLeft == 0,
                  "`projhits = 0` should survive parsing rather than be raised to one");
            check(duel.priority == -2,
                  "`projpriority` should keep its raw value, negative included");
            check(std::fabs(duel.remVelX - 3.0f) < 0.01f,
                  "`remvelocity` x should reach the removal velocity");
            check(std::fabs(duel.remVelY - 2.0f) < 0.01f,
                  "and its y should cross into height above the floor");
            check(duel.cancelAnim == 905,
                  "`projcancelanim` should be kept for the state-3 path");
            check(!duel.cancelAnimFromFightFx,
                  "with a bare number left in the character's own file");
            check(leavingRecorder.projectiles[0].priority == 1,
                  "and the default priority is one");
        }
    }

    // And now the other half: what happens when a projectile is actually told to go.
    // This is the one place in the suite that drives a real `af::EntityLayer`, because
    // the removal is the layer's business -- a state machine only asks for the
    // projectile, and what it does afterwards is not a controller's to decide.
    {
        af::Character shooter;
        // Two actions, both written the way characters really write them. 900 is a
        // one-frame loop, which is what a removal animation usually is: under the
        // rule the layer asks of an animation -- "loops for ever, so never finished"
        // -- it would never end. The corpus' own removal animations are exactly this
        // shape: `dbz_piccolo`'s 3006 is eight one-tick frames with no `LoopStart`
        // and `dbz_gohan`'s 1016 is a single two-tick frame.
        af::parseAirText(
            "[Begin Action 900]\n0,0, 0,0, 1\n"
            "[Begin Action 901]\n0,1, 0,0, 2\n0,2, 0,0, 3\n",
            shooter.animations());

        af::EntityLayer layer;
        af::Projectile beam;
        beam.owner = &shooter;
        beam.anim = 900;
        beam.removeAnim = 901;
        beam.removeTime = 3;  // it runs out of time rather than hitting anything
        beam.velX = 4.0f;
        layer.spawnProjectile(beam);

        for (int i = 0; i < 3; ++i) layer.update(nullptr, nullptr);
        check(layer.projectiles().size() == 1,
              "a projectile with three ticks to live should still be here after three");
        float stoppedX = 0.0f;
        if (!layer.projectiles().empty()) {
            const af::Projectile& going = layer.projectiles()[0];
            check(going.isRemoving() && going.state == af::ProjectileState::Removal,
                  "and its time running out should start it on its way out");
            check(going.anim == 901,
                  "having switched from its flight animation to its removal animation");
            check(going.owner != nullptr, "still owned by the fighter that fired it");
            stoppedX = going.x;
            check(std::fabs(going.x - 16.0f) < 0.01f,
                  "having taken three flight steps and the entry removal step");
        }
        // The reference's `NumProj` (0x4733E0) counts only projectiles whose state
        // is 1; a projectile that has started its removal animation is no longer
        // live, so it does not count even though it is still on screen. See
        // `docs/mugen-re/PROJECTILE-PARAMS-RE.md` §4.9.
        check(layer.countProjectiles(&shooter, nullptr, -1) == 0,
              "and a projectile on its way out no longer counts towards `numproj`");

        // The removal animation is five ticks long -- two and then three -- and the
        // tick that entered state 2 is its first, because §5.3 falls straight through
        // into the removal handler. The projectile must be here for the remaining
        // four and gone on the last of them. That entry tick took one more flight
        // step (§5.3); every later one uses `remvelocity`, which this beam left at
        // zero.
        int heldFor = 0;
        float driftX = 0.0f;
        for (int i = 0; i < 5 && !layer.projectiles().empty(); ++i) {
            layer.update(nullptr, nullptr);
            if (!layer.projectiles().empty()) {
                ++heldFor;
                driftX = std::fabs(layer.projectiles()[0].x - stoppedX);
            }
        }
        check(heldFor == 3, "the removal animation's remaining four ticks hold it");
        check(std::fabs(driftX) < 0.01f,
              "the entry tick already took the last flight step, then it stops");
        check(layer.projectiles().empty(), "and then let it go");

        // `remvelocity` from the second removal tick on (§5.3).
        af::EntityLayer sliding;
        af::Projectile slide;
        slide.owner = &shooter;
        slide.anim = 900;
        slide.removeAnim = 901;
        slide.removeTime = 1;
        slide.velX = 4.0f;
        slide.remVelX = 10.0f;
        sliding.spawnProjectile(slide);
        sliding.update(nullptr, nullptr);  // flies once and falls into removal at x = 8
        check(!sliding.projectiles().empty() &&
                  std::fabs(sliding.projectiles()[0].x - 8.0f) < 0.01f,
              "the entry removal tick uses flight velocity");
        sliding.update(nullptr, nullptr);  // next removal tick: remvelocity
        check(!sliding.projectiles().empty() &&
                  std::fabs(sliding.projectiles()[0].x - 18.0f) < 0.01f,
              "the next removal tick uses remvelocity");

        // Q5 / §5.2: the flight animation ending is not a removal condition at all.
        // `projremovetime = -1` means never, and the projectile stays on its last
        // frame until a bound, the edge or a hit removes it.
        af::EntityLayer persistent;
        af::Projectile endless;
        endless.owner = &shooter;
        endless.anim = 901;
        endless.removeTime = -1;
        persistent.spawnProjectile(endless);
        for (int i = 0; i < 10; ++i) persistent.update(nullptr, nullptr);
        check(persistent.projectiles().size() == 1,
              "a flight animation ending must not remove a projremovetime = -1 projectile");

        // The same for a removal animation that loops. This is the assertion that
        // pins the rule: waiting for such an animation to *finish* would leave the
        // projectile on screen for ever, drawing and still counted.
        af::EntityLayer looped;
        af::Projectile aura;
        aura.owner = &shooter;
        aura.anim = 900;
        aura.removeAnim = 900;
        aura.removeTime = 1;
        looped.spawnProjectile(aura);
        int loopedTicks = 0;
        while (!looped.projectiles().empty() && loopedTicks < 20) {
            looped.update(nullptr, nullptr);
            ++loopedTicks;
        }
        check(looped.projectiles().empty(),
              "a removal animation that loops is played once and does not keep the blast alive");
        check(loopedTicks <= 3,
              "and it is played from its own first tick, so a one-tick one costs a tick");

        // And the ordinary case, where the author named no removal animation at all.
        af::EntityLayer plain;
        af::Projectile simple;
        simple.owner = &shooter;
        simple.anim = 900;
        simple.removeTime = 1;
        plain.spawnProjectile(simple);
        plain.update(nullptr, nullptr);
        check(plain.projectiles().empty(),
              "a projectile with no removal animation goes the tick it is told to");

        // Who is counting matters. The reference keys projectiles and explods by
        // the player struct that spawned them, and every helper has its own slot
        // array (`PROJECTILE-PARAMS-RE.md` §1), so two machines may share one
        // `Character*` and still owe separate counts. A helper's blast must not
        // make its owner read two, and a mirror-match opponent must not either.
        af::StateMachine self;
        af::StateMachine helper;
        af::EntityLayer split;
        af::Projectile mineBolt;
        mineBolt.owner = &shooter;
        mineBolt.ownerMachine = &self;
        mineBolt.anim = 900;
        mineBolt.removeTime = 100;
        split.spawnProjectile(mineBolt);
        af::Projectile helperBolt = mineBolt;
        helperBolt.ownerMachine = &helper;
        split.spawnProjectile(helperBolt);
        check(split.countProjectiles(&shooter, &self, -1) == 1,
              "two machines sharing a character each count only their own projectile");
        check(split.countProjectiles(&shooter, &helper, -1) == 1,
              "and the helper's count is its own, not its owner's");
        check(split.countProjectiles(&shooter, nullptr, -1) == 2,
              "while the character-wide reading still sees both, for callers with no "
              "state owner to name");

        af::Explod spark;
        spark.owner = &shooter;
        spark.ownerMachine = &self;
        spark.anim = 900;
        split.spawnExplod(spark);
        af::Explod helperSpark = spark;
        helperSpark.ownerMachine = &helper;
        split.spawnExplod(helperSpark);
        check(split.countExplods(&shooter, &self, -1) == 1,
              "and the same split holds for explods");
        check(split.countExplods(&shooter, &helper, -1) == 1,
              "one each, from the machine that spawned it");
        check(split.countExplods(&shooter, nullptr, -1) == 2,
              "with the old character-wide count kept as the fallback");

        // The other file. A removal animation written with the `F` prefix lives in the
        // screenpack, and the layer has to look there rather than in the character's
        // own .air -- which is why the two files are given *different* animation 900s
        // here, one one tick long and one two, so the length of the removal tells which
        // of them was read.
        af::AnimationSet fightFx;
        af::parseAirText("[Begin Action 900]\n0,5, 0,0, 1\n0,6, 0,0, 1\n", fightFx);
        af::EntityLayer prefixed;
        prefixed.setFightFx(nullptr, &fightFx);
        af::Projectile packaged;
        packaged.owner = &shooter;  // whose own 900 is a single one-tick frame
        packaged.anim = 900;
        packaged.removeAnim = 900;
        packaged.removeAnimFromFightFx = true;
        packaged.removeTime = 1;
        prefixed.spawnProjectile(packaged);
        int prefixedTicks = 0;
        while (!prefixed.projectiles().empty() && prefixedTicks < 20) {
            prefixed.update(nullptr, nullptr);
            ++prefixedTicks;
        }
        check(prefixed.projectiles().empty(),
              "a `projremanim` from the screenpack should still play and end");
        check(prefixedTicks == 2,
              "taking fightfx's two ticks (the entry tick included) rather than the "
              "character's one, which is what says the right file was read");
    }

    // ---------------------------------------- what a freeze does to an explod
    //
    // An explod answers two different freezes with two different parameters, and the
    // layer is the only thing that can tell them apart: the state machine that made
    // the explod is not consulted again once the spark exists. So the rules are
    // asserted here, on a real layer, with a real fighter's hitpause on one side and
    // the layer's own pause clock on the other.
    {
        af::Character sparker;
        // One frame that holds for ever, so an explod using it is never removed for
        // reaching its looptime -- the only way its clock can be read below is by
        // advancing, which is what every assertion about freezing measures. (`-1` on
        // an element is air.html's "looptime infinite", which is one of the two ways
        // an animation never reaches `AnimTime = 0`.)
        af::parseAirText("[Begin Action 900]\n0,0, 0,0, -1\n", sparker.animations());

        // An actor for the pauses to name. Nothing reads it here -- the explod's
        // own window is what the layer consults -- but a Pause is always made by
        // somebody and the clock is the same either way.
        af::StateMachine pauser;

        const auto makeExplod = [&sparker](const char* why) {
            af::Explod explod;
            explod.owner = &sparker;
            explod.anim = 900;
            (void)why;
            return explod;
        };

        // The defaults have to be read off the *controller*, not off the struct: what
        // matters is what a character that names nothing gets, and a struct default
        // would pass whatever this file happened to write. So one real machine runs a
        // real `Explod` and the sink's copy is what gets read.
        const auto runExplod = [&](const std::string& parameters) {
            af::Character parser;
            parser.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = 1\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 300, spark]\ntype = Explod\ntrigger1 = time = 0\nanim = 900\n" +
                    parameters + "\n",
                "[Statedef -1]\n");

            RecordingEntities recorder;
            af::StateMachine parserMachine;
            parserMachine.setup(&parser, 1);
            parserMachine.setOpponent(&dummy);
            parserMachine.setPosition(320.0f);
            parserMachine.setStageBounds(90.0f, 550.0f);
            parserMachine.setEntitySink(&recorder);
            for (int i = 0; i < 30; ++i) {
                af::InputState tick;
                parserMachine.control(tick);
                parserMachine.step();
            }
            af::Explod out;
            if (!recorder.explods.empty()) out = recorder.explods.front();
            return out;
        };

        // A fresh explod animates through a hitpause unless it said otherwise. This
        // is sctrls.html's default of 1, and it is the difference between a hit
        // spark that plays over the freeze it marks and one that stands still for
        // the whole of it. Note that the controller-level `ignorehitpause` -- the
        // generic one every controller has, defaulting to 0 -- shares the spelling,
        // and reading this parameter with *that* default is the bug this pins.
        {
            const af::Explod plain = runExplod("");
            check(plain.ignoreHitPause && plain.pauseMoveTime == 0 && plain.superMoveTime == 0,
                  "an explod that names no freeze parameters animates through a hitpause "
                  "-- `ignorehitpause` defaults to 1 on an explod -- and asks for no ticks "
                  "of a Pause or of a SuperPause");

            const af::Explod named = runExplod(
                "ignorehitpause = 0\npausemovetime = 4\nsupermovetime = 7");
            check(!named.ignoreHitPause && named.pauseMoveTime == 4 && named.superMoveTime == 7,
                  "and one that names all three gets all three, each on its own parameter");

            // The deprecated `supermove` is deliberately not read here: the reference
            // describes it as "persist until the end of a super pause, regardless of
            // the value of removetime" while pointing at `supermovetime`, which is
            // purely about animation. Which of the two it means is not settled by any
            // source on this machine, so it is left alone rather than guessed at.
            const af::Explod deprecated = runExplod("supermove = 1");
            check(deprecated.superMoveTime == 0,
                  "and the deprecated `supermove` is read as nothing rather than as "
                  "`supermovetime`, whose meaning the reference does not settle");
        }

        // `removetime` is three numbers with three meanings, and the two negatives
        // are not the same one: a positive value is a lifetime, -1 is *displayed
        // indefinitely*, and -2 -- the documented default -- is "removed when its
        // animtime reaches 0". Reading the default as -1 collapsed the two, so an
        // explod that wrote `removetime = -1` for an aura meant to outlive its
        // animation died at the end of it instead. 392 explods in the corpus write
        // that -1.
        {
            check(runExplod("").removeTime == -2,
                  "an explod with no `removetime` takes the documented default of -2, "
                  "which is `remove when the animation has played out`");
            check(runExplod("removetime = -1").removeTime == -1,
                  "and one that writes -1 keeps it, which is `displayed indefinitely`");
            check(runExplod("removetime = 40").removeTime == 40,
                  "while a positive value stays the lifetime it is");
        }

        // The three of them, on one layer, against an animation that really does
        // finish -- which is the only way the -2 rule can be told from the other two.
        af::parseAirText("[Begin Action 901]\n0,0, 0,0, 1\n0,1, 0,0, 1\n", sparker.animations());
        const auto withAnim = [&](int anim) {
            af::Explod explod = makeExplod("");
            explod.anim = anim;
            return explod;
        };
        {
            af::EntityLayer lifetimes;
            lifetimes.spawnExplod(withAnim(901));  // default: -2, so the animation's own end
            af::Explod forever = withAnim(901);
            forever.removeTime = -1;
            lifetimes.spawnExplod(forever);
            af::Explod timed = withAnim(901);
            timed.removeTime = 40;
            lifetimes.spawnExplod(timed);
            for (int i = 0; i < 3; ++i) lifetimes.update(nullptr, nullptr);
            check(lifetimes.explods().size() == 2,
                  "a two-tick animation takes the explod that named no `removetime` with it "
                  "when it finishes one pass, and leaves the other two on screen");
            check(lifetimes.explods()[0].removeTime == -1 && lifetimes.explods()[0].clock == 3,
                  "the one that asked to be displayed indefinitely is untouched and still "
                  "counting");
            check(lifetimes.explods()[1].removeTime == 37,
                  "and the one on a lifetime has spent three of its forty ticks");
            for (int i = 0; i < 40; ++i) lifetimes.update(nullptr, nullptr);
            check(lifetimes.explods().size() == 1 &&
                      lifetimes.explods()[0].removeTime == -1,
                  "so a lifetime explod does go when its ticks are spent, and only the one "
                  "that asked for ever is left");
        }

        af::StateMachine hitter;
        af::EntityLayer hitPaused;
        hitPaused.spawnExplod(makeExplod("default"));
        af::Explod optsOut = makeExplod("optout");
        optsOut.ignoreHitPause = false;
        hitPaused.spawnExplod(optsOut);
        hitter.setPause(5);
        for (int i = 0; i < 5; ++i) hitPaused.update(&hitter, nullptr);
        check(hitPaused.explods().size() == 2 &&
                  hitPaused.explods()[0].clock == 5 && hitPaused.explods()[1].clock == 0,
              "so a hit spark keeps playing through a five-tick hitpause, while the "
              "one that set `ignorehitpause = 0` holds still for all five");

        // It is the *owner's* hitpause that counts, not the other fighter's: a spark
        // belongs to whoever made it, and a hit that stopped someone else is not the
        // spark's business.
        af::StateMachine bystander;
        af::EntityLayer otherSide;
        af::Explod ownedByP1 = makeExplod("owner");
        ownedByP1.ignoreHitPause = false;
        otherSide.spawnExplod(ownedByP1);
        otherSide.update(&bystander, &hitter);  // hitter is the p2 argument
        check(otherSide.explods()[0].clock == 1,
              "and an explod owned by the side that is *not* paused keeps animating");

        // A running Pause is the other freeze. Nothing about it is covered by
        // `ignorehitpause`: the explod is held unless it asked for a number of the
        // pause's ticks by name, and asking for none -- the default -- asks for none.
        af::EntityLayer pausing;
        pausing.spawnExplod(makeExplod("default"));
        af::Explod early = makeExplod("early");
        early.pauseMoveTime = 2;
        pausing.spawnExplod(early);
        pausing.beginPause(5, &pauser, 0);
        for (int i = 0; i < 5; ++i) pausing.update(nullptr, nullptr);
        check(pausing.explods()[0].clock == 0,
              "a Pause holds an explod that asked for no ticks of it");
        check(pausing.explods()[1].clock == 2,
              "and lets one through for exactly the ticks it asked for, then holds it");

        // The two windows are separate parameters, and this is the assertion that
        // says so: an explod that named a `pausemovetime` gets nothing from a
        // SuperPause, because the reference documents one window per freeze.
        af::EntityLayer supering;
        af::Explod pauseOnly = makeExplod("pauseonly");
        pauseOnly.pauseMoveTime = 2;
        supering.spawnExplod(pauseOnly);
        af::Explod superAware = makeExplod("superaware");
        superAware.superMoveTime = 3;
        supering.spawnExplod(superAware);
        supering.beginSuperPause(5, &pauser, 0, false, 1.0f);
        for (int i = 0; i < 5; ++i) supering.update(nullptr, nullptr);
        check(supering.explods()[0].clock == 0,
              "a `pausemovetime` buys nothing during a SuperPause");
        check(supering.explods()[1].clock == 3,
              "while a `supermovetime` buys exactly its own ticks of the flash");

        // A window wider than the pause is the ordinary way a charge aura is written:
        // it runs the whole freeze and carries on afterwards.
        af::EntityLayer throughIt;
        af::Explod aura = makeExplod("aura");
        aura.superMoveTime = 99;
        throughIt.spawnExplod(aura);
        throughIt.beginSuperPause(3, &pauser, 0, false, 1.0f);
        for (int i = 0; i < 3; ++i) throughIt.update(nullptr, nullptr);
        check(throughIt.explods()[0].clock == 3,
              "an explod that asked for more ticks than the pause has runs through it");
        throughIt.update(nullptr, nullptr);
        check(throughIt.explods()[0].clock == 4,
              "and goes on animating once the flash is over");
    }

    // ---------------------- where an explod sits, and which layer it sits on
    //
    // Four parameters of the Explod set that had no reader at all. Three of them
    // are placement -- `vfacing` turns the effect over vertically, `random`
    // displaces its bind point when it is created, and `removeongethit` gives it
    // a second, earlier death -- and the fourth, `ontop`, is not a placement but
    // a *layer*: the reference gives it "precedence over sprpriority" in as many
    // words, so it cannot be a high priority, because a priority orders one
    // sprite against another within a layer and this one asks for a different
    // layer. That is why the assertions below are split the way they are: the
    // three placement parameters are read off the spawned struct, and `ontop` has
    // to be read off a real `EntityLayer`'s three draw passes, because what it
    // means is which pass draws it.
    //
    // All four are common in the corpus -- counted over the 363 character files in
    // local_chars, `ontop` is written 4446 times (3469 of them `ontop = 1`),
    // `removeongethit` 3814 (3064 of them `= 1`), `vfacing` 2609 (54 literal `-1`,
    // plus about as many again written as an `ifelse` that picks -1 half the time),
    // and `random` 463 (349 of them `0,0`, a deliberate no-op) -- so these are not
    // exotic corners being tidied up.
    {
        // A *fresh* character for every parse run, and this is not tidiness.
        // `Character::loadBehaviour` **merges** rather than replaces -- the .cns and
        // the .cmd are deliberately parsed into one state library, so a state the
        // second document also defines keeps its header and only gains controllers
        // -- and a state 300 loaded five times therefore holds five copies of the
        // `[State 300, spark]` controller. All five fire on the same tick and the
        // sink hands back the *first* one, which is the one from the first run. An
        // earlier version of this fixture reused one character and every parameter
        // assertion below was silently reading an explod that had been given no
        // parameters at all -- so the characters are kept alive here, one per run,
        // rather than a single one being reloaded.
        std::vector<std::unique_ptr<af::Character>> exploders;

        // Spawn a real `Explod` controller in a real machine and hand back what the
        // sink was given. The trigger is a parameter because the `random` case needs
        // *many* explods out of one machine -- the displacement is drawn per spawn,
        // so a single one says nothing about the range it comes from.
        const auto explodsFrom = [&exploders, &dummy](const std::string& trigger,
                                                     const std::string& parameters) {
            exploders.push_back(std::make_unique<af::Character>());
            af::Character& exploder = *exploders.back();
            exploder.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = 1\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 300, spark]\ntype = Explod\ntrigger1 = " + trigger +
                    "\nanim = 900\n" + parameters + "\n",
                "[Statedef -1]\n");

            RecordingEntities recorder;
            af::StateMachine machine;
            machine.setup(&exploder, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(320.0f);
            machine.setStageBounds(90.0f, 550.0f);
            machine.setEntitySink(&recorder);
            for (int i = 0; i < 40; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
            return recorder.explods;
        };
        const auto explodFrom = [&explodsFrom](const std::string& parameters) {
            const std::vector<af::Explod> all = explodsFrom("time = 0", parameters);
            return all.empty() ? af::Explod{} : all.front();
        };

        // The fixture's own guard, because everything below reads `front()` of a run
        // and a run that produced nothing would hand back a default-constructed
        // explod -- which fails every positive assertion but *passes* every negative
        // one, so the failure would look like the parameters not being read. It has
        // happened once already (see the note on `exploders` above).
        check(explodsFrom("time = 0", "").size() == 1,
              "the fixture spawns exactly one explod for a `time = 0` trigger on a "
              "fresh character, which is what makes reading its front the right read");

        // The defaults first, read off the controller rather than off the struct for
        // the reason the freeze block gives: a struct default would pass whatever
        // this test file happened to write.
        {
            const af::Explod plain = explodFrom("");
            check(!plain.onTop && !plain.removeOnGetHit && !plain.flipV,
                  "an explod that names none of `ontop`, `removeongethit` or `vfacing` "
                  "gets none of them -- three separate defaults, all off");
            check(plain.bindOffsetX == 0.0f && plain.bindOffsetY == 0.0f,
                  "and a `random` it does not name displaces its bind point nowhere");
        }

        // `vfacing` is the vertical mirror, and it is independent of `facing`: an
        // effect can be turned over without being turned around, which is what a
        // character uses it for when the same art serves a sweep in both directions.
        {
            const af::Explod flipped = explodFrom("vfacing = -1");
            check(flipped.flipV, "`vfacing = -1` turns an explod over vertically");
            check(flipped.facing == 1,
                  "and leaves `facing` alone, which is the whole point of a second "
                  "mirror: the two are separate choices");
        }

        // `ontop`, and the ordering rule it overrides. The reference says the
        // parameter "has precedence over sprpriority", so the case that pins it is
        // an explod whose priority is *below* its owner's -- which would otherwise
        // make it a behind-the-owner aura -- and which must not be one.
        {
            const af::Explod aura = explodFrom("sprpriority = -10");
            check(aura.behindOwner,
                  "an explod whose `sprpriority` is under its owner's is drawn behind "
                  "the owner -- the aura case, and the control for the next assertion");

            const af::Explod overTheTop = explodFrom("sprpriority = -10\nontop = 1");
            check(overTheTop.onTop, "`ontop = 1` puts an explod on the top layer");
            check(!overTheTop.behindOwner,
                  "and that outranks the priority comparison outright: the same "
                  "`sprpriority = -10` that made an aura of the explod above does not "
                  "make one of this one, which is what 'precedence over sprpriority' "
                  "means");
        }

        // `removeongethit`: a second and earlier way for an explod to die, beside
        // its own clock.
        {
            const af::Explod plain = explodFrom("");
            const af::Explod fragile = explodFrom("removeongethit = 1");
            check(!plain.removeOnGetHit && fragile.removeOnGetHit,
                  "`removeongethit = 1` is what arms an explod to go when its owner is "
                  "hit, and it is off by default");
        }

        // `random`, and this one needs the many-explods form. The reference's worked
        // example is the specification: "if pos = 0,0 and random = 40,80, then the
        // explod's x location will be a random number between -20 and 19, and its y
        // location will be a random number between -40 and 39" -- so the low end is
        // half the range below zero, and the range is the span. One spawn would only
        // show that *a* number came out; forty show the span and the displacement
        // together.
        {
            const std::vector<af::Explod> many = explodsFrom("time >= 0", "random = 40, 80");
            check(many.size() >= 30,
                  "a `trigger1 = time >= 0` explod with no `removetime` on a held "
                  "animation really does spawn once a tick, which is what makes the "
                  "spread below visible at all");

            bool inRange = true;
            bool movedX = false;
            bool movedY = false;
            int minX = 0;
            int maxX = 0;
            int minY = 0;
            int maxY = 0;
            for (const af::Explod& explod : many) {
                const int x = static_cast<int>(explod.bindOffsetX);
                const int y = static_cast<int>(explod.bindOffsetY);
                if (x < -20 || x > 19 || y < -40 || y > 39) inRange = false;
                if (explod.bindOffsetX != 0.0f) movedX = true;
                if (explod.bindOffsetY != 0.0f) movedY = true;
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
            check(inRange,
                  "every one of them lands inside the reference's own window -- x in "
                  "[-20,19] and y in [-40,39], which is `-(random / 2)` to "
                  "`random / 2 - 1` and not a symmetric range about zero");
            // The two halves are asserted separately, and the reason is that they are
            // *two* draws off one stream: a version of this that displaced x and left
            // y on the spot reads every other assertion here as a pass, because zero
            // is inside both windows and a spread over x is all the x checks ask for.
            check(movedX,
                  "and the x half of the bind point really is displaced: a `random` "
                  "that read as nothing would put every one of them on the same spot");
            check(movedY,
                  "as is the y half, which is the one a pair that was only half read "
                  "would leave at zero -- and zero is inside the window, so nothing "
                  "else here would notice");
            check(maxX - minX >= 10,
                  "across forty draws the x offsets spread over a real part of the "
                  "window rather than clustering, which is what a per-spawn draw from "
                  "the machine's own stream gives");
            check(maxY - minY >= 20,
                  "and the y offsets spread the same way over their own, twice as wide "
                  "window");

            const std::vector<af::Explod> none = explodsFrom("time = 0", "random = 0, 0");
            check(!none.empty() && none.front().bindOffsetX == 0.0f &&
                      none.front().bindOffsetY == 0.0f,
                  "while `random = 0,0` -- which is what a character writes to mean "
                  "'no random' -- displaces the bind point nowhere");
        }

        // And now `ontop` on a real layer, because the assertions above read a flag
        // and the flag's whole meaning is which of the three draw passes picks the
        // explod up. The layer has no display here, so the shim's `drawSpriteAt`
        // writes each call down (see draw_recorder.h) and the *pass* is identified
        // by the x the explod was spawned at: 10 is the one behind its owner, 20 the
        // ordinary one, 30 the one that asked for the top.
        {
            af::Character layerChar;
            af::parseAirText("[Begin Action 900]\n0,0, 0,0, -1\n", layerChar.animations());

            const auto placed = [&layerChar](float x) {
                af::Explod explod;
                explod.owner = &layerChar;
                explod.ownerSide = 0;
                explod.anim = 900;
                explod.x = x;
                return explod;
            };

            af::EntityLayer layer;
            af::Explod behind = placed(10.0f);
            behind.behindOwner = true;
            layer.spawnExplod(behind);
            layer.spawnExplod(placed(20.0f));
            af::Explod onTop = placed(30.0f);
            onTop.onTop = true;
            layer.spawnExplod(onTop);

            af::Renderer renderer;
            af::draw_stub::clear();
            af::draw_stub::record(true);

            layer.drawBehind(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().size() == 1 &&
                      af::draw_stub::recorded()[0].x == 10.0f,
                  "drawBehind draws the explod that belongs behind its owner and "
                  "neither of the other two");

            // The one combination a character cannot actually write, asserted anyway
            // because the rule belongs to the layer and not to the parser. `doExplod`
            // clears `behindOwner` for an `ontop` explod, so no controller produces
            // both -- which means the `onTop` test inside drawBehind is unreachable
            // through the parse and would be silently deletable if this did not hold
            // it to its own contract. The struct is public; the layer's rule is that
            // an `ontop` explod is drawn by `drawTop` and by nothing else.
            af::EntityLayer contradictory;
            af::Explod both = placed(50.0f);
            both.behindOwner = true;
            both.onTop = true;
            contradictory.spawnExplod(both);
            af::draw_stub::clear();
            contradictory.drawBehind(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().empty(),
                  "and an explod that somehow asked for both the behind-the-owner "
                  "position and the top layer is still drawn by neither drawBehind "
                  "nor -- below -- by anything but drawTop, because `ontop` outranks "
                  "the priority comparison wherever the comparison is read");

            af::draw_stub::clear();
            contradictory.drawFront(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().empty(),
                  "so it is not drawn by drawFront either");

            af::draw_stub::clear();
            contradictory.drawTop(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().size() == 1 &&
                      af::draw_stub::recorded()[0].x == 50.0f,
                  "and drawTop takes it, once");

            af::draw_stub::clear();
            layer.drawFront(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().size() == 1 &&
                      af::draw_stub::recorded()[0].x == 20.0f,
                  "drawFront draws the ordinary explod, and skips both the one behind "
                  "the owner and the one that asked for the top layer -- so an `ontop` "
                  "explod is not drawn twice");

            af::draw_stub::clear();
            layer.drawTop(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().size() == 1 &&
                      af::draw_stub::recorded()[0].x == 30.0f,
                  "and drawTop draws the `ontop` explod and only that one, which is "
                  "what makes the parameter a layer rather than a priority");

            // The vertical mirror reaches the draw call as its own argument, beside
            // the horizontal one rather than folded into it. (How the two corner
            // offsets are then worked out is in `drawSpriteAt` in motif.cpp, which is
            // not linked into this test -- recorded as a gap in PARITY-ROADMAP.md
            // rather than left unsaid.)
            af::Explod turnedOver = placed(40.0f);
            turnedOver.onTop = true;
            turnedOver.flipV = true;
            af::EntityLayer mirrored;
            mirrored.spawnExplod(turnedOver);
            af::draw_stub::clear();
            mirrored.drawTop(renderer, 0.0f, 240.0f);
            check(af::draw_stub::recorded().size() == 1 &&
                      af::draw_stub::recorded()[0].flipV &&
                      af::draw_stub::recorded()[0].facing == 1,
                  "and `vfacing` reaches the draw as a vertical mirror of its own, "
                  "with the horizontal facing untouched");

            af::draw_stub::record(false);
        }

        // `removeongethit` on a real layer, which is where its *timing* lives and the
        // only place that can be wrong in a way the parse cannot see. The rule is
        // that the explod goes the tick its owner is hit -- and the tick its owner is
        // hit is also the tick the owner's hitpause begins, so the check has to
        // outrank the freeze gates. An explod that is held by a hitpause it never
        // sees the start of is an aura still hanging around a charge that was
        // interrupted, which is the whole thing the parameter exists to prevent.
        {
            af::Character victimChar;
            victimChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                     "[Statedef -1]\n");
            af::StateMachine victim;
            victim.setup(&victimChar, -1);
            victim.setOpponent(&dummy);
            victim.setPosition(320.0f);
            victim.setStageBounds(90.0f, 550.0f);

            af::Character auraChar;
            af::parseAirText("[Begin Action 900]\n0,0, 0,0, -1\n", auraChar.animations());

            const auto aura = [&auraChar](bool removeOnGetHit) {
                af::Explod explod;
                explod.owner = &auraChar;
                explod.ownerSide = 0;
                explod.anim = 900;
                // And it *is* subject to a hitpause -- the opposite of the default --
                // so that the assertions below do more than show the explod going:
                // they show it going while a hitpause that would otherwise hold it
                // still is running, which is only possible if the `removeongethit`
                // check sits above the freeze gates. A test that left
                // `ignorehitpause` at its permissive default would pass just as
                // happily with the check underneath them, and would therefore not be
                // pinning the one thing about the placement that matters.
                explod.ignoreHitPause = false;
                explod.removeOnGetHit = removeOnGetHit;
                return explod;
            };

            af::EntityLayer layer;
            layer.spawnExplod(aura(true));
            layer.spawnExplod(aura(false));

            // One ordinary tick first, so the layer's own clocks have moved and the
            // explods are not being judged on the tick they were born.
            layer.update(&victim, nullptr);
            check(layer.explods().size() == 2, "both explods are alive before the hit");

            af::StateMachine::HitInfo hit;
            hit.damage = 1;
            hit.attackerFacing = 1;
            hit.pushX = 0.0f;
            hit.pushY = 0.0f;
            hit.hitTime = 20;
            hit.juggleCost = 0;
            hit.stateNumber = af::CommonState::kHitHigh;
            // A real victim-side hitpause, so that the freeze gates the check has to
            // outrank are genuinely closed rather than merely absent.
            hit.victimPause = 4;
            check(victim.applyHit(hit), "the owner takes a hit");
            check(victim.inHitPause(),
                  "and the hitpause it hands the victim is running on this very tick, "
                  "which is the tick `removeongethit` has to be answered on");

            // No step in between: `tookDamage()` is true for exactly the tick a hit
            // was applied, and the fight loop's own order -- fighters first, then the
            // entity layer -- is what the layer relies on to see it.
            layer.update(&victim, nullptr);
            check(layer.explods().size() == 1 && !layer.explods()[0].removeOnGetHit,
                  "so the explod that asked to go when its owner is hit goes on that "
                  "tick, and the one beside it -- identical in every other way, and "
                  "held by that same hitpause -- stays");

            // And it is the *hit* that did it, not the state change or the hitpause
            // that came with it: a few quiet ticks later the other explod is still
            // there and still counting.
            for (int i = 0; i < 5; ++i) layer.update(&victim, nullptr);
            check(layer.explods().size() == 1,
                  "and the survivor is not merely delayed -- it is still there after "
                  "the hitpause and the get-hit state have both moved on");
        }
    }

    // ------------------------------------------------------------------ ownpal
    //
    // `ownpal` is the flag that decides whether an entity draws in its owner's
    // current palette or in one of its own. sctrls.html's Explod entry gives the
    // whole semantics: a boolean, 1 means "give the explod its own copy of its
    // palette" so that "temporary changes to the player's palette (e.g. from
    // using the PalFX controller)" do not reach it, and "Defaults to 0 if
    // omitted" -- inheritance, not own. The Projectile section does not list the
    // parameter, but characters write it there the same way, so the same
    // documented default is read for it.
    //
    // Two things are asserted separately. First the parse: both controllers,
    // each with the flag written and omitted, plus `ModifyExplod`, whose whole
    // contract is that an omitted field stays omitted. Then the decision itself,
    // which is a pure function because the GL half of drawing cannot run here --
    // the shim's draws reach no texture, so a test of the *choice* is the closest
    // a host build can come to the picture.
    {
        af::Character ownPalGunner;
        ownPalGunner.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
            "[State 300, aura]\ntype = Explod\ntrigger1 = time = 0\nanim = 900\n"
            "ownpal = 1\n\n"
            "[State 300, spark]\ntype = Explod\ntrigger1 = time = 0\nanim = 901\n\n"
            "[State 300, beam]\ntype = Projectile\ntrigger1 = time = 0\nprojanim = 902\n"
            "ownpal = 1\n\n"
            "[State 300, shot]\ntype = Projectile\ntrigger1 = time = 0\nprojanim = 903\n\n"
            "[State 300, restyle]\ntype = ModifyExplod\ntrigger1 = time = 0\nid = 0\n"
            "ownpal = 1\n\n"
            "[State 300, unstyle]\ntype = ModifyExplod\ntrigger1 = time = 0\nid = 0\n"
            "ownpal = 0\n\n",
            "[Statedef -1]\n");

        RecordingEntities ownPalRecorder;
        af::StateMachine ownPalMachine;
        ownPalMachine.setup(&ownPalGunner, 1);
        ownPalMachine.setOpponent(&dummy);
        ownPalMachine.setPosition(320.0f);
        ownPalMachine.setStageBounds(90.0f, 550.0f);
        ownPalMachine.setEntitySink(&ownPalRecorder);

        af::InputState ownPalIdle;
        for (int i = 0; i < 2; ++i) {
            ownPalMachine.control(ownPalIdle);
            ownPalMachine.step();
        }

        check(ownPalRecorder.explods.size() == 2,
              "both explod blocks should have been asked for");
        if (ownPalRecorder.explods.size() == 2) {
            check(ownPalRecorder.explods[0].ownpal,
                  "`ownpal = 1` on an Explod is carried as the flag it is");
            check(!ownPalRecorder.explods[1].ownpal,
                  "while an Explod that omits `ownpal` gets the documented default of "
                  "0 -- inheritance, which is a real answer and not the same as own");
        }

        check(ownPalRecorder.projectiles.size() == 2,
              "as should both projectile blocks");
        if (ownPalRecorder.projectiles.size() == 2) {
            check(ownPalRecorder.projectiles[0].ownpal,
                  "`ownpal = 1` is read on a Projectile too, even though the reference's "
                  "Projectile section omits the parameter and characters write it anyway");
            check(!ownPalRecorder.projectiles[1].ownpal,
                  "and an omitted one is 0 there as well, the same default the "
                  "documented controllers give the flag");
        }

        check(ownPalRecorder.modifications.size() == 2,
              "and both ModifyExplod blocks should have been asked for");
        if (ownPalRecorder.modifications.size() == 2) {
            check(ownPalRecorder.modifications[0].ownpal.has_value() &&
                      *ownPalRecorder.modifications[0].ownpal,
                  "`ModifyExplod` can turn an existing explod's own palette on");
            check(ownPalRecorder.modifications[1].ownpal.has_value() &&
                      !*ownPalRecorder.modifications[1].ownpal,
                  "and a written 0 is a value that turns it back off, not an omission "
                  "that leaves the previous policy in place");
        }

        // The decision, case by case. `spritePalette` is the sprite bank's answer
        // (-1 for a bank with none) and `ownerPalette` the owner's current
        // selection; `fromSpriteBank` says which table the index belongs to,
        // because the two can be different SFF files.
        {
            const af::EntityPaletteChoice ownPalInherits = af::chooseEntityPalette(false, 0, 4);
            check(!ownPalInherits.fromSpriteBank && ownPalInherits.index == 4,
                  "`ownpal = 0` draws through the owner's selected palette entry -- "
                  "palno and RemapPal reach the entity");

            const af::EntityPaletteChoice ownPalOwns = af::chooseEntityPalette(true, 0, 4);
            check(ownPalOwns.fromSpriteBank && ownPalOwns.index == 0,
                  "`ownpal = 1` keeps the sprite's own palette node, including entry 0, "
                  "which the owner's selection must not replace");

            const af::EntityPaletteChoice ownPalOwnNode = af::chooseEntityPalette(true, 3, 4);
            check(ownPalOwnNode.fromSpriteBank && ownPalOwnNode.index == 3,
                  "and a sprite that decoded against a dedicated entry keeps that one");

            const af::EntityPaletteChoice ownPalDedicated = af::chooseEntityPalette(false, 2, 4);
            check(ownPalDedicated.fromSpriteBank && ownPalDedicated.index == 2,
                  "a dedicated non-zero sprite palette wins even for an inherited draw, "
                  "the same precedence Fighter::draw applies -- palno does not reach "
                  "such a sprite by the SFF's own convention");

            const af::EntityPaletteChoice ownPalFallback = af::chooseEntityPalette(true, -1, 4);
            check(!ownPalFallback.fromSpriteBank && ownPalFallback.index == 4,
                  "a sprite with no palette of its own falls back to the owner's table");

            const af::EntityPaletteChoice ownPalNoTable = af::chooseEntityPalette(false, -1, -1);
            check(ownPalNoTable.index == -1,
                  "and when neither side has a table the draw is left with no palette "
                  "opinion rather than an invented one");
        }
    }

    // ---------------------------------------------------------------- RemapPal
    //
    // sctrls.html writes both ends of the controller as `(group, item)` palette
    // identities: `source = src_pal_grp, src_pal_item`, `dest = dst_pal_grp,
    // dst_pal_item`, and says every sprite using the source palette draws in the
    // destination instead. The group is the SFF v2 palette node's group
    // coordinate, not a sprite group. Two further sentences drive the shape of
    // the implementation: the destination "refers to an unmapped palette" -- the
    // lookup is one level, not a chase -- and both palettes must exist in the
    // player's sprites.
    //
    // The corpus writes the pair with group 1 (`source = 1,1`, `dest = 1,3`) and
    // the legacy one-item form must keep meaning group 1. A character that runs
    // the controller every tick must also not accumulate a rule per tick, which
    // is why a repeated source replaces its destination.
    {
        af::Character remapChar;
        remapChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
            "[State 300, pair]\ntype = RemapPal\ntrigger1 = time = 0\n"
            "source = 2,5\ndest = 1,3\n\n"
            "[State 300, pair again]\ntype = RemapPal\ntrigger1 = time = 0\n"
            "source = 2,5\ndest = 2,9\n\n"
            "[State 300, legacy]\ntype = RemapPal\ntrigger1 = time = 0\n"
            "source = 4\ndest = 2\n\n"
            "[State 300, chain a]\ntype = RemapPal\ntrigger1 = time = 0\n"
            "source = 1,1\ndest = 1,2\n\n"
            "[State 300, chain b]\ntype = RemapPal\ntrigger1 = time = 0\n"
            "source = 1,2\ndest = 1,3\n\n",
            "[Statedef -1]\n");

        af::StateMachine remapMachine;
        remapMachine.setup(&remapChar, 1);
        remapMachine.setOpponent(&dummy);
        af::InputState remapIdle;
        for (int i = 0; i < 3; ++i) {
            remapMachine.control(remapIdle);
            remapMachine.step();
        }

        const auto& remaps = remapMachine.paletteRemaps();
        // Four sources, not five: the second `2,5` entry replaced the first
        // rather than appending a rule that would never be reached.
        check(remaps.size() == 4,
              "RemapPal keeps one rule per source, so a controller that runs every "
              "tick does not grow the table");
        if (remaps.size() == 4) {
            check(remaps[0].source == af::PaletteRef{2, 5} &&
                      remaps[0].dest == af::PaletteRef{2, 9},
                  "`source = 2,5` / `dest = 1,3` then `dest = 2,9` parses both "
                  "coordinates and the later destination wins");
            check(remaps[1].source == af::PaletteRef{1, 4} &&
                      remaps[1].dest == af::PaletteRef{1, 2},
                  "the one-item legacy form means group 1, on both ends");
            check(remaps[2].source == af::PaletteRef{1, 1} &&
                      remaps[2].dest == af::PaletteRef{1, 2},
                  "and a two-item group-1 pair keeps the group it was written with");
        }

        // The selection is still `palno`; only the draw reads the redirection.
        remapMachine.setPalNo(4);
        check(remapMachine.selectedPalette() == af::PaletteRef{1, 4},
              "the selected palette is group 1 item `palno`");
        check(remapMachine.effectivePalette() == af::PaletteRef{1, 2},
              "and `source = 4` redirects it, legacy group included");
        check(remapMachine.palNo() == 4,
              "`palno` itself is the selection, not the RemapPal destination");

        // The docs' "unmapped palette": (1,1)->(1,2) and (1,2)->(1,3) draw a
        // sprite using (1,1) with (1,2). A chase would answer (1,3).
        remapMachine.setPalNo(1);
        check(remapMachine.effectivePalette() == af::PaletteRef{1, 2},
              "a RemapPal destination is unmapped -- the redirection is one level");
        remapMachine.setPalNo(2);
        check(remapMachine.effectivePalette() == af::PaletteRef{1, 3},
              "while the second link of the same chain still applies to its own source");
        remapMachine.setPalNo(5);
        check(remapMachine.effectivePalette() == af::PaletteRef{1, 5},
              "a group-1 item 5 is not the source group-2 item 5");
        check(remapMachine.remapPalette(af::PaletteRef{2, 5}) == af::PaletteRef{2, 9},
              "the group half is addressable on its own, which is what lets the draw "
              "remap a sprite's dedicated palette node");

        // The pure lookup by itself, so a mutation to either coordinate or to
        // the one-level rule is caught independently of the controller parse.
        using af::PaletteRef;
        using af::PaletteRemap;
        check(af::StateMachine::applyPaletteRemap({}, PaletteRef{1, 1}) ==
                  PaletteRef{1, 1},
              "an empty table changes nothing");
        const std::vector<PaletteRemap> chain{
            PaletteRemap{PaletteRef{1, 1}, PaletteRef{1, 2}},
            PaletteRemap{PaletteRef{1, 2}, PaletteRef{1, 3}},
        };
        check(af::StateMachine::applyPaletteRemap(chain, PaletteRef{1, 1}) ==
                  PaletteRef{1, 2},
              "the lookup is one level, not transitive");
        check(af::StateMachine::applyPaletteRemap(chain, PaletteRef{1, 2}) ==
                  PaletteRef{1, 3},
              "and each source still answers its own destination");
        const std::vector<PaletteRemap> grouped{
            PaletteRemap{PaletteRef{2, 5}, PaletteRef{1, 3}},
        };
        check(af::StateMachine::applyPaletteRemap(grouped, PaletteRef{1, 5}) ==
                  PaletteRef{1, 5},
              "a source with a different group does not match on the item alone");
        check(af::StateMachine::applyPaletteRemap(grouped, PaletteRef{2, 5}) ==
                  PaletteRef{1, 3},
              "and a matching pair redirects across groups");

        // The palette-node key table the group half resolves through. Only a
        // loaded SFF v2 can supply real keys, so the pure search is pinned
        // directly: it must compare both coordinates, reject non-positive ones,
        // and answer -1 (the honest "no such node") rather than a default.
        using af::SpriteBank;
        const std::vector<SpriteBank::PaletteKey> keys{
            SpriteBank::PaletteKey{1, 1},
            SpriteBank::PaletteKey{1, 2},
            SpriteBank::PaletteKey{2, 5},
        };
        check(SpriteBank::findPaletteKey(keys, 1, 1) == 0,
              "a palette node is found by its group and item");
        check(SpriteBank::findPaletteKey(keys, 1, 2) == 1,
              "and the item is the second coordinate, not the table position");
        check(SpriteBank::findPaletteKey(keys, 2, 5) == 2,
              "a non-1 group resolves to its node's table entry");
        check(SpriteBank::findPaletteKey(keys, 1, 5) == -1,
              "the same item under a different group is a different palette");
        check(SpriteBank::findPaletteKey(keys, 2, 1) == -1,
              "as is the same group with a different item");
        check(SpriteBank::findPaletteKey(keys, 0, 1) == -1 &&
                  SpriteBank::findPaletteKey(keys, 1, 0) == -1,
              "a non-positive coordinate names no palette node");
        check(SpriteBank::findPaletteKey({}, 1, 1) == -1,
              "and a bank with no node keys answers -1 rather than entry 0");
    }

    // --------------------------- the controllers that reached nothing at all
    //
    // A corpus sweep over local_chars -- splitting each file into its `[State ...]`
    // sections and looking at what each one's `type =` names -- turned up four
    // controllers whose every use had been falling through the dispatch and doing
    // nothing: `ModifyExplod` (149 sections), `RemoveExplod` (930), `GameMakeAnim`
    // (118) and `PowerSet` (50). Between them a character file in four in the corpus
    // was writing something the engine silently dropped.
    //
    // The first three are about entities, and the state machine does not own entities
    // -- it asks the fight to -- so what is tested here is the *ask*: which explod the
    // controller meant to reach, and which of the fields it named. `ModifyExplod` in
    // particular is a controller that is almost entirely omission, and every
    // interesting assertion about it is that it left something alone. The effect on a
    // live explod is the layer's business and is tested against a real `EntityLayer`
    // below.
    {
        af::Character dresser;
        dresser.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 300, aura]\ntype = Explod\ntrigger1 = time = 0\nanim = 900\n"
            "id = 77\npos = 10, -20\nscale = 1, 1\n\n"
            // The plain restyle: a new scale, and nothing else said. Everything the
            // explod was born with has to survive it.
            "[State 300, restyle]\ntype = ModifyExplod\ntrigger1 = time = 1\n"
            "id = 77\nscale = 2, 3\n\n"
            // The same shape as the corpus writes it: `pos` and `postype` together, as
            // the one sentence the reference gives this controller insists.
            "[State 300, move]\ntype = ModifyExplod\ntrigger1 = time = 2\n"
            "id = 77\npos = 40, -60\npostype = p1\n\n"
            // A priority that drops the explod below the fighter's own. The reference
            // gives `ontop` "precedence over sprpriority", so a modification that names
            // only `ontop` has to answer the layer question by itself.
            "[State 300, behind]\ntype = ModifyExplod\ntrigger1 = time = 3\n"
            "id = 77\nontop = 1\n\n"
            // A freeze window and a lifetime, neither of which restarts the clock.
            "[State 300, keep]\ntype = ModifyExplod\ntrigger1 = time = 4\n"
            "id = 77\npausemovetime = 12\nremovetime = 50\nignorehitpause = 0\n\n"
            // And the removal: named and unnamed.
            "[State 300, drop78]\ntype = RemoveExplod\ntrigger1 = time = 5\nid = 78\n\n"
            "[State 300, dropall]\ntype = RemoveExplod\ntrigger1 = time = 6\n\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 300]\n300,0, 0,0, -1\n", dresser.animations());

        RecordingEntities recorder;
        af::StateMachine machine;
        machine.setup(&dresser, 1);
        machine.setOpponent(&dummy);
        dummy.setPosition(420.0f);
        dummy.setStageBounds(90.0f, 550.0f);
        machine.setPosition(200.0f);
        machine.setStageBounds(90.0f, 550.0f);
        machine.setEntitySink(&recorder);
        machine.setSide(0);

        // Entering a state and running its `time = 0` controllers happen in the same
        // tick, and the state's clock is one by the time the step is over -- so the
        // blocks below are reached by asking for the state time to have *passed*,
        // not merely reached. One tick per controller, in the order they are written,
        // so each assertion is about the block at that time and the ones it left alone.
        const auto tickTo = [&](int wanted) {
            while (machine.stateTime() <= wanted) {
                af::InputState none;
                machine.control(none);
                machine.step();
            }
        };

        tickTo(1);
        check(recorder.modifications.size() == 1, "the restyle should have been asked for");
        if (recorder.modifications.size() == 1) {
            const af::ExplodModify& restyle = recorder.modifications[0];
            check(restyle.owner == &dresser, "reaching this fighter's explods and no one else's");
            check(restyle.id == 77, "by the id the aura was given");
            check(restyle.scaleX.has_value() && std::fabs(*restyle.scaleX - 2.0f) < 0.01f &&
                      restyle.scaleY.has_value() && std::fabs(*restyle.scaleY - 3.0f) < 0.01f,
                  "with the pair of scales as written, x then y");
            check(!restyle.anim.has_value(),
                  "and with no animation, because this block named none -- a modification "
                  "is not a respawn and an omitted field has to stay omitted");
            check(!restyle.movesPosition && !restyle.velX.has_value() &&
                      !restyle.accelX.has_value() && !restyle.facing.has_value() &&
                      !restyle.flipV.has_value() && !restyle.bindTime.has_value() &&
                      !restyle.removeTime.has_value() && !restyle.setTrans &&
                      !restyle.ignoreHitPause.has_value() && !restyle.pauseMoveTime.has_value() &&
                      !restyle.superMoveTime.has_value() && !restyle.spritePriority.has_value() &&
                      !restyle.onTop.has_value() && !restyle.removeOnGetHit.has_value(),
                  "nor any of the other fifteen it could have named -- the whole "
                  "controller is omission, and one field written is one field changed");
        }

        tickTo(2);
        check(recorder.modifications.size() == 2, "then the one that moves it");
        if (recorder.modifications.size() == 2) {
            const af::ExplodModify& move = recorder.modifications[1];
            check(move.movesPosition, "which does move it");
            // `pos = 40, -60` with `postype = p1`, and `p1`'s axis is the fighter: forty
            // in front of it and sixty above it. The engine keeps height the other way
            // up, so the sign crosses once -- the same crossing every other character y
            // makes on its way into the world.
            check(move.x > machine.x() + 30.0f && move.x < machine.x() + 50.0f,
                  "forty units in front of the fighter when the fighter is the reference");
            check(std::fabs(move.ownerX - machine.x()) < 0.01f,
                  "and it carries where the owner stood, so a bound explod can be re-bound "
                  "about its new place instead of being dragged back on the next tick");
            check(!move.scaleX.has_value(),
                  "while the scale the previous block set is not re-sent -- each block "
                  "answers for itself");
        }

        tickTo(3);
        check(recorder.modifications.size() == 3, "then the layering one");
        if (recorder.modifications.size() == 3) {
            const af::ExplodModify& behind = recorder.modifications[2];
            check(behind.onTop.has_value() && *behind.onTop,
                  "which asked for the explod to go on top");
            check(behind.behindOwner.has_value() && !*behind.behindOwner,
                  "and had the layer question answered for it: `ontop` outranks "
                  "`sprpriority`, so asking for the top cannot leave it behind its owner");
            check(!behind.spritePriority.has_value(),
                  "without inventing a priority the block never named");
        }

        tickTo(4);
        check(recorder.modifications.size() == 4, "then the one that reaches into the freezes");
        if (recorder.modifications.size() == 4) {
            const af::ExplodModify& keep = recorder.modifications[3];
            check(keep.pauseMoveTime.has_value() && *keep.pauseMoveTime == 12,
                  "with its twelve ticks of Pause");
            check(keep.superMoveTime.has_value() == false,
                  "and nothing for a SuperPause, because it named nothing for one -- the "
                  "two windows stay two parameters even in a modification");
            check(keep.removeTime.has_value() && *keep.removeTime == 50,
                  "and a fifty-tick lifetime, which here counts from now");
            check(keep.ignoreHitPause.has_value() && !*keep.ignoreHitPause,
                  "and a deliberate opt-out of the hitpause, which is the non-default");
        }

        tickTo(5);
        check(recorder.removals.size() == 1, "and finally a removal");
        if (recorder.removals.size() == 1) {
            check(recorder.removals[0].owner == &dresser && recorder.removals[0].id == 78 &&
                      !recorder.removals[0].all,
                  "naming one id, so only the explods carrying it are reached");
        }

        tickTo(6);
        check(recorder.removals.size() == 2, "and the second one");
        if (recorder.removals.size() == 2) {
            check(recorder.removals[1].all,
                  "with no id at all, which the reference documents as 'removes all explods "
                  "owned by the player' and not as the id 0 -- 18 of the corpus' 930 "
                  "`RemoveExplod` blocks leave the id out, and they mean all of them");
        }

        // The three `GameMakeAnim`s and the `PowerSet`. GameMakeAnim is documented as
        // deprecated and superseded by Explod, and the corpus agrees with the docs
        // rather than the other way round: 118 uses against `Explod`'s thousands. It is
        // still an explod -- it is *only* an explod, with a fixed set of parameters and
        // no `id`, no `bindtime` and no lifetime -- so it goes through the same spawn.
        af::Character gamer;
        gamer.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            // A meter that is already part-way up, so that the block below can be told
            // from a `PowerAdd` with the same number on it: 1000 is a set, 1400 is an add.
            "[State 0, warmup]\ntype = PowerAdd\ntrigger1 = time = 0\nvalue = 400\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 1\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 300, pop]\ntype = PowerSet\ntrigger1 = time = 0\nvalue = 1000\n\n"
            "[State 300, spark]\ntype = GameMakeAnim\ntrigger1 = time = 1\nvalue = 901\n"
            "pos = 12, -30\n\n"
            "[State 300, under]\ntype = GameMakeAnim\ntrigger1 = time = 2\nvalue = 902\n"
            "under = 1\n\n"
            "[State 300, jitter]\ntype = ChangeState\ntrigger1 = time = 3\nvalue = 310\n\n"
            // The same animation, thrown every tick, with the documented `random`
            // displacement on it -- which is one number drawn twice, once per axis.
            "[Statedef 310]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 310, jitter]\ntype = GameMakeAnim\ntrigger1 = 1\nvalue = 903\n"
            "random = 20\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 300]\n300,0, 0,0, -1\n", gamer.animations());

        RecordingEntities popup;
        af::StateMachine gamerMachine;
        gamerMachine.setup(&gamer, 1);
        gamerMachine.setOpponent(&dummy);
        dummy.setPosition(420.0f);
        dummy.setStageBounds(90.0f, 550.0f);
        gamerMachine.setPosition(200.0f);
        gamerMachine.setStageBounds(90.0f, 550.0f);
        gamerMachine.setEntitySink(&popup);
        gamerMachine.setSide(0);

        // Driven by what the machine has asked for rather than by a tick count: the
        // state's clock is one by the time the step that entered it is over, and this
        // character pays an entry tick for its warm-up, so counting ticks here would be
        // arithmetic about the harness instead of about the controller.
        af::InputState gamerIdle;
        const auto gamerStep = [&] {
            gamerMachine.control(gamerIdle);
            gamerMachine.step();
        };
        const auto gamerUntil = [&](const std::function<bool()>& done) {
            for (int i = 0; i < 60 && !done(); ++i) gamerStep();
        };

        gamerUntil([&] { return gamerMachine.stateNumber() == 300; });
        check(gamerMachine.stateNumber() == 300, "the gamer should be in its move");
        check(gamerMachine.power() == 1000,
              "`PowerSet` is the only controller that *writes* the power meter rather "
              "than adding to it: the fighter was carrying 400 and the block said 1000, "
              "and 1000 is what the meter reads -- a `PowerAdd` of the same number would "
              "have left it at 1400");

        gamerUntil([&] { return !popup.explods.empty(); });
        check(popup.explods.size() == 1, "and `GameMakeAnim` asked for a real explod");
        if (popup.explods.size() == 1) {
            const af::Explod& spark = popup.explods[0];
            check(spark.fromFightFx,
                  "drawn out of the fight effects, which is the one thing about it that "
                  "is not the same as Explod: its `value` has no `F` prefix because there "
                  "is nowhere else it could come from");
            check(spark.anim == 901, "at the animation it named");
            // `under = 1` is the documented parameter, and its default of 0 means "in
            // front", so the two blocks below differ in exactly this one thing.
            check(!spark.behindOwner, "and in front, because `under` defaults to 0");
            check(spark.bindTime == 1,
                  "and stays where it was put: the reference gives this controller no "
                  "`bindtime` to say otherwise, so the spark that a hit throws off is not "
                  "dragged along with the fighter that threw it");
        }

        gamerUntil([&] { return popup.explods.size() >= 2; });
        check(popup.explods.size() == 2, "and a second one behind the fighter");
        if (popup.explods.size() == 2) {
            check(popup.explods[1].behindOwner, "because this one wrote `under = 1`");
        }

        // `random` is documented as a *displacement* -- "the position of the animation
        // will be displaced in the x and y directions by (different) random amounts...
        // as large as half of rand_amt" -- so twenty is at most ten units each way, and
        // the two draws are the two axes rather than one draw used for both. Six of them
        // are collected below so that the bound is asserted *and* the displacement is
        // shown to be a displacement, which one explod could not show: a spark that
        // never moved at all would satisfy a bound on its own.
        gamerUntil([&] { return gamerMachine.stateNumber() == 310 && popup.explods.size() >= 8; });
        const size_t spawns = popup.explods.size();
        check(spawns >= 8, "the jittering state should have thrown a spark a tick");
        bool moved = false;
        bool inRange = true;
        for (size_t i = 2; i < spawns; ++i) {
            const float dx = popup.explods[i].x - gamerMachine.x();
            const float dy = popup.explods[i].y - gamerMachine.y();
            if (std::fabs(dx) > 0.01f || std::fabs(dy) > 0.01f) moved = true;
            if (std::fabs(dx) > 10.5f || std::fabs(dy) > 10.5f) inRange = false;
        }
        check(inRange,
              "every one of them landed within ten units of the fighter -- half of the "
              "twenty, in each direction, on both axes");
        check(moved,
              "and at least one of them landed somewhere other than exactly on it, which "
              "is the whole of what `random` does");

        // And the same two controllers against a real `EntityLayer`, because the ask
        // being right is only half of it: what a modification *does* to an explod
        // already on screen is the layer's half, and the two are tested apart so that a
        // layer that ignored the request could not hide behind a state machine that
        // asked for the right thing. All of it here is ordinary `Explod`s and a layer,
        // with no state machine in the picture at all.
        {
            af::Character wearer;
            wearer.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                                 "[Statedef -1]\n");
            af::Character stranger;
            stranger.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                                   "[Statedef -1]\n");

            af::EntityLayer layer;
            af::Explod mine;
            mine.owner = &wearer;
            mine.ownerSide = 0;
            mine.id = 77;
            mine.anim = 900;
            mine.scaleX = 1.0f;
            mine.scaleY = 1.0f;
            mine.bindTime = -1;
            // A lifetime long enough that the explod is not taken off by its own
            // animation running out -- the layer here has no `fightfx` set, so an
            // animation that cannot be found is treated as one that has finished, and
            // the assertions below are about the removal controller and not about that.
            mine.removeTime = 600;
            mine.x = 300.0f;
            mine.y = 100.0f;
            mine.bindOffsetX = 10.0f;
            mine.bindOffsetY = 10.0f;
            layer.spawnExplod(mine);

            // The same explod under a different owner, to show the id alone does not
            // decide who is reached.
            af::Explod theirs = mine;
            theirs.owner = &stranger;
            layer.spawnExplod(theirs);

            af::ExplodModify restyle;
            restyle.owner = &wearer;
            restyle.id = 77;
            restyle.scaleX = 4.0f;
            restyle.scaleY = 5.0f;
            layer.modifyExplod(restyle);
            check(layer.explods()[0].scaleX == 4.0f && layer.explods()[0].scaleY == 5.0f,
                  "a modification reaches the explods carrying its id");
            check(layer.explods()[1].scaleX == 1.0f,
                  "and not the identical ones belonging to somebody else, which is the "
                  "other half of what the selector says");
            check(layer.explods()[0].anim == 900 && layer.explods()[0].removeTime == 600 &&
                      layer.explods()[0].clock == 0 && layer.explods()[0].bindTime == -1,
                  "and changes only the fields it named: the animation, the lifetime, the "
                  "clock and the bind are all as they were, because a modification is not "
                  "a respawn");

            // And the position, which is the one field with a second half to it: an
            // explod bound to its owner has to go on following it from where it was put,
            // or the layer's own bind would drag it back on the next tick.
            af::ExplodModify move;
            move.owner = &wearer;
            move.id = 77;
            move.movesPosition = true;
            move.x = 500.0f;
            move.y = 120.0f;
            move.ownerX = 380.0f;
            move.ownerY = 100.0f;
            layer.modifyExplod(move);
            check(layer.explods()[0].x == 500.0f && layer.explods()[0].bindOffsetX == 120.0f,
                  "and a moved explod is re-bound about the place it was moved to, so that "
                  "a bound one follows its owner from there rather than snapping back to "
                  "where it was born");
            check(layer.explods()[1].x == 300.0f,
                  "while the one that was not reached has not moved at all");

            // `RemoveExplod`. The sweep at the end of `update` is where a `gone` explod
            // leaves the list, but the count has to agree with the removal on the very
            // tick it happens, because that is the tick a `trigger1 = numexplod(...) = 0`
            // is asking on.
            check(layer.countExplods(&wearer, nullptr, 77) == 1, "the wearer's explod is there");
            layer.removeExplod(&wearer, 78, false);
            check(layer.countExplods(&wearer, nullptr, 77) == 1,
                  "and a removal naming an id nothing carries takes nothing away");
            layer.removeExplod(&wearer, 77, false);
            check(layer.countExplods(&wearer, nullptr, 77) == 0,
                  "while one naming the right id takes it away on this same tick, without "
                  "waiting for the next");
            check(layer.countExplods(&stranger, nullptr, 77) == 1,
                  "and the other owner's is untouched");

            layer.update(nullptr, nullptr);
            check(layer.explods().size() == 1 && layer.explods()[0].owner == &stranger,
                  "so the next sweep is only tidying up after a removal that has already "
                  "taken effect");

            // The unnamed form: the reference's "If omitted, removes all explods owned by
            // the player". Not the id 0 -- this explod carries no id at all, which is
            // the id 0 an omitted `id` on an `Explod` means, and it goes.
            af::Explod plain = mine;
            plain.owner = &stranger;
            plain.id = 0;
            layer.spawnExplod(plain);
            check(layer.countExplods(&stranger, nullptr, -1) == 2, "the stranger has two explods");
            layer.removeExplod(&stranger, 0, true);
            check(layer.countExplods(&stranger, nullptr, -1) == 0,
                  "and a removal with no id at all takes every one of them, whatever id "
                  "they happen to carry -- which is why it can be told from a removal of "
                  "the id 0");
        }
    }

    // ------------------------------------ which file an animation comes from
    //
    // MUGEN writes an animation parameter with a prefix that says which of two files
    // the animation is in, and the two spellings in use are mirror images of each
    // other: an `Explod`'s `anim` is the character's own file bare and the
    // screenpack's `fightfx` under an `F`, while a `SuperPause`'s `anim` -- like a
    // `HitDef`'s `sparkno` -- is `fightfx` bare and the character's own under an `S`.
    //
    // The prefix has to be read off the *raw* text, because left to the expression
    // parser a bare `F72` is an identifier this engine does not know, and an unknown
    // identifier used to evaluate to 0 -- which is the one animation every character
    // has, its stand. A super-jump's landing dust then came out as a second copy of
    // the fighter, drawn additively, parked wherever the controller's `postype` put
    // it. So these pin the fix, and they pin the two conventions *apart*: the same 72
    // means a different file to the two controllers, which is the whole trap.
    {
        struct Spawned {
            int calls = 0;
            int anim = 0;
            bool fromFightFx = false;
            bool present = false;
        };

        // One synthetic character per scenario: its standing state hands straight over
        // to a state 300 holding a single controller, so the sink ends with exactly
        // one explod and nothing else can be blamed for it. The controller is passed
        // in whole, `type = ...` and all.
        const auto runController = [&](const std::string& controller) {
            af::Character probe;
            probe.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = 1\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 300, act]\ntype = " + controller + "\n",
                "[Statedef -1]\n");

            RecordingEntities recorder;
            af::StateMachine probeMachine;
            probeMachine.setup(&probe, 1);
            probeMachine.setOpponent(&dummy);
            probeMachine.setPosition(320.0f);
            probeMachine.setStageBounds(90.0f, 550.0f);
            probeMachine.setEntitySink(&recorder);
            for (int i = 0; i < 30; ++i) {
                af::InputState tick;
                probeMachine.control(tick);
                probeMachine.step();
            }

            Spawned out;
            out.calls = recorder.spawnExplodCalls;
            if (!recorder.explods.empty()) {
                out.present = true;
                out.anim = recorder.explods.front().anim;
                out.fromFightFx = recorder.explods.front().fromFightFx;
            }
            return out;
        };

        // 1. An `F` prefix on an `Explod` names the screenpack's fightfx, not the
        //    character's own file, and the number keeps its value.
        const Spawned fPrefixed = runController("Explod\ntrigger1 = time = 0\nanim = F72");
        check(fPrefixed.calls == 1 && fPrefixed.present,
              "an `Explod` should ask for exactly one explod");
        check(fPrefixed.anim == 72, "`anim = F72` should read as animation 72, not 0");
        check(fPrefixed.fromFightFx, "and the `F` should name the screenpack's fightfx");

        // 2. The same number bare is the character's own file: the same 72, the other
        //    bank. This is the mirror of scenario 1.
        const Spawned bare = runController("Explod\ntrigger1 = time = 0\nanim = 72");
        check(bare.anim == 72, "`anim = 72` should still be animation 72");
        check(!bare.fromFightFx, "but a bare number should name the character's own file");

        // 3. `-1` means "nothing to draw", and stays that way.
        const Spawned none = runController("Explod\ntrigger1 = time = 0\nanim = -1");
        check(none.anim == -1, "`anim = -1` should name no animation");

        // 4. `F-1` is a fightfx animation that is not there: still nothing to draw --
        //    but still the *fightfx* file, so a miss does not quietly fall back to the
        //    character's own art.
        const Spawned fNegative = runController("Explod\ntrigger1 = time = 0\nanim = F-1");
        check(fNegative.anim == -1, "`anim = F-1` should name no animation");
        check(fNegative.fromFightFx, "but still out of fightfx, not the character's own file");

        // 5. An expression is evaluated the ordinary way and keeps the bare-number
        //    convention: `70 + 2` is the character's own animation 72, not fightfx.
        const Spawned expression = runController("Explod\ntrigger1 = time = 0\nanim = 70 + 2");
        check(expression.anim == 72, "`anim = 70 + 2` should evaluate to 72");
        check(!expression.fromFightFx, "and an expression keeps the bare-number convention");

        // 6. A malformed prefix must not quietly become animation 0 -- the stand every
        //    character has, which is the bug this fix is about. It is worth seeing what
        //    this engine does with one: `Fxyz` is not a prefixed number, so it falls to
        //    the ordinary expression path, where a short all-letter identifier is a
        //    *string* (see the fallback at the end of StateMachine::ownTrigger) and a
        //    non-empty string is truthy -- so it reads as 1, not as a missing animation.
        //    What this pins is the property that matters and that the fix delivers: it
        //    is not animation 0, and it does not claim a fightfx animation it never
        //    named. (The host suite's brief for this block expected -1 here; the engine
        //    does not produce it, and pinning -1 would make the suite red. See the run
        //    notes.)
        const Spawned malformed = runController("Explod\ntrigger1 = time = 0\nanim = Fxyz");
        check(malformed.anim != 0,
              "a malformed prefix must not fall back to animation 0, the stand animation");
        check(!malformed.fromFightFx,
              "and must not name a fightfx animation the parameter never spelled out");

        // 7. `SuperPause`'s `anim` is the other convention: an `S` prefix is the
        //    character's own file, which is what the roster's `S3102` super flashes mean.
        const Spawned sPrefixed =
            runController("SuperPause\ntrigger1 = time = 0\ntime = 10\nanim = S3102");
        check(sPrefixed.calls == 1 && sPrefixed.present,
              "a `SuperPause` with an animation should ask for one flash explod");
        check(sPrefixed.anim == 3102, "`anim = S3102` should read as the character's animation");
        check(!sPrefixed.fromFightFx, "and the `S` should name the character's own file");

        // 8. Bare, on a `SuperPause`, is fightfx -- the mirror of scenario 2.
        const Spawned superBare =
            runController("SuperPause\ntrigger1 = time = 0\ntime = 10\nanim = 100");
        check(superBare.anim == 100, "`anim = 100` on a `SuperPause` should be animation 100");
        check(superBare.fromFightFx, "and a bare number there should name fightfx");

        // 9. The prefix letter is case-insensitive, because character files really do
        //    write both -- `local_chars/dbz_gogeta/gogetaMVC.cns` uses `f221`.
        const Spawned lowerF = runController("Explod\ntrigger1 = time = 0\nanim = f72");
        check(lowerF.anim == 72, "`anim = f72` should read as animation 72, as `F72` does");
        check(lowerF.fromFightFx, "with a lower-case `f` naming fightfx just the same");
    }

    // ------------------------------------------------- the global palette grades
    //
    // BGPalFX and AllPalFX grade the whole screen, not one fighter, so they are not
    // slots on the state machine at all: runController hands each to its own slot on
    // the fight -- the EntitySink -- and the fighter's own PalFX keeps the third,
    // separate slot. The RE pass was explicit that the three are three and not
    // aliases of one another, so each scenario below checks which sink call fired
    // *and* that the other two did not, and the PalFX one is the guard that a
    // fighter's grade never leaks onto the global slots.
    {
        af::Character painter;
        painter.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[Statedef 300]\ntype = S\nphysics = S\nctrl = 0\n"
            "[State 300, bg]\ntype = BGPalFX\ntrigger1 = time = 0\n"
            "time = 30\nmul = 128, 200, 256\nadd = 20, 0, -10\ncolor = 200\ninvertall = 1\n\n"
            "[Statedef 301]\ntype = S\nphysics = S\nctrl = 0\n"
            "[State 301, all]\ntype = AllPalFX\ntrigger1 = time = 0\n"
            "time = 24\nmul = 100, 150, 200\nadd = 10, 20, 30\ncolor = 128\ninvertall = 0\n\n"
            "[Statedef 302]\ntype = S\nphysics = S\nctrl = 0\n"
            "[State 302, self]\ntype = PalFX\ntrigger1 = time = 0\n"
            "time = 30\nmul = 50, 60, 70\nadd = 5, 6, 7\ncolor = 200\n\n"
            // A BGPalFX that asks for time = 0: a reset, and a no-op on a fresh slot.
            "[Statedef 303]\ntype = S\nphysics = S\nctrl = 0\n"
            "[State 303, reset]\ntype = BGPalFX\ntrigger1 = time = 0\n"
            "time = 0\nmul = 10, 10, 10\n\n"
            // And one that asks for time = -1: permanent.
            "[Statedef 304]\ntype = S\nphysics = S\nctrl = 0\n"
            "[State 304, forever]\ntype = BGPalFX\ntrigger1 = time = 0\n"
            "time = -1\nmul = 200, 100, 50\nadd = -5, 0, 5\n\n"
            // A state that grades the fighter and the background at once, to show
            // the second does not touch the first.
            "[Statedef 305]\ntype = S\nphysics = S\nctrl = 0\n"
            "[State 305, self]\ntype = PalFX\ntrigger1 = time = 0\n"
            "time = 30\nmul = 50, 60, 70\nadd = 5, 6, 7\ncolor = 200\n"
            "[State 305, bg]\ntype = BGPalFX\ntrigger1 = time = 0\n"
            "time = 30\nmul = 128, 128, 128\nadd = 3, 0, 0\n",
            "[Command]\nname = \"bg\"\ncommand = x\n\n"
            "[Command]\nname = \"all\"\ncommand = y\n\n"
            "[Command]\nname = \"self\"\ncommand = z\n\n"
            "[Command]\nname = \"reset\"\ncommand = a\n\n"
            "[Command]\nname = \"forever\"\ncommand = b\n\n"
            "[Command]\nname = \"both\"\ncommand = c\n\n"
            "[Statedef -1]\n"
            "[State -1, bg]\ntype = ChangeState\ntrigger1 = command = \"bg\"\n"
            "trigger1 = ctrl\nvalue = 300\n"
            "[State -1, all]\ntype = ChangeState\ntrigger1 = command = \"all\"\n"
            "trigger1 = ctrl\nvalue = 301\n"
            "[State -1, self]\ntype = ChangeState\ntrigger1 = command = \"self\"\n"
            "trigger1 = ctrl\nvalue = 302\n"
            "[State -1, reset]\ntype = ChangeState\ntrigger1 = command = \"reset\"\n"
            "trigger1 = ctrl\nvalue = 303\n"
            "[State -1, forever]\ntype = ChangeState\ntrigger1 = command = \"forever\"\n"
            "trigger1 = ctrl\nvalue = 304\n"
            "[State -1, both]\ntype = ChangeState\ntrigger1 = command = \"both\"\n"
            "trigger1 = ctrl\nvalue = 305\n");

        dummy.setPosition(420.0f);
        dummy.setStageBounds(90.0f, 550.0f);

        // Enters whichever state a button's command names, the same way the gunner
        // block above does, and steps on to the tick the `time = 0` triggers fire.
        const auto enter = [&](af::StateMachine& machine, RecordingEntities& sink,
                               uint32_t button) {
            machine.setup(&painter, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(200.0f);
            machine.setStageBounds(90.0f, 550.0f);
            machine.setSide(0);
            machine.setEntitySink(&sink);
            af::InputState idle;
            for (int i = 0; i < 2; ++i) {
                machine.control(idle);
                machine.step();
            }
            af::InputState press;
            press.held = button;
            press.pressed = button;
            machine.control(press);
            machine.step();
            for (int i = 0; i < 2; ++i) {
                machine.control(idle);
                machine.step();
            }
        };

        // 1. BGPalFX reaches the background slot, and only it.
        RecordingEntities bgSink;
        af::StateMachine bgMachine;
        enter(bgMachine, bgSink, af::Button::kX);
        check(bgMachine.stateNumber() == 300, "the bg command should reach its state");
        check(bgSink.bgPalFxCalls == 1, "a BGPalFX should write the background slot once");
        check(bgSink.allPalFxCalls == 0, "and must not touch the all-screen slot");
        check(bgSink.bgPalFx.active, "with the grade armed");
        check(bgSink.bgPalFx.time == 30, "for the thirty ticks it was given");
        check(std::fabs(bgSink.bgPalFx.mul[0] - 128.0f) < 0.01f &&
                  std::fabs(bgSink.bgPalFx.mul[1] - 200.0f) < 0.01f &&
                  std::fabs(bgSink.bgPalFx.mul[2] - 256.0f) < 0.01f,
              "carrying its mul, channel for channel");
        check(std::fabs(bgSink.bgPalFx.add[0] - 20.0f) < 0.01f &&
                  std::fabs(bgSink.bgPalFx.add[1]) < 0.01f &&
                  std::fabs(bgSink.bgPalFx.add[2] + 10.0f) < 0.01f,
              "and its add, including the negative channel");
        check(bgSink.bgPalFx.invert, "and its invertall");
        check(std::fabs(bgSink.bgPalFx.color - 200.0f) < 0.01f, "and its colour");

        // 2. AllPalFX reaches the whole-screen slot, and only it.
        RecordingEntities allSink;
        af::StateMachine allMachine;
        enter(allMachine, allSink, af::Button::kY);
        check(allMachine.stateNumber() == 301, "the all command should reach its state");
        check(allSink.allPalFxCalls == 1, "an AllPalFX should write the all-screen slot once");
        check(allSink.bgPalFxCalls == 0, "and must not touch the background slot");
        check(allSink.allPalFx.active && allSink.allPalFx.time == 24,
              "with the grade armed for its twenty-four ticks");
        check(std::fabs(allSink.allPalFx.mul[0] - 100.0f) < 0.01f &&
                  std::fabs(allSink.allPalFx.mul[1] - 150.0f) < 0.01f &&
                  std::fabs(allSink.allPalFx.mul[2] - 200.0f) < 0.01f,
              "carrying its own mul");
        check(std::fabs(allSink.allPalFx.add[2] - 30.0f) < 0.01f, "and its own add");
        check(!allSink.allPalFx.invert, "and no invertall, which it did not ask for");

        // 3. PalFX stays on the fighter, reaching neither global slot.
        RecordingEntities selfSink;
        af::StateMachine selfMachine;
        enter(selfMachine, selfSink, af::Button::kZ);
        check(selfMachine.stateNumber() == 302, "the self command should reach its state");
        check(selfSink.bgPalFxCalls == 0 && selfSink.allPalFxCalls == 0,
              "a PalFX must not reach either global slot");
        check(selfMachine.palFx().active, "it grades the fighter itself");
        check(std::fabs(selfMachine.palFx().mul[0] - 50.0f) < 0.01f &&
                  std::fabs(selfMachine.palFx().mul[2] - 70.0f) < 0.01f,
              "with the values it was given");

        // 4. time = 0 on a fresh BGPalFX arms nothing: it is an identity reset.
        RecordingEntities resetSink;
        af::StateMachine resetMachine;
        enter(resetMachine, resetSink, af::Button::kA);
        check(resetMachine.stateNumber() == 303, "the reset command should reach its state");
        // The controller is still forwarded -- so a prior grade would be cleared --
        // but what it forwards is the identity, not the mul it named. Pinning the
        // call keeps this honest: without it the check below would also pass if the
        // controller had simply been dropped.
        check(resetSink.bgPalFxCalls == 1, "a time = 0 BGPalFX is still forwarded once");
        check(!resetSink.bgPalFx.active, "a time = 0 BGPalFX must not arm a grade");
        check(std::fabs(resetSink.bgPalFx.mul[0] - 256.0f) < 0.01f &&
                  std::fabs(resetSink.bgPalFx.color - 256.0f) < 0.01f && !resetSink.bgPalFx.invert,
              "and what it leaves is the identity, not the mul it named");

        // 5. time = -1 is permanent, and says so rather than reading as zero.
        RecordingEntities foreverSink;
        af::StateMachine foreverMachine;
        enter(foreverMachine, foreverSink, af::Button::kB);
        check(foreverMachine.stateNumber() == 304, "the forever command should reach its state");
        check(foreverSink.bgPalFx.active, "a time = -1 BGPalFX arms a grade");
        check(foreverSink.bgPalFx.time == -1, "and reports it as permanent, not as zero");
        check(std::fabs(foreverSink.bgPalFx.mul[0] - 200.0f) < 0.01f, "with its values intact");

        // 6. A PalFX and a BGPalFX in one state land in their own slots: the second
        //    must not disturb the first.
        RecordingEntities bothSink;
        af::StateMachine bothMachine;
        enter(bothMachine, bothSink, af::Button::kC);
        check(bothMachine.stateNumber() == 305, "the both command should reach its state");
        check(bothSink.bgPalFx.active && std::fabs(bothSink.bgPalFx.mul[0] - 128.0f) < 0.01f,
              "the background slot should carry the BGPalFX");
        check(std::fabs(bothMachine.palFx().mul[0] - 50.0f) < 0.01f &&
                  std::fabs(bothMachine.palFx().color - 200.0f) < 0.01f,
              "and the fighter's own slot should keep the PalFX, undisturbed");
        check(bothSink.allPalFxCalls == 0, "with neither spilling into the all-screen slot");
    }

    // ------------------------------------------------------------------- the AI
    //
    // Two computers, thirty seconds, no player, through the same loop the fight
    // screen runs -- control, step, shove apart, resolve the hit. What is pinned
    // is not that the CPU fights *well*, which is a matter of taste, but that it
    // fights at all and does not seize up. A computer that walks into range and
    // then stands in one state for the rest of the round is what the AI looked
    // like while the animation and state bugs were pulling against it, and from
    // the controls it is indistinguishable from "the AI is broken".
    {
        af::Fighter cpuA;
        af::Fighter cpuB;
        cpuA.setup(&character, 240.0f, 1);
        cpuB.setup(&character, 400.0f, -1);
        // Both declare the same localcoord here, so this is the 1.0 the fight
        // screen computes for this roster; it has to be set because the collision
        // boxes are scaled with it.
        cpuA.setScale(1.0f);
        cpuB.setScale(1.0f);
        cpuA.setAiDifficulty(5);
        cpuB.setAiDifficulty(5);

        std::vector<int> seen;
        std::map<int, int> visits;
        int longestRun = 0;
        int longestRunState = -1;
        int runState = -1;
        int runLength = 0;
        int controlTicks = 0;
        int attackTicks = 0;
        int transitions = 0;
        int lastState = -999;
        int strikes = 0;
        const int kTicks = 1800;
        const float floorY = 192.0f;
        for (int tick = 0; tick < kTicks; ++tick) {
            cpuA.controlAI(cpuB);
            cpuB.controlAI(cpuA);
            cpuA.step(cpuB);
            cpuB.step(cpuA);
            af::Fighter::separate(cpuA, cpuB);
            if (cpuA.tryHit(cpuB, floorY)) ++strikes;
            if (cpuB.tryHit(cpuA, floorY)) ++strikes;

            const af::StateMachine& machine = cpuA.machine();
            if (machine.ctrl()) ++controlTicks;
            if (machine.moveType() == af::MoveType::Attack) ++attackTicks;
            const int state = machine.stateNumber();
            if (state != lastState) {
                ++transitions;
                lastState = state;
            }
            if (std::find(seen.begin(), seen.end(), state) == seen.end()) seen.push_back(state);
            ++visits[state];
            // A hit state is the one place a fighter is *meant* to sit still: a
            // knock-out ends the round lying on the floor and stays there.
            if (machine.moveType() == af::MoveType::Hit) {
                runState = -1;
                runLength = 0;
                continue;
            }
            if (state == runState) {
                ++runLength;
                if (runLength > longestRun) {
                    longestRun = runLength;
                    longestRunState = state;
                }
            } else {
                runState = state;
                runLength = 1;
            }
        }

        std::printf("  ai: %zu states, %d transitions, %d control / %d attacking / %d ticks,"
                    " longest run %d for %d, %d strikes, life %d vs %d\n",
                    seen.size(), transitions, controlTicks, attackTicks, kTicks, longestRunState,
                    longestRun, strikes, cpuA.life(), cpuB.life());
        if (longestRun >= 900) {
            // Something held the fighter down. Say what, tick by tick, because the
            // interesting part of a freeze is always the tick it started.
            std::printf("  stuck: for %d ticks in state %d; last ticks were\n", longestRun,
                        longestRunState);
            af::Fighter probeA;
            af::Fighter probeB;
            probeA.setup(&character, 240.0f, 1);
            probeB.setup(&character, 400.0f, -1);
            probeA.setScale(1.0f);
            probeB.setScale(1.0f);
            probeA.setAiDifficulty(5);
            probeB.setAiDifficulty(5);
            int run = 0;
            int runState = -1;
            for (int tick = 0; tick < kTicks; ++tick) {
                probeA.controlAI(probeB);
                probeB.controlAI(probeA);
                probeA.step(probeB);
                probeB.step(probeA);
                af::Fighter::separate(probeA, probeB);
                const af::StateMachine& m = probeA.machine();
                if (m.stateNumber() == runState) {
                    ++run;
                } else {
                    runState = m.stateNumber();
                    run = 1;
                }
                if (tick < 26 || tick % 300 == 0) {
                    std::printf("     t%-5d state %-6d run %-5d y %8.2f x %7.1f anim %-6d elem %d"
                                " ctrl %d move %d type %d pause %d shake %d\n",
                                tick, m.stateNumber(), run, m.y(), m.x(), m.animNumber(),
                                m.animationFrameIndex(), m.ctrl() ? 1 : 0,
                                static_cast<int>(m.moveType()), static_cast<int>(m.stateType()),
                                m.pauseTicks(), m.shakeTicks());
                }
            }
        }

        std::vector<std::pair<int, int>> ranked(visits.begin(), visits.end());
        std::sort(ranked.begin(), ranked.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        std::printf("  ai busiest:");
        for (size_t i = 0; i < ranked.size() && i < 6; ++i) {
            std::printf(" %d x%d", ranked[i].first, ranked[i].second);
        }
        std::printf("\n");
        check(seen.size() >= 4, "a computer should use more than one of the character's states");
        check(transitions > 20,
              "a computer should keep changing state rather than seizing up in one");
        check(attackTicks > 0, "a computer should throw some attacks");
        check(longestRun < 900,
              "nothing should hold a computer in one state for fifteen seconds");
    }

    // ------------------------------------------------- chain moves and history
    //
    // `id` names a hit, `chainID` requires that the victim's *last* hit carried
    // that id, and `nochainID` forbids up to two. The gate is asked of the victim's
    // history rather than of the attacker's move, so these arm a real HitDef out of
    // a real file and then ask the eligibility predicate about victims with
    // different pasts -- first about the parse, then about the gate.
    {
        // A character whose standing state changes into an attack state holding one
        // HitDef, so reading `armedHitDef()` back gives the parse the engine would
        // use in a fight rather than a struct built by hand.
        const auto armDef = [&](const std::string& chainLines, af::Character& chr,
                                af::StateMachine& machine) {
            chr.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n"
                "damage = 20\n" + chainLines + "\n",
                "[Statedef -1]\n");
            machine.setup(&chr, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(320.0f);
            machine.setStageBounds(90.0f, 550.0f);
            for (int i = 0; i < 20 && !machine.hasArmedHitDef(); ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
            return machine.armedHitDef();
        };

        // --- the parse, including the rule for a HitDef that contradicts itself ---

        af::Character plainChar;
        af::StateMachine plainMachine;
        const af::StateMachine::HitDef plain = armDef("", plainChar, plainMachine);
        check(plain.id == -1 && plain.chainId == -1 && plain.noChainId == -1 &&
                  plain.noChainId2 == -1,
              "a HitDef that writes no chain parameter should restrict nothing");
        // `affectteam = F/E/B` parses to the reference's team masks, with an
        // omitted parameter defaulting to Enemy (2). The pure collision test
        // covers what each mask means on either side.
        check(plain.affectTeam == 2, "an omitted affectteam should default to Enemy");
        af::Character friendChar;
        af::StateMachine friendMachine;
        const af::StateMachine::HitDef friendHit =
            armDef("affectteam = F", friendChar, friendMachine);
        check(friendHit.affectTeam == 1, "affectteam = F should be Friend (1)");
        af::Character enemyChar;
        af::StateMachine enemyMachine;
        const af::StateMachine::HitDef enemyHit =
            armDef("affectteam = E", enemyChar, enemyMachine);
        check(enemyHit.affectTeam == 2, "affectteam = E should be Enemy (2)");
        af::Character bothChar;
        af::StateMachine bothMachine;
        const af::StateMachine::HitDef bothHit =
            armDef("affectteam = B", bothChar, bothMachine);
        check(bothHit.affectTeam == 3, "affectteam = B should be Both (3)");
        af::Character lowerChar;
        af::StateMachine lowerMachine;
        const af::StateMachine::HitDef lowerHit =
            armDef("affectteam = f", lowerChar, lowerMachine);
        check(lowerHit.affectTeam == 1, "affectteam is case-insensitive");
        af::Character junkChar;
        af::StateMachine junkMachine;
        const af::StateMachine::HitDef junkHit =
            armDef("affectteam = Q", junkChar, junkMachine);
        check(junkHit.affectTeam == 2, "an unknown affectteam leaves the Enemy default");

        af::Character namedChar;
        af::StateMachine namedMachine;
        const af::StateMachine::HitDef named = armDef("id = 5", namedChar, namedMachine);
        check(named.id == 5, "`id = 5` should name the hit 5");

        af::Character linkChar;
        af::StateMachine linkMachine;
        const af::StateMachine::HitDef links = armDef("chainID = 5", linkChar, linkMachine);
        check(links.chainId == 5, "`chainID = 5` should ask for the victim's last hit to be 5");

        af::Character pairChar;
        af::StateMachine pairMachine;
        const af::StateMachine::HitDef pair = armDef("nochainID = 3, 7", pairChar, pairMachine);
        check(pair.noChainId == 3 && pair.noChainId2 == 7,
              "`nochainID = 3, 7` should forbid both, in order");
        // One value given leaves the second at the "nothing forbidden" default
        // rather than at 0, which would forbid a hit with id 0 for free.
        af::Character singleChar;
        af::StateMachine singleMachine;
        const af::StateMachine::HitDef single = armDef("nochainID = 3", singleChar, singleMachine);
        check(single.noChainId == 3 && single.noChainId2 == -1,
              "`nochainID = 3` should leave the second slot unrestricted");

        // The contradiction: requiring an id that is also forbidden. MUGEN clears
        // the whole family rather than choosing, so the hit becomes unrestricted.
        af::Character clashChar;
        af::StateMachine clashMachine;
        const af::StateMachine::HitDef clash =
            armDef("chainID = 5\nnochainID = 5", clashChar, clashMachine);
        check(clash.chainId == -1 && clash.noChainId == -1 && clash.noChainId2 == -1,
              "a chainID that its own nochainID forbids should clear all three");

        // And below -1 clamps up, so a character cannot name an id no hit carries.
        af::Character lowChar;
        af::StateMachine lowMachine;
        const af::StateMachine::HitDef low = armDef("chainID = -4", lowChar, lowMachine);
        check(low.chainId == -1, "a chainID below -1 should clamp to -1");

        // --- the gate, against a victim whose history is known ---

        af::Character victimChar;
        victimChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                 "[Statedef -1]\n");
        af::StateMachine victim;
        victim.setup(&victimChar, -1);
        victim.setOpponent(&dummy);
        victim.setPosition(360.0f);
        victim.setStageBounds(90.0f, 550.0f);

        af::Character needsFiveChar;
        af::StateMachine needsFiveMachine;
        const af::StateMachine::HitDef needsFive =
            armDef("chainID = 5", needsFiveChar, needsFiveMachine);
        af::Character forbidsFiveChar;
        af::StateMachine forbidsFiveMachine;
        const af::StateMachine::HitDef forbidsFive =
            armDef("nochainID = 5", forbidsFiveChar, forbidsFiveMachine);

        check(!af::StateMachine::hitMayReach(needsFive, victim),
              "a chainID link should not connect while nothing has hit the victim");
        // The control: a HitDef asking nothing still connects, which is what makes
        // the check above about the gate and not about the predicate refusing all.
        check(af::StateMachine::hitMayReach(plain, victim),
              "a HitDef with no chain parameters should still reach");

        // A hit that carries id 5. Built directly rather than swung, because what is
        // under test is the history this leaves, not how the swing produced it.
        af::StateMachine::HitInfo opener;
        opener.damage = 0;
        opener.attackerFacing = 1;
        opener.hitTime = 60;
        opener.stateNumber = af::CommonState::kHitHigh;
        opener.id = 5;
        check(victim.applyHit(opener), "the opener lands");
        check(af::StateMachine::hitMayReach(needsFive, victim),
              "and afterwards the chainID link connects, having got what it waited for");
        check(!af::StateMachine::hitMayReach(forbidsFive, victim),
              "while a nochainID for that same id is refused");

        // A block is not a hit, so it must not establish the id either. This is the
        // difference between "the victim was last hit by id 5" and "the victim was
        // last attacked with id 5", and the parameter asks the former.
        af::Character blockerChar;
        blockerChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                  "[Statedef -1]\n");
        af::StateMachine blocker;
        blocker.setup(&blockerChar, -1);
        blocker.setOpponent(&dummy);
        blocker.setPosition(360.0f);
        blocker.setStageBounds(90.0f, 550.0f);
        af::StateMachine::HitInfo blocked;
        blocked.guarded = true;
        blocked.damage = 0;
        blocked.hitTime = 10;
        blocked.stateNumber = af::CommonState::kGuarded;
        blocked.id = 5;
        check(blocker.applyHit(blocked), "a block is taken");
        check(!af::StateMachine::hitMayReach(needsFive, blocker),
              "but a blocked attack does not open a chainID link, because it did not hit");
    }

    // ------------------------------------------- the vertical knockback frame
    //
    // A velocity written in a character's file has positive y *downward*, and this
    // engine keeps height above the floor, so the parser flips the sign the way
    // every other velocity in the engine already flips it. That one sign is the
    // whole of whether knockback works at all: a launcher is written
    // `ground.velocity = -8,-4`, and read without the flip it arrives as a
    // *downward* 4 -- so the victim never leaves the ground and `launches`, which
    // reads a positive y as "off the feet", is never once true for any move in any
    // character file. These parse real HitDefs out of a real character and then ask
    // a victim what it actually got, because a sign error of this kind is invisible
    // in the parse alone and obvious in the reaction.
    // Arms a HitDef out of a character whose attack state carries the given
    // parameter lines, and hands back the machine so the reaction can be read as
    // well as the parse. Shared by the knockback block below and by the one after
    // it, which is why it lives out here rather than inside either.
    const auto swing = [&](const std::string& velocityLines, af::Character& chr,
                           af::StateMachine& machine) {
        chr.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
            "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n"
            "damage = 20\n" + velocityLines + "\n",
            "[Statedef -1]\n");
        machine.setup(&chr, 1);
        machine.setOpponent(&dummy);
        machine.setPosition(320.0f);
        machine.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 20 && !machine.hasArmedHitDef(); ++i) {
            af::InputState tick;
            machine.control(tick);
            machine.step();
        }
    };

    // An idle victim on the ground, for an attacker to hit.
    const auto makeIdle = [&](af::Character& chr, af::StateMachine& machine) {
        chr.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                          "[Statedef -1]\n");
        machine.setup(&chr, -1);
        machine.setOpponent(&dummy);
        machine.setPosition(360.0f);
        machine.setStageBounds(90.0f, 550.0f);
    };

    // The same one height down. `type = C` in the statedef sets the state type, and
    // holding down keeps it there: a fighter whose file says crouch but whose input
    // is not holding down stands back up on the next tick.
    const auto makeCrouch = [&](af::Character& chr, af::StateMachine& machine) {
        chr.loadBehaviour("[Statedef 0]\ntype = C\nphysics = C\nctrl = 1\n",
                          "[Statedef -1]\n");
        machine.setup(&chr, -1);
        machine.setOpponent(&dummy);
        machine.setPosition(360.0f);
        machine.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 4; ++i) {
            af::InputState heldDown;
            heldDown.held = af::Button::kDown;
            machine.control(heldDown);
            machine.step();
        }
    };

    {
        // The launcher as the character files write it: the up-velocity is in the
        // *ground* pair, which is the parameter whose non-zero y is what means
        // "knock into the air", with the air pair repeating it for the follow-up.
        af::Character launcherChar;
        af::StateMachine launcher;
        swing("ground.velocity = -8,-4\nair.velocity = -8,-4", launcherChar, launcher);
        const af::StateMachine::HitDef lift = launcher.armedHitDef();
        check(lift.pushY > 0.5f, "a launcher's ground y should read as upward, not downward");
        check(lift.airPushY > 0.5f, "and its air y likewise");
        check(lift.launches, "so the HitDef reports that it takes the victim off their feet");

        af::Character standingChar;
        af::StateMachine standing;
        makeIdle(standingChar, standing);
        const af::StateMachine::HitInfo launched = launcher.hitInfoFor(standing);
        check(launched.air, "and a standing victim is hit into the air rather than in place");
        check(launched.falls, "with the hit reporting itself as a fall, which is its own question");
        // 5000, not 5030. The stance picks the state and the fall flag rides along
        // inside it; that is the reference's single ground branch, and going
        // straight to 5030 from here is what used to make every character's own
        // `[Statedef 5000]` dead code on a launcher.
        check(launched.stateNumber == af::CommonState::kHitHigh,
              "into 5000, the standing ground reaction, which is where a launch reaction is "
              "written -- 5030 is two states further on");
        check(launched.pushY > 0.5f, "carrying a velocity that takes them up");
        check(standing.applyHit(launched), "the launch lands");
        check(standing.velY() > 0.5f, "and the victim is actually moving upward");

        // The control that keeps the fix honest. A sign flip on a zero is invisible,
        // so a hit with no vertical velocity at all is what separates "the frame is
        // converted" from "everything now launches".
        af::Character jabberChar;
        af::StateMachine jabber;
        swing("ground.velocity = -4\nair.velocity = -2", jabberChar, jabber);
        const af::StateMachine::HitDef jab = jabber.armedHitDef();
        check(!jab.launches, "a hit with no vertical velocity should not launch");
        af::Character groundedChar;
        af::StateMachine grounded;
        makeIdle(groundedChar, grounded);
        const af::StateMachine::HitInfo stayed = jabber.hitInfoFor(grounded);
        check(!stayed.air, "so its victim stays on the ground");
        check(stayed.stateNumber == af::CommonState::kHitHigh,
              "in the standing hit state rather than a fall");
        check(stayed.pushY == 0.0f, "with no vertical velocity at all");
        check(grounded.applyHit(stayed), "the jab lands");
        check(grounded.velY() == 0.0f, "and leaves the victim level");

        // The same for a HitDef that writes no velocities whatsoever, which is how
        // most moves in most files are written. `air.velocity` defaults to nothing,
        // so a default that had drifted upward would show up right here: it would
        // make every plain hit in the game launch.
        af::Character bareChar;
        af::StateMachine bare;
        swing("", bareChar, bare);
        const af::StateMachine::HitDef none = bare.armedHitDef();
        check(!none.launches, "a HitDef that writes no velocity at all should not launch");
        af::Character bareVictimChar;
        af::StateMachine bareVictim;
        makeIdle(bareVictimChar, bareVictim);
        const af::StateMachine::HitInfo plainHit = bare.hitInfoFor(bareVictim);
        check(!plainHit.air && plainHit.stateNumber == af::CommonState::kHitHigh,
              "and its victim is hit standing, as a plain jab should be");

        // A *downward* ground y is the case the non-zero test exists for, and the one a
        // "greater than zero" test gets wrong. It is a slam rather than a launch, and
        // the corpus is full of them: 96 of its ground velocities write a downward y,
        // 89 of those alongside `fall = 1` -- which is what a character writes when it
        // is not sure the engine will take the hint. Reading only "upward" left the rest
        // of them standing with a velocity into the floor, which the floor absorbed.
        af::Character slamChar;
        af::StateMachine slammer;
        // The two pairs are written to *differ*, so that a reaction carrying one can be
        // told from a reaction carrying the other. Same numbers in both is how this
        // was hidden: most characters repeat the ground pair into the air pair, so the
        // wrong one is invisible on those files and shows up only on the sloppy ones.
        swing("ground.velocity = -6,10\nair.velocity = -3,4", slamChar, slammer);
        const af::StateMachine::HitDef slam = slammer.armedHitDef();
        check(std::fabs(slam.pushX - slam.airPushX) > 1.0f,
              "the two pairs in this HitDef should differ, or the check below proves nothing");
        check(slam.pushY < -0.5f, "a slam's ground y should read as downward, not upward");
        check(slam.pushY != 0.0f && !slam.fall,
              "and it is written non-zero without a `fall` to force it");

        af::Character slammedChar;
        af::StateMachine slammed;
        makeIdle(slammedChar, slammed);
        const af::StateMachine::HitInfo slammedInfo = slammer.hitInfoFor(slammed);
        check(slammedInfo.air, "so a downward y should still take the victim off the ground");
        check(slammedInfo.falls, "and read as a fall, which is how a slam is written as well as a launch");
        check(slammedInfo.stateNumber == af::CommonState::kHitHigh,
              "into the standing ground reaction, the same one a launch takes");
        check(std::fabs(slammedInfo.pushX - slam.pushX) < 0.01f &&
                  std::fabs(slammedInfo.pushY - slam.pushY) < 0.01f,
              "and carrying the ground velocity it was hit with, not the air one");
        check(slammedInfo.pushY < -0.5f, "which for a slam points at the floor");

        // The control on the other side of the rule: a y written *as* zero is zero, and
        // must not become a knockdown -- which is exactly what a test of "was a y given"
        // rather than "is the y non-zero" would do, and the difference matters because
        // half the corpus' ground velocities are single values with no y at all.
        af::Character flatChar;
        af::StateMachine flat;
        swing("ground.velocity = -6,0\nair.velocity = -3,4", flatChar, flat);
        const af::StateMachine::HitDef flatDef = flat.armedHitDef();
        af::Character flatVictimChar;
        af::StateMachine flatVictim;
        makeIdle(flatVictimChar, flatVictim);
        const af::StateMachine::HitInfo flatInfo = flat.hitInfoFor(flatVictim);
        check(!flatInfo.air, "an explicit zero y should not take the victim off the ground");
        check(flatInfo.stateNumber == af::CommonState::kHitHigh, "and leaves them standing");
        check(std::fabs(flatInfo.pushY) < 0.01f, "with no vertical velocity");
        check(std::fabs(flatInfo.pushX - flatDef.pushX) < 0.01f,
              "and the ground x, which is the pair a ground hit always uses");

        // And `fall = 1` is the explicit form of the same thing -- a knockdown with a
        // flat ground velocity -- which the non-zero test must not have broken, and
        // which is how a move that knocks down with no up-push at all is written.
        af::Character forcedChar;
        af::StateMachine forced;
        swing("ground.velocity = -6,0\nfall = 1", forcedChar, forced);
        af::Character forcedVictimChar;
        af::StateMachine forcedVictim;
        makeIdle(forcedVictimChar, forcedVictim);
        const af::StateMachine::HitInfo forcedInfo = forced.hitInfoFor(forcedVictim);
        check(forcedInfo.air && forcedInfo.falls &&
                  forcedInfo.stateNumber == af::CommonState::kHitHigh,
              "`fall = 1` still knocks the victim down on its own, on a flat velocity");

        // An airborne victim takes the *air* pair, so the same conversion has to
        // reach there too -- the half a ground-only check would miss.
        af::Character airJabChar;
        af::StateMachine airJab;
        swing("ground.velocity = -4\nair.velocity = -2,-4", airJabChar, airJab);
        af::Character airChar;
        af::StateMachine air;
        makeIdle(airChar, air);
        af::StateMachine::HitInfo aloft;
        aloft.damage = 0;
        aloft.attackerFacing = 1;
        aloft.pushX = 0.0f;
        aloft.pushY = 8.0f;
        aloft.hitTime = 120;  // long enough that the victim is still up a few ticks on
        aloft.juggleCost = 0;
        aloft.stateNumber = af::CommonState::kHitAir;
        air.applyHit(aloft);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            air.control(tick);
            air.step();
        }
        check(air.y() > 0.5f, "the victim for the airborne check should be off the ground");
        const af::StateMachine::HitInfo airHit = airJab.hitInfoFor(air);
        check(airHit.air, "an airborne victim is hit in the air");
        check(airHit.pushY > 0.5f, "and takes the air velocity, upward once converted");
        // The air branch writes its number before it looks at anything else -- the
        // reference's `v12->field_c4c = 5020;` with `air.type = 3` its only override
        // -- so 5020 is where every airborne victim goes, whatever the hit intends
        // to do to them afterwards. 5030 is one state further on, reached from the
        // *character's* 5020 on `HitShakeOver`, which is why a character's own
        // `[Statedef 5020]` is reachable at all.
        check(airHit.stateNumber == af::CommonState::kHitAir,
              "into 5020, the air shake, and not straight past it to 5030");

        // The other half of that switch: the same airborne victim under a move
        // that *does* claim the fall. `air.fall` is the air pair's own `fall`, and
        // the reference's `GetHitVar(fall)` for an air hit is exactly this -- which
        // is why 5020 hands over to 5030 on `HitShakeOver` unconditionally. It is a
        // separate switch from the ground pair's and, per the corpus, written far
        // less often: `air.fall = 1` is how a move that juggles says so.
        af::Character juggleChar;
        af::StateMachine juggle;
        swing("ground.velocity = -4\nair.fall = 1", juggleChar, juggle);
        const af::StateMachine::HitDef jug = juggle.armedHitDef();
        check(jug.airFall, "an `air.fall = 1` should reach the HitDef");
        check(!jug.launches, "and it is not a launch, which is the ground pair's question");
        const af::StateMachine::HitInfo juggled = juggle.hitInfoFor(air);
        check(juggled.air && juggled.falls,
              "so the same airborne victim is knocked down by this one");
        check(juggled.stateNumber == af::CommonState::kHitAir,
              "though still into 5020: the flag decides what 5020 hands over to, not which "
              "state the engine picks");

        // `air.fall` is documented as defaulting "to the same value as fall", and the
        // binary agrees with the shape of that: the fall bit is written once, by the
        // `fall` parameter alone, with no implicit derivation beside the one that
        // `forcestand` gets (`angr_0047f380_at_00480308.c:305`), while `air.fall` is a
        // separate slot read inside the air branch. So a move that says `fall = 1`
        // says it for an airborne victim too, and reads one intent rather than two.
        af::Character fallOnlyChar;
        af::StateMachine fallOnly;
        swing("ground.velocity = -4\nfall = 1", fallOnlyChar, fallOnly);
        check(fallOnly.armedHitDef().airFall,
              "a `fall = 1` with no air pair should reach an airborne victim as well");

        af::Character airOffChar;
        af::StateMachine airOff;
        swing("ground.velocity = -4\nfall = 1\nair.fall = 0", airOffChar, airOff);
        check(!airOff.armedHitDef().airFall, "and a written `air.fall = 0` should reach it as 0");
        check(airOff.hitInfoFor(air).falls,
              "but the flag itself stands anyway: the prelude tests `fall` before anything "
              "else, so `air.fall = 0` can only take back the *default*");

        // --- `forcestand` and `forcenofall`: the two parameters that take it back
        //
        // Both are read *before* the branch that picks the reaction, and both exist to
        // undo something the engine does on its own. `forcestand` moves a crouching
        // victim up into the standing reaction -- 5000 in place of 5010 -- and defaults
        // to 1 whenever `ground.velocity`'s y is non-zero, which is why a launcher
        // written with nothing but velocities stands a croucher up without being asked.
        // `forcenofall` takes the fall back off a victim who is already in one, and it
        // is the *only* thing that can: every other term in the reaction either sets
        // the flag or leaves it alone.
        af::Character crouchChar;
        af::StateMachine croucher;
        makeCrouch(crouchChar, croucher);
        check(croucher.stateType() == af::StateType::Crouch,
              "the victim for the crouching checks should actually be crouching");

        // The control first. Without it every check below could pass on an engine that
        // had stopped telling the two heights apart at all -- a crouching victim always
        // in 5000 would satisfy "a launcher stands a croucher up" for the wrong reason.
        check(jabber.hitInfoFor(croucher).stateNumber == af::CommonState::kHitLow,
              "an ordinary flat hit should leave a crouching victim in the crouching reaction");

        // The implicit default, which is the whole reason the parameter is worth
        // having: `lift` writes no `forcestand` at all, only an upward ground y.
        check(lift.forceStand,
              "a launcher's non-zero ground y should default `forcestand` on, unasked");
        check(launcher.hitInfoFor(croucher).stateNumber == af::CommonState::kHitHigh,
              "so its crouching victim is stood up into the standing reaction anyway");

        // And the explicit form, on a hit that has nothing else to say.
        af::Character standForceChar;
        af::StateMachine standForce;
        swing("ground.velocity = -4\nforcestand = 1", standForceChar, standForce);
        check(standForce.armedHitDef().forceStand, "`forcestand = 1` should parse");
        check(standForce.hitInfoFor(croucher).stateNumber == af::CommonState::kHitHigh,
              "and stand a crouching victim up on a hit that is otherwise flat");

        // The other direction, which is what makes it a default rather than a rule: a
        // file that spells the parameter out wins, even when the y would have turned it
        // on by itself. This is the case the "was it written" flag exists for.
        af::Character keepCrouchChar;
        af::StateMachine keepCrouch;
        swing("ground.velocity = -8,-4\nair.velocity = -8,-4\nforcestand = 0", keepCrouchChar,
              keepCrouch);
        const af::StateMachine::HitDef noStand = keepCrouch.armedHitDef();
        check(noStand.forceStand == false,
              "a written `forcestand = 0` should beat the launcher default beside it");
        check(noStand.pushY > 0.5f, "even though the velocity that default reads is right there");
        check(keepCrouch.hitInfoFor(croucher).stateNumber == af::CommonState::kHitLow,
              "so a crouching victim stays down in the crouching reaction, in the air as well");

        // Now the fall. A victim who is already falling keeps falling: the prelude
        // writes the flag only for `fall`, and its `else` branch deliberately writes
        // nothing, so the victim's own value survives the hit. Before this the engine
        // recomputed the flag from the new HitDef alone, and a victim juggled out of a
        // knockdown got up off the floor in mid-air.
        //
        // Two shapes of "already falling", because a knockdown spends most of its life
        // in the first: a victim in a fall at height zero, still in the ground reaction
        // and about to be handed on to the fall states, and a victim actually up in the
        // air. The first is the one the ground branch reads and the second the air
        // branch, and they are separate expressions.
        const auto knockDown = [&](af::Character& chr, af::StateMachine& machine, bool aloft) {
            makeIdle(chr, machine);
            af::StateMachine::HitInfo reaction;
            reaction.damage = 0;
            reaction.attackerFacing = 1;
            reaction.pushX = 0.0f;
            reaction.pushY = aloft ? 8.0f : 0.0f;
            reaction.hitTime = 120;  // long enough that a few ticks do not time it out
            reaction.juggleCost = 0;
            reaction.falls = true;
            reaction.stateNumber =
                aloft ? af::CommonState::kHitAir : af::CommonState::kHitHigh;
            machine.applyHit(reaction);
            if (!aloft) return;
            for (int i = 0; i < 4; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
        };

        af::Character floorChar;
        af::StateMachine floored;
        knockDown(floorChar, floored, false);
        check(floored.hitFall() && floored.stateType() == af::StateType::Air &&
                  floored.y() <= 0.5f,
              "the knockdown for the ground carry check should leave the victim falling at "
              "height zero -- `[State 5000, 5]` turns the fall airborne -- which is what a "
              "knockdown is between the reaction and the floor");

        af::Character airCarryChar;
        af::StateMachine airCarry;
        knockDown(airCarryChar, airCarry, true);
        check(airCarry.hitFall() && airCarry.stateType() == af::StateType::Air,
              "and the one for the air carry check should leave the victim up in the air");

        af::Character clearChar;
        af::StateMachine clear;  // the control: not falling at all
        makeIdle(clearChar, clear);

        // A flat air pair, so that the HitDef itself has no opinion about the fall: an
        // upward one would take the victim off their feet on its own, and the carry
        // would be invisible behind it. `launches` is the term to keep false here.
        af::Character airFlatChar;
        af::StateMachine airFlat;
        swing("ground.velocity = -4\nair.velocity = -2,0", airFlatChar, airFlat);
        const af::StateMachine::HitDef airPlain = airFlat.armedHitDef();
        check(!airPlain.launches, "the hit for the carry checks should not launch on its own");

        check(airFlat.hitInfoFor(floored).falls,
              "so a hit that says nothing leaves a knocked-down victim in the fall -- and this "
              "is the one that used to send them back to their feet mid-air");
        check(airFlat.hitInfoFor(airCarry).falls,
              "and leaves one already in the air falling as well, on the other branch");
        check(!airFlat.hitInfoFor(clear).falls,
              "while the same hit on a victim who is not falling takes them nowhere at all");

        // `forcenofall` is the one hit allowed to say otherwise -- it is what the
        // parameter is for -- and it has to reach the *carry* and nothing else. A move
        // that writes `forcenofall = 1` beside its own `fall = 1` has not un-knocked-down
        // anybody.
        af::Character releaseChar;
        af::StateMachine release;
        swing("ground.velocity = -4\nair.velocity = -2,0\nforcenofall = 1", releaseChar, release);
        check(release.armedHitDef().forceNoFall, "`forcenofall = 1` should parse");
        check(!release.hitInfoFor(floored).falls && !release.hitInfoFor(airCarry).falls,
              "and it is the one hit that takes an existing fall away, on both branches");
        check(airFlat.hitInfoFor(floored).falls,
              "and the parameter belongs to the hit that wrote it: the ordinary hit beside it "
              "still carries the same falling victim");

        af::Character releaseForcedChar;
        af::StateMachine releaseForced;
        swing("ground.velocity = -6,0\nfall = 1\nforcenofall = 1", releaseForcedChar,
              releaseForced);
        check(releaseForced.hitInfoFor(floored).falls,
              "but a `fall = 1` beside it stands even on the same falling victim: the prelude "
              "tests `fall` before `forcenofall`, so the parameter can only take back a flag "
              "the hit did not set itself");

        // And `air.type`, not `ground.type`, is what overrides the air branch --
        // `if (v11->field_13c == 3) v12->field_c4c = 5070;` right beside it. 5070 is
        // the tripped shake, and it is the same number the ground branch reaches by
        // its own `ground.type = 3`.
        af::Character tripChar;
        af::StateMachine trip;
        swing("ground.velocity = -4\nair.type = Trip", tripChar, trip);
        const af::StateMachine::HitInfo tripped = trip.hitInfoFor(air);
        check(tripped.air && tripped.stateNumber == af::CommonState::kHitTrip,
              "an airborne victim under an `air.type = 3` takes 5070, the tripped reaction");

        // `gethitvar(yvel)` answers in the character's own frame, where down is
        // positive -- the same frame the velocity was written in -- so what the
        // engine stores has to be flipped back on the way out. Without that, a
        // character reading its own knockback to decide how long to stay down
        // reads it upside down.
        check(standing.trigger("gethitvar", {af::Value::text("yvel")}).toFloat() < 0.0f,
              "gethitvar(yvel) should report an upward launch as a negative y");

        // --- `GetHitVar(fall)` is the hit's verdict, not a reading of the state
        //
        // It is the flag the reference's own get-hit states split on -- `[State
        // 5000, 6]` sends a victim whose `GetHitVar(fall)` is false to 5001 and lets
        // the rest fall through to 5030; `[State 5000, 5]` changes the statetype on
        // it; `[State 5080, 5]` chooses between going back into the air and sliding
        // along the floor on it. A character that reads a wrong answer there takes
        // the wrong branch, so it is worth pinning in both directions.
        //
        // The two questions are genuinely different and answering this one out of
        // `air` -- which is what the engine did -- makes them the same: a jab that
        // catches a jumping victim is an air hit and not a knockdown, and reading
        // `fall` off `air` told that character it was falling.
        {
            af::Character plainChar;
            af::StateMachine plain;
            swing("ground.velocity = -4\nair.velocity = -2", plainChar, plain);
            af::Character plainVictimChar;
            af::StateMachine plainVictim;
            makeIdle(plainVictimChar, plainVictim);
            plainVictim.applyHit(plain.hitInfoFor(plainVictim));
            check(plainVictim.trigger("gethitvar", {af::Value::text("fall")}).toFloat() == 0.0f,
                  "a plain ground hit is not a fall");

            // The same velocity, caught in the air: still not a fall. This is the
            // half an `air`-derived answer gets wrong -- `air` is true here and the
            // hit is still not a knockdown.
            af::Character airVictimChar;
            af::StateMachine airVictim;
            makeIdle(airVictimChar, airVictim);
            af::StateMachine::HitInfo aloft;
            aloft.damage = 0;
            aloft.attackerFacing = 1;
            aloft.pushX = 0.0f;
            aloft.pushY = 6.0f;
            aloft.hitTime = 120;
            aloft.air = true;
            aloft.stateNumber = af::CommonState::kHitAir;
            aloft.victimPause = 1;
            check(airVictim.applyHit(aloft), "the air hit lands");
            check(airVictim.trigger("gethitvar", {af::Value::text("fall")}).toFloat() == 0.0f,
                  "and even so it is not a fall: an air hit is not a knockdown");

            af::Character launchChar;
            af::StateMachine launch;
            swing("ground.velocity = -6,-5", launchChar, launch);
            af::Character launchVictimChar;
            af::StateMachine launchVictim;
            makeIdle(launchVictimChar, launchVictim);
            launchVictim.applyHit(launch.hitInfoFor(launchVictim));
            check(launchVictim.trigger("gethitvar", {af::Value::text("fall")}).toFloat() != 0.0f,
                  "while a launch is one, which is what sends it to the fall rather than the "
                  "flat knock-back");
        }
    }

    // ---------------------------------------- the hit values a victim reads back
    //
    // `GetHitVar` is how a character's own hit states learn what hit them, and
    // three keys the corpus leans on were answered 0 because the value was parsed
    // but never exposed: the fall's bounce velocity, the guard's control time and
    // the fall's damage. A key that reads 0 is not a missing feature to the
    // character -- it is a wrong answer it acts on. `fall.yvel` is read only as
    // `= 0`/`!= 0` (130 uses across 41 packs in local_chars) to choose between the
    // bounce and slide branches in the 5100/5110 ground states, and `ctrltime`
    // (126 uses across 42 packs) is the length of a block-stun, so a 0 there is a
    // block that ends on its first frame.
    {
        af::Character strikeChar;
        af::StateMachine strike;
        // One HitDef carrying the whole family, so each key can be read back from
        // a single landing.
        swing("fall.yvelocity = -4.5\nfall.damage = 30\nguard.ctrltime = 9\nguard.hittime = 15",
              strikeChar, strike);
        const af::StateMachine::HitDef strikeDef = strike.armedHitDef();
        // The parse first, so a wrong trigger can be told from a wrong HitDef.
        check(std::fabs(strikeDef.fallVelY - 4.5f) < 0.01f,
              "`fall.yvelocity = -4.5` should reach the engine's upward-positive frame");
        check(strikeDef.fallDamage == 30, "and `fall.damage` should parse");
        check(strikeDef.guardCtrlTime == 9, "and `guard.ctrltime` should parse");
        check(strikeDef.guardHitTime == 15, "and `guard.hittime` with it");
        // The control for the damage fix: a HitDef with only the hit component
        // (which `swing` writes as `damage = 20`) has no chip damage at all.
        check(strikeDef.guardDamage == 0,
              "a `damage` with one component should leave guard damage at zero");

        // An idle victim takes the hit unblocked, and reads the fall values back.
        af::Character openChar;
        af::StateMachine open;
        makeIdle(openChar, open);
        const af::StateMachine::HitInfo landed = strike.hitInfoFor(open);
        check(!landed.guarded, "an idle victim does not block");
        check(open.applyHit(landed), "the hit lands");
        check(open.trigger("gethitvar", {af::Value::text("fall.damage")}).toFloat() == 30.0f,
              "gethitvar(fall.damage) should report the fall's damage, not 0");
        // The sign pinned: `fall.yvelocity` is written *upward* (negative in the
        // character's frame, the way a bounce goes), and `fall.yvel` must answer in
        // that same frame -- so it reads negative too, and a `= 0` test still
        // separates a bounce from a slide without ever caring about the sign.
        const float fallYVel =
            open.trigger("gethitvar", {af::Value::text("fall.yvel")}).toFloat();
        check(fallYVel < 0.0f, "gethitvar(fall.yvel) should report an upward bounce as negative");
        check(std::fabs(fallYVel + 4.5f) < 0.01f,
              "and with the magnitude the character wrote, flipped out of the engine frame");
        // The two keys that are not hit values at all: the reference defines
        // `xveladd`/`yveladd` as the extra velocity added on a KO, which the engine
        // does not carry, so they answer 0 rather than the victim's live velocity.
        check(open.trigger("gethitvar", {af::Value::text("xveladd")}).toFloat() == 0.0f,
              "gethitvar(xveladd) should be 0, not the victim's own velocity");
        check(open.trigger("gethitvar", {af::Value::text("yveladd")}).toFloat() == 0.0f,
              "and so should gethitvar(yveladd)");
        // `hitshakeover` is not a GetHitVar key; the bogus case is gone, so it
        // reads 0 like any unknown key rather than a second, differently-sourced
        // answer. (The standalone trigger of that name is a different code path.)
        check(open.trigger("gethitvar", {af::Value::text("hitshakeover")}).toFloat() == 0.0f,
              "GetHitVar(hitshakeover) is not a key and reads 0 like any unknown one");

        // A blocker holding back reads its guard control time back.
        af::Character braceChar;
        braceChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                "[Statedef -1]\n");
        af::StateMachine brace;
        brace.setup(&braceChar, -1);
        brace.setOpponent(&strike);
        brace.setPosition(360.0f);
        brace.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            // The blocker faces left, so "back" is the right-hand key.
            tick.held = af::Button::kRight;
            brace.control(tick);
            brace.step();
        }
        check(brace.holdingBack(), "a defender facing left holds back with the right key");
        const af::StateMachine::HitInfo bracedInfo = strike.hitInfoFor(brace);
        check(bracedInfo.guarded, "and the hit is blocked");
        check(brace.applyHit(bracedInfo), "the block lands");
        check(brace.trigger("gethitvar", {af::Value::text("ctrltime")}).toFloat() == 9.0f,
              "gethitvar(ctrltime) should report the guard's control time, not 0");
        check(brace.trigger("gethitvar", {af::Value::text("hittime")}).toFloat() == 15.0f,
              "and gethitvar(hittime) the guard's hit time, which is a different number "
              "on this HitDef: `guard.ctrltime = 9` is how long the blocker cannot act, "
              "`guard.hittime = 15` how long it is held");
        check(bracedInfo.hitTime == 15,
              "which is also how long the engine holds the blocker");
    }

    // ------------------------------------- the hits a victim has taken this combo
    //
    // `GetHitVar(hitcount)` is the *victim's* count of hits taken in the combo it
    // is currently in -- not the attacker's `hitcount` trigger, which counts a
    // different thing and is read from the other side of the fight. The
    // reference's own documentation says so plainly ("Returns the number of hits
    // taken by the player in current combo"), and the corpus reads it that way 62
    // times across 7 packs, in two families that both need the victim's number:
    // an attacker scaling its damage by the punishment it has already dealt
    // (`Enemy,GetHitVar(HitCount)` -- Kira, Giorno, Vegeta) and a victim gating
    // its own behaviour while it is being hit (Vegetto, Diavolo, Jolyne, Mista).
    //
    // It answered a flat 0 before this, which is the dangerous kind of wrong
    // answer: damage scaling that multiplies by zero, or a combo counter that
    // never leaves its first hit. The rules that make it a combo counter rather
    // than a hit counter are all tested here, because any one of them alone
    // gives a plausible-looking number: a hit that lands on a fighter who was not
    // *already* in a hit reaction starts a new combo -- but only once it has been
    // out of one for more than a single frame, since one free frame between two
    // hits is a link and not a fresh start -- and a blocked hit is not a hit at
    // all.
    {
        af::Character strikeChar;
        af::StateMachine strike;
        swing("", strikeChar, strike);

        af::Character openChar;
        af::StateMachine open;
        makeIdle(openChar, open);
        const auto hitsTaken = [&] {
            return open.trigger("gethitvar", {af::Value::text("hitcount")}).toFloat();
        };

        check(hitsTaken() == 0.0f, "a fighter who has not been hit has taken no hits");

        // Three hits in a row, with no recovery between them: the victim is still
        // in the hit reaction each time, so this is one combo and the count runs.
        check(open.applyHit(strike.hitInfoFor(open)), "the first hit of the combo lands");
        check(hitsTaken() == 1.0f, "and counts as the first hit taken");
        check(open.applyHit(strike.hitInfoFor(open)), "the second hit lands");
        check(hitsTaken() == 2.0f, "and the count advances");
        check(open.applyHit(strike.hitInfoFor(open)), "the third hit lands");
        check(hitsTaken() == 3.0f, "and again -- a combo is counted, not just detected");

        // The attacker's own `hitcount` is a different number, and this is the
        // case that proves they are not the same field: the victim is on its third
        // hit and the attacker, which has connected with nothing on this path, is
        // still on zero. Answering one from the other would look right in a
        // single-hit test and be wrong in exactly the situation the corpus uses it.
        check(strike.trigger("hitcount", {}).toFloat() == 0.0f,
              "the attacker's own hitcount should not move when the victim is hit");
        check(open.trigger("hitcount", {}).toFloat() == 0.0f,
              "and the victim's standalone hitcount trigger is not the same thing either");

        // And the reading the corpus actually performs: the *attacker* asking for
        // the victim's count through a redirect, which is how the three
        // damage-scaling characters are written (`Enemy,GetHitVar(HitCount)`).
        // That sentence is parsed and evaluated here as a real expression rather
        // than assembled out of calls, because the redirect is the part that could
        // quietly answer zero -- it has to reach the opponent and then re-enter the
        // hit-var lookup *on the opponent*, and a redirect that resolves to the
        // attacker itself would give a perfectly plausible 0 and scale no damage
        // for the rest of the match.
        strike.setOpponent(&open);
        const af::Expr acrossTheFight = af::Expr::parse("enemy, gethitvar(hitcount)");
        check(acrossTheFight.valid(), "the corpus's `Enemy,GetHitVar(HitCount)` should parse");
        check(acrossTheFight.eval(strike).toFloat() == 3.0f,
              "and the attacker should read the victim's combo count through the redirect");
        check(af::Expr::parse("enemy, gethitvar(hitcount)").eval(open).toFloat() == 0.0f,
              "while the same line read from the victim looks at the attacker, who has taken none");
        strike.setOpponent(&dummy);

        // One free frame still counts as the same combo. The victim walks back out
        // of its hit states, and the step that takes it out *is* its first frame
        // of freedom: the reference counts frames outside a hit reaction and calls
        // the next hit a new combo only once that count is above one, so a hit
        // landing here is the fourth of this combo, not the first of the next.
        //
        // This is not a corner case. A move whose hitstun and whose victim's
        // recovery land on adjacent ticks leaves exactly one such frame, and every
        // link written against `GetHitVar(HitCount)` is built on those moves
        // chaining. Resetting on the very first free frame would break the count
        // on the most ordinary two-hit sequence there is.
        for (int i = 0; i < 400 && open.moveType() == af::MoveType::Hit; ++i) {
            af::InputState idleTick;
            open.control(idleTick);
            open.step();
        }
        check(open.moveType() != af::MoveType::Hit, "the victim should recover out of the hit reaction");
        check(open.applyHit(strike.hitInfoFor(open)), "a hit on the first free frame lands");
        check(hitsTaken() == 4.0f,
              "and one free frame does not end a combo, so the count continues");

        // Two free frames do. Recover again, then let one more tick of freedom
        // pass before the hit, and the same attack starts the count over at one.
        for (int i = 0; i < 400 && open.moveType() == af::MoveType::Hit; ++i) {
            af::InputState idleTick;
            open.control(idleTick);
            open.step();
        }
        check(open.moveType() != af::MoveType::Hit, "the victim recovers a second time");
        {
            af::InputState idleTick;
            open.control(idleTick);
            open.step();
        }
        check(open.applyHit(strike.hitInfoFor(open)), "a hit after two free frames lands");
        check(hitsTaken() == 1.0f,
              "and it is the first hit of a new combo, not the fifth of the old one");

        // A blocked hit never reaches the damage path, so it never advances the
        // count. Read on a fresh blocker, because the fighter above is mid-combo.
        af::Character guardChar;
        af::StateMachine guard;
        makeIdle(guardChar, guard);
        // Facing matters more than usual here: `holdingBack()` is a question about
        // the direction the fighter is turned, and `makeIdle` leaves the victim
        // facing the shared dummy rather than this attacker. The blocker has to
        // face the striker or its "back" is the wrong key and it blocks nothing.
        guard.setOpponent(&strike);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            // The blocker faces left, so "back" is the right-hand key.
            tick.held = af::Button::kRight;
            guard.control(tick);
            guard.step();
        }
        const af::StateMachine::HitInfo braced = strike.hitInfoFor(guard);
        check(braced.guarded, "the blocker is holding back");
        check(guard.applyHit(braced), "the block lands");
        check(guard.trigger("gethitvar", {af::Value::text("hitcount")}).toFloat() == 0.0f,
              "a blocked hit is not a hit taken, so the combo count stays at zero");
    }

    // ------------------------------------------------------ being held in a throw
    //
    // `GetHitVar(isbound)` answers whether this fighter is currently held by
    // somebody else's bind. The reference defines it as "True if the player is the
    // subject of an attacker's TargetBind controller. Useful to prevent being
    // stuck in thrown states", and that second sentence is why an unanswered key
    // here is worse than a key that is simply absent: an unknown key reads 0, so
    // `trigger1 = !gethitvar(isbound)` -- the guard MUGEN's *own* character writes
    // in its thrown state to fall back to its landing state -- is true on the
    // first tick of every throw. The victim leaves the throw immediately and the
    // throw holds nobody. Both uses in the corpus are that one idiom.
    //
    // Driven through the real path rather than by poking the bind, because the
    // bind is set by the attacker's TargetBind acting on whoever its move last
    // connected with, and "did the connection and the bind actually meet" is the
    // part that could silently not happen.
    {
        // A throw-like attacker: a state that lands a HitDef and then, on the tick
        // after it connects, binds the target to itself for a known number of
        // ticks. The victim scripts nothing, so it has no throw escape of its own
        // and the only thing that can free it is the bind running out.
        af::Character throwerChar;
        throwerChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 300, grab]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NT\n"
            "damage = 5\npausetime = 0,0\n\n"
            "[State 300, hold]\ntype = TargetBind\ntrigger1 = time = 1\n"
            "time = 20\npos = 20,-40\n",
            "[Statedef -1]\n");
        // A swing only connects where the *animation* says it can: `tryHit` reads
        // the Clsn1 boxes of the current frame and refuses the hit on a frame with
        // none, so a character with no animation cannot land a blow however good
        // its HitDef is. Both fighters get real boxes here rather than leaning on
        // the placeholder hurt box a character with no animation falls back to, so
        // what this drives is the same geometry a real match uses.
        //
        // The box block goes *before* the frame it belongs to -- that is the
        // format's real layout, not a quirk of this parser (KFM's own .air opens
        // action 200 with `Clsn2Default:`, then two bare frames, and only then the
        // `Clsn1:` block that frame two carries) -- and putting it after instead
        // attaches the boxes to the *next* frame while leaving this one empty, so
        // the action parses, reports its frame, and quietly swings at nothing.
        af::parseAirText("[Begin Action 0]\n0,0, 0,0, -1\n\n"
                         "[Begin Action 300]\nClsn1: 1\nClsn1[0] = 8,-56,44,-20\n"
                         "0,0, 0,0, -1\n",
                         throwerChar.animations());
        af::Character heldChar;
        heldChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                               "[Statedef -1]\n");
        af::parseAirText("[Begin Action 0]\nClsn2Default: 1\nClsn2[0] = -12,-64,12,0\n"
                         "0,0, 0,0, -1\n",
                         heldChar.animations());

        af::Fighter thrower;
        af::Fighter held;
        thrower.setup(&throwerChar, 300.0f, 1);
        held.setup(&heldChar, 320.0f, -1);
        thrower.setScale(1.0f);
        held.setScale(1.0f);
        const auto isBound = [&] {
            return held.machine().trigger("gethitvar", {af::Value::text("isbound")}).toFloat();
        };
        check(isBound() == 0.0f, "a fighter nobody is holding does not report itself bound");

        const float floorY = 192.0f;
        bool everBound = false;
        int boundTicks = 0;
        int landedTick = -1;
        for (int tick = 0; tick < 120; ++tick) {
            thrower.control(af::InputState{});
            held.control(af::InputState{});
            thrower.step(held);
            held.step(thrower);
            af::Fighter::separate(thrower, held);
            if (thrower.tryHit(held, floorY) && landedTick < 0) landedTick = tick;
            if (isBound() != 0.0f) {
                everBound = true;
                ++boundTicks;
                // Held means held: the bind is what puts the victim at its anchor,
                // so a bound fighter that is not near the thrower is a bind that
                // is being reported but not applied.
                // Held means held, and the tolerance is loose on purpose:
                // `separate` runs after the bind has placed the victim and shoves
                // the two apart, so this asks that the victim is at its captor's
                // side of the stage rather than that it is at an exact offset.
                check(std::fabs(held.machine().x() - thrower.machine().x()) < 90.0f,
                      "a bound fighter should be held at its captor, not loose on the stage");
            }
        }
        check(landedTick >= 0, "the throw should connect at least once");
        check(everBound, "so the victim should report itself bound while the throw holds it");
        // The bind is asked for 20 ticks, so it must be released by the end of the
        // window -- a bind that never ends is the "stuck in a thrown state" failure
        // the reference's own guard is written against, arriving from the other
        // side.
        check(boundTicks > 5, "and the hold should last more than a moment");
        check(isBound() == 0.0f, "and the victim should be free again once the bind runs out");
    }

    // -------------------------- a custom state out of the attacker's own file
    //
    // `p2stateno` alone is a state in the *attacker's* file: `p2getp1state`
    // "Defaults to 1 if the `p2stateno` parameter is used. Ignored otherwise", and
    // "P2 will retain P1's states and animation data until P2 is hit, or a
    // SelfState controller is used to return P2 to his own states." Every throw in
    // the corpus is written that way -- of 276 literal `p2stateno` writes in
    // local_chars, 272 name a state defined in the writer's own folder, and exactly
    // one asks for `p2getp1state = 0` -- so resolving the number in the victim's
    // file drops the victim into the unknown-state fallback mid-throw: standing,
    // idle, control back. The checks below read the file whose header is in effect
    // (`stateCharacter()`), not just the state number, because the number alone
    // cannot tell the two files apart.
    {
        // Arms a HitDef whose extra lines are the custom-state ones, and appends a
        // whole file suffix after it -- the attacker's throw pose, or that pose plus
        // the controller that ends it.
        const auto armGrab = [&](const std::string& hitLines, const std::string& extra,
                                 af::Character& chr, af::StateMachine& machine) {
            chr.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 300, grab]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NT\n"
                "damage = 5\npausetime = 0,0\n" + hitLines + "\n" + extra,
                "[Statedef -1]\n");
            machine.setup(&chr, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(320.0f);
            machine.setStageBounds(90.0f, 550.0f);
            for (int i = 0; i < 20 && !machine.hasArmedHitDef(); ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
        };
        // The attacker's throw pose is a header the victim could not be in on its
        // own: airborne, `movetype = H`, no control.
        const std::string throwPose =
            "\n[Statedef 9990]\ntype = A\nmovetype = H\nphysics = N\nctrl = 0\n";

        af::Character grabberChar;
        af::StateMachine grabber;
        armGrab("p2stateno = 9990", throwPose, grabberChar, grabber);
        check(grabber.hasArmedHitDef(), "the grab should arm");
        check(grabber.armedHitDef().p2GetP1State,
              "`p2getp1state` defaults to 1 the moment `p2stateno` is written");
        af::Character victimChar;
        af::StateMachine victim;
        makeIdle(victimChar, victim);
        victim.setOpponent(&grabber);
        const af::StateMachine::HitInfo grabbed = grabber.hitInfoFor(victim);
        check(grabbed.stateNumber == 9990, "the grab asks for state 9990");
        check(grabbed.stateSourceChar == &grabberChar,
              "and says the state is in the attacker's file");
        // A projectile lands through the static overload, where there is no attacker
        // machine to ask, so the owner's file has to travel in the HitInfo. The two
        // paths must agree, and an explicit null owner degrades to the victim's file
        // rather than to a dangling pointer.
        const af::StateMachine::HitInfo fromBeam =
            af::StateMachine::hitInfoFor(grabber.armedHitDef(), 1, victim, &grabberChar);
        check(fromBeam.stateSourceChar == &grabberChar,
              "the static path a projectile uses carries the owner's file too");
        const af::StateMachine::HitInfo withoutOwner =
            af::StateMachine::hitInfoFor(grabber.armedHitDef(), 1, victim);
        check(withoutOwner.stateSourceChar == nullptr,
              "while no owner means the victim's own file, not a dangling source");
        check(victim.applyHit(grabbed), "the grab lands");
        check(victim.stateNumber() == 9990, "so the victim is in 9990");
        check(victim.stateCharacter() == &grabberChar,
              "running the attacker's state data, not its own");
        // The header is owed until the freeze lifts: the reference put the whole
        // state-entry block under `hitpausetime <= 0`, so a 9990 entered by a
        // hit's `pausetime` still shows the pre-hit header for that freeze.
        check(victim.stateType() == af::StateType::Stand,
              "the borrowed header waits out the hit's pause");
        victim.step();  // the pause tick comes off...
        victim.step();  // ...and this unfrozen tick runs the entry block
        check(victim.stateType() == af::StateType::Air && victim.moveType() == af::MoveType::Hit &&
                  !victim.ctrl(),
              "with the borrowed header -- airborne, `movetype = H`, no control -- rather "
              "than the unknown-state fallback, which would stand it up with control");
        check(victim.animCharacter() == &grabberChar,
              "and its animation comes from the same file, as the pair travels together");

        // The borrow lasts until the victim is hit or does a `SelfState`. A plain
        // hit is the first of those, and it has to clear the *file*, not just the
        // state number: a victim left pointing at the attacker's state data would
        // resolve its next reaction out of the wrong file.
        af::Character jabChar;
        af::StateMachine jab;
        swing("", jabChar, jab);
        check(jab.hasArmedHitDef(), "the follow-up should arm");
        const af::StateMachine::HitInfo jabbed = jab.hitInfoFor(victim);
        check(jabbed.stateSourceChar == nullptr,
              "a hit that writes no `p2stateno` names no source file");
        check(victim.applyHit(jabbed), "a second hit lands on the held victim");
        check(victim.stateCharacter() == &victimChar,
              "and the borrow ends there -- the victim is back on its own state data, "
              "exactly as `P2 will retain P1's states ... until P2 is hit` says");

        // `SelfState` is the other way home, and it is the one a throw's own
        // script uses: the controller is in the borrowed file (so the attacker
        // wrote it), but it acts on the victim.
        af::Character selfChar;
        af::StateMachine selfGrabber;
        armGrab("p2stateno = 9990",
                throwPose + "\n[State 9990, home]\ntype = SelfState\n"
                            "trigger1 = time = 2\nvalue = 0\n",
                selfChar, selfGrabber);
        af::Character selfVictimChar;
        af::StateMachine selfVictim;
        makeIdle(selfVictimChar, selfVictim);
        selfVictim.setOpponent(&selfGrabber);
        check(selfVictim.applyHit(selfGrabber.hitInfoFor(selfVictim)), "the self-state grab lands");
        check(selfVictim.stateCharacter() == &selfChar,
              "and the victim starts out on the grabber's file");
        for (int i = 0; i < 6 && selfVictim.stateCharacter() != &selfVictimChar; ++i) {
            af::InputState tick;
            selfVictim.control(tick);
            selfVictim.step();
        }
        check(selfVictim.stateNumber() == 0 && selfVictim.stateCharacter() == &selfVictimChar,
              "a `SelfState` in the borrowed file returns the victim to its own states");
        check(selfVictim.ctrl(), "with the control its own state 0 gives it");

        // And the opt-out: `p2getp1state = 0` means the victim's own file defines
        // the state. The two files give 9990 different headers on purpose, so which
        // one ran is visible in the posture rather than inferred from the pointer.
        af::Character ownChar;
        af::StateMachine ownGrabber;
        armGrab("p2stateno = 9990\np2getp1state = 0", throwPose, ownChar, ownGrabber);
        check(!ownGrabber.armedHitDef().p2GetP1State, "`p2getp1state = 0` opts out");
        af::Character ownVictimChar;
        ownVictimChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[Statedef 9990]\ntype = C\nphysics = C\nctrl = 1\n",
            "[Statedef -1]\n");
        af::StateMachine ownVictim;
        ownVictim.setup(&ownVictimChar, -1);
        ownVictim.setOpponent(&ownGrabber);
        ownVictim.setPosition(360.0f);
        ownVictim.setStageBounds(90.0f, 550.0f);
        const af::StateMachine::HitInfo own = ownGrabber.hitInfoFor(ownVictim);
        check(own.stateSourceChar == nullptr, "so the state stays in the victim's own file");
        check(ownVictim.applyHit(own), "and that grab lands");
        check(ownVictim.stateNumber() == 9990 && ownVictim.stateCharacter() == &ownVictimChar,
              "with the victim in 9990 out of its own file");
        ownVictim.step();  // the pause tick comes off...
        ownVictim.step();  // ...and the entry block runs on the next one
        check(ownVictim.stateType() == af::StateType::Crouch,
              "the header that takes effect is the victim's `type = C`, not the "
              "grabber's `type = A`");

        // Written without a `p2stateno` there is no state to take out of anybody's
        // file, and the parameter is documented as ignored. The parser has to say
        // so rather than record a source for a state that was never named.
        af::Character ignoredChar;
        af::StateMachine ignored;
        armGrab("p2getp1state = 1", "", ignoredChar, ignored);
        check(ignored.hasArmedHitDef(), "a hit with only `p2getp1state` still arms");
        check(!ignored.armedHitDef().p2GetP1State,
              "but `p2getp1state` is ignored without a `p2stateno`");
    }

    // ---------------------------------------- who spends a fall's landing
    //
    // A fall's landing owes three things -- the fall damage, the bounce velocity
    // and the screen shake -- and in MUGEN none of the three is the engine's to
    // apply. It is *script*: the reference's common1.cns spends them from states
    // 5100, 5101 and 5110 with `HitFallDamage`, `HitFallVel` and `FallEnvShake`,
    // and 34 of the 41 packs in local_chars ship a common1.cns that does exactly
    // that. The engine's own `land()` is its stand-in for a file that ships no
    // common state file at all.
    //
    // What lets the two coexist is that each effect is *spent* by whoever applies
    // it. The reference really does call HitFallDamage from two states -- 5100 at
    // time 3 and 5110 at time 0 -- and FallEnvShake from the same two, and
    // `FallEnvShake`'s own page says it "sets GetHitVar(fall.envshake.time) to zero
    // after being executed". So a file that scripts its own landing gets each
    // effect exactly once however many states ask for it, and a file that scripts
    // none still gets the engine's.
    //
    // This is worth a test rather than a comment because the failure mode is not a
    // missing feature but a *doubled* one: an engine landing applied on top of a
    // file's own charges the fall twice, and -- worse -- bounces at the moment of
    // contact and drains `GetHitVar(fall.yvel)`, so the file's 5100 then reads 0
    // and skips the bounce state entirely. The corpus leans on all three:
    // `HitFallDamage` is 86 uses across 34 packs, `HitFallVel` 40 across 34,
    // `FallEnvShake` 24 across 22.
    {
        // A launcher, so there is a fall to land. `ground.velocity`'s non-zero y is
        // what puts a standing victim in the air, and the `fall.*` parameters are
        // where a bounce's velocity, a fall's damage and its shake come from.
        af::Character atkChar;
        af::StateMachine atk;
        swing("ground.velocity = -4,-8\nfall.yvelocity = -4.5\nfall.xvelocity = 1.5\n"
              "fall.damage = 30\nfall.envshake.time = 20\nfall.envshake.freq = 60\n"
              "fall.envshake.ampl = 8",
              atkChar, atk);

        // What one landing looked like, so the assertions below can be about the
        // landing rather than about a particular tick number.
        struct Landing {
            bool tookTheHit = false;
            int lifeAfterHit = 0;
            bool fallCharged = false;
            int lifeAfterFall = 0;
            int stateAtLanding = 0;   // the state the fall was charged from
            float landingX = 0.0f;    // and the velocity it was charged with
            float landingY = 0.0f;
            bool sawFileLandingState = false;  // the file's own 5100, not the engine's
            float fileLandingX = 0.0f;
            float fileLandingY = 0.0f;
        };

        // Launches `machine` and runs it forward until the fall has been charged.
        //
        // It has to tick *through* the hitpause to see anything at all: `step()`
        // returns before any controller runs while the victim is frozen, and a hit
        // freezes for at least one tick even when its HitDef says zero. Ticking one
        // frame and concluding the landing controllers never run is an easy and
        // entirely wrong result -- it is what the first probe of this did.
        const auto launchInto = [&](const char* file, af::Character& chr,
                                    af::StateMachine& machine, RecordingEntities& sink,
                                    int ticks) {
            Landing out;
            chr.loadBehaviour(file, "[Statedef -1]\n");
            machine.setup(&chr, -1);
            machine.setOpponent(&atk);
            machine.setPosition(360.0f);
            machine.setStageBounds(90.0f, 550.0f);
            machine.setEntitySink(&sink);
            out.tookTheHit = machine.applyHit(atk.hitInfoFor(machine));
            out.lifeAfterHit = machine.life();
            // The shake is spent by a *later* state than the damage in the
            // reference's layout -- 5100 charges at time 1 and 5110 shakes at its
            // time 0 -- so stopping at the tick the damage lands would never see
            // it. The landing is snapshotted where the fall is charged and the
            // loop runs a few ticks past it.
            int grace = 0;
            for (int i = 0; i < ticks; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
                if (!out.sawFileLandingState &&
                    machine.stateNumber() == af::CommonState::kHitGround) {
                    out.sawFileLandingState = true;
                    out.fileLandingX = machine.velX();
                    out.fileLandingY = machine.velY();
                }
                if (!out.fallCharged && machine.life() < out.lifeAfterHit) {
                    out.fallCharged = true;
                    out.lifeAfterFall = machine.life();
                    out.stateAtLanding = machine.stateNumber();
                    out.landingX = machine.velX();
                    out.landingY = machine.velY();
                    grace = 4;
                } else if (out.fallCharged && --grace <= 0) {
                    break;
                }
            }
            return out;
        };

        // 1. The engine's landing, for a file that says nothing about landing at
        // all -- and no `[Statedef 5030]` header either, so `kBuiltins` gives the
        // fall chain Physics::Air and the gravity is real.
        af::Character silentChar;
        af::StateMachine silent;
        RecordingEntities silentSink;
        const Landing silentLanding =
            launchInto("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n", silentChar,
                       silent, silentSink, 200);
        check(silentLanding.tookTheHit, "a launcher puts a standing victim in a fall");
        check(silentLanding.stateAtLanding == af::CommonState::kHitBounce,
              "the victim reaches the engine's own bounce state as the fall is charged");
        // `damage = 20` from `swing`, plus the fall's 30 -- and 30 only once, which
        // is the whole contract. A second charge would read 920.
        check(silentLanding.lifeAfterFall == 950,
              "the engine's landing should charge fall.damage exactly once (20 + 30)");
        check(silentSink.shakes.size() == 1,
              "and shake the screen once, from fall.envshake");
        // The bounce is read at the landing tick, because that is the only tick it
        // exists on: the bounce velocity is a launch off the floor, and by the next
        // tick the fighter is rising.
        check(std::fabs(silentLanding.landingY - 4.5f) < 0.01f,
              "and bounce back up with the HitDef's fall.yvelocity");
        check(silentLanding.landingY > 0.0f && silent.y() > 1.0f,
              "so a launched victim is left in the air by the bounce, not stood up");

        // 2. A file that runs the whole landing itself, laid out the way the
        // reference's common1.cns is: 5030 applies its own gravity and hands to 5100
        // on the floor, 5100 spends the bounce and the damage, and 5110 spends the
        // damage and the shake *again*. Twice is the point -- the second spend must
        // be a no-op because the first took it. `physics = N` on 5030 is also the
        // reference's, and it is what keeps the engine's floor clamp -- and so
        // `land()` -- out of the way entirely.
        const char* kScriptedLanding =
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[Statedef 5030]\ntype = A\nmovetype = H\nphysics = N\n\n"
            "[State 5030, grav]\ntype = VelAdd\ntrigger1 = 1\ny = GetHitVar(yaccel)\n\n"
            "[State 5030, land]\ntype = ChangeState\ntrigger1 = Vel Y > 0\n"
            "trigger1 = Pos Y >= 0\nvalue = 5100\n\n"
            "[Statedef 5100]\ntype = L\nmovetype = H\nphysics = N\n\n"
            "[State 5100, vel]\ntype = HitFallVel\ntrigger1 = time = 0\n\n"
            "[State 5100, dmg]\ntype = HitFallDamage\ntrigger1 = time = 1\n\n"
            "[State 5100, next]\ntype = ChangeState\ntrigger1 = time = 2\nvalue = 5110\n\n"
            "[Statedef 5110]\ntype = L\nmovetype = H\nphysics = N\n\n"
            "[State 5110, dmgagain]\ntype = HitFallDamage\ntrigger1 = time = 0\n\n"
            "[State 5110, shake]\ntype = FallEnvShake\ntrigger1 = time = 0\n";
        af::Character scriptedChar;
        af::StateMachine scripted;
        RecordingEntities scriptedSink;
        const Landing scriptedLanding =
            launchInto(kScriptedLanding, scriptedChar, scripted, scriptedSink, 200);
        check(scriptedLanding.sawFileLandingState,
              "a file that scripts its landing reaches its own 5100 on the floor");
        check(std::fabs(scriptedLanding.fileLandingX - 1.5f) < 0.01f &&
                  std::fabs(scriptedLanding.fileLandingY - 4.5f) < 0.01f,
              "where its HitFallVel spends the bounce the launch left owing");
        check(scriptedLanding.lifeAfterFall == 950,
              "and its HitFallDamage charges the fall once, not once per state that asks");
        check(std::fabs(scriptedLanding.landingY - 4.5f) < 0.01f,
              "with the velocity the file set, which the engine did not overwrite");
        check(scriptedSink.shakes.size() == 1,
              "and its FallEnvShake shakes once, so the engine's landing stood down");

        // 3. The per-effect half, which is what `usesController` is asked: a file
        // that *declares* FallEnvShake -- so the library answers yes -- but never
        // runs it. The engine must withhold exactly the effect the file claimed and
        // keep the two it did not. Applying the shake anyway would be the engine
        // spending an effect the file said it owns, and the file's own state would
        // then shake a second time.
        af::Character declaredChar;
        af::StateMachine declared;
        RecordingEntities declaredSink;
        const Landing declaredLanding =
            launchInto("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                       "[Statedef 4900]\ntype = S\nmovetype = H\nphysics = N\n\n"
                       "[State 4900, declared]\ntype = FallEnvShake\ntrigger1 = 0\n",
                       declaredChar, declared, declaredSink, 200);
        check(declaredLanding.lifeAfterFall == 950,
              "a file that claims only the shake still gets the engine's fall damage");
        check(std::fabs(declaredLanding.landingY - 4.5f) < 0.01f,
              "and the engine's bounce");
        check(declaredSink.shakes.empty(),
              "but not the engine's shake, which that file said was its own to spend");
    }

    // ------------------------------ what a hit may take, and who wins a clash
    //
    // `HitDef` carries two kinds of parameter that are not about the reaction at
    // all. One family refuses the last point of life, by three different routes:
    // `kill` for the hit itself, `guard.kill` for the chip damage of a block, and
    // `fall.kill` for the damage a fall pays when it lands. The reference defaults
    // all three to 1, and each is the parameter that lets a move juggle a fighter
    // down to nothing *except* dead.
    //
    // The other is `priority = hit_prior, hit_type`: the number decides between two
    // simultaneous attacks, and the class decides a tie between equal numbers. The
    // class is what makes a throw possible at all -- "P1 and P2 should not be able
    // to simultaneously hit each other" -- and it is a *name*, so it has to be read
    // off the raw text rather than evaluated.
    {
        using PC = af::StateMachine::PriorityClass;

        // 1. The classes themselves. The six rows of the reference's table, each
        //    asked of both sides, because the rule is not symmetric and the
        //    asymmetry is the whole content: `Miss` yields to a `Hit` but a `Dodge`
        //    takes the hit out with it.
        check(af::StateMachine::clashLands(PC::Hit, PC::Hit),
              "Hit vs Hit: both attacks land");
        check(af::StateMachine::clashLands(PC::Hit, PC::Miss) &&
                  !af::StateMachine::clashLands(PC::Miss, PC::Hit),
              "Hit vs Miss: the hit lands and the miss does not, in one direction only");
        check(!af::StateMachine::clashLands(PC::Hit, PC::Dodge) &&
                  !af::StateMachine::clashLands(PC::Dodge, PC::Hit),
              "Hit vs Dodge: neither lands, whichever side the dodge is on");
        check(!af::StateMachine::clashLands(PC::Dodge, PC::Dodge),
              "Dodge vs Dodge: neither lands");
        check(!af::StateMachine::clashLands(PC::Dodge, PC::Miss) &&
                  !af::StateMachine::clashLands(PC::Miss, PC::Dodge),
              "Dodge vs Miss: neither lands");
        check(!af::StateMachine::clashLands(PC::Miss, PC::Miss),
              "Miss vs Miss: neither lands, which is two throws sailing past each other");

        // 2. The parse. `priority = 4, Miss` reaches the engine as a *word* beside a
        //    number, and the second item is what `secondParam` cannot read: it would
        //    evaluate the bare `Miss` as an identifier, get 0, and leave every clash
        //    class at its default. The class has to come off the raw text.
        af::Character missChar;
        af::StateMachine missMachine;
        swing("priority = 6, Miss", missChar, missMachine);
        check(missMachine.attackPriority() == 6,
              "`priority = 6, Miss` should set the priority number");
        check(missMachine.attackPriorityClass() == PC::Miss,
              "and the class beside it, which the expression parser would have read as 0");

        af::Character dodgeChar;
        af::StateMachine dodgeMachine;
        swing("priority = 2, Dodge", dodgeChar, dodgeMachine);
        check(dodgeMachine.attackPriorityClass() == PC::Dodge, "`Dodge` should parse as itself");

        //    A class nobody recognises is read as the default rather than as the
        //    worse of the two. Read as `Dodge` a typo would take the attack out of
        //    every clash it was in, and take the opponent's with it.
        af::Character bogusChar;
        af::StateMachine bogusMachine;
        swing("priority = 4, Bogus", bogusChar, bogusMachine);
        check(bogusMachine.attackPriorityClass() == PC::Hit,
              "an unrecognised class should fall back to the default `Hit`");

        af::Character plainChar;
        af::StateMachine plainMachine;
        swing("", plainChar, plainMachine);
        check(plainMachine.attackPriority() == 4 && plainMachine.attackPriorityClass() == PC::Hit,
              "and a HitDef with no `priority` at all keeps the documented default of 4, Hit");

        // 3. `kill = 0`. The hit lands, does its damage, and stops one point short.
        af::Character noKillChar;
        af::StateMachine noKill;
        swing("kill = 0", noKillChar, noKill);
        check(!noKill.armedHitDef().kill, "`kill = 0` should parse");
        af::Character noKillVictimChar;
        af::StateMachine noKillVictim;
        makeIdle(noKillVictimChar, noKillVictim);
        noKillVictim.gainLife(-980);  // twenty points, which the 20-damage hit would take
        af::StateMachine::HitInfo survivable = noKill.hitInfoFor(noKillVictim);
        check(!survivable.kill, "and travel into the resolved hit");
        check(noKillVictim.applyHit(survivable), "a non-lethal hit still lands");
        check(noKillVictim.life() == 1,
              "and `kill = 0` leaves the victim on one point of life, not at zero");

        //    The control, so the assertion above is about the parameter and not
        //    about a damage figure that happened to be survivable anyway.
        af::Character lethalChar;
        af::StateMachine lethal;
        swing("", lethalChar, lethal);
        af::Character lethalVictimChar;
        af::StateMachine lethalVictim;
        makeIdle(lethalVictimChar, lethalVictim);
        lethalVictim.gainLife(-980);
        check(lethalVictim.applyHit(lethal.hitInfoFor(lethalVictim)),
              "the same hit with the default `kill = 1` lands too");
        check(lethalVictim.life() == 0, "and kills");

        // 4. `guard.kill = 0`, the same refusal for a block's chip damage. A block
        //    that could kill would make blocking a losing move at one point of life,
        //    which is exactly the situation the parameter exists for.
        //
        //    Chip damage is the *second* component of `damage`, not a parameter of
        //    its own, so these have to go through an arming helper of their own:
        //    `swing` writes its own `damage = 20` line first, and a controller's
        //    `find` takes the first match for a key, so a second `damage` line after
        //    it would be ignored and every block here would chip for free.
        const auto armGuardHit = [&](const std::string& lines, af::Character& chr,
                                     af::StateMachine& machine) {
            chr.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\n\n"
                "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n" +
                    lines + "\n",
                "[Statedef -1]\n");
            machine.setup(&chr, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(320.0f);
            machine.setStageBounds(90.0f, 550.0f);
            for (int i = 0; i < 20 && !machine.hasArmedHitDef(); ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
        };
        af::Character guardKillChar;
        af::StateMachine guardKill;
        armGuardHit("damage = 20, 10\nguard.kill = 0", guardKillChar, guardKill);
        check(!guardKill.armedHitDef().guardKill, "`guard.kill = 0` should parse");
        check(guardKill.armedHitDef().guardDamage == 10,
              "and the chip damage is the second component of `damage`");
        af::Character guarderChar;
        af::StateMachine guarder;
        guarderChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                  "[Statedef -1]\n");
        guarder.setup(&guarderChar, -1);
        // The attacker as the opponent, at 320 to the blocker's 360: a fighter
        // faces its opponent, so this is what makes the blocker look left and
        // therefore hold back with the *right* key. The shared `dummy` will not do
        // -- earlier blocks move it, and a block that reads back is only testing the
        // key that happened to be back for wherever the dummy was left.
        guarder.setOpponent(&guardKill);
        guarder.setPosition(360.0f);
        guarder.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            tick.held = af::Button::kRight;
            guarder.control(tick);
            guarder.step();
        }
        guarder.gainLife(-995);  // five points of life, against ten of chip damage
        check(guarder.facing() < 0, "the blocker faces the attacker");
        check(guarder.holdingBack(), "and holds back with the key that is behind it");
        const af::StateMachine::HitInfo chipped = guardKill.hitInfoFor(guarder);
        check(chipped.guarded, "so the hit is blocked");
        check(!chipped.guardKill, "with `guard.kill` carried into the resolved hit");
        check(chipped.damage == 10, "and the block costing the chip damage");
        check(guarder.applyHit(chipped), "the block is taken");
        check(guarder.life() == 1,
              "and `guard.kill = 0` leaves the blocker on one point of life");

        af::Character chipChar;
        af::StateMachine chip;
        armGuardHit("damage = 20, 10", chipChar, chip);
        af::Character chipVictimChar;
        af::StateMachine chipVictim;
        chipVictimChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                     "[Statedef -1]\n");
        chipVictim.setup(&chipVictimChar, -1);
        chipVictim.setOpponent(&chip);
        chipVictim.setPosition(360.0f);
        chipVictim.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            tick.held = af::Button::kRight;
            chipVictim.control(tick);
            chipVictim.step();
        }
        chipVictim.gainLife(-995);
        check(chipVictim.holdingBack(), "the control blocker holds back too");
        check(chipVictim.applyHit(chip.hitInfoFor(chipVictim)),
              "the same block with the default `guard.kill = 1` is taken");
        check(chipVictim.life() == 0, "and chips the blocker to nothing");

        // 5. `fall.kill = 0`, the third route -- and the one that is hardest to
        //    wire, because the fall lands ticks after the HitDef that caused it has
        //    been overwritten. The answer has to travel with the damage, not be read
        //    back off the attacker when it is finally needed. So this is measured
        //    the way a player sees it: launch a victim, let it land, and read its
        //    life.
        af::Character fallKillChar;
        af::StateMachine fallKill;
        swing("ground.velocity = -4,-8\nfall.damage = 30\nfall.kill = 0", fallKillChar,
              fallKill);
        check(!fallKill.armedHitDef().fallKill, "`fall.kill = 0` should parse");

        // Launches a victim and runs it to the floor, reporting the life it had
        // after the hit and after the landing. `launch` gives it as many points of
        // life as the two charges need to add up past zero.
        const auto launchAndLand = [&](af::StateMachine& attacker, int lifeBefore, int& lifeAfterHit,
                                       int& lifeAfterFall) {
            af::Character chr;
            af::StateMachine machine;
            RecordingEntities sink;
            chr.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                              "[Statedef -1]\n");
            machine.setup(&chr, -1);
            machine.setOpponent(&attacker);
            machine.setPosition(360.0f);
            machine.setStageBounds(90.0f, 550.0f);
            machine.setEntitySink(&sink);
            machine.gainLife(lifeBefore - machine.life());
            machine.applyHit(attacker.hitInfoFor(machine));
            lifeAfterHit = machine.life();
            lifeAfterFall = lifeAfterHit;
            // The hit freezes for at least one tick even at zero pausetime, so the
            // landing is a good way off; and the fall is charged on the tick the
            // fighter touches the floor, which is where the second reading is taken.
            for (int i = 0; i < 200; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
                if (machine.life() != lifeAfterHit) {
                    lifeAfterFall = machine.life();
                    break;
                }
            }
        };

        // Twenty points of hit damage (swing's `damage = 20`) plus thirty of fall
        // damage, against forty points of life: lethal twice over unless the HitDef
        // says the fall may not finish the job.
        int afterHit = 0;
        int afterFall = 0;
        launchAndLand(fallKill, 40, afterHit, afterFall);
        check(afterHit == 20, "the launch itself takes only the hit's damage");
        check(afterFall == 1,
              "and `fall.kill = 0` leaves the victim on one point of life when the fall lands");

        af::Character fallLethalChar;
        af::StateMachine fallLethal;
        swing("ground.velocity = -4,-8\nfall.damage = 30", fallLethalChar, fallLethal);
        launchAndLand(fallLethal, 40, afterHit, afterFall);
        check(afterFall == 0,
              "the same fall with the default `fall.kill = 1` finishes the victim off");
    }

    // ------------------------------------- which animation the victim reacts with
    //
    // `animtype` is not one parameter but three, and which one a hit uses depends
    // on where the victim was when it landed: `animtype` for a victim who is left
    // on the ground, `air.animtype` for one in the air, and `fall.animtype` for one
    // who is falling -- either already, or as of this hit. Neither of the last two
    // defaults to a constant; each points at the parameter before it, and
    // `fall.animtype`'s default is a rule rather than a value ("Up if `air.animtype`
    // is Up, Back otherwise" -- and, in the binary, DiagUp if `air.animtype` is
    // DiagUp, which is where the code and the page disagree; the code is the one
    // that fights).
    //
    // This is the single most-read trigger in the whole hit family:
    // `GetHitVar(animtype)` is 990 uses across 38 characters in the corpus, against
    // 110 for `groundtype` and 44 for `airtype`, and `air.type` alone is written
    // 746 times. It is what a character's own common state file picks its reaction
    // animation with, so the engine reporting its ground animtype for every hit is
    // the difference between a juggled victim flipping backwards off a launcher and
    // one taking a jab to the face in mid-air.
    {
        // An airborne victim who is not (yet) falling. State type is the authority
        // on that, so a statedef with `type = A` is enough and the victim never has
        // to leave the floor for the air tier to be the right one.
        const auto makeAirborne = [&](af::Character& chr, af::StateMachine& machine) {
            chr.loadBehaviour("[Statedef 0]\ntype = A\nphysics = A\nctrl = 1\n",
                              "[Statedef -1]\n");
            machine.setup(&chr, -1);
            machine.setOpponent(&dummy);
            machine.setPosition(360.0f);
            machine.setStageBounds(90.0f, 550.0f);
        };
        // And one who is airborne *and* in a get-hit, which is what "falling" means
        // to `fall.animtype` -- the same condition `hitflag`'s F accepts. The hit is
        // driven through `applyHit` rather than assembled by hand because movetype
        // is the gethit state's to set, not the HitInfo's.
        const auto makeFalling = [&](af::Character& chr, af::StateMachine& machine) {
            makeIdle(chr, machine);
            af::StateMachine::HitInfo lift;
            lift.damage = 0;
            lift.attackerFacing = 1;
            lift.pushX = 0.0f;
            lift.pushY = 12.0f;
            lift.hitTime = 120;
            lift.juggleCost = 0;
            lift.stateNumber = af::CommonState::kHitAir;
            machine.applyHit(lift);
            for (int i = 0; i < 3; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
        };

        // The air tier, and the launch case that is easy to get wrong: a *standing*
        // victim hit by a launcher is airborne by the time the reaction is chosen,
        // so reading only their pre-hit state would give them a ground reaction for
        // an air launch.
        af::Character upChar;
        af::StateMachine up;
        swing("animtype = Light\nair.animtype = Up", upChar, up);
        af::Character airborneChar;
        af::StateMachine airborne;
        makeAirborne(airborneChar, airborne);
        const af::StateMachine::HitInfo aloftInfo = up.hitInfoFor(airborne);
        check(aloftInfo.animType == 4, "an airborne victim should take air.animtype, not animtype");
        check(aloftInfo.groundType == 1 && aloftInfo.airType == 1,
              "and both hit types should default to High when nothing writes them");

        af::Character fallingChar;
        af::StateMachine falling;
        makeFalling(fallingChar, falling);
        check(falling.y() > 0.5f && falling.moveType() == af::MoveType::Hit,
              "the falling fixture should be airborne and in a get-hit before it is used");
        check(up.hitInfoFor(falling).animType == 4,
              "a falling victim should take fall.animtype, which follows air.animtype when "
              "that is Up");

        // The default's rule, both halves. DiagUp carries through as DiagUp and not
        // as Back -- the page says Back, the parser says otherwise, and the parser is
        // the one that runs.
        af::Character diagChar;
        af::StateMachine diag;
        swing("animtype = Light\nair.animtype = DiagUp", diagChar, diag);
        check(diag.hitInfoFor(falling).animType == 5,
              "an unwritten fall.animtype should follow air.animtype when that is DiagUp");
        check(diag.hitInfoFor(airborne).animType == 5,
              "and an airborne victim takes the DiagUp air animtype itself");
        af::Character mediumChar;
        af::StateMachine medium;
        swing("animtype = Light\nair.animtype = Medium", mediumChar, medium);
        check(medium.hitInfoFor(falling).animType == 3,
              "but should fall back to Back once air.animtype is neither Up nor DiagUp");
        check(medium.hitInfoFor(airborne).animType == 1, "while the air tier stays Medium");

        // Written explicitly, `fall.animtype` beats the rule it replaces.
        af::Character hardChar;
        af::StateMachine hard;
        swing("animtype = Light\nair.animtype = Up\nfall.animtype = Hard", hardChar, hard);
        check(hard.hitInfoFor(falling).animType == 2,
              "a written fall.animtype should override the Up default");

        // The ground tier is untouched by any of it: a victim left standing uses the
        // HitDef's own animtype, and a launcher is the one exception.
        af::Character groundChar;
        af::StateMachine ground;
        swing("animtype = Hard\nair.animtype = Up", groundChar, ground);
        af::Character groundVictimChar;
        af::StateMachine groundVictim;
        makeIdle(groundVictimChar, groundVictim);
        check(ground.hitInfoFor(groundVictim).animType == 2,
              "a victim left on the ground should keep the HitDef's own animtype");
        af::Character launchChar;
        af::StateMachine launch;
        swing("animtype = Light\nfall.animtype = Hard\nground.velocity = -4,-8", launchChar,
              launch);
        check(launch.hitInfoFor(groundVictim).animType == 2,
              "but a standing victim this hit launches should take fall.animtype, because "
              "they are airborne by the time it is read");
        af::Character flatLaunchChar;
        af::StateMachine flatLaunch;
        swing("animtype = Light\nfall.animtype = Hard\nground.velocity = -4,0", flatLaunchChar,
              flatLaunch);
        check(flatLaunch.hitInfoFor(groundVictim).animType == 0,
              "and a hit that leaves them standing should not");

        // The six names are matched by their *first letter* in the reference and by
        // nothing else: the parser loads one byte, lowercases it, subtracts `'b'`,
        // bounds-checks against `0x13` and jumps through a twenty-entry table whose
        // only populated slots are `l`->Light, `m`->Medium, `h`->Hard, `b`->Back,
        // `u`->Up and `d`->DiagUp (`mugen.exe` 0x47EE4A; the same scheme serves
        // `air.animtype` at 0x47EE9B and `fall.animtype` at 0x47F019). The strings
        // `Light` and `DiagUp` do not appear in the binary at all, so a match on the
        // spelling is not merely absent but impossible.
        //
        // This is not a nicety. `animtype = heavy` is written 744 times in the
        // corpus and reads as Hard, where matching against the documented names --
        // which is what this did -- leaves it Light, the difference between a
        // victim's medium and hard reaction animation. `med` (17), `down` (13) and
        // `high` (7) are the same mistake in smaller numbers.
        const auto animTypeOf = [&](const std::string& line) {
            af::Character chr;
            af::StateMachine machine;
            swing(line, chr, machine);
            return machine.armedHitDef().animType;
        };
        check(animTypeOf("animtype = heavy") == 2, "`heavy` should read as Hard, on its `h`");
        check(animTypeOf("animtype = med") == 1, "`med` as Medium, on its `m`");
        check(animTypeOf("animtype = high") == 2, "`high` as Hard, on its `h`");
        check(animTypeOf("animtype = down") == 5, "and `down` as DiagUp, on its `d`");
        check(animTypeOf("animtype = light") == 0 && animTypeOf("animtype = Light") == 0 &&
                  animTypeOf("animtype = Medium") == 1 && animTypeOf("animtype = Hard") == 2 &&
                  animTypeOf("animtype = Back") == 3 && animTypeOf("animtype = Up") == 4 &&
                  animTypeOf("animtype = DiagUp") == 5,
              "while the six documented names still land on their own numbers, in either case");

        // The hit *types* are the same shape of family and reach the victim the same
        // way, through `GetHitVar`. The numbering is the reference's -- 1 high, 2 low,
        // 3 trip -- because that is the number a common state file branches on
        // (`ifelse(GetHitVar(groundtype) = 1, 5000, 5010)`), and the engine used to
        // answer a hardcoded 1 for every hit ever taken.
        af::Character typesChar;
        af::StateMachine types;
        swing("ground.type = Low\nair.type = Trip", typesChar, types);
        af::Character typedVictimChar;
        af::StateMachine typedVictim;
        makeIdle(typedVictimChar, typedVictim);
        const af::StateMachine::HitInfo typed = types.hitInfoFor(typedVictim);
        check(typed.groundType == 2, "ground.type = Low is 2 in the reference's numbering");
        check(typed.airType == 3, "and air.type = Trip is 3");
        af::StateMachine::HitInfo applied = typed;
        applied.damage = 0;
        applied.juggleCost = 0;
        check(typedVictim.applyHit(applied),
              "the typed hit should land, or the three checks below read an empty record");
        check(typedVictim.trigger("gethitvar", {af::Value::text("groundtype")}).toFloat() == 2.0f,
              "gethitvar(groundtype) should report the HitDef's ground.type");
        check(typedVictim.trigger("gethitvar", {af::Value::text("airtype")}).toFloat() == 3.0f,
              "gethitvar(airtype) should report air.type");
        check(typedVictim.trigger("gethitvar", {af::Value::text("type")}).toFloat() == 2.0f,
              "and gethitvar(type) the ground type, which is the legacy name for it");

        // An unwritten `air.type` follows `ground.type` rather than standing on its
        // own, so a character that writes only the ground one still reads it back.
        af::Character inheritChar;
        af::StateMachine inherit;
        swing("ground.type = Low", inheritChar, inherit);
        check(inherit.hitInfoFor(airborne).airType == 2,
              "an unwritten air.type should follow ground.type");

        // And the resolved animtype is the one the victim reads back -- not the
        // HitDef's, which is the whole point of resolving it.
        af::Character reportedChar;
        af::StateMachine reported;
        makeAirborne(reportedChar, reported);
        af::StateMachine::HitInfo reportedHit = up.hitInfoFor(reported);
        reportedHit.damage = 0;
        reportedHit.juggleCost = 0;
        check(reported.applyHit(reportedHit), "the air hit should land");
        check(reported.trigger("gethitvar", {af::Value::text("animtype")}).toFloat() == 4.0f,
              "gethitvar(animtype) should report the air animtype to an airborne victim");
    }

    // ------------------------------------------- which shake, whose posture
    //
    // The state a grounded victim is put in is picked by the *victim's* posture
    // and not by the attack's height, and the two questions are easy to run
    // together because the attack's height is a number that names a state-like
    // pair: `ground.type = Low` is 2, and 5010 is one state below 5000. The
    // reference separates them, and the separation is visible in its own
    // `[Statedef 5000]`: the state is `Stand get-hit (shaking)` with `type = S`
    // while the controller that reads the hit type is a `ChangeAnim` --
    // `value = ifelse((GetHitVar(groundtype) = 1),5000,5010) +
    // GetHitVar(animtype)` -- so a low attack on a *standing* victim draws the
    // crouch-hit art from inside the standing state. Picking the state number
    // off the hit instead sent a standing victim into 5010, a crouch MUGEN never
    // pops anyone into, and left a crouching victim of a high attack in 5000,
    // which stands them up out of the crouch they were hit in. The binary reads
    // it off the victim for the same reason: the hit path writes
    // `(0 < statetype - 1 ? 5010 : 5000)`, "crouching ? 5010 : 5000"
    // (FINDINGS pass 12 §12.2).
    {
        // A low attack is the sharp case: it is the one whose hit type *names*
        // the crouching state.
        af::Character lowChar;
        af::StateMachine lowSwing;
        swing("ground.type = Low\nanimtype = Light", lowChar, lowSwing);

        af::Character standingChar2;
        af::StateMachine standingVictim;
        makeIdle(standingChar2, standingVictim);
        const af::StateMachine::HitInfo onStanding = lowSwing.hitInfoFor(standingVictim);
        check(onStanding.groundType == 2, "the hit under test should be a low one");
        check(standingVictim.stateType() == af::StateType::Stand,
              "and the victim of it should be standing, or the checks below prove nothing");
        check(onStanding.stateNumber == af::CommonState::kHitHigh,
              "a standing victim of a low attack should be hit in 5000, the standing shake");

        // And the other way round, which is the half a fix written from the hit's
        // point of view would still get wrong: a crouching victim of an ordinary
        // high attack belongs in 5010.
        af::Character highChar;
        af::StateMachine highSwing;
        swing("ground.type = High\nanimtype = Light", highChar, highSwing);
        af::Character crouchChar;
        af::StateMachine crouchingVictim;
        crouchChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                 "[Statedef -1]\n");
        crouchingVictim.setup(&crouchChar, -1);
        crouchingVictim.setOpponent(&dummy);
        crouchingVictim.setPosition(360.0f);
        crouchingVictim.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 30 && crouchingVictim.stateNumber() != af::CommonState::kCrouch; ++i) {
            af::InputState holdDown;
            holdDown.held = af::Button::kDown;
            crouchingVictim.control(holdDown);
            crouchingVictim.step();
        }
        check(crouchingVictim.stateNumber() == af::CommonState::kCrouch,
              "the victim under test should be able to crouch, or the next check is empty");
        const af::StateMachine::HitInfo onCrouching = highSwing.hitInfoFor(crouchingVictim);
        check(onCrouching.stateNumber == af::CommonState::kHitLow,
              "a crouching victim of a high attack should be hit in 5010, the crouching shake");

        // Trip is the hit's own answer, and the one state number the reference
        // does take from the hit type -- `if (ground.type == 3) state = 5070`.
        af::Character tripChar;
        af::StateMachine tripSwing;
        swing("ground.type = Trip", tripChar, tripSwing);
        af::Character tripVictimChar;
        af::StateMachine tripVictim;
        makeIdle(tripVictimChar, tripVictim);
        const af::StateMachine::HitInfo tripped = tripSwing.hitInfoFor(tripVictim);
        check(tripped.stateNumber == af::CommonState::kHitTrip,
              "a trip should still be a trip, whatever the victim was doing");

        // The crouching reaction has to *end* in the crouch as well, which is
        // the second half of the same mistake: 5010's hand-over in the reference
        // is `value = 5011`, and 5011's is `value = 11`. Letting the pair end at
        // state 0 stood a crouching victim upright the moment the shake was over
        // -- and it is the *only* state the reaction can end in that is not the
        // crouch, so the check has to be made with the victim still asking to be
        // crouched, which is the posture they were hit in.
        //
        // What the state *route* is matters as much as where it ends, because a
        // fighter that goes through state 0 has been handed back to standing --
        // its control comes back, and `prevstateno` reports a state the reaction
        // was never in. So this is checked on `prevstateno` at the moment the
        // crouch is regained: the reference's route is 5010 -> 5011 -> 11.
        af::Character kbVictimChar2;
        af::StateMachine kbVictim;
        makeIdle(kbVictimChar2, kbVictim);
        af::InputState stillDown;
        stillDown.held = af::Button::kDown;
        for (int i = 0; i < 30 && kbVictim.stateNumber() != af::CommonState::kCrouch; ++i) {
            kbVictim.control(stillDown);
            kbVictim.step();
        }
        af::StateMachine::HitInfo crouchHit = highSwing.hitInfoFor(kbVictim);
        crouchHit.damage = 0;
        crouchHit.hitTime = 4;
        crouchHit.juggleCost = 0;
        check(kbVictim.applyHit(crouchHit), "the hit on the crouching victim should land");
        check(kbVictim.stateNumber() == af::CommonState::kHitLow,
              "and put them in 5010 first");
        for (int i = 0; i < 60 && kbVictim.stateNumber() != af::CommonState::kCrouch; ++i) {
            kbVictim.control(stillDown);
            kbVictim.step();
        }
        check(kbVictim.stateNumber() == af::CommonState::kCrouch,
              "and a victim hit out of a crouch should be back in the crouch when the shake ends");
        check(kbVictim.prevStateNumber() == af::CommonState::kHitLowKb,
              "by way of 5011, the crouching knock-back, and not by standing up through state 0");

        // Walking backwards is state 20 as well, and this one is checked on the
        // animation as well as on the state, because those are the two halves of
        // the mistake: `common1.cns` has no `[Statedef 21]` -- 21 is the
        // *animation* its own state 20 selects for a negative velocity
        // (`[State 20, 4] ChangeAnim / triggerall = vel x < 0 / value = 21`) --
        // so a fighter put into state 21 never reaches its walk state, and a
        // fighter with no state of its own walked backwards in the forward art.
        af::Character walkChar;
        af::StateMachine walker;
        makeIdle(walkChar, walker);
        dummy.setPosition(520.0f);
        af::InputState holdBack;
        holdBack.held = af::Button::kLeft;
        for (int i = 0; i < 3; ++i) {
            walker.control(holdBack);
            walker.step();
        }
        check(walker.stateNumber() == af::CommonState::kWalk,
              "holding back should walk in state 20, the only walking state there is");
        check(walker.animNumber() == af::CommonState::kWalkBackAnim,
              "and use the backwards walk's animation, 21, rather than the state's own 20");
        af::InputState holdFwd2;
        holdFwd2.held = af::Button::kRight;
        for (int i = 0; i < 3; ++i) {
            walker.control(holdFwd2);
            walker.step();
        }
        check(walker.stateNumber() == af::CommonState::kWalk, "and so should walking forward");
        check(walker.animNumber() == af::CommonState::kWalk,
              "with the forward walk's animation, which shares the state's number");
    }

    // --------------------------------- where the two fighters end up looking
    //
    // The `p1facing` family is the one group of HitDef parameters that acts on the
    // *attacker* rather than the victim, and it exists for the reverse throw: a grab
    // written so the player can throw the opponent over either shoulder picks its own
    // facing from the direction being held. Three characters in the corpus do exactly
    // that (`ifelse(command = "holdfwd", 1, -1)`, and a corner-aware version of the
    // same idea that turns the throw away from the near wall), and Mista's AI file
    // writes the plain -1. What the reference stores of the pair is not the two
    // numbers: `p1facing` is kept as the single bit "is it negative", so a 1 and an
    // unwritten parameter are the same no-op, and a non-zero `p1getp2facing` clears
    // that bit and stands in for it. Both halves of that are checked below, because
    // the first half is the one a reader of the reference's docs would get wrong.
    {
        // --- the parse ---
        af::Character turnChar;
        af::StateMachine turner;
        swing("p1facing = -1", turnChar, turner);
        check(turner.armedHitDef().p1facing == -1,
              "a written p1facing should survive the parse");
        check(turner.armedHitDef().p1GetP2Facing == 0,
              "and the other half of the family should default to 0");

        af::Character plainFaceChar;
        af::StateMachine plainFace;
        swing("", plainFaceChar, plainFace);
        check(plainFace.armedHitDef().p1facing == 0 &&
                  plainFace.armedHitDef().p1GetP2Facing == 0,
              "a HitDef that writes neither should leave the attacker's facing alone");

        // A victim to read a facing off. Facing the attacker's own way on purpose:
        // the two halves of the family must be told apart, and they only differ when
        // a flip and a copy of the victim's facing disagree.
        af::Character victimChar;
        af::StateMachine victim;
        makeIdle(victimChar, victim);
        victim.setPosition(360.0f);
        victim.setFacing(-1);

        // --- the sign, which is all `p1facing` is ---
        turner.setFacing(1);
        turner.markHitLanded(&victim);
        check(turner.facing() == -1,
              "a negative p1facing should turn the attacker around when the hit lands");

        af::Character forwardChar;
        af::StateMachine forward;
        swing("p1facing = 1", forwardChar, forward);
        forward.setFacing(1);
        forward.markHitLanded(&victim);
        check(forward.facing() == 1,
              "a positive p1facing is the same no-op as an unwritten one, not the reverse");

        // --- and the pair it is overridden by ---
        af::Character pairChar;
        af::StateMachine paired;
        swing("p1facing = -1\np1getp2facing = 1", pairChar, paired);
        check(paired.armedHitDef().p1GetP2Facing == 1,
              "p1getp2facing should survive the parse");
        paired.setFacing(-1);
        paired.markHitLanded(&victim);
        check(paired.facing() == -1,
              "p1getp2facing = 1 should face the attacker the victim's way, which outranks "
              "the p1facing beside it -- a flip would have sent it the other way");

        af::Character oppositeChar;
        af::StateMachine opposite;
        swing("p1facing = -1\np1getp2facing = -1", oppositeChar, opposite);
        opposite.setFacing(-1);
        opposite.markHitLanded(&victim);
        check(opposite.facing() == 1,
              "and p1getp2facing = -1 should face it the opposite way from the victim");

        // With nobody on the far end there is no facing to copy, and the parameter
        // that stands in for the family must not fall back on the sign.
        af::Character lonelyChar;
        af::StateMachine lonely;
        swing("p1facing = -1\np1getp2facing = 1", lonelyChar, lonely);
        lonely.setFacing(1);
        lonely.markHitLanded(nullptr);
        check(lonely.facing() == 1,
              "a hit with no victim behind it should leave the attacker's facing alone");
    }

    // --------------------------------- hit while down, and the air block
    //
    // Two families that both exist because a fighter is not always standing on the
    // floor in neutral. `down.*` is the reaction for a victim *already lying down*,
    // which only a move whose `hitflag` carries D may produce (`hitMayReach`
    // refuses the rest, so the branch is reached by intent) -- and its shape is
    // chosen by whether the down velocity has a y at all, not by how big it is.
    // `airguard.*` is the reaction for a hit *blocked in the air*, which is not a
    // ground block held aloft: it has its own velocity and its own clock, and both
    // existed in the parse without a consumer until now.
    {
        // --- the parse, and the defaults the two families fall back on ---

        af::Character bareChar;
        af::StateMachine bare;
        swing("", bareChar, bare);
        const af::StateMachine::HitDef bareDef = bare.armedHitDef();
        check(bareDef.downHitTime == 20, "`down.hittime` should default to 20");
        check(!bareDef.downBounce, "and `down.bounce` to off, which is the whole point of it");
        check(bareDef.downVelY == 0.0f,
              "`down.velocity` falls back on `air.velocity`, which defaults to nothing");
        check(bareDef.guardCtrlTime == bareDef.guardSlideTime,
              "`guard.ctrltime` should fall back on `guard.slidetime`");
        check(bareDef.airGuardCtrlTime == bareDef.guardCtrlTime,
              "and `airguard.ctrltime` on `guard.ctrltime`");
        check(bareDef.airGuardVelX == -6.0f && bareDef.airGuardVelY == 0.0f,
              "with `airguard.velocity` scaling the air velocity, which itself fell back on "
              "the ground one -- a bare HitDef still pushes an airborne blocker");

        af::Character downChar;
        af::StateMachine down;
        swing("down.velocity = -4,-5\ndown.hittime = 8\ndown.bounce = 1", downChar, down);
        const af::StateMachine::HitDef written = down.armedHitDef();
        check(written.downVelX == -4.0f, "a written `down.velocity` x goes in as it stands");
        check(written.downVelY == 5.0f, "and its y is converted to the engine's frame, up positive");
        check(written.downHitTime == 8, "`down.hittime` is whatever the character asked for");
        check(written.downBounce, "and `down.bounce = 1` is on");

        // The scaled default, which is the whole reason an air block feels
        // different from a ground one: the blocker is pushed back half again as
        // hard and dropped half as far as the air velocity alone would have.
        af::Character scaledChar;
        af::StateMachine scaledMachine;
        swing("air.velocity = -2,-4", scaledChar, scaledMachine);
        const af::StateMachine::HitDef scaledDef = scaledMachine.armedHitDef();
        check(scaledDef.airGuardVelX == -3.0f,
              "an unwritten `airguard.velocity` x should be the air velocity x times 1.5");
        check(scaledDef.airGuardVelY == 2.0f,
              "and its y should be the air velocity y halved, upward");

        af::Character explicitChar;
        af::StateMachine explicitGuard;
        swing("airguard.velocity = -6,-2\nairguard.ctrltime = 7\nguard.ctrltime = 13",
              explicitChar, explicitGuard);
        const af::StateMachine::HitDef guardDef = explicitGuard.armedHitDef();
        check(guardDef.guardCtrlTime == 13, "`guard.ctrltime` is read when written");
        check(guardDef.airGuardCtrlTime == 7,
              "and `airguard.ctrltime` overrides the `guard.ctrltime` it would have inherited");
        check(guardDef.airGuardVelX == -6.0f && guardDef.airGuardVelY == 2.0f,
              "a written `airguard.velocity` is used as written");

        // --- the reaction to a hit on a victim who is already down ---
        //
        // The victim has to really be in a downed state rather than merely at
        // height zero: `stateType` is what the reaction is chosen from.
        const auto makeDowned = [&](af::Character& chr, af::StateMachine& machine) {
            // Reached the way a character reaches it rather than by calling into
            // the engine: a state whose header says `type = L` is what makes a
            // fighter downed, and entering it from state 0 is how a character
            // would. The header wins over the engine's own table for the number,
            // so this is the character's own claim about its pose.
            chr.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                "[State 0, down]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 5110\n\n"
                "[Statedef 5110]\ntype = L\nmovetype = I\nphysics = N\nctrl = 0\n",
                "[Statedef -1]\n");
            machine.setup(&chr, -1);
            machine.setOpponent(&dummy);
            machine.setPosition(360.0f);
            machine.setStageBounds(90.0f, 550.0f);
            for (int i = 0; i < 6 && machine.stateType() != af::StateType::Liedown; ++i) {
                af::InputState tick;
                machine.control(tick);
                machine.step();
            }
        };

        af::Character slideChar;
        af::StateMachine slide;
        swing("hitflag = MAFD\ndown.velocity = -4\ndown.hittime = 9", slideChar, slide);
        af::Character downedChar;
        af::StateMachine downed;
        makeDowned(downedChar, downed);
        check(downed.stateType() == af::StateType::Liedown,
              "the victim for the down checks should be lying down");
        const af::StateMachine::HitInfo slid = slide.hitInfoFor(downed);
        check(!slid.air, "a `down.velocity` with no y slides the victim along the floor");
        check(!slid.falls,
              "and slides it: the floor is where that branch stays, so there is nothing to "
              "fall to");
        check(slid.stateNumber == af::CommonState::kHitLiedown,
              "into 5080, the state the engine picks for a victim who is already down");
        check(slid.hitTime == 9, "for `down.hittime` ticks");
        check(slid.pushY == 0.0f, "with no vertical velocity at all");
        check(slid.pushX == -4.0f, "and the written slide, away from the attacker");

        af::Character popChar;
        af::StateMachine pop;
        swing("hitflag = MAFD\ndown.velocity = -4,-5\ndown.bounce = 1", popChar, pop);
        af::Character popVictimChar;
        af::StateMachine popVictim;
        makeDowned(popVictimChar, popVictim);
        const af::StateMachine::HitInfo popped = pop.hitInfoFor(popVictim);
        check(popped.air, "a non-zero `down.velocity` y puts the victim back in the air");
        check(popped.falls,
              "which makes it a fall as well, the same answer `[State 5080, 5]` asks for: the "
              "branch that sends the victim back up is the branch that sends it to 5030");
        check(popped.stateNumber == af::CommonState::kHitLiedown,
              "still entering 5080, whose own `GetHitVar(yvel)` test is what sends it up");
        check(popped.pushY == 5.0f, "carrying an upward velocity");
        check(popped.fallVelY > 0.0f, "and `down.bounce = 1` gives the landing a bounce");
        check(popped.hitTime == pop.armedHitDef().airHitTime,
              "with the air clock rather than `down.hittime`");

        // The same again with the bounce left off. This is the check that stops the
        // engine's ordinary fall bounce from leaking in: a fall bounces whenever its
        // fall velocity points up, which by default it does, so a downed hit that did
        // not ask for the pop-up has to hand over no bounce velocity at all.
        af::Character noBounceChar;
        af::StateMachine noBounce;
        swing("hitflag = MAFD\ndown.velocity = -4,-5", noBounceChar, noBounce);
        af::Character noBounceVictimChar;
        af::StateMachine noBounceVictim;
        makeDowned(noBounceVictimChar, noBounceVictim);
        const af::StateMachine::HitInfo settled = noBounce.hitInfoFor(noBounceVictim);
        check(settled.air, "a downed hit into the air is still into the air without a bounce");
        check(settled.fallVelY == 0.0f,
              "but it lands and stays down, because `down.bounce` defaults to off");
        check(noBounce.armedHitDef().fallVelY > 0.0f,
              "even though the move's own fall velocity would have bounced it");

        // The control that keeps the branch honest: a move that does *not* admit D
        // must not reach a downed victim at all, so none of the above applies to it.
        af::Character noDChar;
        af::StateMachine noD;
        swing("hitflag = MAF\ndown.velocity = -4,-5", noDChar, noD);
        af::Character noDVictimChar;
        af::StateMachine noDVictim;
        makeDowned(noDVictimChar, noDVictim);
        check(!af::StateMachine::hitMayReach(noD.armedHitDef(), noDVictim),
              "a hitflag without D should not reach a victim who is already down");

        // --- the reaction to a hit blocked in the air ---

        // The blockers in this block get an opponent of their own, standing to
        // their left, rather than the file's shared dummy -- which an earlier
        // block walks about, and a grounded fighter auto-turns to face whatever
        // it is pointed at, which would silently turn "back" into the other key.
        af::StateMachine guardOpponent;
        guardOpponent.setup(nullptr, 1);
        guardOpponent.setPosition(320.0f);
        guardOpponent.setStageBounds(90.0f, 550.0f);

        // An airborne blocker. It has to be *idle* in the air, because a fighter in
        // hitstun or mid-move cannot guard whatever its thumbs are doing -- so the
        // engine's own jump lifts it rather than a hit being forced on it, and the
        // thumbs then stay back (the blocker faces left, so right is back).
        const auto makeAirBlocker = [&](af::Character& chr, af::StateMachine& machine) {
            makeIdle(chr, machine);
            machine.setOpponent(&guardOpponent);
            for (int tick = 0; tick < 30; ++tick) {
                af::InputState input;
                input.held = tick < 4 ? af::Button::kUp : af::Button::kRight;
                machine.control(input);
                machine.step();
                if (tick >= 4 && machine.stateType() == af::StateType::Air &&
                    machine.y() > 0.5f && machine.holdingBack()) {
                    return;
                }
            }
        };

        af::Character guardChar;
        af::StateMachine guarder;
        swing("guard.velocity = -4\nairguard.velocity = -6,-2\nairguard.ctrltime = 7\n"
              "guard.ctrltime = 13\nground.hittime = 17\nair.velocity = -2,-4",
              guardChar, guarder);

        af::Character airBlockChar;
        af::StateMachine airBlock;
        makeAirBlocker(airBlockChar, airBlock);
        check(airBlock.stateType() == af::StateType::Air && airBlock.y() > 0.5f,
              "the victim for the air-block checks should be up in the air");
        check(airBlock.holdingBack(), "and holding back");
        const af::StateMachine::HitInfo airGuard = guarder.hitInfoFor(airBlock);
        check(airGuard.guarded, "so it blocks the attack in the air");
        check(airGuard.pushX == -6.0f,
              "and is pushed by `airguard.velocity`, not by the ground `guard.velocity`");
        check(airGuard.pushY == 2.0f, "vertically as well, which a ground block never is");
        // A block's two clocks -- and the air path writes only one of them. The
        // reference's block reaction writes the blocker's `+0x1090` (shake) and
        // `+0x109C` (ctrl) and then, for an air block, stops: `+0x1094`
        // (`guard.hittime`) and `+0x1098` (`guard.slidetime`) are not written at
        // all (`0x46E8D9`-`0x46E950`; GUARD-CHAIN-RE.md §1.2). So the air
        // guard's *only* clock is `airguard.ctrltime`, and the hittime and
        // slidetime slots carry on with whatever the last hit left in them.
        // Reading `airguard.ctrltime` into the hittime slot, as this engine did,
        // reported a number the reference never writes there.
        check(airGuard.ctrlTime == 7, "and its control clock is `airguard.ctrltime`");
        check(airGuard.keepStunClock,
              "while the hitstun clock is left alone, as the reference's air path leaves it");
        check(airGuard.slideTime == airBlock.lastHit().slideTime,
              "and the slidetime keeps the previous hit's value rather than the guard's");
        check(airGuard.stateNumber == af::CommonState::kGuardedAir,
              "in the air guard-hit state");

        // The ground control, on the same HitDef: it still uses the ground pair, so
        // the checks above are about the height rather than about the parse.
        af::Character groundedBlockerChar;
        af::StateMachine groundedBlocker;
        makeIdle(groundedBlockerChar, groundedBlocker);
        groundedBlocker.setOpponent(&guardOpponent);
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            tick.held = af::Button::kRight;
            groundedBlocker.control(tick);
            groundedBlocker.step();
        }
        check(groundedBlocker.holdingBack(), "the grounded blocker should be holding back too");
        const af::StateMachine::HitInfo groundGuard = guarder.hitInfoFor(groundedBlocker);
        check(groundGuard.guarded, "and it blocks as well");
        check(groundGuard.pushX == -4.0f, "pushed by `guard.velocity`");
        check(groundGuard.pushY == 0.0f, "which never lifts a blocker off the floor");
        // The two clocks of a block, and they are different numbers on purpose:
        // `guard.hittime` -- which this HitDef does *not* write, so it arrives
        // through its own default from `ground.hittime = 17` -- is how long the
        // blocker is held, while `guard.ctrltime = 13` is how long it cannot act.
        // The old engine had these swapped: the held time was read straight off
        // `guard.ctrltime`, and `guard.hittime` was parsed but never used at all,
        // so `GetHitVar(hittime)` on a block was the ctrl time and a HitDef that
        // wrote `guard.hittime` was writing to nothing.
        check(groundGuard.hitTime == 17,
              "and held for `guard.hittime`, defaulted from `ground.hittime`");
        check(groundGuard.ctrlTime == 13, "while its own clock stays `guard.ctrltime`");
        check(!groundGuard.keepStunClock,
              "and a ground block seeds the hitstun clock, which the air block is the one "
              "exception to");
    }

    // ---------------------------- the guard reaction, shake to knock-back
    //
    // A blocked hit is a reaction of its own, and in the reference it is a *chain*
    // of six states rather than one. `common1.cns` answers a block with the shake
    // for the blocker's height -- 150 standing, 152 crouching, 154 in the air --
    // and each shake hands over to the knock-back beside it (151, 153, 155), which
    // walks the blocker into a guard *hold* (130 standing, 131 crouching) rather
    // than standing them up. Three things about that chain are why it is checked
    // here and not assumed:
    //
    //   * the shake is `movetype = H`, `physics = N` and `velset = 0,0`. A block is
    //     still a hit reaction while it is held, and the blocker is held *still*
    //     for it, which means the push cannot be applied on the contact tick;
    //   * the knock-back is where the push is applied -- each of 151/153/155 opens
    //     with `HitVelSet x = 1`, 155 with `y = 1` as well, which is "read the
    //     guard velocity back out of the hit that caused this";
    //   * which knock-back the shake ends in is decided by the *input* at the
    //     moment the shake ends, not by the posture the shake was entered in. 150
    //     and 152 carry the same exit, `value = 151 + 2*(command = "holddown")`, so
    //     a blocker that presses down during the shake is knocked back crouching
    //     and a crouch-blocker that lets go is knocked back standing.
    //
    // A block that went straight from the shake to the feet would be wrong in all
    // three: it would shove the blocker on the contact tick, it would hand control
    // back on the tick the shake ended, and it would stand a crouching blocker up.
    {
        // The blockers get an opponent of their own, standing to their left, for
        // the reason the air-block block above gives: a grounded fighter with
        // control auto-turns to face whatever it is pointed at, and the shared
        // dummy has been walked about by the tests in front of this one.
        af::StateMachine blockOpponent;
        blockOpponent.setup(nullptr, 1);
        blockOpponent.setPosition(320.0f);
        blockOpponent.setStageBounds(90.0f, 550.0f);

        const auto press = [](af::StateMachine& machine, uint32_t held, int ticks) {
            for (int i = 0; i < ticks; ++i) {
                af::InputState input;
                input.held = held;
                machine.control(input);
                machine.step();
            }
        };

        // The guard poses, given an action to play. A fixture with no art at all
        // hands every pose straight on -- a state entered with no action behind it
        // reads `AnimTime = 0` on its own first tick, which is what every hand-over
        // in the format waits on -- so 150 -> 151 -> 130 -> 140 -> 0 collapses into
        // one tick and the middle of the chain is unobservable. That collapse is the
        // engine being right about a character that ships no art; it is not what a
        // character looks like, and the checks below are about the chain. The numbers
        // are the reference's own: 120/130/140 with +1 for the crouching pose and +2
        // for the air one, written as `ChangeAnim 130 + (statetype = C) + (statetype =
        // A)*2`. The lengths differ on purpose, so that a pose that is held and a
        // pose that is passed through cannot be confused for one another.
        const auto giveGuardArt = [](af::Character& chr) {
            af::parseAirText("[Begin Action 120]\n0,0, 0,0, 8\n"
                             "[Begin Action 121]\n0,0, 0,0, 8\n"
                             "[Begin Action 122]\n0,0, 0,0, 8\n"
                             "[Begin Action 130]\n0,0, 0,0, 40\n"
                             "[Begin Action 131]\n0,0, 0,0, 40\n"
                             "[Begin Action 132]\n0,0, 0,0, 40\n"
                             "[Begin Action 140]\n0,0, 0,0, 8\n"
                             "[Begin Action 141]\n0,0, 0,0, 8\n"
                             "[Begin Action 142]\n0,0, 0,0, 8\n",
                             chr.animations());
        };

        af::Character guardSwingChar;
        af::StateMachine guardSwing;
        // `pausetime = 4, 10`: the attacker freezes for 4, the blocker is held for
        // 10. The second number is the *shake* clock -- `guard.pausetime`, which this
        // HitDef does not write and which therefore defaults to `pausetime`'s second
        // value -- and it is deliberately not `guard.ctrltime = 6` below, so that a
        // shake measured off the ctrl clock (or off a single tick of it) cannot pass
        // the length checks further down.
        swing("guard.velocity = -3\nguard.ctrltime = 6\nairguard.velocity = -6,-2\n"
              "airguard.ctrltime = 6\npausetime = 4, 10",
              guardSwingChar, guardSwing);

        // --- the standing block: 150 -> 151 -> 130
        af::Character standChar;
        af::StateMachine standBlocker;
        makeIdle(standChar, standBlocker);
        giveGuardArt(standChar);
        standBlocker.setOpponent(&blockOpponent);
        press(standBlocker, af::Button::kRight, 3);
        check(standBlocker.holdingBack(), "the blocker under test should be holding back");
        const af::StateMachine::HitInfo stood = guardSwing.hitInfoFor(standBlocker);
        check(stood.guarded, "and it should block the attack");
        check(stood.stateNumber == af::CommonState::kGuarded,
              "which is the standing shake, state 150");
        // The state-entry block is owed until `hitpausetime <= 0`: a machine
        // inspected at the instant of impact is still wearing its old header, and
        // the freeze outlasts this shake. So the header is checked through a copy
        // of the hit with its freeze taken off -- the pause itself is covered by
        // the dedicated freeze tests -- and the real hit is re-applied below for
        // the shake-length count, which is about the clock.
        af::StateMachine::HitInfo stoodNow = stood;
        stoodNow.victimPause = 0;
        check(standBlocker.applyHit(stoodNow), "the block should land");
        check(standBlocker.stateNumber() == af::CommonState::kGuarded,
              "and put the blocker in the shake");
        // The block is *held* only while the attack is still there: 130's own exit
        // is `command != "holdback" || !inguarddist`, and a host test drives two
        // state machines rather than two fighters, so the engine's half of
        // `inguarddist` -- the opponent's live attack boxes -- is set by hand, as it
        // is in the air-block block below. Without it the blocker would be released
        // on the tick its shake ended, which is the *other* path (see the release
        // checks further down).
        standBlocker.setInGuardDist(true);
        check(standBlocker.moveType() == af::MoveType::Hit,
              "which is a hit reaction -- `movetype = H` -- even though nothing landed on it");
        check(standBlocker.physics() == af::Physics::None,
              "and `physics = N`, so the engine adds neither gravity nor friction to it");
        check(standBlocker.velX() == 0.0f && standBlocker.velY() == 0.0f,
              "and the blocker is held still, because the shake is written `velset = 0,0`");
        check(af::StateMachine::isGuardState(af::CommonState::kGuarded),
              "the shake is one of the guard states, which is what stops a second block "
              "entry out of it");
        check(af::StateMachine::isGuardState(af::CommonState::kGuardedKb) &&
                  af::StateMachine::isGuardState(af::CommonState::kGuardHitKb) &&
                  af::StateMachine::isGuardState(af::CommonState::kGuardedAirKb),
              "and so are the three knock-backs the shakes hand over to");
        check(!af::StateMachine::isGuardState(af::CommonState::kStand),
              "while an ordinary state is not");

        // The push is deliberately *not* applied yet, and this is the half that
        // says so: the blocker does not move a unit while the shake lasts. The
        // shake's *length* is the hit's shake clock -- the second value of
        // `pausetime`, which this HitDef writes as 10 and which `guard.pausetime`
        // defaults to -- and deliberately not `guard.ctrltime = 6`, so a shake
        // measured off the ctrl clock (or off a single tick of shake) cannot pass
        // the count below. The shake is entered on the tick the block lands, so
        // the clock runs one tick beyond the number written.
        //
        // The hit is landed again here, with its freeze, so the count below sees
        // the shake clock the file actually wrote rather than the clock-free copy
        // used for the header checks above.
        check(standBlocker.applyHit(stood), "the real block lands");
        const float shakeX = standBlocker.x();
        int standShakeTicks = 0;
        float shakeTravel = 0.0f;
        while (standBlocker.stateNumber() == af::CommonState::kGuarded && standShakeTicks < 60) {
            // Measured before the tick runs, so the tick the shake *ends* on -- the
            // one the push lands on, in 151 -- is not counted as travel.
            shakeTravel = std::max(shakeTravel, std::fabs(standBlocker.x() - shakeX));
            press(standBlocker, af::Button::kRight, 1);
            ++standShakeTicks;
        }
        check(standShakeTicks == 11,
              "the blocker should be held in the shake for its clock -- 10 ticks, plus "
              "the tick it is entered on -- and not for a single tick of it");
        check(shakeTravel == 0.0f, "with no push at all while the shake lasts");
        check(standBlocker.stateNumber() == af::CommonState::kGuardedKb,
              "when it ends the blocker is handed to 151, the standing knock-back");
        // 151 walks the blocker into 130 rather than dropping it to its feet. For a
        // character that ships common1.cns the length of 151 is the file's own
        // action; these blockers have none, so what holds the built-in 151 is the
        // stun clock, exactly as it holds any other built-in reaction (see
        // `builtinExitsOnShake`'s comment).
        //
        // That clock is `guard.hittime`, and how long the reference keeps a blocker
        // in its guard knock-back is no longer an open question: `+0x1094` is
        // decremented while it is `>= 0` (so it runs `H … 1, 0, -1`), it is frozen
        // while the shake clock is non-zero (so it starts only after the shake),
        // and 151 leaves on `HitOver`, i.e. `hittime < 0`. With this HitDef writing
        // no `guard.hittime` -- so the default is `ground.hittime = 12` -- the
        // clock is already running on the tick 151 is entered: the loop below sees
        // it read 11 down through 0, then -1 on the tick the hand-over fires.
        // The block is therefore held for shake then hittime, one after the other,
        // not for the longer of the two: 11 + 13 ticks here, where `max` would
        // have ended the knock-back when the shake did.
        int guardKbTicks = 0;
        while (standBlocker.stateNumber() == af::CommonState::kGuardedKb && guardKbTicks < 60) {
            press(standBlocker, af::Button::kRight, 1);
            ++guardKbTicks;
        }
        check(guardKbTicks == 13,
              "which it is held in for `guard.hittime` after the shake -- eleven down "
              "through zero, and the tick the clock reads -1");
        check(standBlocker.stateNumber() == af::CommonState::kGuardStand,
              "and then the blocker is in 130, the standing guard hold");
        check(standBlocker.prevStateNumber() == af::CommonState::kGuardedKb,
              "by way of 151 rather than being handed back to 0");
        check(standBlocker.velX() < 0.0f && standBlocker.velX() >= -3.0f,
              "and the push `guard.velocity` is applied there, less the friction of the "
              "tick it lands on");
        const float pushedX = standBlocker.x();
        press(standBlocker, af::Button::kRight, 2);
        check(standBlocker.x() < pushedX, "which carries the blocker backwards");
        check(standBlocker.stateNumber() == af::CommonState::kGuardStand,
              "while the key stays held, so the block is still up");

        // --- the two clocks a character reads back
        //
        // `GetHitVar(hittime)` and `GetHitVar(hitshaketime)` are the reference's
        // live counters (`+0x1094` and `+0x1090`), not the numbers the HitDef
        // wrote, and `HitOver` is `hittime < 0` rather than `== 0`. Those are the
        // clocks a character's own get-hit states are written against -- the
        // shipped `common1.cns` waits on `Time = GetHitVar(hittime)` -- so they are
        // measured directly here instead of being inferred from the built-in chain
        // above. A fresh blocker is used so the readings start from the contact.
        af::Character clockChar;
        af::StateMachine clockBlocker;
        makeIdle(clockChar, clockBlocker);
        clockBlocker.setOpponent(&blockOpponent);
        press(clockBlocker, af::Button::kRight, 3);
        check(clockBlocker.holdingBack(), "a second blocker should hold back too");
        check(clockBlocker.applyHit(guardSwing.hitInfoFor(clockBlocker)),
              "and take the same block");
        const auto clock = [&](const char* key) {
            return clockBlocker.trigger("gethitvar", {af::Value::text(key)}).toFloat();
        };
        check(clock("hittime") == 12.0f,
              "GetHitVar(hittime) starts at `guard.hittime`, defaulted to twelve");
        check(clock("hitshaketime") == 10.0f,
              "and GetHitVar(hitshaketime) at the shake clock, ten");
        check(clockBlocker.trigger("hitover", {}).toFloat() == 0.0f,
              "neither of which is over on the contact tick");
        // The freeze covers the shake, and the hitstun clock is frozen with it --
        // the reference gates `+0x1094` on `+0x1090 == 0` -- so ten paused ticks
        // leave the countdown untouched.
        press(clockBlocker, af::Button::kRight, 10);
        check(clock("hittime") == 12.0f,
              "the countdown does not run during the freeze, which is the shake");
        check(clock("hitshaketime") == 0.0f, "while the shake clock does run it out");
        check(clockBlocker.stateNumber() == af::CommonState::kGuarded,
              "with the blocker still in the shake on the last frozen tick");
        // The first unfrozen tick ends the shake and hands over to 151; the same
        // tick's update is where the countdown starts.
        press(clockBlocker, af::Button::kRight, 1);
        check(clockBlocker.stateNumber() == af::CommonState::kGuardedKb,
              "the next tick is the knock-back");
        check(clock("hittime") == 11.0f,
              "and the countdown has taken its first tick, not started from zero");
        // Run it out. The countdown reads 0 for one tick before it reads -1, and
        // `HitOver` is the strict comparison, so the trigger must stay false on the
        // tick the number reads zero and only turn true on the tick after it.
        int ticksToZero = 0;
        while (clock("hittime") > 0.0f && ticksToZero < 60) {
            press(clockBlocker, af::Button::kRight, 1);
            ++ticksToZero;
        }
        check(clock("hittime") == 0.0f, "the countdown reaches zero, and stops there");
        check(clockBlocker.trigger("hitover", {}).toFloat() == 0.0f,
              "which is not `HitOver` -- the trigger is `hittime < 0`");
        press(clockBlocker, af::Button::kRight, 1);
        check(clock("hittime") == -1.0f,
              "the next tick carries it to -1, where the reference's clock stops");
        check(clockBlocker.trigger("hitover", {}).toFloat() != 0.0f,
              "and that is the tick `HitOver` turns true");

        // --- letting go: 130 -> 140, and 140 is a pose rather than a hold
        //
        // A block is held only while *both* halves are true: back down, and the
        // opponent still inside guard distance. The reference's 130 writes that as
        // one controller, `Stop Guarding: trigger1 = command != "holdback"; trigger2 =
        // !inguarddist -> 140`, and on the tick either half lets go the blocker does
        // not fall out of the guard to its feet. It leaves through 140, the guard-*end*
        // pose -- the one of the five with `ctrl = 1`, and the one whose own hand-over
        // the file comments out as `;Implemented within engine`, which is the exit
        // checked at the end of this. The key stays down throughout here: what ends is
        // the attack, not the thumb.
        standBlocker.setInGuardDist(false);
        press(standBlocker, af::Button::kRight, 1);
        check(standBlocker.prevStateNumber() == af::CommonState::kGuardStand,
              "an attack that ends takes the blocker out of 130");
        check(standBlocker.stateNumber() == af::CommonState::kGuardEnd,
              "into 140, the guard-end pose, rather than back to its feet");
        check(standBlocker.ctrl(),
              "which is the one guard state that hands control back, on its first tick");
        check(!af::StateMachine::isGuardState(af::CommonState::kGuardEnd),
              "and is not one of the states a block is held in -- 140 is where a block "
              "is left, not where it is kept");

        int poseTicks = 0;
        while (standBlocker.stateNumber() == af::CommonState::kGuardEnd && poseTicks < 60) {
            press(standBlocker, af::Button::kRight, 1);
            ++poseTicks;
        }
        check(poseTicks >= 4, "so the pose is held for as long as its action runs");
        check(standBlocker.stateNumber() == af::CommonState::kWalk,
              "and then the fighter is on its feet and moving again -- with back still "
              "held and nothing left to block, walking away");

        // --- the crouching block: 152 -> 153 -> 131, and it stays crouched
        af::Character crouchChar;
        af::StateMachine crouchBlocker;
        makeIdle(crouchChar, crouchBlocker);
        giveGuardArt(crouchChar);
        crouchBlocker.setOpponent(&blockOpponent);
        press(crouchBlocker, af::Button::kDown | af::Button::kRight, 30);
        check(crouchBlocker.stateNumber() == af::CommonState::kCrouch,
              "the second blocker should be crouching");
        check(crouchBlocker.holdingBack(), "with back held as well");
        const af::StateMachine::HitInfo crouched = guardSwing.hitInfoFor(crouchBlocker);
        check(crouched.guarded, "so the attack is blocked from the crouch");
        check(crouched.stateNumber == af::CommonState::kGuardHit,
              "into 152, the crouching shake");
        af::StateMachine::HitInfo crouchedNow = crouched;
        crouchedNow.victimPause = 0;  // the header now, the clock below; see the standing block
        check(crouchBlocker.applyHit(crouchedNow), "the crouching block should land");
        crouchBlocker.setInGuardDist(true);
        check(crouchBlocker.stateNumber() == af::CommonState::kGuardHit &&
                  crouchBlocker.stateType() == af::StateType::Crouch,
              "and it is a crouching state -- `type = C` -- so the blocker does not stand "
              "up to be knocked back");
        check(crouchBlocker.moveType() == af::MoveType::Hit && crouchBlocker.velX() == 0.0f,
              "a held hit reaction with no push on the contact tick");

        check(crouchBlocker.applyHit(crouched), "the real crouching block lands");
        int crouchShakeTicks = 0;
        while (crouchBlocker.stateNumber() == af::CommonState::kGuardHit && crouchShakeTicks < 60) {
            press(crouchBlocker, af::Button::kDown | af::Button::kRight, 1);
            ++crouchShakeTicks;
        }
        check(crouchShakeTicks == 11,
              "the crouching shake is the same clock as the standing one");
        check(crouchBlocker.stateNumber() == af::CommonState::kGuardHitKb,
              "and 152 hands over to 153, the crouching knock-back -- not to the standing "
              "one");
        int crouchKbTicks = 0;
        while (crouchBlocker.stateNumber() == af::CommonState::kGuardHitKb && crouchKbTicks < 60) {
            press(crouchBlocker, af::Button::kDown | af::Button::kRight, 1);
            ++crouchKbTicks;
        }
        check(crouchKbTicks == 13,
              "and its knock-back is held for the same `guard.hittime` as 151's");
        check(crouchBlocker.stateNumber() == af::CommonState::kGuardCrouch,
              "and the crouching chain ends in 131, the crouching guard hold");
        check(crouchBlocker.prevStateNumber() == af::CommonState::kGuardHitKb,
              "by way of 153, the crouching knock-back -- not the standing one");
        check(crouchBlocker.stateType() == af::StateType::Crouch,
              "with the blocker still crouched, which is the whole difference from the "
              "standing chain");

        // And a crouching block that loses the attack leaves the same way, through
        // 140 in its *crouching* pose -- 141, by the same `+1` every guard pose
        // carries for the crouch. 140 keeps the posture it was entered in (`Hi to Lo`
        // / `Lo to Hi` move the stick between the two *guard holds*, and neither fires
        // for a fighter whose block has just ended), and its own exit is the one place
        // the height is asked for by name: `value = (statetype = C)*11 +
        // (statetype = A)*51`. So the fighter comes out of the guard-end pose in 11,
        // still crouched, rather than standing up out of a crouching block.
        crouchBlocker.setInGuardDist(false);
        press(crouchBlocker, af::Button::kDown | af::Button::kRight, 1);
        check(crouchBlocker.prevStateNumber() == af::CommonState::kGuardCrouch &&
                  crouchBlocker.stateNumber() == af::CommonState::kGuardEnd,
              "a crouching block that loses the attack leaves 131 through 140 as well");
        check(crouchBlocker.stateType() == af::StateType::Crouch &&
                  crouchBlocker.physics() == af::Physics::Crouch,
              "in the crouching guard-end pose, `type = C, physics = C` like the hold it "
              "came from");
        press(crouchBlocker, af::Button::kDown | af::Button::kRight, 20);
        check(crouchBlocker.stateNumber() == af::CommonState::kCrouch,
              "and the guard-end pose hands the fighter down into 11, still crouched");

        // --- and the height comes from the input, not from the shake entered
        //
        // 150 and 152 carry the *same* exit, so this is the check that keeps the two
        // shakes from being written as two fixed routes: a stand-blocker that presses
        // down during the shake is knocked back crouching and settles in 131.
        af::Character tallChar;
        af::StateMachine tallBlocker;
        makeIdle(tallChar, tallBlocker);
        tallBlocker.setOpponent(&blockOpponent);
        press(tallBlocker, af::Button::kRight, 3);
        const af::StateMachine::HitInfo tall = guardSwing.hitInfoFor(tallBlocker);
        check(tall.stateNumber == af::CommonState::kGuarded,
              "a standing blocker enters the standing shake");
        check(tallBlocker.applyHit(tall), "and takes the block");
        tallBlocker.setInGuardDist(true);
        // Ten ticks of the eleven the shake lasts, with back held and *not* down: the
        // blocker is still standing when the stick moves. The height is read when the
        // shake *ends*, not when it was entered, which is the whole difference
        // between 150 and 152 -- they carry the same exit.
        int tallShakeTicks = 0;
        while (tallBlocker.stateNumber() == af::CommonState::kGuarded && tallShakeTicks < 10) {
            press(tallBlocker, af::Button::kRight, 1);
            ++tallShakeTicks;
        }
        check(tallBlocker.stateNumber() == af::CommonState::kGuarded,
              "a standing blocker should still be in its own shake ten ticks in");
        press(tallBlocker, af::Button::kDown | af::Button::kRight, 1);
        check(tallBlocker.stateNumber() == af::CommonState::kGuardHitKb,
              "but pressing down on the tick that shake ends is knocked back crouching, "
              "into 153 -- the crouching knock-back, not the standing one");
        int tallKbTicks = 0;
        while (tallBlocker.stateNumber() == af::CommonState::kGuardHitKb && tallKbTicks < 60) {
            press(tallBlocker, af::Button::kDown | af::Button::kRight, 1);
            ++tallKbTicks;
        }
        check(tallBlocker.stateNumber() == af::CommonState::kGuardCrouch &&
                  tallBlocker.prevStateNumber() == af::CommonState::kGuardHitKb,
              "and it settles in 131, the crouching guard hold, even though the shake was "
              "150's");

        // --- and a guard can be taken up again out of 140
        //
        // The other half of 140's `ctrl = 1`: it is the state a blockstring comes back
        // up through. A guard is entered the same way from every state -- back held,
        // opponent attacking, inside guard distance -- so a fighter that lost one
        // attack and is swung at again goes back up through 120, the guard-*start*
        // pose, rather than standing there and taking the second hit in the face. That
        // this works is exactly what `isGuardState(140)` being false buys: were 140
        // still counted as a guard state, the entry would refuse it and the fighter
        // would stand in its guard-end pose until the next hit landed.
        //
        // The fixture writes `[Statedef 120]` because the entry is the *engine's* only
        // where the character has a guard-start state to enter -- a character with no
        // 120 of its own keeps blocking without a pose, which is what the guards above
        // are doing -- and it writes no 130 or 140, which is what keeps the engine's
        // built-in chain in play rather than the file's own controllers. `anim = 120`
        // is written out for the reason cns.html gives: "If you do not want P1 to change
        // animation at the start of the state, omit this parameter", so a state that
        // omits it keeps whatever pose the fighter was already in and this one would
        // be handed straight on to 130 with nothing to draw.
        af::Character reboundChar;
        af::StateMachine rebound;
        reboundChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[Statedef 120]\ntype = U\nphysics = U\nanim = 120\n",
            "[Statedef -1]\n");
        rebound.setup(&reboundChar, -1);
        rebound.setOpponent(&blockOpponent);
        rebound.setPosition(360.0f);
        rebound.setStageBounds(90.0f, 550.0f);
        giveGuardArt(reboundChar);
        rebound.setInGuardDist(true);
        press(rebound, af::Button::kRight, 1);
        check(rebound.stateNumber() == af::CommonState::kGuardStart,
              "holding back in guard distance goes up through 120, the guard-start pose");
        press(rebound, af::Button::kRight, 10);
        check(rebound.stateNumber() == af::CommonState::kGuardStand,
              "and the start pose settles into the standing guard hold");

        rebound.setInGuardDist(false);
        press(rebound, af::Button::kRight, 1);
        check(rebound.stateNumber() == af::CommonState::kGuardEnd,
              "losing the attack drops the block through 140 as it did above");

        // The second attack. Nothing about the fighter has changed -- back is still
        // held -- so the guard goes straight back up, out of the guard-end pose. (A
        // host test drives no arena, so what the opponent is doing is this flag.)
        rebound.setInGuardDist(true);
        press(rebound, af::Button::kRight, 1);
        check(rebound.prevStateNumber() == af::CommonState::kGuardEnd &&
                  rebound.stateNumber() == af::CommonState::kGuardStart,
              "and being swung at again takes the guard back up, out of 140 into 120");

        // --- the air block: 154 -> 155, and the landing asks a question
        //
        // An airborne blocker is held in place by its shake in the most literal
        // sense there is: 154 is `physics = N`, so the engine's gravity does not
        // pull it down while the block is being held, which is what stops an air
        // block from turning into a landing. What happens when it *does* reach the
        // floor is the reference's `[State 155, 6]` and `[State 155, 7]`: holding
        // back inside guard distance lands it in the standing guard hold, and
        // anything else lands it on its feet in 52. `inGuardDist_` is the engine's
        // half of "inside guard distance" and is set from the attacker's live
        // attack boxes, so it is set by hand here -- a host test has no arena
        // driving the two machines past each other.
        const auto jumpBack = [&](af::Character& chr, af::StateMachine& machine) {
            makeIdle(chr, machine);
            machine.setOpponent(&blockOpponent);
            for (int tick = 0; tick < 30; ++tick) {
                af::InputState input;
                input.held = tick < 4 ? af::Button::kUp : af::Button::kRight;
                machine.control(input);
                machine.step();
                if (tick >= 4 && machine.stateType() == af::StateType::Air && machine.y() > 0.5f &&
                    machine.holdingBack()) {
                    return;
                }
            }
        };

        af::Character airKeepChar;
        af::StateMachine airKeep;
        jumpBack(airKeepChar, airKeep);
        check(airKeep.stateType() == af::StateType::Air && airKeep.holdingBack(),
              "the airborne blocker should be up in the air, holding back");
        const af::StateMachine::HitInfo aloft = guardSwing.hitInfoFor(airKeep);
        check(aloft.guarded, "and it blocks in the air");
        check(aloft.stateNumber == af::CommonState::kGuardedAir,
              "into 154, the air guard shake");
        check(aloft.pushY > 0.0f, "which it is pushed up by as well as back");
        af::StateMachine::HitInfo aloftNow = aloft;
        aloftNow.victimPause = 0;  // the pose's header now; see the standing block
        airKeep.setInGuardDist(true);
        check(airKeep.applyHit(aloftNow), "the air block should land");
        check(airKeep.stateNumber() == af::CommonState::kGuardedAir &&
                  airKeep.physics() == af::Physics::None,
              "putting the blocker in an air state with no physics behind it");
        // The air block writes the ctrl clock and the shake clock, and nothing
        // else: `+0x1094` keeps its `-1`, so the blocker has no hitstun to run
        // out and `hitover` is true from the first tick. The pose is held by
        // `HitShakeOver` instead (154's own exit), which is the next check.
        check(airKeep.trigger("gethitvar", {af::Value::text("hittime")}).toFloat() == -1.0f,
              "an air block must not start a hitstun clock -- `GetHitVar(hittime)` stays -1, "
              "as the reference leaves `+0x1094`");
        check(airKeep.trigger("gethitvar", {af::Value::text("ctrltime")}).toFloat() == 6.0f,
              "while `GetHitVar(ctrltime)` is `airguard.ctrltime`");
        // Re-land the real block so the hang below is held by the file's own
        // shake clock rather than by the clock-free copy.
        check(airKeep.applyHit(aloft), "the real air block lands");
        const float hangingY = airKeep.y();
        press(airKeep, af::Button::kRight, 3);
        check(airKeep.stateNumber() == af::CommonState::kGuardedAir && airKeep.y() == hangingY,
              "so it hangs where it was hit for the block's whole clock, not falling");

        int airShakeTicks = 0;
        while (airKeep.stateNumber() == af::CommonState::kGuardedAir && airShakeTicks < 60) {
            press(airKeep, af::Button::kRight, 1);
            ++airShakeTicks;
        }
        check(airKeep.stateNumber() == af::CommonState::kGuardedAirKb,
              "and the air shake hands over to 155, the air knock-back");
        check(airKeep.velY() > 0.0f && airKeep.velX() < 0.0f,
              "where the air block's push is applied -- up and away, from "
              "`airguard.velocity`");

        int fallTicks = 0;
        while (airKeep.stateType() == af::StateType::Air && fallTicks < 300) {
            press(airKeep, af::Button::kRight, 1);
            ++fallTicks;
        }
        check(fallTicks < 300, "the blocker should come back down");
        check(airKeep.prevStateNumber() == af::CommonState::kGuardedAirKb,
              "having fallen out of 155");
        check(airKeep.stateNumber() == af::CommonState::kGuardStand,
              "and a blocker that lands still holding back inside guard distance keeps the "
              "block up, landing in 130 rather than on its feet");

        // The control for the other half of that choice: the same fall with the
        // same key held, but nothing in guard distance any more. This is the check
        // that stops the landing from being written as "an air block always lands
        // blocking" -- which would leave a blocker holding its guard at an opponent
        // that is no longer attacking.
        af::Character airDropChar;
        af::StateMachine airDrop;
        jumpBack(airDropChar, airDrop);
        const af::StateMachine::HitInfo dropped = guardSwing.hitInfoFor(airDrop);
        check(dropped.guarded && dropped.stateNumber == af::CommonState::kGuardedAir,
              "the control blocker should block in the air as well");
        check(airDrop.applyHit(dropped), "and take the block");
        int dropTicks = 0;
        while (airDrop.stateType() == af::StateType::Air && dropTicks < 300) {
            press(airDrop, af::Button::kRight, 1);
            ++dropTicks;
        }
        check(dropTicks < 300, "the control blocker should come back down too");
        check(airDrop.prevStateNumber() == af::CommonState::kGuardedAirKb,
              "out of 155 the same way");
        check(airDrop.stateNumber() == af::CommonState::kLand,
              "but out of guard distance it lands in 52, on its feet, block down");

        // --- the air guard *hold*, 132, and the physics that comes with a posture
        //
        // Everything above is a block that is *reacted* to. This is the other half of
        // an air block: a fighter that is attacked while airborne and simply keeps its
        // guard up, which is 132. Two things about it are written down rather than
        // inherited, and both are the reference's own `[Statedef 132]`:
        //
        //   * it falls. Its header is `physics = N` -- no physics at all -- and the
        //     fall is added by its own controllers, `VelAdd y = Const(movement.yaccel)`
        //     every tick, so an air-blocking fighter comes down out of the sky rather
        //     than hanging at the height it blocked at;
        //   * it lands on `sysvar(0)`, `(pos y >= 0) && (vel y > 0)`, and the landing
        //     asks the same question of the blocker that 155's does: 130 while it is
        //     still holding back inside guard distance, 52 otherwise.
        //
        // 132 is only ever *entered* through 120 -- the guard start, which the engine
        // will only use for a character that ships one -- so this fixture writes
        // `[Statedef 120]` and nothing else, and the engine's built-in chain carries
        // it from there.
        const auto makeAirGuarder = [&](af::Character& chr, af::StateMachine& machine,
                                        bool inGuardDist) {
            chr.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
                "[Statedef 120]\ntype = U\nphysics = U\nanim = 120\n",
                "[Statedef -1]\n");
            giveGuardArt(chr);
            machine.setup(&chr, -1);
            machine.setOpponent(&blockOpponent);
            machine.setPosition(360.0f);
            machine.setStageBounds(90.0f, 550.0f);
            // Set after `setup`, which builds the machine afresh.
            machine.setInGuardDist(inGuardDist);
            // Up, then back: the jump takes off with up, and holding back from the
            // takeoff on is what puts the guard up in the air.
            for (int tick = 0; tick < 40; ++tick) {
                af::InputState input;
                input.held = tick < 4 ? af::Button::kUp : af::Button::kRight;
                machine.control(input);
                machine.step();
                if (tick >= 4 && machine.stateType() == af::StateType::Air &&
                    machine.stateNumber() == af::CommonState::kGuardAir) {
                    return;
                }
            }
        };

        af::Character airHoldChar;
        af::StateMachine airHold;
        makeAirGuarder(airHoldChar, airHold, true);
        check(airHold.stateNumber() == af::CommonState::kGuardAir,
              "an airborne blocker holding back in guard distance goes up through 120 into "
              "the air guard hold, 132");
        check(airHold.stateType() == af::StateType::Air &&
                  airHold.physics() == af::Physics::Air,
              "which is airborne with the engine's air physics behind it -- the fall 132 "
              "writes out by hand, as `VelAdd y = Const(movement.yaccel)`");
        check(airHold.holdingBack() && airHold.ctrl() == false,
              "holding back, with no control: it is a guard, not a state it can walk out of");

        const float blockedAtY = airHold.y();
        int descendTicks = 0;
        while (airHold.stateNumber() == af::CommonState::kGuardAir &&
               airHold.y() >= blockedAtY - 0.5f && descendTicks < 30) {
            press(airHold, af::Button::kRight, 1);
            ++descendTicks;
        }
        check(airHold.stateNumber() == af::CommonState::kGuardAir && airHold.y() < blockedAtY,
              "and it comes down out of the sky while the guard is held");

        int holdFallTicks = 0;
        while (airHold.stateType() == af::StateType::Air && holdFallTicks < 300) {
            press(airHold, af::Button::kRight, 1);
            ++holdFallTicks;
        }
        check(holdFallTicks < 300, "the air-blocking fighter should reach the floor");
        check(airHold.prevStateNumber() == af::CommonState::kGuardAir,
              "landing out of 132 rather than falling through it");
        check(airHold.stateNumber() == af::CommonState::kGuardStand,
              "and it lands in the guard hold, 130, still blocking");

        // The control: the same hold, released in the air before the floor. 132's
        // `Stop Guarding` sends that fighter through 140 -- which, being an air pose,
        // keeps the air physics and keeps falling -- and it comes down with its arms
        // down: out of 140's own pose-out into 51, or out of the floor into 52, both of
        // which are the fighter on its feet rather than a fighter still blocking.
        af::Character airReleaseChar;
        af::StateMachine airRelease;
        makeAirGuarder(airReleaseChar, airRelease, true);
        check(airRelease.stateNumber() == af::CommonState::kGuardAir,
              "the control blocker should be in the air guard hold as well");
        airRelease.setInGuardDist(false);
        press(airRelease, af::Button::kRight, 1);
        check(airRelease.stateNumber() == af::CommonState::kGuardEnd &&
                  airRelease.stateType() == af::StateType::Air,
              "letting the guard go in the air drops it into 140 in its *air* pose, 142");
        check(airRelease.physics() == af::Physics::Air,
              "which keeps falling -- 140's `[State 140, 4]` is the `statetype = A` arm of "
              "the same physics trio, and without it the release hangs");
        int releaseFallTicks = 0;
        while (airRelease.stateType() == af::StateType::Air && releaseFallTicks < 300) {
            press(airRelease, af::Button::kRight, 1);
            ++releaseFallTicks;
        }
        check(releaseFallTicks < 300, "the released blocker should reach the floor too");
        check(airRelease.stateNumber() == af::CommonState::kLand,
              "and land on its feet in 52, block down");
    }

    // ------------------------------------------------------------ StateTypeSet
    //
    // The one controller that changes what a state *is*: its `statetype` (S, C, A or
    // L, "the state type of P1 in that state"), its `movetype` (I, A or H, what it is
    // doing) and its `physics` (S, C, A or N, how it moves). All three parameters are
    // optional and each one "defaults to no change", which is the sentence that makes
    // the controller useful in the middle of a state rather than only at its head --
    // and the type and the physics are deliberately two different questions:
    // `[Statedef 132]`, the air guard hold, is `type = A` with `physics = N`, and its
    // own controllers do the falling by hand. A release out of that guard asks for the
    // physics back and nothing else, which is `[State 140, 4]`: `StateTypeSet` with a
    // single `physics` line. Dropping that parameter -- reading it and throwing it away,
    // as the engine did -- leaves a fighter that blocked in the air hanging in it.
    {
        af::Character splitChar;
        af::StateMachine split;
        splitChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, hand]\ntype = StateTypeSet\ntrigger1 = time = 0\nphysics = N\n",
            "[Statedef -1]\n");
        split.setup(&splitChar, -1);
        split.setOpponent(&dummy);
        split.setPosition(360.0f);
        split.setStageBounds(90.0f, 550.0f);
        af::InputState still;
        split.control(still);
        split.step();
        check(split.physics() == af::Physics::None,
              "a `physics = N` in a StateTypeSet takes the engine's physics off a fighter");
        check(split.stateType() == af::StateType::Stand,
              "leaving the state type it did not name alone -- S, as the header wrote it");
        check(split.moveType() == af::MoveType::Idle, "and the movetype likewise");

        // All three at once, in the shape a state header would write them, so that the
        // controller is shown to reach each of the three fields rather than only the
        // one the guard chain happens to need.
        af::Character upChar;
        af::StateMachine up;
        upChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, up]\ntype = StateTypeSet\ntrigger1 = time = 0\n"
            "statetype = A\nmovetype = A\nphysics = N\n",
            "[Statedef -1]\n");
        up.setup(&upChar, -1);
        up.setOpponent(&dummy);
        up.setPosition(360.0f);
        up.setStageBounds(90.0f, 550.0f);
        up.control(still);
        up.step();
        check(up.stateType() == af::StateType::Air && up.moveType() == af::MoveType::Attack &&
                  up.physics() == af::Physics::None,
              "and all three together move the fighter into the air, attacking, with no "
              "physics of its own");
    }

    // ---------------------------------- the guard chain through a real hit
    //
    // The block above sets `inguarddist` by hand, because a host test drives two
    // state machines rather than two fighters. This one goes in the way a match
    // does: two `Fighter`s with real collision boxes, `updateGuardDistances`
    // computing the flag from the attacker's own HitDef exactly as the fight screen
    // calls it, and the contact resolved by `Fighter::tryHit`. It is
    // here for the two things the hand-set flag cannot show -- that the engine
    // ever sets it, and that a swing which is *blocked* is not also a swing that
    // lands.
    {
        const float floorY = 192.0f;

        af::Character strikerChar;
        strikerChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, swing]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n"
            "guard.velocity = -3\nguard.ctrltime = 6\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 300]\nClsn1: 1\nClsn1[0] = 24,-70,56,-40\n"
                         "300,0, 0,0, -1\n",
                         strikerChar.animations());

        af::Character blockerChar;
        blockerChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                                  "[Statedef -1]\n");
        af::parseAirText("[Begin Action 0]\nClsn2Default: 1\nClsn2[0] = -12,-64,12,0\n"
                         "0,0, 0,0, -1\n",
                         blockerChar.animations());

        // The striker on the left with the blocker facing it, close enough that
        // the striker's own box reaches: `worldBox` mirrors a box by facing, so
        // the 24..56 written in front of the striker is the 344..376 the blocker
        // stands in.
        // Nothing is in the air in this pair; the beam case has its own block
        // below.
        const std::vector<af::Projectile> noBeams;
        const auto tickPair = [&](af::Fighter& a, af::Fighter& b, uint32_t held) {
            af::InputState none;
            a.control(none);
            a.step(b);
            af::InputState input;
            input.held = held;   // the blocker faces left, so right is back
            b.control(input);
            b.step(a);
            af::Fighter::separate(a, b);
            af::Fighter::updateGuardDistances(a, b, noBeams, floorY);
        };

        af::Fighter striker;
        af::Fighter blocker;
        striker.setup(&strikerChar, 320.0f, 1);
        blocker.setup(&blockerChar, 366.0f, -1);
        striker.setScale(1.0f);
        blocker.setScale(1.0f);

        bool engaged = false;
        for (int i = 0; i < 30 && !engaged; ++i) {
            tickPair(striker, blocker, af::Button::kRight);
            engaged = striker.machine().hasArmedHitDef() && striker.wouldHit(blocker, floorY);
        }
        check(engaged,
              "the striker's swing should reach the blocker, or nothing below is about a block");
        check(blocker.machine().inGuardDist(),
              "and the blocker should be inside guard distance -- 46 units apart against the "
              "160 the player variables give an unwritten `guard.dist`");
        check(blocker.machine().holdingBack(), "with back held, which is what makes a block");

        bool landed = false;
        check(striker.tryHit(blocker, floorY, &landed), "the swing should meet the blocker");
        // `landed` is the fight loop's "the swing connected", and a *block* is a
        // connect: the combo counter counts a blocked string as hits with no damage.
        // What separates the two here is the attacker's own `moveguarded`, which is
        // the flag a character reads back to know its move was turned away.
        check(landed, "which connects -- being stopped by a guard is still a connect");
        check(striker.machine().moveGuarded(),
              "but it is blocked, which is the attacker's own half of it");
        check(blocker.machine().stateNumber() == af::CommonState::kGuarded &&
                  blocker.machine().velX() == 0.0f,
              "leaving the blocker in the standing shake with no push yet");
        check(blocker.machine().life() == blocker.machine().maxLife(),
              "and no damage, because the block let nothing through");

        int shaken = 0;
        while (blocker.machine().stateNumber() == af::CommonState::kGuarded && shaken < 60) {
            tickPair(striker, blocker, af::Button::kRight);
            ++shaken;
        }
        check(blocker.machine().stateNumber() == af::CommonState::kGuardedKb,
              "and the chain leaves the shake through 151, the standing knock-back");
        int shakenKb = 0;
        while (blocker.machine().stateNumber() == af::CommonState::kGuardedKb && shakenKb < 60) {
            tickPair(striker, blocker, af::Button::kRight);
            ++shakenKb;
        }
        check(blocker.machine().stateNumber() == af::CommonState::kGuardStand &&
                  blocker.machine().prevStateNumber() == af::CommonState::kGuardedKb,
              "and the chain runs 150 -> 151 -> 130 through the real hit path as well");

        // The control: the same swing at the same distance, with nobody holding
        // back. Without it every check above would pass for a move that simply
        // never lands.
        af::Fighter openStriker;
        af::Fighter openBlocker;
        openStriker.setup(&strikerChar, 320.0f, 1);
        openBlocker.setup(&blockerChar, 366.0f, -1);
        openStriker.setScale(1.0f);
        openBlocker.setScale(1.0f);
        bool openEngaged = false;
        for (int i = 0; i < 30 && !openEngaged; ++i) {
            tickPair(openStriker, openBlocker, 0);
            openEngaged =
                openStriker.machine().hasArmedHitDef() && openStriker.wouldHit(openBlocker, floorY);
        }
        check(openEngaged, "the control swing should reach the standing blocker too");
        bool openLanded = false;
        check(openStriker.tryHit(openBlocker, floorY, &openLanded), "and it should connect");
        check(openLanded && !openStriker.machine().moveGuarded(),
              "and land, because nothing was held back");
        check(openBlocker.machine().stateNumber() == af::CommonState::kHitHigh,
              "putting the blocker in the standing *hit* shake, 5000, rather than in a block");
    }

    // ------------------------------- the guard distance is a number, not a reach
    //
    // The block above runs the whole chain through a real hit. This one is about the
    // single number that decides whether a held back was a block *at all*, which the
    // reference takes off the attacker's HitDef rather than off how far its art
    // happens to reach. Three readings of the same number, each of which the old
    // measurement -- the live attack box, grown by a hardcoded 24 -- got backwards:
    //
    //   no `guard.dist` written   -> the player variable, 160, which reaches well
    //                                past a short attack's own box
    //   `guard.dist = 50`         -> 50, whatever the box does
    //   `guard.dist = 0`          -> nothing, however far the box reaches
    //
    // and the beam, which is a case the old measurement could not see at all: the
    // fighter that fired it is standing in a recovery state with nothing armed, and
    // its own 90 is a different number from the melee 160.
    {
        const float floorY = 192.0f;

        // A striker whose only move arms a HitDef with `hitExtra` on it, and whose
        // attack box spans `box`. Every fighter here is at scale 1, so a distance
        // written in the character's own space is that same number of world units.
        const auto makeStriker = [](af::Character& chr, const char* hitExtra, const char* box) {
            std::string text =
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
                "[State 0, swing]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
                "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
                "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n";
            text += hitExtra;
            chr.loadBehaviour(text, "[Statedef -1]\n");
            af::parseAirText(std::string("[Begin Action 300]\nClsn1: 1\nClsn1[0] = ") + box +
                                 "\n300,0, 0,0, -1\n",
                             chr.animations());
        };

        // A blocker with a body and nothing else: it never swings, so everything the
        // flag below says came from the other side.
        const auto makeBlocker = [](af::Character& chr) {
            chr.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                              "[Statedef -1]\n");
            af::parseAirText("[Begin Action 0]\nClsn2Default: 1\nClsn2[0] = -12,-64,12,0\n"
                             "0,0, 0,0, -1\n",
                             chr.animations());
        };

        // Runs the pair for a few ticks at a fixed `gap` -- `separate` is deliberately
        // *not* called, because what is being measured here is the distance and not the
        // shove -- and answers with the blocker's flag. The striker is on the left
        // facing right and the blocker on the right facing left.
        //
        // `expectArmed` is there for the one case where the point is that nothing is
        // armed: `inguarddist` does not ask for a HitDef, and a fixture that silently
        // stopped arming one would otherwise look like a passing test of the opposite.
        std::vector<af::Projectile> beams;
        const auto guardDistAt = [&](af::Character& strikerChr, af::Character& blockerChr,
                                     float gap, float scale = 1.0f, bool expectArmed = true) {
            af::Fighter striker;
            af::Fighter blocker;
            striker.setup(&strikerChr, 320.0f, 1);
            blocker.setup(&blockerChr, 320.0f + gap, -1);
            striker.setScale(scale);
            blocker.setScale(scale);
            bool answer = false;
            for (int i = 0; i < 8; ++i) {
                af::InputState none;
                striker.control(none);
                striker.step(blocker);
                blocker.control(none);
                blocker.step(striker);
                af::Fighter::updateGuardDistances(striker, blocker, beams, floorY);
                answer = blocker.machine().inGuardDist();
            }
            if (expectArmed) {
                check(striker.machine().hasArmedHitDef(),
                      "the striker should end this swing armed, or the flag below is about nothing");
            } else {
                check(!striker.machine().hasArmedHitDef(),
                      "the striker should end this swing with nothing armed, which is the "
                      "whole of the case below");
            }
            return answer;
        };

        // A short jab that reaches 20 units, written with no `guard.dist` at all.
        af::Character jabChar;
        af::Character bodyChar;
        makeStriker(jabChar, "", "5,-70,20,-40");
        makeBlocker(bodyChar);

        check(guardDistAt(jabChar, bodyChar, 100.0f),
              "a hundred units apart is inside guard distance even though the jab's box "
              "stops 44 short, because an unwritten `guard.dist` is `size.attack.dist`");
        check(!guardDistAt(jabChar, bodyChar, 200.0f),
              "and two hundred is outside it, so the 160 is a distance and not 'always'");

        // The same jab with its guard distance written down: 50 is what decides, and
        // the box no longer has a say in it.
        af::Character rangedChar;
        makeStriker(rangedChar, "guard.dist = 50\n", "5,-70,20,-40");
        check(guardDistAt(rangedChar, bodyChar, 40.0f),
              "a written 50 covers forty units");
        check(!guardDistAt(rangedChar, bodyChar, 60.0f),
              "and stops at sixty -- past the 44 the box-and-margin would have reached, "
              "so it is the number and not the box");
        // The number is in the character's own space, as every distance in its files
        // is, so it scales with the fighter exactly as its collision boxes do: the
        // same sixty units at twice the size is inside the same fifty.
        check(guardDistAt(rangedChar, bodyChar, 60.0f, 2.0f),
              "at twice the size the 50 covers twice the world distance, which is what "
              "`worldBox` does with a hitbox");

        // And the 0 that 90 of the corpus' 102 write-sites write, on a move whose box is
        // long enough to touch: "This prevents p2 from going into a guard state if close",
        // in kfm's own words.
        af::Character meleeChar;
        makeStriker(meleeChar, "guard.dist = 0\n", "24,-70,56,-40");
        check(!guardDistAt(meleeChar, bodyChar, 10.0f),
              "a written 0 means the blocker is never put in a guard state by it, even "
              "at ten units with the box overlapping");

        // And the comparison is signed and one-sided along the attacker's facing, not a
        // distance between two points: an opponent standing *behind* the attacker is not
        // being threatened by it, however close the two of them are. `guardDistAt` always
        // puts the blocker in front, so this one is placed by hand -- same pair, same
        // eight ticks, blocker on the *left* of a striker facing right, five units away.
        {
            af::Fighter striker;
            af::Fighter blocker;
            striker.setup(&jabChar, 320.0f, 1);
            blocker.setup(&bodyChar, 315.0f, -1);
            striker.setScale(1.0f);
            blocker.setScale(1.0f);
            bool answer = false;
            for (int i = 0; i < 8; ++i) {
                af::InputState none;
                striker.control(none);
                striker.step(blocker);
                blocker.control(none);
                blocker.step(striker);
                af::Fighter::updateGuardDistances(striker, blocker, beams, floorY);
                answer = blocker.machine().inGuardDist();
            }
            check(!answer,
                  "an opponent behind the attacker is not in its guard distance even at "
                  "five units, because the test is signed along the attacker's facing and "
                  "not a distance between two points");
        }

        // `AttackDist` is how a character takes the number off a HitDef it has already
        // armed -- the controller exists for exactly that -- and every one of the corpus'
        // 124 uses writes 0.
        af::Character lockoutChar;
        lockoutChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, swing]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n\n"
            "[State 300, lockout]\ntype = AttackDist\ntrigger1 = 1\nvalue = 0\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 300]\nClsn1: 1\nClsn1[0] = 5,-70,20,-40\n"
                         "300,0, 0,0, -1\n",
                         lockoutChar.animations());
        check(!guardDistAt(lockoutChar, bodyChar, 100.0f),
              "`AttackDist value = 0` on the tick after the HitDef armed takes the "
              "distance away, leaving the default 160 with nothing to reach");

        // The beam. Nothing is armed on its owner -- it is in a recovery state, which is
        // the whole reason the old measurement could not see this -- and the number that
        // covers it is the other one in `[Size]`, `proj.attack.dist`, 90 rather than 160.
        af::Character firerChar;
        firerChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                                "[Statedef -1]\n");
        af::Character beamBlockerChar;
        makeBlocker(beamBlockerChar);

        // The firer is off to the left having long since moved on, the blocker stands at
        // 200 holding back, and the beam is out in front of it travelling *towards* it.
        // That is the only arrangement in which the beam is a threat: the comparison is
        // one-sided along the projectile's own facing, so a beam that has gone past is not
        // guarding distance just because the two are close.
        af::Projectile beam;
        beam.owner = &firerChar;
        beam.ownerSide = 0;
        beam.facing = 1;
        beams.push_back(beam);
        {
            af::Fighter firer;
            af::Fighter blocker;
            firer.setup(&firerChar, 0.0f, 1);
            blocker.setup(&beamBlockerChar, 200.0f, -1);
            firer.setScale(1.0f);
            blocker.setScale(1.0f);
            beams[0].x = 170.0f;
            af::Fighter::updateGuardDistances(firer, blocker, beams, floorY);
            check(blocker.machine().inGuardDist(),
                  "a beam thirty units away puts the blocker in guard distance with its "
                  "owner standing there with nothing armed");
            beams[0].x = 80.0f;
            af::Fighter::updateGuardDistances(firer, blocker, beams, floorY);
            check(!blocker.machine().inGuardDist(),
                  "and a hundred and twenty is outside it, which the melee 160 would have "
                  "called inside -- `proj.attack.dist` is 90");
            // Now the same beam, same distance, but past the blocker and going away.
            beams[0].x = 230.0f;
            af::Fighter::updateGuardDistances(firer, blocker, beams, floorY);
            check(!blocker.machine().inGuardDist(),
                  "a beam that has gone past and is travelling away is not guarding "
                  "distance however close it is: the test is one-sided along its facing");
            // And the reach scales with the *owner*, not with the projectile's own
            // `projscale`: the 90 was written in the character's coordinates, so a firer
            // twice the size guards twice the world distance.
            beams[0].x = 170.0f;
            firer.setScale(2.0f);
            af::Fighter::updateGuardDistances(firer, blocker, beams, floorY);
            beams[0].x = 20.0f;
            af::Fighter::updateGuardDistances(firer, blocker, beams, floorY);
            check(blocker.machine().inGuardDist(),
                  "a firer at twice the size guards twice the world distance, so a beam "
                  "a hundred and eighty units off is inside its 90");
            firer.setScale(1.0f);
        }
        beams.clear();

        // The gate on the melee half is `movetype = A` and nothing else. That is the
        // reference's own test, and it is worth spelling out what it does *not* ask,
        // because two plausible readings of `inguarddist` are both wrong:
        //
        //   * it does not need a HitDef to be armed. A state in movetype A with nothing
        //     armed -- a super's wind-up, the frames of a move after its box has gone --
        //     still has a guard distance, because the distance is seeded at state entry
        //     and belongs to the player. This is the case the parameter exists for.
        //   * it is not satisfied by an armed HitDef in a state that is not attacking.
        //     A state that arms one while standing idle is not swinging.
        af::Character windupChar;
        windupChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, swing]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 0\n",
            "[Statedef -1]\n");
        check(guardDistAt(windupChar, bodyChar, 100.0f, 1.0f, false),
              "a state in movetype A with nothing armed at all is still an attack as far "
              "as guarding distance is concerned -- the wind-up before the box comes out");

        af::Character idleArmChar;
        idleArmChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, arm]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\ndamage = 10\n",
            "[Statedef -1]\n");
        check(!guardDistAt(idleArmChar, bodyChar, 40.0f),
              "and an armed HitDef in a state that is not movetype A is not one, however "
              "close the two are standing");

        // `AttackDist` writes the player's own slot, so its value outlives the HitDef it
        // was written against and is still in force in a state that arms nothing at all.
        // The corpus' 110 no-op uses are exactly this shape, and they are no-ops no longer.
        af::Character lastingChar;
        lastingChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[State 0, swing]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 300\n\n"
            "[Statedef 300]\ntype = S\nmovetype = A\nphysics = N\nctrl = 0\nanim = 300\n\n"
            "[State 300, swing]\ntype = HitDef\ntrigger1 = time = 0\nattr = S, NA\n\n"
            "[State 300, hold]\ntype = AttackDist\ntrigger1 = 1\nvalue = 20\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 300]\nClsn1: 1\nClsn1[0] = 5,-70,20,-40\n"
                         "300,0, 0,0, -1\n",
                         lastingChar.animations());
        {
            af::Fighter striker;
            af::Fighter blocker;
            striker.setup(&lastingChar, 320.0f, 1);
            blocker.setup(&bodyChar, 380.0f, -1);
            striker.setScale(1.0f);
            blocker.setScale(1.0f);
            for (int i = 0; i < 6; ++i) {
                af::InputState none;
                striker.control(none);
                striker.step(blocker);
                blocker.control(none);
                blocker.step(striker);
                af::Fighter::updateGuardDistances(striker, blocker, beams, floorY);
            }
            check(striker.machine().guardDist() == 20,
                  "`AttackDist` lands on the player's own guard distance, which is the "
                  "slot the HitDef executor writes and the only one `inguarddist` reads");
            check(!blocker.machine().inGuardDist(),
                  "so the twenty it wrote is what the sixty units between them is "
                  "measured against, not the 160 the state was seeded with");
        }
    }

    // ------------------------------------ the coordinate-space conversions
    //
    // `const240p`/`const480p`/`const720p` take a *length* written in one
    // resolution's space and return it in this screenpack's own, by the ratio of
    // the two widths. They read like booleans -- "is this game running at 480p?"
    // -- and they are not: the argument is a distance, and a character that
    // writes `const720p(12)` for the three pixels it means on a 320-wide canvas
    // lands correctly only if the trigger actually scales it.
    //
    // Before they existed here they were unknown identifiers, and the fallback at
    // the end of `ownTrigger` answers any unknown name longer than four letters
    // with 0 -- so every offset a character had written in another coordinate
    // space collapsed to zero rather than landing wrong. Elecbyte's own worked
    // example is the oracle: `const480p(6)` is 3 and `const720p(12)` is 3 on a
    // 240p screenpack.
    {
        af::Character probe;
        probe.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                            "[Statedef -1]\n");
        af::StateMachine converting;
        converting.setup(&probe, 1);
        converting.setOpponent(&dummy);
        converting.setPosition(320.0f);
        converting.setStageBounds(90.0f, 550.0f);

        const auto converted = [&converting](const char* name, float argument) {
            return converting.trigger(name, {af::Value::number(argument)}).toFloat();
        };

        // The reference's worked example, which is the whole point: these divide.
        check(converted("const480p", 6.0f) == 3.0f,
              "`const480p(6)` should be 3 on a 240p screenpack");
        check(converted("const720p", 12.0f) == 3.0f,
              "`const720p(12)` should be 3 on a 240p screenpack");
        // 240p is this screenpack's own space, so it converts nothing -- the
        // identity, not a passthrough that happens to agree.
        check(converted("const240p", 6.0f) == 6.0f,
              "`const240p` should leave a value already in 240p space alone");
        // Scaling is linear in the argument, which is what makes these usable for
        // a velocity and a distance alike rather than only for a fixed offset.
        check(converted("const720p", 1280.0f) == 320.0f,
              "a full 720p screen width should convert to this canvas's width");
        check(converted("const480p", 640.0f) == 320.0f,
              "and so should a full 480p width");
        // A 720p offset must not stay at its raw size: that is the difference
        // between dust at a character's feet and dust a third of the screen up.
        check(converted("const720p", 12.0f) == 3.0f,
              "a 720p offset should shrink to a quarter, not pass through");
        // The division is real rather than rounded or integer: a value that does
        // not halve evenly has to come back fractional, or every offset a 480p
        // character writes lands on the wrong pixel.
        check(converted("const480p", 7.0f) == 3.5f,
              "`const480p` should halve exactly, keeping the fraction");
        // Sign and zero survive, because these are used for negative offsets too.
        check(converted("const720p", -12.0f) == -3.0f,
              "a negative 720p offset should convert to a negative one");
        // A malformed call converts nothing rather than failing to answer.
        check(converting.trigger("const720p", {}).toFloat() == 0.0f,
              "`const720p` with no argument should convert nothing");

        // The same conversion through the file format, because a trigger the
        // evaluator cannot parse its way to is the same bug in a different place:
        // the controller writes `const480p(6)` into an fvar and it must arrive as
        // 3, having gone through the parser and back out through `fvar`.
        af::Character writer;
        writer.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, set]\ntype = VarSet\ntrigger1 = time = 0\nfv = 0\n"
            "value = const480p(6)\n",
            "[Statedef -1]\n");
        af::StateMachine written;
        written.setup(&writer, 1);
        written.setOpponent(&dummy);
        written.setPosition(320.0f);
        written.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 3; ++i) {
            af::InputState tick;
            written.control(tick);
            written.step();
        }
        check(written.trigger("fvar", {af::Value::number(0.0f)}).toFloat() == 3.0f,
              "a state writing `value = const480p(6)` should store 3 in an fvar");
    }

    // ------------------------------------------- the collision-box space
    //
    // A character writes its collision boxes in its own space, where y is
    // negative *upward*: Kung Fu Man's standing body is
    // `Clsn2[0] = -13, 0, 16,-79`, a box reaching 79 units above the feet and
    // not one unit below them. The screen is the other way up -- y grows
    // downward from the floor line -- so converting a box into the world means
    // *adding* its y to the feet, exactly as the renderer adds a frame's
    // `offsetY` to the feet when it draws the sprite.
    //
    // Subtracting it instead put every box on the far side of the floor line
    // from the fighter it belongs to, and nothing caught it: attack boxes and
    // hurt boxes went through the same routine and so were mirrored together,
    // and a pair of boxes mirrored about a common line still overlap where they
    // used to, which is why fights still landed. What it did break is the
    // agreement between a fighter that has art and the fighter that has none --
    // the placeholder's hurt box was a box above the feet all along -- and that
    // disagreement is what this measures.
    {
        af::Character bodyChar;
        bodyChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n",
                               "[Statedef -1]\n");
        af::parseAirText("[Begin Action 0]\nClsn2Default: 1\nClsn2[0] = -12,-64,12,0\n"
                         "0,0, 0,0, -1\n",
                         bodyChar.animations());
        af::Fighter body;
        af::Fighter other;
        body.setup(&bodyChar, 320.0f, -1);
        other.setup(nullptr, 360.0f, 1);
        body.setScale(1.0f);
        other.setScale(1.0f);
        for (int i = 0; i < 2; ++i) {
            body.control(af::InputState{});
            body.step(other);
        }
        check(body.machine().currentAnimation() != nullptr,
              "the fighter for the box checks should be showing an animation with boxes");

        // The floor line the fight draws at, and a point-sized box: a projectile
        // that carries no box of its own, so that only the *fighter's* box can
        // decide whether the two touch.
        const float floorY = 192.0f;
        const af::CollisionBox point{0.0f, 0.0f, 0.0f, 0.0f};
        const float chest = 40.0f;  // 40 units above the floor line

        check(body.overlapsEntityBox(point, 1, body.machine().x(), chest, floorY),
              "a character's own Clsn2 should stand above the floor line, where the art draws it");
        check(!body.overlapsEntityBox(point, 1, body.machine().x(), -20.0f, floorY),
              "and should not reach below the floor line, where nothing is drawn");

        // The control: a fighter with no character is a placeholder whose hurt
        // box is drawn as well as collided with, so where *it* is is not in
        // question -- and the two must agree about which way is up.
        af::Fighter placeholder;
        placeholder.setScale(1.0f);
        const float placeholderX = placeholder.machine().x();
        check(placeholder.overlapsEntityBox(point, 1, placeholderX, chest, floorY),
              "the placeholder's body should be above the floor line");
        check(!placeholder.overlapsEntityBox(point, 1, placeholderX, -20.0f, floorY),
              "and likewise should not reach below it");

        // A projectile's boxes are in the same space and take the same sign. This
        // one spans 20 to 5 units above the projectile's own origin, low enough
        // that only a box that reaches *up* from that origin can touch a body at
        // chest height -- so it is the check that separates the two signs for the
        // entity conversion rather than only for the fighter's.
        const af::CollisionBox shot{-8.0f, -20.0f, 8.0f, -5.0f};
        check(placeholder.overlapsEntityBox(shot, 1, placeholderX, 5.0f, floorY),
              "a projectile's own Clsn1 should reach up from its origin, not down");
        check(body.overlapsEntityBox(shot, 1, body.machine().x(), 5.0f, floorY),
              "and a shot at chest height should meet a real character's body too");
    }

    // ------------------------------------------------- one draw's transparency
    //
    // Three sources feed the transparency of a single sprite and they are ranked
    // rather than merged: the frame's own `.air` code, a `Trans` controller that
    // overrides it for exactly one tick, and the `trans`/`alpha` pair an Explod,
    // a Projectile or an AfterImage names for itself. What the renderer then does
    // with the pair is GL's business, but the pair itself is decided here.
    {
        // The GL translation, which is a pure function of the pair and so needs no
        // context at all. `A` is AS256D256 -- full additive -- and a destination
        // factor below that has to go through the constant-alpha factor, because
        // GL has no other way to scale the destination by a number that does not
        // ride the vertex stream.
        {
            const af::BlendState addFull = af::blendStateFor(af::Blend::Additive, 1.0f);
            check(addFull.equation == 0x8006u && addFull.source == 0x0302u &&
                      addFull.destination == 0x0001u && !addFull.constantAlpha.has_value(),
                  "`A` -- additive at AS256D256 -- is src*srcAlpha + dst");
            const af::BlendState addHalf = af::blendStateFor(af::Blend::Additive, 0.5f);
            check(addHalf.equation == 0x8006u && addHalf.source == 0x0302u &&
                      addHalf.destination == 0x8004u,
                  "while a destination factor below one switches to constant alpha, "
                  "which is the only route that number has into GL");
            // The number itself, not just the fact that a constant is used: a
            // state that says `GL_CONSTANT_ALPHA` and then arms the wrong
            // constant is a draw that is wrong by a factor, and nothing outside
            // this comparison could see it on a machine with no GL.
            check(addHalf.constantAlpha.has_value() &&
                      std::fabs(*addHalf.constantAlpha - 0.5f) < 0.001f,
                  "and carries the destination alpha with it, so the draw cannot arm "
                  "a constant it was not asked for");
            const af::BlendState subFull = af::blendStateFor(af::Blend::Subtractive, 1.0f);
            check(subFull.equation == 0x800Bu && subFull.destination == 0x0001u,
                  "and `S` is the same shape under the subtract equation");
            const af::BlendState subHalf = af::blendStateFor(af::Blend::Subtractive, 0.5f);
            check(subHalf.equation == 0x800Bu && subHalf.destination == 0x8004u &&
                      subHalf.constantAlpha.has_value(),
                  "with its own destination factor switched the same way");
            // Two states that differ only in the destination alpha have to compare
            // unequal, because the renderer's batch cache is this comparison and
            // nothing else: a blend cache that conflated them would draw the
            // second sprite of a fade with the first one's constant.
            check(!(addFull == addHalf) && addFull == af::blendStateFor(af::Blend::Additive, 1.0f),
                  "two draws that differ only in their destination alpha are two blend "
                  "states, which is what the batch cache compares");
            // Normal never carries an S/D pair in the format: every code in the
            // corpus's 415 .air/.def files begins with `A` or `S`, which is also
            // what the reference documents ("the parameters for color addition and
            // subtraction are A and S"). So a Normal draw takes the alpha-key path
            // whatever destination it is handed, and this asserts that rather than
            // leaving it to be discovered by a sprite rendering wrong.
            const af::BlendState normal = af::blendStateFor(af::Blend::Normal, 0.25f);
            check(normal.equation == 0x8006u && normal.source == 0x0302u &&
                      normal.destination == 0x0303u && !normal.constantAlpha.has_value(),
                  "a Normal draw keys on the source alpha and ignores a destination "
                  "factor, which is the one thing the format never gives it");
            // And the half of the pair that does not go through the blender: the
            // source alpha rides the vertex colour, where the shader multiplies it
            // into the texel and `GL_SRC_ALPHA` reads it back out.
            const af::Color faded = af::transTint(af::Color{1.0f, 1.0f, 1.0f, 0.8f},
                                                  af::TransParams{af::Blend::Additive, 0.5f, 0.25f});
            check(std::fabs(faded.r - 1.0f) < 0.001f && std::fabs(faded.g - 1.0f) < 0.001f &&
                      std::fabs(faded.b - 1.0f) < 0.001f &&
                      std::fabs(faded.a - 0.4f) < 0.001f,
                  "the source alpha scales the vertex alpha and leaves the colour alone, "
                  "which is the whole of how the source half reaches the shader");
        }

        // The controller's six types and their `alpha` default. `default` and an
        // unrecognised value are both "does nothing" -- MUGEN logs the second as
        // "Invalid trans type" and drops it -- and "does nothing" is an *absent*
        // value, not a Normal one, because a frame that asked for `A` has to
        // survive a `Trans` that says nothing.
        {
            check(!af::parseTransType("", 256.0f, 0.0f).has_value() &&
                      !af::parseTransType("default", 256.0f, 0.0f).has_value() &&
                      !af::parseTransType("bogus", 256.0f, 0.0f).has_value(),
                  "`default`, an empty trans and a typo all mean 'does nothing'");
            const std::optional<af::TransParams> none = af::parseTransType("none", 256.0f, 0.0f);
            check(none.has_value() && none->blend == af::Blend::Normal &&
                      none->srcAlpha == 1.0f && none->dstAlpha == 1.0f,
                  "`none` is the one that forces Normal rather than leaving the frame alone");
            const std::optional<af::TransParams> add = af::parseTransType("add", 256.0f, 0.0f);
            check(add.has_value() && add->blend == af::Blend::Additive && add->dstAlpha == 1.0f,
                  "`add` is full additive, so its destination factor is one");
            const std::optional<af::TransParams> add1 = af::parseTransType(" ADD1 ", 256.0f, 0.0f);
            check(add1.has_value() && add1->blend == af::Blend::Additive &&
                      std::fabs(add1->srcAlpha - 1.0f) < 0.001f &&
                      std::fabs(add1->dstAlpha - 0.5f) < 0.001f,
                  "`A1` is AS256D128 whichever way it is spelled or cased");
            const std::optional<af::TransParams> mix =
                af::parseTransType("addalpha", 128.0f, 128.0f);
            check(mix.has_value() && std::fabs(mix->srcAlpha - 0.5f) < 0.001f &&
                      std::fabs(mix->dstAlpha - 0.5f) < 0.001f,
                  "and `addalpha` carries its own pair, on MUGEN's 0..256 scale");
            const std::optional<af::TransParams> clamped =
                af::parseTransType("addalpha", 400.0f, -20.0f);
            check(clamped.has_value() && std::fabs(clamped->srcAlpha - 1.0f) < 0.001f &&
                      std::fabs(clamped->dstAlpha - 0.0f) < 0.001f,
                  "a pair outside 0..256 is clamped rather than wrapping");
            const std::optional<af::TransParams> sub = af::parseTransType("sub", 256.0f, 0.0f);
            check(sub.has_value() && sub->blend == af::Blend::Subtractive &&
                      sub->dstAlpha == 1.0f,
                  "and `sub` is full subtractive");
        }

        // The frame's own code, then the controller's override, then the tick the
        // override stops holding. The fighter is drawn for real -- into the shim's
        // recorder rather than onto a screen -- so what is asserted is the pair
        // that would reach the renderer, not a restatement of the parse.
        {
            af::Character transChar;
            transChar.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
                // Two ticks of override out of a state that never leaves, so the
                // third tick is the one where the slot has to be empty again.
                "[State 0, fade]\ntype = Trans\ntrigger1 = time < 2\n"
                "trans = addalpha\nalpha = 128, 128\n\n"
                "[Statedef -1]\n",
                "[Statedef -1]\n");
            // The frame asks for `A` -- full additive -- so the two sources are
            // told apart by the destination factor alone, and an override that
            // silently fell through to a default would be visible as one.
            af::parseAirText("[Begin Action 0]\n0,0, 0,0, -1, ,A\n", transChar.animations());

            af::Fighter body;
            af::Fighter other;
            body.setup(&transChar, 320.0f, 1);
            other.setup(nullptr, 420.0f, -1);
            body.setScale(1.0f);

            af::Renderer renderer;
            af::ShadowStyle shadow;
            shadow.intensity = 0.0f;  // no contact shadow: the body is the only draw
            af::draw_stub::record(true);

            // One tick of the fixture, then the one draw that tick would have made.
            const auto drawBody = [&]() {
                af::draw_stub::clear();
                body.draw(renderer, 0.0f, 240.0f, shadow);
            };

            for (int tick = 0; tick < 3; ++tick) {
                body.control(af::InputState{});
                body.step(other);
                drawBody();
                const std::vector<af::draw_stub::DrawCall>& calls = af::draw_stub::recorded();
                check(calls.size() == 1, "the body should be the only thing drawn");
                if (calls.size() != 1) break;
                const af::TransParams& drawn = calls.front().trans;
                if (tick < 2) {
                    check(drawn.blend == af::Blend::Additive &&
                              std::fabs(drawn.srcAlpha - 0.5f) < 0.001f &&
                              std::fabs(drawn.dstAlpha - 0.5f) < 0.001f,
                          "a `Trans` controller writes the pair the body draws with, "
                          "over the frame's own");
                } else {
                    check(drawn.blend == af::Blend::Additive &&
                              std::fabs(drawn.srcAlpha - 1.0f) < 0.001f &&
                              std::fabs(drawn.dstAlpha - 1.0f) < 0.001f,
                          "and it is gone the tick after its trigger stops holding, so "
                          "the frame's own `A` -- not any default -- draws again");
                }
            }
            af::draw_stub::record(false);
        }

        // An Explod's own pair against the animation's. `trans` absent is the
        // whole of "let the animation say", which is why it is not defaulted to
        // Normal: a fightfx frame that carries `A` must still be additive.
        {
            af::Character fx;
            af::parseAirText("[Begin Action 900]\n0,0, 0,0, -1, ,A\n"
                             "[Begin Action 901]\n0,0, 0,0, -1\n",
                             fx.animations());

            af::Explod asked;
            asked.owner = &fx;
            asked.anim = 901;  // a plain frame, so the override is the only source
            asked.x = 10.0f;
            asked.trans = af::TransParams{af::Blend::Subtractive};
            af::Explod inherited;
            inherited.owner = &fx;
            inherited.anim = 900;  // `A` -- additive, from the animation itself
            inherited.x = 20.0f;

            af::Renderer renderer;
            af::draw_stub::clear();
            af::draw_stub::record(true);
            af::EntityLayer layer;
            layer.drawExplod(renderer, asked, 0.0f, 240.0f);
            layer.drawExplod(renderer, inherited, 0.0f, 240.0f);
            af::draw_stub::record(false);

            const std::vector<af::draw_stub::DrawCall>& calls = af::draw_stub::recorded();
            check(calls.size() == 2, "two explods, two draws");
            if (calls.size() == 2) {
                check(calls[0].trans.blend == af::Blend::Subtractive,
                      "an explod that wrote `trans = sub` draws subtractively whatever "
                      "its animation says");
                check(calls[1].trans.blend == af::Blend::Additive &&
                          std::fabs(calls[1].trans.srcAlpha - 1.0f) < 0.001f &&
                          std::fabs(calls[1].trans.dstAlpha - 1.0f) < 0.001f,
                      "and one that wrote none takes the animation's own code");
            }
        }

        // A projectile draws as its own animation asks, with no blend of the
        // engine's own in the way. It used to be forced additive here, which
        // flattened the two thirds of fightfx's frames that carry no code at all.
        {
            af::Character shooter;
            af::parseAirText("[Begin Action 902]\n0,0, 0,0, -1, ,A1\n", shooter.animations());

            af::Projectile shot;
            shot.owner = &shooter;
            shot.anim = 902;
            shot.x = 100.0f;

            af::Renderer renderer;
            af::draw_stub::clear();
            af::draw_stub::record(true);
            af::EntityLayer layer;
            layer.drawFront(renderer, 0.0f, 240.0f);
            layer.spawnProjectile(shot);
            layer.drawFront(renderer, 0.0f, 240.0f);
            af::draw_stub::record(false);

            const std::vector<af::draw_stub::DrawCall>& calls = af::draw_stub::recorded();
            check(calls.size() == 1, "the flight frame should be the one draw");
            if (calls.size() == 1) {
                check(calls[0].trans.blend == af::Blend::Additive &&
                          std::fabs(calls[0].trans.srcAlpha - 1.0f) < 0.001f &&
                          std::fabs(calls[0].trans.dstAlpha - 0.5f) < 0.001f,
                      "and drawn as `A1`, the pair its frame carries");
            }
        }

        // The trail's pair, which is the AfterImage controller's and not the
        // body frame's. The ghosts are drawn before the body, oldest first, so the
        // two are told apart by position in the recording.
        {
            af::Character trailChar;
            trailChar.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
                "[State 0, trail]\ntype = AfterImage\ntrigger1 = time = 0\n"
                "time = 10\nlength = 3\ntrans = sub\n\n"
                "[Statedef -1]\n",
                "[Statedef -1]\n");
            af::parseAirText("[Begin Action 0]\n0,0, 0,0, -1\n", trailChar.animations());

            af::Fighter body;
            af::Fighter other;
            body.setup(&trailChar, 320.0f, 1);
            other.setup(nullptr, 420.0f, -1);
            body.setScale(1.0f);
            for (int i = 0; i < 4; ++i) {
                body.control(af::InputState{});
                body.step(other);
            }

            af::Renderer renderer;
            af::ShadowStyle shadow;
            shadow.intensity = 0.0f;
            af::draw_stub::clear();
            af::draw_stub::record(true);
            body.draw(renderer, 0.0f, 240.0f, shadow);
            af::draw_stub::record(false);

            const std::vector<af::draw_stub::DrawCall>& calls = af::draw_stub::recorded();
            check(calls.size() >= 2,
                  "an AfterImage trail should draw its ghosts and then the body");
            if (calls.size() >= 2) {
                check(calls.front().trans.blend == af::Blend::Subtractive,
                      "and the ghosts blend the way the controller asked, not the "
                      "additive default");
                check(calls.back().trans.blend == af::Blend::Normal,
                      "while the body follows its own frame, which asked for nothing");
            }
        }
    }

    // ------------------------------------------------------- the width bar
    //
    // MUGEN's push is the sum of the two fighters' width bars, not a constant
    // gap: the `[Size]` ground/air width on the side facing the other player,
    // plus the Width controller's player bar (added, never substituted). A
    // near-coincident pair is recentred at +-screenWidth/64 first, and the sum
    // is still applied against the original dx. See
    // docs/mugen-re/WIDTH-INVESTIGATION.md.
    {
        af::Character sizer;
        sizer.loadBehaviour(
            "[Size]\n"
            "ground.back = 10\nground.front = 10\n"
            "air.back = 10\nair.front = 10\nheight = 60\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[Statedef -1]\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 0]\n0,0, 0,0, -1\n", sizer.animations());

        // 15 apart with 10+10 bars: the reference pushes by (20-15)/2 = 2.5,
        // not the old constant-gap 5.5. dx >= 320/32, so no recentre.
        {
            af::Fighter a;
            af::Fighter b;
            a.setup(&sizer, 300.0f, 1);
            b.setup(&sizer, 315.0f, -1);
            af::StateMachine::separate(a.machine(), b.machine());
            check(std::fabs(a.machine().x() - 297.5f) < 0.001f &&
                      std::fabs(b.machine().x() - 317.5f) < 0.001f,
                  "the push should be half the width-sum overlap, not a constant gap");
        }

        // 5 apart: recentre about the midpoint to 302.5 +- 5, then push by
        // (20-5)/2 against the original dx: 290 and 315.
        {
            af::Fighter a;
            af::Fighter b;
            a.setup(&sizer, 300.0f, 1);
            b.setup(&sizer, 305.0f, -1);
            af::StateMachine::separate(a.machine(), b.machine());
            check(std::fabs(a.machine().x() - 290.0f) < 0.001f &&
                      std::fabs(b.machine().x() - 315.0f) < 0.001f,
                  "a near-coincident pair is recentred and then pushed");
        }

        // Bars do not reach: nothing moves.
        {
            af::Fighter a;
            af::Fighter b;
            a.setup(&sizer, 300.0f, 1);
            b.setup(&sizer, 330.0f, -1);
            af::StateMachine::separate(a.machine(), b.machine());
            check(std::fabs(a.machine().x() - 300.0f) < 0.001f &&
                      std::fabs(b.machine().x() - 330.0f) < 0.001f,
                  "bars that do not overlap should not move the fighters");
        }

        // No character behind either machine: zero widths keep the old 26-unit
        // placeholder gap the debug arena has always used.
        {
            af::Fighter a;
            af::Fighter b;
            a.setup(nullptr, 300.0f, 1);
            b.setup(nullptr, 320.0f, -1);
            af::StateMachine::separate(a.machine(), b.machine());
            check(std::fabs(a.machine().x() - 297.0f) < 0.001f &&
                      std::fabs(b.machine().x() - 323.0f) < 0.001f,
                  "zero-width machines keep the placeholder's constant-gap push");
        }
    }

    // The Width controller writes all four bar slots, and the edge triggers
    // read the bar rather than the hurt box. `edge = 6, 7`, player `2, 3`;
    // standing, facing right, x=100, stage bounds 90..550.
    {
        af::Character widthChar;
        widthChar.loadBehaviour(
            "[Size]\n"
            "ground.back = 10\nground.front = 12\n"
            "air.back = 8\nair.front = 9\nheight = 60\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\nanim = 0\n\n"
            "[Statedef -1]\n"
            "[State -1, Width]\ntype = Width\ntrigger1 = 1\n"
            "edge = 6, 7\nplayer = 2, 3\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 0]\n0,0, 0,0, -1\n", widthChar.animations());

        af::Fighter f;
        af::Fighter dummy;
        f.setup(&widthChar, 100.0f, 1);
        dummy.setup(nullptr, 200.0f, -1);
        f.control(af::InputState{});
        f.step(dummy);

        check(std::fabs(f.machine().trigger("frontedgedist", {}).toFloat() - 450.0f) <
                  0.001f,
              "FrontEdgeDist should measure x to the bound the fighter faces");
        check(std::fabs(f.machine().trigger("frontedgebodydist", {}).toFloat() - 444.0f) <
                  0.001f,
              "FrontEdgeBodyDist should add the edge-front width");
        check(std::fabs(f.machine().trigger("backedgedist", {}).toFloat() - 10.0f) <
                  0.001f,
              "BackEdgeDist should measure x to the bound behind the fighter");
        check(std::fabs(f.machine().trigger("backedgebodydist", {}).toFloat() - 3.0f) <
                  0.001f,
              "BackEdgeBodyDist should add the edge-back width");
    }

    // ------------------------------------------ the common states MUGEN owns
    //
    // The rows below are states the reference's own `common1.cns` defines and
    // the engine has to supply for a character that ships no file of its own.
    // Every one of them is a state number a character can be handed directly,
    // so each test drives the machine into the number and reads what the engine
    // does with it.

    // ------------------------------------------------- 190 -> 191 -> 0, one tick
    //
    // `[State 190, 1] ChangeState / trigger1 = Time = 0 / value = 191` and
    // `[State 191, 1] ChangeState / trigger1 = Time = 0 / value = 0`: the
    // pre-intro and the intro override point each last exactly one tick, and
    // with no art behind them that is the tick they are entered on. Nothing may
    // be held in either state waiting for an animation -- there is none, and a
    // file-less fighter that stopped in 190 would never reach its own state 0.
    //
    // The route is observed from a third state because 190 and 191 both pass
    // through inside one `runCurrentState` loop and so are never visible
    // between ticks. State 900 marks with a variable and only hands to 190 on
    // its second tick; after the hand-over the chain returns to 900 through
    // state 0. Seeing 900 again, one tick old, means both Time = 0 exits fired
    // in the tick 190 was entered.
    {
        af::Character builtinIntroChar;
        builtinIntroChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 900\n\n"
            "[Statedef 900]\ntype = S\nphysics = N\nctrl = 0\n\n"
            "[State 900, mark]\ntype = VarSet\ntrigger1 = time = 0\nv = 0\nvalue = 1\n\n"
            "[State 900, intro]\ntype = ChangeState\ntrigger1 = time = 1\nvalue = 190\n",
            "[Statedef -1]\n");
        af::StateMachine builtinIntro;
        builtinIntro.setup(&builtinIntroChar, 1);
        builtinIntro.setOpponent(&dummy);
        builtinIntro.setPosition(320.0f);
        builtinIntro.setStageBounds(90.0f, 550.0f);

        for (int builtinIntroTick = 0; builtinIntroTick < 2; ++builtinIntroTick) {
            af::InputState builtinIntroInput;
            builtinIntro.control(builtinIntroInput);
            builtinIntro.step();
        }
        check(builtinIntro.stateNumber() == 900,
              "190 and 191 should both hand over on their Time = 0 tick, leaving the state "
              "that entered them");
        check(builtinIntro.stateTime() == 1,
              "and both hand-overs should happen on that one tick, not one tick each");
        check(builtinIntro.trigger("var", {af::Value::number(0.0f)}).toFloat() == 1.0f,
              "so the state they returned through is entered and runs on the same tick");
    }

    // --------------------------------------------- 45's Time = 2 air-jump takeoff
    //
    // `[State 45, 6] ChangeState / trigger1 = Time = 2 / value = 50 ctrl = 1`
    // and `[State 45, 7]` beside it read `velocity.airjump.*`. The takeoff
    // therefore has no velocity of its own for two ticks and leaves on the
    // third, not when an animation ends -- a file-less fighter has no takeoff
    // art, and an animation-driven reading would jump it on the entry tick.
    {
        af::Character builtinJumpChar;
        builtinJumpChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 900\n\n"
            "[Statedef 900]\ntype = A\nphysics = N\nctrl = 0\n\n"
            "[State 900, lift]\ntype = VelSet\ntrigger1 = time = 0\ny = -10\n\n"
            "[State 900, airjump]\ntype = ChangeState\ntrigger1 = time = 1\nvalue = 45\n",
            "[Statedef -1]\n");
        af::StateMachine builtinJump;
        builtinJump.setup(&builtinJumpChar, 1);
        builtinJump.setOpponent(&dummy);
        builtinJump.setPosition(320.0f);
        builtinJump.setStageBounds(90.0f, 550.0f);

        int builtinJumpTakeoffTicks = 0;
        float builtinJumpVelocity = 0.0f;
        bool builtinJumpTookOff = false;
        for (int builtinJumpTick = 0; builtinJumpTick < 20; ++builtinJumpTick) {
            af::InputState builtinJumpInput;
            builtinJump.control(builtinJumpInput);
            builtinJump.step();
            if (builtinJump.stateNumber() == af::CommonState::kAirJumpStart) {
                ++builtinJumpTakeoffTicks;
            } else if (builtinJump.stateNumber() == af::CommonState::kAir) {
                builtinJumpTookOff = true;
                builtinJumpVelocity = builtinJump.velY();
                break;
            }
        }
        check(builtinJumpTookOff, "state 45 should hand over to the air state");
        check(builtinJumpTakeoffTicks == 2,
              "and should hold for the two ticks `Time = 2` names rather than jump at once");
        // The default `velocity.airjump.neu.y` is -8.1 in the character's own
        // frame, and `stepPhysics` has already spent one tick of gravity on the
        // air state by the time this is read.
        check(builtinJumpVelocity > 7.0f && builtinJumpVelocity < 8.1f,
              "with the air-jump velocity taken at Time = 2, not carried from the launch");
        check(builtinJump.ctrl(), "and 50 is the ordinary air state, so control is back");
    }

    // ------------------------------------------------ 105 -> 106's seven-tick land
    //
    // `[State 105, 3] ChangeState / trigger1 = AnimTime = 0 / value = 106` is the
    // hand-over the existing row already names, and `[State 106, 5] ChangeState /
    // trigger1 = Time = 7 / value = 0` is the fixed hold 106 exists for. Seven is
    // counted in ticks, not read off the 47 pose -- a file-less fighter owns no
    // such pose, and this is the one hop state whose length is written down.
    {
        af::Character builtinHopChar;
        builtinHopChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0 && prevstateno != 106\n"
            "value = 105\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 105]\n0,0, 0,0, 2\n", builtinHopChar.animations());
        af::StateMachine builtinHop;
        builtinHop.setup(&builtinHopChar, 1);
        builtinHop.setOpponent(&dummy);
        builtinHop.setPosition(320.0f);
        builtinHop.setStageBounds(90.0f, 550.0f);

        int builtinHopAirTicks = 0;
        int builtinHopLandTicks = 0;
        bool builtinHopReturned = false;
        for (int builtinHopTick = 0; builtinHopTick < 40; ++builtinHopTick) {
            af::InputState builtinHopInput;
            builtinHop.control(builtinHopInput);
            builtinHop.step();
            if (builtinHop.stateNumber() == af::CommonState::kHop) {
                ++builtinHopAirTicks;
            } else if (builtinHop.stateNumber() == af::CommonState::kHopLand) {
                ++builtinHopLandTicks;
            } else if (builtinHop.stateNumber() == af::CommonState::kStand &&
                       builtinHopLandTicks > 0) {
                builtinHopReturned = true;
                break;
            }
        }
        check(builtinHopAirTicks >= 1, "the backward hop should hold its takeoff pose");
        check(builtinHopReturned, "and 106 should return the fighter to standing");
        check(builtinHopLandTicks == 7,
              "after exactly the seven ticks `[State 106, 5]` writes, not on its entry tick");
    }

    // ------------------------------------------- a trip's fall lands in 5110
    //
    // `[State 5071, 3] ChangeState / value = 5110` is the trip's own arrival at
    // the floor: it lies the victim down directly, with no 5100 bounce arrival
    // in between. `ground.type = 3` is what the reference uses to pick 5070,
    // and the engine's `hitInfoFor` makes the same choice.
    {
        af::Character builtinTripChar;
        builtinTripChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 900\n\n"
            "[Statedef 900]\ntype = A\nphysics = N\nctrl = 0\n\n"
            "[State 900, lift]\ntype = VelSet\ntrigger1 = time = 0\ny = -40\n",
            "[Statedef -1]\n");
        af::StateMachine builtinTrip;
        builtinTrip.setup(&builtinTripChar, 1);
        builtinTrip.setOpponent(&dummy);
        builtinTrip.setPosition(320.0f);
        builtinTrip.setStageBounds(90.0f, 550.0f);
        for (int builtinTripLift = 0; builtinTripLift < 1; ++builtinTripLift) {
            af::InputState builtinTripInput;
            builtinTrip.control(builtinTripInput);
            builtinTrip.step();
        }

        af::StateMachine::HitInfo builtinTripHit;
        builtinTripHit.damage = 5;
        builtinTripHit.hitTime = 12;
        builtinTripHit.stateNumber = af::CommonState::kHitTrip;
        builtinTripHit.groundType = 3;
        builtinTripHit.victimPause = 0;
        check(builtinTrip.applyHit(builtinTripHit), "the trip lands");

        bool builtinTripSawFall = false;
        bool builtinTripSawFloorArrival = false;
        int builtinTripTicks = 0;
        while (builtinTripTicks++ < 200 &&
               builtinTrip.stateNumber() != af::CommonState::kDowned) {
            af::InputState builtinTripInput;
            builtinTrip.control(builtinTripInput);
            builtinTrip.step();
            if (builtinTrip.stateNumber() == af::CommonState::kHitTripFall) {
                builtinTripSawFall = true;
            }
            if (builtinTrip.stateNumber() == af::CommonState::kHitGround) {
                builtinTripSawFloorArrival = true;
            }
        }
        check(builtinTripSawFall, "the trip should turn into its fall state");
        check(builtinTrip.stateNumber() == af::CommonState::kDowned,
              "and the trip's fall should land the victim in 5110");
        check(!builtinTripSawFloorArrival,
              "without going through 5100's bounce arrival, which `[State 5071, 3]` does not use");
    }

    // ---------------------------------------- a dead 5040 lands as the fall, not 5100
    //
    // `[State 5040, ...] !Alive -> 5050`: dying in the air-recovery state is a
    // fall, and 5050 then carries the body to the floor and 5100. Landing in
    // 5100 directly (as the old `land()` did) skips the fall state's own
    // gravity and its landing rules. The 5050 marker is what makes the two
    // routes distinguishable -- they share their final destination.
    {
        af::Character builtinDeadChar;
        builtinDeadChar.loadBehaviour(
            "[Data]\nlife = 1000\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 900\n\n"
            "[Statedef 900]\ntype = A\nphysics = N\nctrl = 0\n\n"
            "[State 900, lift]\ntype = VelSet\ntrigger1 = time = 0\ny = -5\n\n"
            "[State 900, die]\ntype = ChangeState\ntrigger1 = time = 1\nvalue = 5040\n\n"
            "[Statedef 5040]\ntype = A\nmovetype = H\nphysics = A\nctrl = 0\n\n"
            "[State 5040, kill]\ntype = LifeAdd\ntrigger1 = time = 0\nvalue = -2000\n\n"
            "[Statedef 5050]\ntype = A\nmovetype = H\nphysics = A\nctrl = 0\n\n"
            "[State 5050, mark]\ntype = VarSet\ntrigger1 = 1\nv = 0\nvalue = 42\n",
            "[Statedef -1]\n");
        af::StateMachine builtinDead;
        builtinDead.setup(&builtinDeadChar, 1);
        builtinDead.setOpponent(&dummy);
        builtinDead.setPosition(320.0f);
        builtinDead.setStageBounds(90.0f, 550.0f);

        float builtinDeadMarker = 0.0f;
        int builtinDeadTicks = 0;
        while (builtinDeadTicks++ < 300 && builtinDead.stateNumber() != af::CommonState::kDead) {
            af::InputState builtinDeadInput;
            builtinDead.control(builtinDeadInput);
            builtinDead.step();
            if (builtinDeadMarker == 0.0f) {
                builtinDeadMarker =
                    builtinDead.trigger("var", {af::Value::number(0.0f)}).toFloat();
            }
        }
        check(builtinDead.life() == 0, "the LifeAdd in 5040 should be lethal");
        check(builtinDeadMarker == 42.0f,
              "so a dead 5040 should pass through the fall state 5050, not land in 5100");
        check(builtinDead.stateNumber() == af::CommonState::kDead,
              "and the fall should still carry the body to the floor and the dead state");
    }

    // ------------------------------------- 5030 -> 5035 while the hitstun is still on
    //
    // `[State 5030, 6] ChangeState / trigger1 = AnimTime = 0 / value = 5035` is
    // the reference's earliest exit from 5030, and it is the one route in the
    // reaction family that is allowed to run before `HitOver`. 5035 then waits
    // for the stun: a file-less fighter's 5035 has no art, but this engine
    // deliberately does not take 5035's `Time = 0` no-art exit until `HitOver`,
    // so the victim cannot leave the hit early.
    {
        af::Character builtinTransitionChar;
        builtinTransitionChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 900\n\n"
            "[Statedef 900]\ntype = A\nphysics = N\nctrl = 0\n\n"
            "[State 900, lift]\ntype = VelSet\ntrigger1 = time = 0\ny = -150\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 5030]\n0,0, 0,0, 2\n",
                         builtinTransitionChar.animations());
        af::StateMachine builtinTransition;
        builtinTransition.setup(&builtinTransitionChar, 1);
        builtinTransition.setOpponent(&dummy);
        builtinTransition.setPosition(320.0f);
        builtinTransition.setStageBounds(90.0f, 550.0f);
        for (int builtinTransitionLift = 0; builtinTransitionLift < 1;
             ++builtinTransitionLift) {
            af::InputState builtinTransitionInput;
            builtinTransition.control(builtinTransitionInput);
            builtinTransition.step();
        }

        af::StateMachine::HitInfo builtinTransitionHit;
        builtinTransitionHit.damage = 1;
        builtinTransitionHit.hitTime = 20;
        builtinTransitionHit.stateNumber = af::CommonState::kHitAirFall;
        builtinTransitionHit.falls = false;
        builtinTransitionHit.victimPause = 0;
        check(builtinTransition.applyHit(builtinTransitionHit), "the launch lands");

        bool builtinTransitionWasStunned = false;
        int builtinTransitionTicks = 0;
        while (builtinTransitionTicks++ < 60 &&
               builtinTransition.stateNumber() != af::CommonState::kHitAirTransition) {
            af::InputState builtinTransitionInput;
            builtinTransition.control(builtinTransitionInput);
            builtinTransition.step();
        }
        check(builtinTransition.stateNumber() == af::CommonState::kHitAirTransition,
              "the 5030 animation running out should enter the transition state 5035");
        builtinTransitionWasStunned =
            builtinTransition.trigger("hitover", {}).toFloat() == 0.0f;
        check(builtinTransitionWasStunned,
              "and it should do so while the hit is still on, which is the reference's "
              "AnimTime = 0 exit rather than the HitOver one");

        // The transition owns no exit until the stun is spent, so 5035 is held
        // through the remaining hitstun and only then splits on `HitFall` -- here
        // to 5040, because the launch was not a fall.
        int builtinTransitionHold = 0;
        int builtinTransitionAfter = af::CommonState::kHitAirTransition;
        while (builtinTransitionHold++ < 60 &&
               builtinTransition.stateNumber() == af::CommonState::kHitAirTransition) {
            af::InputState builtinTransitionInput;
            builtinTransition.control(builtinTransitionInput);
            builtinTransition.step();
        }
        builtinTransitionAfter = builtinTransition.stateNumber();
        check(builtinTransitionAfter == af::CommonState::kHitAirRecover,
              "and once HitOver passes it should take the non-fall exit to 5040");
    }

    // ------------------------------------------------ match-level triggers
    //
    // `matchno` and `matchover` describe the run, not the character: no .cns
    // can supply them. The default must read as a versus match (1, false), and
    // a character must be able to branch on both once the fight screen has set
    // them -- which is what the Victory/Win/Continue flow reads.
    {
        af::Character matchChar;
        matchChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, third]\ntype = ChangeState\ntrigger1 = matchno = 3\nvalue = 900\n\n"
            "[Statedef 900]\ntype = S\nphysics = S\nctrl = 0\n",
            "[Statedef -1]\n");
        af::StateMachine matchMachine;
        matchMachine.setup(&matchChar, 1);
        matchMachine.setOpponent(&dummy);
        matchMachine.setPosition(320.0f);
        matchMachine.setStageBounds(90.0f, 550.0f);

        check(matchMachine.trigger("matchno", {}).toFloat() == 1.0f,
              "matchno should default to match 1");
        check(matchMachine.trigger("matchover", {}).toFloat() == 0.0f,
              "matchover should default to false");
        check(matchMachine.trigger("roundsremaining", {}).toFloat() == 0.0f,
              "roundsremaining should stay 0 -- it is not a MUGEN trigger");

        // Run first at match 1, so the change below can only be the match
        // number and not a state 0 that never held.
        for (int i = 0; i < 5; ++i) {
            af::InputState matchIdle;
            matchMachine.control(matchIdle);
            matchMachine.step();
        }
        check(matchMachine.stateNumber() == 0,
              "a matchno = 3 ChangeState should not fire while matchno is 1");

        matchMachine.setMatchNo(3);
        check(matchMachine.trigger("matchno", {}).toFloat() == 3.0f,
              "setMatchNo should be what matchno reports");
        af::InputState matchIdle;
        matchMachine.control(matchIdle);
        matchMachine.step();
        check(matchMachine.stateNumber() == 900,
              "matchno = 3 should fire once setMatchNo(3) has been called");

        matchMachine.setMatchOver(true);
        check(matchMachine.trigger("matchover", {}).toFloat() != 0.0f,
              "matchover should be true after setMatchOver(true)");
    }

    // ------------------------------------------------ TeamMode keywords
    //
    // `teammode` is comparison-only and its right-hand side is a keyword, not
    // a trigger: `single`, `simul` and `turns` compile to 0, 1 and 3
    // (`TEAM-MODES-RE.md` §2.3). Read as bare identifiers they used to fall
    // through to "an unknown word is zero", and `teammode` itself returned the
    // text "single", so a string-vs-number compare made `teammode = single`
    // false. Four corpus characters gate states on exactly that test.
    {
        af::Character singleChar;
        singleChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, single]\ntype = ChangeState\ntrigger1 = teammode = single\nvalue = 920\n\n"
            "[Statedef 920]\ntype = S\nphysics = S\nctrl = 0\n",
            "[Statedef -1]\n");
        af::StateMachine singleMachine;
        singleMachine.setup(&singleChar, 1);
        singleMachine.setOpponent(&dummy);
        singleMachine.setPosition(320.0f);
        singleMachine.setStageBounds(90.0f, 550.0f);
        check(singleMachine.trigger("teammode", {}).toFloat() == 0.0f,
              "teammode should answer the single-mode code 0, not text");
        af::InputState teamIdle;
        singleMachine.control(teamIdle);
        singleMachine.step();
        check(singleMachine.stateNumber() == 920,
              "`teammode = single` should be true in a single fight");

        af::Character simulChar;
        simulChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, not simul]\ntype = ChangeState\ntrigger1 = teammode != simul\n"
            "value = 921\n\n"
            "[Statedef 921]\ntype = S\nphysics = S\nctrl = 0\n",
            "[Statedef -1]\n");
        af::StateMachine simulMachine;
        simulMachine.setup(&simulChar, 1);
        simulMachine.setOpponent(&dummy);
        simulMachine.setPosition(320.0f);
        simulMachine.setStageBounds(90.0f, 550.0f);
        af::InputState simulIdle;
        simulMachine.control(simulIdle);
        simulMachine.step();
        check(simulMachine.stateNumber() == 921,
              "`teammode != simul` should be true in a single fight");

        af::Character turnsChar;
        turnsChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, turns]\ntype = ChangeState\ntrigger1 = teammode = turns\nvalue = 922\n\n"
            "[Statedef 922]\ntype = S\nphysics = S\nctrl = 0\n",
            "[Statedef -1]\n");
        af::StateMachine turnsMachine;
        turnsMachine.setup(&turnsChar, 1);
        turnsMachine.setOpponent(&dummy);
        turnsMachine.setPosition(320.0f);
        turnsMachine.setStageBounds(90.0f, 550.0f);
        for (int i = 0; i < 3; ++i) {
            af::InputState turnsIdle;
            turnsMachine.control(turnsIdle);
            turnsMachine.step();
        }
        check(turnsMachine.stateNumber() == 0,
              "`teammode = turns` should be false in a single fight");
    }

    // --------------------------------------------------------- VictoryQuote
    //
    // The controller only *records* an intent: an index that names a quote the
    // character actually has. Every other value -- absent (the documented -1),
    // negative, or past the last quote -- leaves the machine on the random
    // marker, which the victory screen turns into a random present quote. The
    // controller runs on both players, so what matters is that it records on
    // this machine and not anywhere global.
    {
        const std::string quoteSection =
            "[Quotes]\nvictory0 = \"Zero\"\nvictory3 = \"Three\"\n"
            "victory99 = \"Ninety-nine\"\n\n";

        // Builds a character whose state 0 runs one VictoryQuote every tick,
        // drives it for a tick, and reports what the machine recorded. `asHelper`
        // makes the same run on a helper, where the controller must do nothing.
        auto runQuote = [&dummy](const std::string& quotes, const std::string& valueLine,
                                 bool asHelper) {
            af::Character quoteChar;
            quoteChar.loadBehaviour(
                quotes +
                    "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
                    "[State 0, quote]\ntype = VictoryQuote\ntrigger1 = 1\n" +
                    valueLine,
                "[Statedef -1]\n");

            af::StateMachine parent;
            if (asHelper) parent.setup(&quoteChar, 1);
            af::StateMachine machine;
            machine.setup(&quoteChar, 1);
            machine.setOpponent(&dummy);
            machine.setPosition(320.0f);
            machine.setStageBounds(90.0f, 550.0f);
            if (asHelper) machine.becomeHelper(&parent, 1, "quotehelper", false, 0, 0, 0);

            af::InputState idleInput;
            machine.control(idleInput);
            machine.step();
            return machine.victoryQuoteIndex();
        };

        check(runQuote(quoteSection, "value = 3\n", false) == 3,
              "VictoryQuote stores an index whose quote exists");
        check(runQuote(quoteSection, "value = 0\n", false) == 0,
              "quote 0 is selectable, not confused with the random marker");
        check(runQuote(quoteSection, "value = 99\n", false) == 99,
              "the last legal quote index is selectable");
        check(runQuote(quoteSection, "value = 2\n", false) == -1,
              "an index with no quote falls back to the random marker");
        check(runQuote(quoteSection, "value = 100\n", false) == -1,
              "an index past victory99 falls back to the random marker");
        check(runQuote(quoteSection, "value = -1\n", false) == -1,
              "the documented -1 default is the random marker");
        check(runQuote(quoteSection, "", false) == -1,
              "an omitted value uses the -1 default");
        check(runQuote("[Quotes]\n", "value = 3\n", false) == -1,
              "a character with no [Quotes] cannot select one");
        check(runQuote(quoteSection, "value = 3\n", true) == -1,
              "a helper running VictoryQuote records nothing");
    }

    // A selection belongs to the match, not to a round: StateMachine::setup()
    // runs again for every round and must leave it alone, while the match setup
    // clears it so it cannot leak into the next match's victory screen.
    {
        af::Character quoteChar;
        quoteChar.loadBehaviour(
            "[Quotes]\nvictory3 = \"Three\"\n\n"
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, quote]\ntype = VictoryQuote\ntrigger1 = 1\nvalue = 3\n",
            "[Statedef -1]\n");
        af::StateMachine machine;
        machine.setup(&quoteChar, 1);
        machine.setOpponent(&dummy);
        machine.setPosition(320.0f);
        machine.setStageBounds(90.0f, 550.0f);
        af::InputState idleInput;
        machine.control(idleInput);
        machine.step();
        check(machine.victoryQuoteIndex() == 3, "the selection was recorded before the round ends");

        machine.setup(&quoteChar, 1);
        check(machine.victoryQuoteIndex() == 3,
              "a round reset keeps the match's quote selection");
        machine.resetVictoryQuote();
        check(machine.victoryQuoteIndex() == -1,
              "the per-match reset clears the quote selection");
    }

    // ------------------------------------------------- projectile contact
    //
    // `ProjHit` / `ProjGuarded` / `ProjContact` and their `*Time` forms read the
    // owning side's last contact (the reference's player-side `+0x25C` family),
    // not a live projectile. The sink supplies the record, which is what the
    // fight screen's passes otherwise fill in.
    {
        af::Character contactChar;
        contactChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                  "[Statedef -1]\n");
        af::StateMachine machine;
        RecordingEntities sink;
        machine.setup(&contactChar, 1);
        machine.setOpponent(&dummy);
        machine.setPosition(320.0f);
        machine.setStageBounds(90.0f, 550.0f);
        machine.setEntitySink(&sink);

        const auto value = [&machine](const char* name,
                                      const std::vector<af::Value>& args = {}) {
            return machine.trigger(name, args).toFloat();
        };

        // No contact yet: false / -1, not "0 ticks ago".
        check(value("projhit") == 0.0f, "ProjHit is false before any contact");
        check(value("projhittime", {af::Value::number(0)}) == -1.0f,
              "ProjHitTime is -1 before any contact");

        // A hit, one tick old. Every documented spelling resolves the id.
        af::recordProjectileContact(sink.contacts[0], af::kProjectileContactHit, 1234);
        af::advanceProjectileContact(sink.contacts[0]);
        check(value("projhit") == 1.0f, "ProjHit fires on the tick after a hit");
        check(value("projhit", {af::Value::number(1234)}) == 1.0f,
              "the function form filters by id");
        check(value("projhit", {af::Value::number(456)}) == 0.0f,
              "a different id does not fire");
        check(value("projhit1234") == 1.0f, "the documented suffix form fires");
        check(value("projhit456") == 0.0f, "the suffix form filters by id");
        check(value("projhittime", {af::Value::number(0)}) == 1.0f,
              "ProjHitTime(0) checks every projectile");
        check(value("projhittime", {af::Value::number(1234)}) == 1.0f,
              "ProjHitTime answers the matching id");
        check(value("projhittime", {af::Value::number(456)}) == -1.0f,
              "ProjHitTime rejects a different id");
        check(value("projcontact") == 1.0f, "ProjContact includes a hit");
        check(value("projguarded") == 0.0f, "ProjGuarded does not");

        // One more tick: the one-tick boolean window closes, the clock does not.
        af::advanceProjectileContact(sink.contacts[0]);
        check(value("projhit") == 0.0f, "ProjHit is only true for one tick");
        check(value("projhittime", {af::Value::number(0)}) == 2.0f,
              "ProjHitTime keeps counting");

        // A guard answers ProjGuarded and ProjContact, never ProjHit.
        af::recordProjectileContact(sink.contacts[0], af::kProjectileContactGuarded, 77);
        af::advanceProjectileContact(sink.contacts[0]);
        check(value("projguarded") == 1.0f, "ProjGuarded fires on a block");
        check(value("projcontact") == 1.0f, "ProjContact includes a block");
        check(value("projhit") == 0.0f, "ProjHit does not");
        check(value("projguardedtime", {af::Value::number(0)}) == 1.0f,
              "ProjGuardedTime answers the clock");
        check(value("projhittime", {af::Value::number(0)}) == -1.0f,
              "ProjHitTime does not see a block");

        // A cancel (code 3) is recorded but is none of the player-contact
        // triggers; the docs' ProjContact is "hit or guarded".
        af::recordProjectileContact(sink.contacts[0], af::kProjectileContactCancelled, 9);
        af::advanceProjectileContact(sink.contacts[0]);
        check(value("projhit") == 0.0f && value("projguarded") == 0.0f &&
                  value("projcontact") == 0.0f,
              "a cancel answers none of the contact triggers");
        check(value("projcontacttime", {af::Value::number(0)}) == -1.0f,
              "and ProjContactTime is -1");

        // Side separation: a contact on the other side is invisible here.
        af::recordProjectileContact(sink.contacts[1], af::kProjectileContactHit, 5);
        af::advanceProjectileContact(sink.contacts[1]);
        check(value("projhit") == 0.0f, "the other side's contact does not leak across");

        // Helpers answer the neutral value; projectile bookkeeping belongs to
        // the root in this engine.
        af::StateMachine helper;
        helper.setup(&contactChar, 1);
        helper.setOpponent(&dummy);
        helper.setPosition(320.0f);
        helper.setStageBounds(90.0f, 550.0f);
        helper.setEntitySink(&sink);
        helper.becomeHelper(&machine, 1, "contacthelper", false, 0, 0, 0);
        af::recordProjectileContact(sink.contacts[0], af::kProjectileContactHit, 1234);
        af::advanceProjectileContact(sink.contacts[0]);
        check(helper.trigger("projhit", {}).toFloat() == 0.0f,
              "a helper does not answer ProjHit");
        check(helper.trigger("projhittime", {af::Value::number(0)}).toFloat() == -1.0f,
              "a helper does not answer ProjHitTime");

        // The layer's side-keyed store: each pass calls it with the owning
        // side, the projectile id and its code. Side separation and the
        // per-round clear are what the trigger's reading depends on.
        af::EntityLayer layer;
        layer.noteProjectileContact(0, 42, af::kProjectileContactHit);
        check(layer.projectileContact(0).id == 42 &&
                  layer.projectileContact(0).code == af::kProjectileContactHit,
              "the layer records a hit against the owning side");
        check(!layer.projectileContact(1).active,
              "the other side's record stays untouched");
        layer.noteProjectileContact(1, 7, af::kProjectileContactGuarded);
        check(layer.projectileContact(1).id == 7 &&
                  layer.projectileContact(0).id == 42,
              "both sides keep their own last contact");
        layer.noteProjectileContact(5, 9, af::kProjectileContactHit);
        check(layer.projectileContact(0).id == 42 && layer.projectileContact(1).id == 7,
              "an out-of-range side writes nothing");
        layer.clear();
        check(!layer.projectileContact(0).active && !layer.projectileContact(1).active,
              "a round clear forgets both sides' contacts");
    }

    // ------------------------------------------------- the parity fix list
    //
    // The §14 items that had no direct coverage. Each one is a rule read out of
    // `common1.cns` or the reference's binary; the state files below are written
    // to make the rule's answer visible in the machine's public state rather
    // than inferred from the code that implements it.
    {
        // --- N3(c): only -1 holds. Any other negative is zero ticks.
        af::AnimationSet skipped;
        af::parseAirText("[Begin Action 903]\n0,0, 0,0, -2\n0,1, 0,0, 3\n", skipped);
        const af::Animation* skip = skipped.find(903);
        check(skip != nullptr, "an action with a negative duration should parse");
        if (skip != nullptr) {
            check(!skip->infinite,
                  "a duration of -2 is not a hold -- only -1 is the no-loop element");
            check(skip->frameAt(0) == 1,
                  "and the element lasts zero ticks, so the next one is on screen at once");
        }

        // --- N2: state 50 picks its rising pose from the horizontal velocity,
        // then re-dresses to the falling pose once the climb is nearly spent.
        af::Character jumpChar;
        af::parseAirText(
            "[Begin Action 40]\n0,0, 0,0, -1\n\n[Begin Action 41]\n0,1, 0,0, -1\n\n"
            "[Begin Action 42]\n0,2, 0,0, -1\n\n[Begin Action 43]\n0,3, 0,0, -1\n\n"
            "[Begin Action 44]\n0,4, 0,0, -1\n\n[Begin Action 45]\n0,5, 0,0, -1\n\n"
            "[Begin Action 46]\n0,6, 0,0, -1\n",
            jumpChar.animations());
        af::StateMachine jump;
        // A `ChangeState` guarded by a variable this test sets. The built-in
        // state table is reached the way a character reaches it -- through a
        // controller -- rather than by a test-only door into `changeState`.
        const auto enterJump = [&](float vx, float vy) {
            jumpChar.loadBehaviour(
                "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                "[Statedef -1]\n[State -1, go]\ntype = ChangeState\n"
                "trigger1 = var(0) = 1 && stateno = 0\nvalue = 50\n");
            jump.setup(&jumpChar, 1);
            jump.setOpponent(&dummy);
            jump.setPosition(320.0f);
            jump.setStageBounds(90.0f, 550.0f);
            jump.setVelocity(vx, vy);
            jump.assignVar('v', 0, 1.0f);
            af::InputState tick;
            jump.control(tick);
            jump.step();
        };
        enterJump(0.0f, 8.0f);
        check(jump.stateNumber() == af::CommonState::kAir && jump.animNumber() == 41,
              "state 50 should take 41 when the jump has no horizontal velocity");
        jump.setVelocity(0.0f, 1.0f);
        af::InputState rise;
        jump.control(rise);
        jump.step();
        check(jump.animNumber() == 44,
              "and `Anim + 3` once `Vel y > Const720p(-8)` -- the falling art, 44 here");
        enterJump(1.0f, 8.0f);
        check(jump.animNumber() == 42,
              "42 when it travels forward, which is `vel x > 0` in the file's frame");
        enterJump(-1.0f, 8.0f);
        check(jump.animNumber() == 43,
              "and 43 when it travels back, re-dressing to 46 as its climb ends");

        // --- state 52 gives control back at Time = 3, not when the pose ends.
        af::Character landChar;
        af::parseAirText("[Begin Action 47]\n0,7, 0,0, 8\n", landChar.animations());
        af::StateMachine land;
        landChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
            "[Statedef -1]\n[State -1, go]\ntype = ChangeState\n"
            "trigger1 = var(0) = 1 && stateno = 0\nvalue = 52\n");
        land.setup(&landChar, 1);
        land.setOpponent(&dummy);
        land.setPosition(320.0f);
        land.setStageBounds(90.0f, 550.0f);
        land.assignVar('v', 0, 1.0f);
        af::InputState landing;
        land.control(landing);
        land.step();
        check(land.animNumber() == 47, "state 52 should take its own landing pose");
        check(!land.ctrl(), "and it enters without control");
        for (int i = 0; i < 4; ++i) {
            af::InputState tick;
            land.control(tick);
            land.step();
        }
        check(land.stateNumber() == af::CommonState::kLand && land.ctrl(),
              "`[State 52, 4] CtrlSet value = 1 / trigger1 = Time = 3` should hand control "
              "back while the pose is still held");

        // --- N3(a): a `ChangeState` that writes `ctrl = 0` revokes control even
        // when the destination's header grants it.
        af::Character ctrlChar;
        ctrlChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 400\nctrl = 0\n\n"
            "[Statedef 400]\ntype = S\nphysics = S\nctrl = 1\n",
            "");
        af::StateMachine ctrlMachine;
        ctrlMachine.setup(&ctrlChar, 1);
        ctrlMachine.setOpponent(&dummy);
        ctrlMachine.setPosition(320.0f);
        ctrlMachine.setStageBounds(90.0f, 550.0f);
        af::InputState ctrlTick;
        ctrlMachine.control(ctrlTick);
        ctrlMachine.step();
        check(ctrlMachine.stateNumber() == 400 && !ctrlMachine.ctrl(),
              "`ChangeState ctrl = 0` must take control away even though 400's header grants it");

        af::Character keptChar;
        keptChar.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[State 0, go]\ntype = ChangeState\ntrigger1 = time = 0\nvalue = 700\n\n"
            "[Statedef 700]\ntype = S\nphysics = S\nctrl = 1\n",
            "");
        af::StateMachine keptMachine;
        keptMachine.setup(&keptChar, 1);
        keptMachine.setOpponent(&dummy);
        keptMachine.setPosition(320.0f);
        keptMachine.setStageBounds(90.0f, 550.0f);
        af::InputState keptTick;
        keptMachine.control(keptTick);
        keptMachine.step();
        check(keptMachine.stateNumber() == 700 && keptMachine.ctrl(),
              "while an omitted `ctrl` leaves the destination header's answer alone");

        // --- N5: `AnimExist` reads the file the fighter is running, which a
        // `p2stateno` borrow changes; `SelfAnimExist` always reads its own.
        af::Character borrowFile;
        borrowFile.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[Statedef 9990]\ntype = S\nphysics = S\nctrl = 0\n",
            "[Statedef -1]\n");
        af::parseAirText("[Begin Action 500]\n0,0, 0,0, -1\n", borrowFile.animations());
        af::Character borrowingVictimChar;
        borrowingVictimChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                          "[Statedef -1]\n");
        af::StateMachine borrowingVictim;
        borrowingVictim.setup(&borrowingVictimChar, 1);
        borrowingVictim.setOpponent(&dummy);
        borrowingVictim.setPosition(360.0f);
        borrowingVictim.setStageBounds(90.0f, 550.0f);
        af::StateMachine::HitInfo borrowHit;
        borrowHit.damage = 0;
        borrowHit.hitTime = 10;
        borrowHit.attackerFacing = 1;
        borrowHit.victimPause = 0;
        borrowHit.stateNumber = 9990;
        borrowHit.stateSourceChar = &borrowFile;
        check(borrowingVictim.applyHit(borrowHit), "the borrow should land");
        check(borrowingVictim.trigger("animexist", {af::Value::number(500)}).toFloat() != 0.0f,
              "AnimExist should see the borrowed file's action 500");
        check(borrowingVictim.trigger("selfanimexist", {af::Value::number(500)}).toFloat() ==
                  0.0f,
              "while SelfAnimExist still asks the victim's own file, which has no 500");

        af::Character ownAnimVictimChar;
        ownAnimVictimChar.loadBehaviour("[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n",
                                        "[Statedef -1]\n");
        af::parseAirText("[Begin Action 500]\n0,0, 0,0, -1\n",
                         ownAnimVictimChar.animations());
        af::Character plainBorrowFile;
        plainBorrowFile.loadBehaviour(
            "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
            "[Statedef 9990]\ntype = S\nphysics = S\nctrl = 0\n",
            "[Statedef -1]\n");
        af::StateMachine ownAnimVictim;
        ownAnimVictim.setup(&ownAnimVictimChar, 1);
        ownAnimVictim.setOpponent(&dummy);
        ownAnimVictim.setPosition(360.0f);
        ownAnimVictim.setStageBounds(90.0f, 550.0f);
        af::StateMachine::HitInfo plainBorrowHit = borrowHit;
        plainBorrowHit.stateSourceChar = &plainBorrowFile;
        check(ownAnimVictim.applyHit(plainBorrowHit), "the plain borrow should land");
        check(ownAnimVictim.trigger("animexist", {af::Value::number(500)}).toFloat() == 0.0f,
              "AnimExist follows the borrow out of a file that lacks 500");
        check(ownAnimVictim.trigger("selfanimexist", {af::Value::number(500)}).toFloat() != 0.0f,
              "and SelfAnimExist still finds the victim's own 500");

        // --- N6: the shake clock's boundary is `<= 0`, and the clock really is
        // the hit's pause.
        af::Character shakeVictimChar;
        af::StateMachine shakeVictim;
        makeIdle(shakeVictimChar, shakeVictim);
        af::StateMachine::HitInfo shakeHit;
        shakeHit.damage = 0;
        shakeHit.hitTime = 10;
        shakeHit.victimPause = 5;
        shakeHit.attackerFacing = 1;
        shakeHit.stateNumber = af::CommonState::kHitHigh;
        check(shakeVictim.applyHit(shakeHit), "the shaking hit should land");
        check(shakeVictim.trigger("hitshakeover", {}).toFloat() == 0.0f,
              "hitshakeover is false while the shake clock is still running");
        for (int i = 0; i < 5; ++i) {
            af::InputState tick;
            shakeVictim.control(tick);
            shakeVictim.step();
        }
        check(shakeVictim.trigger("hitshakeover", {}).toFloat() != 0.0f,
              "and true once it reaches zero");

        // Reaches a built-in state the way a hit does. Checking the header the
        // moment `applyHit` returns is the point: a *step* would run the
        // state's built-in hand-over, and 5040 leaves for 5030 as soon as its
        // shake is over.
        const auto landBuiltin = [&](af::StateMachine& m, int stateNo) {
            af::StateMachine::HitInfo hit;
            hit.damage = 0;
            hit.hitTime = 20;
            hit.victimPause = 0;
            hit.attackerFacing = 1;
            hit.stateNumber = stateNo;
            return m.applyHit(hit);
        };

        // --- D22: the dead lie behind the living.
        af::Character deadChar;
        af::StateMachine dead;
        makeIdle(deadChar, dead);
        check(landBuiltin(dead, af::CommonState::kDead), "the fatal pose should be entered");
        check(dead.stateNumber() == af::CommonState::kDead && dead.spritePriority() == -3,
              "5150 should write `sprpriority = -3`, so a body does not cover a fighter");

        // --- D12/D19: the rows whose header the reference writes out.
        af::Character headerChar;
        af::StateMachine header;
        makeIdle(headerChar, header);
        check(landBuiltin(header, 5040), "the air get-hit should be entered");
        check(header.stateType() == af::StateType::Air && header.moveType() == af::MoveType::Hit &&
                  !header.ctrl(),
              "5040 is airborne, `movetype = H`, no control");
        check(landBuiltin(header, 5080), "the lying-down get-hit should be entered");
        check(header.stateType() == af::StateType::Liedown &&
                  header.physics() == af::Physics::None,
              "5080 is the lying-down get-hit with `physics = N`");
        check(landBuiltin(header, 5081), "the knocked-back lie should be entered");
        check(header.stateType() == af::StateType::Liedown &&
                  header.physics() == af::Physics::Crouch,
              "and 5081 the same posture with the file's crouching physics");

        // --- D15: the knock-back slides free for `slidetime`, then loses 40% a
        // tick. A hit's `ground.velocity` is what makes the difference visible.
        af::Character slideChar;
        af::StateMachine slide;
        makeIdle(slideChar, slide);
        af::StateMachine::HitInfo slideHit;
        slideHit.damage = 0;
        slideHit.hitTime = 20;
        slideHit.victimPause = 0;
        slideHit.attackerFacing = 1;
        slideHit.pushX = -8.0f;
        slideHit.slideTime = 2;
        slideHit.stateNumber = af::CommonState::kHitHigh;
        check(slide.applyHit(slideHit), "the sliding hit should land");
        af::InputState slideTick;
        slide.control(slideTick);
        slide.step();  // 5000 hands over to 5001 on its shake clock
        check(slide.stateNumber() == af::CommonState::kHitHighKb,
              "the flat reaction should hand over to 5001");
        check(std::fabs(slide.velX() + 8.0f) < 0.01f,
              "the knock-back keeps the hit's velocity untouched for `slidetime` ticks");
        slide.control(slideTick);
        slide.step();
        check(std::fabs(slide.velX() + 8.0f) < 0.01f,
              "and still at full speed on the tick after the hand-over");
        slide.control(slideTick);
        slide.step();
        check(std::fabs(slide.velX() + 4.8f) < 0.01f,
              "until `VelMul x = .6` takes it down once `Time >= GetHitVar(slidetime)`, not "
              "the standing friction");
    }

    if (failures == 0) std::printf("ok\n");
    return failures == 0 ? 0 : 1;
}

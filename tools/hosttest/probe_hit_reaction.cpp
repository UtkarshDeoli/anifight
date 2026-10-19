// An instrumented trace of one hit reaction, tick by tick.
//
// Not part of run_all_tests.sh: this is the probe that answers a question the
// documentation could not, and it prints rather than asserts. The question is
// whether the victim of a hit ever runs its *own* controllers -- the ones a
// character's common1.cns keeps its get-hit states in -- because
// `runStateLogic()` returns early while `stunTicks_ > 0`, and if it does not,
// then every character-authored reaction in the corpus is dead code and the
// animtype / slidetime / yaccel work is inert.
//
// The victim is synthetic so the trace shows the engine's behaviour and not
// somebody's animation: every state it may be put into pings its own sound, so
// a ping in the trace is a tick on which *the character's controllers for that
// state ran*. The ping's sample number is the state's own `time`, which makes
// the trace readable as "state N, tick T".
//
// Build (from the repo root):
//   clang++ -std=c++20 -O1 -Wall -Wextra \
//     -I app/src/main/cpp -I tools/hosttest/shim \
//     -o /tmp/probe_hit_reaction \
//     tools/hosttest/probe_hit_reaction.cpp \
//     tools/hosttest/shim/sprite_bank_stub.cpp \
//     app/src/main/cpp/engine/statemachine.cpp \
//     app/src/main/cpp/engine/cns.cpp app/src/main/cpp/engine/cmd.cpp \
//     app/src/main/cpp/engine/expr.cpp app/src/main/cpp/engine/character.cpp \
//     app/src/main/cpp/engine/anim.cpp app/src/main/cpp/engine/fighter.cpp \
//     app/src/main/cpp/engine/entities.cpp app/src/main/cpp/audio/snd_bank.cpp \
//     app/src/main/cpp/core/ini.cpp

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "engine/character.h"
#include "engine/statemachine.h"

namespace {

// Reads a character's files straight off the filesystem, the same three-line
// shim tools/hosttest/test_statemachine.cpp brings.
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

class PingSink : public af::SoundSink {
public:
    void play(int group, int sample) override { pings.push_back({group, sample}); }
    void clear() { pings.clear(); }
    std::vector<std::pair<int, int>> pings;
};

// Every state the engine can put a hit victim into, plus the ones a character's
// common1.cns hands over to, each one pinging on every tick its own controllers
// run. The hand-over controllers are the reference's own shape, reduced to the
// one trigger that matters: 5000 → 5030 on `hitshakeover` when the hit launched
// (which is where the engine already puts a launch, so the state is a witness
// for "did the file get a turn").
std::string victimCns() {
    std::string cns;
    auto state = [&](int number, const char* header, const char* extra) {
        cns += "[Statedef " + std::to_string(number) + "]\n" + header + "\n";
        // The ping: group is the state number, sample is the state's own clock.
        cns += "[State " + std::to_string(number) + ", ping]\n"
               "type = PlaySnd\n"
               "trigger1 = 1\n"
               "value = " + std::to_string(number) + ", time\n";
        if (extra != nullptr) cns += extra;
        cns += "\n";
    };

    state(0, "type = S\nphysics = S\nctrl = 1", nullptr);
    state(5000, "type = S\nmovetype = H\nphysics = N\nctrl = 0\nvelset = 0,0", nullptr);
    state(5001, "type = S\nmovetype = H\nphysics = S\nctrl = 0", nullptr);
    state(5010, "type = C\nmovetype = H\nphysics = N\nctrl = 0\nvelset = 0,0", nullptr);
    state(5011, "type = C\nmovetype = H\nphysics = S\nctrl = 0", nullptr);
    state(5020, "type = A\nmovetype = H\nphysics = N\nctrl = 0\nvelset = 0,0", nullptr);
    state(5030, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5035, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5050, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5070, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5071, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5080, "type = L\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5081, "type = L\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5100, "type = L\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5101, "type = L\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5110, "type = L\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5120, "type = L\nmovetype = H\nphysics = S\nctrl = 0", nullptr);
    state(5150, "type = L\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5200, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    state(5210, "type = A\nmovetype = H\nphysics = N\nctrl = 0", nullptr);
    return cns;
}

// The attacker's own file, so a HitDef can be armed and read back. The numbers
// under test are written on the HitDef itself, one scenario at a time.
const char* kAttackerCns =
    "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
    "[Statedef 200]\ntype = S\nmovetype = A\nphysics = S\nctrl = 0\n\n"
    "[State 200, hold]\ntype = HitDef\ntrigger1 = time = 0\n"
    "attr = S, NA\ndamage = 20\npausetime = 0, 0\n"
    "guardflag = MA\n";

// The reference's own falling shape, reduced to the controllers that matter: the
// gravity a launched victim keeps itself in the air with, the velocity restore
// that lands it there in the first place, and the hand-over to the ground on the
// way down. State 5030 and 5050 both declare `physics = N`, so nothing here is the
// engine's -- every tick of the arc is one of these controllers.
const char* kFallingVictimCns =
    "[Statedef 0]\ntype = S\nphysics = S\nctrl = 1\n\n"
    "[Statedef 5030]\ntype = A\nmovetype = H\nphysics = N\nctrl = 0\n\n"
    // The ping proves the state's controllers ran on a tick; the var records what
    // `GetHitVar(yaccel)` evaluated to *inside a controller parameter*, which is a
    // different path through the expression parser from a trigger. It is scaled by
    // 100 because a var is an integer: `VarSet` rounds, and the whole question is
    // whether 0.35 survives that or arrives as the zero an unknown name gives.
    "[State 5030, ping]\ntype = PlaySnd\ntrigger1 = 1\nvalue = 5030, time\n\n"
    "[State 5030, note]\ntype = VarSet\ntrigger1 = 1\nv = 7\n"
    "value = GetHitVar(yaccel) * 100\n\n"
    "[State 5030, gravity]\ntype = VelAdd\ntrigger1 = 1\ny = GetHitVar(yaccel)\n\n"
    "[State 5030, velset]\ntype = HitVelSet\ntrigger1 = Time = 0\nx = 1\ny = 1\n\n"
    "[State 5030, tofall]\ntype = ChangeState\ntrigger1 = Vel Y > 0\n"
    "trigger1 = Pos Y >= 0\nvalue = 5050\n\n"
    "[Statedef 5050]\ntype = A\nmovetype = H\nphysics = N\nctrl = 0\n\n"
    "[State 5050, ping]\ntype = PlaySnd\ntrigger1 = 1\nvalue = 5050, time\n\n"
    "[State 5050, gravity]\ntype = VelAdd\ntrigger1 = 1\ny = GetHitVar(yaccel)\n\n"
    "[State 5050, toground]\ntype = ChangeState\ntrigger1 = Vel Y > 0\n"
    "trigger1 = Pos Y >= 0\nvalue = 5100\n\n"
    // 5100 is what the reference hands a landing victim to -- not straight to the
    // lying-down state -- and 5100 is what plants it on the floor and stops it.
    "[Statedef 5100]\ntype = L\nmovetype = H\nphysics = N\nctrl = 0\n\n"
    "[State 5100, ping]\ntype = PlaySnd\ntrigger1 = 1\nvalue = 5100, time\n\n"
    "[State 5100, plant]\ntype = PosSet\ntrigger1 = Time = 0\ny = 0\n\n"
    "[State 5100, stop]\ntype = VelSet\ntrigger1 = Time = 0\ny = 0\n\n"
    "[State 5100, down]\ntype = ChangeState\ntrigger1 = Time = 0\n"
    "trigger1 = GetHitVar(fall.yvel) = 0\nvalue = 5110\n\n"
    "[Statedef 5110]\ntype = L\nmovetype = H\nphysics = N\nctrl = 0\n\n"
    "[State 5110, ping]\ntype = PlaySnd\ntrigger1 = 1\nvalue = 5110, time\n\n"
    "[State 5110, up]\ntype = ChangeState\ntrigger1 = Time > 10\nvalue = 5120\n\n"
    "[Statedef 5120]\ntype = L\nmovetype = H\nphysics = S\nctrl = 0\n\n"
    "[State 5120, up]\ntype = ChangeState\ntrigger1 = AnimTime = 0\nvalue = 0\n";

// Pressing x puts the attacker into 200, which is where the HitDef above arms.
// `changeState` is private on purpose -- a state is entered the way a character
// enters it -- so the probe goes through the command state like a player would.
const char* kAttackerCmd =
    "[Command]\nname = \"x\"\ncommand = x\n\n"
    "[Statedef -1]\n"
    "[State -1, attack]\ntype = ChangeState\ntrigger1 = command = \"x\"\n"
    "trigger1 = ctrl\nvalue = 200\n";

void trace(const char* title, const std::string& hitParams, int ticks,
           float victimY = 0.0f, af::Character* victimOverride = nullptr,
           const char* victimCnsText = nullptr) {
    af::Character attackerChar;
    attackerChar.loadBehaviour(
        std::string(kAttackerCns) + hitParams +
            "\n[State 200, end]\ntype = ChangeState\ntrigger1 = time > 200\nvalue = 0\n",
        kAttackerCmd);

    af::Character synthetic;
    if (victimOverride == nullptr) {
        synthetic.loadBehaviour(victimCnsText != nullptr ? victimCnsText : victimCns(),
                                "[Statedef -1]\n");
    }
    af::Character& victimChar = victimOverride != nullptr ? *victimOverride : synthetic;

    af::StateMachine attacker;
    attacker.setup(&attackerChar, 1);
    attacker.setPosition(320.0f);
    attacker.setStageBounds(90.0f, 550.0f);

    af::StateMachine victim;
    victim.setup(&victimChar, -1);
    victim.setPosition(360.0f);
    victim.setHeight(victimY);
    victim.setStageBounds(90.0f, 550.0f);
    victim.setOpponent(&attacker);
    attacker.setOpponent(&victim);

    PingSink sink;
    victim.setSoundSink(&sink);

    // Settle both fighters, then press x so the attacker arms its HitDef.
    for (int i = 0; i < 4; ++i) {
        af::InputState idle;
        attacker.control(idle);
        attacker.step();
        victim.control(idle);
        victim.step();
    }
    af::InputState press;
    press.held = af::Button::kX;
    press.pressed = af::Button::kX;
    attacker.control(press);
    attacker.step();
    af::InputState idle;
    attacker.control(idle);
    attacker.step();
    std::printf("    attacker state %d, armed %d\n", attacker.stateNumber(),
                attacker.hasArmedHitDef() ? 1 : 0);

    const af::StateMachine::HitInfo info = attacker.hitInfoFor(victim);
    std::printf("=== %s\n", title);
    std::printf("    chosen state %d, hitTime %d, push (%.2f, %.2f), air %d, guarded %d\n",
                info.stateNumber, info.hitTime, info.pushX, info.pushY, info.air ? 1 : 0,
                info.guarded ? 1 : 0);
    std::printf("    GetHitVar(slidetime) after the hit: %.0f\n",
                victim.trigger("gethitvar", {af::Value::text("slidetime")}).num);
    std::printf("    GetHitVar(yaccel) after the hit: %.2f\n",
                victim.trigger("gethitvar", {af::Value::text("yaccel")}).num);

    victim.applyHit(info);
    std::printf("    state right after applyHit: %d\n", victim.stateNumber());
    std::printf("    GetHitVar(slidetime) once applied: %.0f\n",
                victim.trigger("gethitvar", {af::Value::text("slidetime")}).num);

    for (int tick = 0; tick < ticks; ++tick) {
        sink.clear();
        attacker.control(idle);
        attacker.step();
        victim.control(idle);
        victim.step();

        std::printf("  t%-3d state %-5d time %-4d phys %-5s y %8.2f  vel (%.2f, %.2f)"
                    "  pause %d hitover %d shakeover %d var7 %.2f",
                    tick, victim.stateNumber(), victim.stateTime(),
                    victim.physics() == af::Physics::None
                        ? "N"
                        : victim.physics() == af::Physics::Air ? "A"
                                                               : victim.physics() == af::Physics::Crouch
                                                                     ? "C"
                                                                     : "S",
                    victim.y(), victim.velX(), victim.velY(), victim.pauseTicks(),
                    victim.trigger("hitover", {}).truthy() ? 1 : 0,
                    victim.trigger("hitshakeover", {}).truthy() ? 1 : 0,
                    victim.trigger("var", {af::Value::number(7.0f)}).num);
        if (sink.pings.empty()) {
            std::printf("   -- no controllers ran");
        } else {
            std::printf("   pings:");
            for (const auto& ping : sink.pings) {
                std::printf(" [%d at t=%d]", ping.first, ping.second);
            }
        }
        std::printf("\n");
    }
    std::printf("\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) {
        // A real character as the victim, so the same launch can be watched on a
        // file that ships its own common1.cns -- which is the case the synthetic
        // victim above cannot speak for.
        DirectorySource files(std::string(argv[1], std::string(argv[1]).find_last_of('/')));
        af::Character real;
        std::string error;
        const std::string leaf = std::string(argv[1]).substr(
            std::string(argv[1]).find_last_of('/') + 1);
        if (!real.load(files, leaf, &error)) {
            std::printf("FAIL: %s: %s\n", argv[1], error.c_str());
            return 1;
        }
        std::printf("loaded %s: %zu states\n", argv[1], real.states().size());
        trace("REAL. launch: ground.velocity = -4, -8, air.hittime = 30",
              "ground.hittime = 20\nground.slidetime = 8\n"
              "ground.velocity = -4, -8\nair.hittime = 30\n",
              140, 0.0f, &real);
        return 0;
    }

    trace("A. ground hit, no launch: ground.hittime = 20, ground.slidetime = 8",
          "ground.hittime = 20\nground.slidetime = 8\nground.velocity = -4, 0\n", 40);

    trace("B. launch: ground.velocity = -4, -8, air.hittime = 30",
          "ground.hittime = 20\nground.slidetime = 8\n"
          "ground.velocity = -4, -8\nair.hittime = 30\n",
          140);

    trace("C. trip: ground.type = Trip, ground.hittime = 20",
          "ground.hittime = 20\nground.type = Trip\nground.velocity = -4, 0\n", 40);

    trace("D. air hit: air.hittime = 30, air.velocity = -4, -6, victim starts 60 above the floor",
          "air.hittime = 30\nair.velocity = -4, -6\nground.hittime = 20\n", 120, 60.0f);

    // The same launch again, on a victim that carries the reference's own falling
    // controllers. This is the scenario the real character could not answer on its
    // own: if the arc comes out here and not there, the difference is in the file
    // and not in the engine.
    trace("E. launch on a victim that carries the reference's fall controllers",
          "ground.hittime = 20\nground.velocity = -4, -8\nair.hittime = 30\n", 90, 0.0f,
          nullptr, kFallingVictimCns);
    return 0;
}

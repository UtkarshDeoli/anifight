#include "engine/statemachine.h"

#include "engine/entities.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "core/ini.h"
#include "core/log.h"

namespace af {

namespace {

constexpr int kPowerMax = 3000;

// MUGEN's standard movement command names. The engine's own movement behaviour
// reads these, and every character that ships a common state file reads them
// too -- which is why they are MUGEN's names and not this engine's.
constexpr char kCmdUp[] = "holdup";
constexpr char kCmdDown[] = "holddown";
constexpr char kCmdFwd[] = "holdfwd";
constexpr char kCmdBack[] = "holdback";

// Ground friction, from [Movement]. MUGEN applies these automatically in its
// standing and crouching physics modes, which is what stops a knocked-back
// fighter sliding forever -- a character is not expected to damp its own
// velocity, and nearly none of them do.
constexpr float kStandFriction = 0.85f;
constexpr float kCrouchFriction = 0.82f;
constexpr float kFrictionThreshold = 0.05f;

// The virtual canvas, as the screenpack declares it in `[Config] gamewidth` and
// `gameheight`. This game ships a 240p screenpack, and the coordinate-space
// conversion triggers are expressed against its width, so this is the numerator
// of each of those conversions and not merely a number the `gamewidth` trigger
// reports.
constexpr float kScreenWidth = 320.0f;
constexpr float kScreenHeight = 240.0f;

// The widths of the three coordinate spaces MUGEN names. A length written in one
// of them becomes the player's own by the ratio of the two widths, which is why
// these are the denominators of the `const240p`/`480p`/`720p` family.
constexpr float kWidth240p = 320.0f;
constexpr float kWidth480p = 640.0f;
constexpr float kWidth720p = 1280.0f;

int nextRandom(uint32_t& seed) {
    seed = seed * 1103515245u + 12345u;
    return static_cast<int>(seed >> 22);
}

int clampPower(int value) { return std::clamp(value, -kPowerMax, kPowerMax); }

// ---------------------------------------------------------------- parameters

// A parameter written as a comma-separated list of expressions, which is how
// MUGEN writes the ones that are not single values: `damage = 30, 40`,
// `ground.velocity = -4, 0`, `offset = -10, -60`. Each part is evaluated in the
// owning context, so `x = enemynear, pos x + 10` works as well as `x = 4`.
std::vector<float> listParam(const Controller& controller, std::string_view key,
                             const TriggerContext& context) {
    std::vector<float> out;
    const std::string_view raw = controller.raw(key);
    if (raw.empty()) return out;

    const std::vector<std::string> parts = splitCommas(std::string(raw));
    out.reserve(parts.size());
    for (const std::string& part : parts) {
        const Expr expr = Expr::parse(part);
        if (!expr.valid()) continue;
        out.push_back(expr.eval(context).toFloat());
    }
    return out;
}

float firstParam(const Controller& controller, std::string_view key,
                 const TriggerContext& context, float fallback) {
    const std::vector<float> values = listParam(controller, key, context);
    return values.empty() ? fallback : values[0];
}

float secondParam(const Controller& controller, std::string_view key,
                  const TriggerContext& context, float fallback) {
    const std::vector<float> values = listParam(controller, key, context);
    return values.size() < 2 ? fallback : values[1];
}

// An object's `shadow` / `projshadow` triple. The default is `0,0,0`, which
// means "no shadow"; `-1` in the selecting component means "use the stage's
// `[Shadow] color`". Which components may select the stage colour is the
// caller's business because the reference is asymmetric: Explod reads only
// `r == -1`, Projectile reads any component (SHADOW-RE §4.4, §4.5).
//
// A value outside -1..255 is an error in the reference, which logs and falls
// back to the stage colour; doing the same keeps a malformed file looking like
// MUGEN rather than drawing a shadow with a wrapped byte.
ShadowRgb parseShadowRgb(const Controller& controller, std::string_view key,
                         const TriggerContext& context) {
    const std::vector<float> values = listParam(controller, key, context);
    if (values.empty()) return ShadowRgb{};
    int parts[3] = {0, 0, 0};
    for (size_t i = 0; i < 3 && i < values.size(); ++i) {
        parts[i] = static_cast<int>(std::lround(values[i]));
    }
    for (int value : parts) {
        if (value < -1 || value > 255) {
            AF_LOGW("%.*s: illegal shadow colour %d,%d,%d; using the stage colour",
                    static_cast<int>(key.size()), key.data(), parts[0], parts[1], parts[2]);
            return ShadowRgb{-1, 0, 0};
        }
    }
    return ShadowRgb{parts[0], parts[1], parts[2]};
}

// One animation named by a controller, and which of the two files it is in.
struct AnimTarget {
    int anim = -1;
    bool fromFightFx = false;
};

// Which file an animation *parameter* names. Two spellings are in use in MUGEN and they
// are mirror images of each other, so which letter means what is the caller's to say:
//
//   * `F_IsFightFx` -- a bare number is the character's own animation and an `F` in
//     front of it names one in the screenpack's `fightfx.air`. This is how an
//     `Explod`'s `anim` and a `Projectile`'s `projanim` are written, and the numbers
//     bear it out: the prefixed ones in this roster are `F60` and `F72`, which is
//     fightfx's dust, where the bare ones run to 1000, 5800 and 7400.
//   * `S_IsOwnFile` -- the other way round: a bare number is a fightfx animation and an
//     `S` in front of it is the character's own. This is how a `HitDef`'s `sparkno` and
//     a `SuperPause`'s `anim` are written (`S3102` is the character's animation 3102).
enum class AnimPrefix { F_IsFightFx, S_IsOwnFile };

// Reads one of those parameters. The prefix has to come off the *raw* text, because left
// to the expression parser a bare `F72` is an identifier -- and an identifier this engine
// does not know is a trigger that returns 0. So `anim = F72` quietly became animation 0,
// and animation 0 is the one animation every character has: their stand. Across the
// roster installed here that turned ten characters' super-jump landing dust and guard
// sparks into a second copy of the fighter itself, drawn additively, parked wherever the
// controller's own `postype` put it -- which for `postype = back` is a fixed point in the
// stage that slides to the edge of the screen as the camera moves.
//
// What a *malformed* prefix does is not "draw nothing", and the difference is worth
// writing down because the test suite pins it. `anim = Fxyz` is not a prefixed number, so
// it falls through to the expression evaluator, where a bare lower-case word of four
// letters or fewer is a *string literal* (that is MUGEN's rule, and it is what makes
// `statetype = A` work) -- and a non-empty string's numeric value is 1. So it lands on
// animation 1 of the character's own file. What the prefix handling buys is that it can no
// longer land on animation **0**, which is the stand, which is the whole bug. A parameter
// that fails to parse outright -- `anim = F-` -- does come back as -1 and draws nothing.
// Everything else -- `anim = var(5)`, a bare expression -- is evaluated the ordinary way
// and keeps the convention's default file.
AnimTarget animTarget(const Controller& controller, std::string_view key,
                      const TriggerContext& context, AnimPrefix convention) {
    const bool prefixedMeansFightFx = convention == AnimPrefix::F_IsFightFx;
    const char prefix = prefixedMeansFightFx ? 'F' : 'S';

    const std::string_view text = trim(controller.raw(key));
    if (text.size() > 1) {
        const char lead = text[0] >= 'a' && text[0] <= 'z'
                              ? static_cast<char>(text[0] - 'a' + 'A')
                              : text[0];
        if (lead == prefix) {
            const std::string number(trim(text.substr(1)));
            const bool negative = !number.empty() && number[0] == '-';
            const std::string_view digits =
                negative ? std::string_view(number).substr(1) : std::string_view(number);
            bool allDigits = !digits.empty();
            for (const char c : digits) {
                if (c < '0' || c > '9') {
                    allDigits = false;
                    break;
                }
            }
            if (allDigits) {
                AnimTarget out;
                out.anim = std::atoi(number.c_str());
                out.fromFightFx = prefixedMeansFightFx;
                return out;
            }
        }
    }

    AnimTarget out;
    out.anim = static_cast<int>(std::lround(firstParam(controller, key, context, -1.0f)));
    out.fromFightFx = !prefixedMeansFightFx;
    return out;
}

// The guard meter, in points out of StateMachine::kGuardEnergyMax.
//
// A blocked hit costs a seventh of the meter, so a fighter can hold off about seven
// hits before the block stops working -- near enough to Shin Budokai's feel, where an
// opponent's guard energy is something you can deliberately wear down, and far enough
// from a single jab that blocking an exchange is still worth doing. Recovery waits
// three quarters of a second before it starts and then refills in under two, so being
// left alone is rewarded but being *nearly* left alone is not.
constexpr int kGuardDrainPerBlock = 14;
constexpr int kGuardRestTicks = 45;
constexpr int kGuardRecoverPerTick = 1;

// How long a knocked-out fighter lies there after the round has ended, before
// the engine stands it up and gives it its life back.
//
// This is the engine's, not the file's: 5150 has no exit in any common state
// file -- it is the one get-hit state written with nothing to follow it -- and
// no pack in the corpus changes out of it. The reference instead watches the
// round clock: once the round is in its over state and the loser has been dead
// in 5150 for ninety ticks, it writes state 5120, resets the time in state, and
// restores the fighter's life to its maximum. Ninety ticks is a second and a
// half, which is the beat between the knock-out landing and both fighters
// squaring up for the next round.
constexpr int kKnockOutStandUpTicks = 90;

// `animtype = Light`, `air.animtype = Back`. MUGEN names the six reaction
// animations rather than numbering them, and the difference between them is
// what the victim's hit states are written against.
//
// The reference matches the *first character only*, lowercased -- not the whole
// name and not a prefix of the documented spelling. The parser (`0x47EE4A`)
// loads the first byte, calls `tolower`, subtracts `'b'`, bounds-checks the
// result against `0x13` and jumps through a twenty-entry table whose only
// populated slots are `l`->Light, `m`->Medium, `h`->Hard, `b`->Back, `u`->Up,
// `d`->DiagUp; every other letter takes the "Invalid animtype" error path. The
// same three-line scheme serves `air.animtype` (`0x47EE9B`) and `fall.animtype`
// (`0x47F019`), and the strings `Light`/`Hard`/`DiagUp` do not occur anywhere in
// the binary as comparison operands, so a whole-name match is not merely absent
// but impossible.
//
// Reading it as a *prefix* of the documented names -- which is what this used to
// do -- is close but not the same function, and the corpus is where the two come
// apart: `animtype = heavy` is written 744 times and the reference reads `h`, so
// it is Hard, while a prefix match against "hard" leaves it Light. `med` (17)
// and `down` (13) and `high` (7) are the same mistake in smaller numbers.
//
// An unrecognised first letter is the reference's error path -- it writes
// "Invalid animtype" to the console and the parse reports failure. This returns
// Light for it, which is the documented default for an omitted parameter and the
// closest thing to a sane one for a name the file got wrong; nothing in the
// corpus reaches that branch.
int parseAnimType(std::string_view text) {
    const std::string t = toLower(trim(text));
    if (t.empty()) return 0;
    switch (t[0]) {
        case 'l': return 0;  // Light
        case 'm': return 1;  // Medium
        case 'h': return 2;  // Hard -- and the reason `heavy` is Hard
        case 'b': return 3;  // Back
        case 'u': return 4;  // Up
        case 'd': return 5;  // DiagUp
        default: return 0;
    }
}

// `ground.type = High`, `Low`, `Trip`, `None`, and `air.type`, which is the same
// choice made for a victim in the air. A trip puts the victim on the floor rather
// than on their feet, and is the reason the ground hit states exist separately.
//
// The numbers are the reference's own -- 0 none, 1 high, 2 low, 3 trip -- and it
// is those that a character reads back as `GetHitVar(groundtype)`,
// `GetHitVar(airtype)` and `GetHitVar(type)`. They are not an engine detail: a
// common state file picks between its standing and crouching get-hit animations
// with `ifelse(GetHitVar(groundtype) = 1, 5000, 5010)`, so reporting the wrong
// one plays the wrong animation from a file the engine never sees.
//
// `None` ("attacks that do nothing besides pause P1 and P2") is rare enough that
// the corpus never writes it, but it is a choice of its own rather than a
// synonym for High and gets the number for it.
//
// Same first-character scheme as `animtype` above: the reference's `ground.type`
// switch (`0x47F067`, key offset `'h'`) reads `h`->High, `l`->Low, `n`->None,
// `t`->Trip and errors on anything else. Nothing in the corpus writes a number
// here -- `ground.type` and `air.type` are names in all 3,600-odd uses -- and a
// number would be the error path in the reference rather than a value.
int parseHitType(std::string_view text) {
    const std::string t = toLower(trim(text));
    if (t.empty()) return 1;  // the documented default is High
    switch (t[0]) {
        case 'h': return 1;  // High
        case 'l': return 2;  // Low
        case 'n': return 0;  // None
        case 't': return 3;  // Trip
        default: return 1;
    }
}

// The second item of a comma-separated parameter, as text, with any nesting
// respected. `secondParam` evaluates its item as an arithmetic expression, which
// is right for the numbers MUGEN puts in these lists and wrong for the names --
// `priority = 4, Miss` would reach the expression parser as a bare `Miss`, come
// back as an unknown identifier, and read as 0. So the enumerations are read off
// the raw text instead, the same way `animtype` and `guardflag` are.
std::string_view secondComponent(std::string_view raw) {
    int depth = 0;
    for (size_t i = 0; i < raw.size(); ++i) {
        const char c = raw[i];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            if (depth > 0) --depth;
        } else if (c == ',' && depth == 0) {
            return trim(raw.substr(i + 1));
        }
    }
    return {};
}

// `priority = hit_prior, hit_type`. Only `Hit` can win a clash and every other
// pairing takes both attacks out of it, so an unrecognised spelling is read as the
// default -- read as `Dodge` or `Miss` it would lose every clash it was in, which
// is a far worse answer for a typo than for it to be ignored.
StateMachine::PriorityClass parsePriorityClass(std::string_view text) {
    const std::string t = toLower(trim(text));
    if (t.compare(0, 5, "dodge") == 0) return StateMachine::PriorityClass::Dodge;
    if (t.compare(0, 4, "miss") == 0) return StateMachine::PriorityClass::Miss;
    return StateMachine::PriorityClass::Hit;
}

// The animation number a Statedef header names, which is usually a number and
// occasionally an expression (`anim = 40+var(49)`).
int animFromHeader(const StateDef& state, const TriggerContext& context) {
    if (!state.hasAnim) return -1;
    if (state.anim >= 0 && state.animRaw.find_first_not_of(" \t0123456789") == std::string::npos) {
        return state.anim;
    }
    const Expr expr = Expr::parse(state.animRaw);
    if (!expr.valid()) return state.anim;
    return static_cast<int>(std::lround(expr.eval(context).toFloat()));
}

// ------------------------------------------------------------ built-in states
//
// The header MUGEN gives each of the states it defines itself: what kind of
// state it is, how it moves, whether the fighter can act, and what art it uses.
// A character that ships a common state file defines these for itself and never
// reaches this table; a character that does not -- Kung Fu Man is one -- gets
// exactly this, and it is the difference between a fighter that plays and one
// that stands still forever.
//
// What the table has no column for is *timing*, and that is its one known
// shortcoming. A hand-over waits on `animationFinished()`, and a character with
// no animation is finished on the frame it enters a state -- so the built-in
// states before this one pass through a fighter one per tick. For the resting
// states that is invisible. For the landing chain, which is written as a
// sequence of holds (5100 and 5110 and 5120, the floor and a lie and a get-up),
// it means a character with no common1.cns lands, bounces once, and is standing
// again on the tick it stops -- where a real one lies there for the length of
// its animation. The falls themselves no longer care (see `builtinAwaitsGround`)
// and neither does the lie (the engine owns `[Data] liedown.time`); the
// remaining holds still do. Giving the table real durations means taking them
// from the common state file the reference ships, which is a separate piece of
// work and is not guessed at here.
struct BuiltinState {
    int number;
    StateType type;
    MoveType moveType;
    Physics physics;
    bool ctrl;
    int anim;
    // Where the state hands over once its animation has run out, or -1 when it
    // does not: a resting state is left by the input instead, which is how
    // MUGEN's own standing and walking states work. Zero is a real destination
    // -- back to standing -- and so cannot double as "nowhere".
    int thenState;
};

// The built-in states that are *falls*: the fighter is in the air and the state
// exists to carry it down.
//
// Their hand-over is not the animation running out. A character with no
// animation is "finished" on the frame it enters one, so an animation-driven
// hand-over walks a launched victim out of its fall while it is still sixty
// units up, hands it to a state whose physics is `Stand`, and the floor clamp
// drops it there in one step -- no landing, no fall damage, no bounce, no
// shake, and the arc the launch drew is thrown away. A fall ends where a fall
// ends, which is when the fighter reaches the ground. `land()` has already been
// called by then (the air physics clamps and calls it), so the fall's effects
// are spent before the state moves on.
[[nodiscard]] bool builtinAwaitsGround(int number) {
    switch (number) {
        case CommonState::kHitFall:      // 5050, the fall itself
        case CommonState::kHitTripFall:  // 5071, the trip's fall
        case CommonState::kHitGround:    // 5100, the floor the fall reaches
        case CommonState::kHitBounce:    // 5101, and the bounce off it
        // The recovery 5200 sinks into the floor and hands over to 5201 there,
        // and the mid-air recovery 5210 rolls in place and then falls out of it
        // ("Land on ground" in the reference: `Vel Y > 0 && Pos Y >= 0`). Both
        // end where the fight's floor is, not where their animation is.
        case CommonState::kHitFallRecover:
        case CommonState::kHitAirFallRecover:
            return true;
        default:
            return false;
    }
}

// The built-in states that are *shakes*: the fighter is held still while the pose
// jitters, and what ends the hold is the hit's own shake clock rather than the
// pose's animation.
//
// The reference writes every one of them the same way -- `physics = N`,
// `velset = 0,0`, and a single `ChangeState` on `trigger1 = HitShakeOver` -- and
// the two halves of that are one design. A victim is held frozen for the shake and
// then *recoils* on the tick the shake ends, and the state it hands over to is
// where the hit's velocity finally applies, through its own `HitVelSet x = 1`
// (5001, 5011, 5030) or `HitVelSet x = 1, y = 1` (5030, 5071). Nothing moves the
// victim before that: the shake is a phase of the reaction and not merely a pause
// inside it.
//
// Which is why the built-in table cannot time these on `animationFinished()`. A
// character with no art for the pose is "finished" on the frame it enters one, so
// the shake would be over before the freeze it exists to cover had run at all --
// and on the animation the hold then lasted as long as the *stun*, because
// `runStateLogic` keeps the whole table out of the way while `stunTicks_` is
// running. For a launcher, whose stun is `air.hittime` and runs the length of the
// flight, that is the whole reaction: the victim hung in the air in the pose they
// were hit in and only then began to fall.
//
// The *guard* shakes -- 150, 152, 154 -- are in the list too, and they are the same
// three states one family over: their hold is not the animation either. It is the
// hit's shake clock, which for a block is the second value of `guard.pausetime` --
// and that is the same number this engine already seeds `shakeTicks_` from, because
// `guard.pausetime` defaults to the raw `pausetime` pair and the blocker's shake is
// its second value. Nothing is shortened by moving them here: a block's shake is its
// `pausetime`, and the guard ctrl time is a *different* clock that the character reads
// back through `GetHitVar(ctrltime)`. The comment that used to sit here held them back
// on the belief that most HitDefs write no `guard.pausetime` and would therefore shake
// for a single tick; 2,544 of the corpus's 2,618 HitDefs write `pausetime` itself,
// which is what `guard.pausetime` inherits from, and for the handful that write
// neither, the clock the reference seeds is 0 and its shake is over on the tick it is
// entered. And it is the same number the
// reference uses: `sub_46E7C0` feeds the blocker's `+0x1090` from the active HitDef's
// `+0x180` -- `guard.p2shaketime`, the second value of `guard.pausetime` -- which is
// what this engine's guard path carries after the two clocks were separated
// (FINDINGS pass 17 §17.1).
[[nodiscard]] bool builtinExitsOnShake(int number) {
    switch (number) {
        case CommonState::kGuarded:     // 150, the standing guard shake
        case CommonState::kGuardHit:    // 152, its crouching twin
        case CommonState::kGuardedAir:  // 154, the airborne one
        case CommonState::kHitHigh:     // 5000, the standing ground reaction
        case CommonState::kHitLow:      // 5010, its crouching twin
        case CommonState::kHitAir:      // 5020, the airborne reaction
        case CommonState::kHitTrip:     // 5070, the trip
        case CommonState::kHitLiedown:  // 5080, hit while already down
            return true;
        default:
            return false;
    }
}

// The knock-back halves of the ground reactions, whose only exit in
// `common1.cns` is `trigger1 = HitOver` -- unconditionally, in the middle of
// whatever the pose is doing:
//
//   [State 5001, 6] ChangeState / trigger1 = HitOver / value = 0 / ctrl = 1
//   [State 5011, 6] ChangeState / trigger1 = HitOver / value = 11 / ctrl = 1
//   [State 5081, 4] ChangeState / trigger1 = HitOver / value = 5110
//
// The generic hand-over in `runBuiltinState` waits for the animation to run out,
// which is the wrong clock here. A hit pose commonly ends on a `-1` frame that
// never runs out, so a fighter whose `ground.slidetime` was shorter than its
// animation -- every ordinary hit -- stayed in the knock-back until something
// else moved it. The stun gate has already kept the table still until HitOver,
// so when these states reach the table the answer is simply "yes".
[[nodiscard]] bool builtinExitsOnHitOver(int number) {
    return number == CommonState::kHitHighKb || number == CommonState::kHitLowKb ||
           number == CommonState::kHitLiedownKb;
}

constexpr BuiltinState kBuiltins[] = {    {0, StateType::Stand, MoveType::Idle, Physics::Stand, true, 0, -1},
    {5, StateType::Stand, MoveType::Idle, Physics::Stand, true, 5, -1},
    // The two crouch transitions, whose types run the way the transition is
    // going rather than the way they read: `[Statedef 10]` is the *way into* the
    // crouch and is `type = C, physics = C` -- a fighter dropping into a crouch
    // is already crouching as far as a hit is concerned -- while `[Statedef 12]`
    // is the way out and is `type = S, physics = S`. They were mirrored here,
    // which put a crouching fighter's hurt height back up for the few ticks it
    // took to get down and held a standing one low on the way up.
    //
    // Neither header writes `ctrl`. An omitted `ctrl` in a `[Statedef]` is not
    // "no control" -- the get-hit states leave it out too and they are the one
    // place it must be off -- so the fighter keeps what it had, and both of
    // these are entered from states that are in control: 10 from standing and
    // from the walk, 12 from the crouch.
    {10, StateType::Crouch, MoveType::Idle, Physics::Crouch, true, 10, 11},
    {11, StateType::Crouch, MoveType::Idle, Physics::Crouch, true, 11, -1},
    {12, StateType::Stand, MoveType::Idle, Physics::Stand, true, 12, 0},
    {20, StateType::Stand, MoveType::Idle, Physics::Stand, true, 20, -1},
    {40, StateType::Stand, MoveType::Idle, Physics::Stand, false, 40, 50},
    // The air-jump takeoff. `common1.cns` writes it `type = A, physics = N,
    // velset = 0,0` and holds the pose for two ticks of its own: `[State 45, 6]`
    // sets the `velocity.airjump.*` pair at `Time = 2` and `[State 45, 7]` hands
    // over to 50 with `ctrl = 1` on the same tick. `runBuiltinState` supplies the
    // velocity and the hand-over, because the row cannot carry a two-tick clock.
    // Nothing in this engine enters 45 -- it has no air jump -- so the row is only
    // reachable by a character that changes to the reference's number itself.
    {CommonState::kAirJumpStart, StateType::Air, MoveType::Idle, Physics::Air, false, 45, 50},
    // There was a row here for state 47, carried as "MUGEN 1.0's dedicated
    // landing state". There is no such state: `data/common1.cns` defines 40, 45,
    // 50, 51 and 52 and nothing at 47, the jump chain the engine itself writes
    // lands in 52, and no state file in the corpus defines or changes to 47. The
    // row invented both the number and the animation it named, so it is gone --
    // a number the reference does not define is better served by the generic
    // default below, which leaves the animation alone rather than switching to an
    // animation that was never drawn.
    // Airborne, but in control: air attacks, an air dash and an air block are
    // all reachable from here, and every `trigger1 = ctrl` move in a character's
    // state -1 depends on this being set.
    {50, StateType::Air, MoveType::Idle, Physics::Air, true, 40, -1},
    {CommonState::kAirDown, StateType::Air, MoveType::Idle, Physics::Air, true, 40, -1},
    {52, StateType::Stand, MoveType::Idle, Physics::Stand, false, 47, 0},
    {100, StateType::Stand, MoveType::Idle, Physics::Stand, true, 100, -1},
    {102, StateType::Stand, MoveType::Idle, Physics::Stand, true, 100, -1},
    // 105 is the hop-back takeoff and 106 is its landing. The reference's own
    // `[State 105, 3] ChangeState / trigger1 = Vel Y > 0 / trigger1 = Pos Y >= 0
    // / value = 106` makes the landing a state of its own, and `[State 106, 5]`
    // then holds it for seven ticks before state 0. The engine's 105 never had a
    // route to 106 -- it went straight to 0 -- so a character that reached the
    // built-in 105 skipped the landing pose entirely. The row cannot express the
    // seven-tick hold, which is `runBuiltinState`'s narrow special case.
    {105, StateType::Stand, MoveType::Idle, Physics::Stand, false, 105, CommonState::kHopLand},
    {CommonState::kHopLand, StateType::Stand, MoveType::Idle, Physics::Stand, false, 47, 0},
    // What a blocker *holds* once its guard reaction is over. These two are only
    // ever reached as the end of that reaction -- a file-less character is put
    // into 120 by nothing, because the engine only enters the guard pose for a
    // character that ships one -- and they are here so the reaction has somewhere
    // to end that is not "stand up and drop the arms". Their exit is the input:
    // releasing back is what leaves them, which `runBuiltinMovement` owns. The
    // reference's own 130/131 are `type = S/C, physics = S/C` with no `movetype`
    // written, which means "idle" (an omitted `movetype` defaults to I), and no
    // `ctrl` written, which means unchanged.
    {130, StateType::Stand, MoveType::Idle, Physics::Stand, false, 130, -1},
    {131, StateType::Crouch, MoveType::Idle, Physics::Crouch, false, 131, -1},
    // The guard-hit shake and the knock-back each one hands over to, in the
    // reference's three heights: 150/151 standing, 152/153 crouching, 154/155
    // airborne. `hitInfoFor` picks the shake off the blocker's own posture; the
    // rest of the chain is written down here.
    //
    // All three shakes are `movetype = H` with `physics = N` and `velset = 0,0`:
    // a blocked hit is still a hit as far as the fighter is concerned -- it cannot
    // act, and a state that asks what it is doing is told "being hit" -- while the
    // pose shakes *in place*, held still for `guard.ctrltime` ticks. The knock-back
    // beside it is what moves the blocker: it is where the block's push is applied
    // (see `applyHit` and the hand-over below), and it is what walks the blocker
    // back into a guard hold rather than dropping it to state 0. Crouch-blocking a
    // hit used to stand the blocker up, because 152 claimed `type = S` and the pair
    // ended at 0 -- which is the same mistake the get-hit pair 5010/5011 had.
    {150, StateType::Stand, MoveType::Hit, Physics::None, false, 150, CommonState::kGuardedKb},
    {151, StateType::Stand, MoveType::Hit, Physics::Stand, false, 150,
     CommonState::kGuardStand},
    {152, StateType::Crouch, MoveType::Hit, Physics::None, false, 151, CommonState::kGuardHitKb},
    {153, StateType::Crouch, MoveType::Hit, Physics::Crouch, false, 151,
     CommonState::kGuardCrouch},
    {154, StateType::Air, MoveType::Hit, Physics::None, false, 152, CommonState::kGuardedAirKb},
    // 155 is the air knock-back, and it is the one guard state that asks the
    // floor a question: the reference's own landing controller sends it to 130
    // while the blocker is still holding back in guard distance and to 52 when it
    // is not, which `land()` makes the call on. The air physics are the engine's
    // -- the reference has `physics = N` and adds `Const(movement.yaccel)` in its
    // own controllers, so the engine supplies what those controllers would have.
    {155, StateType::Air, MoveType::Hit, Physics::Air, false, 152, -1},
    {180, StateType::Stand, MoveType::Idle, Physics::Stand, true, 180, -1},
    // 190 is the pre-intro. `common1.cns` writes `physics = N` by omission, picks
    // animation 190 only when the character has one (`[State 190, 1] ChangeAnim /
    // trigger1 = SelfAnimExist(190)`), and then leaves on `[State 190, 2]
    // ChangeState / trigger1 = Time = 0 / value = 191` whatever it is drawing.
    // The row used to carry `thenState = -1`, which held a file-less fighter in
    // 190 forever, and it forced animation 190 even when nothing was drawn there.
    // `enterState` decides the animation and `runBuiltinState` makes the entry-tick
    // hand-over; 191 is the character's intro override point, whose built-in half
    // is a single `ChangeState 0` at `Time = 0`.
    {190, StateType::Stand, MoveType::Idle, Physics::None, false, -1, CommonState::kIntroPoint},
    {CommonState::kIntroPoint, StateType::Stand, MoveType::Idle, Physics::None, false, -1, 0},
    // `[Statedef 5900]` -- round initialization. `type = S` and nothing else in
    // the header: movetype defaults to idle, physics to none, and the omitted
    // `ctrl` keeps whatever the fighter had. It lasts one tick; `runBuiltinState`
    // owns the hand-over because it reads the round number.
    {CommonState::kRoundInit, StateType::Stand, MoveType::Idle, Physics::None, false, -1, -1},
    // The ground reaction and the knock-back it can hand over to. 5000 is a
    // *shake* -- `physics = N`, `velset = 0,0`, out on `HitShakeOver` -- and its
    // two possible exits are the reference's own pair: `[State 5000, 6]` sends a
    // victim whose `GetHitVar(yvel) = 0 && !GetHitVar(fall)` to 5001, the flat
    // knock-back, and `[State 5000, 7]` sends the rest -- a launch -- to 5030.
    // `hitFallFlag_` is that same question answered once, at the hit, so the row's
    // `thenState` is the flat one and `runBuiltinState` overrides it for a fall.
    {5000, StateType::Stand, MoveType::Hit, Physics::Stand, false, 5000, 5001},
    {5001, StateType::Stand, MoveType::Hit, Physics::Stand, false, 5001, 0},
    // The crouching pair, the same shape one height down. 5011's hand-over is the
    // reference's own `value = 11`: a victim hit out of a crouch is put back into
    // the crouch, and does not stand up for having been hit. Letting the pair end
    // at state 0, as this did, stood a crouching victim upright the moment the
    // shake was over.
    {5010, StateType::Crouch, MoveType::Hit, Physics::Crouch, false, 5010, 5011},
    {5011, StateType::Crouch, MoveType::Hit, Physics::Crouch, false, 5011, CommonState::kCrouch},
    // An air hit: the shake, and then the two air reactions the reference splits
    // between. Which one a knocked-away victim ends in is `hitFallFlag_`, and
    // `runBuiltinState` is what makes that call -- being knocked *away* and being
    // knocked *down* are different reactions and they end in different places.
    {5020, StateType::Air, MoveType::Hit, Physics::Air, false, 5020, 5030},
    // 5030's hand-over to 5035 is the reference's `[State 5030, 6]`, which fires on
    // the 5030 animation ending rather than on `HitOver` and is therefore the one
    // route in this family that can run while the stun is still counting: see the
    // narrow exception in `runCurrentState`. `physics = N` in the reference is the
    // `VelAdd y = GetHitVar(yaccel)` in its own controllers, which `Physics::Air`
    // substitutes here like the rest of the fall family.
    {5030, StateType::Air, MoveType::Hit, Physics::Air, false, 5030, CommonState::kHitAirTransition},
    // 5035 is the transition pose. Its own exits are the same `HitFall` split as
    // 5030's -- 5040 for a non-fall, 5050 for a fall -- on `HitOver`, its
    // animation, the floor, or (in the reference) no 5035 art at all on entry.
    // `runBuiltinState` and `land()` supply the exits; the row has no `thenState`
    // because the split needs `hitFallFlag_`, and it is never reached from the
    // generic animation hand-over.
    {CommonState::kHitAirTransition, StateType::Air, MoveType::Hit, Physics::Air, false, 5035, -1},
    // 5040 is airborne and *not* falling, despite sitting between the fall and
    // the floor. Its own exit is the jump-land state -- "on its feet" -- which
    // `land()` knows about, and it is the reference's own `value = 52`.
    {5040, StateType::Air, MoveType::Hit, Physics::Air, false, 5040, -1},
    // 5050 is the fall. It is `type = A, physics = N` in the reference because
    // the file's own controllers carry it down and decide when it has landed;
    // the built-in supplies the air physics those controllers would have applied
    // and waits for the ground rather than for an animation.
    {5050, StateType::Air, MoveType::Hit, Physics::Air, false, 5050, 5100},
    // The trip: a stumble held where it stands, then the fall it turns into.
    {5070, StateType::Air, MoveType::Hit, Physics::None, false, 5070, 5071},
    {5071, StateType::Air, MoveType::Hit, Physics::Air, false, 5071, 5110},
    // Hit while already down: the shake, and then the same air-or-floor split,
    // made by `runBuiltinState` off `hitFallFlag_`. Their physics are the
    // file's own: 5080 is `physics = N` with `velset = 0,0` (the zero is
    // applied at entry), and 5081 is `physics = C` -- the crouching friction is
    // what slows a downed fighter's knock-back, since the state itself only
    // zeroes the velocity at `HitOver`.
    {5080, StateType::Liedown, MoveType::Hit, Physics::None, false, 5080, -1},
    {5081, StateType::Liedown, MoveType::Hit, Physics::Crouch, false, 5081, 5110},
    // The floor a fall reaches, and the bounce back off it. Both are airborne
    // because both are entered *moving*: the bounce is launched from `land()`,
    // and a landing with no bounce is at rest and hands over at once.
    {5100, StateType::Liedown, MoveType::Hit, Physics::Air, false, 5100, 5110},
    {5101, StateType::Liedown, MoveType::Hit, Physics::Air, false, 5101, 5110},
    // The recovery states a fall can be talked out of, in the reference's own
    // three rows.
    //
    // 5200 is written `physics = N` with its own `VelAdd y = GetHitVar(yaccel)`
    // in the controllers beside it, which is the same shape as 155's block above:
    // the engine supplies the air physics those controllers would have applied,
    // and the *floor* they were carrying the fighter to becomes the hand-over.
    // What the reference does instead is fall past the floor -- physics `N` clamps
    // nothing, and its own exit is `Pos Y >= 10`, ten units *below* the floor,
    // which is where the ground-roll anim's body sits when its feet are drawn
    // under the origin. The engine lands at the floor, ten units early, and hands
    // over to 5201 there instead. The difference is one or two frames of a pose
    // that is rolling into the floor anyway.
    {5200, StateType::Air, MoveType::Hit, Physics::Air, false, 5200, CommonState::kHitFallRecoverGround},
    // 5201 is `physics = A` and writes its whole entry state in controllers --
    // a turn toward the opponent, a hop up and slightly back, and `PosSet y = 0`
    // to put the fighter back on the floor it had sunk through. `enterState`
    // supplies those three for a character that ships no file, and the hop's own
    // physics carries it from there. The reference gives this state *no* exit at
    // all (there is no ChangeState anywhere in its 5201): it is the hop, the
    // white blink and the dust, and then the fighter is simply back on its feet,
    // which is what the engine's landing does with it.
    {5201, StateType::Air, MoveType::Hit, Physics::Air, false, 5200, -1},
    // 5210 is the mid-air recovery: `physics = N` again, `movetype = I` -- the
    // fighter is out of the hit reaction, which is what the state is *for* -- and
    // `ctrl = 0`, so it cannot act yet. Its whole content is the roll: four ticks
    // with the position frozen, then a lift whose size depends on which way the
    // stick is held, and then the reference's own "Land on ground" hand-over,
    // which is `value = 52 ;JUMP_LAND`. Hence the `thenState` below and the
    // per-tick block in `runBuiltinState`.
    {5210, StateType::Air, MoveType::Idle, Physics::None, false, 5210, CommonState::kLand},
    // The continue-screen pose. `[Statedef 5500]` is `type = S`, `ctrl = 0`,
    // `anim = 5300`, `velset = 0,0`, and it has no exit at all: the continue screen
    // holds it. There is no continue screen in this engine, so the row is inert;
    // it is here so a direct entry gets the reference's rest rather than the
    // generic controllable-standing default. `enterState` prefers the character's
    // own animation 5500 over the statedef's 5300.
    {CommonState::kContinue, StateType::Stand, MoveType::Idle, Physics::None, false, 5300, -1},
    // Both are `physics = N` in the file. The engine supplies the friction
    // those states would otherwise apply by hand -- 5110's `VelMul x = .85` and
    // its threshold stop are the stand friction, and 5120 opens with
    // `VelSet x = 0`.
    {5110, StateType::Liedown, MoveType::Hit, Physics::None, false, 5110, 5120},
    // Getting up is `movetype = I` in the reference, not `H`: the fighter is out
    // of the reaction and on its feet again, and a state that asked what it was
    // doing would be told "idle".
    {5120, StateType::Liedown, MoveType::Idle, Physics::None, false, 5120, 0},
    // Knocked out: the fighter lies where it fell, and nothing in this table
    // stands it up. The engine does that at the end of the round, on its own
    // clock, which is the reference's behaviour and the reason this row has no
    // `thenState`.
    {5150, StateType::Liedown, MoveType::Hit, Physics::None, false, 5150, -1},
};

const BuiltinState* builtinFor(int number) {
    for (const BuiltinState& state : kBuiltins) {
        if (state.number == number) return &state;
    }
    return nullptr;
}

}  // namespace

bool StateMachine::directionHeld(std::string_view command, uint32_t symbols) const {
    if (commands_ != nullptr && commands_->find(command) != nullptr) {
        return commands_->triggered(command, buffer_);
    }
    const uint32_t present = commandSymbols(lastInput_, facing_);
    return (present & symbols) == symbols;
}

// The (group, sample) pair a `value` names.
//
// `value = 5, 0` is the classic form. MUGEN 1.0 also writes `value = S5, 0`,
// where the S marks the number as a sound index rather than a sound id -- the
// two mean the same pair to a player, and the letter is peeled off before the
// part is read as an expression, because `S5` on its own is an identifier and
// an unknown identifier is zero.
std::vector<int> soundPair(const Controller& controller, std::string_view key,
                           const TriggerContext& context) {
    std::vector<int> out;
    const std::string_view raw = controller.raw(key);
    if (raw.empty()) return out;

    const std::vector<std::string> parts = splitCommas(std::string(raw));
    for (const std::string& part : parts) {
        std::string text(trim(part));
        if (!text.empty() && (text[0] == 's' || text[0] == 'S')) text.erase(text.begin());
        // A group written as a name (`value = hitsound`) is a sound the file
        // does not index; zero is the honest reading and the mixer reports the
        // miss.
        const Expr expr = Expr::parse(text);
        out.push_back(expr.valid() ? static_cast<int>(std::lround(expr.eval(context).toFloat()))
                                   : 0);
    }
    return out;
}

float StateMachine::velocityConstant(std::string_view key, float fallback) const {
    if (character_ == nullptr) return fallback;
    float value = 0.0f;
    return character_->constant(key, &value) ? value : fallback;
}

// The `recovery` command, which is how a victim asks to be let out of a fall.
//
// It is not one of the four hold directions, so `directionHeld` cannot answer it:
// that helper's raw fallback is a *symbol mask*, and a mask of zero -- no symbols
// to require -- would answer "yes" to every question ever asked of it. So the
// fallback is written out here instead, and it is the binding the reference's own
// kfm.cmd gives it:
//
//     [Command] name = "recovery";Required (do not remove)
//     command = x+y
//
// A character with a .cmd is asked its own list, which is the point of the
// command being required: a character that binds recovery to z+start gets its own
// binding, and the 128 .cmd files in local_chars that carry the reference's line
// verbatim all mean x+y anyway.
bool StateMachine::recoveryHeld() const {
    if (commands_ != nullptr && commands_->find("recovery") != nullptr) {
        return commands_->triggered("recovery", buffer_);
    }
    const uint32_t held = commandSymbols(lastInput_, facing_);
    return (held & CmdKey::button('x')) != 0 && (held & CmdKey::button('y')) != 0;
}

// `CanRecover`, in one place. The `canrecover` trigger asks this, and so does the
// fall's own recovery check, so that a character testing the trigger and the
// engine acting on it cannot come to different answers -- which is exactly the
// kind of drift that makes a character's own 5050 fire the recovery one tick
// before the engine's, or never. The window itself is the clock on the last hit,
// whose default is the reference's 60 and which a launcher shortens by writing
// `fall.recovertime` (30, 25, 20 and 4 are the common values in local_chars).
bool StateMachine::canRecover() const {
    return alive() && moveType_ == MoveType::Hit && stateType_ == StateType::Air &&
           lastHit_.fallRecover && lastHit_.fallRecoverTicks <= 0;
}

// A recovery pressed during a fall: the reference's own two controllers out of
// 5050, `[State 5050, 4]` and `[State 5050, 5]`, in its own order.
//
// The order *is* the behaviour and is why the near-ground test comes first: 5200
// is "recovering near ground" and 5210 "recovering in mid air", and both are
// gated only on `Vel Y > 0` (falling) and `Vel Y > -1` (also falling, or barely),
// so a fighter falling fast near the floor satisfies *both*. The reference
// resolves it by having written the ground one as controller 4, and 5200 is the
// one that ends on the floor -- the mid-air roll thrown instead would leave a
// victim that has already fallen most of the way to the ground rolling in place
// above it.
//
// `nohardcodedkeys` turns all of it off, like every other key the engine supplies
// on a character's behalf: a character that has taken its own input away is not
// listening for this one either.
bool StateMachine::tryFallRecover() {
    if (asserted(kAssertNoHardcodedKeys)) return false;
    if (!canRecover()) return false;
    if (!recoveryHeld()) return false;

    // `Vel Y > 0 && Pos Y >= -20`: falling, and within the
    // groundrecover.ground.threshold of the floor.
    if (velY_ < 0.0f && posY() >= -20.0f) {
        changeState(CommonState::kHitFallRecover, false);
        return true;
    }
    // `Vel Y > -1`: any fall, including one that has only just started.
    if (velY_ < 1.0f) {
        changeState(CommonState::kHitAirFallRecover, false);
        return true;
    }
    return false;
}

// MUGEN's built-in movement states, in code.
//
// These are not optional, and this is the part of the format that surprises
// people. A MUGEN character names `stcommon = common1.cns` in its .def, and
// MUGEN looks for that file in the character's own folder and then in its
// `data/` folder. A great many characters only ever have the first -- Elecbyte's
// own Kung Fu Man among them, whose folder ships no common1.cns at all -- which
// means the *engine* has to supply the states that make a fighter stand, crouch,
// walk, jump and land. Nothing in a character's files moves it from state 0 into
// state 20; check any real .cmd and state -1 holds only the specials.
//
// So this is that supply: the input that starts a walk, a crouch and a jump, the
// velocity each of those uses, and the hand-over from one state to the next.
//
// Read Elecbyte's own common1.cns and its shape settles how far the supply has
// to reach. `[Statedef 0]` (standing) contains no way into the crouch, the walk
// or the jump; `[Statedef 11]` (crouching) contains no way back to standing; and
// `[Statedef 20]` (walking) contains no way to stop. Everything else is written
// out in full -- 10 hands over to 11, 12 and 52 hand back to 0. So these
// direction-driven transitions are not the character's to write: the engine owns
// them, and it owns them for *every* character, including the many that define a
// state 0 of their own -- a tired idle, a transformation check -- without
// reimplementing the crouch inside it. Which is why runBuiltinMovement() is
// called for a state the character did define as well, after its own controllers
// have had their say.
//
// The rest of the hand-overs are only needed when the character wrote no state at
// all, because a character that defines one of those states has its own exit: it
// is standing next to this code in common1.cns.
//
// Returns true when the current state is one of these four, whether or not
// anything came of it, so the caller knows the rest of the built-in behaviour
// does not apply to it.
namespace {

// The `physics` a guard pose takes from the posture it was entered in.
//
// `[State 120, 2..4]` and `[State 140, 2..4]` write the same trio twice over --
// three `StateTypeSet`s side by side, `trigger1 = Time = 0 && statetype = S` with
// `physics = S`, then the crouching one, then the air one -- because both poses
// are `type = U`: they keep whatever posture they were entered with, and then set
// the physics to *match* it. The one that matters is the air. Physics is what
// gives a fighter gravity, and while the two grounded poses agree with their own
// types anyway, an airborne 140 would otherwise inherit `physics = N` from the
// guard hold it was released out of (132's header) and hang in the sky with its
// arms coming down. `statetype = L` fires none of the three triggers, and no lying
// fighter can reach these poses -- `canGuard` refuses one, and the guard chain is
// the only way in.
//
// The guard hold 132 asks the same question with a different answer, which is why
// it does not go through here: its header is `physics = N` and its own controllers
// add the fall by hand (`VelAdd y = Const(movement.yaccel)` every tick), so the
// engine's substitute for it is the whole of `Physics::Air`.
Physics physicsForStateType(StateType type) {
    switch (type) {
        case StateType::Crouch: return Physics::Crouch;
        case StateType::Air: return Physics::Air;
        default: return Physics::Stand;
    }
}

}  // namespace

bool StateMachine::setGuardPosture(bool crouch) {
    // The two `StateTypeSet`s that sit beside each other in 120 and 140, read the
    // way they are written: `statetype = S && command = "holddown"` goes down,
    // `statetype = C && command != "holddown"` comes back up, and a fighter in
    // the air is neither, so neither fires and its posture is left alone. The
    // type and the physics move together because that is what the file writes --
    // `statetype = C` on one line and `physics = C` on the next, and physics is
    // what decides how the fighter moves.
    if (crouch && stateType_ == StateType::Stand) {
        stateType_ = StateType::Crouch;
        physics_ = physicsForStateType(stateType_);
        return true;
    }
    if (!crouch && stateType_ == StateType::Crouch) {
        stateType_ = StateType::Stand;
        physics_ = physicsForStateType(stateType_);
        return true;
    }
    return false;
}

int StateMachine::guardedStateForPosture() const {
    switch (stateType_) {
        case StateType::Crouch: return CommonState::kGuardCrouch;
        case StateType::Air: return CommonState::kGuardAir;
        default: return CommonState::kGuardStand;
    }
}

bool StateMachine::runBuiltinMovement() {
    // `nohardcodedkeys` is a character taking its movement away from the engine: an
    // intro, a cutscene, a state where the thumbs should do nothing at all.
    if (asserted(kAssertNoHardcodedKeys)) return false;

    const bool up = directionHeld(kCmdUp, CmdKey::kUp);
    const bool down = directionHeld(kCmdDown, CmdKey::kDown);
    const bool fwd = directionHeld(kCmdFwd, CmdKey::kForward);
    const bool back = directionHeld(kCmdBack, CmdKey::kBack);

    // ---------------------------------------------------------------- guarding
    //
    // A block is holding *back*. There is no block button in the format, which is
    // why the on-screen pad needs no extra control for it and why a CPU that walks
    // away is also a CPU that defends -- and why guarding comes before the
    // direction transitions below: in range of a swinging opponent, holding back
    // means block, not walk, and a fighter that walked instead would be walking
    // into the attack it was trying to avoid.
    //
    // The state is only entered where the character has one to enter. Guarding is a
    // property of the *hit*, not of the state -- a fighter blocks because it held
    // back with control, whatever state number it happens to be standing in -- so a
    // character that ships no guard pose still defends; it just does it without
    // changing how it looks. Entering a state number nothing defines is how a
    // fighter ends up stuck in a state it cannot leave, which is the trap the crouch
    // fell into.
    if (back && inGuardDist_ && !isGuardState(stateNumber_) && canGuard()) {
        const StateDef* guard =
            stateLibrary() != nullptr ? stateLibrary()->find(CommonState::kGuardStart) : nullptr;
        if (guard != nullptr && guard->defined) {
            changeState(CommonState::kGuardStart, false);
            return true;
        }
    }

    switch (stateNumber_) {
        case CommonState::kStand:
        case 5: {
            // Standing still and free to act: a held direction starts whatever
            // it means. This is the whole of how a fighter leaves state 0.
            if (!ctrl_) return true;
            if (up) {
                changeState(CommonState::kJumpStart, false);
                return true;
            }
            if (down) {
                changeState(CommonState::kStandToCrouch, false);
                return true;
            }
            if (fwd || back) {
                // One state for both directions, because there is no [Statedef
                // 21]: the reference's own walk state picks its *animation* from
                // the sign of the velocity, so walking back is 20 as well -- see
                // `kWalkBackAnim`. Which of the two is being walked, when both
                // keys are down, the walk state itself decides.
                changeState(CommonState::kWalk, false);
                return true;
            }
            return true;
        }

        // The guard chain, written the way `data/common1.cns` writes it. Every
        // hand-over below is a controller in the reference's own guard states,
        // and they are here for a character that ships no file to carry them --
        // but a character that *does* ship one has its controllers run first,
        // so what this code sees is either a file that did not handle its own
        // guard or a fighter with no file at all.
        //
        // The shape of it, in the file's own order:
        //
        //   120  `[State 120, 5]` AnimTime = 0 -> 130/131/132, by the state type
        //        the stick left it in (`Hi to Lo` / `Lo to Hi` set the type in
        //        place, so a blocker that presses down mid-start settles into
        //        the *crouching* guard without restarting the pose).
        //   130  `[State 130, Hi to Lo]` holddown -> 131
        //   131  `[State 131, Lo to Hi]` !holddown -> 130
        //   all  `Stop Guarding`: !holdback or !inguarddist -> 140
        //   140  its own hand-over is commented out in the file as "Implemented
        //        within engine": AnimTime = 0 -> 0, 11 or 51 by state type.
        //
        // All of it but 140's exit is the *file's* where the file has it, which is
        // what `scriptedGuardChain_` asks: the reference's guard controllers stand
        // next to this code in the character's own common state file, and two
        // packs deliberately rewrite them (see `scanGuardChain`). `140`'s exit is
        // the exception, and the file says so itself.
        case CommonState::kGuardStart: {
            if (scriptedGuardChain_) return true;
            if (!back || !inGuardDist_) {
                changeState(CommonState::kGuardEnd, false);
                return true;
            }
            // The posture, for a 120 that did not set it itself. `Hi to Lo` and
            // `Lo to Hi` are two `StateTypeSet`s rather than a ChangeState, and
            // that is deliberate in the file: a ChangeState would restart the
            // guard-start animation from frame 0 every time the stick moved.
            setGuardPosture(down);
            if (animationFinished()) changeState(guardedStateForPosture(), false);
            return true;
        }

        case CommonState::kGuardStand:
        case CommonState::kGuardCrouch: {
            if (scriptedGuardChain_) return true;
            // Releasing back -- or the attack that was being blocked ending --
            // leaves through 140, the guard-*end* pose, which is the one guard
            // state with `ctrl = 1`: the fighter gets its control back on the
            // first tick of the release while the arms come down behind
            // whatever it does next. Going straight to the idle state instead
            // is what this used to do, and it skipped the whole pose.
            if (!back || !inGuardDist_) {
                changeState(CommonState::kGuardEnd, false);
                return true;
            }
            // The stand/crouch swap. Both directions ask for the *command* --
            // `command = "holddown"` and `command != "holddown"` -- and `down`
            // above is that command as the character defines it, falling back to
            // the raw direction for a .cmd that declares no `holddown` at all
            // (see `directionHeld`). Which matters: one pack in the corpus rebinds
            // `holddown` to a button, and testing the stick instead would swap its
            // guard on the wrong input. A fighter crouch-guarding is a crouching
            // fighter as far as a hit is concerned -- its height is the crouch's --
            // which is the point of the swap.
            const int held = down ? CommonState::kGuardCrouch : CommonState::kGuardStand;
            if (held != stateNumber_) changeState(held, false);
            return true;
        }

        case CommonState::kGuardAir:
            if (scriptedGuardChain_) return true;
            // In the air, only the release is the engine's: 132's own landing is
            // its `sysvar(0)` pair, which sends the blocker to 130 while it is
            // still holding back in guard distance and to 52 when it is not --
            // that is `land()`'s, beside the same choice for the air knock-back
            // 155, and a character with a file does it in its own state.
            if (!back || !inGuardDist_) changeState(CommonState::kGuardEnd, false);
            return true;

        case CommonState::kGuardEnd: {
            // `[State 140, Hi to Lo]` / `[State 140, Lo to Hi]`: the release
            // keeps changing posture with the stick, in place.
            setGuardPosture(down);
            // The file's commented-out exit, which it leaves to the engine:
            // `value = (statetype = C)*11 + (statetype = A)*51` -- back to the
            // crouch, back to the air, or standing. Control is already back (140
            // is `ctrl = 1`), so this only fires for a fighter that has done
            // nothing with it, and the pose plays out behind whatever it does.
            if (!animationFinished()) return true;
            changeState(stateType_ == StateType::Crouch ? CommonState::kCrouch
                          : stateType_ == StateType::Air  ? CommonState::kAirDown
                                                          : CommonState::kStand,
                        false);
            return true;
        }

        case CommonState::kCrouch:
            // Letting go of down stands back up, and holding up jumps out of
            // the crouch -- which is what a down-then-up motion means.
            if (up) {
                changeState(CommonState::kJumpStart, false);
                return true;
            }
            if (!down) changeState(CommonState::kCrouchToStand, false);
            return true;

        case CommonState::kWalk: {
            // The walk speed comes from [Velocity] every tick, which is what the
            // character's own walk state does as well. Without it here a
            // character with no common file would not move at all. Only while the
            // direction is actually held, matching common1.cns's own state 20:
            // writing a velocity every tick regardless would overrule a character
            // that has its own idea of how fast it walks.
            //
            // Which way is being walked is read from the input and not from the
            // state, because there is one walking state for both directions --
            // the reference's 20, whose own controllers choose the animation by
            // the sign of the velocity it ends up with. Where both directions
            // are held the forward one wins, which is how the entry above reads
            // them too.
            const bool forward = fwd || !back;
            if (character_ != nullptr && (forward ? fwd : back)) {
                const float speed = forward ? character_->walkFwd() : -character_->walkBack();
                velX_ = worldX(speed);
            }
            // A fighter whose own file defines no state 20 gets its walk art from
            // the table, and the table carries one number for it; walking back is
            // the same art under animation 21, which is the number the reference
            // picks it with. A character that *does* define 20 chose for itself
            // and is left alone -- its own `ChangeAnim` reads the velocity this
            // has just written and settles on the next tick, exactly as it does
            // in MUGEN.
            const StateDef* ownWalk =
                stateLibrary() != nullptr ? stateLibrary()->find(CommonState::kWalk) : nullptr;
            if (ownWalk == nullptr || !ownWalk->defined) {
                const int wanted =
                    forward ? CommonState::kWalk : CommonState::kWalkBackAnim;
                if (animNumber() != wanted) setAnim(wanted, false);
            }
            if (up && ctrl_) {
                changeState(CommonState::kJumpStart, false);
                return true;
            }
            if (down) {
                changeState(CommonState::kStandToCrouch, false);
                return true;
            }
            // Releasing the direction stops the walk, which is the other half of
            // what makes walking feel like walking.
            if (forward && !fwd) changeState(CommonState::kStand, false);
            if (!forward && !back) changeState(CommonState::kStand, false);
            return true;
        }

        default:
            return false;
    }
}

void StateMachine::runBuiltinState() {
    if (runBuiltinMovement()) return;

    switch (stateNumber_) {
        case CommonState::kStandToCrouch:
            if (animationFinished()) changeState(CommonState::kCrouch, false);
            return;

        case CommonState::kCrouchToStand:
            if (animationFinished()) changeState(CommonState::kStand, false);
            return;

        case CommonState::kJumpStart: {
            // Which way the jump goes is decided at the end of the takeoff
            // animation, from what is held then -- so a stick rolled mid-takeoff
            // still lands in the direction it ends up.
            if (!animationFinished()) return;
            if (character_ == nullptr) {
                changeState(CommonState::kAir, false);
                return;
            }
            const bool fwd = directionHeld(kCmdFwd, CmdKey::kForward);
            const bool back = directionHeld(kCmdBack, CmdKey::kBack);
            const float jumpX =
                fwd ? velocityConstant("velocity.jump.fwd.x", 0.0f)
                    : back ? velocityConstant("velocity.jump.back.x", 0.0f)
                           : velocityConstant("velocity.jump.neu.x", 0.0f);
            // MUGEN writes the jump's y-velocity negative, because up is negative
            // in a character's frame; it is occasionally given per direction, and
            // the neutral one is the default. `jumpUp()` is that same number kept
            // as the positive height it describes, so both need the flip into this
            // engine's frame.
            const float jumpY = velocityConstant(
                "velocity.jump.y", velocityConstant("velocity.jump.neu.y", -character_->jumpUp()));
            velX_ = worldX(jumpX);
            velY_ = -jumpY;
            changeState(CommonState::kAir, false);
            return;
        }

        // `AirJump Start`, `common1.cns`'s 45: the takeoff itself has no
        // velocity for two ticks (`[State 45, 6] ChangeState / trigger1 =
        // Time = 2`) and only then does the file read the `velocity.airjump.*`
        // pair and hand over to 50 with `ctrl = 1` (`[State 45, 7]`). 50 is the
        // ordinary air state, so the takeoff is over as soon as it has begun --
        // there is no separate "air jump" pose to wait on.
        case CommonState::kAirJumpStart: {
            if (stateTime_ < 2) return;
            if (character_ == nullptr) {
                changeState(CommonState::kAir, false);
                return;
            }
            const bool fwd = directionHeld(kCmdFwd, CmdKey::kForward);
            const bool back = directionHeld(kCmdBack, CmdKey::kBack);
            const float jumpX =
                fwd ? velocityConstant("velocity.airjump.fwd.x", 0.0f)
                    : back ? velocityConstant("velocity.airjump.back.x", 0.0f)
                           : velocityConstant("velocity.airjump.neu.x", 0.0f);
            // Same spelling question as the ground jump: MUGEN's y is negative
            // upward, this engine's is positive upward.
            const float jumpY = velocityConstant(
                "velocity.airjump.y",
                velocityConstant("velocity.airjump.neu.y", -character_->jumpUp()));
            velX_ = worldX(jumpX);
            velY_ = -jumpY;
            changeState(CommonState::kAir, false);
            return;
        }

        case CommonState::kAir:
            // `[State 50, 3] ChangeAnim / trigger1 = Vel y > Const720p(-8) /
            // trigger1 = SelfAnimExist(anim + 3) / trigger1 = anim = [41,43] /
            // persistent = 0 / value = Anim + 3` -- the falling pose replaces
            // the rising one once the jump has nearly spent its climb. MUGEN's
            // `Vel y` is negative-up, so `> -8` at 720p is `< 2` in the
            // engine's 240p numbers and upward-positive velocity. The entry
            // tick chose 41/42/43 from the horizontal velocity; a fighter that
            // kept its takeoff art has nothing to re-dress.
            if (character_ != nullptr && anim_ >= 41 && anim_ <= 43 &&
                velY_ < 8.0f * kScreenWidth / kWidth720p &&
                character_->animation(anim_ + 3) != nullptr) {
                setAnim(anim_ + 3, true);
            }
            // Gravity and the landing are physics' job; there is nothing else
            // for this state to do but wait to touch the ground.
            return;

        case CommonState::kLand:
            // `[State 52, 4] CtrlSet value = 1 / trigger1 = Time = 3`: the
            // landing pose gives control back after three ticks, not when its
            // animation runs out. (`[State 52, 5] ChangeState value = 0 /
            // trigger1 = AnimTime = 0 / ctrl = 1` is the hand-over below.)
            if (!ctrl_ && stateTime_ >= 3) ctrl_ = true;
            if (animationFinished()) changeState(CommonState::kStand, false);
            return;

        // `Hop backwards (land)`: 105's landing half. The reference's
        // `[State 105, 3] ChangeState / trigger1 = AnimTime = 0 / value = 106`
        // enters it and `[State 106, 5] ChangeState / trigger1 = Time = 7`
        // leaves it, so the backward hop always spends seven ticks on its
        // landing pose -- it is a fixed hold, not the 47 animation's length.
        case CommonState::kHopLand:
            if (stateTime_ >= 7) changeState(CommonState::kStand, false);
            return;

        // The intro pair. Both reference states hand over on `Time = 0` and
        // have no other controller: `[State 190, 1] ChangeState / value = 191`
        // and `[State 191, 1] ChangeState / value = 0`. 191 is the character's
        // override point, so a file that defines it gets its own intro there;
        // with no file the two states cost one tick each.
        case CommonState::kIntro:
            changeState(CommonState::kIntroPoint, false);
            return;

        case CommonState::kIntroPoint:
            changeState(CommonState::kStand, false);
            return;

        // `[Statedef 5900]`, round initialization. Both exits are `trigger1 =
        // 1` work on the entry tick: `[State 5900, 4] ChangeState / trigger1 =
        // roundno = 1 / value = 190` and `[State 5900, 5] ChangeState /
        // trigger1 = 1 / value = 0`. The two `VarRangeSet` controllers above
        // them answer `roundsexisted = 0`, which is "the first round of the
        // match"; `setup()` does that clearing, since the variables are engine
        // state and `setup()` is where a round begins.
        case CommonState::kRoundInit:
            changeState(roundNo_ == 1 ? CommonState::kIntro : CommonState::kStand, false);
            return;

        // The reactions the reference splits on a number rather than on time,
        // and the splits are all the same one: whether the hit put the victim
        // into a *fall*. 5030 is "knocked away" and passes through its
        // transition state 5035, which ends in the fall itself (5050) or in the
        // air-recovery that lands on its feet (5040); 5080 is a victim hit while
        // already down, which goes back into the air as 5030 when the hit wrote
        // an upward `down.velocity` and slides along the floor as 5081 when it
        // did not. `hitFallFlag_` is where `applyHit` wrote the answer, and it
        // is the same flag the `hitfall` trigger reads.
        case CommonState::kHitAirFall:
            // `[State 5030, 6] ChangeState / trigger1 = AnimTime = 0 / value =
            // 5035` is the reference's earliest exit, and in this engine the
            // animation clock is exactly the `AnimTime` it reads. 5035 in turn
            // does the split 5030 used to do here, and does it on the same tick
            // the reference would have: the transition state's own exits fire
            // under `HitOver`, `AnimTime = 0`, ground contact, or `Time = 0`
            // when there is no transition art.
            changeState(CommonState::kHitAirTransition, false);
            return;

        // `Air get-hit (transition)`. The file gives it no pose of its own
        // beyond an optional 5035 animation, and its exit is the same
        // `HitFall` question 5030's used to answer: `[State 5035, 3]` goes to
        // 5040 when `!HitFall` and `[State 5035, 4]` to 5050 when `HitFall`.
        // By the time the stun gate lets this state run, `HitOver` has passed,
        // so the split is unconditional here.
        case CommonState::kHitAirTransition:
            changeState(hitFallFlag_ ? CommonState::kHitFall : CommonState::kHitAirRecover,
                        false);
            return;

        // `Air get-hit (recovering in air, not falling)`. Its `HitOver` work is
        // not an exit -- `land()` owns the floor -- but two property changes:
        // `[State 5040, 3] CtrlSet value = 1` and `[4] StateTypeSet movetype =
        // I`. The stun gate only lets the built-in run once the stun is spent,
        // which is the tick those triggers fire on; the pose itself keeps
        // playing until the fighter lands.
        case 5040:
            ctrl_ = true;
            moveType_ = MoveType::Idle;
            return;

        case CommonState::kHitLiedown:
            if (animationFinished()) {
                changeState(hitFallFlag_ ? CommonState::kHitAirFall : CommonState::kHitLiedownKb,
                            false);
            }
            return;

        // `Downed get-hit (getting up)`. The file makes the whole get-up
        // untouchable (`[State 5120, 3] NotHitBy value = SCA / trigger1 = 1`)
        // and then, on the tick the animation ends, trades that for two windows
        // written together: twelve ticks in which throws cannot reach the
        // fighter (`value = , NT,ST,HT`) and three in which nothing can
        // (`value2 = SCA`). `HitFallSet value = 1` is the same tick, and the
        // hand-over is `ChangeState 0 / ctrl = 1`.
        case CommonState::kDownedGetUp:
            setNotHitTicks(1);
            if (animationFinished()) {
                setThrowNotHitTicks(12);
                setNotHitTicks(3);
                hitFallFlag_ = true;
                // State 0's row is controllable, so this is the `ctrl = 1` the
                // hand-over writes; `keepCtrl` would restore 5120's own "no".
                changeState(CommonState::kStand, false);
            }
            return;

        // The fall's own way out: recovery, pressed while falling. `break` rather
        // than `return`, because a fall that found no recovery still has the
        // hand-over below waiting for the ground.
        case CommonState::kHitFall:
            if (tryFallRecover()) return;
            break;

        // The mid-air recovery's whole content, instruction for instruction out of
        // the reference's 5210. Four ticks with the position frozen
        // (`[State 5210, PosFreeze]` `Time < 4`), then the lift written by the
        // airrecover pair at `Time = 4`: `VelMul x/y = Const(velocity.air.gethit.
        // airrecover.mul)` -- its defaults are .5 and .2 -- and
        // `VelAdd x/y = Const(velocity.air.gethit.airrecover.add)`, whose y is
        // -4.5 in the character's frame, so the lift is 4.5 up in this one. The
        // stick steers the same tick, off the four airrecover directions, and the
        // gravity that takes the fighter down again is
        // `movement.air.gethit.airrecover.yaccel` -- .35, deliberately much
        // weaker than the fighter's own gravity, which is where the roll's whole
        // feel lives. (The null-commented 2002 controllers in the file -- the
        // `.8` mul, the -4.5 write and the three conditional nudges on `Vel Y` --
        // are not read by the state that runs them, and neither are they here.)
        case CommonState::kHitAirFallRecover: {
            const int roll = stateTime_;
            if (roll < 4) {
                posFreezeThisTick_ = true;
                return;
            }
            if (roll == 4) {
                velX_ *= velocityConstant("velocity.air.gethit.airrecover.mul.x", 0.5f);
                velY_ *= velocityConstant("velocity.air.gethit.airrecover.mul.y", 0.2f);
                // The add is written in the character's frame, where up is
                // negative, so its default -4.5 is the +4.5 lift this engine's
                // upward-positive velocity spells.
                velX_ += velocityConstant("velocity.air.gethit.airrecover.add.x", 0.0f);
                velY_ -= velocityConstant("velocity.air.gethit.airrecover.add.y", -4.5f);
                // The steer, read the same tick: up and down are their own
                // keys; forward and back are relative to facing.
                if (directionHeld(kCmdUp, CmdKey::kUp)) {
                    velY_ -= velocityConstant("velocity.air.gethit.airrecover.up", -2.0f);
                }
                if (directionHeld(kCmdDown, CmdKey::kDown)) {
                    velY_ -= velocityConstant("velocity.air.gethit.airrecover.down", 1.5f);
                }
                if (directionHeld(kCmdFwd, CmdKey::kForward)) {
                    velX_ += worldX(velocityConstant("velocity.air.gethit.airrecover.fwd", 0.0f));
                }
                if (directionHeld(kCmdBack, CmdKey::kBack)) {
                    velX_ += worldX(velocityConstant("velocity.air.gethit.airrecover.back", -1.0f));
                }
            }
            velY_ -= velocityConstant("movement.air.gethit.airrecover.yaccel", 0.35f);
            if (roll >= 20 && !ctrl_) {
                // `[State 5210, 4] CtrlSet value = 1` at `Time = 20`: the kill
                // clock, control back long after the lift and its steering.
                ctrl_ = true;
            }
            // The floor, put back by hand: 5210 is `physics = N`, which clamps
            // nothing -- the reference's own "Land on ground" is
            // `Vel Y > 0 && Pos Y >= 0`, a test rather than a clamp, and its
            // `ChangeState 52` carries `ctrl = 1`, which is control back the
            // tick the fighter touches the ground.
            if (y_ <= 0.0f && velY_ <= 0.0f) {
                y_ = 0.0f;
                velY_ = 0.0f;
                changeState(CommonState::kLand, true);
                ctrl_ = true;
                return;
            }
            break;
        }

        default:
            break;
    }

    // Every other built-in state does one thing: hand over once its animation
    // has run out. That is what returns a fighter from a hit state to its feet,
    // and what carries a knock-out through its fall to the floor -- except for
    // the falls, which hand over when the fighter is actually down, and down
    // means stopped: the landing tick is the tick a bounce starts on, and a
    // fighter rising off the floor has not finished falling. See
    // `builtinAwaitsGround`.
    //
    // Lying down is the one state that is neither. The reference's own
    // common1.cns defines 5110 and gives it no hand-over at all -- its only
    // ChangeState is the `!alive` one to 5150 -- because the time spent on the
    // floor is `[Data] liedown.time`, which the *engine* counts down and acts
    // on. Driving it from the animation instead is wrong in both directions: a
    // character that ships no art for the pose is "finished" on the frame it
    // enters one and would get up instantly, and one whose lying animation is
    // longer than its liedown time would stay down for the art rather than for
    // the number. So this state waits on the clock, not on `animationFinished()`.
    //
    // The one exit the file *does* write is the dead one, and it is checked
    // first: a fighter that died on the way down never gets up, it lies there
    // as 5150 until the round is over. The clock is re-armed as it hands over,
    // which is what the reference does on that path -- the countdown is a
    // per-knockdown thing and the next one starts from the full number.
    const int number = stateNumber_;
    if (number == CommonState::kDowned) {
        // `[State 5110, 11] ChangeState / triggerall = !alive / trigger1 =
        // AnimTime = 0 / trigger2 = SysVar(0) / trigger3 = Anim = [5110,5119]`.
        // `SysVar(0)` is 5081's "came from the knock-back, skip the ground-hit
        // pose" flag; the third trigger is the ground pose itself. A death in
        // any other pose -- the 5170 ground-hit animation a fall sets -- waits
        // for that pose to finish before the body is marked dead.
        const bool viaKnockback = !sysVars_.empty() && sysVars_[0] != 0;
        const bool groundPose = anim_ >= 5110 && anim_ <= 5119;
        if (life_ <= 0 && (viaKnockback || groundPose || animationFinished())) {
            changeState(CommonState::kDead, false);
            return;
        }
        // `[State 5110, 12] VarSet sysvar(0) = 0 / trigger1 = SysVar(0) /
        // trigger1 = Time = 0`: the flag is spent on the first tick.
        if (viaKnockback && stateTime_ == 0) sysVars_[0] = 0;
        // `[State 5110, 13] VelMul x = .85` and `[14] VelSet x = 0` below the
        // friction threshold. The state is `physics = N`, so the engine applies
        // none of its own.
        velX_ *= kStandFriction;
        if (std::fabs(velX_) < kFrictionThreshold) velX_ = 0.0f;
        if (life_ > 0 && liedownTicks_ <= 0) {
            liedownTicks_ = character_ != nullptr ? character_->liedownTime() : 0;
            changeState(CommonState::kDownedGetUp, false);
            return;
        }
    }

    const BuiltinState* builtin = builtinFor(stateNumber_);
    bool holdOver = false;
    if (builtin != nullptr) {
        // What ends the hold. For almost every one of these states that is the pose's
        // animation running out; for the shakes it is the hit's own shake clock, and
        // the reason is `builtinExitsOnShake` above. The three knock-backs end on
        // `HitOver` unconditionally -- see `builtinExitsOnHitOver`.
        holdOver = builtinExitsOnShake(builtin->number)
                       ? shakeTicks_ == 0
                       : builtinExitsOnHitOver(builtin->number) ? true : animationFinished();
    }
    if (builtin != nullptr && builtin->thenState >= 0 && number != CommonState::kDowned && holdOver &&
        (!builtinAwaitsGround(builtin->number) || (y_ <= 0.0f && velY_ <= 0.0f))) {
        int next = builtin->thenState;
        // A guard shake is the one hand-over that carries two things with it: the
        // push the block owes, and which of the two grounded knock-backs it ends
        // in.
        //
        // The push, because the shake just ended is written `velset = 0,0` -- the
        // blocker was held still for it -- and 151, 153 and 155 each open with
        // `HitVelSet x = 1` (155 with `y = 1` as well), which is "read the guard
        // velocity back out of the hit that caused this". `applyHit` kept that
        // velocity in `lastHit_` for exactly this tick; a character that ships its
        // own guard states gets it from the same controller in its own file.
        //
        // And the height, from the input rather than from the posture the shake was
        // entered in: the reference writes the *same* `ChangeState trigger1 =
        // HitShakeOver value = 151 + 2*(command = "holddown")` in 150 and 152
        // alike, so a stand-blocker that presses down during the shake is knocked
        // back crouching and a crouch-blocker that lets go is knocked back
        // standing. The posture the shake was entered with has already done its
        // job -- it chose the shake.
        if (builtin->number == CommonState::kGuarded || builtin->number == CommonState::kGuardHit ||
            builtin->number == CommonState::kGuardedAir) {
            if (builtin->number != CommonState::kGuardedAir) {
                next = directionHeld(kCmdDown, CmdKey::kDown) ? CommonState::kGuardHitKb
                                                             : CommonState::kGuardedKb;
            }
            velX_ = lastHit_.pushX;
            if (builtin->number == CommonState::kGuardedAir) velY_ = lastHit_.pushY;
        }
        // A ground reaction that was a *launch* hands over to 5030 instead of to the
        // knock-back, which is the reference's own pair of controllers again: in
        // 5000 and in 5010 alike the flat exit is written first and asked for by
        // name (`GetHitVar(yvel) = 0 && !GetHitVar(fall)`) and the one that follows
        // it, `value = 5030`, is what everything else takes. `hitFallFlag_` is that
        // question answered once, at the hit, so the row's `thenState` above is the
        // flat case and this is the other one.
        //
        // The velocity does not have to be re-applied here the way the guard's does.
        // A guard block zeroes it -- `velset = 0,0` in the reference and in
        // `applyHit`, with 151/153/155 reading it back with `HitVelSet` -- while a
        // get-hit never does, and a character that ships its own 5000 zeroes it from
        // that state's own header and re-applies it from 5030's. Either way the
        // victim's `pushX`/`pushY` are still the hit's when 5030 is entered, which
        // is the same number its `HitVelSet x = 1, y = 1` would write.
        if ((builtin->number == CommonState::kHitHigh || builtin->number == CommonState::kHitLow) &&
            hitFallFlag_) {
            next = CommonState::kHitAirFall;
        }
        // The `HitOver` block the knob-backs share, all of it written beside
        // their ChangeState: `[State 5001, 4] VelSet x = 0` and `[5]
        // DefenceMulSet value = 1` (likewise 5011), and `[State 5081, 2]
        // VelSet x = 0`. The defence multiplier belongs to the *victim* -- it
        // scales the damage it takes -- and the file puts it back to 1 when the
        // reaction ends, so a hit that lowered it does not linger.
        if (builtin->number == CommonState::kHitHighKb ||
            builtin->number == CommonState::kHitLowKb) {
            velX_ = 0.0f;
            defenceMul_ = 1.0f;
        }
        if (builtin->number == CommonState::kHitLiedownKb) {
            velX_ = 0.0f;
        }
        changeState(next, false);
    }
}

namespace {

// -------------------------------------------------------------- input symbols

// The inverse of cmd.cpp's symbolsFromHeld: a command's symbols back into this
// engine's button masks, resolving back and forward against facing. The AI
// needs it to *play* a command rather than match one.
uint32_t heldFromSymbols(uint32_t symbols, int facing) {
    uint32_t held = 0;
    if ((symbols & CmdKey::kUp) != 0) held |= Button::kUp;
    if ((symbols & CmdKey::kDown) != 0) held |= Button::kDown;
    const bool backIsLeft = facing >= 0;
    if ((symbols & CmdKey::kBack) != 0) held |= backIsLeft ? Button::kLeft : Button::kRight;
    if ((symbols & CmdKey::kForward) != 0) held |= backIsLeft ? Button::kRight : Button::kLeft;

    const char letters[6] = {'x', 'y', 'z', 'a', 'b', 'c'};
    const uint32_t buttons[6] = {Button::kX, Button::kY, Button::kZ,
                                 Button::kA, Button::kB, Button::kC};
    for (int i = 0; i < 6; ++i) {
        if ((symbols & CmdKey::button(letters[i])) != 0) held |= buttons[i];
    }
    return held;
}

}  // namespace

// --------------------------------------------------------------------- setup

void StateMachine::setup(const Character* character, int facing) {
    character_ = character;
    facing_ = facing >= 0 ? 1 : -1;

    // The library is read from the character once and pointed at rather than
    // copied: a character's state list runs to tens of thousands of controllers
    // and two fighters share the same one.
    states_ = character != nullptr ? &character->states() : nullptr;
    commands_ = character != nullptr ? &character->commands() : nullptr;
    // Which landing effects this character's file runs itself, read from the
    // library now that it is pointed at. See `land()` for why it matters.
    scanLanding();
    scanGuardChain();

    maxLife_ = character != nullptr ? std::max(1, character->life()) : 1000;
    life_ = maxLife_;
    power_ = 0;
    yAccel_ = character != nullptr ? character->gravity() : 0.44f;
    tookDamage_ = false;

    // A fresh round is a fresh costume: the palette selection comes from the
    // character's .def and the grade is off, matching MUGEN's round reset.
    palFx_ = PalFxState{};
    palNo_ = character != nullptr ? character->defaultPalNo() : 1;
    palRemaps_.clear();

    velX_ = 0.0f;
    velY_ = 0.0f;
    offsetX_ = 0.0f;
    offsetY_ = 0.0f;

    // A fresh setup is a fresh root: the family below was built by a match that
    // is over, and the fight clears the helpers themselves alongside this. The
    // one field that survives a round reset is playerId_, which the host assigns
    // once per match rather than once per round.
    parent_ = nullptr;
    root_ = this;
    isHelper_ = false;
    helperId_ = 0;
    helperName_.clear();
    keyCtrl_ = false;
    pauseMoveTime_ = 0;
    superMoveTime_ = 0;
    destroyed_ = false;
    helpers_.clear();
    lastTarget_ = nullptr;
    // A new round erases what has hit this fighter, which is what stops a chain
    // link from a previous round opening the combo in this one.
    lastHitById_ = -1;
    hitsTakenInCombo_ = 0;
    framesSinceHitReaction_ = 0;
    bindMachine_ = nullptr;
    bindTime_ = 0;

    // `[State 5900, 1]`/`[2]`: `VarRangeSet value = 0 / fvalue = 0` guarded by
    // `roundsexisted = 0`, which is the *first* round of the match. Character
    // variables are match-long state -- a `var` counting rounds, a stored
    // palette -- and the reference deliberately keeps them across rounds. The
    // engine cleared them every round, which silently undid that.
    //
    // `sysvar` is different: it is the scratch space the common states use for
    // themselves (5080's "skip the ground pose", 5110's dead-exit flag), not a
    // place a character is meant to keep anything across a round, so it is
    // always reset.
    if (roundNo_ == 1) {
        vars_.fill(0);
        fVars_.fill(0.0f);
    }
    sysVars_.fill(0);
    sysFVars_.fill(0.0f);
    hitDef_ = HitDef{};
    lastUsedHitDef_ = HitDef{};
    lastHit_ = HitInfo{};
    hitCount_ = 0;
    uniqHitCount_ = 0;
    // How much air-combo allowance is left, which a hit taken in the air spends
    // and a landing restores. Zero for a machine with no character behind it.
    jugglePoints_ = character != nullptr ? character->airJuggle() : 0;
    // The liedown clock starts armed, which is the state a fighter on its feet is
    // meant to report: `GetHitVar(recovertime)` read from somebody who is not
    // lying down is the file's full liedown time, and the tick below keeps it
    // there for as long as the fighter is not in a hit reaction.
    liedownTicks_ = character != nullptr ? character->liedownTime() : 0;
    // A fresh round starts with a whole guard, as it does with a whole life bar.
    guardEnergy_ = kGuardEnergyMax;
    guardRestTicks_ = 0;
    moveContact_ = 0;
    moveHit_ = 0;
    // `-1` is "not stunned", not 0: the clock counts a hit's `hittime` down
    // through 0 to -1 and stops, so that `HitOver` (`hittime < 0`) turns true one
    // tick after the number reaches zero. See `stunTicks_`.
    stunTicks_ = -1;
    stateChangesThisTick_ = 0;
    pendingStateEntry_ = false;
    buffer_.clear();
    lastInput_ = InputState{};
    aiScript_.clear();
    aiScriptStep_ = 0;
    aiScriptTicks_ = 0;

    // `[Statedef 5900]` -- the reference's round initialization. Its two
    // `VarRangeSet` controllers are what the `vars_.fill(0)` above stands in
    // for, and both of its exits land in state 0. The intro pair 190/191 is
    // played by the reference's own round-announcement flow, which this engine
    // models as screen phases that hold the fighters rather than as states; so
    // the stand-in for "the round has been initialized" is entering 0 directly.
    // `prevstateno` still reports the round-init number on the first tick, as
    // it does in the reference the moment state 0 is reached.
    prevStateNumber_ = CommonState::kRoundInit;
    enterState(CommonState::kStand);
}

const std::string& StateMachine::name() const {
    static const std::string kPlaceholder = "FIGHTER";
    return character_ != nullptr ? character_->displayName() : kPlaceholder;
}

int StateMachine::nextVictoryQuoteRoll() const {
    // `randomSeed_` is mutable for exactly this reason: the `random` trigger is
    // asked from a const context and still advances the stream. The victory
    // screen draws from the same stream so its choice is not a second,
    // unrelated sequence that repeats whenever the fight does.
    return nextRandom(randomSeed_);
}

void StateMachine::setAiLevel(int level) { aiLevel_ = std::clamp(level, 0, 9); }

// ---------------------------------------------------------------- the family

void StateMachine::becomeHelper(StateMachine* parent, int helperId, std::string name,
                                bool keyCtrl, int pauseMoveTime, int superMoveTime,
                                int stateNo) {
    // Called once, straight after setup() made this a fresh root. The helper's
    // family anchor is the spawner's own root, which in a chain of helpers
    // (a helper spawning a helper) is still the real fighter at the top.
    parent_ = parent;
    root_ = parent != nullptr ? parent->root_ : this;
    isHelper_ = parent != nullptr;
    helperId_ = helperId;
    helperName_ = std::move(name);
    keyCtrl_ = keyCtrl;
    pauseMoveTime_ = pauseMoveTime;
    superMoveTime_ = superMoveTime;
    if (root_ != this) root_->helpers_.push_back(this);

    // The state the controller named, in place of the state 0 setup() entered.
    // Helpers start uncontrollable whatever the header says: control is the
    // spawner's to hand over, through keyctrl, not the state's.
    ctrl_ = false;
    prevStateNumber_ = stateNo;
    enterState(stateNo);
}

void StateMachine::removeFamilyHelper(StateMachine* machine) {
    std::vector<StateMachine*>& list = root_->helpers_;
    list.erase(std::remove(list.begin(), list.end(), machine), list.end());
}

void StateMachine::clearReferencesTo(StateMachine* machine) {
    if (lastTarget_ == machine) lastTarget_ = nullptr;
    if (bindMachine_ == machine) {
        bindMachine_ = nullptr;
        bindTime_ = 0;
    }
    if (parent_ == machine) parent_ = root_;
}

bool StateMachine::doomed() const {
    // A helper whose spawner chain is gone goes with it: a flash double whose
    // helper-parent was destroyed has nothing left to be the double of.
    return destroyed_ || (parent_ != nullptr && parent_->doomed());
}

int StateMachine::countHelpers(int id) const {
    int count = 0;
    for (const StateMachine* machine : root_->helpers_) {
        if (machine->doomed()) continue;
        if (id >= 0 && machine->helperId_ != id) continue;
        ++count;
    }
    return count;
}

// ---------------------------------------------------------------- state entry

void StateMachine::enterState(int number) {
    stateNumber_ = number;
    stateTime_ = 0;
    offsetX_ = 0.0f;
    offsetY_ = 0.0f;
    // `persistent` counters reset with the state, which is what makes
    // `persistent = 2` in a looping state mean "every other tick of this stay".
    lastRunTick_.clear();
    // A ChangeAnim2 pose survives only until the state it posed for moves on.
    animOverrideChar_ = nullptr;

    const StateDef* state = stateLibrary() != nullptr ? stateLibrary()->find(number) : nullptr;
    // A HitDef belongs to the state that armed it. Carrying one across a state
    // change is what would let a single attack keep hitting after the
    // animation that justified it had ended. The same goes for a ReversalDef.
    // The statedef's own `*persist = 1` flags are MUGEN's written exceptions
    // (and absent flags default to resetting -- the binary's statedef record
    // pays this out at entry).
    if (state == nullptr || !state->hitDefPersist) hitDef_.armed = false;
    // The player's live guard distance is seeded here rather than when a HitDef
    // arrives, and it is seeded from the player's own `[Size] attack.dist`. That is the
    // reference's arithmetic and the reason its documentation can say the parameter
    // "defaults to the value in the player variables": the value is put in place at
    // state entry, and a HitDef that omits it simply leaves it there. A state that
    // carries `hitdefpersist` keeps the value it had, exactly as it keeps the HitDef.
    //
    // The consequence is worth stating plainly, because it is not obvious from the
    // trigger's name: a state in `movetype = A` with *no* HitDef armed -- a super's
    // wind-up, the frames after an attack's box has gone -- still answers
    // `inguarddist` for an opponent standing in front of it. That is the point of the
    // number. It is not a reach; it is the distance at which holding back counts.
    if (state == nullptr || !state->hitDefPersist) {
        guardDist_ = static_cast<int>(std::lround(
            character_ != nullptr
                ? character_->constant("size.attack.dist", static_cast<float>(kDefaultAttackDist))
                : static_cast<float>(kDefaultAttackDist)));
    }
    reversalDef_.armed = false;
    if (state == nullptr || !state->moveHitPersist) {
        moveContact_ = 0;
        moveHit_ = 0;
        moveReversed_ = 0;
    }
    if (state == nullptr || !state->hitCountPersist) hitCount_ = 0;

    const bool fromFile = state != nullptr && state->defined;

    if (fromFile) {
        // `U` keeps what the fighter already had, which is how a character
        // writes a state that must not disturb the hit state it interrupts.
        if (!state->typeUnchanged) stateType_ = state->type;
        if (!state->moveTypeUnchanged) moveType_ = state->moveType;
        if (!state->physicsUnchanged) physics_ = state->physics;
        // A `[Statedef]` that leaves `ctrl` out does not mean "no control", and
        // the common state files prove it: none of the hit states (5000, 5050,
        // 5150...) writes `ctrl`, and neither does state 0 or the walking and
        // crouching states -- and the hit states have to be uncontrollable while
        // standing has to be controllable. The value MUGEN uses for the states
        // it defines itself is the one that fills the gap, which is exactly what
        // this table is.
        const BuiltinState* builtin = builtinFor(number);
        ctrl_ = state->hasCtrl ? state->ctrl : (builtin != nullptr && builtin->ctrl);
        if (state->sprPriority >= 0) spritePriority_ = state->sprPriority;
        if (state->hasVelSet) {
            // The header's `velset` is in the local frame like every other x a
            // character writes, and its y is upside down like every other y.
            velX_ = worldX(state->velX);
            velY_ = -state->velY;
        }
        if (state->hasPowerAdd) power_ = clampPower(power_ + state->powerAdd);
        // A Statedef's `juggle` is the fighter's remaining air-combo allowance once
        // it is in that state. MUGEN documents a default of 0 for the parameter,
        // which taken literally would mean every ordinary hit state -- which does
        // not write it -- wiped the allowance the moment it was entered, and air
        // combos could never happen at all. Absent therefore means "leave it
        // alone", and the 246 states in this roster that do write it mean exactly
        // what they say.
        if (state->hasJuggle) jugglePoints_ = std::max(0, state->juggle);
        if (state->hasFaceP2 && state->faceP2 && opponent_ != nullptr) {
            facing_ = opponent_->x_ >= x_ ? 1 : -1;
        }
        const int anim = animFromHeader(*state, *this);
        if (anim >= 0) setAnim(anim, true);
        return;
    }

    // The character never wrote a header for this state. MUGEN's own behaviour
    // for the states it defines itself is the right answer, and the generic
    // defaults below are the right answer for a state number it does not
    // recognise. `state` may be null here, or may be a state the character
    // attached controllers to without ever giving it a header.
    //
    // The three guard poses the table has no row for come first, because the table
    // cannot say what they do and a fixed row would say the opposite. Their numbers
    // are the reference's own, and so is the fact that they are written down as
    // controllers rather than as rows:
    //
    //   120 and 140 are `type = U, physics = U` -- the guard start and the guard
    //   *end* are transitions that keep the posture they were entered with, so a
    //   crouching blocker stays crouched through its release instead of standing up
    //   to play it -- and each takes its animation from the posture it is in rather
    //   than from a table: `value = 120 + (statetype = C) + (statetype = A)*2`, and
    //   the same arithmetic for 140. Those poses are 120/121/122 and 140/141/142:
    //   standing, crouching, airborne, in that order. The physics that goes with
    //   that posture is the trio below.
    //
    //   132 is `type = A, physics = N` with its fall written out by hand in its own
    //   controllers: `VelAdd y = Const(movement.yaccel)` every tick, and the
    //   `sysvar(0)` landing test -- `pos y >= 0 && vel y > 0` -- whose two outcomes
    //   are 130 while the blocker is still holding back inside guard distance and 52
    //   otherwise. `Physics::Air` is the engine's substitute for the whole of that
    //   (`land()` makes the same call, 155's), and its posture is not a question: a
    //   fighter in the air guard state is airborne by definition. Falling through to
    //   the generic defaults below instead stood the blocker up and dropped it to
    //   the floor in the one tick, which is a wrong answer to all three of them.
    //
    // 140 also carries the one `ctrl` the reference's own file writes into a guard
    // state (`ctrl = 1` on `[Statedef 140]`), which is what lets a fighter act during
    // the release; 120 keeps the no-control an omitted `ctrl` means, and 132 the same.
    if (number == CommonState::kGuardStart || number == CommonState::kGuardEnd ||
        number == CommonState::kGuardAir) {
        if (number == CommonState::kGuardAir) {
            stateType_ = StateType::Air;
            physics_ = Physics::Air;
        } else {
            physics_ = physicsForStateType(stateType_);
        }
        ctrl_ = number == CommonState::kGuardEnd;
        // The air hold is its own pose rather than a posture of 120's, so it names
        // 132 outright; the other two are the posture arithmetic above.
        const int pose = number == CommonState::kGuardAir
                             ? number
                             : number + (stateType_ == StateType::Crouch ? 1 : 0) +
                                   (stateType_ == StateType::Air ? 2 : 0);
        // Only a character that ships the pose gets it. A fighter with no art for
        // 140 would otherwise be told to draw an animation it does not have for
        // the tick or two before its release hands over, which draws *nothing*
        // where the fighter should be -- the same trap the table's own comment
        // describes for the state 47 row that used to be here. Leaving the
        // animation alone instead keeps whatever pose it was already in.
        if (animation(pose) != nullptr) setAnim(pose, true);
        return;
    }
    if (const BuiltinState* builtin = builtinFor(number)) {
        stateType_ = builtin->type;
        moveType_ = builtin->moveType;
        physics_ = builtin->physics;
        ctrl_ = builtin->ctrl;
        // The recovery's own entry state, out of the reference's controllers,
        // for a character that has no file to carry them. 5201 is the hop the
        // fall's recovery ends in: it turns to face the opponent, drops the
        // 10-odd units of floor 5200 sank through (`PosSet y = 0`), and leaves
        // the ground again at 3.5, slightly backwards -- which is what makes the
        // recovery read as a roll back onto the feet rather than as a teleport
        // to standing.
        //
        // The blink and the dust the reference writes beside these
        // (`PalFX add = 128,128,128` and `GameMakeAnim 60`) are not supplied
        // here: they are display, and the engine has neither a per-fighter
        // palette-add it can raise without a controller to read one from nor an
        // engine-driven `MakeAnim` yet.
        if (number == CommonState::kHitFallRecoverGround) {
            // `Turn` with `trigger1 = p2dist X < -5`: the fighter turns only when
            // the opponent is *behind* it by more than five units, which is what
            // makes the roll end up facing the fight rather than away from it.
            if (opponent_ != nullptr &&
                (opponent_->x_ - x_) * static_cast<float>(facing_) < -5.0f) {
                facing_ = -facing_;
            }
            y_ = 0.0f;
            velY_ = 3.5f;
            velX_ = worldX(-0.15f);
        }
        if (number == CommonState::kHitAirFallRecover) {
            // `[State 5210, 3] NotHitBy SCA` for 15 ticks covers the whole
            // roll, and `[State 5210, PosFreeze]` starts holding on the entry
            // tick itself -- the entry tick has already run the reset of the
            // freeze flag above, so it is asserted here for the physics step
            // this very tick will take before the state gets a case of its own.
            setNotHitTicks(15);
            posFreezeThisTick_ = true;
        }
        // `[State 5000, 5]` and `[State 5010, 5]`: a ground reaction that is
        // really a launch becomes *airborne* for the shake it spends before the
        // flight. The reference writes `StateTypeSet statetype = a` at `Time =
        // 0` whenever the hit carried a y-velocity or the fall flag (or the
        // victim was already off the floor), which is what puts its hurt box at
        // the airborne height for the rest of the hitstun. The row's own type
        // is the flat reaction's; this is the launch's.
        if ((number == CommonState::kHitHigh || number == CommonState::kHitLow) &&
            (lastHit_.pushY != 0.0f || hitFallFlag_)) {
            stateType_ = StateType::Air;
        }
        // `[Statedef 5080]` writes `velset = 0,0` and `[Statedef 5081]` opens
        // with `HitVelSet x = 1`; the zero belongs to 5080 alone. 5081 also
        // sets `sysvar(0) = 1` (`[State 5081, 3]`), the flag 5110 reads to skip
        // its ground-hit pose and which decides whether a *dead* fighter can go
        // straight to 5150.
        if (number == CommonState::kHitLiedown) {
            velX_ = 0.0f;
            velY_ = 0.0f;
        }
        if (number == CommonState::kHitLiedownKb) {
            if (sysVars_.size() > 0) sysVars_[0] = 1;
        }
        // `[State 5120, 2] VelSet x = 0` at `Time = 0`.
        if (number == CommonState::kDownedGetUp) {
            velX_ = 0.0f;
            velY_ = 0.0f;
        }
        // `[Statedef 5150] sprpriority = -3`: the body lies behind the living.
        if (number == CommonState::kDead) {
            spritePriority_ = -3;
        }
        int anim = -1;
        // Whether a get-hit state has already answered "keep the current pose".
        // Their own ChangeAnim controllers are conditional -- `Anim !=
        // [5000,5199]` for 5030, `SelfAnimExist(5035)` for the transition, `Anim
        // != 5035` for the air recovery -- so the row's number is not a safe
        // fallback for them. `getHitSelected` says the selection function was
        // consulted; `anim < 0` says it chose to keep.
        bool getHitSelected = false;
        // `SelfAnimExist` reads the fighter's *own* file. The get-hit chain is
        // never borrowed art; `animCharacter()` is for `ChangeAnim2` poses and
        // `p2stateno` states, and answering this question from there would find
        // the attacker's 5035 and draw it on a victim who has none.
        const auto selfAnimExists = [&](int candidate) {
            return character_ != nullptr && character_->animation(candidate) != nullptr;
        };
        // `common1.cns` builds each get-hit pose out of the hit that produced
        // it. The rows above carry the *flat light* reaction as a fallback; the
        // file's rules are:
        //
        //   5000: (groundtype = 1 ? 5000 : 5010) + animtype
        //   5010: 5020 + animtype
        //   5020: (airtype = 1 ? 5000 : 5010) + animtype
        //   animtype 3 or 5 (back / diagonal-up): 5030, the launch pose
        //   animtype 4 or 5 with `SelfAnimExist(5047 + animtype)`: 5047 + animtype
        //
        // 5030, 5035 and 5040 have conditional re-dresses instead, so they can
        // answer "keep what is playing". A fighter with no file at all is left
        // to the row, which is the only art it could have.
        const auto getHitAnim = [&](int stateNumber, int& out) -> bool {
            if (character_ == nullptr) return false;
            const int animtype = lastHit_.animType;
            switch (stateNumber) {
                case CommonState::kHitHigh:
                case CommonState::kHitLow:
                case CommonState::kHitAir: {
                    if (stateNumber == CommonState::kHitLow) {
                        out = 5020;
                    } else if (stateNumber == CommonState::kHitAir) {
                        out = lastHit_.airType == 1 ? 5000 : 5010;
                    } else {
                        out = lastHit_.groundType == 1 ? 5000 : 5010;
                    }
                    if (animtype == 3 || animtype == 4 || animtype == 5) {
                        out = 5030;
                    }
                    if ((animtype == 4 || animtype == 5) &&
                        selfAnimExists(5047 + animtype)) {
                        out = 5047 + animtype;
                    } else if (animtype >= 0 && animtype <= 2) {
                        out += animtype;
                    }
                    return true;
                }
                case CommonState::kHitAirFall:
                    // `Anim != [5000,5199] && SelfAnimExist(5030)`. The anim
                    // from a 3/5-animtype ground reaction is already 5030, so
                    // this mostly matters when a character entered 5030 from
                    // its own controllers.
                    out = (anim_ < 5000 || anim_ > 5199) && selfAnimExists(5030) ? 5030 : -1;
                    return true;
                case CommonState::kHitAirTransition:
                    // `Time = 0`, `SelfAnimExist(5035)`, not in [5051,5059] and
                    // not 5090 -- the up-launch and off-ground poses own the
                    // transition in the reference's eyes.
                    out = selfAnimExists(5035) && (anim_ < 5051 || anim_ > 5059) && anim_ != 5090
                              ? 5035
                              : -1;
                    return true;
                case 5040:
                    // `(AnimTime = 0 && Anim != 5040) || (Time = 0 && Anim !=
                    // 5035)`; on entry Time is 0, so the transition pose is the
                    // one animation that survives. The reference does not guard
                    // this one with `SelfAnimExist`.
                    out = anim_ == 5035 ? -1 : 5040;
                    return true;
                default:
                    return false;
            }
        };
        // A character that named an animation for a state it otherwise left to
        // the engine keeps it: a fighter whose walk art is not at 20 says so
        // with `anim`, header or not.
        if (state != nullptr && state->hasAnim) {
            anim = animFromHeader(*state, *this);
        } else if (number == 50) {
            // `[State 50, 2] ChangeAnim / value = ifelse((vel x) = 0, 41,
            // ifelse((vel x) > 0, 42, 43))`. `vel x` in a character's file is
            // facing-relative, which is `localVelX()`. A fighter without those
            // poses keeps the row's takeoff art, and one without any file is
            // handled by the `character_ == nullptr` early return below.
            if (character_ != nullptr) {
                const float vx = localVelX();
                const int candidate = vx == 0.0f ? 41 : (vx > 0.0f ? 42 : 43);
                if (selfAnimExists(candidate)) anim = candidate;
            }
        } else if (getHitAnim(number, anim)) {
            getHitSelected = true;
        } else if (number == CommonState::kIntro) {
            // `[Statedef 190]`'s `ChangeAnim 190` is guarded by
            // `SelfAnimExist(190)`. A file-less fighter has no pre-intro pose,
            // and telling it to draw one would leave it invisible for the tick
            // it spends there; leaving `anim` at -1 keeps the pose it was in.
            if (animation(CommonState::kIntro) != nullptr) anim = CommonState::kIntro;
        } else if (number == CommonState::kContinue) {
            // `[Statedef 5500]` names 5300 outright and only takes 5500 when
            // `SelfAnimExist(5500)`; the continue screen's fighter keeps its
            // knocked-out pose otherwise.
            anim = animation(CommonState::kContinue) != nullptr ? CommonState::kContinue : 5300;
        }
        // A negative answer from both the selection and the row is the table's
        // "keep the pose you are in", the same convention the header's `anim`
        // uses. Setting `-1` here would instead blank the fighter: the row
        // writes it for 190/191 precisely because those poses are conditional,
        // and a character without them is meant to keep its standing art
        // through the intro.
        const int chosen = anim >= 0 ? anim : builtin->anim;
        if (!getHitSelected && chosen >= 0) setAnim(chosen, true);
        else if (getHitSelected && anim >= 0) setAnim(anim, true);
        return;
    }

    // An unknown state number with no header. Standing physics and control on is
    // the forgiving reading: the character's own controllers -- there are
    // usually some, or it would not have reached this state -- decide
    // everything else, and a state that cannot be interrupted would be a much
    // more common way to freeze a fighter than one that can.
    stateType_ = StateType::Stand;
    moveType_ = MoveType::Idle;
    physics_ = Physics::Stand;
    ctrl_ = true;
    if (state != nullptr && state->hasAnim) {
        const int anim = animFromHeader(*state, *this);
        if (anim >= 0) setAnim(anim, true);
    }
}

void StateMachine::changeState(int number, bool keepCtrl) {
    // A knocked-out fighter does not get up.
    //
    // MUGEN ends the round when a life bar empties, so its own common states
    // hand the fighter back to standing without asking whether it is alive --
    // state 5110 and 5120 are a get-up animation whose last controller is
    // `ChangeState 0`. Following that would stand a corpse up, so once life has
    // run out the transitions back to an idle state are refused. The states that
    // keep a dead fighter dead -- lying down, and the moves between them -- are
    // still allowed, and so is anything the round's own ending does.
    if (life_ <= 0) {
        const StateDef* target = stateLibrary() != nullptr ? stateLibrary()->find(number) : nullptr;
        const bool idleTarget = number == 0 || (target != nullptr && !target->moveTypeUnchanged &&
                                                target->moveType == MoveType::Idle);
        if (idleTarget) return;
    }

    // A state change that fires under hitpause moves the state but leaves its
    // entry work for the tick the freeze lifts: the reference's runner puts the
    // whole entry block under `hitpausetime <= 0`, so an `ignorehitpause`
    // ChangeState is the one kind that reaches it frozen. Deferring the header
    // is not cosmetic -- the entry is what applies the new statedef's physics,
    // animation and type, and MUGEN does not have those in force while the
    // fighter is still frozen.
    const bool frozen = pauseTicks_ > 0;
    if (number == stateNumber_) {
        // MUGEN allows a state to restart itself, which resets its time and
        // clears its HitDef -- that is how a looping attack re-arms.
        if (frozen) {
            stateTime_ = 0;
            pendingStateEntry_ = true;
            return;
        }
        enterState(number);
        return;
    }
    const bool ctrlBefore = ctrl_;
    prevStateNumber_ = stateNumber_;
    if (frozen) {
        stateNumber_ = number;
        stateTime_ = 0;
        pendingStateEntry_ = true;
        if (keepCtrl) ctrl_ = ctrlBefore;
        return;
    }
    enterState(number);
    if (keepCtrl) ctrl_ = ctrlBefore;
}

// ------------------------------------------------------------------ the tick

void StateMachine::control(const InputState& input) {
    lastInput_ = input;
    // The buffer stores screen directions and the facing they were read with,
    // because a fighter can turn round mid-motion and the same tick then means
    // the opposite.
    buffer_.push(input, facing_);
}

void StateMachine::step() {
    tookDamage_ = false;

    // A match-wide freeze (Pause/SuperPause) holds this whole machine *before*
    // its own hitpause is even counted: the world stops, and the fighter's own
    // bookkeeping stops with it. The pauser is exempt for its movetime, and a
    // helper of the pauser's family is exempt inside its pausemovetime budget --
    // which is what makes a super flash play while the victim hangs in the air.
    if (entities_ != nullptr) {
        const PauseInfo pause = entities_->pauseInfo();
        if (pause.ticks > 0) {
            bool frozen;
            if (pause.actor == this) {
                frozen = pause.actorMove <= 0;
            } else if (isHelper_ && pause.actor != nullptr &&
                       pause.actor->root_ == root_) {
                const int budget = pause.isSuper ? superMoveTime_ : pauseMoveTime_;
                frozen = (pause.total - pause.ticks) >= budget;
            } else {
                frozen = true;
            }
            if (frozen) return;
        }
    }

    // A bind holds through everything -- hitpause, hitstun, the lot -- because a
    // throw's victim is dragged by exactly this while both fighters are frozen.
    applyBind();

    // Hit pause freezes the fighter completely: no controllers, no animation, no
    // physics. It is the single cheapest thing that makes a hit read as a hit --
    // without it a jab and a super-land are the same event with different numbers,
    // and with it the impact has a moment of dead silence to land in. Both
    // fighters are frozen, for their own lengths of time, which is why a hit that
    // pauses the attacker for 8 and the victim for 12 looks like the move
    // connecting rather than like two animations that happen to overlap.
    //
    // MUGEN counts the pause down before anything else runs, so the frame the hit
    // lands is the frame the freeze starts, and the fighter resumes exactly
    // `pause` ticks later.
    if (pauseTicks_ > 0) {
        --pauseTicks_;
        // The shake runs on alongside the freeze rather than being stopped by it.
        // It is a rendering offset only -- a shaken fighter still stands where it
        // stands -- and `hitshakeover` reports it, which is why a character can
        // tell "I am still being shaken" from "I am still stunned".
        //
        // The jitter's parity comes from the shake counter and not from the state
        // time, which is MUGEN's own ingredient. The state time cannot be used here
        // because the pause is exactly what stops it advancing, so the shake would
        // hold one offset for the whole freeze instead of alternating.
        if (shakeTicks_ > 0) --shakeTicks_;
        shakeX_ = (shakeTicks_ & 1) != 0 ? -static_cast<float>(facing_) : 0.0f;

        // Controllers that wrote `ignorehitpause = 1` keep running through the
        // freeze -- all the states, in the same order. A freeze that silenced even
        // those breaks the spark and bookkeeping controllers characters put inside
        // their hits on exactly this flag.
        if (character_ != nullptr && !doomed()) {
            stateChangesThisTick_ = 0;
            if (!isHelper_ || keyCtrl_) runControllersIf(stateLibrary(), -1, true);
            // The same conditions as the unfrozen path: -2 is the no-control
            // state and -3 the computer's, not "whatever runs during a freeze".
            if (!ctrl_) runControllersIf(stateLibrary(), -2, true);
            if (aiControlled_) runControllersIf(stateLibrary(), -3, true);
            if (const StateDef* state = stateLibrary() != nullptr
                                            ? stateLibrary()->find(stateNumber_)
                                            : nullptr) {
                runControllersFiltered(state->controllers, true);
            }
        }
        return;
    }
    // The state entry an `ignorehitpause` ChangeState owed from inside the
    // freeze. It applies now, on the first tick the fighter is unfrozen, which
    // is where the reference's entry block sits.
    if (pendingStateEntry_) {
        pendingStateEntry_ = false;
        enterState(stateNumber_);
    }

    // The recovery window on the last hit runs down with the fighter's clock --
    // and not during its own hitpause, which the early return above has already
    // taken care of, because a freeze that counted the window down would shorten
    // it by however long the impact held.
    if (lastHit_.fallRecoverTicks > 0) --lastHit_.fallRecoverTicks;

    shakeX_ = 0.0f;

    // AssertSpecial flags last one tick, so they are cleared here and raised again by
    // whatever runs below. A character that wants a property to hold writes the
    // controller with `trigger1 = 1` inside a state that lasts as long as it should.
    assert_ = 0;
    if (invisibleTicks_ > 0) --invisibleTicks_;
    if (notHitTicks_ > 0) --notHitTicks_;
    if (notHitThrowTicks_ > 0) --notHitThrowTicks_;
    // PosFreeze is asserted afresh every tick by whoever wants it.
    posFreezeThisTick_ = false;
    // And so is `Trans`, whose own documentation says it holds "for current game
    // tick" only. A character that wants it to last keeps triggering it.
    transThisTick_.reset();
    transAddAlphaThisTick_ = false;
    // The width bar is asserted the same way: docs/sctrls.html says `Width`
    // "Temporarily changes the size of the player's width bar for 1 tick", and
    // the reference zeroes the four fields at the top of the player's update.
    // The controller runs below, so every reader later in this tick sees it.
    edgeFront_ = 0;
    edgeBack_ = 0;
    playerFront_ = 0;
    playerBack_ = 0;

    if (character_ == nullptr) {
        // A machine with no character behind it is a dummy: it holds its ground
        // and can be hit, which is all the engine test's arena asks of it.
        stepPhysics();
        ++stateTime_;
        prevStateNumber_ = stateNumber_;
        return;
    }

    stateChangesThisTick_ = 0;
    runStateLogic();
    updateAnimation();
    stepPhysics();

    // `time` counts ticks spent in the state, and is 0 on the tick the state was
    // entered -- which is what makes `trigger1 = time = 0` mean "on entry".
    ++stateTime_;

    // The trail's clock runs with the fighter's, so it freezes when the fighter
    // does (the early returns above) and ends with its `time` either way.
    if (afterImage_.ticksLeft > 0) --afterImage_.ticksLeft;

    // PalFX ages the same way, for the same reason: a hitstop freezes the flash
    // mid-fade, which is what a frozen frame of a super looks like in MUGEN.
    if (palFx_.active) {
        ++palFx_.elapsed;
        if (palFx_.time > 0 && --palFx_.time == 0) palFx_.active = false;
    }

    // HitOverride slots age the same way; an expired one catches nothing.
    for (HitOverrideSlot& slot : hitOverrides_) {
        if (slot.active && --slot.ticksLeft <= 0) slot.active = false;
    }

    // The reference's own clock, instruction for instruction: it decrements while
    // the value is `>= 0` and leaves it at -1, which is why `hitover` is
    // `hittime < 0` rather than `hittime == 0` and why `GetHitVar(hittime)` can
    // report -1 once a reaction is over. It does not run while the fighter is
    // frozen -- the early return above is the reference's `if shake != 0: skip`
    // gate, arrived at from the other side (the shake and the freeze are the same
    // length here, so the shot clock ends with the freeze).
    if (stunTicks_ >= 0) --stunTicks_;

    // The move-contact counters age with the stun clock: a contact set them to
    // one between the ticks, the controllers of the tick after read that one,
    // and each later tick counts on -- which is what makes `movecontact = 3` a
    // number three ticks after the connect and `>= 2 && <= 4` a window it held
    // true for. Like the clock, the counters do not run while the fighter is
    // frozen; the early return above is what keeps them still.
    if (moveContact_ > 0) ++moveContact_;
    if (moveHit_ > 0) ++moveHit_;
    if (moveGuarded_ > 0) ++moveGuarded_;
    if (moveReversed_ > 0) ++moveReversed_;

    // Hitstun over and still in a hit state: for a character that wrote the
    // reaction itself, nothing happens here -- `HitOver` has just become true
    // and the file's own get-hit state is what leaves (the reference's 5001
    // hands over on exactly that trigger). This is the fallback for the
    // characters that did *not*: a character with no `[Statedef 5000]` of its
    // own is played by the engine's built-in table, which has no opinion about
    // when a reaction ends, so the engine ends it. A knocked-out fighter stays
    // where it fell.
    //
    // The reaction has to be over in the air *and* on the ground, which is why
    // this is not inside the countdown above. A launched victim's stun is
    // shorter than its fall -- `air.hittime` is a couple of dozen ticks and the
    // arc runs longer -- so ending the reaction the moment the counter reaches
    // zero hands a fighter sixty units up to a state whose physics is `Stand`,
    // and the floor clamp drops it there in a single step: no landing, no fall
    // damage, no bounce, the whole arc discarded. And because the countdown only
    // reaches zero once, the test has to be re-asked on every later tick rather
    // than only on that one -- otherwise a victim whose stun expired at the top
    // of its arc never leaves the hit state at all and lies on the floor in it
    // forever.
    //
    // `y_ <= 0` alone is not enough, because this runs *after* `stepPhysics`
    // and the landing tick is the tick the bounce starts: the fighter is at the
    // floor with the whole bounce velocity still on it, and ending the reaction
    // there cancels the bounce the slam it just took was written to produce. A
    // bounce that is still going up is not a fighter that has finished falling.
    //
    // Lying down is exempt, and has to be. 5110 is not a reaction the engine is
    // waiting to end -- it is a hold the engine itself times, out of
    // `[Data] liedown.time`, and this fallback would cut that hold to the length
    // of the hit that caused it. Without the exemption a fighter with no art for
    // the pose is back on its feet one tick after the knockdown, which is the
    // exact failure the liedown clock exists to fix.
    //
    // A state the built-in table *knows* is exempt as well, and for the same
    // kind of reason: the table is where the reference's own routes are written
    // down -- 5000 to 5001, 5010 to 5011 and 5011 back to the crouch, 5030 to
    // the fall or to the air recovery -- and those states are only ever reached
    // from here, because the table is gated on the stun being over. Cutting them
    // off at the stun boundary sent every file-less victim to state 0 instead:
    // a fighter knocked out of a crouch stood up when the shake ended, and a
    // victim the table would have carried to 5110 or 5120 was handed back to
    // standing from the middle of its own reaction. The fallback is still the
    // right answer for a state number the table has no row for -- a character's
    // own get-hit state, or a `p2stateno` destination the engine does not
    // recognise -- which is what it is left to do.
    if (stunTicks_ < 0 && life_ > 0 && moveType_ == MoveType::Hit && !ownsCurrentState() &&
        builtinFor(stateNumber_) == nullptr && stateNumber_ != CommonState::kDowned &&
        y_ <= 0.0f && velY_ <= 0.0f) {
        changeState(0, false);
    }

    // Guard energy comes back once the fighter has been left alone for a moment, which
    // is the delay that stops a single exchange from refilling the meter. Hitstun does
    // not count as being left alone: being comboed should not restore a block.
    if (guardEnergy_ < kGuardEnergyMax) {
        if (++guardRestTicks_ >= kGuardRestTicks) {
            guardEnergy_ = std::min(kGuardEnergyMax, guardEnergy_ + kGuardRecoverPerTick);
        }
    } else {
        guardRestTicks_ = 0;
    }

    // The knock-out stand-up. Being dead in this engine is a *lying* state with
    // no exit, which is what makes a fighter stay down for the rest of the round
    // -- but not for the rest of the match. The engine's own round clock is what
    // ends it: once the round has been decided (round state 3, which is where the
    // fight screen puts it on a knock-out or a time-over) and the loser has been
    // lying dead for the full ninety ticks, it is stood up and given its life
    // back, ready for the next round. `life_` is restored *before* the state
    // change, because 5110's hand-over reads the same number to decide whether
    // this is a fighter getting up or a corpse: set after, and the get-up would
    // immediately collapse back into 5150.
    if (roundState_ == 3 && stateNumber_ == CommonState::kDead &&
        stateTime() >= kKnockOutStandUpTicks) {
        life_ = maxLife_;
        changeState(CommonState::kDownedGetUp, false);
    }

    // The combo clock `GetHitVar(hitcount)` is judged against: how many ticks
    // this fighter has spent free of a hit reaction. The reference keeps it as a
    // count that a hit reaction zeroes and every other tick advances, and then
    // asks `> 1` rather than `== 0` before calling the next hit a new combo -- so
    // one free frame does not end a combo, two do. See `applyHit`.
    //
    // Written at the *end* of the tick, after everything above has had its say
    // about the move type, because that is what makes the tick a reaction ends on
    // its own first free frame: the fallback hand-back to state 0 a few lines up
    // is the moment this fighter stops being in a hit reaction, and the hit that
    // lands on the very next tick is meant to read a count of one -- a link --
    // rather than of zero. Counting the tick before the hand-back would spend
    // that free frame on the reaction itself and silently turn every such link
    // into a new combo.
    if (moveType_ == MoveType::Hit) {
        framesSinceHitReaction_ = 0;
    } else {
        ++framesSinceHitReaction_;
    }

    // The liedown clock behind `GetHitVar(recovertime)`. Re-armed from the file's
    // `[Data] liedown.time` on every tick this fighter is *not* in a hit
    // reaction, and counted down only while it is lying in 5110 -- which is what
    // makes the key answer different things at different times, and both of them
    // the right thing. Read from a fighter on its feet it is the file's own
    // number, which is what a state writing `GetHitVar(recovertime)` before a
    // knockdown is asking about; read from one on the floor it is how much of
    // that time is actually left. The re-arm is guarded by the move type rather
    // than by the state for the same reason: a knocked-down fighter is in a hit
    // reaction, so nothing resets the clock under it while it runs.
    if (moveType_ == MoveType::Hit) {
        if (stateNumber_ == CommonState::kDowned && liedownTicks_ > 0) --liedownTicks_;
    } else {
        liedownTicks_ = character_ != nullptr ? character_->liedownTime() : 0;
    }
}

void StateMachine::runStateLogic() {
    // Hitstun is not a suspension of the character's state machine. MUGEN's own
    // model runs every state on every tick -- "for each tick of game-time, MUGEN
    // makes a single pass through each of the special states ... After all the
    // state controllers in the special states have been checked, the player's
    // current state is processed" -- with no exemption for a fighter in a
    // get-hit state, and the reason is that the get-hit states *are* the
    // reaction: the gravity that brings a launched victim down is a
    // `VelAdd y = GetHitVar(yaccel)` inside the character's own 5030, and the
    // hand-over to the landing states is a `ChangeState` in the same file.
    //
    // What hitstun takes away is *control*: the hit states are written
    // `ctrl = 0`, so nothing the pad or the AI does reaches them. Suppressing the
    // file's controllers as well leaves a launched victim in a state that
    // declares `physics = N` with nobody left to apply gravity -- it rises out of
    // the screen at a constant velocity and is dropped back to standing when the
    // stun runs out, which is what this engine did.
    //
    // Hit *pause* is the freeze that stops controllers, and it is handled in
    // `step()` before this runs: only the controllers that wrote
    // `ignorehitpause = 1` are run there, which is the reference's own default
    // for that parameter (0).
    if (stunTicks_ > 0) {
        // The special states are left out for now while stunned; what a get-hit
        // state needs is its own controllers.
        runCurrentState();
        return;
    }
    const int stunBefore = stunTicks_;

    // State -1 runs once every tick, whatever state the fighter is in, and
    // *before* the state itself: it is where commands become moves. A character
    // without one has only the states its physics can reach, which is none.
    //
    // A helper skips it: commands are the pad's, and a helper is not its own
    // player. The exception is `keyctrl = 1`, which hands the helper its side's
    // input and with it the command table that input drives.
    //
    // It is deliberately not re-run when a controller changes state below:
    // MUGEN reads the command state once a tick, and running it again would
    // fire every command in it twice.
    if (!isHelper_ || keyCtrl_) runStateMinus1();
    // A controller here that *started* the hitstun -- a LifeAdd that trips a hit
    // state -- ends the tick: the rest of it belongs to a fighter that has just
    // been hit. Merely being stunned already does not, or a character's own
    // reaction would stop after its first controller.
    if (stunTicks_ > stunBefore) return;

    // State -2 is the no-control state, and state -3 the computer's. Both are
    // read before the current state, so a controller in either can change it
    // this very tick.
    const int before = stateNumber_;
    if (!ctrl_) runStateMinus2();
    if (aiControlled_) runStateMinus3();

    // A controller in -2 or -3 that changed the state has already had its say;
    // the state it moved to gets its own turn from runCurrentState().
    if (stateNumber_ == before) runCurrentState();
}

void StateMachine::runCurrentState() {
    // A change of state inside a controller hands over to the new state in the
    // same tick, at time 0. The loop is what does that, and the count is what
    // stops a character whose states point at each other from taking the match
    // loop with them.
    for (int guard = 0; guard < kMaxStateChangesPerTick; ++guard) {
        const int before = stateNumber_;
        stateChangesThisTick_ = guard;

        const StateDef* state = stateLibrary() != nullptr ? stateLibrary()->find(stateNumber_)
                                                         : nullptr;
        // A state the character wrote a header for is the character's own: its
        // controllers run, and the engine contributes the physics. Everything
        // else -- a state it left out, or one it only attached controllers to --
        // is MUGEN's built-in behaviour, with the character's controllers running
        // on top of it.
        //
        // What the engine contributes to a state the character *did* define is
        // narrower but not empty: the direction-driven transitions that
        // common1.cns does not contain, so that a character which defines its own
        // state 0 -- or its own state 11 -- still crouches, stands back up and
        // walks. The character's own controllers run first, so a state that does
        // handle its own movement, or its own transformation, still wins.
        if (state != nullptr && state->defined) {
            runControllers(state->controllers);
            if (stateNumber_ == before) runBuiltinMovement();
        } else if (stunTicks_ >= 0 && !builtinExitsOnShake(stateNumber_)) {
            // A fighter in hitstun runs its own controllers and nothing else. The
            // built-in table below is the engine's substitute for a character
            // that ships no common state file, and it carries the reference's own
            // reaction routes -- 5000 to 5001, 5010 to 5011 and 5011 back into
            // the crouch, 5030 to the fall or the air recovery, 5050 down to the
            // floor. What it cannot be is *early*: those hand-overs fire as soon
            // as the pose's animation runs out, which for a character with no art
            // for the pose is the tick it enters, and a reaction that ended on
            // its first tick would not be a reaction. So the stun is held here,
            // and the table takes the reaction over on the tick it expires.
            // "Expires" is `hittime < 0`, so the hold covers the tick the clock
            // reads 0 as well: the reference's 151 leaves on `HitOver`, which is
            // true one tick after the number passes zero, and this is where that
            // tick is kept.
            //
            // The shakes are the exception, and it is the reference's own rather
            // than a hole in this: their hold is not the animation and not the stun
            // either, but the hit's *shake* clock, which is the number the reference
            // gives them as their only exit (`trigger1 = HitShakeOver`). Holding
            // them for the stun as well meant a launched victim stayed in the pose
            // they were hit in for the whole of `air.hittime` before the flight
            // began -- the shake covered by the freeze, and the reaction by nothing.
            // See `builtinExitsOnShake`.
            if (state != nullptr) runControllers(state->controllers);
            // The one animation-driven exit allowed inside hitstun: 5030's
            // `[State 5030, 6] ChangeState / trigger1 = AnimTime = 0 / value =
            // 5035`. It is the reference's earliest exit for that state, and it
            // is deliberately narrower than letting the whole built-in table
            // run early -- only the move to 5035 happens here. 5035's own exits
            // still wait for the stun gate below, so the victim cannot leave
            // the hit before `HitOver`.
            if (stateNumber_ == before && stateNumber_ == CommonState::kHitAirFall &&
                animationFinished()) {
                changeState(CommonState::kHitAirTransition, false);
            }
        } else {
            runBuiltinState();
        }
        if (stateNumber_ == before) return;
    }
}

void StateMachine::runStateMinus1() {
    if (stateLibrary() == nullptr) return;
    const StateDef* state = stateLibrary()->find(-1);
    if (state == nullptr) return;
    runControllers(state->controllers);
}

void StateMachine::runStateMinus2() {
    if (stateLibrary() == nullptr) return;
    const StateDef* state = stateLibrary()->find(-2);
    if (state == nullptr) return;
    runControllers(state->controllers);
}

void StateMachine::runStateMinus3() {
    if (stateLibrary() == nullptr) return;
    const StateDef* state = stateLibrary()->find(-3);
    if (state == nullptr) return;
    runControllers(state->controllers);
}

void StateMachine::runControllers(const std::vector<Controller>& controllers) {
    runControllersFiltered(controllers, false);
}

void StateMachine::runControllersIf(const StateLibrary* states, int stateNo,
                                    bool duringHitPause) {
    if (states == nullptr) return;
    const StateDef* state = states->find(stateNo);
    if (state == nullptr) return;
    runControllersFiltered(state->controllers, duringHitPause);
}

void StateMachine::runControllersFiltered(const std::vector<Controller>& controllers,
                                          bool duringHitPause) {
    for (const Controller& controller : controllers) {
        // A controller with no triggers never runs. MUGEN requires the trigger,
        // and running one unconditionally would be the worst possible reading
        // of a half-written state.
        if (controller.empty() || !controller.hasTriggers()) continue;
        // Through a freeze, only the controllers that opted out of it run.
        if (duringHitPause && !controller.ignoresHitPause()) continue;
        // `persistent = n`: at most once per n ticks of this state entry.
        const int persistent = controller.persistent();
        if (persistent > 1) {
            const auto it = lastRunTick_.find(&controller);
            if (it != lastRunTick_.end() && gameTime_ - it->second < persistent) continue;
        }
        if (!controller.triggered(*this)) continue;

        const int before = stateNumber_;
        const int stunBefore = stunTicks_;
        if (!runController(controller)) {
            // Recorded once per type rather than once per occurrence: a
            // character can carry hundreds of instances of a controller this
            // machine does not implement, and a line each would bury the log.
            const std::string& type = controller.type();
            if (unsupported_.find(type) == unsupported_.end()) {
                unsupported_[type] = 1;
                AF_LOGD("state machine: %s: controller type \"%s\" not implemented",
                        name().c_str(), type.c_str());
            }
            continue;
        }
        if (persistent > 1) lastRunTick_[&controller] = gameTime_;

        // A ChangeState ends the list: runCurrentState() picks the new state up
        // in the same tick, and running the rest of the abandoned state's
        // controllers would be running a state the character has left.
        if (stateNumber_ != before) return;

        // Hitstun can start inside a state's own controllers (a VarSet that
        // trips a hit state, a LifeAdd), and the rest of the list belongs to a
        // fighter that is no longer in control. Already being stunned does not
        // end the list: that is the state a get-hit reaction *runs* in, and it
        // would stop after the character's first controller.
        if (stunTicks_ > stunBefore) return;
    }
}

// ------------------------------------------------------------- controllers

bool StateMachine::runController(const Controller& controller) {
    const std::string& type = controller.type();

    // The dispatch is by first letter groups to keep the comparisons cheap:
    // this runs for every controller of every state of both fighters, every
    // tick, and a character's state 0 alone can hold a dozen.
    switch (type.empty() ? '\0' : type[0]) {
        case 'c':
            if (type == "changestate") {
                doChangeState(controller);
                return true;
            }
            if (type == "changeanim") {
                doChangeAnim(controller);
                return true;
            }
            if (type == "changeanim2") {
                doChangeAnim2(controller);
                return true;
            }
            if (type == "ctrlset") {
                ctrl_ = controller.boolParam("value", false) ||
                        controller.evalNumber("value", *this, 0.0f) != 0.0f;
                return true;
            }
            return false;
        case 'v':
            if (type == "velset" || type == "veladd" || type == "velmul") {
                doVelocity(controller);
                return true;
            }
            if (type == "victoryquote") {
                // "Specifies the index of the quote to use. Valid index values
                // are from 0 to 99. If quote_index evaluates to an invalid
                // index, a random quote will be selected. Defaults to -1."
                // (sctrls.html.) The value is recorded on this machine; a
                // controller on the losing side is harmless because the
                // victory flow reads only the winner's machine.
                //
                // "This controller has no effect if executed by a helper."
                // Checking the machine, not the character, is what makes that
                // true: a helper runs the same character's states, and an
                // unguarded write would let a flash double decide the quote of
                // a fighter it does not own.
                if (!isHelper_ && character_ != nullptr) {
                    const float requested = controller.evalNumber(
                        "value", *this, static_cast<float>(kVictoryQuoteRandom));
                    int index = kVictoryQuoteRandom;
                    if (std::isfinite(requested)) {
                        index = static_cast<int>(std::lround(requested));
                    }
                    // A value in range names a quote only when the author
                    // actually wrote it. An out-of-range value and a blank
                    // victoryN both mean "there is no quote at that index", so
                    // both leave the random marker for the screen to resolve.
                    victoryQuoteIndex_ =
                        hasVictoryQuote(character_->victoryQuotes(), index)
                            ? index
                            : kVictoryQuoteRandom;
                }
                return true;
            }
            if (type == "varset" || type == "varadd" || type == "varrangeset" ||
                type == "varrandom") {
                doVariables(controller);
                return true;
            }
            return false;
        case 'p':
            if (type == "posset" || type == "posadd") {
                doPosition(controller);
                return true;
            }
            if (type == "projectile") {
                doProjectile(controller);
                return true;
            }
            if (type == "poweradd") {
                power_ = clampPower(power_ + static_cast<int>(
                                                std::lround(controller.evalNumber("value", *this, 0))));
                return true;
            }
            // `PowerSet`: "value is the new value to set the player's power to" -- the
            // sibling of the LifeSet beside it, and the one place a character can hand
            // power *back* without a `poweradd` of a negative number. Clamped through
            // the same function `poweradd` uses, because the two have to agree about
            // what a power total can be: `poweradd` may take it below zero (that is how
            // a super's cost is paid when the move is entered on a state that has
            // already spent it), and a set to a value past the cap is a file asking for
            // something the meter cannot show.
            if (type == "powerset") {
                power_ = clampPower(static_cast<int>(
                    std::lround(controller.evalNumber("value", *this, power_))));
                return true;
            }
            if (type == "playsnd") {
                doPlaySnd(controller);
                return true;
            }
            if (type == "pause") {
                doPause(controller, false);
                return true;
            }
            if (type == "playerpush") {
                playerPush_ = firstParam(controller, "value", *this, 1.0f) != 0.0f;
                return true;
            }
            if (type == "posfreeze") {
                posFreezeThisTick_ = firstParam(controller, "value", *this, 1.0f) != 0.0f;
                return true;
            }
            if (type == "palfx") {
                doPalFx(controller);
                return true;
            }
            if (type == "parentvarset" || type == "parentvaradd") {
                doParentVar(controller, type == "parentvaradd");
                return true;
            }
            return false;
        case 'h':
            if (type == "hitdef") {
                doHitDef(controller);
                return true;
            }
            if (type == "hitvelset") {
                doHitVelSet(controller);
                return true;
            }
            if (type == "hitby") {
                doHitBy(controller);
                return true;
            }
            if (type == "helper") {
                doHelper(controller);
                return true;
            }
            if (type == "hitoverride") {
                doHitOverride(controller);
                return true;
            }
            if (type == "hitfalldamage") {
                doHitFallDamage();
                return true;
            }
            if (type == "hitfallvel") {
                doHitFallVel();
                return true;
            }
            if (type == "hitfallset") {
                hitFallFlag_ = firstParam(controller, "value", *this, 1.0f) != 0.0f;
                return true;
            }
            if (type == "hitadd") {
                hitCount_ += static_cast<int>(
                    std::lround(controller.evalNumber("value", *this, 0.0f)));
                return true;
            }
            return false;
        case 'n':
            if (type == "null") return true;
            // `NotHitBy` starts with an n, not with the h of the family it belongs to --
            // it is filed under 'n' beside Null, and filed under 'h' it never ran at all.
            if (type == "nothitby") {
                doNotHitBy(controller);
                return true;
            }
            return false;
        case 'o':
            // `Offset` is the visual displacement -- a lean, a step the artwork
            // does not carry -- and it belongs to the same group as the position
            // controllers. It is dispatched on its own letter because the letter
            // is what the switch reads: filed under `p` it was never reached,
            // and every character that leans was told its controller was not
            // implemented.
            if (type == "offset") {
                doPosition(controller);
                return true;
            }
            return false;
        case 't':
            if (type == "turn") {
                facing_ = -facing_;
                return true;
            }
            if (type == "trans") {
                doTrans(controller);
                return true;
            }
            if (type == "targetstate" || type == "targetlifeadd" || type == "targetpoweradd" ||
                type == "targetvelset" || type == "targetveladd" || type == "targetfacing" ||
                type == "targetbind" || type == "targetdrop" || type == "targetadd") {
                doTargetOp(controller);
                return true;
            }
            return false;
        case 's':
            if (type == "selfstate") {
                // `Like ChangeState, except that this changes a player back to a
                // state in his own state data. Use this when you have placed an
                // opponent player in a custom state via an attack, and wish to
                // restore the opponent to his own states.` This is the one place
                // the difference between the two spellings shows: while a
                // `p2stateno` hit has this fighter running the attacker's file,
                // ChangeState means another state of *that* file and SelfState means
                // home. It is the line a throw ends on.
                borrowedStateChar_ = nullptr;
                doChangeState(controller);
                return true;
            }
            if (type == "statetypeset") {
                // `statetype = A` and `movetype = A` are written as bare
                // letters, so the raw text is what identifies them. All three
                // parameters are optional and each defaults to *no change*,
                // which is why every one of them is parsed with the current
                // value as its fallback rather than with a fixed default: the
                // reference's own states write them one at a time.
                stateType_ = parseStateType(controller.raw("statetype"), stateType_);
                moveType_ = parseMoveType(controller.raw("movetype"), moveType_);
                // `physics` was being read and thrown away, and the states that
                // need it most are the guard ones: `[State 140, 4]` is the only
                // thing that gives an *air* guard release its gravity back --
                // 140's own header is `physics = U`, so without this a fighter
                // that blocked in the air and let go hung in the sky, 132 being
                // `physics = N`. The reference's `Hi to Lo` / `Lo to Hi` pairs
                // in 120 and 140 set both letters together for the same reason.
                physics_ = parsePhysics(controller.raw("physics"), physics_);
                return true;
            }
            if (type == "sprpriority") {
                spritePriority_ = static_cast<int>(
                    std::lround(controller.evalNumber("value", *this, spritePriority_)));
                return true;
            }
            if (type == "superpause") {
                doPause(controller, true);
                return true;
            }
            if (type == "screenbound") {
                screenBound_ = firstParam(controller, "value", *this, 1.0f) != 0.0f;
                return true;
            }
            if (type == "stopsnd") {
                if (sound_ != nullptr) {
                    sound_->stopChannel(static_cast<int>(std::lround(
                        firstParam(controller, "channel", *this, -1.0f))));
                }
                return true;
            }
            if (type == "sndpan") {
                if (sound_ != nullptr) {
                    const int channel = static_cast<int>(
                        std::lround(firstParam(controller, "channel", *this, 0.0f)));
                    float pan = controller.has("pan") ? firstParam(controller, "pan", *this, 0.0f)
                                                      : firstParam(controller, "abspan", *this, 0.0f);
                    pan = std::fabs(pan) > 4.0f ? std::clamp((pan - 128.0f) / 128.0f, -1.0f, 1.0f)
                                                : std::clamp(pan, -1.0f, 1.0f);
                    sound_->panChannel(channel, pan);
                }
                return true;
            }
            if (type == "stop" || type == "selfstateset") {
                return true;
            }
            return false;
        case 'a':
            if (type == "assertspecial") {
                doAssertSpecial(controller);
                return true;
            }
            if (type == "afterimage") {
                doAfterImage(controller);
                return true;
            }
            if (type == "afterimagetime") {
                // The trail with the state's own knobs, for n ticks. The state's
                // knobs are MUGEN's `afterimage` block, which the statedef header
                // does not model, so the last AfterImage-written knobs stand.
                const int time = static_cast<int>(
                    std::lround(controller.evalNumber("time", *this, 0.0f)));
                afterImage_.ticksLeft =
                    time <= 0 ? 0 : std::max(afterImage_.ticksLeft, time);
                return true;
            }
            if (type == "angleset") {
                drawAngleDeg_ = firstParam(controller, "value", *this, drawAngleDeg_);
                return true;
            }
            if (type == "angleadd") {
                drawAngleDeg_ += firstParam(controller, "value", *this, 0.0f);
                return true;
            }
            if (type == "anglemul") {
                drawAngleDeg_ *= firstParam(controller, "value", *this, 1.0f);
                return true;
            }
            if (type == "angledraw") {
                // The marker that draws the fighter rotated. Its optional `value`
                // overrides the standing angle, which is how a one-off spin is
                // written.
                if (controller.has("value")) {
                    drawAngleDeg_ = firstParam(controller, "value", *this, drawAngleDeg_);
                }
                return true;
            }
            if (type == "allpalfx") {
                // The whole-screen scope: a grade on the fight's second global slot,
                // drawn over everything -- background, fighters, effects and HUD. It is
                // a slot of its own, not an alias of BGPalFX or of the fighter's PalFX.
                if (entities_ != nullptr) entities_->setAllPalFx(readPalFxState(controller));
                return true;
            }
            if (type == "appendtoclipboard") {
                // The debug clipboard has no host here.
                return true;
            }
            if (type == "attackmulset") {
                attackMul_ = controller.evalNumber("value", *this, 1.0f);
                return true;
            }
            if (type == "attackdist") {
                // `sctrls.html`: "Changes the value of the guard.dist parameter for
                // the player's current HitDef" -- a controller rather than a HitDef
                // parameter, which is the whole of why it exists: it is how a
                // character takes the guard distance off a HitDef it has already
                // armed, and how a state that arms one conditionally can set it
                // without repeating the HitDef.
                //
                // "Current" is the operative word. All 124 uses in the corpus run on
                // `trigger1 = 1`, so ordering within the state is the only thing that
                // decides the winner between this and the state's own HitDef -- and 110
                // of the 124 sit in states that arm nothing at all, a block the packs
                // copied wholesale out of a throw state and never re-pointed. That is
                // harmless rather than pointless here: what the controller writes is
                // the player's live guard distance, which such a state still answers
                // `inguarddist` with while its `movetype` is A.
                const int distance = static_cast<int>(
                    std::lround(controller.evalNumber("value", *this, -1.0f)));
                // The player's own live slot, which is the same one the HitDef executor
                // writes, and the only reading under which this controller can do
                // anything at all: a value written into the armed HitDef alone would be
                // overwritten by the next tick's parse, and a HitDef that omits
                // `guard.dist` -- the usual case -- never touches the slot, so a value
                // set here survives. A HitDef that *does* write `guard.dist` writes the
                // same slot on the ticks it runs, and wins by order.
                guardDist_ = distance;
                return true;
            }
            return false;
        case 'z':
            if (type == "zoom") return true;
            return false;
        case 'l':
            // Life changes are recognised but not allowed to revive: a fighter
            // whose life has run out has been knocked out, and that is the
            // result of the round. Characters do write a LifeSet that restores
            // life -- a second wind, or a reset meant for the *next* round that
            // their triggers let through here -- and honouring it after a
            // knock-out would put a defeated fighter back on its feet and leave
            // the match with no winner at all.
            if (type == "lifeadd") {
                if (life_ > 0) {
                    life_ = std::max(0, life_ + static_cast<int>(std::lround(
                                                   controller.evalNumber("value", *this, 0))));
                }
                tookDamage_ = true;
                return true;
            }
            if (type == "lifeset") {
                if (life_ > 0) {
                    life_ = std::max(0, static_cast<int>(std::lround(
                                           controller.evalNumber("value", *this, life_))));
                }
                return true;
            }
            return false;
        case 'e':
            if (type == "explod") {
                doExplod(controller);
                return true;
            }
            if (type == "envshake") {
                doEnvShake(controller);
                return true;
            }
            if (type == "envcolor") {
                doEnvColor(controller);
                return true;
            }
            if (type == "envadjust" || type == "explodbindtime") {
                // A change to an explod already out, and the bind time of one.
                // Neither is here.
                return true;
            }
            return false;
        case 'r':
            if (type == "reversaldef") {
                doReversalDef(controller);
                return true;
            }
            // `RemapPal` starts with an r, not with the p of the `PalFX` family it
            // looks like it belongs to. Filed under `p` it never ran: every
            // character that recoloured itself was told its controller was not
            // implemented, while the dispatch quietly did nothing.
            if (type == "remappal") {
                doRemapPal(controller);
                return true;
            }
            if (type == "removeexplod") {
                doRemoveExplod(controller);
                return true;
            }
            // `RemoveMappedExplods` is the screenpack's own bookkeeping and no pack in
            // the corpus writes it.
            if (type == "removemappedexplods") return true;
            return false;
        case 'm':
            if (type == "movehitreset") {
                // The contact flags are per-move bookkeeping; resetting them mid-
                // state is how a move re-arms its "did I connect" questions, and
                // with the count reset the armed HitDef may connect again.
                moveContact_ = 0;
                moveHit_ = 0;
                moveGuarded_ = 0;
                hitCount_ = 0;
                // A spent HitDef re-arms: MoveHitReset's reason for existing is
                // that the same attack definition may connect again.
                if (!hitDef_.armed && moveType_ == MoveType::Attack) {
                    hitDef_.armed = true;
                    hitDef_.hitCount = 1;
                }
                return true;
            }
            if (type == "makeanim" || type == "makepalette" || type == "makedust") return true;
            if (type == "modifyexplod") {
                doModifyExplod(controller);
                return true;
            }
            return false;
        case 'd':
            if (type == "destroyself") {
                // A helper is the fight's to remove; this flags it, and the
                // owning host sweeps it at the end of the tick, taking any
                // helpers it spawned along. On a root fighter the controller has
                // no meaning -- a match is made of its two roots -- and MUGEN's
                // own engine only warns there, so the ask is dropped rather than
                // honoured by deleting the fighter.
                if (isHelper_) destroyed_ = true;
                return true;
            }
            if (type == "defencemulset") {
                defenceMul_ = std::max(0.05f, controller.evalNumber("value", *this, 1.0f));
                return true;
            }
            if (type == "displaytoclipboard" || type == "drawgame") {
                return true;
            }
            return false;
        case 'b':
            if (type == "bindtoparent") {
                doBind(controller, 0);
                return true;
            }
            if (type == "bindtoroot") {
                doBind(controller, 1);
                return true;
            }
            if (type == "bindtotarget") {
                doBind(controller, 2);
                return true;
            }
            if (type == "bgpalfx") {
                // The background scope: a grade on the fight's background slot. The
                // fight owns the slot and draws it over the stage; this only fills it.
                if (entities_ != nullptr) entities_->setBgPalFx(readPalFxState(controller));
                return true;
            }
            if (type == "bgm") {
                return true;
            }
            return false;
        case 'g':
            if (type == "gravity") {
                // MUGEN's Gravity controller applies the character's own yaccel
                // for the tick: an air state with `physics = N` that wants normal
                // falling asks for exactly this.
                velY_ -= yAccel_;
                return true;
            }
            if (type == "gamemakeanim") {
                doGameMakeAnim(controller);
                return true;
            }
            return false;
        case 'w':
            if (type == "width") {
                // `Width` temporarily changes the player's width bar. The
                // reference compiles it to four int slots -- edge front/back and
                // player front/back -- preset to zero, and the applier stores all
                // four every run. So an omitted pair means "set to 0", and
                // `value` is the shorthand read only when neither `edge` nor
                // `player` was written (sctrls.html: "This may only be used if
                // the edge and player parameters are not specified"). Player
                // width is additive to `[Size]`, never a replacement; see
                // `widthToward`.
                int edgeFront = 0;
                int edgeBack = 0;
                int playerFront = 0;
                int playerBack = 0;
                if (controller.has("edge") || controller.has("player")) {
                    if (controller.has("edge")) {
                        edgeFront = static_cast<int>(std::lround(
                            firstParam(controller, "edge", *this, 0.0f)));
                        edgeBack = static_cast<int>(std::lround(
                            secondParam(controller, "edge", *this, 0.0f)));
                    }
                    if (controller.has("player")) {
                        playerFront = static_cast<int>(std::lround(
                            firstParam(controller, "player", *this, 0.0f)));
                        playerBack = static_cast<int>(std::lround(
                            secondParam(controller, "player", *this, 0.0f)));
                    }
                } else if (controller.has("value")) {
                    const int front = static_cast<int>(
                        std::lround(firstParam(controller, "value", *this, 0.0f)));
                    const int back = static_cast<int>(
                        std::lround(secondParam(controller, "value", *this, 0.0f)));
                    edgeFront = front;
                    edgeBack = back;
                    playerFront = front;
                    playerBack = back;
                }
                edgeFront_ = edgeFront;
                edgeBack_ = edgeBack;
                playerFront_ = playerFront;
                playerBack_ = playerBack;
                return true;
            }
            return false;
        case 'f':
            // `ForceFeedback` is accepted and ignored on the reference's own
            // authority: the 1.0 docs say it "is not implemented in MUGEN 1.0", so
            // a file that calls it is calling something the reference drops too.
            // (34 packs do, alongside the FallEnvShake beside it.)
            if (type == "forcefeedback") return true;
            if (type == "fallenvshake") {
                doFallEnvShake();
                return true;
            }
            return false;
        default:
            // Everything else: `Trans`, `StopSnd`, `PalCopy` and the rest, which this
            // engine has no picture or speaker for yet (PalFX, RemapPal, BGPalFX and
            // AllPalFX are implemented; PalCopy's table copy is the open seam).
            //
            // These are reported once per character rather than dropped in
            // silence, so a reader of the log can tell the difference between an
            // *effect* that is missing and a *behaviour* that is missing --
            // helpers, projectiles and targets, which are the three the machine
            // genuinely does not do, and the three worth knowing about.
            return false;
    }
}

void StateMachine::doChangeState(const Controller& controller) {
    const int value = static_cast<int>(std::lround(controller.evalNumber("value", *this, -1)));
    if (value < 0) return;

    // MUGEN lets ChangeState force the ctrl flag as well as naming a state.
    // When it does not, the new state's own setting applies. Both answers are
    // forced, not just `1`: `ctrl = 0` is how a character takes control away
    // across a transition into a state whose header grants it, and reading only
    // the true answer made that spelling do nothing. The value is read before
    // the entry, in the state that wrote the controller, as every controller
    // parameter is.
    bool hasCtrl = false;
    bool forceCtrl = false;
    if (controller.has("ctrl")) {
        hasCtrl = true;
        forceCtrl = controller.evalNumber("ctrl", *this, 0.0f) != 0.0f;
    }
    // The optional `anim` (binary pass 6: present means set, absent -1 means keep
    // whatever the statedef header picked).
    const bool forceAnim = controller.has("anim");
    int anim = -1;
    if (forceAnim) {
        anim = static_cast<int>(std::lround(controller.evalNumber("anim", *this, -1)));
    }
    changeState(value, false);
    if (hasCtrl) ctrl_ = forceCtrl;
    if (forceAnim && anim >= 0) setAnim(anim, true);
}

void StateMachine::doChangeAnim(const Controller& controller) {
    const int value = static_cast<int>(std::lround(controller.evalNumber("value", *this, -1)));
    if (value < 0) return;

    // `trigger1 = Anim != 200` is the standard guard, but a character that
    // leaves it off would otherwise restart its own animation on every tick and
    // never play past frame one. Re-setting the animation that is already
    // playing keeps the time instead of resetting it, which is what makes
    // ChangeAnim mean "switch to this" and not "restart this".
    const bool same = value == anim_;
    if (same && !controller.has("elem")) return;

    setAnim(value, true);

    if (controller.has("elem")) {
        // `elem = 3` starts the animation at its third element. Expressed by
        // pushing the animation's clock forward past the elements before it.
        const int elem = static_cast<int>(std::lround(controller.evalNumber("elem", *this, 1)));
        const Animation* animation = this->animation(value);
        if (animation != nullptr && elem > 1 && !animation->frames.empty()) {
            long long skip = 0;
            const size_t upto = std::min(static_cast<size_t>(elem - 1), animation->frames.size());
            for (size_t i = 0; i < upto; ++i) {
                skip += std::max(1, animation->frames[i].duration);
            }
            animTime_ = skip;
        }
    }
}

void StateMachine::doVelocity(const Controller& controller) {
    const std::string& type = controller.type();
    const float x = firstParam(controller, "x", *this, 0.0f);
    const float y = firstParam(controller, "y", *this, 0.0f);

    // The y a character writes is measured the other way up from the y this
    // engine integrates: in the format +y is *down*, so `velset y = 7` is a
    // downward drift and a jump `y = -8` goes up. Both halves of the crossing flip
    // -- see the note on posY() -- and this is the write half. Missing it is not
    // subtle in the end: every character in the roster jumps straight back down
    // through the floor and lands on the tick it took off, and any move that
    // carries itself with `veladd y` climbs instead of falling.
    if (type == "velset") {
        velX_ = worldX(x);
        velY_ = -y;
    } else if (type == "veladd") {
        velX_ += worldX(x);
        velY_ -= y;
    } else {
        // VelMul: a scale, not a displacement, so the x it names multiplies the
        // local velocity and the sign of the factor is what carries. No axis flip
        // on a scale either way.
        velX_ *= x;
        velY_ *= y;
    }
}

void StateMachine::doPosition(const Controller& controller) {
    const std::string& type = controller.type();
    // `x` and `y` are two separate single-valued keys, not one pair:
    // `posadd y = -8` writes one number under `y`, and reading the *second*
    // number of that list returned the fallback -- zero -- so every
    // y-only PosSet/PosAdd/Offset in the corpus silently did nothing.
    const float x = firstParam(controller, "x", *this, 0.0f);
    const float y = firstParam(controller, "y", *this, 0.0f);

    if (type == "offset") {
        // Visual only: a lean, a step the animation does not carry. It does not
        // move the fighter, and nothing else in the machine sees it.
        offsetX_ = x;
        offsetY_ = y;
        return;
    }
    if (type == "posset") {
        // Absolute, measured from the middle of the stage -- characters teleport
        // with `posset x = enemynear, pos x + 10` and mean exactly that. Each
        // axis stands alone: `posset x = 100` moves the fighter sideways and
        // leaves its height where it was.
        if (controller.has("x")) x_ = stageCenter() + x;
        if (controller.has("y")) {
            y_ = -y;
            if (y_ < 0.0f) y_ = 0.0f;
        }
        return;
    }
    // PosAdd is a displacement in the local frame, so a positive x is a step
    // forward whichever way the fighter is looking.
    x_ += worldX(x);
    y_ += -y;
    if (y_ < 0.0f) y_ = 0.0f;
}

void StateMachine::doVariables(const Controller& controller) {
    const std::string& type = controller.type();
    if (type == "varset" || type == "varadd" || type == "varrandom") {
        // `var(n)` and `fvar(n)` are the same controller with different stores,
        // and a character may set either from the same section.
        const bool isFloat = controller.has("fv");
        const int index = static_cast<int>(std::lround(
            controller.evalNumber(isFloat ? "fv" : "v", *this, 0)));
        if (!isFloat && controller.has("sysvar")) {
            const int sysIndex = static_cast<int>(std::lround(
                controller.evalNumber("sysvar", *this, -1)));
            if (sysIndex < 0 || sysIndex >= static_cast<int>(sysVars_.size())) return;
            sysVars_[static_cast<size_t>(sysIndex)] = static_cast<int>(std::lround(
                controller.evalNumber("value", *this, 0)));
            return;
        }
        if (isFloat) {
            if (index < 0 || index >= static_cast<int>(fVars_.size())) return;
            const float value = controller.evalNumber("value", *this, 0.0f);
            if (type == "varadd") {
                fVars_[static_cast<size_t>(index)] += value;
            } else if (type == "varrandom") {
                // `varrandom` picks a value in [0, range), which is how a
                // character varies an effect without a random trigger.
                const int range = std::max(1, static_cast<int>(std::lround(value)));
                fVars_[static_cast<size_t>(index)] =
                    static_cast<float>(nextRandom(aiSeed_) % range);
            } else {
                fVars_[static_cast<size_t>(index)] = value;
            }
            return;
        }
        if (index < 0 || index >= static_cast<int>(vars_.size())) return;
        const int value = static_cast<int>(std::lround(controller.evalNumber("value", *this, 0)));
        if (type == "varadd") {
            vars_[static_cast<size_t>(index)] += value;
        } else if (type == "varrandom") {
            const int range = std::max(1, value);
            vars_[static_cast<size_t>(index)] = nextRandom(aiSeed_) % range;
        } else {
            vars_[static_cast<size_t>(index)] = value;
        }
        return;
    }

    // VarRangeSet sets a span of vars at once.
    const int from = static_cast<int>(std::lround(controller.evalNumber("first", *this, 0)));
    const int last = static_cast<int>(std::lround(controller.evalNumber("last", *this, from)));
    const int value = static_cast<int>(std::lround(controller.evalNumber("value", *this, 0)));
    const bool isFloat = controller.has("fv");
    if (isFloat) {
        for (int i = std::max(0, from);
             i <= last && i < static_cast<int>(fVars_.size()); ++i) {
            fVars_[static_cast<size_t>(i)] = static_cast<float>(value);
        }
    } else {
        for (int i = std::max(0, from); i <= last && i < static_cast<int>(vars_.size()); ++i) {
            vars_[static_cast<size_t>(i)] = value;
        }
    }
}

void StateMachine::doPlaySnd(const Controller& controller) {
    if (sound_ == nullptr) return;
    const std::vector<int> pair = soundPair(controller, "value", *this);
    if (pair.empty()) return;

    // MUGEN's full parameter row: the exclusive channel the sound takes
    // (replaying channel 3 stops whatever channel 3 last had), whether it loops,
    // and volume/pan/frequency shaping on top.
    SndCommand command;
    command.group = pair[0];
    command.sample = pair.size() > 1 ? pair[1] : 0;
    command.channel = static_cast<int>(
        std::lround(firstParam(controller, "channel", *this, -1.0f)));
    command.loop = firstParam(controller, "loop", *this, 0.0f) != 0.0f;
    // `volume` is 0..255 with about 255 the authored level; `pan` is a position
    // in the field, and `abspan` counts from the screen rather than the player --
    // one pan is as useful as the other on a phone speaker, so they share a lane.
    command.volume = std::clamp(firstParam(controller, "volume", *this, 255.0f) / 255.0f,
                                0.0f, 1.5f);
    // pan and abspan share a lane: a value written in position-like magnitudes
    // (128 = centre) maps through the centre offset, a small one is already
    // fractional -1..1.
    float pan = 0.0f;
    if (controller.has("pan")) {
        pan = firstParam(controller, "pan", *this, 0.0f);
    } else if (controller.has("abspan")) {
        pan = firstParam(controller, "abspan", *this, 0.0f);
    }
    command.pan = std::fabs(pan) > 4.0f ? std::clamp((pan - 128.0f) / 128.0f, -1.0f, 1.0f)
                                        : std::clamp(pan, -1.0f, 1.0f);
    command.freqMul = std::clamp(firstParam(controller, "freqmul", *this, 1.0f), 0.01f, 100.0f);
    sound_->playFull(command);
}


// A `trans` value, as the draw path wants it. The six types are the reference's own
// list, and three of them carry a fixed alpha pair, which is the whole reason this
// hands back a `TransParams` rather than a blend mode:
//
//   default  -- does nothing; the caller keeps whatever the animation said
//   none     -- no transparency at all
//   add      -- full additive, the format's `A`
//   add1     -- additive at 256,128, which `.air` spells `A1` and `AS256D128`
//   addalpha -- additive at the `alpha` pair, whose documented default is 256,0
//   sub      -- full subtractive, the format's `S`
//
// `nullopt` is "does nothing", and it is what an unrecognised value returns too:
// MUGEN reports those as "Invalid trans type" and drops them rather than guessing,
// which is the behaviour a typo in a character's file should have.
//
// The `alpha` pair is in MUGEN's own 0..256 units; the caller supplies the defaults
// for its own controller, because they are only documented for one of them.
std::optional<TransParams> parseTransType(std::string_view text, float alphaSrc, float alphaDst) {
    const std::string t = toLower(trim(text));
    if (t.empty() || t == "default") return std::nullopt;
    if (t == "none") return TransParams{Blend::Normal};
    if (t == "add") return TransParams{Blend::Additive};
    if (t == "add1") return TransParams{Blend::Additive, 1.0f, 128.0f / 256.0f};
    if (t == "addalpha") {
        return TransParams{Blend::Additive, std::clamp(alphaSrc, 0.0f, 256.0f) / 256.0f,
                           std::clamp(alphaDst, 0.0f, 256.0f) / 256.0f};
    }
    if (t == "sub") return TransParams{Blend::Subtractive};
    return std::nullopt;
}

// MUGEN's Explod: an animation with a place. It is the single most used visual effect in
// the format -- sparks where a hit lands, dust where a jump takes off, the aura around a
// fighter charging -- and the reason a fight here used to look like two sprites hitting
// each other with no consequences.
void StateMachine::doExplod(const Controller& controller) {
    if (entities_ == nullptr) return;

    Explod explod;
    explod.owner = character_;
    // The spawning machine is kept, not just its character: `ownpal = 0` inherits
    // the owner's palette and the selection can change while the explod lives, so
    // the draw reads palno/RemapPal from here rather than a copied index.
    explod.ownerMachine = this;
    explod.ownerSide = side_;
    explod.id = static_cast<int>(std::lround(firstParam(controller, "id", *this, 0.0f)));
    // `anim = F100` means "animation 100 out of fight.def" rather than out of this
    // character's own file -- see `animTarget`, which is also where the reason this has
    // to be read off the raw text rather than evaluated lives.
    const AnimTarget anim = animTarget(controller, "anim", *this, AnimPrefix::F_IsFightFx);
    explod.anim = anim.anim;
    explod.fromFightFx = anim.fromFightFx;

    const float offsetX = firstParam(controller, "pos", *this, 0.0f);
    // MUGEN measures an explod's y downward from the bind point; this engine keeps height
    // above the floor the other way up, so the sign flips once, here.
    const float offsetY = secondParam(controller, "pos", *this, 0.0f);
    // Cast to int because `statemachine.h` cannot see `entities.h` -- the dependency runs
    // the other way. The value is still a `PostType`.
    resolveEntitySpawn(static_cast<int>(parsePostType(controller.raw("postype"), PostType::P1)),
                       offsetX, offsetY, &explod.x, &explod.y);
    // `random`, before the bind offset is measured off the resolved position and
    // not after: the reference displaces the *bind point*, so the jitter has to
    // be part of where the explod is bound to, not a one-off nudge of where it
    // was drawn once. A bound explod that only jittered on its first tick would
    // snap back to the exact position on the second.
    //
    // The range is the reference's own worked example turned into arithmetic:
    // `random = 40, 80` puts x "between -20 and 19", which is a low of
    // -(rand/2) and a span of rand. (Its example is in MUGEN's frame, where y
    // is down; the jitter is symmetric, so the same number is used here rather
    // than negated -- the difference is which end of the range is short by one,
    // and no effect is drawn differently for it.)
    //
    // The generator is the one the `random` trigger already draws from, and it
    // is deliberately the same stream: MUGEN has one random number generator per
    // player, so an explod asking for jitter and a controller asking for a roll
    // are competing for the same sequence. It answers in 0..1023, which is the
    // scale of its own `random` trigger and comfortably past every range the
    // corpus asks for -- the widest `random` in local_chars is `500,500`, and
    // most are `30,30` or `60,60`. A range wider than 1024 would repeat rather
    // than cover, which is the point at which this wants a wider draw instead.
    const int randX = static_cast<int>(std::lround(firstParam(controller, "random", *this, 0.0f)));
    const int randY = static_cast<int>(std::lround(secondParam(controller, "random", *this, 0.0f)));
    if (randX > 0) {
        explod.x += static_cast<float>(-(randX / 2) + nextRandom(randomSeed_) % randX);
    }
    if (randY > 0) {
        explod.y += static_cast<float>(-(randY / 2) + nextRandom(randomSeed_) % randY);
    }
    explod.bindOffsetX = explod.x - x_;
    explod.bindOffsetY = explod.y - y_;
    // The default bind time is 1 tick, which is the same as not being bound at all -- and
    // is why a stationary explod written without a bindtime stays where it was put.
    explod.bindTime = static_cast<int>(std::lround(firstParam(controller, "bindtime", *this, 1.0f)));
    explod.velX = worldX(firstParam(controller, "vel", *this, 0.0f));
    explod.velY = -secondParam(controller, "vel", *this, 0.0f);
    explod.accelX = worldX(firstParam(controller, "accel", *this, 0.0f));
    explod.accelY = -secondParam(controller, "accel", *this, 0.0f);
    explod.facing = firstParam(controller, "facing", *this, 1.0f) < 0.0f ? -facing_ : facing_;
    // `vfacing`: -1 turns the effect over vertically, about its own axis, and it
    // is an independent choice from `facing` -- a falling droplet that faces away
    // from its owner still has to fall the way round it was drawn. Parsed here
    // and read by `drawExplod`, which is where the mirror happens.
    explod.flipV = firstParam(controller, "vfacing", *this, 1.0f) < 0.0f;
    explod.scaleX = firstParam(controller, "scale", *this, 1.0f);
    explod.scaleY = secondParam(controller, "scale", *this, 1.0f);
    explod.spritePriority =
        static_cast<int>(std::lround(firstParam(controller, "sprpriority", *this, 0.0f)));
    // `removetime`, whose three meanings are three different numbers and not a
    // range: positive is a lifetime, -1 is *for ever*, and -2 -- the documented
    // default -- is "when the animation has played out". Reading the default as -1
    // made the two negatives the same number, so the 392 explods in the corpus that
    // write `removetime = -1` for an aura that should outlive its animation died at
    // the end of it instead. See `Explod::removeTime`.
    explod.removeTime =
        static_cast<int>(std::lround(firstParam(controller, "removetime", *this, -2.0f)));
    // `trans` and its companion `alpha`. The parameter overrides the *animation's*
    // colour-add settings, which is why the absent case is `nullopt` and not a
    // default: "if omitted, does nothing" means the frame's own `A1` still applies.
    // The pair is only consulted for `addalpha`, whose documented default is 256,0.
    explod.trans = parseTransType(controller.raw("trans"),
                                  firstParam(controller, "alpha", *this, 256.0f),
                                  secondParam(controller, "alpha", *this, 0.0f));
    // `ownpal`: sctrls.html defines it as a boolean, describes 1 as giving the
    // explod "its own copy of its palette" so a PalFX on the player does not
    // recolour it, and states "Defaults to 0 if omitted". So 0 is inheritance of
    // the owner's palette, and the bool is read with that same default rather
    // than the controller-level one.
    explod.ownpal = firstParam(controller, "ownpal", *this, 0.0f) != 0.0f;
    // `shadow = r,g,b`: the explod's own floor shadow. Absent is `0,0,0`, which
    // means none; `r == -1` selects the stage colour. See `parseShadowRgb`.
    explod.shadow = parseShadowRgb(controller, "shadow", *this);
    // `ignorehitpause` defaults to 1 on an explod -- the parameter is documented
    // that way and the generic controller-level default of 0 is a different
    // parameter that happens to share the spelling. Reading it as 0 here left
    // every hit spark stopped for the length of the hitpause it exists to fill.
    explod.ignoreHitPause = firstParam(controller, "ignorehitpause", *this, 1.0f) != 0.0f;
    explod.pauseMoveTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "pausemovetime", *this, 0.0f))));
    explod.superMoveTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "supermovetime", *this, 0.0f))));
    // An aura is drawn under its owner and a spark over everything, which MUGEN decides
    // from the explod's own sprite priority against the fighter's.
    explod.behindOwner = explod.spritePriority < spritePriority_;
    // `ontop`, which the reference says "has precedence over sprpriority" -- so it
    // is not one more input to that comparison, it replaces it. An explod that
    // asked for the top layer is not drawn behind its owner whatever its
    // priority says, and every `ontop = 1` explod with a sprpriority below its
    // owner's was being drawn as an aura until this did so.
    explod.onTop = firstParam(controller, "ontop", *this, 0.0f) != 0.0f;
    if (explod.onTop) explod.behindOwner = false;
    // `removeongethit`: the glow goes when the move that made it is interrupted,
    // which is the whole of what the parameter is for. The engine's answer is the
    // owner's own record of having been hit this tick -- see `EntityLayer::update`
    // for why that check sits above the explod's freeze gates rather than under
    // them.
    explod.removeOnGetHit = firstParam(controller, "removeongethit", *this, 0.0f) != 0.0f;

    entities_->spawnExplod(explod);
}

// MUGEN's GameMakeAnim: a fightfx animation at a place, over or under the fighters.
// The reference's own note on it is that it "has been superseded by Explod and is now
// considered deprecated", and it is the last controller the corpus uses that this
// engine did not know at all: 115 uses across 37 of its packs -- more packs than use
// `Projectile`, and more than use `ModifyExplod`. What it costs to support is a
// fraction of Explod's own knobs, because most of them do not exist here: no id, no
// bind time, no lifetime, no scale, no `trans`, no sprite priority.
void StateMachine::doGameMakeAnim(const Controller& controller) {
    if (entities_ == nullptr) return;

    Explod explod;
    explod.owner = character_;
    // Attributed to this machine, not just the character: `numexplod` counts
    // per state owner, and a `GameMakeAnim` is the machine's own creation.
    explod.ownerMachine = this;
    explod.ownerSide = side_;
    // The one thing the reference is explicit about: `value` "specifies the animation
    // number (from fightfx)". There is no spelling here that names the character's own
    // .air, which is the whole of the difference between this and an `Explod` written
    // `anim = F<n>` -- and it is why an animation this character does not own still has
    // somewhere to be found.
    explod.fromFightFx = true;
    explod.anim = static_cast<int>(std::lround(controller.evalNumber("value", *this, 0.0f)));

    // `pos`, "relative to the player axis" -- which is Explod's own default postype and
    // the one case of it that needs no `postype` parameter to say so.
    const float offsetX = firstParam(controller, "pos", *this, 0.0f);
    const float offsetY = secondParam(controller, "pos", *this, 0.0f);
    resolveEntitySpawn(static_cast<int>(PostType::P1), offsetX, offsetY, &explod.x,
                       &explod.y);
    // `random`, "The position of the animation will be displaced in the x and y
    // directions by (different) random amounts. The displacement can be as large as
    // half of `rand_amt`." Note the shape of that: one number, two different draws --
    // unlike Explod's `random = x, y`, and it is not a mistake to read it as one. The
    // arithmetic is Explod's own, on the same per-player stream, and the jitter lands
    // on the bind point for the same reason it does there.
    const int spread =
        static_cast<int>(std::lround(firstParam(controller, "random", *this, 0.0f)));
    if (spread > 0) {
        explod.x += static_cast<float>(-(spread / 2) + nextRandom(randomSeed_) % spread);
        explod.y += static_cast<float>(-(spread / 2) + nextRandom(randomSeed_) % spread);
    }
    explod.bindOffsetX = explod.x - x_;
    explod.bindOffsetY = explod.y - y_;
    // `under`: "If under_flag is 1, the animation is drawn behind the character sprites.
    // Defaults to 0 (draw over characters)" -- the same layer question `sprpriority`
    // answers for an explod, asked with a flag rather than a number.
    explod.behindOwner = firstParam(controller, "under", *this, 0.0f) != 0.0f;
    // The rest is Explod's own default and is left as such: `bindtime` 1 -- written out,
    // because the struct's zero means something else to the layer (`Explod::bindTime`)
    // and a game animation is a spark that stays where it was put -- and `removetime`
    // -2, one pass of the animation, which is what a hit spark does.
    explod.bindTime = 1;
    entities_->spawnExplod(explod);
}

// MUGEN's ModifyExplod. The reference gives it three sentences -- its "syntax is
// basically the same as Explod", that it is "subject to future change", and that "the
// pos and postype parameters must be specified together" -- and the first of those is
// the whole specification: everything Explod can be told at birth, this can tell an
// explod that is already out, except for the things that are only true of a birth.
//
// What it is for, in the corpus, is the one long-lived effect a move re-dresses as it
// goes: a charge aura that changes colour and grows, a beam core that steps through its
// animations, a portal that widens. A character that wants a different effect makes a
// different explod; a character that wants *this* one to change, cannot.
void StateMachine::doModifyExplod(const Controller& controller) {
    if (entities_ == nullptr) return;

    ExplodModify modify;
    modify.owner = character_;
    // `id`, and the id an explod written without one carries is 0, which is what an
    // omitted `id` here therefore asks for.
    modify.id = static_cast<int>(std::lround(firstParam(controller, "id", *this, 0.0f)));

    if (controller.has("anim")) {
        const AnimTarget anim = animTarget(controller, "anim", *this, AnimPrefix::F_IsFightFx);
        modify.anim = anim.anim;
        modify.animFromFightFx = anim.fromFightFx;
    }
    // `pos` and `postype` together, as the reference insists. The pair is the difference
    // between "put it twenty in front of me" and "put it twenty in front of where it
    // already was", and only the first of those is a thing a file can mean.
    if (controller.has("pos")) {
        const float offsetX = firstParam(controller, "pos", *this, 0.0f);
        const float offsetY = secondParam(controller, "pos", *this, 0.0f);
        resolveEntitySpawn(
            static_cast<int>(parsePostType(controller.raw("postype"), PostType::P1)),
            offsetX, offsetY, &modify.x, &modify.y);
        modify.movesPosition = true;
        modify.ownerX = x_;
        modify.ownerY = y_;
    }
    if (controller.has("vel")) {
        modify.velX = worldX(firstParam(controller, "vel", *this, 0.0f));
        modify.velY = -secondParam(controller, "vel", *this, 0.0f);
    }
    if (controller.has("accel")) {
        modify.accelX = worldX(firstParam(controller, "accel", *this, 0.0f));
        modify.accelY = -secondParam(controller, "accel", *this, 0.0f);
    }
    if (controller.has("facing")) {
        modify.facing = firstParam(controller, "facing", *this, 1.0f) < 0.0f ? -facing_ : facing_;
    }
    if (controller.has("vfacing")) {
        modify.flipV = firstParam(controller, "vfacing", *this, 1.0f) < 0.0f;
    }
    if (controller.has("scale")) {
        modify.scaleX = firstParam(controller, "scale", *this, 1.0f);
        modify.scaleY = secondParam(controller, "scale", *this, 1.0f);
    }
    // `sprpriority` and `ontop`, and the layer they imply. The recomputation is the
    // engine's own invariant rather than MUGEN's: an explod is drawn behind its owner
    // when its priority is below the fighter's, and a modification that changed the
    // priority without changing that answer would leave a spark drawn as an aura.
    if (controller.has("sprpriority")) {
        modify.spritePriority =
            static_cast<int>(std::lround(firstParam(controller, "sprpriority", *this, 0.0f)));
        modify.behindOwner = *modify.spritePriority < spritePriority_;
    }
    if (controller.has("ontop")) {
        const bool onTop = firstParam(controller, "ontop", *this, 0.0f) != 0.0f;
        modify.onTop = onTop;
        if (onTop) modify.behindOwner = false;
    }
    if (controller.has("bindtime")) {
        modify.bindTime =
            static_cast<int>(std::lround(firstParam(controller, "bindtime", *this, 1.0f)));
    }
    if (controller.has("removetime")) {
        modify.removeTime =
            static_cast<int>(std::lround(firstParam(controller, "removetime", *this, -2.0f)));
    }
    if (controller.has("trans")) {
        modify.setTrans = true;
        modify.trans = parseTransType(controller.raw("trans"),
                                      firstParam(controller, "alpha", *this, 256.0f),
                                      secondParam(controller, "alpha", *this, 0.0f));
    }
    // `ownpal`, reachable here because "the syntax is basically the same as
    // Explod". An omitted parameter leaves the explod's own policy alone; a
    // written 0 is a real value and switches it back to inheritance.
    if (controller.has("ownpal")) {
        modify.ownpal = firstParam(controller, "ownpal", *this, 0.0f) != 0.0f;
    }
    // `shadow`, optional on the same terms: naming it re-dresses the explod,
    // omitting it leaves its colour alone.
    if (controller.has("shadow")) {
        modify.shadow = parseShadowRgb(controller, "shadow", *this);
    }
    if (controller.has("ignorehitpause")) {
        modify.ignoreHitPause = firstParam(controller, "ignorehitpause", *this, 1.0f) != 0.0f;
    }
    if (controller.has("pausemovetime")) {
        modify.pauseMoveTime = std::max(
            0,
            static_cast<int>(std::lround(firstParam(controller, "pausemovetime", *this, 0.0f))));
    }
    if (controller.has("supermovetime")) {
        modify.superMoveTime = std::max(
            0,
            static_cast<int>(std::lround(firstParam(controller, "supermovetime", *this, 0.0f))));
    }
    if (controller.has("removeongethit")) {
        modify.removeOnGetHit = firstParam(controller, "removeongethit", *this, 0.0f) != 0.0f;
    }
    // The parameters Explod has and this deliberately does not: `random`, which is a
    // displacement of a *spawn* and has no meaning applied to an explod already
    // drifting, and `id`, which is the selector rather than a property. A file that
    // writes either here is writing something this controller has no reading for, and
    // the rest of the block is honoured regardless -- which is also what the reference
    // does with the parameters it does not know.
    entities_->modifyExplod(modify);
}

// MUGEN's RemoveExplod: "Removes all of a player's explods, or just the explods with a
// specified ID number. ... `ID` is the ID number of the explods to remove. If omitted,
// removes all explods owned by the player." Two cases, and no third: there is no id-0
// reading of the omitted form, which is what makes it the way a character clears its
// own screen at the end of a super.
void StateMachine::doRemoveExplod(const Controller& controller) {
    if (entities_ == nullptr) return;
    const bool named = controller.has("id");
    const int id = named
                       ? static_cast<int>(std::lround(firstParam(controller, "id", *this, 0.0f)))
                       : 0;
    entities_->removeExplod(character_, id, !named);
}

// MUGEN's Projectile: a beam, a blast, a fireball. It inherits the owner's armed HitDef,
// which is why a character writes `type = Projectile` inside an attack state and why the
// damage comes out right without repeating it.
void StateMachine::doProjectile(const Controller& controller) {
    if (entities_ == nullptr) return;

    Projectile projectile;
    projectile.owner = character_;
    // Same reason as `Explod`: the palette an `ownpal = 0` projectile inherits is
    // the owner's live selection, not a number copied at spawn.
    projectile.ownerMachine = this;
    projectile.ownerSide = side_;
    projectile.id = static_cast<int>(std::lround(firstParam(controller, "projid", *this, 0.0f)));
    // The three animations take the same one-letter prefix convention an explod's `anim`
    // does: a bare number is the character's own .air, an `F` in front of it is the
    // screenpack's `fightfx.air`. See `animTarget` for why the prefix has to be read off
    // the raw text rather than left to the expression parser -- briefly, a bare `F60`
    // evaluates to 0, and animation 0 is the stand, so an unprefixed read drew the
    // fighter itself as its own fireball.
    const AnimTarget flyAnim = animTarget(controller, "projanim", *this, AnimPrefix::F_IsFightFx);
    projectile.anim = flyAnim.anim;
    projectile.animFromFightFx = flyAnim.fromFightFx;
    const AnimTarget strikeAnim =
        animTarget(controller, "projhitanim", *this, AnimPrefix::F_IsFightFx);
    projectile.hitAnim = strikeAnim.anim;
    projectile.hitAnimFromFightFx = strikeAnim.fromFightFx;
    // `projremanim` falls back on `projhitanim` rather than on nothing, which is what
    // the reference says and what a character file means by naming only one of the two.
    // A move that wrote `projhitanim` for the pose its blast strikes in has already
    // said what the blast looks like when it goes; reading a flat -1 here instead made
    // it vanish instead of playing the animation its author had written for it. The
    // fallback is on the *resolved* hit animation, so a move that wrote neither still
    // gets nothing, and that is the common case rather than the exception. Which file it
    // is in travels with it: `projhitanim = F60` alone means the removal animation is
    // fightfx's 60, not animation 60 of the character's own file.
    if (trim(controller.raw("projremanim")).empty()) {
        projectile.removeAnim = projectile.hitAnim;
        projectile.removeAnimFromFightFx = projectile.hitAnimFromFightFx;
    } else {
        const AnimTarget removalAnim =
            animTarget(controller, "projremanim", *this, AnimPrefix::F_IsFightFx);
        projectile.removeAnim = removalAnim.anim;
        projectile.removeAnimFromFightFx = removalAnim.fromFightFx;
    }
    // `projcancelanim` is the animation a projectile cancelled by another projectile
    // plays (§2.5). It is left at -1 when the file does not name one, because the
    // removal-state rule then falls through to `projremanim` and `projhitanim`
    // itself (§5.3) -- writing the fallback here as well would lose the distinction
    // between "not configured" and "configured to animation 0".
    if (!trim(controller.raw("projcancelanim")).empty()) {
        const AnimTarget cancelAnim =
            animTarget(controller, "projcancelanim", *this, AnimPrefix::F_IsFightFx);
        projectile.cancelAnim = cancelAnim.anim;
        projectile.cancelAnimFromFightFx = cancelAnim.fromFightFx;
    }

    const float offsetX = firstParam(controller, "offset", *this, 0.0f);
    const float offsetY = secondParam(controller, "offset", *this, 0.0f);
    resolveEntitySpawn(static_cast<int>(parsePostType(controller.raw("postype"), PostType::P1)),
                       offsetX, offsetY, &projectile.x, &projectile.y);

    projectile.velX = worldX(firstParam(controller, "velocity", *this, 0.0f));
    projectile.velY = -secondParam(controller, "velocity", *this, 0.0f);
    projectile.accelX = worldX(firstParam(controller, "accel", *this, 0.0f));
    projectile.accelY = -secondParam(controller, "accel", *this, 0.0f);
    projectile.velMulX = firstParam(controller, "velmul", *this, 1.0f);
    projectile.velMulY = secondParam(controller, "velmul", *this, 1.0f);
    // `remvelocity` drives a removal state after its first tick (§5.3). Same axis
    // crossing as `velocity`: the character's +y is down, this engine's height is up.
    projectile.remVelX = worldX(firstParam(controller, "remvelocity", *this, 0.0f));
    projectile.remVelY = -secondParam(controller, "remvelocity", *this, 0.0f);
    projectile.scaleX = firstParam(controller, "projscale", *this, 1.0f);
    projectile.scaleY = secondParam(controller, "projscale", *this, 1.0f);
    projectile.spritePriority =
        static_cast<int>(std::lround(firstParam(controller, "projsprpriority", *this, 0.0f)));
    // `projpriority` defaults to 1 and is kept raw: the projectile-vs-projectile
    // rule compares the value exactly, negatives included (§2.4).
    projectile.priority =
        static_cast<int>(std::lround(firstParam(controller, "projpriority", *this, 1.0f)));
    // No minimum: `projhits = 0` is a real projectile that can never hit a player
    // and still cancels against another projectile (§3.1, §10.3).
    projectile.hitsLeft =
        static_cast<int>(std::lround(firstParam(controller, "projhits", *this, 1.0f)));
    projectile.missTime = static_cast<int>(
        std::lround(firstParam(controller, "projmisstime", *this, 0.0f)));
    // A negative `projmisstime` is a parameter error in the reference, and the
    // value it substitutes is 2 rather than the 0 default (`movl $2, 0x68(%esp)`
    // at 0x481349). Reproduced because it is cheap and observable: the
    // substitute is what the character actually gets.
    if (projectile.missTime < 0) projectile.missTime = 2;
    projectile.removeTime = static_cast<int>(
        std::lround(firstParam(controller, "projremovetime", *this, -1.0f)));
    // `projremove`: default 1, and the hit path is where it is read -- a beam
    // written with `projremove = 0` stays in the world after connecting.
    projectile.removeOnHit = firstParam(controller, "projremove", *this, 1.0f) != 0.0f;
    projectile.edgeBound =
        firstParam(controller, "projedgebound", *this, 40.0f);
    projectile.stageBound =
        firstParam(controller, "projstagebound", *this, 40.0f);
    projectile.lowBound = firstParam(controller, "projheightbound", *this, -240.0f);
    projectile.highBound = secondParam(controller, "projheightbound", *this, 100000.0f);
    // `ownpal`. The reference's Projectile section does not list it, but the
    // corpus writes it on projectiles exactly as on explods, and the documented
    // default of the flag where it is defined is 0 -- inherited, not own -- so
    // that is the reading here too. See `chooseEntityPalette`.
    projectile.ownpal = firstParam(controller, "ownpal", *this, 0.0f) != 0.0f;
    // `projshadow = r,g,b`: `0,0,0` is none; unlike Explod's `shadow`, any
    // component of -1 selects the stage colour (SHADOW-RE §4.5).
    projectile.shadow = parseShadowRgb(controller, "projshadow", *this);
    projectile.facing = facing_;
    // The hits it will deal. Copied now, because by the time it connects the owner will
    // be in some later state with no HitDef armed at all.
    projectile.hit = hitDef_;

    entities_->spawnProjectile(projectile);
}

// MUGEN's Helper: a second fighter running *this* character's states -- a summoned
// partner, the double in a super flash, a beam that needs its own collision and its
// own brain. The state machine cannot build one, because fighters belong to the
// fight, so it resolves everything the controller wrote into a HelperSpec and hands
// it to whoever is listening. With nobody listening (a host test), the spawn is
// dropped exactly as a sound is with no mixer.
void StateMachine::doHelper(const Controller& controller) {
    if (entities_ == nullptr) return;

    HelperSpec spec;
    spec.character = character_;
    spec.spawner = this;
    spec.side = side_;
    spec.helperId =
        static_cast<int>(std::lround(firstParam(controller, "id", *this, 0.0f)));
    std::string name = std::string(trim(controller.raw("name")));
    if (name.size() >= 2 && name.front() == '"' && name.back() == '"') {
        name = name.substr(1, name.size() - 2);
    }
    spec.name = std::move(name);
    spec.facing = firstParam(controller, "facing", *this, 1.0f) < 0.0f ? -facing_ : facing_;
    spec.stateNo = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "stateno", *this, 0.0f))));
    spec.keyCtrl = firstParam(controller, "keyctrl", *this, 0.0f) != 0.0f;
    spec.pauseMoveTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "pausemovetime", *this, 0.0f))));
    spec.superMoveTime = std::max(
        0,
        static_cast<int>(std::lround(firstParam(controller, "supermovetime", *this, 0.0f))));

    const float offsetX = firstParam(controller, "pos", *this, 0.0f);
    const float offsetY = secondParam(controller, "pos", *this, 0.0f);
    resolveEntitySpawn(static_cast<int>(parsePostType(controller.raw("postype"), PostType::P1)),
                       offsetX, offsetY, &spec.x, &spec.y);

    entities_->spawnHelper(spec);
}

// A helper reaching back into the spawner's store: `parentvarset a = ...` writes the
// parent's var, and the add form adds into it. There is deliberately no RootVar*
// spelling here because the parent of a first-generation helper *is* the root, which
// covers everything that is written in the wild.
void StateMachine::doParentVar(const Controller& controller, bool additive) {
    if (parent_ == nullptr || parent_->doomed()) return;
    const bool isFloat = controller.has("fv");
    const int index = static_cast<int>(
        std::lround(controller.evalNumber(isFloat ? "fv" : "v", *this, 0)));
    if (isFloat) {
        if (index < 0 || index >= static_cast<int>(parent_->fVars_.size())) return;
        const float value = controller.evalNumber("value", *this, 0.0f);
        if (additive) {
            parent_->fVars_[static_cast<size_t>(index)] += value;
        } else {
            parent_->fVars_[static_cast<size_t>(index)] = value;
        }
        return;
    }
    if (index < 0 || index >= static_cast<int>(parent_->vars_.size())) return;
    const int value = static_cast<int>(std::lround(controller.evalNumber("value", *this, 0)));
    if (additive) {
        parent_->vars_[static_cast<size_t>(index)] += value;
    } else {
        parent_->vars_[static_cast<size_t>(index)] = value;
    }
}

void StateMachine::doBind(const Controller& controller, int kind) {
    StateMachine* anchor =
        kind == 0 ? parent_ : kind == 1 ? (root_ != this ? root_ : nullptr) : lastTarget_;
    if (anchor == nullptr || anchor->doomed()) return;
    bindTo(anchor,
           static_cast<int>(std::lround(firstParam(controller, "time", *this, 1.0f))),
           static_cast<int>(std::lround(firstParam(controller, "facing", *this, 0.0f))),
           firstParam(controller, "pos", *this, 0.0f),
           secondParam(controller, "pos", *this, 0.0f));
}

void StateMachine::bindTo(StateMachine* anchor, int time, int facing, float offsetX,
                          float offsetY) {
    bindMachine_ = anchor;
    bindTime_ = time;
    bindFacing_ = facing;
    bindOffsetX_ = offsetX;
    bindOffsetY_ = offsetY;
}

// The `Target*` family. All of them act on the machine this one's move last
// connected with, and all of them quietly do nothing when there is none -- which
// is MUGEN's own answer, and the reason a mistimed throw just whiffs instead of
// exploding.
void StateMachine::doTargetOp(const Controller& controller) {
    const std::string& type = controller.type();

    if (type == "targetdrop") {
        // Only the default form (drop everything) is honoured; the excludeID and
        // keepone refinements matter when a move holds several targets, which the
        // single-target bookkeeping here never accumulates.
        lastTarget_ = nullptr;
        return;
    }
    if (type == "targetadd") {
        // Adds the player the `id` names to the move's targets without a hit.
        // Match-wide id lookup is the sink's, since the machine sees only its
        // own family.
        if (entities_ != nullptr) {
            StateMachine* found = entities_->playerById(static_cast<int>(std::lround(
                controller.evalNumber("playerid", *this, -1.0f))));
            if (found != nullptr && found != this && !found->doomed()) lastTarget_ = found;
        }
        return;
    }
    StateMachine* target = lastTarget_;
    if (target == nullptr || target->doomed()) return;

    if (type == "targetstate") {
        const int value =
            static_cast<int>(std::lround(controller.evalNumber("value", *this, -1)));
        if (value >= 0) target->changeState(value, false);
        return;
    }
    if (type == "targetlifeadd") {
        const int amount =
            static_cast<int>(std::lround(controller.evalNumber("value", *this, 0.0f)));
        if (target->life_ > 0) {
            target->life_ = std::max(1, target->life_ + amount);
        }
        return;
    }
    if (type == "targetpoweradd") {
        target->gainPower(
            static_cast<int>(std::lround(controller.evalNumber("value", *this, 0.0f))));
        return;
    }
    if (type == "targetvelset" || type == "targetveladd") {
        const float vx = worldX(firstParam(controller, "x", *this, 0.0f));
        const float vy = -firstParam(controller, "y", *this, 0.0f);
        if (type == "targetvelset") {
            target->velX_ = vx;
            target->velY_ = vy;
        } else {
            target->velX_ += vx;
            target->velY_ += vy;
        }
        return;
    }
    if (type == "targetfacing") {
        const int value = static_cast<int>(
            std::lround(firstParam(controller, "value", *this, 1.0f)));
        // +1 faces the attacker, -1 away from it.
        if (value != 0) target->facing_ = value > 0 ? facing_ : -facing_;
        return;
    }
    if (type == "targetbind") {
        const int time = static_cast<int>(
            std::lround(firstParam(controller, "time", *this, 1.0f)));
        target->bindTo(this, time, 0, firstParam(controller, "pos", *this, 0.0f),
                       secondParam(controller, "pos", *this, 0.0f));
        return;
    }
}


void StateMachine::applyBind() {
    if (bindMachine_ == nullptr) return;
    if (bindTime_ == 0 || bindMachine_->doomed()) {
        bindMachine_ = nullptr;
        return;
    }
    const StateMachine& anchor = *bindMachine_;
    // The same frame conversion resolveEntitySpawn makes: the anchor's facing
    // owns the x sign, and MUGEN's downward y crosses into height above floor.
    x_ = anchor.x_ + static_cast<float>(anchor.facing_) * bindOffsetX_;
    y_ = anchor.y_ - bindOffsetY_;
    if (bindFacing_ > 0) {
        facing_ = anchor.facing_;
    } else if (bindFacing_ < 0) {
        facing_ = -anchor.facing_;
    }
    // A bound fighter is moved, not moving: whatever velocity it carried would
    // fight the anchor for the position and lose a tick later anyway.
    velX_ = 0.0f;
    velY_ = 0.0f;
    if (bindTime_ > 0) --bindTime_;
}

// `type = HitVelSet` puts back the velocity the hit gave this fighter, for the
// components the character asks for: "when the player has been hit, sets the
// desired components of the player's velocity to the appropriate gethit
// velocities", with `x` and `y` as nonzero flags. An omitted flag leaves that half
// of the velocity alone, and the corpus is why that reading and not the other:
// of its 336 uses, 128 name both components and 208 name `x = 1` by itself, which
// is a character asking to be slid along the floor without being lifted off it.
// Neither flag is ever left out entirely.
//
// The velocity written is the *hit's*, which lives in `lastHit_` -- not this
// machine's own armed `hitDef_`, which is what this read before. A victim runs
// this in its own get-hit state and has nothing armed of its own, so reading its
// own HitDef set the component to zero: the reference's own 5030 writes
// `HitVelSet x = 1 y = 1` as its second controller, so every launched victim of a
// character that ships a common state file had the launch velocity wiped off it on
// the tick it arrived and stood still in the air until its hitstun ran out.
void StateMachine::doHitVelSet(const Controller& controller) {
    const float useX = controller.evalNumber("x", *this, 0.0f);
    const float useY = controller.evalNumber("y", *this, 0.0f);
    if (useX != 0.0f) velX_ = lastHit_.pushX;
    if (useY != 0.0f) velY_ = lastHit_.pushY;
}

// The three controllers that spend a fall landing. Each one is what the
// reference's common1.cns calls, and each is a no-op once its effect is spent --
// which is the point, because the reference calls `HitFallDamage` and
// `FallEnvShake` from *two* states each (5100 and 5110) and the two paths through
// the landing run one or the other. Consuming is what makes both paths charge the
// fall exactly once. See the fields' comment in the header.

// `HitFallDamage`: "when the player has been hit and is in a falling state, apply
// damage from the fall (specified in the hitdef) to the player." 88 uses across 35
// packs in local_chars, always from the 5100/5110 landing states.
void StateMachine::doHitFallDamage() { applyFallDamage(); }

// The fall damage and its guard, factored out because `land()` applies the same
// thing as the built-in default. `life_ > 0` both ways: a victim that arrived at
// the floor already knocked out keeps that state rather than being charged again,
// and the pending amount is cleared either way so the file's later call cannot come
// back for it. Whether the charge it *does* apply may be lethal is the HitDef's
// `fall.kill`, which travelled here with the damage.
void StateMachine::applyFallDamage() {
    if (fallDamagePending_ <= 0) return;
    if (life_ > 0) {
        // `fall.kill = 0` on the HitDef that armed this: the fall is charged and
        // the victim is left on one point of life. Same clamp as `kill`, applied
        // to the other way a hit takes life.
        life_ = std::max(fallKillPending_ ? 0 : 1, life_ - fallDamagePending_);
        tookDamage_ = true;
    }
    fallDamagePending_ = 0;
}

// `FallEnvShake`: "shakes the screen using the fall.envshake parameters set by an
// attack. Effective only if GetHitVar(fall.envshake.time) is not zero, and it sets
// GetHitVar(fall.envshake.time) to zero after being executed." The doc's own words,
// and the reason `fallShakeTime_` is the live answer to that key rather than a copy
// of the hit's value. 88 uses across 35 packs -- the same count as `HitFallDamage`,
// because the files write the two together, one shake per charge.
void StateMachine::doFallEnvShake() { applyFallShake(); }

void StateMachine::applyFallShake() {
    if (fallShakeTime_ <= 0) return;
    if (entities_ != nullptr) entities_->shake(fallShakeTime_, fallShakeFreq_, fallShakeAmpl_);
    fallShakeTime_ = 0;
}

// `HitFallVel`: "if the player has been hit and is in a falling state, sets the
// player's velocities to the fall velocities (fall.xvel and fall.yvel) specified
// in the HitDef." No consumption: a velocity is not a quantity that gets used up,
// and the reference calls it once, from 5101 on its first tick. The y is stored
// up-positive (see `setFallLanding`), so it goes in as it stands in this engine's
// frame -- the same sign convention `land()` uses for the built-in bounce.
void StateMachine::doHitFallVel() {
    velY_ = fallVelY_;
    if (fallVelX_ != 0.0f) velX_ = fallVelX_;
}

// Which of the landing effects the character's own file runs. Read once per
// character and cached against the pointer, because two fighters share one library
// and a round reset must not pay for it again.
//
// A file that scripts none of the three keeps the engine's built-in landing, which
// is what a character shipping no common1.cns relies on and what MUGEN itself
// supplies from its own data folder. A file that scripts any of them is taken at
// its word, per effect: the three are independent, and a pack that writes
// `HitFallDamage` but not `HitFallVel` is a pack that wants the engine's bounce and
// its own damage timing.
void StateMachine::scanLanding() {
    if (landingScriptedBy_ == stateCharacter()) return;
    landingScriptedBy_ = stateCharacter();
    scriptedFallDamage_ = false;
    scriptedFallVel_ = false;
    scriptedFallShake_ = false;
    if (stateLibrary() == nullptr) return;
    scriptedFallDamage_ = stateLibrary()->usesController("hitfalldamage");
    scriptedFallVel_ = stateLibrary()->usesController("hitfallvel");
    scriptedFallShake_ = stateLibrary()->usesController("fallenvshake");
}

// Whether the character brings its own guard chain, read from its file the same
// way `scanLanding` reads the landing effects.
//
// The reference's guard states are a *common state file*'s content, and a fighter
// that ships one owns them: all 38 common state files in local_chars define 120,
// 130, 131, 132 and 140, and the two the engine has to write for itself are the
// ones a fighter with no file would otherwise not have. Where the file is present
// the engine keeps its hands off the chain, and that is not a formality. Two packs
// in the corpus prove the point: `mugen_sai` rebinds `holddown` to the X button in
// its own .cmd, so its crouch guard is a button and not the down direction, and
// `mugen_teen_gohan` comments out `trigger2 = !inguarddist` because it wants the
// guard held for as long as back is held. An engine fallback that tested the raw
// stick rather than the character's own command, or that pulled a fighter out of
// its guard whenever no attack was live, would overrule both.
//
// The test is the pair 130 and 140 rather than the whole set: 130 is the state a
// block is *held* in, which is what the chain hangs off, and 140 is where its exit
// goes, so a character with one and not the other has no release to reach and the
// engine's chain would walk it into a state number nothing defines.
void StateMachine::scanGuardChain() {
    if (guardScriptedBy_ == stateCharacter()) return;
    guardScriptedBy_ = stateCharacter();
    scriptedGuardChain_ = false;
    if (stateLibrary() == nullptr) return;
    const auto defines = [this](int number) {
        const StateDef* state = stateLibrary()->find(number);
        return state != nullptr && state->defined;
    };
    scriptedGuardChain_ = defines(CommonState::kGuardStand) && defines(CommonState::kGuardEnd);
}

// The flags a character raises about itself for a tick. See the header for why they are
// per-tick rather than sticky.
void StateMachine::doAssertSpecial(const Controller& controller) {
    const auto apply = [&](std::string_view flag) {
        const std::string name = toLower(trim(flag));
        if (name == "noshadow") assert_ |= kAssertNoShadow;
        else if (name == "globalnoshadow") assert_ |= kAssertGlobalNoShadow;
        else if (name == "invisible") assert_ |= kAssertInvisible;
        else if (name == "noairguard") assert_ |= kAssertNoAirGuard;
        else if (name == "nostandguard") assert_ |= kAssertNoStandGuard;
        else if (name == "nocrouchguard") assert_ |= kAssertNoCrouchGuard;
        else if (name == "nohardcodedkeys") assert_ |= kAssertNoHardcodedKeys;
        else if (name == "nojugglecheck") assert_ |= kAssertNoJuggleCheck;
        else {
            // Everything else -- noshadow's companions, `nowalk`, `nobardisplay`, the
            // `intro`/`roundnotover` family -- is accepted and ignored: the fight does not
            // have the thing it would switch off.
        }
    };
    apply(controller.raw("flag"));
    apply(controller.raw("flag2"));
    apply(controller.raw("flag3"));

    const float time = controller.evalNumber("time", *this, 0.0f);
    if (time > 0.0f) setInvisibleTicks(static_cast<int>(std::lround(time)));
}

// `NotHitBy`: un-hittable for a time. See the header for what this does and does not do.
void StateMachine::doNotHitBy(const Controller& controller) {
    const std::string_view value = controller.raw("value");
    const float time = controller.evalNumber("time", *this, 0.0f);
    const int ticks = time > 0.0f ? static_cast<int>(std::lround(time)) : 1;
    if (!trim(value).empty()) setNotHitTicks(ticks);
}

// `attr = SCA, NA`: first half the class flags (S/C/A), second the type, whose
// letters are N/S/H for strength and A/T/P for attack/throw/projectile. The
// normalised text is kept because `hitdefattr = SCA, NA` compares strings.
struct AttrSpec {
    std::string text;  // e.g. "sca, na", lower-cased, single spaces
    bool isThrow = false;
    bool isProjectile = false;
    // Letter presence as masks, for HitOverride matching: classes S/C/A and the
    // second-part letters (N/S/H strength and A/T/P kind).
    uint8_t classMask = 0;
    uint8_t typeMask = 0;
};

static AttrSpec parseAttr(std::string_view raw) {
    AttrSpec out;
    const std::string text = toLower(trim(raw));
    if (text.empty()) {
        out.text = "s, na";  // MUGEN's effective default
        out.classMask = 1;   // s
        out.typeMask = 1 | 8;  // n, a(ttack)
        return out;
    }
    const size_t comma = text.find(',');
    std::string classes = comma == std::string::npos ? text : text.substr(0, comma);
    std::string type = comma == std::string::npos ? "" : std::string(trim(text.substr(comma + 1)));
    // Class letters, in MUGEN's own order, deduplicated.
    std::string cls;
    for (const char want : {'s', 'c', 'a'}) {
        if (classes.find(want) != std::string::npos) cls.push_back(want);
    }
    if (cls.empty()) cls = "s";
    out.isThrow = type.find('t') != std::string::npos;
    out.isProjectile = type.find('p') != std::string::npos;
    out.text = cls + ", " + (type.empty() ? "na" : type);
    for (const char c : cls) out.classMask |= static_cast<uint8_t>(1 << (c == 's' ? 0 : c == 'c' ? 1 : 2));
    for (const char c : type) {
        switch (c) {
            case 'n': out.typeMask |= 1; break;
            case 's': out.typeMask |= 2; break;
            case 'h': out.typeMask |= 4; break;
            case 'a': out.typeMask |= 8; break;
            case 't': out.typeMask |= 16; break;
            case 'p': out.typeMask |= 32; break;
            default: break;
        }
    }
    return out;
}

// `ReversalDef` arms the capture; `tryReversal` is the other side answering it.
// Mask semantics match HitOverride's: the incoming attr's letters must all be
// inside the reversal's. One capture spends it, like a hit spends a HitDef.
void StateMachine::doReversalDef(const Controller& controller) {
    const AttrSpec attr = parseAttr(controller.raw("reversal.attr"));
    if (attr.classMask == 0 || attr.typeMask == 0) return;

    reversalDef_.classMask = attr.classMask;
    reversalDef_.typeMask = attr.typeMask;
    reversalDef_.pause[0] = std::max(
        1, static_cast<int>(std::lround(firstParam(controller, "pausetime", *this, 0.0f))));
    reversalDef_.pause[1] = std::max(
        1, static_cast<int>(std::lround(secondParam(controller, "pausetime", *this, 0.0f))));
    reversalDef_.armed = true;
}

bool StateMachine::tryReversal(const HitInfo& hit) {
    if (!reversalDef_.armed) return false;
    if ((hit.attrClassMask & reversalDef_.classMask) != hit.attrClassMask) return false;
    if ((hit.attrTypeMask & reversalDef_.typeMask) != hit.attrTypeMask) return false;

    reversalDef_.armed = false;
    moveReversed_ = 1;
    moveContact_ = 1;
    // The capturer freezes as if it were hit -- the wind-up a counter reads -- but
    // takes nothing else: no state, no damage, no velocity.
    setPause(reversalDef_.pause[1]);
    shakeTicks_ = std::max(shakeTicks_, reversalDef_.pause[1]);
    return true;
}

void StateMachine::doHitBy(const Controller& controller) {
    // `HitBy` re-enables hits that a `NotHitBy` turned off. The common form is a
    // NotHitBy/NotHitBy/HitBy sequence in an intro, so this is the other half of it.
    const std::string_view value = controller.raw("value");
    if (!trim(value).empty()) notHitTicks_ = 0;
}

// `HitOverride`: the parry/counter mechanic. A slot says "for the next `time`
// ticks, a hit whose attr is fully inside my mask does not land -- it starts my
// state instead", which is what turns a counter stance's first frames into the
// counter itself. MUGEN's defaults are the binary's: time 1, forceair 0, and an
// unnamed `slot` finds the first free one.
void StateMachine::doHitOverride(const Controller& controller) {
    const AttrSpec attr = parseAttr(controller.raw("attr"));
    if (attr.classMask == 0 || attr.typeMask == 0) return;  // an empty mask catches nothing

    HitOverrideSlot slot;
    slot.classMask = attr.classMask;
    slot.typeMask = attr.typeMask;
    slot.stateNo =
        static_cast<int>(std::lround(controller.evalNumber("stateno", *this, -1.0f)));
    slot.ticksLeft =
        std::max(1, static_cast<int>(std::lround(controller.evalNumber("time", *this, 1.0f))));
    slot.forceAir = controller.evalNumber("forceair", *this, 0.0f) != 0.0f;
    slot.active = true;

    const int wanted =
        static_cast<int>(std::lround(controller.evalNumber("slot", *this, -1.0f)));
    if (wanted >= 0 && wanted < static_cast<int>(hitOverrides_.size())) {
        hitOverrides_[static_cast<size_t>(wanted)] = slot;
        return;
    }
    for (HitOverrideSlot& free : hitOverrides_) {
        if (!free.active) {
            free = slot;
            return;
        }
    }
}

// `EnvShake`: the screen itself shakes, which is what a heavy landing or a super's first
// frame does in MUGEN and what a hit with no camera movement does not.
void StateMachine::doEnvShake(const Controller& controller) {
    if (entities_ == nullptr) return;
    const int time = static_cast<int>(std::lround(firstParam(controller, "time", *this, 0.0f)));
    const float frequency = firstParam(controller, "freq", *this, 60.0f);
    const float amplitude = firstParam(controller, "ampl", *this, 0.0f);
    if (time > 0 && std::fabs(amplitude) > 0.0f) entities_->shake(time, frequency, amplitude);
}

// `Pause` freezes the match around the actor for `time` ticks; `SuperPause` is the
// same freeze with the super theatre around it -- the dimmed screen, the flash
// animation, the sound, the power swing, the victim's softened defence. A second
// pause replaces the first: MUGEN's clock has one hand.
void StateMachine::doPause(const Controller& controller, bool super) {
    if (entities_ == nullptr) return;

    const int time = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "time", *this, 30.0f))));
    const int moveTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "movetime", *this, 0.0f))));
    if (!super) {
        entities_->beginPause(time, this, moveTime);
        return;
    }

    const bool darken = firstParam(controller, "darken", *this, 1.0f) != 0.0f;
    const float p2DefMul = firstParam(controller, "p2defmul", *this, 1.0f);
    entities_->beginSuperPause(time, this, moveTime, darken, p2DefMul);

    power_ = clampPower(
        power_ + static_cast<int>(std::lround(firstParam(controller, "poweradd", *this, 0.0f))));

    // The flash's animation is fightfx's by default and the character's own when it is
    // written `S<n>` -- the `S_IsOwnFile` convention, the same one `sparkno` uses and the
    // opposite of the `Explod` controller's. 416 of the super pauses in this roster name
    // no animation at all and the rest are a mix of both kinds, so guessing either way
    // would have been wrong for a third of them.
    const AnimTarget flashAnim =
        animTarget(controller, "anim", *this, AnimPrefix::S_IsOwnFile);
    if (flashAnim.anim >= 0) {
        // The flash is a fightfx explod that ignores its own pause -- it is the
        // one thing the freeze is *for*.
        Explod flash;
        flash.owner = character_;
        flash.ownerMachine = this;
        flash.ownerSide = side_;
        flash.fromFightFx = flashAnim.fromFightFx;
        flash.anim = flashAnim.anim;
        // pos defaults to the fighter's own axis, in its facing frame.
        resolveEntitySpawn(static_cast<int>(parsePostType(controller.raw("postype"),
                                                          PostType::P1)),
                           firstParam(controller, "pos", *this, 0.0f),
                           secondParam(controller, "pos", *this, 0.0f), &flash.x, &flash.y);
        flash.bindTime = 0;
        flash.facing = facing_;
        flash.spritePriority = 5;
        flash.ignoreHitPause = true;
        flash.removeTime = time;
        entities_->spawnExplod(flash);
    }
    const std::vector<int> sound = soundPair(controller, "sound", *this);
    if (!sound.empty() && sound_ != nullptr) {
        sound_->play(sound[0], sound.size() > 1 ? sound[1] : 0);
    }
    // `unhittable = 1`: the super's tell cannot be interrupted out of the flash.
    if (firstParam(controller, "unhittable", *this, 1.0f) != 0.0f) setNotHitTicks(time);
}

// `EnvColor`: the whole screen washed in one colour for a moment -- the red flash
// on a super kill is one. The draw belongs to the fight; this only sets it.
// `AfterImage`: the motion trail. The knobs the fighter drawing it needs are the
// length of the ghost run and the colour handling; palcolor/palinvertall reach
// the ghosts through the palette path (they are PalFX `color`/`invertall`
// verbatim), with the grey-multiply tint as the fallback for true-colour art.
void StateMachine::doAfterImage(const Controller& controller) {
    const int time = static_cast<int>(
        std::lround(firstParam(controller, "time", *this, 1.0f)));
    if (time == 0) {
        afterImage_.ticksLeft = 0;
        return;
    }
    afterImage_.ticksLeft = std::max(afterImage_.ticksLeft, time);
    afterImage_.length = std::clamp(
        static_cast<int>(std::lround(firstParam(controller, "length", *this, 9.0f))), 1, 60);
    afterImage_.color =
        static_cast<int>(std::lround(firstParam(controller, "palcolor", *this, 1.0f)));
    afterImage_.invert = firstParam(controller, "palinvertall", *this, 0.0f) != 0.0f;
    // `trans = add/sub/none` as everywhere else in the format. The reference's
    // parameter list for AfterImage also carries `alpha` (FINDINGS.md:195, the
    // reader at 0x47bc33), so a trail can name a pair the same way an Explod
    // can, and the same default of (256, 0) applies when it does not.
    const std::optional<TransParams> trans =
        parseTransType(controller.raw("trans"), firstParam(controller, "alpha", *this, 256.0f),
                       secondParam(controller, "alpha", *this, 0.0f));
    // A trail that wrote neither names add, which is the documented default: the
    // absence of a `trans` means "inherit" for a body frame, but AfterImage's own
    // default blend is the additive one.
    afterImage_.trans = trans.value_or(TransParams{Blend::Additive});
}

// `type = Trans`: MUGEN's per-tick override of the animation's own transparency.
//
// "Overrides the player's animation transparency parameters for **current game
// tick**" is the whole of the semantics, and it is a stronger statement than it
// looks: the override lasts one tick and is not remembered, so a character that
// wants a fade writes the controller with a trigger and lets it fire every tick.
// The reference's own example is exactly that --
//
//   [State 0, Fade in]
//   type = Trans
//   trigger1 = time < 256
//   trans = addalpha
//   alpha = time, 256-time
//
// which is why the slot clears at the top of every tick and why `default` clears
// it rather than setting it: `default` means "do nothing", and doing nothing to a
// one-tick slot is to leave it empty.
void StateMachine::doTrans(const Controller& controller) {
    const std::string_view raw = controller.raw("trans");
    transThisTick_ = parseTransType(raw,
                                    firstParam(controller, "alpha", *this, 256.0f),
                                    secondParam(controller, "alpha", *this, 0.0f));
    // The shadow path needs the *type*, not the blend: `add`, `add1` and
    // `addalpha` are all additive TransParams, and only addalpha's destination
    // alpha reaches the shadow (SHADOW-RE §5).
    transAddAlphaThisTick_ = toLower(trim(raw)) == "addalpha";
}

void StateMachine::doEnvColor(const Controller& controller) {
    if (entities_ == nullptr) return;
    const std::vector<float> rgb = listParam(controller, "value", *this);
    const float r = rgb.empty() ? 255.0f : std::clamp(rgb[0], 0.0f, 255.0f);
    const float g = rgb.size() < 2 ? 255.0f : std::clamp(rgb[1], 0.0f, 255.0f);
    const float b = rgb.size() < 3 ? 255.0f : std::clamp(rgb[2], 0.0f, 255.0f);
    const int time =
        static_cast<int>(std::lround(firstParam(controller, "time", *this, 1.0f)));
    const bool under = firstParam(controller, "under", *this, 0.0f) != 0.0f;
    entities_->envColor(r, g, b, time, under);
}

// `PalFX`: the palette grade. The machine keeps the parameters in MUGEN's own
// units (mul 0..256, add/sinadd 0..255, color 0..256); the draw path scales
// them into the shader's 0..1 space as it applies the phase, so what is stored
// here is the effect at FULL strength.
//
// `time` is the whole lifetime story, and it is a hold, not a fade: positive holds
// the grade at full strength for that many ticks and then the slot drops straight
// back to identity, -1 holds it until rewritten, and 0 -- including the parameter's
// absence -- resets the slot, which is the idiom. Sticky across state changes, like
// MUGEN's: PalFX lives on the player, not the state.
void StateMachine::doPalFx(const Controller& controller) {
    palFx_ = readPalFxState(controller);
}

// Reads a PalFX-family controller into a slot. Palette effects come in three scopes --
// PalFX on the fighter, BGPalFX on the background, AllPalFX on the whole screen -- and
// the grade they describe is byte for byte the same; only the slot it lands in differs.
// Everything that is not the slot lives here.
StateMachine::PalFxState StateMachine::readPalFxState(const Controller& controller) const {
    PalFxState state;
    const int time =
        static_cast<int>(std::lround(firstParam(controller, "time", *this, 0.0f)));
    if (time == 0) return state;  // identity: a reset, and a no-op for a new grade
    state.active = true;
    state.time = time < 0 ? -1 : time;
    state.elapsed = 0;

    const std::vector<float> mul = listParam(controller, "mul", *this);
    for (size_t i = 0; i < 3; ++i) {
        state.mul[i] = i < mul.size() ? std::clamp(mul[i], 0.0f, 256.0f) : 256.0f;
    }
    const std::vector<float> add = listParam(controller, "add", *this);
    for (size_t i = 0; i < 3; ++i) {
        state.add[i] = i < add.size() ? std::clamp(add[i], -255.0f, 255.0f) : 0.0f;
    }
    state.invert = firstParam(controller, "invertall", *this, 0.0f) != 0.0f;
    state.color = std::clamp(firstParam(controller, "color", *this, 256.0f), 0.0f, 256.0f);

    // `sinadd = r, g, b, period`: the add term oscillates over the period.
    // This is the part that makes an aura shimmer rather than glow flat.
    state.hasSinAdd = controller.has("sinadd");
    if (state.hasSinAdd) {
        const std::vector<float> sin = listParam(controller, "sinadd", *this);
        for (size_t i = 0; i < 3; ++i) {
            state.sinAdd[i] = i < sin.size() ? std::clamp(sin[i], -255.0f, 255.0f) : 0.0f;
        }
        state.sinPeriod = sin.size() >= 4
                              ? std::max(1, static_cast<int>(std::lround(sin[3])))
                              : 60;
    }
    return state;
}

// The stored grade in the renderer's vocabulary, with the sine term evaluated at the
// slot's own tick counter. `elapsed` is what the phase is read from, so a slot that
// stops aging -- frozen under a hitpause or a super pause -- freezes its shimmer with
// it, which is the whole reason the counter is kept rather than derived from `time`.
PalFxParams StateMachine::palFxParams(const PalFxState& state) {
    PalFxParams fx;
    if (!state.active) return fx;
    for (size_t c = 0; c < 3; ++c) {
        fx.mul[c] = state.mul[c] / 256.0f;
        fx.add[c] = state.add[c] / 255.0f;
    }
    if (state.hasSinAdd) {
        const float phase = std::sin(6.2831853f * static_cast<float>(state.elapsed) /
                                     static_cast<float>(state.sinPeriod));
        for (size_t c = 0; c < 3; ++c) {
            fx.add[c] += state.sinAdd[c] * phase / 255.0f;
        }
    }
    fx.invert = state.invert;
    fx.color = std::clamp(state.color / 256.0f, 0.0f, 1.0f);
    return fx;
}

// One end of a RemapPal pair. The reference writes both as two-item lists,
// `source = src_pal_grp, src_pal_item` and `dest = dst_pal_grp, dst_pal_item`
// (sctrls.html); a one-item list is the legacy spelling the corpus uses when
// the group is the usual 1, so `source = 3` means `{1, 3}`. The parameter type
// stays float because that is how `listParam` evaluates it -- `dest = 1,
// var(55)` is a real line in the corpus.
namespace {
PaletteRef paletteRefParam(const std::vector<float>& values, const PaletteRef& fallback) {
    if (values.empty()) return fallback;
    if (values.size() >= 2) {
        return PaletteRef{static_cast<int>(std::lround(values[0])),
                          static_cast<int>(std::lround(values[1]))};
    }
    return PaletteRef{1, static_cast<int>(std::lround(values[0]))};
}
}  // namespace

// `RemapPal`: re-point what palette this fighter draws in. Every sprite that
// uses the source palette is drawn with the destination instead. The
// documentation names the two ends as palette identities -- the palette node's
// `(group, item)`, not a sprite group -- and requires both palettes to exist
// in the player's sprites. The destination is explicitly an *unmapped*
// palette: its own entry is not applied on top, which is why the lookup below
// is one level rather than a chase.
//
// The map is keyed by source, so a character that runs the controller every
// tick with the same source (which is exactly what the corpus does) does not
// accumulate entries. A sprite whose own palette node is a source is remapped
// too; see Fighter::draw's binding.
void StateMachine::doRemapPal(const Controller& controller) {
    const std::vector<float> sourceValues = listParam(controller, "source", *this);
    const std::vector<float> destValues = listParam(controller, "dest", *this);
    const PaletteRef source = paletteRefParam(sourceValues, selectedPalette());
    const PaletteRef dest = paletteRefParam(destValues, selectedPalette());
    // A pair with a non-positive coordinate names no palette. Ignore it rather
    // than store a rule that can never match a real palette.
    if (source.group <= 0 || source.item <= 0 || dest.group <= 0 || dest.item <= 0) return;

    for (PaletteRemap& remap : palRemaps_) {
        if (remap.source == source) {
            remap.dest = dest;
            return;
        }
    }
    palRemaps_.push_back(PaletteRemap{source, dest});
}

PaletteRef StateMachine::applyPaletteRemap(const std::vector<PaletteRemap>& remaps,
                                           PaletteRef source) {
    for (const PaletteRemap& remap : remaps) {
        if (remap.source == source) return remap.dest;
    }
    return source;
}

// Where an entity's `pos` lands, in this engine's world frame. MUGEN's postypes measure

// from six different places; the two that matter most are p1 (the owner, in its own
// frame) and front/back (the screen edge the owner faces).
void StateMachine::resolveEntitySpawn(int type, float offsetX, float offsetY, float* outX,
                                      float* outY) {
    // `outY` is height above the floor, so MUGEN's downward-positive y is negated once.
    const float down = -offsetY;
    const PostType post = static_cast<PostType>(type);
    switch (post) {
        case PostType::P2: {
            const float otherX = opponent_ != nullptr ? opponent_->x_ : x_;
            const int otherFacing = opponent_ != nullptr ? opponent_->facing_ : -facing_;
            *outX = otherX + static_cast<float>(otherFacing) * offsetX;
            *outY = (opponent_ != nullptr ? opponent_->y_ : y_) + down;
            break;
        }
        case PostType::Front:
        case PostType::Back: {
            // Relative to the screen edge the owner faces. "Front" is the direction it is
            // looking, so a positive x is further that way; "back" is the other, and
            // MUGEN's own behaviour is that a positive x goes toward the middle.
            const float towards = post == PostType::Front ? static_cast<float>(facing_)
                                                          : -static_cast<float>(facing_);
            *outX = stageCenter() + towards * offsetX;
            *outY = down;
            break;
        }
        case PostType::Left:
            *outX = stageCenter() + offsetX;
            *outY = down;
            break;
        case PostType::Right:
            *outX = stageCenter() - offsetX;
            *outY = down;
            break;
        case PostType::P1:
        default:
            *outX = x_ + static_cast<float>(facing_) * offsetX;
            *outY = y_ + down;
            break;
    }
}


void StateMachine::doHitDef(const Controller& controller) {
    // A HitDef stays armed until the state changes, so one section covers a
    // whole attack animation and the collision boxes of the animation decide
    // *which* frames can connect.
    HitDef hit;
    hit.armed = true;
    hit.damage = static_cast<int>(std::lround(firstParam(controller, "damage", *this, 30.0f) *
                                              attackMul_));
    hit.animType = parseAnimType(controller.raw("animtype"));
    hit.groundType = parseHitType(controller.raw("ground.type"));
    // The animation and the hit type for a victim who is not standing on the
    // floor. Both are `-1` until written, so that the defaults -- which point at
    // *other parameters* rather than at constants -- can be applied at hit time.
    {
        const std::string_view airAnim = trim(controller.raw("air.animtype"));
        if (!airAnim.empty()) hit.airAnimType = parseAnimType(airAnim);
        const std::string_view fallAnim = trim(controller.raw("fall.animtype"));
        if (!fallAnim.empty()) hit.fallAnimType = parseAnimType(fallAnim);
        const std::string_view airType = trim(controller.raw("air.type"));
        if (!airType.empty()) hit.airType = parseHitType(airType);
    }

    // `ground.velocity` and `air.velocity` are written in the attacker's frame:
    // `-4` means "backwards", which in world terms is away from the attacker.
    // The push is resolved into world space here, once, so the victim's own
    // frame never enters into it.
    const float groundVX = firstParam(controller, "ground.velocity", *this, -4.0f);
    const float groundVY = secondParam(controller, "ground.velocity", *this, 0.0f);
    const float airVX = firstParam(controller, "air.velocity", *this, groundVX);
    // The y defaults to nothing rather than to the -3 the engine used to carry:
    // the reference is explicit that `air.velocity` "defaults to 0,0 if omitted",
    // and the difference is not academic. `launches` asks whether this move lifts
    // the victim, and a default of -3 (up 3, once the sign is converted) would
    // answer yes for *every* HitDef ever written -- a light jab would take an
    // airborne opponent into a fall instead of a wobble.
    const float airVY = secondParam(controller, "air.velocity", *this, 0.0f);

    hit.pushX = worldX(groundVX);
    // The y of a velocity out of a character's file is written in the screen's
    // frame, where *down* is positive -- the reference is explicit about it, and
    // `ground.velocity = -8,-4` is how every launcher in every character file is
    // written. This engine keeps height above the floor instead, so a y out of a
    // file is negated on the way in, and every other velocity in the engine
    // already does exactly that: `Velocity`, `VelAdd`, `jump`, `fall.yvelocity`
    // and the `vel y` trigger all flip the sign in one place and let the rest of
    // the engine think in one frame.
    //
    // These two were the ones that did not, and the cost was the whole of
    // knockback. `launches` reads a positive y as "off the feet", so a launcher's
    // -4 arrived as a *downward* 4: `launches` stayed false, the victim was given
    // a standing hit state and a velocity into the floor, and no move in any
    // character file could put an opponent in the air.
    hit.pushY = -groundVY;
    hit.airPushX = worldX(airVX);
    hit.airPushY = -airVY;
    hit.hitTime = std::max(
        1, static_cast<int>(std::lround(firstParam(controller, "ground.hittime", *this, 12.0f))));
    hit.airHitTime = std::max(
        1, static_cast<int>(std::lround(firstParam(controller, "air.hittime", *this, 20.0f))));
    hit.slideTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "ground.slidetime", *this, 0.0f))));
    // `yaccel` is read as a plain number and not through the frame flip the
    // velocities above take: the character's own falling states use it as the `y`
    // of a `VelAdd`, and that controller flips its own argument. Flipping it here
    // as well would cancel the two out and make a launched victim rise.
    hit.yaccel = firstParam(controller, "yaccel", *this, 0.35f);
    hit.priority =
        static_cast<int>(std::lround(firstParam(controller, "priority", *this, 4.0f)));
    // The class beside the number, which is the half that decides a tie. It is a
    // *name* -- `priority = 4, Miss` -- so it is read off the raw text rather than
    // evaluated: `secondParam` would hand it to the expression parser, where the
    // bare word comes back as an unknown identifier and evaluates to 0.
    hit.priorityClass = parsePriorityClass(secondComponent(controller.raw("priority")));

    // A hit that drives the victim upwards takes them off their feet, which is
    // the difference between a jab and a launch. MUGEN writes a launch with an
    // air.velocity y that is positive (up).
    hit.launches = hit.airPushY > 0.5f || hit.pushY > 0.5f;

    // The sound of the hit. `hitsound = 5, 0` is played by the engine when the
    // hit lands, not by a controller, because the character cannot know when its
    // own move connects -- `movecontact` exists but nothing in a character's
    // files plays a sound from it. Group 5 is MUGEN's hit bucket and the samples
    // in it are the ones common.snd carries.
    const std::vector<int> hitSound = soundPair(controller, "hitsound", *this);
    if (!hitSound.empty()) {
        hit.hitSoundGroup = hitSound[0];
        hit.hitSoundSample = hitSound.size() > 1 ? hitSound[1] : 0;
    }

    // ------------------------------------------------ the freeze frames
    //
    // `pausetime = attacker, victim`. A stated zero still freezes for one tick:
    // that floor is MUGEN's, not a rounding convenience, and it is why no impact
    // in the format is ever instantaneous.
    const float pauseAttackRaw = firstParam(controller, "pausetime", *this, 0.0f);
    const float pauseVictimRaw = secondParam(controller, "pausetime", *this, 0.0f);
    hit.pause[0] = std::max(1, static_cast<int>(std::lround(pauseAttackRaw)));
    hit.pause[1] = std::max(1, static_cast<int>(std::lround(pauseVictimRaw)));

    // A blocked hit freezes for its own length, defaulting to the same as an
    // unblocked one.
    hit.guardPause[0] = std::max(
        1, static_cast<int>(std::lround(
               firstParam(controller, "guard.pausetime", *this, pauseAttackRaw))));
    hit.guardPause[1] = std::max(
        1, static_cast<int>(std::lround(
               secondParam(controller, "guard.pausetime", *this, pauseVictimRaw))));

    // ------------------------------------------------ what blocking costs
    //
    // The chip damage a block lets through is the *second* component of `damage`
    // -- the reference writes the parameter as `damage = hit_damage, guard_damage`
    // -- and not a parameter of its own. The corpus is unambiguous about it: 1437
    // HitDefs across 101 packs write the pair, and not one writes a `guard.damage`
    // (the spelling this engine used to read, against the documentation and every
    // character ever written, so every block was free). It scales with `attackMul_`
    // exactly as the hit damage beside it does, since both are the same parameter.
    // The second component defaults to 0 when a character writes only the first,
    // which is MUGEN's own default for guard damage.
    hit.guardDamage = static_cast<int>(
        std::lround(secondParam(controller, "damage", *this, 0.0f) * attackMul_));
    hit.guardKill = controller.boolParam("guard.kill", true);
    // The other two thirds of the same question. `kill = 0` is a hit that cannot
    // take the last point of life, and `fall.kill = 0` the same for the fall
    // damage the landing pays -- which is how a character ships a move that
    // juggles the opponent down to nothing *except* dead.
    hit.kill = controller.boolParam("kill", true);
    hit.fallKill = controller.boolParam("fall.kill", true);

    // `guard.hittime` is the blocker's *hit* time and defaults to the attack's own
    // ground hit time, `guard.slidetime` to its ground slide time, and `guard.ctrltime`
    // to `guard.slidetime` -- all of which the binary registers as template-slot
    // defaults rather than hard-coding (`angr_0047da40_hitdef_schema_parse.c:647-676`,
    // FINDINGS pass 17 §17.1). This engine had the first two in the wrong places, on
    // the strength of a comment that asserted MUGEN defaulted `guard.hittime` to the
    // *slide* time. It does not, and the two errors compounded: `guard.slidetime`
    // defaulted to `guard.hittime`, so with both unwritten the pair came out as the
    // slide time twice over and a blocked hit's reported `GetHitVar(hittime)` was the
    // slide clock. The corpus is where that shows: 600 HitDefs write `guard.hittime`
    // and 224 write `guard.slidetime`, against 2,544 that write `pausetime` and
    // inherit these.
    hit.guardHitTime = static_cast<int>(std::lround(
        firstParam(controller, "guard.hittime", *this, static_cast<float>(hit.hitTime))));
    hit.guardSlideTime = static_cast<int>(std::lround(
        firstParam(controller, "guard.slidetime", *this, static_cast<float>(hit.slideTime))));
    hit.guardPushX =
        worldX(firstParam(controller, "guard.velocity", *this, groundVX));

    // `guardflag` lists the state types that can block this attack: M (or H) for
    // mid-height, L for low, A for air. A character that writes `guardflag = MA`
    // is saying its move cannot be blocked by a crouching opponent, which is the
    // whole of what makes crouching a real choice.
    const std::string guardFlag = toLower(trim(controller.raw("guardflag")));
    if (!guardFlag.empty()) {
        hit.guardHigh = guardFlag.find('m') != std::string::npos ||
                        guardFlag.find('h') != std::string::npos;
        hit.guardLow = guardFlag.find('l') != std::string::npos;
        hit.guardAir = guardFlag.find('a') != std::string::npos;
    }

    // `guard.ctrltime`, the time before the blocker regains control. It defaults
    // to `guard.slidetime` rather than to `guard.hittime` directly, and the chain
    // is the reason: `guard.slidetime` already defaults to `guard.hittime`, so
    // both spellings give a character that writes neither the same number, and a
    // character that writes one of them gets what it asked for.
    hit.guardCtrlTime = static_cast<int>(std::lround(firstParam(
        controller, "guard.ctrltime", *this, static_cast<float>(hit.guardSlideTime))));

    // `guard.dist`, the distance at which holding back is enough. It stays -1
    // when the file writes none, because the value it falls back on is a player
    // variable of the *owner* and a projectile's HitDef is resolved long after it
    // left its owner's hands -- see `guardDistanceFor`. A written one needs no
    // conversion: it is a distance in the character's own space, the same frame
    // `size.attack.dist` is written in, and it is compared against one of those
    // rather than folded into a velocity, so there is no facing to flip.
    if (controller.has("guard.dist")) {
        hit.guardDist = static_cast<int>(
            std::lround(firstParam(controller, "guard.dist", *this, 0.0f)));
        // And the player's own live copy, which is the one `inguarddist` actually reads
        // -- see `guardDist()`. A HitDef that does not write the parameter leaves it
        // where it was, which is what the reference's single conditional store does.
        guardDist_ = hit.guardDist;
    }

    // ------------------------------------------------ the air block's own numbers
    //
    // The scaled default is the reference's, and it is not decoration: an air
    // block pushes the blocker back half again as hard as the air velocity would
    // have and drops them half as far, which is what keeps a blocked air attack
    // from either whiffing the blocker out of range or spiking them into the
    // floor. Reading `guard.velocity` here instead -- a *ground* number -- was
    // what this engine did before, and it gave an air-blocking defender the
    // knockback of somebody standing on the floor.
    hit.airGuardVelX = worldX(
        firstParam(controller, "airguard.velocity", *this, airVX * 1.5f));
    hit.airGuardVelY = -secondParam(controller, "airguard.velocity", *this, airVY / 2.0f);
    // `airguard.ctrltime` defaults to `guard.ctrltime`, which is why it is read
    // second and falls back on the value just resolved rather than on a constant.
    hit.airGuardCtrlTime = static_cast<int>(std::lround(firstParam(
        controller, "airguard.ctrltime", *this, static_cast<float>(hit.guardCtrlTime))));

    // ------------------------------------------------ hit while lying down
    //
    // `down.velocity` falls back to `air.velocity` -- the parser's own default in
    // the binary, and the reference says the same -- so a character that writes
    // neither gets the slide it would have got in the air, which is a still
    // victim rather than a flung one. `down.hittime` defaults to 20, which is the
    // one number here the reference does not state and pass 7 read out of the
    // parser.
    hit.downVelX = worldX(firstParam(controller, "down.velocity", *this, airVX));
    hit.downVelY = -secondParam(controller, "down.velocity", *this, airVY);
    hit.downHitTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "down.hittime", *this, 20.0f))));
    hit.downBounce = firstParam(controller, "down.bounce", *this, 0.0f) != 0.0f;

    // `air.juggle`: what this hit costs against the victim's air-combo allowance.
    hit.airJuggle = static_cast<int>(
        std::lround(firstParam(controller, "air.juggle", *this, 15.0f)));

    // `attr`, for the record and for the throw rule. The binary normalises to a
    // lower-case spelling, which is what `hitdefattr` comparisons see.
    const AttrSpec attr = parseAttr(controller.raw("attr"));
    hit.attrText = attr.text;
    hit.attrThrow = attr.isThrow;
    hit.attrProjectile = attr.isProjectile;
    hit.attrClassMask = attr.classMask;
    hit.attrTypeMask = attr.typeMask;

    // `affectteam = F/E/B`. The binary's parser maps the first letter to the
    // team mask and leaves the default 2 (`Enemy`) in place for an omitted or
    // unrecognised spelling (`0x47DA40`); the docs say the same. Only the
    // player-attack-vs-projectile pass reads it so far.
    {
        const std::string team = toLower(trim(controller.raw("affectteam")));
        if (!team.empty()) {
            switch (team.front()) {
                case 'f': hit.affectTeam = 1; break;  // Friend: own team
                case 'e': hit.affectTeam = 2; break;  // Enemy: opponents
                case 'b': hit.affectTeam = 3; break;  // Both: either team
                default: break;                       // default stays Enemy
            }
        }
    }

    // `hitflag`: the victim conditions this attack may touch. The binary's own
    // reading (pass 5): absent means M|A|F and the letters OR in, so a present
    // parameter *replaces* the default rather than adding to it. `+` counts the
    // downed in -- juggle follow-ups are written with it.
    if (controller.has("hitflag")) {
        const std::string flag = toLower(trim(controller.raw("hitflag")));
        hit.hitStand = flag.find('m') != std::string::npos;
        hit.hitAir = flag.find('a') != std::string::npos;
        hit.hitFall = flag.find('f') != std::string::npos;
        hit.hitDown = flag.find('d') != std::string::npos ||
                      flag.find('+') != std::string::npos;
    }

    // The fall family. `fall = 1` knocks a grounded victim down whether or not the
    // velocities do; `air.fall` the same for an airborne one; and `fall.recover`
    // (with its time) is what keeps a fallen victim from teching out of the throw
    // or the slam the author wanted inescapable.
    hit.fall = firstParam(controller, "fall", *this, 0.0f) != 0.0f;
    // `air.fall` is documented as defaulting "to the same value as fall", so the
    // two are one intent spelled twice: a move that knocks a standing victim down
    // knocks an airborne one down as well unless it says otherwise. The reference
    // agrees structurally -- the fall bit is written once, by the `fall` parameter
    // alone (`angr_0047f380_at_00480308.c:305`, `if (sub_439260(1)) a1->field_78 |=
    // 1;`, with no implicit derivation next to the one `forcestand` gets two lines
    // above), while `air.fall` is read inside the air branch where it can only set
    // the flag.
    hit.airFall = controller.has("air.fall")
                      ? firstParam(controller, "air.fall", *this, 0.0f) != 0.0f
                      : hit.fall;

    // `forcestand` and `forcenofall` are the two parameters that *undo* what the
    // reaction would otherwise do, and both are read before the branch that picks
    // it. `hitapply` opens with
    //
    //     if (hitdef->field_78 & 1 || v10) victim->field_10c8 = 1;    // `fall`
    //     else if (hitdef->field_b8)       victim->field_10c8 = 0;    // `forcenofall`
    //
    // and then, inside the grounded branch, `if (hitdef->field_b4 || v10) victim->
    // field_e58 = 1;` directly in front of `state = (0 < field_e58 - 1 ? 5010 :
    // 5000)` (`angr_p9_hitapply.c:261-269`, `:287`). So `forcenofall` clears the
    // fall flag the victim is still carrying, and `forcestand` gives a crouching
    // victim the standing reaction 5000 where the statetype alone would have given
    // them 5010.
    //
    // `forcestand`'s implicit default is why a launcher needs to write nothing at
    // all. The filler writes it when `ground.velocity.y` is non-zero *and the file
    // did not spell `forcestand` out* -- `if (v29 != 0.0f && !a0[101]) a1->field_b4 =
    // 1;` (`angr_0047f380_at_00480308.c:385`) -- where `a0` is the parsed template
    // seen as 32-bit words and slot 101 is the "was written" flag, which the schema
    // parser sets for this one parameter alone: `a1[404] = 1;` beside the value it
    // stores at `a1[380]` (`angr_0047da40_hitdef_schema_parse.c:565-572`), and 404/4
    // is 101. It follows the documentation exactly -- "Normally defaults to 0, but
    // if the y_velocity of the `ground.velocity` parameter is non-zero, it defaults
    // to 1" -- which is what "defaults" means: an explicit `forcestand = 0` wins.
    hit.forceStand = controller.has("forcestand")
                         ? firstParam(controller, "forcestand", *this, 0.0f) != 0.0f
                         : hit.pushY != 0.0f;
    hit.forceNoFall = firstParam(controller, "forcenofall", *this, 0.0f) != 0.0f;
    hit.fallRecover = firstParam(controller, "fall.recover", *this, 1.0f) != 0.0f;
    hit.fallRecoverTime = std::max(
        1, static_cast<int>(std::lround(firstParam(controller, "fall.recovertime", *this, 60.0f))));
    hit.fallDamage = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "fall.damage", *this, 0.0f))));
    hit.fallVelX = firstParam(controller, "fall.xvelocity", *this, 0.0f);
    // Written down-positive like every MUGEN y; stored up-positive like ours.
    hit.fallVelY = -firstParam(controller, "fall.yvelocity", *this, -4.5f);
    hit.fallShakeTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "fall.envshake.time", *this, 0.0f))));
    hit.fallShakeFreq = firstParam(controller, "fall.envshake.freq", *this, 60.0f);
    hit.fallShakeAmpl = firstParam(controller, "fall.envshake.ampl", *this, 0.0f);
    hit.shakeTime = std::max(
        0, static_cast<int>(std::lround(firstParam(controller, "envshake.time", *this, 0.0f))));
    hit.shakeFreq = firstParam(controller, "envshake.freq", *this, 60.0f);
    hit.shakeAmpl = firstParam(controller, "envshake.ampl", *this, 0.0f);
    if (controller.has("snap")) {
        hit.hasSnap = true;
        hit.snapX = firstParam(controller, "snap", *this, 0.0f);
        hit.snapY = secondParam(controller, "snap", *this, 0.0f);
    }
    if (controller.has("mindist")) {
        hit.hasMinDist = true;
        hit.minDistX = std::fabs(firstParam(controller, "mindist", *this, 0.0f));
    }
    if (controller.has("maxdist")) {
        hit.hasMaxDist = true;
        hit.maxDistX = std::fabs(firstParam(controller, "maxdist", *this, 1.0e9f));
    }
    if (controller.has("p2facing")) {
        hit.p2facing = static_cast<int>(
            std::lround(firstParam(controller, "p2facing", *this, 0.0f)));
    }
    // `p1facing`/`p1getp2facing`: the same family, but for the *attacker's* facing
    // once the hit lands. Written as an expression by every character in the corpus
    // that uses it, which is why both are evaluated here like any other parameter.
    hit.p1facing =
        static_cast<int>(std::lround(firstParam(controller, "p1facing", *this, 0.0f)));
    hit.p1GetP2Facing =
        static_cast<int>(std::lround(firstParam(controller, "p1getp2facing", *this, 0.0f)));
    hit.getPower = controller.has("getpower")
                       ? static_cast<int>(std::lround(firstParam(controller, "getpower", *this, -1.0f)))
                       : -1;
    hit.givePower = controller.has("givepower")
                        ? static_cast<int>(std::lround(firstParam(controller, "givepower", *this, -1.0f)))
                        : -1;

    // The chain family. Absent is -1 -- "no restriction" -- and a value below -1
    // is clamped up to -1 rather than kept, so a character cannot write a negative
    // id that no hit will ever carry. `nochainid` is the pair, "nochainid = a, b".
    const auto chainParam = [&](const char* key, float fallback) {
        return controller.has(key)
                   ? std::max(-1, static_cast<int>(std::lround(firstParam(controller, key, *this, fallback))))
                   : -1;
    };
    hit.id = chainParam("id", -1.0f);
    hit.chainId = chainParam("chainid", -1.0f);
    hit.noChainId = controller.has("nochainid")
                        ? std::max(-1, static_cast<int>(std::lround(
                                            firstParam(controller, "nochainid", *this, -1.0f))))
                        : -1;
    hit.noChainId2 = controller.has("nochainid")
                         ? std::max(-1, static_cast<int>(std::lround(
                                             secondParam(controller, "nochainid", *this, -1.0f))))
                         : -1;
    // A HitDef that requires the very id it forbids has asked for something it can
    // never have, so MUGEN resolves the contradiction by clearing all three rather
    // than by picking a winner -- which is why the rule is applied here at parse
    // time and to the whole family at once.
    if (hit.chainId != -1 && (hit.chainId == hit.noChainId || hit.chainId == hit.noChainId2)) {
        hit.chainId = -1;
        hit.noChainId = -1;
        hit.noChainId2 = -1;
    }

    // Custom-state flow: which states contact puts each side in. `p2getp1state`
    // defaults to 1 as soon as `p2stateno` is written -- see the field's note -- and
    // is otherwise meaningless, because a move that names no victim state has no
    // state to take out of anybody's file. So the fallback here is not `false` but
    // "was a `p2stateno` written", which is the same answer the reference's parser
    // arrives at by registering 1 as the pending default on the branch where the
    // name is found and 0 where it is not.
    hit.p1StateNo = controller.has("p1stateno")
                        ? static_cast<int>(std::lround(firstParam(controller, "p1stateno", *this, -1.0f)))
                        : -1;
    hit.p2StateNo = controller.has("p2stateno")
                        ? static_cast<int>(std::lround(firstParam(controller, "p2stateno", *this, -1.0f)))
                        : -1;
    hit.p2GetP1State = hit.p2StateNo >= 0 &&
                       (!controller.has("p2getp1state") ||
                        firstParam(controller, "p2getp1state", *this, 1.0f) != 0.0f);

    // `hitcount`: how many times one HitDef may connect. Defaults to once, which
    // is what stops a multi-frame hitbox hitting on every frame it is out. An
    // author spelling it `numhits` means the same thing -- the binary reads both
    // names into the same slot.
    hit.hitCount = std::max(
        1, static_cast<int>(std::lround(firstParam(controller, "hitcount", *this, 1.0f))));
    if (controller.has("numhits")) {
        hit.hitCount = std::max(
            1, static_cast<int>(std::lround(firstParam(controller, "numhits", *this, 1.0f))));
    }

    // `sparkno = 2` is an animation in the screenpack's fightfx, and `sparkno = S2` is one
    // in the attacker's own file -- the `S` is the whole difference, and it is the
    // opposite way round from what the prefix looks like it should mean.
    const auto sparkTarget = [&](std::string_view raw) {
        const std::string text(trim(raw));
        const bool own = !text.empty() && (text[0] == 'S' || text[0] == 's');
        const std::string number = own ? text.substr(1) : text;
        // A number this does not understand leaves the spark unnamed, which draws nothing
        // rather than drawing animation 0 of the wrong file.
        int value = -1;
        if (!number.empty()) {
            if (const int parsed = std::atoi(number.c_str()); parsed > 0) value = parsed;
        }
        return std::pair<int, bool>{value, !own};
    };
    const auto [sparkNumber, sparkFx] = sparkTarget(controller.raw("sparkno"));
    hit.sparkAnim = sparkNumber;
    hit.sparkFromFightFx = sparkFx;
    const auto [guardSpark, guardFx] = sparkTarget(controller.raw("guard.sparkno"));
    hit.guardSparkAnim = guardSpark;
    hit.guardSparkFromFightFx = guardFx;
    // `sparkxy` is measured from the front of the victim: a negative x is deeper *inside*
    // it, which is why nearly every character writes a negative one.
    hit.sparkX = firstParam(controller, "sparkxy", *this, 0.0f);
    hit.sparkY = secondParam(controller, "sparkxy", *this, 0.0f);

    const std::vector<int> guardSound = soundPair(controller, "guardsound", *this);
    if (!guardSound.empty()) {
        hit.guardSoundGroup = guardSound[0];
        hit.guardSoundSample = guardSound.size() > 1 ? guardSound[1] : 0;
    }

    hitDef_ = hit;
    lastUsedHitDef_ = hit;
}

// ------------------------------------------------------------------- physics

void StateMachine::stepPhysics() {
    // MUGEN auto-turns a grounded, controllable fighter toward its opponent.
    // Attacks and hit states are exempt, which is what makes a reversal a
    // reversal rather than a free re-aim.
    if (opponent_ != nullptr && ctrl_ && stunTicks_ < 0 && moveType_ == MoveType::Idle &&
        physics_ != Physics::Air && y_ <= 0.0f) {
        if (opponent_->x_ > x_ + 0.5f) facing_ = 1;
        else if (opponent_->x_ < x_ - 0.5f) facing_ = -1;
    }

    switch (physics_) {
        case Physics::None:
            // `physics = N` says the *engine* applies nothing -- no gravity and no
            // friction. It does not say the fighter stays where it is put. Velocity
            // still carries position, and a character leaning on that is not being
            // eccentric: vegeta.cns implements its own gravity with `VelAdd y = 7`
            // and waits for `Pos Y > 0` to know it has come back down, so freezing
            // position on `physics = N` leaves a fighter hanging in the air inside
            // its own super until the round ends. Which is what it did.
            if (posFreezeThisTick_) break;
            x_ += velX_;
            y_ += velY_;
            break;

        case Physics::Air: {
            // Gravity, for a corpse as much as for a fighter. There used to be a
            // `life_ > 0` here, on the reading that a dead body should stop
            // moving -- but a death is a *fall* before it is anything else: the
            // reference sends a fighter whose life just ran out to 5050, the
            // falling get-hit state, and 5050 adds `GetHitVar(yaccel)` to its
            // velocity with no `alive` test anywhere in it. Exempting the dead
            // left a knocked-out fighter rising out of the screen at a constant
            // three units a tick, never reaching the floor, and so never reaching
            // the dead state that ends the round.
            //
            // And the fall a *hit* built falls by the hit's own gravity. The
            // falling family -- 5030, 5035, 5040, 5050, 5071 and 5200 -- all
            // write `VelAdd y = GetHitVar(yaccel)` with `physics = N` in their
            // headers; the substitution row here is what those controllers
            // apply, and the number they name is the hit's, carried in
            // `lastHit_`, not the fighter's `movement.yaccel`. The two defaults
            // differ (.35 against .44), so a fall built by a default hit was
            // visibly too heavy next to a mid-air jump, and a hit writing
            // `yaccel = .7` fell no heavier than lightning-weak one that did
            // not -- a ReServe projectile super relies on that heavy fall.
            float pull = yAccel_;
            if (stateNumber_ == CommonState::kHitAirFall ||
                stateNumber_ == CommonState::kHitAirTransition ||
                stateNumber_ == CommonState::kHitAirRecover ||
                stateNumber_ == CommonState::kHitFall ||
                stateNumber_ == CommonState::kHitTripFall ||
                stateNumber_ == CommonState::kHitFallRecover) {
                pull = lastHit_.yaccel;
            }
            velY_ -= pull;
            if (!posFreezeThisTick_) {
                x_ += velX_;
                y_ += velY_;
            }
            if (y_ <= 0.0f && velY_ <= 0.0f) {
                y_ = 0.0f;
                velY_ = 0.0f;
                velX_ = 0.0f;
                // The character's own controllers normally land it -- the
                // common states test `pos y > -vel y` and switch to a landing
                // state. This is the fallback for the ones that do not, and it
                // picks the landing state that matches how the fighter arrived.
                land();
            }
            break;
        }

        case Physics::Stand:
        case Physics::Crouch: {
            y_ = 0.0f;
            velY_ = 0.0f;
            if (!posFreezeThisTick_) x_ += velX_;
            // The knock-back halves of the ground reactions do not use the
            // engine's friction. `[State 5001, 3] VelMul x = .6 / trigger1 =
            // Time >= GetHitVar(slidetime)` (and its twin in 5011) is the whole
            // slide model: the victim keeps the hit's velocity untouched until
            // `slidetime` has passed and then loses 40% of it each tick, which
            // is a long, heavy knock-back rather than the .85 the standing
            // fighter's feet would apply. `slidetime` defaults to 0, so an
            // ordinary hit starts slowing on the entry tick. 5081 writes no
            // `VelMul` at all; its crouching friction is the file's own
            // `physics = C`.
            const bool sliding = stateNumber_ == CommonState::kHitHighKb ||
                                 stateNumber_ == CommonState::kHitLowKb;
            if (sliding && stateTime_ >= lastHit_.slideTime) {
                velX_ *= 0.6f;
                if (std::fabs(velX_) < kFrictionThreshold) velX_ = 0.0f;
            } else if (!sliding) {
                const float friction =
                    physics_ == Physics::Crouch ? kCrouchFriction : kStandFriction;
                velX_ *= friction;
                if (std::fabs(velX_) < kFrictionThreshold) velX_ = 0.0f;
            }
            break;
        }
    }

    // `ScreenBound = 0` releases the edges: a fly-off super's victim travels past
    // the stage walls, and the camera just does not follow it.
    if (screenBound_) x_ = std::clamp(x_, stageMin_, stageMax_);
}

void StateMachine::land() {
    // Feet on the ground, allowance restored. The air-combo limit is a limit on
    // one *combo* -- MUGEN's own pool is per juggle, not per round -- so a fighter
    // that reaches the floor can be launched again.
    if (character_ != nullptr) jugglePoints_ = character_->airJuggle();

    const int arrived = stateNumber_;

    // The fall landing. A fall that owes damage pays it once, here, and a fall
    // with a bounce velocity goes *back up* instead of landing -- the slam-bounce
    // that most launch-into-floor combos are written with. The impact is also
    // where its `fall.envshake` screen shake happens.
    //
    // But only for a character that does not do this itself. MUGEN's engine has no
    // such built-in: the fall landing is *script* -- the reference's common1.cns
    // runs `HitFallDamage`/`HitFallVel`/`FallEnvShake` from states 5100, 5101 and
    // 5110 -- and 27 of the 41 packs in local_chars ship a common1.cns that does
    // exactly that, with their own timings (damage at 5100's time 3, the bounce in
    // 5101 behind a `PosSet y = 20` and a `VelAdd y = .4`). Applying the engine's
    // version on top of a file that scripts its own does not merely double the
    // damage: `land()` bounces at the moment of contact and drains the fall, so
    // the file's 5100 then sees `GetHitVar(fall.yvel) = 0` and skips straight to
    // 5110 -- the whole bounce state, its animation and its lift are lost. So the
    // engine's landing is the *default* common1.cns and stands down when the file
    // brought its own. `scanLanding` is what knows.
    if (arrived == CommonState::kHitAirFall || arrived == CommonState::kHitFall) {
        if (!scriptedFallShake_ && fallShakeTime_ > 0 && entities_ != nullptr) {
            applyFallShake();
        }
        if (!scriptedFallDamage_) applyFallDamage();
        if (!scriptedFallVel_ && velY_ == 0.0f && fallVelY_ > 0.0f && life_ > 0) {
            // fallVelY_ positive here is upward: the bounce. Zeroed so the next
            // arrival lands for good -- a fall bounces once. The x was resolved
            // to world space when the hit was built; it goes in as it stands.
            velY_ = fallVelY_;
            if (fallVelX_ != 0.0f) velX_ = fallVelX_;
            fallVelY_ = 0.0f;
            // Sent to 5101 while it is still rising: 5100 is where a fall
            // arrives, and 5101 is the lift back off it. Entering 5101 rather
            // than 5100 is what makes the two arrivals distinguishable -- the
            // reference holds 5100 for its animation and only then hands to
            // 5101, so a built-in fighter's bounce state is the same one.
            changeState(CommonState::kHitBounce, false);
            return;
        }
    }

    // Which state to land into depends on how the fighter got there: a plain air
    // hit -- knocked away rather than knocked down -- recovers on its feet, a
    // fall reaches the floor, and a fighter that was already on its way down
    // stays there. A death is a fall like any other, and where it ends up being
    // dead is 5110's business rather than this one's.
    int target = CommonState::kLand;
    switch (arrived) {
        // The two states that are *falling* when they reach the ground. Both
        // end at 5100, which is the reference's own `value = 5100 ;Downed
        // get-hit (hit ground from fall)` out of 5050 -- and 5030 reaches it by
        // the same door, because a fall's floor is the same floor whatever the
        // fall was called.
        case CommonState::kHitAirFall:
        case CommonState::kHitFall:
            target = CommonState::kHitGround;
            break;
        // A trip's fall is the exception: `[State 5071, 3] ChangeState / value
        // = 5110` goes straight to lying down, with no 5100 arrival and no
        // bounce, however much fall velocity the hit wrote.
        case CommonState::kHitTripFall:
            target = CommonState::kDowned;
            break;
        // Knocked into the air but *not* falling: this one lands on its feet, and
        // 52 is the reference's own exit for it. Dead, it falls after all --
        // `[State 5040, ...] !Alive -> 5050` -- and 5050's floor is where a
        // death is meant to reach 5100.
        case CommonState::kHitAirRecover:
            target = life_ > 0 ? CommonState::kLand : CommonState::kHitFall;
            break;
        // A transition touching the floor before its own exit fired. The
        // reference would go through 5040/5050 first and then land; the
        // non-fall case's destination (52) is the same either way, and the
        // fall case is 5100.
        case CommonState::kHitAirTransition:
            target = hitFallFlag_ ? CommonState::kHitGround : CommonState::kLand;
            break;
        // The near-ground recovery reaching the floor. The reference gets here
        // through its own `SelfState 5201` ten units under it; the engine's air
        // physics puts the floor where the fight's is and this is the door out of
        // 5200. 5201's own entry state (the hop, the turn, `PosSet y = 0`) is
        // `enterState`'s, so the landing does not have to know about it.
        case CommonState::kHitFallRecover:
            target = CommonState::kHitFallRecoverGround;
            break;
        // The floor itself, reached a second time: that is the bounce coming
        // down, and a fighter that has bounced is done bouncing.
        case CommonState::kHitGround:
        case CommonState::kHitBounce:
            target = CommonState::kDowned;
            break;
        // An air block that reaches the floor, which is the one guard state that
        // asks the ground a question. The reference's own `[State 155, 6]` sends
        // the blocker to the guard hold (130) while it is still holding back
        // inside guard distance, and its `[State 155, 7]` sends everything else
        // to 52 -- the jump-land, on its feet -- which is what the default below
        // does for every other air state. Keeping the guard up on landing is the
        // difference between an air-blocked fighter that lands blocking and one
        // that lands with its arms down.
        //
        // 132 asks its own version of the same question -- `[State 132, 6]` and
        // `[State 132, 7]`, on the identical pair of triggers -- so a fighter
        // that is still holding its guard while it comes down lands in the guard
        // and not in the jump-land.
        case CommonState::kGuardedAirKb:
        case CommonState::kGuardAir:
            target = (holdingBack() && inGuardDist_) ? CommonState::kGuardStand
                                                     : CommonState::kLand;
            break;
        default:
            // Anything else that lands alive is on its feet. A dead one goes to
            // the fall state, which is the same door state 0's `!alive`
            // controller opens, and walks from there to the floor and then to
            // dead like every other death.
            target = life_ > 0 ? CommonState::kLand : CommonState::kHitFall;
            break;
    }
    // If the state the character is in defines its own landing, its controllers
    // have already run this tick and it will have changed state -- reaching
    // here at all means it did not.
    changeState(target, false);
}

void StateMachine::updateAnimation() {
    // The clock always advances; which frame that lands on is `frameAt`'s job,
    // and it is what holds the last frame of an animation whose duration is -1.
    // `animtime` still counts on a held frame, which is what MUGEN does.
    ++animTime_;
}

void StateMachine::setAnim(int number, bool resetTime) {
    anim_ = number;
    if (resetTime) animTime_ = 0;
}

const Animation* StateMachine::animation(int number) const {
    const Character* source = animCharacter();
    if (source == nullptr) return nullptr;
    return source->animation(number);
}

// Which character's file this fighter's art comes from. The ChangeAnim2 pose is the
// narrower of the two overrides -- it borrows an .air for one state -- and a
// `p2stateno` borrow is the wider one, because it brings the states *and* the art.
// When both are set they name the same character anyway: the throw's own file.
const Character* StateMachine::animCharacter() const {
    if (borrowedStateChar_ != nullptr) return borrowedStateChar_;
    return animOverrideChar_ != nullptr ? animOverrideChar_ : character_;
}

const Animation* StateMachine::currentAnimPtr() const { return animation(anim_); }

const Animation* StateMachine::currentAnimation() const { return animation(anim_); }

// `ChangeAnim2`: pose the move's current target out of *this* character's file.
// The throw states that carry a common1.cns write this so the grabbed victim
// shows the attacker's grab poses -- the attacker's frame numbers index the
// attacker's art, which is why the sprite bank follows the animation file.
void StateMachine::doChangeAnim2(const Controller& controller) {
    if (lastTarget_ == nullptr || lastTarget_->doomed()) return;
    const int value = static_cast<int>(std::lround(controller.evalNumber("value", *this, -1)));
    if (value < 0 || character_ == nullptr) return;
    lastTarget_->animOverrideChar_ = character_;
    lastTarget_->setAnim(value, true);
}
int StateMachine::animationFrameIndex() const {
    const Animation* animation = currentAnimPtr();
    if (animation == nullptr) return 0;
    return animation->frameAt(animTime_);
}

int StateMachine::currentElem() const { return animationFrameIndex() + 1; }

// `animtime`: how many ticks are left before the animation ends or loops.
//
// MUGEN counts this one *down*, which is the opposite of what the name and the
// neighbouring `animelemtime` suggest, and getting it backwards is not a small
// error: a character ends its attacks with
//
//     [State 200, end]
//     type = ChangeState
//     trigger1 = AnimTime = 0
//     value = 0
//
// -- "when the animation has finished, go back to standing". Counting up from
// zero makes that trigger fire on the *first* tick of the move instead of the
// last, and every attack in every character is over before it starts. The same
// trigger is how a walk state waits for a turn animation and how a jump state
// waits for its takeoff, so nothing a character does survives the mistake.
long long StateMachine::currentAnimTime() const {
    const Animation* animation = currentAnimPtr();
    if (animation == nullptr || animation->frames.empty()) return 0;

    const size_t frameCount = animation->frames.size();
    const size_t loopBegin = std::min(animation->loopStart, frameCount);

    // Ticks spent in the frames before `loopstart`, which play once.
    long long prefix = 0;
    for (size_t i = 0; i < loopBegin; ++i) {
        prefix += std::max(0, animation->frames[i].duration);
    }

    // The length of the repeating part; negative when a frame in it holds
    // forever, which means the animation never reaches its end.
    long long loopLength = 0;
    for (size_t i = loopBegin; i < frameCount; ++i) {
        const int duration = animation->frames[i].duration;
        if (duration < 0) {
            loopLength = -1;
            break;
        }
        loopLength += std::max(0, duration);
    }

    if (loopLength < 0) return 0;
    if (animTime_ < prefix) {
        // Still in the once-only part: it ends when the whole animation does.
        return std::max(0LL, prefix + loopLength - 1 - animTime_);
    }
    if (loopLength == 0) return 0;
    // Zero on the last tick of the loop, which is the tick the `AnimTime = 0`
    // guard is written for.
    return loopLength - 1 - ((animTime_ - prefix) % loopLength);
}

// Whether the current animation has run out.
//
// This is MUGEN's `AnimTime = 0` and nothing else, because that is the trigger
// every hand-over in the format is written with -- `[State 10, 3] ChangeState 11
// / trigger1 = AnimTime = 0` is verbatim what common1.cns says. `AnimTime` counts
// down and reaches zero at the end of the animation, and for an animation that
// loops it reaches zero at the end of each time round.
//
// That last part is the whole reason this is not written as a separate predicate
// of its own. An earlier version of this function asked "is the animation
// non-looping and past its end?", which sounds like the same question and answers
// `false` forever for a looping animation -- and almost every action in the format
// loops, because all that is needed to loop is the absence of a `LoopStart` line,
// which is the common case. Kung Fu Man's stand-to-crouch action is two elements
// long with no `LoopStart`, so its fighter crouched and never got up, and the same
// test left state 12, the jump takeoff and the landing unable to finish.
bool StateMachine::animationFinished() const { return currentAnimTime() == 0; }

// --------------------------------------------------------------------- hits

bool StateMachine::holdingBack() const { return directionHeld(kCmdBack, CmdKey::kBack); }

void StateMachine::spendGuard(int amount) {
    guardEnergy_ = std::max(0, guardEnergy_ - amount);
    guardRestTicks_ = 0;
}

// MUGEN's `guard` flag: whether this fighter is in a position to block at all.
//
// It is not "am I holding back" -- that is a separate test, and the two together
// are what make a block. A fighter that is mid-move, in hitstun, lying down, or
// already being hit cannot block whatever it does with its thumbs, and one that
// is idle with control on its feet, crouching or in the air can.
bool StateMachine::canGuard() const {
    // A block that has run out of energy is not a block. This is the one place the
    // guard meter changes what happens, rather than merely being drawn.
    if (guardEnergy_ <= 0) return false;
    if (stateType_ == StateType::Liedown) return false;
    // A character can refuse to block at a particular height -- `nostandguard` on a
    // move that is meant to be a commitment, `noairguard` on one that leaves it
    // helpless. Without these, a fighter could block its way out of its own animation.
    if (stateType_ == StateType::Air && asserted(kAssertNoAirGuard)) return false;
    if (stateType_ == StateType::Crouch && asserted(kAssertNoCrouchGuard)) return false;
    if (stateType_ == StateType::Stand && asserted(kAssertNoStandGuard)) return false;
    // A fighter keeps its block through a guard state, which is what the guard
    // states themselves are for: `ctrl` is off while guarding, so the flag cannot
    // be read off it.
    //
    // This has to come *before* the movetype test below, and that is not a
    // detail: the reference's own guard-hit states are `movetype = H` -- a
    // blocked hit is still a hit -- so a test on the movetype sits exactly where
    // a fighter in the guard state 150 or 151 would be refused the next block of
    // a blockstring. The stun test has the same shape: those states are held for
    // `guard.ctrltime` ticks, so "stunned, therefore not guarding" reads as "the
    // middle of a block is not a block". Being *in* one of these states is the
    // answer to the question both tests are trying to ask.
    if (isGuardState(stateNumber_)) return true;
    if (stunTicks_ > 0 || moveType_ != MoveType::Idle) return false;
    return ctrl_ || stateNumber_ == CommonState::kLand;
}

// Whether the character wrote a `[Statedef]` header for the state it is in.
//
// The distinction decides who ends a hit reaction. A character that ships its own
// get-hit states is the one that decides: its `common1.cns` hands over on
// `HitOver`, on `AnimTime = 0`, or on the landing. A character that does not --
// Kung Fu Man is the reference one -- is played by the engine's built-in table
// instead, and that table has no opinion about when a reaction should end, so the
// engine supplies one.
bool StateMachine::ownsCurrentState() const {
    if (stateLibrary() == nullptr) return false;
    const StateDef* state = stateLibrary()->find(stateNumber_);
    return state != nullptr && state->defined;
}

// The states a block lives in. MUGEN numbers them; a character that ships a
// common state file defines them, and one that does not gets the engine's. The
// knock-back states count: a fighter holding back through 151 is blocking, not
// standing around, and the guard entry above must not fire out of one of them.
//
// 140 (`Guard (end)`) is deliberately *not* one of them. It is the pose a block
// leaves through, not one it is held in: the reference gives it `ctrl = 1` so
// the fighter has its control back on the first tick of the release, and a
// fighter that presses back again while the arms are still coming down must be
// able to re-guard -- that re-entry is the blockstring bob, and nothing in the
// character's own file can supply it, because the guard entry itself is the
// engine's. Its own hand-over (the anim running out to 0, 11 or 51) is what
// carries it back to an idle state when the fighter does nothing.
bool StateMachine::isGuardState(int state) {
    return state == CommonState::kGuardStart || state == CommonState::kGuardStand ||
           state == CommonState::kGuardCrouch || state == CommonState::kGuardAir ||
           state == CommonState::kGuarded ||
           state == CommonState::kGuardHit || state == CommonState::kGuardedAir ||
           state == CommonState::kGuardedKb || state == CommonState::kGuardHitKb ||
           state == CommonState::kGuardedAirKb;
}

float StateMachine::guardDistanceFor(const HitDef& hit, const Character* owner, bool projectile) {
    // A file that writes one has said what it means, including the 0 that most
    // of them write.
    if (hit.guardDist >= 0) return static_cast<float>(hit.guardDist);
    // Otherwise the player variable for the case. `sctrls.html` only says
    // "defaults to the value in the player variables" and leaves the name to be
    // worked out, but the pair of names in `[Size]` -- `attack.dist` 160 and
    // `proj.attack.dist` 90 -- is that answer twice over: `trigger.html` lists
    // both as `Const` names, `tutorial3.html` groups them under "You can leave
    // the next 3 unchanged" beside the size of the player, and the packs that
    // carry comments on them say what they are for, in this order:
    //
    //   attack.dist      = 160  ; the minimum distance at which the opponent
    //                           ; enters guard posture
    //   proj.attack.dist = 90   ; ...against your own Projectile
    //
    // which is `trigger.html`'s own two-case sentence for `InGuardDist`: "within
    // guarding distance of an opponent's physical or projectile attack".
    //
    // The reference settles the same question twice more, from the other side: its
    // `[Size]` reader hard-codes 160 and 90 as the fallbacks for the two keys, and the
    // two places that give this parameter a value -- the state change that seeds it and
    // the projectile spawn -- are exactly where those two constants are read. Only the
    // projectile half is left here, because the direct half no longer goes through a
    // HitDef at all: `guardDist_` holds the resolved number for the whole player.
    const float fallback = projectile ? 90.0f : 160.0f;
    if (owner == nullptr) return fallback;
    return owner->constant(projectile ? "size.proj.attack.dist" : "size.attack.dist", fallback);
}

bool StateMachine::hitMayReach(const StateMachine& victim) const {
    if (!hitDef_.armed) return false;
    // The reach window: mindist/maxdist gate on axis distance, a move's real
    // "range" being its art's boxes. Written as x,y pairs; the x is the gate.
    if (hitDef_.hasMinDist || hitDef_.hasMaxDist) {
        const float gap = std::fabs(victim.x_ - x_);
        if (hitDef_.hasMinDist && gap < hitDef_.minDistX) return false;
        if (hitDef_.hasMaxDist && gap > hitDef_.maxDistX) return false;
    }
    return hitMayReach(hitDef_, victim);
}

bool StateMachine::hitMayReach(const HitDef& def, const StateMachine& victim) {
    if (!victim.alive()) return false;
    // The chain gate. `chainID` asks whether this victim's last hit carried that
    // id, and `nochainID` refuses the hit outright when it did. Both are asked of
    // the *victim's* history rather than the attacker's move, which is the point:
    // the gate is about what has already happened to the fighter being hit.
    //
    // A HitDef that contradicts itself has already had the whole family cleared to
    // -1 at parse time, so nothing here needs to re-check the tie.
    if (def.chainId != -1 && victim.lastHitById_ != def.chainId) return false;
    if (def.noChainId != -1 && victim.lastHitById_ == def.noChainId) return false;
    if (def.noChainId2 != -1 && victim.lastHitById_ == def.noChainId2) return false;
    // Lying on the floor is its own letter, and the default MAF excludes it: a
    // fighter already down is not juggled forever unless the move is written to.
    const bool downed = victim.stateType_ == StateType::Liedown;
    if (downed) return def.hitDown;
    const bool air = victim.y_ > 0.5f || victim.stateType_ == StateType::Air;
    if (air) {
        // Falling counts separately: a victim already launched needs the F.
        const bool falling = victim.moveType_ == MoveType::Hit;
        return falling ? def.hitFall : def.hitAir;
    }
    return def.hitStand;
}

StateMachine::HitInfo StateMachine::hitInfoFor(const StateMachine& victim) const {
    HitInfo info = hitInfoFor(hitDef_, facing_, victim, character_);
    // The snap needs to know where the attacker's axis is; the static path used
    // by projectiles leaves it zero, and a projectile that snaps reads as one
    // that does not.
    info.attackerX = x_;
    return info;
}

StateMachine::HitInfo StateMachine::hitInfoFor(const HitDef& hit, int attackerFacing,
                                               const StateMachine& victim,
                                               const Character* attackerCharacter) {
    HitInfo info;
    const bool victimAir = victim.y_ > 0.5f || victim.stateType_ == StateType::Air;
    // Lying on the floor is its own condition, not a flavour of standing: the
    // reaction is chosen from `down.*` and every other pair is skipped. `air` is
    // false here even though the fighter may be at height zero -- state type is
    // the authority on whether a fighter is down.
    const bool victimDown = victim.stateType_ == StateType::Liedown;

    // Whether the victim is already in a fall -- which is a question about the
    // state they are in rather than about the hit that is landing now, and the one
    // the reference asks as `if (hitdef->field_78 & 1 || v10) ... = 1; else if
    // (hitdef->field_b8) ... = 0;` where the value it writes or leaves alone is the
    // victim's own fall flag. `hitFall()` is this engine's latch of the same thing
    // (gated on the victim still being in a hit state), so a hit that says nothing
    // inherits it, and `forcenofall = 1` is the hit that takes it away.
    const bool carriedFall = victim.hitFall() && !hit.forceNoFall;

    // Which animtype a hit uses is not the HitDef's `animtype` unless the victim
    // is on the ground and stays there. `air.animtype` covers a victim in the air,
    // and `fall.animtype` a victim who is falling -- either already, or as of this
    // hit. Neither defaults to a constant: `air.animtype` defaults to `animtype`,
    // and `fall.animtype` to `air.animtype` when that is Up or DiagUp and to Back
    // otherwise.
    //
    // The defaults are read off the binary rather than off the documentation,
    // because the two disagree in a way that shows. The parameter's own page says
    // "Defaults to Up if air.animtype is Up, or Back otherwise", but the parser
    // carries DiagUp through as well -- `if (a1[760] == 4) a1[424] = 4; else if
    // (a1[760] == 5) a1[424] = 5; else a1[424] = 3;` (`sub_47da40`, the HitDef
    // template parser) -- so a DiagUp air reaction falls DiagUp and not Back.
    //
    // This is the resolution the reference does as the hit lands, and it is worth
    // doing for one reason: `GetHitVar(animtype)` is the single most used trigger
    // in the whole hit family -- 990 uses across 38 characters -- and it is what a
    // common state file picks its reaction animation with. Reporting the ground
    // animtype for a juggle is the difference between a victim who flips
    // backwards off a launcher and one who takes a jab to the face in mid-air.
    const int airAnim = hit.airAnimType >= 0 ? hit.airAnimType : hit.animType;
    const int fallAnim = hit.fallAnimType >= 0 ? hit.fallAnimType
                                               : (airAnim == 4 || airAnim == 5 ? airAnim : 3);
    const bool falling = victimAir && victim.moveType_ == MoveType::Hit;
    // A victim this very hit is about to knock into the air takes the fall
    // animtype too, and that is not an extrapolation: the reference picks the
    // victim's animtype in the same place it decides whether the hit is a
    // knockdown, and its knockdown flag is the same condition this engine uses to
    // choose the fall reaction below (`ground.velocity`'s y being non-zero, or
    // `fall`). A standing victim hit by a launcher is airborne by the time the
    // animtype is read, so reading only their *pre-hit* state would give them a
    // ground reaction for an air launch.
    const bool launchesNow = !victimAir && (hit.pushY != 0.0f || hit.fall);

    info.damage = hit.damage;
    info.attackerFacing = attackerFacing;
    info.animType = (falling || launchesNow) ? fallAnim : (victimAir ? airAnim : hit.animType);
    info.groundType = hit.groundType;
    // `air.type` is the same choice made for a victim in the air, and it defaults
    // to the ground one rather than to a constant of its own.
    info.airType = hit.airType >= 0 ? hit.airType : hit.groundType;
    info.attackerPause = hit.pause[0];
    info.victimPause = hit.pause[1];
    // How long the victim's knockback slide lasts, which `GetHitVar(slidetime)`
    // reports and the character's own knocked-back states read to know when to
    // start slowing it down (`VelMul ... trigger1 = Time >= GetHitVar(slidetime)`
    // in the reference's 5001). It is the *hit's* number and not the guard's; the
    // block branch below overwrites it with `guard.slidetime`, which is the one
    // case where the two differ.
    info.slideTime = hit.slideTime;
    // The hit's own downward acceleration, written down-positive as the character
    // wrote it: `GetHitVar(yaccel)` goes straight into the `y` of its `VelAdd`, and
    // that controller is what flips it into this engine's frame.
    info.yaccel = hit.yaccel;
    info.guardSoundGroup = hit.guardSoundGroup;
    info.guardSoundSample = hit.guardSoundSample;
    info.juggleCost = hit.airJuggle;
    info.attrText = hit.attrText;
    info.attrClassMask = hit.attrClassMask;
    info.attrTypeMask = hit.attrTypeMask;
    info.fallRecover = hit.fallRecover;
    info.fallRecoverTime = hit.fallRecoverTime;
    // The same number, as the clock it describes: `fallRecoverTime` is what
    // `GetHitVar(fall.recovertime)` reports back and stays the file's number,
    // while this is what `step()` counts down from the tick of the hit. Armed
    // here, at the one place a HitInfo is built, so that both paths into
    // `lastHit_` carry it.
    info.fallRecoverTicks = info.fallRecoverTime;
    info.fallDamage = hit.fallDamage;
    info.fallVelX = static_cast<float>(attackerFacing) * hit.fallVelX;
    info.fallVelY = hit.fallVelY;
    info.fallShakeTime = hit.fallShakeTime;
    info.fallShakeFreq = hit.fallShakeFreq;
    info.fallShakeAmpl = hit.fallShakeAmpl;
    info.shakeTime = hit.shakeTime;
    info.shakeFreq = hit.shakeFreq;
    info.shakeAmpl = hit.shakeAmpl;
    info.p2facing = hit.p2facing;
    info.getPower = hit.getPower;
    info.givePower = hit.givePower;
    info.id = hit.id;
    info.kill = hit.kill;
    info.guardKill = hit.guardKill;
    info.fallKill = hit.fallKill;
    info.priorityClass = hit.priorityClass;
    info.hasSnap = hit.hasSnap;
    info.snapX = hit.snapX;
    info.snapY = hit.snapY;

    // ---------------------------------------------------------------- blocking
    //
    // A hit only gets blocked if the defender was *holding back*, could guard at
    // all, and the attack's `guardflag` admits the height the defender is at. That
    // last test is the whole point of the parameter: a move written
    // `guardflag = MA` cannot be blocked from a crouch, so an opponent who has
    // learned to block high has to stand up to do it. And a throw cannot be
    // blocked at all, whatever the height -- its attr says so.
    const bool holdingBack = victim.holdingBack();
    const bool heightAllowed = victimAir ? hit.guardAir
                                         : victim.stateType_ == StateType::Crouch ? hit.guardLow
                                                                                  : hit.guardHigh;
    const bool guarded =
        !hit.attrThrow && victim.canGuard() && holdingBack && heightAllowed;

    if (guarded) {
        // A block is not a hit with the numbers turned down: it is its own
        // reaction. The blocker keeps their feet, is pushed away rather than
        // carried, and is left standing in a guard state with the attacker's own
        // freeze to match.
        info.guarded = true;
        info.damage = hit.guardDamage;
        if (victimAir) {
            // An air block has its own velocity and its own clock, and neither is
            // the ground guard's. The velocity defaults to the *air* velocity
            // scaled -- x by 1.5, y by a half -- and the clock to `guard.ctrltime`
            // through `airguard.ctrltime`. Using the ground pair here, as this did
            // before, shoved an airborne blocker by a standing blocker's amount.
            info.pushX = hit.airGuardVelX;
            info.pushY = hit.airGuardVelY;
            // Only the ctrl clock is written on the air path. `+0x1094` is not,
            // so the blocker's hitstun is whatever it already was -- `-1` for a
            // blocker who was not already stunned -- and `hitover` is true
            // throughout. Seeding it from `airguard.ctrltime`, as this did,
            // handed `GetHitVar(hittime)` a number the reference never writes.
            info.keepStunClock = true;
            info.ctrlTime = hit.airGuardCtrlTime;
        } else {
            info.pushX = hit.guardPushX;
            info.pushY = 0.0f;
            // The blocker's hitstun is `guard.hittime`, not `guard.ctrltime`. The two
            // are separate clocks in the reference and both are reported to the
            // character -- `sub_46E7C0` feeds the victim's `+0x1094` from the active
            // HitDef's `+0x170` (`guard.p2hittime`) and its `+0x109C` from `+0x178`
            // (`guard.p2ctrltime`), and `GetHitVar(hittime)` reads the former. Seeding
            // both from the control clock, as this did, told every blocked victim that
            // its hitstun was its control time, so a character that branches on
            // `GetHitVar(hittime)` after a block read the wrong number.
            info.hitTime = hit.guardHitTime;
            info.ctrlTime = hit.guardCtrlTime;
        }
        info.slideTime = hit.guardSlideTime;
        if (victimAir) {
            // `+0x1098` is not written on the air path either, so the blocker
            // keeps the previous hit's slidetime. A character that branches on
            // `GetHitVar(slidetime)` in an air block therefore reads the number
            // of the hit before it, which is what the reference's write-once
            // constant leaves there; the alternative -- the guard slidetime --
            // is a value the reference never puts in that slot.
            info.slideTime = victim.lastHit_.slideTime;
        }
        info.attackerPause = hit.guardPause[0];
        info.victimPause = hit.guardPause[1];
        info.stateNumber = victimAir ? CommonState::kGuardedAir
                                     : victim.stateType_ == StateType::Crouch
                                           ? CommonState::kGuardHit
                                           : CommonState::kGuarded;
        // A block makes its own spark -- a flat flash rather than a starburst -- and only
        // if the character named one. One that did not makes no spark at all when it is
        // blocked, which is what it meant by naming only a hit spark.
        info.sparkAnim = hit.guardSparkAnim;
        info.sparkFromFightFx = hit.guardSparkFromFightFx;
        info.sparkX = hit.sparkX;
        info.sparkY = hit.sparkY;
        return info;
    }

    if (victimAir) {
        info.air = true;
        info.pushX = hit.airPushX;
        info.pushY = hit.airPushY;
        info.hitTime = hit.airHitTime;
        // An airborne victim is always hit into 5020, and this engine used to send
        // the ones it thought were falling straight past it to 5030 on the strength
        // of an upward `air.velocity` alone. The reference's air branch writes the
        // number before it looks at anything else --
        //     v12->field_c4c = 5020;
        //     if (v11->field_13c == 3) v12->field_c4c = 5070;
        // (`docs/mugen-re/decompiled/angr_p9_hitapply.c`, the `LABEL_46ddde` block,
        // which is `air.type` at `+0x13c`) -- and 5070 on `air.type = 3` is the only
        // override. The fall flag is *not* consulted: it was already resolved above
        // the branch, and all it does here is choose between `air.animtype` and
        // `fall.animtype`.
        //
        // Skipping 5020 is not a missing pose, it is a missing decision, and the
        // same one the ground branch had: 5020 is the *air shake*, and it is the
        // state that hands the victim on to 5030 (`[State 5020, 5] ChangeState
        // trigger1 = HitShakeOver value = 5030`). Sending a victim past it made
        // every character's own `[Statedef 5020]` dead code, and left the launch
        // with no shake at all -- the freeze covered for it, and then the victim
        // was already in 5030 as the freeze ended.
        // The flag has four sources and this line takes all of them. `fall` is
        // first because the reference's prelude tests it first --
        // `if (v11->field_78 & 1 || v10) v12->field_10c8 = 1;`, and that prelude
        // runs *above* the statetype branch, so it decides the flag for an
        // airborne victim exactly as it does for a grounded one. `air.fall` is
        // the air-only spelling of the same intent (now defaulted from `fall`
        // where it was parsed). `launches` is this engine's own term: it reads an
        // upward velocity as "off their feet", which is how 1,466 HitDefs in the
        // corpus write a launch -- an upward `air.velocity` and nothing else. The
        // reference's second term (`v10`) is supplied by its caller and its exact
        // computation is still open, but what it must mean is settled by what it
        // does: it sets the fall flag *and* forces the victim's state up to the
        // standing one, which is precisely what a launch is, and precisely what
        // `forcestand`'s implicit default (`ground.velocity.y` non-zero) exists to
        // cover. So `launches` stands in for it and the reading is a good deal
        // stronger than it was; see the roadmap for what is left open.
        //
        // `carriedFall` is the fourth and it is the prelude's `else` branch, which
        // writes nothing: a hit that neither knocks the victim down nor says
        // `forcenofall` leaves the fall state where it was. That is the whole of
        // what `forcenofall` is for -- "force P2 out of a fall state, if he is in
        // one" -- and without the carry a victim juggled out of a knockdown got up
        // off the floor in mid-air, because the flag they were carrying was
        // recomputed as false by the second hit.
        info.falls = hit.fall || hit.airFall || hit.launches || carriedFall;
        info.stateNumber =
            info.airType == 3 ? CommonState::kHitTrip : CommonState::kHitAir;
    } else {
        // A victim on the ground is hit into one of the reference's two ground
        // reactions, and which one is the *victim's* posture and not the attack's
        // height. The two are different questions. `ground.type` says where the
        // attack lands, and what it is read for is the *animation*: the reference's
        // own `[Statedef 5000]` opens with `[State 5000, 1] ChangeAnim / trigger1 =
        // Time = 0 / value = ifelse((GetHitVar(groundtype) = 1),5000,5010) +
        // GetHitVar(animtype)`, which is "draw the crouch-hit art for a
        // `ground.type = Low` attack" -- and it can only draw it because the victim
        // is standing in 5000 to begin with. The state number comes from the
        // victim: the binary's hit path reads a *statetype* off the fighter it is
        // hitting and writes `(0 < statetype - 1 ? 5010 : 5000)` -- "crouching ?
        // 5010 : 5000" (FINDINGS pass 12 §12.2). Choosing it from `ground.type`
        // instead put a standing victim of a low attack into 5010, a crouch MUGEN
        // would never pop them into, and left a crouching victim of a high attack
        // in 5000, which stands them up out of the crouch they were hit in.
        //
        // Trip is the one exception and it is the hit's own: 5070 is `Tripped
        // get-hit`, and it is the one statedef the binary overrides the number
        // for (`if (ground.type == 3) state = 5070`).
        //
        // A *launch* is not a third reaction. It is one of these two with the fall
        // flag set, which is the reference's structure rather than a reading of it:
        // its ground path picks 5000 or 5010 and then lets 5000's own controllers
        // ask `GetHitVar(yvel)` and `GetHitVar(fall)` about the same hit. Entering
        // 5030 here instead, as this did, skipped 5000 altogether -- so a character
        // that ships its own 5000, which is every character with a common state
        // file, never ran it for a launch. `[State 5000, 2]` and `[State 5000, 3]`
        // are what pick the reaction *art* for a Back or an Up launch (5030, or the
        // `5047 + animtype` pair when the character drew one), `[State 5000, 5]` is
        // what turns the victim airborne for the states that ask, and
        // `[State 5000, 7]` is what hands a launched victim on to 5030 once the
        // shake is over. A custom 5000 is also the usual place to hang a special
        // knocked-away animation or a variable the rest of the character reads.
        // All of it was dead code.
        //
        // What the launch does change is the velocity, the clock, and the fall flag.
        // A standing victim goes into the air when `ground.velocity`'s y is
        // *non-zero*, and `fall = 1` does the same with flat velocities, which is how
        // a move is written to knock down without any up-push. The test is non-zero
        // and not "upward": the reference says it in as many words ("If y_velocity is
        // not zero, P2 will be knocked into the air") and so does the binary, whose
        // implicit `forcestand` writer compares the parsed y against `0.0f` --
        // `sub_47F380`, `if (v29 != 0.0f && !a0[101]) a1->field_b4 = 1;` (FINDINGS pass
        // 12 §12.1, corrected in pass 17: `field_b4` is `forcestand`, and the flag at
        // `a0[101]` is "the file wrote it"). That is the same test one step earlier
        // in the pipeline, which is why it is quoted here rather than the launch
        // itself: a launch is exactly the case the implicit default is there for.
        // A *downward* y is 96 of the corpus' ground velocities and every one of them
        // is a slam: 89 also write `fall = 1`, and the ones that do not are the cases
        // this test is for. Reading only "upward" left those hits standing with a
        // velocity into the floor, which the floor then absorbed.
        //
        // The velocity is `ground.velocity`'s own pair, and not `air.velocity`'s. That
        // is the reference's design rather than an accident of it: the two families are
        // mirrors, a ground victim takes `ground.velocity`/`guard.velocity` and an
        // airborne one `air.velocity`/`airguard.velocity`, and the defaults say so out
        // loud (`guard.velocity` "defaults to the x_velocity value of the ground.velocity
        // parameter", `airguard.velocity` to the air pair's). The binary's standing and
        // crouching branch agrees, handing over `field_130`/`field_134` -- the ground
        // pair -- and keeping `field_138`, the y a second time, for the corner push
        // (FINDINGS pass 12 §12.2). Handing over the air pair instead made a slam with
        // no `air.velocity` written arrive with no velocity at all, because that is what
        // the air pair defaults to.
        //
        // The clock is the air one for a victim who is now off the ground --
        // `air.hittime` is "the time p2 stays in the hit state after being hit in or
        // into the air" -- and the ground one for a victim who stays there.
        info.pushX = hit.pushX;
        info.pushY = hit.pushY;
        info.falls = hit.pushY != 0.0f || hit.fall || carriedFall;
        if (info.falls) {
            info.air = true;
            info.hitTime = hit.airHitTime;
        } else {
            info.hitTime = hit.hitTime;
        }
        // Which of the two standstates a grounded victim enters is the victim's own
        // posture and not the attack's height -- and `forcestand` is the one thing
        // that overrides it, which is what makes it worth a parameter. A launcher
        // therefore needs to say nothing: its `ground.velocity.y` is non-zero, so
        // the parameter defaults to 1 and a crouching victim is stood up into 5000
        // rather than left in 5010. The reference's line reads
        // `if (forcestand || v10) victim->field_e58 = 1;` -- one expression in front
        // of the 5010/5000 choice, above (`angr_p9_hitapply.c:287-288`).
        info.stateNumber = victim.stateType_ == StateType::Crouch && !hit.forceStand
                               ? CommonState::kHitLow
                               : CommonState::kHitHigh;
        if (hit.groundType == 3) info.stateNumber = CommonState::kHitTrip;
    }

    // `p2stateno`: the move chooses the victim's state itself, instead of letting
    // the engine pick a reaction. A command grab's posing state is exactly this,
    // and so is the custom hit reaction -- "can also be used for custom hit
    // reactions" -- which is how a move makes its own knock-down look.
    //
    // Which file that state comes out of is `p2getp1state`, and it defaults to the
    // attacker's. That is the whole idiom: the grab writes its victim-posing state
    // in its own .cns, next to the grabbing state, and the victim is handed the
    // number with no way to have it. Of the 276 `p2stateno` writes in local_chars,
    // 272 name a state their own file defines, and one asks for the other reading.
    // This engine used to look the number up in the *victim's* file whatever the
    // parameter said, which sent the victim to a state it had never heard of -- and
    // the fallback for a state number nothing defines is a fighter standing up with
    // control on, so every command grab in the game posed its victim by standing it
    // back on its feet in the middle of the throw.
    if (hit.p2StateNo >= 0) {
        info.stateNumber = hit.p2StateNo;
        if (hit.p2GetP1State) info.stateSourceChar = attackerCharacter;
    }

    // ------------------------------------------------------- hit while down
    //
    // A victim already lying on the floor has its own reaction, which is the
    // whole reason `down.*` exists: the move was written to touch someone down
    // there, and what it does to them is not what it does to someone standing.
    // Only a `hitflag` carrying D gets this far -- `hitMayReach` refuses the rest
    // -- so the branch is reached by intent and not by accident.
    //
    // The y of `down.velocity` is the switch between the two reactions, and it is
    // a switch and not a magnitude: zero slides the victim back along the floor
    // for `down.hittime`, and any non-zero value puts them back in the air, where
    // `down.hittime` is ignored and the air clock applies instead. That is why the
    // two branches disagree about where the hit time comes from rather than just
    // about the velocity.
    if (victimDown) {
        info.air = hit.downVelY != 0.0f;
        // The same question `[State 5080, 4]` and `[State 5080, 5]` ask between
        // them -- `GetHitVar(yvel) = 0` to 5081, and the rest to 5030 -- so the
        // fall flag and the branch above are one answer given twice.
        info.falls = info.air;
        info.pushX = hit.downVelX;
        info.pushY = hit.downVelY;
        // The engine's own choice for a victim that is already down is 5080,
        // whatever the hit then does to it: that is the one get-hit state the
        // *engine* picks rather than the victim's height. Which of the two ways
        // it goes is 5080's own decision and not this function's -- the
        // reference makes it from `GetHitVar(yvel)` inside 5080, and a built-in
        // character has no controllers to make it with, so `applyHit` carries
        // the same answer in `hitFallFlag_` and `runBuiltinState` reads it.
        info.stateNumber = CommonState::kHitLiedown;
        // The clock, though, is still the hit's: an upward `down.velocity` puts
        // the victim back in the air where `down.hittime` is ignored and the air
        // clock applies instead, and a flat one slides it for `down.hittime`.
        info.hitTime = info.air ? hit.airHitTime : hit.downHitTime;
        // `down.bounce` decides whether this comes back off the floor. The
        // engine's ordinary fall bounces whenever its fall velocity points up,
        // which is most of the time, so a downed hit that did *not* ask to
        // bounce has to say so by handing over no bounce velocity at all --
        // otherwise every hit on a grounded victim would pop them back up and
        // the parameter would mean nothing.
        info.fallVelX = hit.downBounce ? static_cast<float>(attackerFacing) * hit.fallVelX : 0.0f;
        info.fallVelY = hit.downBounce ? hit.fallVelY : 0.0f;
        info.sparkAnim = hit.sparkAnim;
        info.sparkFromFightFx = hit.sparkFromFightFx;
        info.sparkX = hit.sparkX;
        info.sparkY = hit.sparkY;
        return info;
    }

    info.sparkAnim = hit.sparkAnim;
    info.sparkFromFightFx = hit.sparkFromFightFx;
    info.sparkX = hit.sparkX;
    info.sparkY = hit.sparkY;
    return info;
}

bool StateMachine::applyHit(const HitInfo& hit) {
    if (life_ <= 0) return false;

    // An armed HitOverride whose mask contains the incoming attr redirects the
    // attack entirely: no damage, no guard, no juggle cost -- the victim freezes
    // like any contact (the attacker got its tell) and enters the chosen state.
    // Parries are written as stances of exactly these.
    for (HitOverrideSlot& slot : hitOverrides_) {
        if (!slot.active) continue;
        if ((hit.attrClassMask & slot.classMask) != hit.attrClassMask) continue;
        if ((hit.attrTypeMask & slot.typeMask) != hit.attrTypeMask) continue;
        slot.active = false;  // spent on the hit it caught
        setPause(hit.victimPause);
        shakeTicks_ = std::max(shakeTicks_, hit.victimPause);
        lastHit_ = hit;
        if (slot.stateNo >= 0) {
            changeState(slot.stateNo, false);
            if (slot.forceAir) {
                stateType_ = StateType::Air;
                physics_ = Physics::Air;
            }
        }
        return true;
    }

    // A super's flash softens what lands while it runs: `p2defmul` scales the
    // *victim's* damage for the pause's length, so the follow-up combo out of a
    // cinematic hits less hard than the same hits in neutral.
    HitInfo resolved = hit;
    if (entities_ != nullptr) {
        const PauseInfo pause = entities_->pauseInfo();
        if (pause.isSuper && pause.ticks > 0 && pause.actor != this &&
            pause.p2DefMul != 1.0f && hit.damage > 0) {
            resolved.damage = std::max(
                1, static_cast<int>(std::lround(hit.damage * pause.p2DefMul)));
        }
    }
    // DefenceMulSet on this machine scales what it takes; 1 is MUGEN's default.
    if (defenceMul_ != 1.0f && resolved.damage > 0) {
        resolved.damage = std::max(1, static_cast<int>(
                                          std::lround(resolved.damage / defenceMul_)));
    }

    // Decide first, and only then change anything.
    //
    // A hit the victim can refuse -- an airborne victim whose air-combo allowance
    // cannot pay for it -- has to leave *nothing* behind. Freezing or shaking on the
    // way to a refusal is worse than landing the hit would have been, because the
    // attacker can hold the hitbox out for as long as it likes: every tick would
    // re-freeze the victim, and a fighter frozen forever is a fighter that never
    // does anything again for the rest of the round.
    const bool paysForIt =
        !(y_ > 0.5f) || jugglePoints_ >= resolved.juggleCost || asserted(kAssertNoJuggleCheck);
    if (!resolved.guarded && !paysForIt) return false;

    // Freeze frames, for both sides. They are set before anything else so that
    // the tick a hit lands on is already a paused tick, which is what makes the
    // impact look like it happened *between* two frames.
    setPause(resolved.victimPause);
    shakeTicks_ = std::max(shakeTicks_, resolved.victimPause);

    if (resolved.guarded) {
        // A block. The blocker takes whatever sliver the attack lets through --
        // usually nothing -- keeps its feet, and is pushed *away* from the
        // attacker rather than carried by them. It does not change state type and
        // it is not put into hitstun, because being unable to act is the thing a
        // block exists to avoid.
        tookDamage_ = true;
        // `guard.kill = 0`: a block cannot be lethal, so the chip damage stops at
        // one point of life. Without this the parameter was parsed and thrown
        // away, and the branch below killed anyway.
        life_ = std::max(resolved.guardKill ? 0 : 1, life_ - resolved.damage);
        // Blocking builds a little power and gives the attacker a little less,
        // which is what makes blocking an investment rather than a stall.
        power_ = std::clamp(power_ + (resolved.givePower >= 0 ? resolved.givePower / 2 : 30),
                            0, kPowerMax);
        // The block *stops* the blocker rather than shoving it: all three of the
        // reference's guard-hit shakes are written `velset = 0,0` with
        // `physics = N`, and the push the block carries is applied by the state
        // *after* the shake instead -- the knock-back's `HitVelSet x = 1`, which
        // reads `guard.velocity` back out of the hit. Sliding from the moment of
        // contact, as this did, is the same mistake as sliding through a get-hit
        // shake: the fighter is meant to be held still while it shakes. A
        // character that ships the shake states gets this from their own headers
        // either way, because `enterState` applies the header's `velset`; this is
        // the file-less path's half of it.
        velX_ = 0.0f;
        velY_ = 0.0f;
        offsetX_ = 0.0f;
        offsetY_ = 0.0f;
        lastHit_ = resolved;
        // The blocker is held in the guard-hit state for the guard's own hit
        // time, and control comes back on its own afterwards -- except in the
        // air, where the reference writes no `+0x1094` at all (see
        // `HitInfo::keepStunClock`). There the block is held by its shake and
        // the hittime slot keeps whatever it had, which for a blocker who was
        // not already stunned is `-1`.
        if (!resolved.keepStunClock) stunTicks_ = std::max(1, resolved.hitTime);
        moveGuarded_ = 1;
        // A block makes its own spark -- a flat flash rather than a starburst.
        spawnHitSpark(resolved);
        // Every block costs something, even a free one: that is what makes the guard
        // meter a resource rather than a readout.
        spendGuard(kGuardDrainPerBlock);
        if (sound_ != nullptr) sound_->play(resolved.guardSoundGroup, resolved.guardSoundSample);
        if (life_ <= 0) {
            // The block broke and killed. MUGEN lets a character refuse this with
            // `guard.kill = 0`, and the ones that do are the ones that want chip
            // damage to be survivable. The route out is the same one any death
            // takes: the fall, then the floor, then dead. See the same branch in
            // `applyHit`.
            changeState(CommonState::kHitFall, false);
            stateType_ = StateType::Air;
            physics_ = Physics::Air;
            if (y_ <= 0.0f && velY_ == 0.0f) velY_ = 3.0f;
            return true;
        }
        changeState(resolved.stateNumber, false);
        return true;
    }

    // Juggle: an air combo is paid for out of the victim's allowance, and a hit
    // the allowance cannot cover does not land at all. That is the whole of what
    // stops an air combo from being infinite, and it is why a launcher spends more
    // than a jab does.
    if (y_ > 0.5f) jugglePoints_ -= resolved.juggleCost;

    tookDamage_ = true;
    // `kill = 0` on the HitDef: the hit lands, does its damage, and stops one
    // point short of a knockout. The victim still takes the reaction -- the fall,
    // the knockdown, the whole animation -- and simply is not dead at the end of
    // it, which is what a move written this way is for.
    life_ = std::max(resolved.kill ? 0 : 1, life_ - resolved.damage);
    // Remember what just hit this fighter, for a later link's `chainID` to ask
    // about. Recorded here, on the path a hit actually travels, and not on the
    // block path above: the parameter reads "if P2 was last *hit* by a move with
    // this ID", and a blocked attack did not hit anybody. A refused hit returned
    // long before this point, so it leaves no trace either.
    lastHitById_ = resolved.id;

    // `GetHitVar(hitcount)`: the number of hits taken in the combo in progress.
    // Two rules, and they are both load-bearing for the way the corpus reads it.
    //
    // A hit that arrives after this fighter has been *out* of a hit reaction
    // long enough starts a new combo, so the count goes back to zero first --
    // otherwise a jab that lands after a recovery would be numbered as a
    // continuation of the combo the fighter has already left, and the damage
    // scaling that reads this number (Kira, Giorno and Vegeta all scale by
    // `Enemy,GetHitVar(HitCount)`) would keep discounting a fresh combo as if it
    // were the twentieth hit.
    //
    // "Long enough" is one frame, not zero, which is the part that is easy to
    // get wrong. A victim still in a hit reaction obviously continues the combo;
    // so does one that has had a *single* free frame since, because that is what
    // a great many moves produce -- a hit whose hitstun and the victim's
    // recovery land on adjacent ticks leaves exactly one frame of movetype I
    // between them, and those are meant to chain. Only from the second free
    // frame on is the next hit a new combo. `framesSinceHitReaction_` is that
    // clock, maintained in `step()`, and the test is `> 1`.
    //
    // A blocked hit never reaches here at all -- the block path returned long
    // above -- which is the other half of the rule: guarding a hit does not
    // advance the combo the opponent is building.
    //
    // Asked *before* the move type below becomes Hit, because that assignment is
    // what the test would otherwise be reading.
    if (moveType_ != MoveType::Hit && framesSinceHitReaction_ > 1) hitsTakenInCombo_ = 0;
    ++hitsTakenInCombo_;

    power_ = std::clamp(power_ + (resolved.givePower >= 0 ? resolved.givePower : 60), 0,
                        kPowerMax);

    // `p2facing` if written; otherwise the classic standing-hit turn toward the
    // attacker, which is what makes mirrored hit animations come out right.
    if (resolved.p2facing == 1) {
        facing_ = -resolved.attackerFacing;
    } else if (resolved.p2facing == -1) {
        facing_ = resolved.attackerFacing;
    } else if (resolved.p2facing == 0) {
        // keep the victim's facing as it stands
    } else {
        facing_ = -resolved.attackerFacing;  // not written: the classic turn
    }

    // The impact's own shake rides the hit -- a heavy connect shakes the screen,
    // not just the victim.
    if (resolved.shakeTime > 0 && entities_ != nullptr) {
        entities_->shake(resolved.shakeTime, resolved.shakeFreq, resolved.shakeAmpl);
    }

    // `snap`: the victim posed at the attacker's axis plus the written offset,
    // in the attacker's facing frame. Down-positive y on the page crosses to
    // height-above-floor here like every other y.
    if (resolved.hasSnap) {
        x_ = resolved.attackerX + static_cast<float>(resolved.attackerFacing) * resolved.snapX;
        y_ = std::max(0.0f, -resolved.snapY);
    }

    // The HitDef wrote its push in the attacker's frame; `hitInfoFor` has
    // already turned it into a world velocity, so it goes in as it stands.
    velX_ = resolved.pushX;
    velY_ = resolved.pushY;
    offsetX_ = 0.0f;
    offsetY_ = 0.0f;

    lastHit_ = resolved;
    // Every hit seeds the stun clock except an air block, which is the one path
    // where the reference does not write `+0x1094` at all. See
    // `HitInfo::keepStunClock`: the blocker's clock simply keeps running, and a
    // blocker who was not already stunned stays at -1, so `hitover` is true for
    // the whole block. The one-tick floor on every other hit keeps a
    // `hittime = 0` reaction in its pose for the tick it was applied on.
    if (!resolved.keepStunClock) stunTicks_ = std::max(1, resolved.hitTime);

    // The spark, where the hit landed. It belongs to the victim's tick rather than the
    // attacker's because this is where the contact happened and because the victim is the
    // one that knows what it got hit by.
    spawnHitSpark(resolved);

    // MUGEN marks the victim as being in a hit *before* the hit state takes
    // over, which matters for the states that ask to be transparent: a common
    // state file may write `movetype = U` on its hit states -- "keep whatever I
    // already was" -- and the value it means to keep is this one. Setting it
    // afterwards instead would let those states leave the victim looking idle
    // while a hit is playing out, and hitstun, which is read off the move type,
    // would end on the first tick.
    moveType_ = MoveType::Hit;

    // A fall remembers what its landing owes (fall.damage and the bounce) and
    // flags itself for `hitfall`. The flag is the hit's own verdict -- MUGEN's
    // `GetHitVar(fall)`, resolved once in `hitInfoFor` -- and not a restatement of
    // the state number: the number is now 5000 or 5010 for a launch, and the whole
    // point of the flag is that those two states split on it a tick later. A plain
    // air hit -- a state that wobbles rather than falls -- is not a fall, and
    // neither is a hit on a victim already down unless it lifts them back into the
    // air.
    hitFallFlag_ = resolved.falls;
    if (resolved.air) {
        setFallLanding(resolved.fallDamage, resolved.fallKill, resolved.fallVelX,
                       resolved.fallVelY, resolved.fallShakeTime, resolved.fallShakeFreq,
                       resolved.fallShakeAmpl);
    }

    // A state file borrowed by an earlier hit ends here on every real hit: "P2 will
    // retain P1's states and animation data until P2 is hit, or a SelfState
    // controller is used to return P2 to his own states." This is that hit. It ends
    // whether or not the new hit hands over a file of its own -- the line below
    // re-borrows when it does -- and it ends on the *non*-guard path only, because
    // blocking a move is not being hit by it. The reference spells the same fact out
    // in its debug overlay, which annotates a fighter that is not running its own
    // state data with "(in <name> <n>'s state rc)": the borrow is a property of the
    // player record and not of the state, which is why a state change inside the
    // borrowed file does not end it and only these two things do.
    borrowedStateChar_ = nullptr;
    if (resolved.stateSourceChar != nullptr) borrowedStateChar_ = resolved.stateSourceChar;

    if (life_ <= 0) {
        // Knocked out. Dying is not a state the engine keeps to itself: the
        // fighter falls like any other victim and the *file* decides what being
        // dead looks like from there. State 0's own `!alive` controller sends it
        // into 5050, the falling get-hit state; 5110's `!alive` controller then
        // sends it from the floor into 5150, where it lies until the round ends
        // and the engine stands it up. The built-in states follow that same
        // route, so a character that ships no common state file dies the same
        // way as one that does.
        //
        // The hop is this engine's own and not the reference's: 5050 would
        // otherwise be entered at height zero with no velocity, and would hand
        // straight over to 5100 on the tick it began. Three units a tick is
        // enough to make the death read as a fall without moving the fighter
        // anywhere they were not already.
        changeState(CommonState::kHitFall, false);
        stateType_ = StateType::Air;
        physics_ = Physics::Air;
        if (y_ <= 0.0f && velY_ == 0.0f) velY_ = 3.0f;  // a small death hop
        return true;
    }
    changeState(resolved.stateNumber, false);
    return true;
}

// Draws the spark a hit makes. Small enough to be one function: it is an explod with a
// short life, drawn additively, placed at the point of contact.
void StateMachine::spawnHitSpark(const HitInfo& hit) {
    if (entities_ == nullptr || hit.sparkAnim < 0) return;

    Explod spark;
    spark.owner = character_;
    spark.ownerMachine = this;
    spark.ownerSide = side_;
    spark.fromFightFx = hit.sparkFromFightFx;
    spark.anim = hit.sparkAnim;
    // `sparkxy` is relative to the *front* of the victim, so the offset goes the way the
    // victim is facing -- and the victim has already been turned to face the attack by the
    // time this runs, which is what makes the spark land on the right side of it.
    spark.x = x_ + static_cast<float>(facing_) * hit.sparkX;
    // MUGEN measures the y downward; this engine keeps height the other way up.
    spark.y = y_ - hit.sparkY;
    spark.facing = facing_;
    // No `trans` of its own: a spark is drawn the way its own animation asks, which
    // is what the reference does for every effect and what `fightfx.air` is written
    // for. Its hit-spark actions carry an `A` or an `A1` on every frame, so they
    // come out additive anyway -- and the `A1` ones now come out at the half
    // destination alpha the code spells, where a hardcoded `Additive` here used to
    // over-brighten them. The guard spark and the dust clouds are actions with no
    // colour-add code at all, and those draw normally, as their author wrote them.
    spark.spritePriority = 5;
    // Short, and never bound: a spark is the impact, not the fighter. It also
    // never freezes: the engine's own sparks play through the hitpause while the
    // fighters stand in it, which is the whole read of a connect in MUGEN.
    spark.removeTime = -2;
    spark.ignoreHitPause = true;
    entities_->spawnExplod(spark);
}

void StateMachine::markHitLanded(StateMachine* victim) {
    if (sound_ != nullptr) sound_->play(hitDef_.hitSoundGroup, hitDef_.hitSoundSample);
    moveHit_ = 1;
    moveContact_ = 1;
    ++hitCount_;
    ++uniqHitCount_;
    // Contact made the victim this move's target, which the `target` redirect and
    // the bindtotarget a throw holds its victim with both read. A guarded hit
    // does not acquire: blocking a throw is not being thrown.
    if (victim != nullptr && !victim->doomed()) lastTarget_ = victim;

    // `hitcount = n` lets one HitDef connect n times, which is how a move is
    // written as a multi-hit without a HitDef per hit. Once the count runs out the
    // hit is spent, and a single-frame hit is spent on the first contact.
    if (--hitDef_.hitCount <= 0) {
        hitDef_.armed = false;
    }

    // `p1stateno`: the attacker walks into the state the grab written for itself,
    // which -- paired with the victim's `p2stateno` on the same HitDef -- is the
    // whole of how a command grab poses both fighters.
    if (hitDef_.p1StateNo >= 0) {
        changeState(hitDef_.p1StateNo, false);
    }

    // `p1facing`/`p1getp2facing`: where the attacker looks now that the hit has
    // landed, which is the other half of the reverse throw -- the state above poses
    // it, and this faces it the way the throw is being played. The pair is read the
    // way the reference reads it: `p1getp2facing` alone when it is non-zero,
    // otherwise the sign of `p1facing`. Applied *after* the state change, so a
    // facing the HitDef asked for beats a `facep2` in the state it walks into; and
    // after the victim's own reaction, which is inside `applyHit` on the far side of
    // the contact, so `victim->facing()` is already the post-hit one.
    if (hitDef_.p1GetP2Facing != 0) {
        // It stands in for the pair, so it does not fall back on the sign: a hit
        // with nobody on the far end -- the placeholder's, or a host test's -- has
        // no facing to copy and leaves the attacker where it was.
        if (victim != nullptr) {
            facing_ = hitDef_.p1GetP2Facing > 0 ? victim->facing_ : -victim->facing_;
        }
    } else if (hitDef_.p1facing < 0) {
        facing_ = -facing_;
    }
}

void StateMachine::separate(StateMachine& a, StateMachine& b) {
    // Only fighters whose feet are on the ground can be stood between; an
    // airborne one passes over rather than through.
    if (a.y_ > 0.0f || b.y_ > 0.0f) return;
    // `PlayerPush = 0` turns the shove off for whoever wrote it: a ghost dash
    // passes through the opponent instead of shouldering into them.
    if (!a.playerPush_ || !b.playerPush_) return;

    // The reference has no gap constant: each fighter's reach towards the other
    // is its width bar, i.e. the `[Size]` ground/air width for its posture plus
    // whatever the Width controller added (always additive, never a
    // replacement). With no character behind either machine -- the placeholder
    // arena -- both bars are zero, so keep the old constant-gap behaviour.
    const float widthA = static_cast<float>(a.widthToward(b.x_));
    const float widthB = static_cast<float>(b.widthToward(a.x_));
    if (widthA <= 0.0f && widthB <= 0.0f) {
        constexpr float kMinGap = 26.0f;
        const float gap = b.x_ - a.x_;
        const float distance = std::fabs(gap);
        const float overlapAmount = kMinGap - distance;
        if (overlapAmount <= 0.0f) return;

        const float half = overlapAmount * 0.5f * (gap > 0.0f ? 1.0f : -1.0f);
        a.x_ = std::clamp(a.x_ - half, a.stageMin_, a.stageMax_);
        b.x_ = std::clamp(b.x_ + half, b.stageMin_, b.stageMax_);
        return;
    }

    // The reference orders the pair low-x first and works from `dx` between the
    // axes. Widths were taken before the ordering; the sum does not care.
    StateMachine* left = &a;
    StateMachine* right = &b;
    if (left->x_ > right->x_) std::swap(left, right);
    const float dx = right->x_ - left->x_;

    // A near-coincident pair -- crossed over, or one tick after a teleport --
    // is first recentred about its midpoint at +-(int)screenWidth/64. This is
    // *not* either/or with the push: the reference does it and then still
    // applies the width sum, against the original `dx`.
    if (dx < static_cast<float>(static_cast<int>(kScreenWidth) / 32)) {
        const float mid = (left->x_ + right->x_) * 0.5f;
        const float offset = static_cast<float>(static_cast<int>(kScreenWidth) / 64);
        left->x_ = mid - offset;
        right->x_ = mid + offset;
    }

    // Push apart by the overlap of the two width bars, split evenly about the
    // midpoint (no facing or mass weighting). The reference does not re-apply
    // the screen bounds here; the next physics step's own clamp does.
    const float sum = widthA + widthB;
    if (sum > dx) {
        const float push = (sum - dx) * 0.5f;
        left->x_ -= push;
        right->x_ += push;
    }
}

// ------------------------------------------------------------------ geometry

int StateMachine::widthToward(float otherX) const {
    // Front or back is whichever half of the bar points at the other fighter.
    // The reference makes the same choice from facing and from which of the
    // ordered pair the player is, which comes to the same thing.
    const bool front = (facing_ > 0) == (otherX >= x_);
    float raw = 0.0f;
    if (character_ != nullptr) {
        const char* key;
        if (stateType_ == StateType::Air) {
            key = front ? "size.air.front" : "size.air.back";
        } else {
            key = front ? "size.ground.front" : "size.ground.back";
        }
        raw = character_->constant(key, 0.0f);
    }
    const int bar = front ? playerFront_ : playerBack_;
    // The reference rounds half up after scaling; here the numbers are already
    // in the character's own units, so only the rounding remains.
    return static_cast<int>(std::floor(raw + static_cast<float>(bar) + 0.5f));
}

int StateMachine::edgeBodyOffset(bool front) const {
    // The body variants add a small statetype term on top of the edge width:
    // +1 in the air and +2 lying down. It is added to both the front (`0xb`)
    // and back (`0xc`) variants, and to neither of the plain ones.
    int offset = front ? edgeFront_ : edgeBack_;
    if (stateType_ == StateType::Air) {
        offset += 1;
    } else if (stateType_ == StateType::Liedown) {
        offset += 2;
    }
    return offset;
}

float StateMachine::bodyExtentTowardOpponent() const {
    const Animation* animation = currentAnimPtr();
    if (animation == nullptr || animation->hurtBoxes.empty()) return 12.0f;
    const int frame = animation->frameAt(animTime_);
    if (frame < 0 || frame >= static_cast<int>(animation->hurtBoxes.size())) return 12.0f;
    const std::vector<CollisionBox>& boxes = animation->hurtBoxes[static_cast<size_t>(frame)];
    if (boxes.empty()) return 12.0f;

    // Collision boxes are in the fighter's local frame, so the extreme in the
    // facing direction is either x2 (facing right) or -x1 (facing left).
    float extent = 12.0f;
    for (const CollisionBox& box : boxes) {
        const float front = facing_ >= 0 ? box.x2 : -box.x1;
        extent = std::max(extent, front);
    }
    return extent;
}

float StateMachine::p2DistX() const {
    if (opponent_ == nullptr) return 0.0f;
    return (opponent_->x_ - x_) * static_cast<float>(facing_);
}

float StateMachine::p2BodyDistX() const {
    if (opponent_ == nullptr) return 0.0f;
    const float axis = p2DistX();
    const float gap = std::fabs(opponent_->x_ - x_) - bodyExtentTowardOpponent() -
                      opponent_->bodyExtentTowardOpponent();
    return axis >= 0.0f ? gap : -gap;
}

// ------------------------------------------------------------------- triggers

const TriggerContext* StateMachine::redirect(std::string_view target, int selector) const {
    // `p2` and `enemynear` are the opponent; `p1` and `self` are this fighter,
    // which is how a character reads its own values through a redirect.
    if (target == "p2" || target == "enemynear" || target == "enemy") return opponent_;
    if (target == "self" || target == "p1") return this;

    // The family redirects. root is the real fighter at the top of the spawn
    // chain (this machine itself, for a root); parent is whoever spawned this
    // helper. A root asking for either gets itself and nothing, respectively --
    // MUGEN raises an error for `parent` from a root, and reading zero here is
    // the harmless version of that.
    if (target == "root") return root_;
    if (target == "parent") return parent_;

    // `helper` picks among the family's living helpers: the one whose id matches
    // the selector, or the most recent one without. From inside a helper the
    // machine itself is skipped, because MUGEN's helper redirect names *another*
    // of the family's helpers, never the one asking.
    if (target == "helper") {
        const std::vector<StateMachine*>& family = root_->helpers_;
        if (selector >= 0) {
            for (StateMachine* machine : family) {
                if (machine->helperId_ == selector && !machine->doomed()) return machine;
            }
            return nullptr;
        }
        for (auto it = family.rbegin(); it != family.rend(); ++it) {
            if (*it != this && !(*it)->doomed()) return *it;
        }
        return nullptr;
    }

    // Whoever this move is currently connecting with: the throw-and-drag
    // redirect. Expires with the move -- a character asks for a new one by
    // landing another hit.
    if (target == "target") {
        return (lastTarget_ != nullptr && !lastTarget_->doomed()) ? lastTarget_ : nullptr;
    }

    // `playerid(n)`: a match-wide lookup the sink answers, because the machine
    // only knows its own family.
    if (target == "playerid" && entities_ != nullptr) {
        return entities_->playerById(selector);
    }

    // `partner` belongs to team modes, which this match has none of.
    return nullptr;
}

Value StateMachine::trigger(std::string_view name, const std::vector<Value>& args) const {
    // A redirect reaches here as the target's name with the inner trigger as
    // its only argument -- `enemynear, vel x` arrives as ("enemynear", [vel x]).
    // The expression evaluator resolves it before this is called, but a name
    // written bare in a file still lands here, so it is answered rather than
    // treated as unknown.
    if (name == "p2" || name == "enemynear" || name == "enemy") {
        return Value::number(opponent_ != nullptr ? 1.0f : 0.0f);
    }
    if (name == "p1" || name == "self") return Value::number(1.0f);
    if (name == "root") return Value::number(1.0f);
    if (name == "parent") return Value::number(parent_ != nullptr ? 1.0f : 0.0f);
    if (name == "helper") {
        return Value::number(redirect("helper", args.empty() ? -1 : args[0].toInt()) != nullptr
                                 ? 1.0f
                                 : 0.0f);
    }
    if (name == "target") {
        return Value::number(lastTarget_ != nullptr && !lastTarget_->doomed() ? 1.0f : 0.0f);
    }

    return ownTrigger(name, args);
}

Value StateMachine::ownTrigger(std::string_view name, const std::vector<Value>& args) const {
    auto argInt = [&args](size_t index, int fallback) {
        return index < args.size() ? args[index].toInt() : fallback;
    };
    auto argFloat = [&args](size_t index, float fallback) {
        return index < args.size() ? args[index].toFloat() : fallback;
    };
    auto boolValue = [](bool value) { return Value::boolean(value); };

    // ------------------------------------------------------------- the state
    if (name == "stateno") return Value::number(static_cast<float>(stateNumber_));
    if (name == "prevstateno") return Value::number(static_cast<float>(prevStateNumber_));
    if (name == "time") return Value::number(static_cast<float>(stateTime_));
    if (name == "ctrl") return boolValue(ctrl_);
    if (name == "statenoexist") {
        return boolValue(stateLibrary() != nullptr && stateLibrary()->has(argInt(0, -1)));
    }

    // -------------------------------------------------------- state and move
    //
    // `statetype = A` compares against the bare letter A, which the expression
    // parser hands over as an unknown identifier. Returning the same lower-case
    // letter here is what makes those comparisons true -- see the fallback at
    // the end of this function for the other half of it.
    if (name == "statetype") {
        switch (stateType_) {
            case StateType::Stand: return Value::text("s");
            case StateType::Crouch: return Value::text("c");
            case StateType::Air: return Value::text("a");
            case StateType::Liedown: return Value::text("l");
        }
        return Value::text("s");
    }
    if (name == "movetype") {
        switch (moveType_) {
            case MoveType::Idle: return Value::text("i");
            case MoveType::Attack: return Value::text("a");
            case MoveType::Hit: return Value::text("h");
            case MoveType::Unchanging: return Value::text("u");
        }
        return Value::text("i");
    }
    if (name == "physics") {
        switch (physics_) {
            case Physics::Stand: return Value::text("s");
            case Physics::Crouch: return Value::text("c");
            case Physics::Air: return Value::text("a");
            case Physics::None: return Value::text("n");
        }
        return Value::text("s");
    }

    // ------------------------------------------------------------ animation
    if (name == "anim") return Value::number(static_cast<float>(anim_));
    if (name == "animelem") return Value::number(static_cast<float>(currentElem()));
    if (name == "animtime") return Value::number(static_cast<float>(currentAnimTime()));
    if (name == "animelemtime") {
        // Ticks since element n started: 0 on the tick it appears, negative
        // before it. Characters use it to time a window inside an animation.
        const int elem = argInt(0, 1);
        const Animation* animation = currentAnimPtr();
        if (animation == nullptr || elem < 1 ||
            elem > static_cast<int>(animation->frames.size())) {
            return Value::number(-1.0f);
        }
        long long start = 0;
        for (int i = 0; i < elem - 1; ++i) {
            start += std::max(0, animation->frames[static_cast<size_t>(i)].duration);
        }
        return Value::number(static_cast<float>(animTime_ - start));
    }
    if (name == "animelemno") {
        const int elem = argInt(0, -1);
        return boolValue(elem >= 1 && elem == currentElem());
    }
    if (name == "animexist") {
        // `AnimExist` asks about the art the fighter is being drawn with right
        // now, which follows a `p2stateno` borrow and a ChangeAnim2 pose.
        return boolValue(animation(argInt(0, -1)) != nullptr);
    }
    if (name == "selfanimexist") {
        // `SelfAnimExist` is the spelling that deliberately does not: the
        // reference's docs say it "only checks P1's animation data", so a
        // victim dressed in the attacker's file still answers from its own.
        const int candidate = argInt(0, -1);
        return boolValue(character_ != nullptr && character_->animation(candidate) != nullptr);
    }

    // ------------------------------------------------------------- geometry
    if (name == "pos x") return Value::number(posX());
    if (name == "pos y") return Value::number(posY());
    if (name == "vel x") return Value::number(localVelX());
    // Read back through the same flip the file writes through: `vel y` is positive
    // while a fighter is falling, whichever way this engine stores it.
    if (name == "vel y") return Value::number(-velY_);
    if (name == "facing") return Value::number(static_cast<float>(facing_));
    // The four edge triggers are one pair of opcodes in the reference (`0xb` and
    // `0xc`), with the "body" spelling setting a flag that moves the reference
    // point from the x-axis to the end of the width bar. `0xb` measures towards
    // the bound the fighter faces; `0xc` towards the one behind it.
    if (name == "backedgedist" || name == "backedgebodydist") {
        const bool body = name == "backedgebodydist";
        const float offset = body ? static_cast<float>(edgeBodyOffset(false)) : 0.0f;
        const float back = x_ - static_cast<float>(facing_) * offset;
        return Value::number(facing_ >= 0 ? back - stageMin_ : stageMax_ - back);
    }
    if (name == "frontedgedist" || name == "frontedgebodydist") {
        const bool body = name == "frontedgebodydist";
        const float offset = body ? static_cast<float>(edgeBodyOffset(true)) : 0.0f;
        const float front = x_ + static_cast<float>(facing_) * offset;
        return Value::number(facing_ >= 0 ? stageMax_ - front : front - stageMin_);
    }
    // Screen space, for the chars that lay effects out in pixels: the camera's
    // middle is 160 across the canonical 320-wide space, y down from the floor
    // line as MUGEN measures.
    if (name == "screenpos x") {
        const float mid = entities_ != nullptr ? entities_->cameraMidX() : 160.0f;
        return Value::number(x_ - (mid - 160.0f));
    }
    if (name == "screenpos y") {
        const float floor = entities_ != nullptr ? entities_->floorLineY() : 210.0f;
        return Value::number(floor - y_);
    }

    // ------------------------------------------------------------- the fight
    if (name == "life") return Value::number(static_cast<float>(life_));
    if (name == "lifemax") return Value::number(static_cast<float>(maxLife_));
    if (name == "power") return Value::number(static_cast<float>(power_));
    if (name == "alive") return boolValue(life_ > 0);
    if (name == "attack") {
        return Value::number(static_cast<float>(character_ != nullptr ? character_->attack() : 100));
    }
    if (name == "defence") {
        return Value::number(static_cast<float>(
            character_ != nullptr ? character_->defence() : 100));
    }
    if (name == "gametime") return Value::number(static_cast<float>(gameTime_));
    if (name == "roundstate") return Value::number(static_cast<float>(roundState_));
    if (name == "roundno") return Value::number(static_cast<float>(roundNo_));
    // The round's verdict, read from this fighter's own side. win/lose/drawgame
    // are how a character picks its celebration; winperfect layers on top.
    if (name == "win") return boolValue(roundOutcome_ == 1);
    if (name == "lose") return boolValue(roundOutcome_ == 0);
    if (name == "drawgame") return boolValue(roundOutcome_ == 2);
    if (name == "winperfect") {
        return boolValue(roundOutcome_ == 1 && life_ == maxLife_);
    }
    if (name == "roundno") return Value::number(static_cast<float>(roundNo_));
    // The palette the machine is drawn with: the selection itself, not the
    // RemapPal redirection -- `palno` is what a character tests to pick between
    // "which costume am I", and the remap table is not part of that answer.
    if (name == "palno") return Value::number(static_cast<float>(palNo_));
    // 0 for a human, which is what a character tests to decide whether it is
    // being played by the computer.
    if (name == "ailevel") {
        return Value::number(aiControlled_ ? static_cast<float>(aiLevel_) : 0.0f);
    }
    // The virtual canvas the screenpack declared (MUGEN's gamewidth/gameheight).
    // 320x240 is the screenpack this game ships, and the stage/char space scale
    // is resolved against it.
    if (name == "gamewidth") return Value::number(kScreenWidth);
    if (name == "gameheight") return Value::number(kScreenHeight);
    // The coordinate-space conversion triggers. Each takes a length written in
    // one resolution's space and returns it in the player's own; the ratio is
    // the ratio of the widths, per Elecbyte's reference. The argument is a
    // *length*, not a question about what resolution the game is running at,
    // which is the mistake worth avoiding -- these are not booleans.
    //
    // The reference recommends them for the non-zero position and velocity
    // offsets in `common1.cns`, so that a character drawn in 720p space keeps
    // its offsets where its author put them. Returning the argument unscaled,
    // or zero, is the difference between a 720p character's landing dust
    // sitting at its feet and sitting a third of the way up the screen.
    //
    // This screenpack is 240p, so the first is the identity and the other two
    // divide -- the reference's own worked example is `const480p(6)` = 3 and
    // `const720p(12)` = 3, which is what these produce.
    if (name == "const240p") {
        return Value::number(argFloat(0, 0.0f) * kScreenWidth / kWidth240p);
    }
    if (name == "const480p") {
        return Value::number(argFloat(0, 0.0f) * kScreenWidth / kWidth480p);
    }
    if (name == "const720p") {
        return Value::number(argFloat(0, 0.0f) * kScreenWidth / kWidth720p);
    }
    if (name == "tickspersecond") return Value::number(60.0f);
    if (name == "teamside") return Value::number(facing_ >= 0 ? 1.0f : 2.0f);
    // The team-mode surface answers for the mode this fight actually is: single,
    // no partners, not on standby.
    if (name == "numpartner") return Value::number(0.0f);
    if (name == "standby") return boolValue(false);
    if (name == "ishometeam") return boolValue(false);
    // `TeamMode` answers a mode code, not text: the reference compiles the
    // right-hand keywords to 0 (`single`), 1 (`simul`) and 3 (`turns`), and
    // this engine only ever fields a single fighter per side
    // (`TEAM-MODES-RE.md` §2.3). Returning the text "single" made the keyword
    // comparison fall back to a string-vs-number compare, which is never true
    // for a zero.
    if (name == "teammode") return Value::number(0.0f);
    // The run's counter, which the fight screen owns: versus is always match 1,
    // and an arcade match only advances when the previous one was won.
    if (name == "matchno") return Value::number(static_cast<float>(matchNo_));
    // The reference does not call the match over when the deciding round ends;
    // it calls it over once the winner has entered its victory pose. The fight
    // screen sets this at the phase that follows the KO spill (where this
    // engine's winner holds its pose), so a character reading it here gets the
    // same "the fight is still settling" answer.
    if (name == "matchover") return boolValue(matchOver_);
    if (name == "roundsexisted") return Value::number(static_cast<float>(roundNo_ - 1));
    // Not a trigger in the reference. Answering 0 rather than inventing a count
    // keeps a character that tests it from firing a state on a number that
    // means nothing; `roundsexisted` above is the round-count trigger that does
    // exist.
    if (name == "roundsremaining") return Value::number(0.0f);
    if (name == "ingame") return boolValue(roundState_ == 2);
    // Not a trigger MUGEN 1.0 has -- it is absent from the reference's own
    // alphabetical index, where `roundno`, `roundsexisted` and `roundstate` are
    // the round triggers that do exist. It is answered anyway because a
    // character written for a later build may test it, and 0 ("the round just
    // began") is the neutral answer rather than a value that would fire a
    // state the character did not mean to enter.
    if (name == "timeroundstart") return Value::number(0.0f);
    if (name == "playeridexist") {
        const int id = argInt(0, 0);
        if (id == playerId_) return boolValue(true);
        return boolValue(entities_ != nullptr && entities_->playerById(id) != nullptr);
    }
    // Likewise not a trigger: in the reference `Turns` is a *team mode* -- the
    // value `teammode` compares equal to -- and not a number a state can test.
    // Answering 0 keeps a character that writes `trigger1 = turns` false, which
    // is right twice over: this game is never in turns mode, and the test is
    // malformed anyway.
    if (name == "turns") return Value::number(0.0f);

    // ------------------------------------------------------- the opponent
    if (name == "numenemy") return Value::number(opponent_ != nullptr ? 1.0f : 0.0f);
    // Nothing here creates a helper, a projectile or an explod, so the counts
    // that describe them are genuinely zero rather than unknown.
    if (name == "numexplod") {
        return Value::number(entities_ != nullptr
                                 ? static_cast<float>(
                                       entities_->countExplods(character_, this, argInt(0, -1)))
                                 : 0.0f);
    }
    if (name == "numproj" || name == "numprojectile") {
        return Value::number(entities_ != nullptr
                                 ? static_cast<float>(
                                       entities_->countProjectiles(character_, this, argInt(0, -1)))
                                 : 0.0f);
    }
    if (name == "numprojid" || name == "numprojectileid") {
        // `NumProjID` (`0x473440`) is NumProj's state filter plus a plain
        // equality against the projectile's own ID at +0x0. The docs say a
        // negative ID is treated as 0, but the routine has no clamp
        // (`PROJECTILE-PARAMS-RE.md` §4.10); `projid` should be positive, so a
        // negative argument matches nothing either way. `countProjectiles`
        // treats a negative id as "any", which is right for bare `numproj` and
        // wrong for this trigger, hence the guard.
        const int id = argInt(0, 0);
        return Value::number(entities_ != nullptr && id >= 0
                                 ? static_cast<float>(entities_->countProjectiles(character_, this, id))
                                 : 0.0f);
    }
    if (name == "numhelper") {
        return Value::number(static_cast<float>(countHelpers(argInt(0, -1))));
    }
    if (name == "ishelper") {
        // Bare: whether this machine is any helper. With an argument: whether it
        // is the one with that id.
        if (!isHelper_) return boolValue(false);
        const int id = argInt(0, -1);
        return boolValue(id < 0 || id == helperId_);
    }
    // A "target" is whoever is currently being hit by this fighter's move, which
    // is what the contact bookkeeping records. More than one target is a team
    // mode or a multi-hit idiom this match does not have, so one is the count.
    if (name == "numtarget") {
        return Value::number(lastTarget_ != nullptr && !lastTarget_->doomed() ? 1.0f : 0.0f);
    }
    if (name == "parentdist x") {
        return Value::number(parent_ != nullptr
                                 ? (parent_->x_ - x_) * static_cast<float>(facing_)
                                 : 0.0f);
    }
    if (name == "parentdist y") {
        return Value::number(parent_ != nullptr ? parent_->y_ - y_ : 0.0f);
    }
    if (name == "rootdist x") {
        return Value::number(root_ != this ? (root_->x_ - x_) * static_cast<float>(facing_)
                                            : 0.0f);
    }
    if (name == "rootdist y") {
        return Value::number(root_ != this ? root_->y_ - y_ : 0.0f);
    }

    // --------------------------------------------------------------- combat
    //
    // Projectile-contact triggers. The reference keeps one triple on the
    // *owning player* (`+0x25C` projectile id, `+0x260` code, `+0x264` frames
    // since), so these read the side's last contact rather than any live
    // projectile; a beam that has already been freed still answers them.
    //
    // Only roots answer. MUGEN's players include helpers, but this engine's
    // projectile bookkeeping is keyed by side on `EntityLayer` and helpers
    // follow the root's player model here, so a helper gets the neutral answer
    // (false / -1) instead of a mix of its own and its root's contacts.
    //
    // Accepted spellings: the documented suffix (`ProjHit1234`), the function
    // form found in the corpus (`projhit(1234)`), and the bare form (any id).
    {
        auto suffixNumber = [](std::string_view full, std::string_view prefix, int& out) {
            if (full.size() <= prefix.size()) return false;
            if (full.compare(0, prefix.size(), prefix) != 0) return false;
            int value = 0;
            for (size_t i = prefix.size(); i < full.size(); ++i) {
                const char c = full[i];
                if (c < '0' || c > '9') return false;
                value = value * 10 + (c - '0');
                if (value > 1000000000) return false;
            }
            out = value;
            return true;
        };
        const ProjectileContactRecord contact =
            (!isHelper_ && entities_ != nullptr) ? entities_->projectileContact(side())
                                                 : ProjectileContactRecord{};
        const int maskHit = projectileContactMask(kProjectileContactHit);
        const int maskGuarded = projectileContactMask(kProjectileContactGuarded);
        const int maskContact = maskHit | maskGuarded;

        // The `Proj*Time` family. A no-match answers -1, which is what the
        // docs say and what a `!= -1` test expects.
        if (name == "projhittime" || name == "projguardedtime" ||
            name == "projcontacttime") {
            if (isHelper_) return Value::number(-1.0f);
            const int mask = name == "projhittime"   ? maskHit
                             : name == "projguardedtime" ? maskGuarded
                                                         : maskContact;
            return Value::number(static_cast<float>(
                projectileContactTicks(contact, mask, argInt(0, 0))));
        }

        // The one-tick boolean forms. The suffix/argument is the projectile id
        // filter; zero or omitted means every projectile.
        int booleanMask = 0;
        bool suffixForm = false;
        int suffix = 0;
        if (name == "projhit") {
            booleanMask = maskHit;
        } else if (suffixNumber(name, "projhit", suffix)) {
            booleanMask = maskHit;
            suffixForm = true;
        } else if (name == "projguarded") {
            booleanMask = maskGuarded;
        } else if (suffixNumber(name, "projguarded", suffix)) {
            booleanMask = maskGuarded;
            suffixForm = true;
        } else if (name == "projcontact") {
            booleanMask = maskContact;
        } else if (suffixNumber(name, "projcontact", suffix)) {
            booleanMask = maskContact;
            suffixForm = true;
        }
        if (booleanMask != 0) {
            if (isHelper_) return boolValue(false);
            // A suffix form carries its id in the name; the bare/function form
            // carries it (or nothing) in the argument list.
            const int filter = suffixForm ? suffix : argInt(0, 0);
            return boolValue(projectileContactJustHappened(contact, booleanMask, filter));
        }
    }

    if (name == "movecontact") return Value::number(moveContact_);
    if (name == "movehit") return Value::number(moveHit_);
    if (name == "movereversed") return Value::number(moveReversed_);
    if (name == "moveguarded") return Value::number(moveGuarded_);
    if (name == "hitcount") return Value::number(static_cast<float>(hitCount_));
    if (name == "uniqhitcount") return Value::number(static_cast<float>(uniqHitCount_));
    if (name == "jugglepoints") return Value::number(static_cast<float>(jugglePoints_));
    // The shake is shorter than the stun it sits inside, which is the whole reason
    // these are two triggers: a character that wants to react the instant the
    // freeze ends asks `hitshakeover`, and one that wants to wait until it is free
    // again asks `hitover`. Both are the reference's own comparisons against its
    // live clocks -- `hitshakeover` is `+0x1090 <= 0` and `hitover` is
    // `+0x1094 < 0` -- which is why the second is tested against -1 and not 0.
    if (name == "hitshakeover") return boolValue(shakeTicks_ <= 0);
    if (name == "hitover") return boolValue(stunTicks_ < 0 && pauseTicks_ == 0);
    // Whether the current hit reaction is a fall, per the state the engine put
    // the victim in and whatever HitFallSet last wrote over it.
    if (name == "hitfall") return boolValue(hitFall());
    // `canrecover`: in the air, mid-hit, past the hit's no-recovery window, and
    // the hit allowed recovery at all. MUGEN's air-recovery states check this
    // before they listen for the tech input. The engine's own recovery check
    // asks the same function, so the trigger and the behaviour agree by
    // construction.
    if (name == "canrecover") return boolValue(canRecover());
    if (name == "inguarddist") return boolValue(inGuardDist_);
    if (name == "hitpause") return boolValue(pauseTicks_ > 0);
    // The attacker's own freeze remaining, which is how a move knows it is still
    // inside the impact it made.
    if (name == "hitpausetime") return Value::number(static_cast<float>(pauseTicks_));
    if (name == "hitdefattr") {
        // The attr of *this* machine's own HitDef, which is what a character
        // checks to continue a chain ("last thing I did was a normal attack").
        // Probe semantics are MUGEN's: the class letters are a subset test
        // (SCA matches an S move) and the type letters are exact (NA is not SA).
        // Arrives as hitdefattr("sca", "na") from the parser's attr fold.
        if (args.size() < 2) return boolValue(false);
        const HitDef& def = hitDef_.armed ? hitDef_ : lastUsedHitDef_;
        const std::string& text = def.attrText;
        if (text.empty()) return boolValue(false);
        const size_t comma = text.find(',');
        const std::string classes = text.substr(0, comma);
        const std::string type =
            comma == std::string::npos ? "" : std::string(trim(text.substr(comma + 1)));
        bool subset = true;
        for (const char c : classes) {
            if (args[0].str.find(c) == std::string::npos) {
                subset = false;
                break;
            }
        }
        return boolValue(subset && type == args[1].str);
    }
    if (name == "gethitvar") {
        // The values of the hit that last landed, which is how a hit state
        // knows how hard it was hit and how long to stay down.
        const std::string& key = args.empty() ? std::string() : args[0].str;
        if (key == "damage") return Value::number(static_cast<float>(lastHit_.damage));
        // The live hitstun clock, not the number the HitDef wrote: the reference
        // reads `+0x1094` in this getter, the same field `HitOver` compares, and
        // it counts down through 0 to -1. So `Time = GetHitVar(hittime)` inside a
        // reaction is a countdown, and after the reaction it reads -1.
        if (key == "hittime") return Value::number(static_cast<float>(stunTicks_));
        if (key == "animtype") return Value::number(static_cast<float>(lastHit_.animType));
        if (key == "xvel") return Value::number(lastHit_.pushX);
        // Negated like `fall.yvel` below, and for the same reason: the hit carries
        // its velocity in the engine's frame, where up is positive, and this
        // trigger answers in the character's frame, where down is positive.
        if (key == "yvel") return Value::number(-lastHit_.pushY);
        if (key == "fall") return boolValue(lastHit_.falls);
        // The three type keys are one value each out of the HitDef, in the
        // reference's numbering (1 high, 2 low, 3 trip, 0 none). `type` is the
        // odd one out: it reads as the legacy name for the ground type, the same
        // number `groundtype` reports. Nothing in the corpus reads it -- zero
        // uses against 110 for `groundtype` and 44 for `airtype` -- so the
        // reading is the documentation's and not a character's.
        if (key == "groundtype") return Value::number(static_cast<float>(lastHit_.groundType));
        if (key == "airtype") return Value::number(static_cast<float>(lastHit_.airType));
        if (key == "type") return Value::number(static_cast<float>(lastHit_.groundType));
        // `xveladd`/`yveladd` are not the victim's velocity and never were: the
        // reference defines them as the extra velocity MUGEN adds to the victim's
        // own *when it is KOed*, a separate quantity from either the hit's push or
        // the fighter's live motion. This engine carries no such add -- no HitDef
        // parameter feeds one -- and the corpus never reads these two keys (0 uses
        // across all 908 files), so the honest answer is 0. Returning `velX_`/
        // `-velY_` here, as this did, answered the victim's *current* velocity
        // under a name that means something else.
        if (key == "xveladd") return Value::number(0.0f);
        if (key == "yveladd") return Value::number(0.0f);
        // Note: `hitshakeover` is *not* a GetHitVar key -- the reference lists it
        // as a standalone trigger, and this machine answers it as one further down
        // (`if (name == "hitshakeover")`). It used to be duplicated here; a
        // character that wrongly wrote GetHitVar(hitshakeover) now reads 0 like any
        // other unknown key rather than a second, differently-sourced answer.
        if (key == "slidetime") return Value::number(static_cast<float>(lastHit_.slideTime));
        // The guard's control time, stored on the guard branch of `hitInfoFor` and
        // nowhere else. This is the single most consequential of the missing keys:
        // a block-stun that reads 0 ends on the block's first frame, so a character
        // written around `CtrlSet value = 1; trigger1 = Time = GetHitVar(ctrltime)`
        // never got its stun at all.
        if (key == "ctrltime") return Value::number(static_cast<float>(lastHit_.ctrlTime));
        // The gravity of the fall, and the one key in this family that a launched
        // victim cannot do without: the falling states are `physics = N`, so the
        // `VelAdd y = GetHitVar(yaccel)` inside them is the whole of the fall.
        if (key == "yaccel") return Value::number(lastHit_.yaccel);
        if (key == "hitshaketime") return Value::number(static_cast<float>(shakeTicks_));
        // The fall landing. `fall.damage` is the extra cost the bounce takes, and
        // `fall.yvel` is the bounce velocity in the character's own frame, so it is
        // negated on the way out: `info.fallVelY` is stored upward-positive (the
        // `fall.yvelocity = -4.5` default arrives as `+4.5`), and the corpus reads
        // `GetHitVar(fall.yvel)` only as `= 0`/`!= 0` -- 133 uses across 35 packs in
        // local_chars, every one of them a zero test -- to choose the slide or the
        // bounce branch in its 5100/5110 ground states, so the sign never moves a
        // branch; it only has to agree with what the number means.
        if (key == "fall.damage") return Value::number(static_cast<float>(lastHit_.fallDamage));
        if (key == "fall.yvel") return Value::number(-lastHit_.fallVelY);
        // `fall.envshake.time` is not in the reference's list of GetHitVar keys, but
        // the reference's own `FallEnvShake` entry is written in terms of it --
        // "effective only if GetHitVar(fall.envshake.time) is not zero, and it sets
        // GetHitVar(fall.envshake.time) to zero after being executed" -- so the key
        // exists, it is live rather than a snapshot (it counts down as the
        // controller spends it), and `fallShakeTime_` is where that lives.
        if (key == "fall.envshake.time") return Value::number(static_cast<float>(fallShakeTime_));
        // `hitcount` here is the hits *taken* in the current combo -- 62 uses
        // across 7 packs in local_chars, in two families: an attacker scaling its
        // own damage by reading the opponent's count
        // (`Enemy,GetHitVar(HitCount)`, Kira, Giorno, Vegeta) and a victim
        // gating its own AI while it is being hit (Vegetto, Diavolo, Jolyne,
        // Mista). Both want the same number and neither can be served by the
        // attacker's own `hitcount` trigger below, which counts a different
        // thing. `hitsTakenInCombo_` is that counter.
        if (key == "hitcount") return Value::number(static_cast<float>(hitsTakenInCombo_));
        // `recovertime`: the ticks left before a downed fighter gets up. The
        // reference's words are "Returns time before player gets up from liedown
        // state. This number counts down to 0 for each game tick, and will count
        // down faster if buttons are hit", and the second sentence is where a
        // file's expectations of it are set -- so it answers `liedownTicks_`,
        // which is that countdown, rather than a constant.
        //
        // It is the one GetHitVar key that is *live* rather than a snapshot of the
        // hit: a fighter on its feet reads the file's whole `[Data] liedown.time`
        // back, and one lying in 5110 reads what is left of it, because the
        // reference re-arms it every tick the fighter is not in a hit reaction.
        // That is what lets a character ask the question before the knockdown and
        // get the answer it actually wants -- the length of the lie it is about to
        // take -- instead of a zero that would read as "already up".
        //
        // The button-accelerated part of the documented behaviour is not
        // implemented: the engine has no evidence for the rate (how much faster,
        // per what input) and inventing one would put a wrong number under a
        // documented name. It counts down a tick a tick, which is the half of the
        // sentence that is certainly true.
        if (key == "recovertime") return Value::number(static_cast<float>(liedownTicks_));
        // `isbound`: whether this fighter is being *held* by someone else's bind.
        // The reference's definition is narrow and its purpose is stated with it --
        // "True if the player is the subject of an attacker's TargetBind
        // controller. Useful to prevent being stuck in thrown states."
        //
        // That second sentence is the whole reason the key exists, and it is not
        // hypothetical: it is written into MUGEN's own character, in its thrown
        // state, as `trigger1 = !gethitvar(isbound)` handing to 5050 -- "if I am
        // no longer held, go to the landing state rather than waiting in a throw
        // that has already ended". The failure of an unanswered key here is worse
        // than a missing feature: an unknown key reads 0, `!0` is true, so that
        // guard fires on the first tick of every throw -- the victim walks out of
        // the thrown state immediately and the throw holds nobody at all. Both of
        // the corpus's uses (two packs) are exactly this idiom.
        //
        // `bindTime_ != 0` is part of the answer, not just the pointer: a bind
        // whose time has run out is released on the next `applyBind`, so for the
        // rest of the tick it expired on the fighter is still pointed at its
        // anchor but is no longer held. A negative time means held indefinitely.
        //
        // The reference documents this for TargetBind; the engine answers it for
        // any of the three binds (TargetBind, BindToRoot, BindToParent), which are
        // the same state to a character asking whether it is free to move.
        if (key == "isbound") {
            return boolValue(bindMachine_ != nullptr && bindTime_ != 0 && !bindMachine_->doomed());
        }
    }

    // ------------------------------------------------------- ifelse and cond
    //
    // `ifelse(cond, a, b)` is an *expression*, not a controller, and it is everywhere:
    // `anim = ifelse(var(10) = 1, 1205, 1200)`, and state exits like
    // `trigger1 = ifelse(statetype = A, time = 7, animtime = 0)`. A parser that does not
    // know it evaluates the whole trigger to zero, which is not a small error -- it is a
    // trigger that is false for ever, so a move that waits on one never ends. Five of
    // the MUGEN-install characters stalled in exactly that way.
    //
    // The arguments arrive already evaluated (the expression parser evaluates a trigger's
    // arguments before calling here), so both branches are computed even though only one
    // is returned. That is MUGEN's behaviour too, and it is why these are functions of
    // values rather than a control-flow construct.
    if (name == "ifelse") {
        if (args.size() < 3) return Value::number(0.0f);
        return args[0].truthy() ? args[1] : args[2];
    }
    // `cond(c1, v1, c2, v2, ..., default)`: the first true condition's value, or the last
    // argument when none is true. An even number of arguments means no default, which
    // reads as zero.
    if (name == "cond") {
        for (size_t i = 0; i + 1 < args.size(); i += 2) {
            if (args[i].truthy()) return args[i + 1];
        }
        return Value::number(args.size() % 2 == 1 ? args.back().toFloat() : 0.0f);
    }

    // --------------------------------------------------------------- input
    if (name == "command") {
        if (commands_ == nullptr || args.empty()) return Value::text("");
        const std::string& wanted = args[0].str;
        return boolValue(commands_->triggered(wanted, buffer_));
    }
    if (name == "random") {
        return Value::number(static_cast<float>(nextRandom(randomSeed_) % 1000));
    }

    // ------------------------------------------------------------- the store
    if (name == "var") {
        const int index = argInt(0, -1);
        if (index < 0 || index >= static_cast<int>(vars_.size())) return Value::number(0.0f);
        return Value::number(static_cast<float>(vars_[static_cast<size_t>(index)]));
    }
    if (name == "fvar") {
        const int index = argInt(0, -1);
        if (index < 0 || index >= static_cast<int>(fVars_.size())) return Value::number(0.0f);
        return Value::number(fVars_[static_cast<size_t>(index)]);
    }
    if (name == "sysvar") {
        const int index = argInt(0, -1);
        if (index < 0 || index >= static_cast<int>(sysVars_.size())) return Value::number(0.0f);
        return Value::number(static_cast<float>(sysVars_[static_cast<size_t>(index)]));
    }
    if (name == "sysfvar") {
        const int index = argInt(0, -1);
        if (index < 0 || index >= static_cast<int>(sysFVars_.size())) return Value::number(0.0f);
        return Value::number(sysFVars_[static_cast<size_t>(index)]);
    }

    // ---------------------------------------------------------- the character
    if (name == "name") return Value::text(character_ != nullptr ? character_->name() : "fighter");
    if (name == "p1name" || name == "p2name") return Value::text(this->name());
    if (name == "authorname") return Value::text("");
    if (name == "const") return constTrigger(args);

    // Everything else about the opponent reads through the redirect. Handling
    // the common ones here as well means a character that writes `p2life`
    // instead of `enemynear, life` still gets an answer.
    return opponentTrigger(name, args);
}

Value StateMachine::opponentTrigger(std::string_view name, const std::vector<Value>& args) const {
    (void)args;
    if (opponent_ == nullptr) return Value::number(0.0f);

    if (name == "p2life") return Value::number(static_cast<float>(opponent_->life_));
    if (name == "p2power") return Value::number(static_cast<float>(opponent_->power_));
    if (name == "p2stateno") return Value::number(static_cast<float>(opponent_->stateNumber_));
    if (name == "p2facing") return Value::number(static_cast<float>(opponent_->facing_));
    if (name == "p2anim") return Value::number(static_cast<float>(opponent_->anim_));
    if (name == "p2movetype") {
        switch (opponent_->moveType_) {
            case MoveType::Idle: return Value::text("i");
            case MoveType::Attack: return Value::text("a");
            case MoveType::Hit: return Value::text("h");
            case MoveType::Unchanging: return Value::text("u");
        }
        return Value::text("i");
    }
    if (name == "p2statetype") {
        switch (opponent_->stateType_) {
            case StateType::Stand: return Value::text("s");
            case StateType::Crouch: return Value::text("c");
            case StateType::Air: return Value::text("a");
            case StateType::Liedown: return Value::text("l");
        }
        return Value::text("s");
    }
    if (name == "p2bodydist x") return Value::number(p2BodyDistX());
    if (name == "p2bodydist y") return Value::number(opponent_->y_ - y_);
    if (name == "p2dist x") return Value::number(p2DistX());
    if (name == "p2dist y") return Value::number(opponent_->y_ - y_);

    // An identifier this machine does not know is a *string literal* in MUGEN,
    // not a missing value. `statetype = A` is the case that matters and it is
    // everywhere: the A is written bare, and it has to compare equal to what
    // `statetype` returns. Short names are the shape those constants come in
    // (A, S, C, L, U, NA, SA, HA, SCA, HT, NT, AT), so they are answered as
    // strings; anything longer is a trigger name this machine does not have and
    // reads as zero rather than as an accidental truth.
    if (name.size() <= 4) {
        bool letters = true;
        for (const char c : name) {
            if (c < 'a' || c > 'z') {
                letters = false;
                break;
            }
        }
        if (letters) return Value::text(std::string(name));
    }
    return Value::number(0.0f);
}

Value StateMachine::constTrigger(const std::vector<Value>& args) const {
    if (character_ == nullptr || args.empty()) return Value::number(0.0f);
    const std::string& key = args[0].str;
    if (key.empty()) return Value::number(0.0f);

    // Every [Data]/[Size]/[Velocity]/[Movement] key is already indexed under
    // its `const()` spelling, so this is a lookup rather than a switch with a
    // branch per constant.
    float value = 0.0f;
    if (character_->constant(key, &value)) return Value::number(value);
    return Value::number(0.0f);
}

// `var(0) := 5` from inside an expression. Out-of-range writes are dropped --
// the binary's own answer is a "set var out of range" warning, which reads as
// exactly that with the write skipped.
bool StateMachine::assignVar(char kind, int index, float value) const {
    switch (kind) {
        case 'v':
            if (index < 0 || index >= static_cast<int>(vars_.size())) return false;
            vars_[static_cast<size_t>(index)] = static_cast<int>(std::lround(value));
            return true;
        case 'f':
            if (index < 0 || index >= static_cast<int>(fVars_.size())) return false;
            fVars_[static_cast<size_t>(index)] = value;
            return true;
        case 's':
            if (index < 0 || index >= static_cast<int>(sysVars_.size())) return false;
            sysVars_[static_cast<size_t>(index)] = static_cast<int>(std::lround(value));
            return true;
        case 'S':
            if (index < 0 || index >= static_cast<int>(sysFVars_.size())) return false;
            sysFVars_[static_cast<size_t>(index)] = value;
            return true;
        default:
            return false;
    }
}

// ------------------------------------------------------------------------ AI

void StateMachine::controlAI() {
    aiControlled_ = true;
    InputState input;

    // Being hit is not a decision. A fighter in hitstun cannot act, and pressing
    // buttons through it only loads the command buffer with moves it was never
    // allowed to start.
    if (stunTicks_ > 0) {
        control(input);
        return;
    }

    if (!aiScript_.empty()) {
        // Step through the move's command, one element at a time. A released
        // element is fed as no input at all, which is what a `~` element asks
        // for; everything else is held for a couple of ticks so the press is
        // seen.
        const CmdStep& step = aiScript_[aiScriptStep_];
        if (step.release) {
            input.held = 0;
            ++aiScriptStep_;
            aiScriptTicks_ = 0;
        } else {
            input.held = heldFromSymbols(step.symbols, facing_);
            const int ticks = std::max(2, step.holdTicks);
            if (++aiScriptTicks_ >= ticks) {
                ++aiScriptStep_;
                aiScriptTicks_ = 0;
            }
        }
        if (aiScriptStep_ >= aiScript_.size()) aiScript_.clear();
        control(input);
        return;
    }

    const float gap = opponent_ != nullptr ? std::fabs(opponent_->x_ - x_) : 999.0f;
    // A fighter can hold a move only from its feet and from a neutral move type.
    // Pressing a command while a move is already out is how a character's own
    // `trigger2 = stateno = 400 && time > 5` chain links are reached, and it is
    // also how a CPU that presses at random looks like it is flailing.
    const bool canStartMove = moveType_ != MoveType::Attack && physics_ != Physics::Air;
    // Guarding in MUGEN is not a button: it is *holding back*, and it is the only
    // defensive act the format gives a fighter. A CPU that never holds back stands
    // in front of an attacking opponent and takes the whole match, which is most
    // of what "the AI is broken" looked like.
    const bool threatened =
        opponent_ != nullptr && opponent_->moveType_ == MoveType::Attack && gap < 110.0f;

    if (--aiTicks_ <= 0) {
        // A higher difficulty decides more often as well as more aggressively,
        // which is the difference between a CPU that reacts and one that is
        // merely lucky.
        aiTicks_ = std::max(3, 18 - aiLevel_ * 2);
        const int roll = static_cast<int>(nextRandom(aiSeed_) % 100);

        if (gap > 140.0f) {
            aiWalk_ = kAiToward;
            if (roll < 8) beginAiMove(true);
        } else if (gap > 60.0f) {
            if (roll < 58) {
                aiWalk_ = kAiToward;
            } else if (roll < 88 && canStartMove) {
                beginAiMove(roll < 78);
            } else {
                aiWalk_ = kAiHold;
            }
        } else if (canStartMove) {
            if (roll < 45 + aiLevel_ * 4) {
                beginAiMove(false);
            } else if (roll < 75) {
                aiWalk_ = kAiBack;
            } else if (roll < 90) {
                aiWalk_ = kAiToward;
            } else {
                aiWalk_ = kAiHold;
            }
        } else {
            aiWalk_ = kAiHold;
        }
    }

    if (!aiScript_.empty()) {
        // beginAiMove() armed a move this tick; play its first element now so
        // the decision is not a tick late.
        controlAI();
        return;
    }

    // Walking and guarding are both *commands* in MUGEN -- `holdfwd`, `holdback`
    // -- so the AI presses the direction and the character's own walk state does
    // the rest, which is why the CPU walks at the character's speed. Forward and
    // back are written the way a character writes them, relative to facing, and
    // not as left and right.
    switch (aiWalk_) {
        case kAiToward: input.held = heldFromSymbols(CmdKey::kForward, facing_); break;
        case kAiBack: input.held = heldFromSymbols(CmdKey::kBack, facing_); break;
        default: break;
    }
    if (input.held == 0 && threatened) input.held = heldFromSymbols(CmdKey::kBack, facing_);
    control(input);
}

void StateMachine::beginAiMove(bool jumpFirst) {
    aiScript_.clear();
    aiScriptStep_ = 0;
    aiScriptTicks_ = 0;
    aiWalk_ = kAiHold;

    if (commands_ != nullptr) {
        // Only commands that name a button are attacks; the rest are the holds
        // and the directional commands the walk logic already uses.
        const std::vector<CommandDef>& all = commands_->commands();
        std::vector<const CommandDef*> attacks;
        for (const CommandDef& command : all) {
            if (command.steps.empty() || command.steps.size() > 4) continue;
            bool hasButton = false;
            for (const CmdStep& step : command.steps) {
                if ((step.symbols & ~CmdKey::kDirections) != 0) {
                    hasButton = true;
                    break;
                }
            }
            if (hasButton) attacks.push_back(&command);
        }
        if (!attacks.empty()) {
            // Normals come up more often than specials: a one- or two-element
            // command is a normal, and a CPU that opened with a super every time
            // would be neither fun nor fair.
            const bool wantSimple = nextRandom(aiSeed_) % 100 < 70;
            std::vector<const CommandDef*> pool;
            for (const CommandDef* command : attacks) {
                if (wantSimple == (command->steps.size() <= 2)) pool.push_back(command);
            }
            if (pool.empty()) pool = attacks;
            const CommandDef* chosen = pool[static_cast<size_t>(nextRandom(aiSeed_) %
                                                                  pool.size())];
            aiScript_ = chosen->steps;

            if (jumpFirst) {
                // Jumping in: the direction is folded in front of the move, so
                // the character leaves the ground and then does the move in the
                // air where it is allowed to. Forward is toward the opponent, which
                // is the way a jump-in has to go -- the earlier version read the
                // walk direction here *after* clearing it, so every jump-in went
                // the same way regardless.
                CmdStep jump;
                jump.symbols = CmdKey::kUp | CmdKey::kForward;
                jump.hold = true;
                jump.holdTicks = 2;
                aiScript_.insert(aiScript_.begin(), jump);
            }
            return;
        }
    }

    // A character with no commands yet still has hands. Pressing a button is
    // what a basic attack is in the format, and any character with basic attack
    // states answers it.
    static const char kLetters[6] = {'x', 'y', 'z', 'a', 'b', 'c'};
    CmdStep step;
    step.symbols = CmdKey::button(kLetters[nextRandom(aiSeed_) % 6]);
    aiScript_.push_back(step);
}

}  // namespace af

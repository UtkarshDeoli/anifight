// The MUGEN state machine, running.
//
// This is what turns a character from a set of animations into a fighter that
// plays the way its author wrote it. Once a tick it does what MUGEN does, in
// the same order:
//
//   1. Record this tick's input, then run state -1 -- the command state, where
//      every `command = "..."` trigger is answered. This is why the order
//      matters: a command detected here can change the state below, and the
//      new state's own controllers then still get their turn this tick.
//   2. Run the current state's controllers in file order. Each is a type (what
//      to do) plus triggers (whether to do it). A ChangeState stops the list.
//   3. Run physics for the state's `physics` mode: gravity and ground contact
//      for an airborne state, nothing for a state that has opted out.
//
// Everything a character can read about itself or its opponent is answered by
// `trigger()`, which is the whole of engine/expr.h's TriggerContext.
//
// What is deliberately missing: nothing structural any more. Helpers are real entities
// (their own StateMachine, spawned through the host's HelperHost and linked through
// parent_/root_/helpers_ so the family redirects resolve), and targets are tracked on
// contact so `target, ...` and bindtotarget work. Team modes (partners, simul, turns) are
// the one population this model does not describe: a match here is still two families.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "audio/mixer.h"
#include "engine/cmd.h"
#include "engine/cns.h"
#include "engine/character.h"
#include "engine/expr.h"
#include "engine/input.h"
#include "engine/projectile_rules.h"  // ProjectileContactRecord, for the Proj* triggers
#include "gfx/renderer.h"  // TransParams, for the trans and AfterImage knobs

namespace af {

class AnimationSet;

// A palette's identity in a player's sprite set, as MUGEN's `RemapPal` writes
// it: `source = src_group, src_item`, `dest = dst_group, dst_item`
// (sctrls.html). The group is the SFF v2 palette node's group coordinate, not
// a sprite group; item is the palette number within it. Group 1 is the
// numbering `palno` and `pal1..pal12` use, so an index-only `source = 3` is
// the legacy spelling of `{1, 3}`.
struct PaletteRef {
    int group = 1;
    int item = 1;
    bool operator==(const PaletteRef&) const = default;
};

// One redirection. MUGEN keeps these as a map from source palette to dest
// palette: writing the same source twice replaces the destination rather than
// stacking.
struct PaletteRemap {
    PaletteRef source;
    PaletteRef dest;
};

// The numbers MUGEN gives its common states. Characters define most of these
// themselves, but a state that is only *reached* and never defined still has to
// do something sane, and these are what this machine falls back to.
namespace CommonState {
constexpr int kStand = 0;
constexpr int kStandToCrouch = 10;   // transition into crouching
constexpr int kCrouch = 11;
constexpr int kCrouchToStand = 12;   // transition out of crouching
constexpr int kWalk = 20;
// Walking away is state 20 as well, and 21 is an *animation* rather than a
// state: `common1.cns` has no `[Statedef 21]` at all, and its own state 20
// chooses between the two walks by the sign of the velocity --
// `[State 20, 4] ChangeAnim / triggerall = vel x < 0 / value = 21`. Walking back
// in a state of its own put a fighter into a statedef no character defines, so
// the character's own walk state -- and the art choice inside it -- was never
// reached at all, and a file-less fighter walked backwards in the forward art.
constexpr int kWalkBackAnim = 21;
constexpr int kJumpStart = 40;       // takeoff
// `AirJump Start`, which `common1.cns` defines as 45 and which no built-in route
// reaches -- this engine has no air jump. The row exists so that a character (or a
// future engine path) that enters 45 gets the reference's two-tick takeoff and its
// `Time = 2` hand-over to 50 rather than the generic one-tick pass-through.
constexpr int kAirJumpStart = 45;
constexpr int kAir = 50;
// "Jump Down (empty state for compatibility)", the reference's own heading for
// 51. It is the state a downward jump would have used, kept empty for the
// formats that came before, and it is not dead: `Guard (end)` exits into it, so
// a fighter that releases its block in the air goes back to 51 and not to 50.
constexpr int kAirDown = 51;
constexpr int kLand = 52;
constexpr int kRunFwd = 100;         // also "run" in older characters
constexpr int kHop = 105;
// `Hop backwards (land)`: the reference's 105 hands over to 106 when the hop
// touches the floor, and 106 holds for a fixed seven ticks (`[State 106, 5]
// ChangeState / trigger1 = Time = 7`) before it gives control back to state 0.
constexpr int kHopLand = 106;
constexpr int kRunBack = 101;
constexpr int kGuarded = 150;
constexpr int kGuardHit = 152;
constexpr int kGuardedAir = 154;
// The knock-back half of each guard reaction, which the shake hands over to when
// `HitShakeOver` comes: 151 standing, 153 crouching, 155 airborne. They are where
// a block's push is actually applied -- each opens with `HitVelSet x = 1`, and
// 155 with `x = 1, y = 1` -- because the shake in front of them is written
// `velset = 0,0` and holds the blocker still. Each ends in a guard hold: 151 in
// 130, 153 in 131, and 155 on the floor, where it chooses between 130 and 52.
constexpr int kGuardedKb = 151;
constexpr int kGuardHitKb = 153;
constexpr int kGuardedAirKb = 155;
// The guard states, under the reference file's own headings: `GUARD (start)`
// 120, `Stand guard (guarding)` 130, `Crouch guard (guarding)` 131, `Air guard
// (guarding)` 132, and `Guard (end)` 140.
//
// The shape of the chain is the file's, and it is worth writing down because
// the three held states are not the only ones 140 arrives from:
//
//   120 --AnimTime = 0--> 130/131/132     (the anim is 120/121/122, and the
//                                          state type it ends in is the one it
//                                          was left with)
//   120/130/131/132 --!holdback or !inguarddist--> 140
//   130 <--holddown--> 131
//   140 --AnimTime = 0--> 0, 11 or 51     (stand, crouch, air -- this hand-over
//                                          is commented out in the file and is
//                                          the engine's, which is what `Guard
//                                          (end)`'s `ctrl = 1` is for: the
//                                          release gives control back on its
//                                          first tick and the pose plays out
//                                          behind whatever the fighter does
//                                          next)
//
// `120` and `140` are entered with `type = U, physics = U` -- they change
// neither, so a crouching blocker stays crouched through its release -- and
// neither writes an animation but as `120 + (statetype = C) + (statetype = A)*2`.
constexpr int kGuardStart = 120;
constexpr int kGuardStand = 130;
constexpr int kGuardCrouch = 131;
constexpr int kGuardAir = 132;
constexpr int kGuardEnd = 140;
// The get-hit states. The numbers are the reference's, and so are the moves
// between them: these are the states `data/common1.cns` defines and the engine
// only supplies when a character ships no common state file of its own, so a
// built-in that used a different number would put a file-less character in a
// state no file agrees with. The names are the reference's own section headings
// (`; HITG_SHAKE`, `; HITA_FALL`, ...).
//
// The chain a knocked-away victim walks is worth spelling out, because the
// numbers are not in the order they are walked in:
//
//   5020 HITA_SHAKE  -> 5030 HIT_AIRFALL -> 5050 HITA_FALL -> 5100 -> 5110
//                                          \ 5040 HITA_RECOV -> 52 (on the floor)
//
// 5040 is the odd one: despite sitting between the fall and the landing, it is
// *not* a landing state. It is "air get-hit (recovering in air, not falling)" --
// where a victim goes when the hit knocked it into the air without putting it
// into a fall -- and its own exit is to the jump-land state, on its feet. 5050
// is the falling state, and the floor it reaches is 5100, not itself.
// The two grounded shakes, and the choice between them is the *victim's* posture
// rather than the attack's height: 5000 is `Stand get-hit (shaking)`, `type = S`,
// and 5010 is `Crouch get-hit (shaking)`, `type = C`. A standing victim of a
// `ground.type = Low` attack still enters 5000 -- the hit type is what 5000 draws
// its *animation* from (`value = ifelse((GetHitVar(groundtype) = 1),5000,5010) +
// GetHitVar(animtype)`), which is the whole reason it exists as a separate
// choice, and the binary reads the state number off the victim's statetype
// (`0 < statetype - 1 ? 5010 : 5000`, FINDINGS pass 12 §12.2). Each hands over to
// the knocked-back state beside it -- 5000 to 5001, 5010 to 5011 -- and 5011
// returns the victim to the crouch (11) rather than standing them up.
constexpr int kHitHigh = 5000;
constexpr int kHitLow = 5010;
constexpr int kHitHighKb = 5001;     // HITG_SLIDE -- the standing knock-back
constexpr int kHitLowKb = 5011;      // HITC_SLIDE -- and the crouching one
constexpr int kHitAir = 5020;        // HITA_SHAKE -- an airborne victim's hit shake
constexpr int kHitAirFall = 5030;    // HIT_AIRFALL -- knocked away, going down
// `HIT_AIRFALL`'s transition half: `[State 5030, 6] ChangeState / trigger1 =
// AnimTime = 0 / value = 5035`. It is a pose the victim passes through while the
// 5030 art runs out, and it exits on the same split as 5030 -- 5040 or 5050 from
// `HitFall` -- on `HitOver`, its own animation, or the floor.
constexpr int kHitAirTransition = 5035;
// HITA_RECOV: knocked into the air but *not* falling. Its only exits are to
// 5050 when the victim is dead and to the jump-land state when it reaches the
// floor -- a victim recovering in the air lands on its feet.
constexpr int kHitAirRecover = 5040;
constexpr int kHitFall = 5050;       // HITA_FALL -- the fall itself
// The three states a fall *recovers* into, which is the one way out of 5050
// that does not go through the floor. All three are the reference's own
// (`data/common1.cns`), and so are their names: `[State 5050, 4]` sends the
// victim to ``value = 5200 ;HITFALL_RECOVER`` when recovery is pressed close to
// the ground, `[State 5050, 5]` sends it to ``value = 5210 ;HITFALL_AIRRECOVER``
// when it is pressed anywhere higher, and 5200's own exit is the `SelfState
// 5201` on ``Pos Y >= 10``.
//
// The distinction between 5200 and 5210 is the height, and it is written into
// the states themselves: 5200 is entered within `movement.air.gethit.
// groundrecover.ground.threshold` (the reference's -20) of the floor and sinks
// the rest of the way into it -- its own comment calls it "recovering near
// ground", and 5201 is the state it lands in, whose `PosSet y = 0` and
// `VelSet y = -3.5` pop the fighter back out of the floor and onto its feet.
// 5210 is the mid-air version: the fighter rolls where it is, four ticks of
// frozen position and then a lift, and lands on its feet from there.
//
// Neither is reached from 5071, the trip's fall, which carries no recovery
// controllers at all in the reference -- a tripped fighter stays tripped.
constexpr int kHitFallRecover = 5200;        // HITFALL_RECOVER -- near the ground
constexpr int kHitFallRecoverGround = 5201;  // HIT_FALLRECOVER (on the ground)
constexpr int kHitAirFallRecover = 5210;     // HITFALL_AIRRECOVER -- in mid air
constexpr int kHitTrip = 5070;       // HIT_TRIP -- the stumble before the fall
constexpr int kHitTripFall = 5071;   // HIT_TRIP2 -- and the fall it turns into
// 5080 and its 5081 follow-on: what the engine puts a victim in when it is hit
// while already lying down, which is the one get-hit state the *engine* chooses
// rather than the victim's height -- the reference's hit path tests the victim's
// state type and hard-codes this number for an `L` one. Which of the two the
// victim ends up in is the file's own call, and 5080 makes it from
// `GetHitVar(yvel)`: a non-zero y sends it back into the air as 5030, zero slides
// it along the floor as 5081.
constexpr int kHitLiedown = 5080;
constexpr int kHitLiedownKb = 5081;
// HIT_BOUNCE: the floor, hit from a fall. This is where a fall ends, and it is
// reached from 5050 rather than from the fall's own state -- 5050's exit is
// `Vel Y > 0 && Pos Y >= groundlevel`, and it is a ChangeState *to 5100*.
constexpr int kHitGround = 5100;
constexpr int kHitBounce = 5101;     // and the bounce back off it
constexpr int kDowned = 5110;        // lying on the ground
constexpr int kDownedGetUp = 5120;   // standing up
// Knocked out and lying defeated. It is a *lying* state, not a fall, and it has
// no exit -- a dead fighter is stood up by the engine at the round's end and not
// by anything in its own file. The states this engine used to carry above it
// (5160, 5170) do not exist: 5140, 5160 and 5170 are *animations* in the
// reference, and no pack in the corpus defines a statedef for any of them.
constexpr int kDead = 5150;
constexpr int kWin = 180;
constexpr int kIntro = 190;
// The intro override point: `[Statedef 191]` is where a character gives itself an
// intro, and the reference's built-in version is a single `ChangeState 0` at
// `Time = 0`. A character that defines 191 runs its own; this is the fallback.
constexpr int kIntroPoint = 191;
// The reference's round-initialization state. It lasts exactly one tick and
// exists for two jobs: clear the character's variables when this is the first
// round of the match (`roundsexisted = 0`), and hand over to 190 on round one
// or to state 0 on every round after it. The engine resolves it inside
// `setup()` because its intro is a screen phase rather than a state, but a
// character that writes `ChangeState 5900` still gets the same hand-over.
constexpr int kRoundInit = 5900;
// `Continue screen animation`, held until the continue screen leaves it. There is
// no continue screen in this engine, so the row is inert; it is here so the table
// does not disagree with the reference's list, and it has no exit.
constexpr int kContinue = 5500;
}  // namespace CommonState

// One fighter's running state machine.
class EntitySink;

class StateMachine : public TriggerContext {
public:
    StateMachine() = default;

    // Points at a character and starts it in its own state 0. `character` may
    // be null, which makes the machine a placeholder that stands still -- the
    // engine test's dummy and the fighter drawn as a box.
    void setup(const Character* character, int facing);

    // The other fighter, for every p2/enemynear trigger. Not owned.
    void setOpponent(StateMachine* opponent) { opponent_ = opponent; }
    [[nodiscard]] StateMachine* opponent() const { return opponent_; }

    // ------------------------------------------------------------ the family
    //
    // A helper is a second machine running the same character's states, spawned by
    // the `Helper` controller and owned by the fight; this machine is then its
    // `parent`, its family anchor is the `root` (the real fighter at the top of the
    // spawn chain), and the family's helpers all live on the root's list so that
    // `helper(id)` and `numhelper(id)` see the whole of it from anywhere inside.
    //
    // Called by the host that answered the spawn, once, after setup().
    void becomeHelper(StateMachine* parent, int helperId, std::string name, bool keyCtrl,
                      int pauseMoveTime, int superMoveTime, int stateNo);
    [[nodiscard]] bool isHelper() const { return isHelper_; }
    [[nodiscard]] int helperId() const { return helperId_; }
    [[nodiscard]] StateMachine* parent() { return parent_; }
    [[nodiscard]] const StateMachine* parent() const { return parent_; }
    [[nodiscard]] StateMachine* root() { return root_; }
    [[nodiscard]] const StateMachine* root() const { return root_; }
    // The family's helpers, for whoever owns the fighters (sweeps) and for the
    // redirects that pick among them. Empty on a helper itself.
    [[nodiscard]] const std::vector<StateMachine*>& familyHelpers() const {
        return root_->helpers_;
    }
    void removeFamilyHelper(StateMachine* machine);
    // Drops the weak links (target, bind anchor) that point at a machine which is
    // about to be destroyed. Called by the host's sweep for every machine that
    // stays, against every machine that goes.
    void clearReferencesTo(StateMachine* machine);
    // How many of the family's helpers are still around (optionally with one id).
    // A helper counts the family it belongs to, as MUGEN does.
    [[nodiscard]] int countHelpers(int id) const;

    // `DestroySelf`: flags this machine for removal. Only meaningful on a helper --
    // the runController case refuses it for a root, because a fighter that deleted
    // itself would take the match with it. A destroyed helper's own helpers go with
    // it, which is what `doomed` reports.
    void markDestroyed() { destroyed_ = true; }
    [[nodiscard]] bool destroyed() const { return destroyed_; }
    [[nodiscard]] bool doomed() const;
    // Still moving while the fight around it is frozen: pausemovetime/supermovetime.
    [[nodiscard]] int pauseMoveTime() const { return pauseMoveTime_; }
    [[nodiscard]] int superMoveTime() const { return superMoveTime_; }
    [[nodiscard]] bool keyCtrl() const { return keyCtrl_; }

    // MUGEN's player ids. Roots are 1 and 2 as MUGEN's own are; helpers take ids the
    // host hands out. Read by `playeridexist` and the `playerid(n)` redirect.
    void setPlayerId(int id) { playerId_ = id; }
    [[nodiscard]] int playerId() const { return playerId_; }

    // A helper can start in the air: the spawner resolves a `postype` y into a
    // height before this machine exists, and Fighter::setup's startX path has no
    // room for it.
    void setHeight(float y) { y_ = y; }

    // ------------------------------------------------------------ per tick
    //
    // Input from a human, or from the AI. `aiLevel` is 0..9 and only changes
    // what `ailevel` reports and how the built-in AI plays.
    void control(const InputState& input);
    void controlAI();

    // State -1 then the current state, then physics. Called after control().
    void step();

    // -------------------------------------------------------------- queries
    [[nodiscard]] int stateNumber() const { return stateNumber_; }
    [[nodiscard]] int prevStateNumber() const { return prevStateNumber_; }
    [[nodiscard]] int stateTime() const { return stateTime_; }
    [[nodiscard]] bool ctrl() const { return ctrl_; }
    [[nodiscard]] StateType stateType() const { return stateType_; }
    [[nodiscard]] MoveType moveType() const { return moveType_; }
    [[nodiscard]] Physics physics() const { return physics_; }

    [[nodiscard]] int animNumber() const { return anim_; }
    [[nodiscard]] long long animTime() const { return animTime_; }

    [[nodiscard]] float x() const { return x_; }
    [[nodiscard]] float y() const { return y_; }
    [[nodiscard]] float velX() const { return velX_; }
    [[nodiscard]] float velY() const { return velY_; }
    [[nodiscard]] int facing() const { return facing_; }

    [[nodiscard]] int life() const { return life_; }
    [[nodiscard]] int maxLife() const { return maxLife_; }
    [[nodiscard]] int power() const { return power_; }
    [[nodiscard]] bool knockedOut() const { return life_ <= 0; }

    [[nodiscard]] const Character* character() const { return character_; }
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] const CommandBuffer& buffer() const { return buffer_; }

    // ----------------------------------------------------------- win quotes
    //
    // `VictoryQuote` selects a quote for the victory screen that follows the
    // *match*. The controller runs on both players, so the choice belongs to
    // the machine that ran it and the victory flow reads it from the winner
    // alone. -1 is both the controller's documented default and "nothing
    // selected": either way the screen chooses a random quote from the
    // character's `[Quotes]`.
    //
    // The selection is deliberately not cleared by setup(). setup() runs at
    // the start of every *round* (FightScreen::startRound), and a quote chosen
    // in round one must survive to the card shown after round three.
    // FightScreen::setupMatch clears it once per match, which is exactly the
    // scope the reference gives the controller.
    static constexpr int kVictoryQuoteRandom = -1;
    [[nodiscard]] int victoryQuoteIndex() const { return victoryQuoteIndex_; }
    void resetVictoryQuote() { victoryQuoteIndex_ = kVictoryQuoteRandom; }
    // A roll for the screen's random pick, from the same generator the
    // `random` trigger and Explod's displacement use. The match has one RNG
    // stream; giving each screen its own would make the quote repeat whenever
    // the rest of the match did.
    [[nodiscard]] int nextVictoryQuoteRoll() const;

    // Where the machine wants the fighter drawn: the animation for the current
    // state, or nullptr when there is nothing to draw.
    [[nodiscard]] const Animation* currentAnimation() const;
    [[nodiscard]] int animationFrameIndex() const;
    // The sprite bank matching whatever file the current animation comes from:
    // the character's own, the attacker's while a ChangeAnim2 poses this fighter,
    // or the attacker's whole file while a `p2stateno` hit has it running that
    // file's states. The two must agree or a victim would draw its own sprite with
    // the attacker's frame numbers.
    [[nodiscard]] const Character* animCharacter() const;
    // The file whose states this fighter is running, which is its own unless a
    // `p2stateno` hit has handed it the attacker's. Everything that resolves a
    // state number -- the state table, the `-1`/`-2`/`-3` controller lists, the
    // "does your file define this" questions -- goes through here rather than
    // through `states_`, because in MUGEN the pair of files travels together:
    // "P2 will get P1's state and animation data."
    [[nodiscard]] const Character* stateCharacter() const {
        return borrowedStateChar_ != nullptr ? borrowedStateChar_ : character_;
    }
    [[nodiscard]] const StateLibrary* stateLibrary() const {
        return borrowedStateChar_ != nullptr ? &borrowedStateChar_->states() : states_;
    }
    // The sprite priority the current state asked for, for draw ordering.
    [[nodiscard]] int spritePriority() const { return spritePriority_; }

    // ---------------------------------------------------------- hit handling
    // A hit landed on this fighter. Damage and the reaction come from the
    // attacker's HitDef, which is why they arrive together.
    //
    // The tiebreak class of `priority = hit_prior, hit_type`, declared here rather
    // than beside HitDef because a HitInfo carries one too: a hit that is already
    // on its way to a victim, or waiting on the floor for the fall it caused,
    // still has to answer for the class of the HitDef that made it.
    enum class PriorityClass {
        Dodge,
        Hit,
        Miss,
    };

    // Whether an attack of class `mine` still lands when it meets one of class
    // `theirs` with the same priority -- the reference's six-row table, asked one
    // side at a time. Its rows are (Hit,Hit) both land, (Hit,Miss) the hit lands
    // and the miss does not, (Hit,Dodge) neither, (Dodge,Dodge) neither,
    // (Dodge,Miss) neither, (Miss,Miss) neither; which reduces to this: a hit
    // lands unless it is not a `Hit`, or the other side is a `Dodge`. The classes
    // exist for throws -- "P1 and P2 should not be able to simultaneously hit each
    // other" -- so a throw that could be traded with another throw would be decided
    // by nothing but which fighter the fight loop happened to check first.
    //
    // A tie that produces no hits leaves both HitDefs enabled, the reference says
    // in as many words, which is why the caller must fall through without spending
    // the armed hit rather than calling `tryHit` and taking the miss.
    [[nodiscard]] static constexpr bool clashLands(PriorityClass mine, PriorityClass theirs) {
        return mine == PriorityClass::Hit && theirs != PriorityClass::Dodge;
    }

    struct HitInfo {
        int damage = 0;
        float pushX = 0.0f;
        float pushY = 0.0f;
        int hitTime = 12;      // ticks of hitstun before returning to state 0
        // Whether this hit *leaves the victim's hitstun clock alone* instead of
        // seeding it. Only an air block does: the reference's block path writes
        // the blocker's `+0x1090` (shake) and `+0x109C` (ctrl) and deliberately
        // not `+0x1094` (hittime) or `+0x1098` (slidetime) -- see
        // GUARD-CHAIN-RE.md §1.2, `0x46E8D9`-`0x46E950`. An airborne blocker
        // therefore keeps whatever hitstun it already had, and one that was not
        // already stunned keeps `-1`, so `hitover` reads true for the whole
        // block exactly as it does in the reference. The clock that ends an air
        // block is its shake, not its hittime.
        bool keepStunClock = false;
        // How long the blocker is held before control comes back, which
        // `GetHitVar(ctrltime)` reports and a guard-held character reads to end
        // its own block state (`CtrlSet value = 1; trigger1 = Time =
        // GetHitVar(ctrltime)` -- 126 uses across 42 packs in local_chars, every
        // one of them that idiom). It is `guard.ctrltime` (or `airguard.ctrltime`
        // for an air block); a hit that was not blocked never guards, so it stays
        // 0 and a character that reads it unguarded reads nothing.
        int ctrlTime = 0;
        int slideTime = 0;
        // The downward acceleration the hit carries, for `GetHitVar(yaccel)`.
        // It is the HitDef's `yaccel` -- "the y acceleration to impart to P2 if
        // the hit connects", defaulting to .35 at this engine's 240p coordinate
        // space -- and the victim's own file is what applies it: the reference's
        // 5030, 5035 and 5050 each hold a
        // `type = VelAdd; trigger1 = 1; y = GetHitVar(yaccel)`, and those states
        // declare `physics = N`, so this number is the *only* gravity a launched
        // or falling victim has. Reporting 0 for it, as this did while the key was
        // unimplemented, left the victim drifting at whatever velocity it was
        // launched with. Written down-positive like every other y a character
        // writes; `VelAdd` flips it into the engine's frame on the way in.
        float yaccel = 0.35f;
        int stateNumber = CommonState::kHitHigh;
        int animType = 0;      // 0 light, 1 medium, 2 hard, 3 back, 4 up
        // `ground.type` and `air.type` in the reference's numbering, which is what
        // `GetHitVar(groundtype)`, `GetHitVar(airtype)` and `GetHitVar(type)`
        // report to the victim's own states. The animtype above is the one that
        // actually applies to *this* hit, which is not always the HitDef's: see
        // `hitInfoFor`.
        int groundType = 1;    // 0 none, 1 high, 2 low, 3 trip
        int airType = 1;
        bool air = false;
        // MUGEN's `GetHitVar(fall)`, which is the hit's own verdict on whether
        // the reaction is a *fall* rather than merely an airborne one. The two are
        // different questions and the reference answers them separately: `air` is
        // "the victim was off the ground when this landed or left it", and this is
        // "what is happening to them is a knockdown", so a plain air hit sets the
        // first and not the second, a launch sets both, and a hit on a victim
        // already on the floor sets it only if it lifts them back up.
        //
        // It is what the reference's `HitFall` trigger reports, and it is the
        // split its own get-hit states make: `[State 5000, 6]` sends a victim whose
        // `GetHitVar(fall)` is false to 5001, the flat knock-back, and lets 5000's
        // fallback controller send the rest to 5030; 5030 in turn chooses between
        // the fall (5050) and the air recovery (5040) on the same flag. Deriving it
        // from the state number, as this did, made it a restatement of a decision
        // rather than an input to one, and left it false for every launch the
        // moment the number stopped being 5030.
        bool falls = false;
        int attackerFacing = 1;
        // Ticks each side is frozen for. MUGEN's hitstop: the fighter that was
        // hit and the fighter that hit both stop dead for a moment, and the
        // impact reads as an impact instead of as a gap in the animation. Even a
        // stated zero pauses for a tick, which is why these are never 0.
        int attackerPause = 0;
        int victimPause = 0;
        // The hit was blocked. A guard takes no real damage (only the second
        // component of the HitDef's `damage` -- its guard damage, usually a tenth
        // of the hit), pushes the blocker
        // back rather than the attacker, and puts the blocker in a guard state.
        bool guarded = false;
        int guardSoundGroup = 6;
        int guardSoundSample = 0;
        // What this hit costs against an airborne victim's air-combo allowance.
        int juggleCost = 0;
        // The hit spark: which animation, where, and out of which file. A plain `sparkno`
        // names an animation in the screenpack's fightfx; an `S`-prefixed one names the
        // attacker's own. It is drawn at the point of contact, which is the victim's body.
        int sparkAnim = -1;
        float sparkX = 0.0f;
        float sparkY = 0.0f;
        bool sparkFromFightFx = true;
        // Carried through for the triggers that ask about the hit afterwards:
        // `hitdefattr` reads the attr text, `canrecover` the fall recoverability.
        std::string attrText;
        bool fallRecover = true;
        int fallRecoverTime = 60;
        // The window above, as a clock: armed to `fallRecoverTime` on the tick the
        // hit lands and counted down one a tick, which is the moment MUGEN
        // measures it from -- "the minimum time before the player can recover from
        // a fall". The fall itself begins later than the hit: a launched victim
        // spends its hitstun in the shake before the engine's table hands it to
        // 5050, so a window measured from the *fall state* would be that much
        // later than the reference's and would put recovery out of reach of a
        // default hit altogether.
        int fallRecoverTicks = 60;
        // The attr as masks, so HitOverride slots can match what is coming in.
        uint8_t attrClassMask = 1;
        uint8_t attrTypeMask = 1 | 8;
        // What the landing of this fall should do: fall.damage cost and the
        // bounce velocities, applied when the victim arrives at the floor.
        int fallDamage = 0;
        float fallVelX = 0.0f;
        float fallVelY = 4.5f;
        // And the screen shake it makes, from `fall.envshake.*`.
        int fallShakeTime = 0;
        float fallShakeFreq = 60.0f;
        float fallShakeAmpl = 0.0f;
        // The impact's own `envshake.*`, applied where the contact lands.
        int shakeTime = 0;
        float shakeFreq = 60.0f;
        float shakeAmpl = 0.0f;
        // As written on the HitDef: -100 means "not written" (the engine's
        // historical always-turn), else MUGEN's 0 keep / 1 toward / -1 away.
        int p2facing = -100;
        int getPower = -1;
        int givePower = -1;
        // The snap: pose the victim here. attackerX rides along because a HitInfo
        // built by the static path (a projectile) has no attacker to point at.
        bool hasSnap = false;
        float snapX = 0.0f;
        float snapY = 0.0f;
        float attackerX = 0.0f;
        // The `id` this hit carries, so the victim can record *what* last hit it.
        // It travels in the HitInfo rather than being read off the attacker because
        // a projectile deals a hit with no attacker machine behind it.
        int id = -1;
        // What a hit is *allowed* to do to the victim's life, and the class it
        // fights a clash with. These travel in the HitInfo rather than being read
        // back off the attacker's HitDef for the same reason `id` does: a
        // projectile lands a hit with no attacker machine behind it, and the
        // landing of a fall is resolved ticks after the HitDef that caused it was
        // overwritten by whatever move came next.
        bool kill = true;
        bool guardKill = true;
        bool fallKill = true;
        PriorityClass priorityClass = PriorityClass::Hit;
        // Whose file the state above is in, when `p2stateno` is asking for a state
        // that belongs to the *attacker*. Null means the victim's own file, which
        // is what a `p2stateno` hit that writes `p2getp1state = 0` wants and what
        // every hit that writes no `p2stateno` gets. It travels in the HitInfo for
        // the reason `attackerX` does -- the static path a projectile lands on has
        // no attacker machine to ask, and the projectile carries its owner's
        // character.
        const Character* stateSourceChar = nullptr;
    };
    // Returns whether the hit was taken. A hit can be refused -- by the victim
    // being dead already, or by an air-combo allowance that cannot pay for it --
    // and a refusal is not a hit: nothing freezes, nothing sounds, and the attack
    // is still looking for a contact.
    bool applyHit(const HitInfo& hit);

    // The HitDef the current state has armed, if any. Set by the HitDef
    // controller and cleared when the state changes.
    //
    // The velocities in it are world-space: MUGEN writes them in the attacker's
    // frame (`ground.velocity = -4` is "backwards"), and the machine resolves
    // facing once, here, so nothing downstream has to.
    struct HitDef {
        bool armed = false;
        int damage = 30;
        int hitTime = 12;      // ground hitstun, ticks
        int airHitTime = 20;   // air hitstun, ticks
        int slideTime = 0;
        // `yaccel`: the downward acceleration the hit gives its victim, which
        // `GetHitVar(yaccel)` hands to the character's own falling states. Only 67
        // of the corpus' 2558 HitDefs write one, so the default carries the rest:
        // MUGEN's is .35 at 240p, and the ones that are written are .36 and .44
        // most often, both within a hair of it.
        float yaccel = 0.35f;
        float pushX = -4.0f;   // world velocity given to the victim
        float pushY = 0.0f;
        float airPushX = -3.0f;
        // Positive is *up* here, unlike in the character's file: the parser
        // converts on the way in, so everything downstream -- the `launches`
        // test, the fall velocity, the velocity this engine hands the victim --
        // reads one frame. See `doHitDef`.
        float airPushY = 0.0f;
        int animType = 0;      // 0 light, 1 medium, 2 hard, 3 back, 4 up, 5 diag-up
        // `air.animtype` and `fall.animtype`: the reaction for an airborne victim,
        // and for one who is already falling. -1 marks "not written", because
        // neither defaults to a constant -- `air.animtype` defaults to `animtype`
        // itself, and `fall.animtype` to Up if `air.animtype` is Up and Back
        // otherwise -- so the default is resolved where the hit lands, once the
        // victim's own situation is known.
        int airAnimType = -1;
        int fallAnimType = -1;
        // `ground.type` and `air.type`, in the reference's numbering: 0 none,
        // 1 high, 2 low, 3 trip. `ground.type` defaults to High and `air.type` to
        // whatever `ground.type` says, so -1 is "not written".
        int groundType = 1;
        int airType = -1;
        // `priority = hit_prior, hit_type`. The number is 1-7 and defaults to 4;
        // the class is Dodge, Hit or Miss and defaults to Hit. Only the *class*
        // settles a tie, and it is what stops a throw and a punch from both
        // landing on the same tick -- see the table in the clash pass.
        int priority = 4;
        PriorityClass priorityClass = PriorityClass::Hit;
        bool launches = false;
        // `pausetime = attacker, victim`, the freeze frames of a hit.
        int pause[2] = {0, 0};
        // `guard.pausetime`, `guard.hittime`, `guard.velocity`, `guard.slidetime`
        // and `guard.ctrltime`: what a *blocked* hit does instead.
        int guardPause[2] = {0, 0};
        // The same fallback the parse would arrive at: `guard.hittime` inherits
        // `ground.hittime`, whose own default is 12, and `guard.slidetime` inherits
        // `ground.slidetime`, whose own default is 0. A HitDef that goes through
        // `parseHitDef` always has both written; these are for the one that does not.
        int guardHitTime = 12;
        float guardPushX = -4.0f;
        int guardSlideTime = 0;
        // The chip damage of `damage = hit_damage, guard_damage` -- its second
        // component, which is where the reference puts it -- and `guard.kill`,
        // whether a block may be lethal. Most characters allow it -- the guard
        // breaks and the blocker takes a knockout rather than a graze -- so the
        // decision is the character's, not the engine's.
        int guardDamage = 0;
        bool guardKill = true;
        // `kill` and `fall.kill`, the other two halves of the same question.
        // `kill = 0` means "this hit cannot KO", so the victim is left on one
        // point of life however hard it lands; `fall.kill = 0` is the same refusal
        // for the fall damage a landing pays. All three default to 1.
        bool kill = true;
        bool fallKill = true;
        // `guard.flag = "HLA"`, the state types this attack can be blocked by:
        // high (standing), low (crouching), air.
        bool guardHigh = true;
        bool guardLow = true;
        bool guardAir = true;
        // `air.juggle`: how much of the victim's air-combo allowance this hit
        // spends. MUGEN's default is 15, which is one launcher's worth.
        int airJuggle = 15;
        // `juggle`: how much allowance the *victim* needs left for this hit to
        // land at all.
        int juggleNeeded = 0;
        // `hitcount`: how many times one HitDef may connect. -1 is the default and
        // means "once per move".
        int hitCount = 1;
        // The hit spark: which animation, where, and out of which file. A plain `sparkno`
        // names one in the screenpack's fightfx and an `S`-prefixed one names the
        // attacker's own, which is the opposite way round from how the prefix reads.
        int sparkAnim = -1;
        float sparkX = 0.0f;
        float sparkY = 0.0f;
        bool sparkFromFightFx = true;
        // `hitsound = 5, 0`: the pair the *engine* plays when this hit lands.
        // It is not a PlaySnd controller -- nothing in a character's files plays
        // it -- which is why it is the one sound a character cannot make for
        // itself, and the one a fight is quietest without.
        int hitSoundGroup = 5;
        int hitSoundSample = 0;
        // `guardsound = 6, 0`: the same, for a hit that was blocked.
        int guardSoundGroup = 6;
        int guardSoundSample = 0;
        // `guard.sparkno`: the spark a *blocked* hit makes, which is a different one --
        // usually a flat blue flash rather than a starburst.
        int guardSparkAnim = -1;
        bool guardSparkFromFightFx = true;
        // `guard.ctrltime`: the time before the blocker regains control, which is
        // the thing the engine's guard stun actually measures. It defaults to
        // `guard.slidetime`, which itself defaults to `guard.hittime`, so a
        // character that writes none of the three gets the same number three
        // times and only one that writes this one sees a difference.
        int guardCtrlTime = 0;
        // `guard.dist`: the x-distance at which the *defender* may hold back and
        // be in a guard state at all -- `trigger.html`'s definition of the
        // trigger that reads it is "the value of the guard.dist parameter of the
        // opponent's HitDef", and `sctrls.html`'s is "the x-distance from P1 in
        // which P2 will go into a guard state if P2 is holding the direction away
        // from P1". It is a distance, not a reach: a move whose box spans ninety
        // units and one whose box spans ten can both say 100, and it is the
        // number, not the box, that decides whether a held back was a block.
        //
        // -1 is "the file did not write one", which is not the same as the 0
        // that most files do write: 90 of the corpus' 102 write-sites say
        // `guard.dist = 0`, and kfm's template comments that one in as many words
        // -- "This prevents p2 from going into a guard state if close". An
        // omitted one takes a player variable instead, which is why the field is
        // left unresolved here: the variable belongs to the HitDef's *owner*, and
        // a projectile's HitDef outlives the state that fired it. See
        // `guardDistanceFor`.
        int guardDist = -1;
        // `airguard.velocity` and `airguard.ctrltime`: what an air *block* does
        // instead. An air block is not a ground block held up in the air -- the
        // blocker is pushed by the *air* velocity scaled, x by 1.5 and y by a
        // half (the reference's own default), and held for the air guard's own
        // clock rather than the ground guard's.
        float airGuardVelX = 0.0f;
        float airGuardVelY = 0.0f;
        int airGuardCtrlTime = 0;
        // `down.*`: the reaction for a victim who is already lying on the floor.
        // Only a move whose `hitflag` carries D may touch such a victim at all --
        // `hitMayReach` refuses the rest -- so this is off the common path. It is
        // the whole of how a downed opponent is juggled back up, though, and the
        // y of the velocity is the switch: zero slides the victim back along the
        // floor for `down.hittime`, non-zero puts them back in the air, where the
        // air clock applies and `down.hittime` is ignored altogether.
        //
        // `down.bounce` is the one that needs the velocity *and* itself: it is
        // what decides whether the victim comes back off the floor after being
        // knocked into the air from it, and it defaults to off, so a character
        // that does not ask for the pop-up does not get one.
        float downVelX = 0.0f;
        float downVelY = 0.0f;
        int downHitTime = 20;
        bool downBounce = false;

        // `attr = S, NA`: the attack's class (Stand/Crouch/Air) and type
        // (Normal/Special/Hyper + Attack/Throw/Projectile). Read for the
        // `hitdefattr` trigger and for the throw rule: a throw is not guardable.
        std::string attrText;      // normalised "sca, na" spelling
        bool attrThrow = false;    // second-letter T
        bool attrProjectile = false;  // second-letter P
        // The same attr as letter masks, for HitOverride matching.
        uint8_t attrClassMask = 1;      // S=1, C=2, A=4
        uint8_t attrTypeMask = 1 | 8;   // N=1, S=2, H=4; A=8, T=16, P=32
        // `affectteam = F/E/B`: which team's players this attack may hit.
        // MUGEN's parser writes Friend = 1, Enemy = 2, Both = 3 and defaults to
        // Enemy when the parameter is absent (`sctrls.html`; the binary parser
        // initialises the slot to 2 at `0x47DA40`). The player-attack-vs-
        // projectile pass reads this instead of assuming Enemy.
        int affectTeam = 2;
        // `hitflag`: who this attack may touch. The binary's own default is
        // `MAF` (it ORs 62 in when the parameter is absent), and its letters are
        // the victim's *condition*: mid, air, falling, downed. `+` counts downed
        // in, which a launcher follow-up writes to keep a juggle alive.
        bool hitStand = true;
        bool hitAir = true;
        bool hitFall = true;
        bool hitDown = false;
        // The fall family: `fall = 1` and `air.fall = 1` put the victim down
        // whether or not the velocity lifts it, and the recover pair says whether
        // (and when) it may tech out of the fall.
        bool fall = false;
        bool airFall = false;
        // The two parameters that take the fall back off again, or move a crouching
        // victim's reaction up a height. `forceStand` defaults to true whenever
        // `ground.velocity`'s y is non-zero, which is the reference's own implicit
        // default and not a choice made here.
        bool forceStand = false;
        bool forceNoFall = false;
        bool fallRecover = true;
        int fallRecoverTime = 60;
        // Custom-state flow: `p2stateno` puts the victim into a state the move
        // chooses instead of the reaction the engine picks, which is the whole of
        // how a move like a command-grab poses its victim. `p1stateno` moves the
        // *attacker* on contact, the other half of the same idiom.
        int p2StateNo = -1;
        int p1StateNo = -1;
        // ...and *whose* file that state is in. `p2getp1state` defaults to 1 the
        // moment `p2stateno` is written -- the reference registers that default as a
        // pending value right before it parses the name, and its own docs say so
        // outright: "Defaults to 1 if the `p2stateno` parameter is used. Ignored
        // otherwise." So the ordinary throw pose is a state in the *attacker's*
        // file, and that is where local_chars puts it: of its 276 literal
        // `p2stateno` writes, 272 name a state defined in the writer's own folder,
        // and exactly one write asks for `p2getp1state = 0`.
        bool p2GetP1State = false;
        // The fall landing: what happens when a victim this move dropped hits the
        // floor. `fall.damage` is its extra cost, and the velocity pair is the
        // bounce (MUGEN's default y bounce is -4.5 in its own frame).
        int fallDamage = 0;
        float fallVelX = 0.0f;
        float fallVelY = 4.5f;
        // `fall.envshake.*`: the screen shake the fall landing makes. Zero time
        // means none, which is the default.
        int fallShakeTime = 0;
        float fallShakeFreq = 60.0f;
        float fallShakeAmpl = 0.0f;
        // The impact's own shake: `envshake.*` inside a HitDef. Zero time = none.
        int shakeTime = 0;
        float shakeFreq = 60.0f;
        float shakeAmpl = 0.0f;
        // `mindist`/`maxdist`: how far the victim's axis may be for the hit to
        // count at all. Written as x,y pairs; only the x gate applies (a move's
        // reach is a horizontal thing).
        bool hasMinDist = false;
        bool hasMaxDist = false;
        float minDistX = 0.0f;
        float maxDistX = 1.0e9f;
        // `p2facing`: 0 keeps the victim's facing, 1 turns it toward the attacker
        // (MUGEN's standing-hit look), -1 away from it.
        int p2facing = -100;  // -100 = not written: the engine's old always-turn
        // `p1facing`/`p1getp2facing`: where the *attacker* ends up looking once the
        // hit has landed. Both are 0 by default, and for `p1facing` a 1 means the
        // same as an unwritten one: the reference keeps only the sign of it (its
        // active block stores `p1facing < 0` as a single bit), so a positive value
        // and no value are the same no-op. The negative case is the reverse throw --
        // the attacker turns around and plays its throw the other way, which is what
        // Mista and Giorno both ask for with `ifelse(command = "holdfwd", 1, -1)`.
        // `p1getP2Facing` then overrides the pair when it is non-zero: 1 to face the
        // way the victim ended up facing, -1 to face the other way.
        int p1facing = 0;
        int p1GetP2Facing = 0;
        // `snap = x, y`: the victim posed at the attacker's position plus the
        // offset, in the attacker's facing frame. The grab-plunge idiom.
        bool hasSnap = false;
        float snapX = 0.0f;
        float snapY = 0.0f;
        // Power economics: -1 keeps the engine's built-in numbers.
        int getPower = -1;   // attacker, on contact
        int givePower = -1;  // victim
        // The chain family. `id` names this hit, so a later link can ask what the
        // victim was last hit by; `chainID` requires that answer to be this number,
        // and `nochainID` (a pair) forbids up to two. That pair is the whole of how
        // a combo is written to continue out of the move before it and not from
        // neutral -- mashing the second link alone does nothing, because nothing
        // has established the id it is waiting for.
        //
        // -1 on any of them is "no restriction", which is also the default: a
        // character that never writes any of this chains exactly as it always did.
        int id = -1;
        int chainId = -1;
        int noChainId = -1;
        int noChainId2 = -1;
    };
    [[nodiscard]] const HitDef& armedHitDef() const { return hitDef_; }
    [[nodiscard]] bool hasArmedHitDef() const { return hitDef_.armed; }
    // The player's *live* guard distance: the value `inguarddist` asks about, and the
    // one the reference keeps on the player rather than on the HitDef.
    //
    // Read out of the binary, it is a slot on the player struct (`+0x132C`) with exactly
    // two writers, and neither of them is "the HitDef was just parsed":
    //
    //   * a state change seeds it, from the player's own copy of `[Size] attack.dist`,
    //     and only when the state it is entering does not carry `hitdefpersist` -- which
    //     is the documented default ("Defaults to the value in the player variables if
    //     omitted") arriving at state entry rather than at the HitDef;
    //   * a HitDef with a `guard.dist` in it overwrites it, and one without leaves it
    //     exactly as it was -- the reference's own arithmetic is a single conditional
    //     store of `parsed == -1 ? old : parsed`.
    //
    // So it is *not* a property of the armed HitDef, which is why this is a field of its
    // own rather than a read of `hitDef_.guardDist`: a state in `movetype = A` with no
    // HitDef armed at all -- a super's wind-up, a whiffed attack's recovery -- still has
    // a guard distance, and still makes an opponent holding back count as guarding. That
    // is the whole point of the number: it is not a hitbox and never claimed to be.
    [[nodiscard]] int guardDist() const { return guardDist_; }
    // Whether the armed HitDef may touch this victim at all: the `hitflag`
    // letters name the victim's *condition* -- mid, air, falling, downed -- and
    // the default `MAF` deliberately excludes a fighter already on the floor.
    // Throws additionally cannot be blocked, which is an `attr` property.
    [[nodiscard]] bool hitMayReach(const StateMachine& victim) const;
    // The same test for a HitDef that has no live attacker behind it: a
    // projectile's, whose owner has moved on to another state by the time it lands.
    [[nodiscard]] static bool hitMayReach(const HitDef& hit, const StateMachine& victim);
    // The `priority` the armed HitDef declares. Two attacks landing in the same
    // tick are settled on it: the higher one wins, and equal ones trade.
    [[nodiscard]] int attackPriority() const { return hitDef_.priority; }
    // Its class beside it. Read from the armed HitDef for the same reason the
    // number is: a clash is decided between two attacks that are both still
    // looking for a contact, so both are armed by definition.
    [[nodiscard]] PriorityClass attackPriorityClass() const { return hitDef_.priorityClass; }

    // The reaction this machine's armed HitDef would produce on a particular
    // victim. Built at the moment of contact rather than when the HitDef ran,
    // because which hit state fits depends on where the victim is: the same
    // move hits a standing fighter high and an airborne one into a fall.
    [[nodiscard]] HitInfo hitInfoFor(const StateMachine& victim) const;
    // The same, from a HitDef that is not this machine's -- which is what a projectile
    // needs, since the fighter that fired it has moved on to another state by the time
    // the beam arrives. `attackerCharacter` is whose file a `p2stateno` names, and is
    // null for a hit that has no character behind it at all (see HitInfo).
    [[nodiscard]] static HitInfo hitInfoFor(const HitDef& hit, int attackerFacing,
                                           const StateMachine& victim,
                                           const Character* attackerCharacter = nullptr);

    // The x-distance from behind which this HitDef's attack can be blocked:
    // `guard.dist` itself when the file wrote one, and otherwise the player
    // variable that covers the case -- `size.attack.dist` (160 by default) for a
    // direct attack and `size.proj.attack.dist` (90) for a projectile's. Those
    // two names exist for exactly this pair, and the corpus says so twice over:
    // seventeen packs declare both in their `[Size]`, and the ones carrying
    // Japanese comments gloss them as "the minimum distance at which the
    // opponent enters guard posture" and, for the projectile one, "…against your
    // own Projectile".
    //
    // `owner` is the character the variable is read from, which for a projectile
    // is its firer rather than whoever is on screen now; null falls back to
    // Elecbyte's own numbers. The value comes back in the character's own space,
    // as everything a file writes does -- the caller scales it.
    //
    // Only the projectile case is left, and that is the reference's own shape rather
    // than a simplification: the direct case never asks a HitDef anything, because the
    // player carries the resolved number (`guardDist()`), and the only thing that ever
    // needs to resolve an omitted one is a projectile, whose HitDef was filled in at
    // spawn time by an owner who has since moved on to another state.
    [[nodiscard]] static float guardDistanceFor(const HitDef& hit, const Character* owner,
                                                bool projectile);

    // Clears the armed hit, records the victim as this move's current target (the
    // `target` redirect and the bindtotarget family read it), and marks the attack
    // as having connected, so a multi-frame hitbox does not hit twice. A null
    // victim leaves the previous target alone, which is what the spark-only
    // callers mean.
    void markHitLanded(StateMachine* victim = nullptr);
    // Who this move is currently hitting, if anyone: the `target` redirect.
    [[nodiscard]] StateMachine* lastTarget() { return lastTarget_; }
    [[nodiscard]] const StateMachine* lastTarget() const { return lastTarget_; }

    // The hit that last landed on this fighter, for the fight screen's gauges as much
    // as for `gethitvar`: whether it was blocked is what decides whether the damage is
    // recoverable, and how much is what the damage counter counts.
    [[nodiscard]] const HitInfo& lastHit() const { return lastHit_; }
    // Hands life back. MUGEN's `LifeAdd` controller does this; here it is also what the
    // recovered part of a blocked hit drains into.
    void gainLife(int amount) { life_ = std::clamp(life_ + amount, 0, maxLife_); }

    [[nodiscard]] int moveContact() const { return moveContact_; }
    [[nodiscard]] int moveGuarded() const { return moveGuarded_; }
    [[nodiscard]] int moveReversed() const { return moveReversed_; }

    // ---------------------------------------------------------- hit pause
    //
    // The freeze frames of a hit. While a fighter is paused it does nothing at
    // all -- no controllers, no animation, no physics -- which is what makes an
    // impact read as an impact. Both the attacker and the victim get a pause,
    // and they are usually different lengths: `pausetime = 8, 12` freezes the
    // attacker for 8 and the victim for 12.
    [[nodiscard]] bool inHitPause() const { return pauseTicks_ > 0; }
    // Shoves the fighter sideways, clamped to the stage. Used for the corner push,
    // where the shove lands on the *attacker* rather than on the victim.
    void nudge(float dx) {
        x_ = std::clamp(x_ + dx, stageMin_, stageMax_);
    }
    [[nodiscard]] int pauseTicks() const { return pauseTicks_; }
    // Ticks left of the victim's shake, which is what `hitshakeover` reports. It
    // runs alongside the pause and is why `hitshakeover` and `hitover` are two
    // different triggers rather than one.
    [[nodiscard]] int shakeTicks() const { return shakeTicks_; }
    // The jitter the shake puts on the *drawn* sprite, in virtual units. It never
    // touches the fighter's real position, because MUGEN's shake is a rendering
    // effect: a shaken fighter still stands where it stands.
    [[nodiscard]] float shakeOffsetX() const { return shakeX_; }

    // ---------------------------------------------------------- guarding
    //
    // Whether this fighter may block at all right now: it needs control, to be
    // idle, and to be on its feet or crouching or in the air. That is MUGEN's
    // `guard` flag, and holding *back* is what turns it into an actual block.
    [[nodiscard]] bool canGuard() const;
    // Whether the character wrote a `[Statedef]` header for the state it is in,
    // which is what decides whether the file or the engine ends a hit reaction.
    [[nodiscard]] bool ownsCurrentState() const;
    // Whether the back direction is held. Public because blocking is *decided by
    // the victim*, and the attacker is the one holding the HitDef that has to ask.
    [[nodiscard]] bool holdingBack() const;
    // MUGEN's `inguarddist`: the opponent is swinging from close enough that a
    // block is worth holding. Set by the fight loop, which is where the two
    // fighters' collision boxes are in the same space.
    [[nodiscard]] bool inGuardDist() const { return inGuardDist_; }
    void setInGuardDist(bool value) { inGuardDist_ = value; }
    // Spends guard energy on a blocked hit. Called by applyHit().
    void spendGuard(int amount);
    // Whether a state number is one of the guard states, in which a fighter keeps
    // its block even though guarding costs it control.
    [[nodiscard]] static bool isGuardState(int state);

    // Guard energy, 0..kGuardEnergyMax.
    //
    // MUGEN has no such thing: in MUGEN a block that is not explicitly unblockable
    // works forever. Shin Budokai does have one -- its HUD carries a bar for it, and
    // its own challenges talk about pressing an opponent until they "lose their guard
    // energy" -- and a bar drawn on screen has to mean something, so the engine keeps
    // the quantity the bar shows. A blocked hit spends some; it comes back if the
    // blocker is left alone. At zero, a block stops working until it recovers, which is
    // what makes relentless pressure worth more than patience.
    static constexpr int kGuardEnergyMax = 100;
    [[nodiscard]] int guardEnergy() const { return guardEnergy_; }
    // Whether the block is currently working at all. Separate from `canGuard`, which is
    // about position and control; this is about the meter.
    [[nodiscard]] bool guardHolds() const { return guardEnergy_ > 0; }

    // The freeze frames a hit hands out. Longer wins, so a fighter caught by two
    // attacks in one tick freezes for the longer of the two rather than for the
    // last one the loop happened to resolve.
    void setPause(int ticks) {
        if (ticks > pauseTicks_) pauseTicks_ = ticks;
    }
    // A hit that was blocked rather than landed: the attacker's own bookkeeping,
    // which is what `moveguarded` reports.
    void markGuarded() {
        moveGuarded_ = 1;
        moveContact_ = 1;
    }

    // ------------------------------------------------------- air juggling
    //
    // How much of this fighter's air-combo allowance is left. A launcher spends
    // its HitDef's `air.juggle` against the victim's pool, and a hit the pool
    // cannot pay for simply does not land -- which is the whole of what stops an
    // air combo from being infinite.
    [[nodiscard]] int juggleLeft() const { return jugglePoints_; }
    [[nodiscard]] bool canJuggle(int cost) const { return jugglePoints_ >= cost; }
    [[nodiscard]] int moveHit() const { return moveHit_; }
    [[nodiscard]] bool tookDamage() const { return tookDamage_; }
    [[nodiscard]] bool alive() const { return life_ > 0; }

    // Where `PlaySnd` sends its sounds. Null by default, which makes a sound a
    // no-op -- the right answer for a machine with no audio, and what lets the
    // host tests run a real character without a device.
    void setSoundSink(SoundSink* sink) { sound_ = sink; }
    // Where the explods, projectiles and screen shakes this machine asks for go. Null
    // means they are dropped, which is what a host test that only cares about the state
    // machine wants, and what a fight with no entity layer would do.
    void setEntitySink(EntitySink* sink) { entities_ = sink; }
    [[nodiscard]] EntitySink* entitySink() const { return entities_; }
    // Which side of the fight this machine is, which is all an explod needs to know to
    // follow the right fighter in a mirror match.
    void setSide(int side) { side_ = side; }
    [[nodiscard]] int side() const { return side_; }

    // `HitOverride` slots: for `time` ticks, an incoming hit whose attr lies
    // entirely inside the slot's masks is *redirected* -- no damage, no usual
    // reaction; the victim enters the slot's state instead. This is the counter
    // mechanic: a parry stance is a state of stacked HitOverrides that turn the
    // incoming attack into the counter's first frame.
    struct HitOverrideSlot {
        bool active = false;
        int ticksLeft = 0;
        int stateNo = -1;
        bool forceAir = false;
        uint8_t classMask = 0;
        uint8_t typeMask = 0;
    };
    static constexpr size_t kHitOverrideSlots = 8;

    // `ReversalDef`: the attacker's mirror of HitOverride. While one is armed,
    // an incoming hit whose attr fits the reversal's mask is *captured*: no
    // reaction, no damage; the attacker is halted in place and this move learns
    // through `movereversed` that it was countered -- the counter super's state
    // machine branch fires next tick.
    struct ReversalDef {
        bool armed = false;
        uint8_t classMask = 0;
        uint8_t typeMask = 0;
        int pause[2] = {0, 0};  // attacker, victim, like HitDef's pausetime
    };

    // ------------------------------------------------- AssertSpecial
    //
    // Flags a character raises for one tick at a time, to say something about itself that
    // no state parameter can: that it is invisible, that it cannot be hit high, that its
    // shadow should not be drawn. They are cleared every tick and must be re-asserted,
    // which is how MUGEN does it and why they are written `trigger1 = 1` inside a state
    // that lasts as long as the property should.
    enum AssertFlag : uint32_t {
        kAssertNoShadow = 1u << 0,
        kAssertInvisible = 1u << 1,
        kAssertNoAirGuard = 1u << 2,
        kAssertNoStandGuard = 1u << 3,
        kAssertNoCrouchGuard = 1u << 4,
        kAssertNoHardcodedKeys = 1u << 5,
        kAssertNoJuggleCheck = 1u << 6,
        // `globalnoshadow` is engine-global in the reference: it skips every
        // shadow pass, not just this machine's. Here the flag still lives on
        // the machine that asserted it and the fight screen ORs the running
        // machines for a tick -- see FightScreen::anyGlobalNoShadow().
        kAssertGlobalNoShadow = 1u << 7,
    };
    [[nodiscard]] bool asserted(uint32_t flag) const { return (assert_ & flag) != 0; }
    // MUGEN's angle draw: the spin this fighter is drawn with, in degrees.
    // AngleSet writes it; AngleAdd/AngleMul adjust it; the .air frame's own angle
    // adds on top at draw time; the Offset-like visual-only rule applies (it
    // never touches collision).
    [[nodiscard]] float drawAngle() const { return drawAngleDeg_; }
    void setDrawAngle(float degrees) { drawAngleDeg_ = degrees; }
    [[nodiscard]] bool invisible() const {
        return asserted(kAssertInvisible) || invisibleTicks_ > 0;
    }
    [[nodiscard]] uint32_t assertFlags() const { return assert_; }
    void setAssertFlag(uint32_t flag) { assert_ |= flag; }
    void setInvisibleTicks(int ticks) { invisibleTicks_ = std::max(invisibleTicks_, ticks); }

    // ----------------------------------------------- NotHitBy, in part
    //
    // `NotHitBy` makes a fighter un-hittable for a time -- an intro, or the startup of a
    // move that is meant to go through whatever is coming. MUGEN filters by hit
    // *attribute* as well; this ignores every hit, which is what nearly every use of it
    // in the wild means and is documented as the approximation it is.
    [[nodiscard]] bool ignoringHits() const { return notHitTicks_ > 0; }
    // The other half of the same controller: `value = , NT,ST,HT` refuses
    // throws only. The common get-up writes it for twelve ticks beside three
    // ticks of all-classes invulnerability, so a fighter standing up cannot be
    // grabbed for a moment after it can be hit again.
    [[nodiscard]] bool ignoringThrows() const { return notHitThrowTicks_ > 0; }
    void setNotHitTicks(int ticks) { notHitTicks_ = std::max(notHitTicks_, ticks); }
    void setThrowNotHitTicks(int ticks) {
        notHitThrowTicks_ = std::max(notHitThrowTicks_, ticks);
    }
    // Whether the hit this fighter has armed right now is a throw, which is the
    // question the throw-only window has to ask of its attacker.
    [[nodiscard]] bool armedHitIsThrow() const { return hitDef_.armed && hitDef_.attrThrow; }

    // The HitDef an armed projectile would inherit: MUGEN gives a projectile the
    // properties of the HitDef that was active when it was created.
    [[nodiscard]] const HitDef& armedHitDefForProjectile() const { return hitDef_; }
    [[nodiscard]] SoundSink* soundSink() const { return sound_; }

    // Sets the ground line the machine's y is measured from. Kept here so that
    // `pos y` triggers read the same thing the physics acts on.
    [[nodiscard]] float yAccel() const { return yAccel_; }

    // Start position and the limits the fighter may walk to. The bounds are in
    // the same units as x(), and are what `backedgebodydist`/`frontedgebodydist`
    // are measured against.
    void setPosition(float x) { x_ = x; }
    void setVelocity(float velX, float velY) { velX_ = velX; velY_ = velY; }
    void setStageBounds(float min, float max) {
        stageMin_ = min;
        stageMax_ = max;
    }
    [[nodiscard]] float stageCenter() const { return (stageMin_ + stageMax_) * 0.5f; }
    [[nodiscard]] float stageMin() const { return stageMin_; }
    [[nodiscard]] float stageMax() const { return stageMax_; }

    // The visual displacement the Offset controller asked for, in the fighter's
    // own units. Drawing adds it; nothing else sees it.
    [[nodiscard]] float offsetX() const { return offsetX_; }
    [[nodiscard]] float offsetY() const { return offsetY_; }

    // Whether the machine has a character behind it. A machine without one is
    // the engine test's dummy: it stands, it can be hit, and it runs nothing.
    [[nodiscard]] bool hasCharacter() const { return character_ != nullptr; }

    // Sticky movement policies: `ScreenBound` releases the stage-edge clamp (a
    // fly-off-screen super is built on it), and `PlayerPush` switches the
    // body-push between fighters off (a ghost dash needs to pass through). Both
    // stay until changed, which is how MUGEN reads them.
    void setScreenBound(bool value) { screenBound_ = value; }
    void setPlayerPush(bool value) { playerPush_ = value; }
    [[nodiscard]] bool playerPush() const { return playerPush_; }
    // `PosFreeze = 1` holds position for the tick it's written on: re-asserted
    // every tick (trigger1 = 1), not sticky, like the hitpause it imitates.
    void setPosFreeze(bool value) { posFreezeThisTick_ = value; }
    // A `Trans` controller run this tick, if any. Absent means MUGEN's `default`
    // -- "does nothing" -- so the draw falls through to the frame's own `.air`
    // code rather than to any particular blend. That distinction is why this is
    // an optional and not a TransParams with a Normal default: `Trans = none`
    // (which forces Normal) and no `Trans` at all are different draws.
    [[nodiscard]] const std::optional<TransParams>& transThisTick() const {
        return transThisTick_;
    }
    // Whether that controller wrote `addalpha` specifically. The shadow path
    // consults only that Trans type, and only its destination alpha
    // (SHADOW-RE §5); `add` and `add1` also produce an additive TransParams, so
    // the blend alone cannot tell them apart.
    [[nodiscard]] bool transIsAddAlphaThisTick() const { return transAddAlphaThisTick_; }
    // The AfterImage trail knobs, as the controller wrote them. Ticks above zero
    // is the whole of "on": the trail lives exactly as long as its time asks.
    struct AfterImageParams {
        int ticksLeft = 0;     // 0 = off
        int length = 9;        // ghosts kept
        int color = 1;         // palcolor: 1 keeps colour, 0 goes grey
        bool invert = false;   // palinvertall
        float brightMul = 1.0f;
        // MUGEN's default trail blend is add. A full TransParams rather than a
        // bare Blend so a trail can carry its own `alpha` pair, which the binary
        // reads even though the controller's documentation does not list it.
        TransParams trans{Blend::Additive};
    };
    [[nodiscard]] const AfterImageParams& afterImage() const { return afterImage_; }

    // ---------------------------------------------------------- palette state
    //
    // PalFX's working state. The numbers stay in MUGEN's own units (mul on a
    // 0..256 scale where 256 is identity, add and sinadd on 0..255, color on
    // 0..256) so the draw path can quote the file's arithmetic without a second
    // conversion table. `time` counts down in step() at the effect's full strength
    // and the slot drops back to identity when it reaches zero -- MUGEN does not
    // interpolate, so `add = 255,255,255, time = 4` is a white flash that holds
    // for four ticks and then simply stops. -1 holds the grade until another
    // PalFX or a time=0 reset.
    struct PalFxState {
        bool active = false;
        int time = 0;    // ticks left; -1 = until reset
        std::array<float, 3> mul{256.0f, 256.0f, 256.0f};
        std::array<float, 3> add{0.0f, 0.0f, 0.0f};
        bool invert = false;
        float color = 256.0f;  // 0 = greyscale, 256 = full colour
        std::array<float, 3> sinAdd{0.0f, 0.0f, 0.0f};
        int sinPeriod = 60;
        bool hasSinAdd = false;
        int elapsed = 0;  // ticks since the effect armed, for the sine's phase
    };
    [[nodiscard]] const PalFxState& palFx() const { return palFx_; }

    // The same grade in the renderer's 0..1 space, with the sine term folded into
    // the add at this tick's phase. One conversion for all three slots: this machine
    // applies it to its own grade in the draw, and the fight applies it to the
    // BGPalFX/AllPalFX slots, which carry the identical arithmetic.
    [[nodiscard]] static PalFxParams palFxParams(const PalFxState& state);

    // The palette the fighter is drawn in, 1-based as MUGEN counts it: the
    // def's pal.defaults at first, then whatever RemapPal has re-pointed.
    // `palno` reads palNo_ (the selection, not the remap); the draw uses
    // effectivePalette().
    [[nodiscard]] int palNo() const { return palNo_; }
    // The selection as a palette identity: `palno` is the item of group 1,
    // which is MUGEN's own numbering for a player's palettes.
    [[nodiscard]] PaletteRef selectedPalette() const { return PaletteRef{1, palNo_}; }
    // The palette the draw should bind, after RemapPal. The redirection is
    // applied once: the docs are explicit that a destination is an *unmapped*
    // palette, so `(1,1)->(1,2)` and `(1,2)->(1,3)` draw a sprite using (1,1)
    // with (1,2), not (1,3).
    [[nodiscard]] PaletteRef effectivePalette() const {
        return applyPaletteRemap(palRemaps_, selectedPalette());
    }
    // The redirection for an arbitrary palette identity, used by the draw for a
    // sprite whose own palette node is the source (a sprite with a dedicated
    // palette). Unchanged when no entry names it.
    [[nodiscard]] PaletteRef remapPalette(PaletteRef source) const {
        return applyPaletteRemap(palRemaps_, source);
    }
    // The table of redirections, in the order their sources were first written.
    // Exposed for the host tests, which pin the parse; the draw only needs
    // effectivePalette()/remapPalette().
    [[nodiscard]] const std::vector<PaletteRemap>& paletteRemaps() const { return palRemaps_; }
    // The pure one-level lookup behind both accessors above. A source written
    // twice has one entry (the later destination), and the returned
    // destination is never looked up again.
    [[nodiscard]] static PaletteRef applyPaletteRemap(
        const std::vector<PaletteRemap>& remaps, PaletteRef source);
    // The select screen's choice, applied to the fighter it picked. MUGEN has no
    // controller for this -- it is a decision made before the round, not a move --
    // so it lives here rather than among the controllers.
    //
    // It has to be applied *after* `setup`, not once at the fight's start:
    // setup is called at the top of every round and resets palNo_ to the
    // character's `pal.defaults`, so a palette set before it would survive
    // exactly one round. A character's own RemapPal can still re-point the
    // colour at runtime afterwards, which is the MUGEN-correct order.
    void setPalNo(int palNo) {
        if (palNo >= 1) palNo_ = palNo;
    }
    // `HitFallSet`: whether this fighter's current airborne hit state counts as
    // fallable, which is what the `hitfall` trigger answers. MUGEN's default is
    // "air hit states fall", and a state that wants a soft air hit writes 0.
    void setHitFall(bool value) { hitFallFlag_ = value; }
    // The fall-landing bookkeeping. `setFallLanding` records what the last hit
    // left owing; *who spends it* is decided by the character's file, so see
    // `landingScriptedBy` before assuming `land()` is the one that applies it.
    //
    // `kill` is the HitDef's `fall.kill`, carried here for the same reason the
    // damage is: the fall lands ticks after the HitDef that caused it has been
    // overwritten by whatever the attacker did next, so the answer cannot be read
    // back off the attacker when it is finally needed.
    void setFallLanding(int damage, bool kill, float velX, float velY, int shakeTime,
                        float shakeFreq, float shakeAmpl) {
        fallDamagePending_ = damage;
        fallKillPending_ = kill;
        fallVelX_ = velX;
        fallVelY_ = velY;
        fallShakeTime_ = shakeTime;
        fallShakeFreq_ = shakeFreq;
        fallShakeAmpl_ = shakeAmpl;
    }
    [[nodiscard]] bool hitFall() const { return hitFallFlag_ && moveType_ == MoveType::Hit; }

    // Pushes both grounded fighters apart when they overlap.
    static void separate(StateMachine& a, StateMachine& b);

    // How hard the CPU plays. Only meaningful once `setAiControlled(true)` has
    // said that this fighter *is* the CPU -- see `ailevel` below.
    void setAiLevel(int level);
    [[nodiscard]] int aiLevel() const { return aiLevel_; }

    // Whether this fighter is the CPU. It decides what `ailevel` reports, and
    // that one number is how a character knows whether to run its own AI: the
    // idiom is `trigger1 = ailevel > 0` guarding a VarSet that switches the
    // whole character over to computer control. Reporting the difficulty for a
    // human player hands the character over to its AI instead -- no control, no
    // response to the pad -- so this has to be told apart from the difficulty
    // rather than folded into it.
    void setAiControlled(bool ai) { aiControlled_ = ai; }
    [[nodiscard]] bool aiControlled() const { return aiControlled_; }

    // Power, on MUGEN's 3000 scale: gained for landing a hit and for taking
    // one, which is the whole of how a meter fills.
    void gainPower(int amount) { power_ = std::clamp(power_ + amount, 0, 3000); }

    // Match-wide clock, for `gametime`.
    void setGameTime(long long ticks) { gameTime_ = ticks; }
    // Which round of the match is being fought, for `roundno`.
    void setRoundNo(int round) { roundNo_ = round; }
    // Which match of the run this is, for `matchno`. Versus is always match 1;
    // an arcade run counts opponents from 1 and only advances when a match is
    // won. The fight screen drives it when the match is set up.
    void setMatchNo(int match) { matchNo_ = match; }
    // Whether the match is decided and the winner has begun its win pose, for
    // `matchover`. The fight screen drives it once a tick.
    void setMatchOver(bool over) { matchOver_ = over; }
    // MUGEN's roundstate: 0 pre-intro, 1 intro running, 2 fighting, 3 hit/KO
    // playing out, 4 decided. The fight screen drives it once a tick.
    void setRoundState(int state) { roundState_ = state; }
    // The round result from this fighter's side: win/lose/draw, for its triggers.
    // -1 undecided; 0 lost; 1 won; 2 drawn.
    void setRoundOutcome(int outcome) { roundOutcome_ = outcome; }
    [[nodiscard]] int roundOutcome() const { return roundOutcome_; }
    void setFacing(int facing) { facing_ = facing; }

    // -------------------------------------------------------- TriggerContext
    Value trigger(std::string_view name, const std::vector<Value>& args) const override;
    const TriggerContext* redirect(std::string_view target, int selector) const override;
    bool assignVar(char kind, int index, float value) const override;

private:
    // --- state changes
    void changeState(int number, bool keepCtrl);
    void enterState(int number);
    // State -1, then the current state's controllers. A state change inside
    // them re-runs the *new* state's controllers this tick, which is what makes
    // `trigger1 = time = 0` fire on the tick a move starts.
    void runStateLogic();
    void runCurrentState();
    // MUGEN's three engine-run states, in the order and under the conditions it
    // runs them: -1 always, -2 while the fighter has no control, -3 while the
    // computer is playing it. They are separate states because a character keeps
    // different things in each -- -1 holds the command table, -2 the things that
    // have to happen during hitstun, and -3 an entire second move list for the
    // CPU, which is how a character packaged for arcade play has any moves at
    // all under computer control.
    void runStateMinus1();
    void runStateMinus2();
    void runStateMinus3();
    void runControllers(const std::vector<Controller>& controllers);
    // Hitpause-path runners: the same walks, limited to controllers that wrote
    // `ignorehitpause = 1`.
    void runControllersIf(const StateLibrary* states, int stateNo, bool duringHitPause);
    void runControllersFiltered(const std::vector<Controller>& controllers,
                                bool duringHitPause);

    // MUGEN's own behaviour for the movement states -- standing, crouching,
    // walking, jumping -- which the engine supplies rather than the character.
    // See the definition for why this is not optional.
    void runBuiltinState();
    // The direction-driven half of the built-in states: stand into crouch,
    // walk and jump, crouch back to standing, and the walk stopping. Returns
    // true when the current state is one of those. Separate from
    // runBuiltinState() because it also runs for states the character defined.
    bool runBuiltinMovement();
    // The posture the guard states 120 and 140 change *in place*, which is what
    // their `Hi to Lo` / `Lo to Hi` `StateTypeSet` pairs do: `crouch` is the down
    // direction held, and the swap only ever happens between standing and
    // crouching -- a fighter in the air is left in the air, its posture being
    // 120's and 140's other job (and one they do from the state type they came in
    // with). Returns whether anything changed.
    //
    // In place rather than by state number, and that is the file's choice rather
    // than the engine's: 120 and 140 are transition poses, and moving to another
    // *state* to change posture would restart their animations from frame 0.
    bool setGuardPosture(bool crouch);
    // Which of 130/131/132 the state type says a guard is being held in -- what
    // the reference writes as `130 + (statetype = C) + (statetype = A)*2`.
    [[nodiscard]] int guardedStateForPosture() const;
    // Whether a direction is held. Asked of the character's own commands first,
    // so a character that redefines `holdfwd` gets its own meaning, and of the
    // raw input second, because state 0 cannot work without it and a minimal
    // .cmd does not declare it.
    [[nodiscard]] bool directionHeld(std::string_view command, uint32_t symbols) const;
    // A [Velocity] key, in MUGEN's units, with the sign MUGEN writes it in.
    [[nodiscard]] float velocityConstant(std::string_view key, float fallback) const;
    // Whether the `recovery` command is being played. Every character is required
    // to declare it -- the .cmd files in the wild carry it with the reference's
    // own comment, `name = "recovery";Required (do not remove)` -- so a character
    // with a command file is asked, and one without is answered with the binding
    // the reference's own kfm.cmd uses.
    [[nodiscard]] bool recoveryHeld() const;
    // `CanRecover`: alive, mid-hit, in the air, past the hit's no-recovery
    // window, and with a hit that permits recovery at all. The `canrecover`
    // trigger reads this, and so does the fall's own way out, so that a
    // character testing the trigger and the engine acting on it cannot drift.
    [[nodiscard]] bool canRecover() const;
    // Recovery pressed during a fall: the one exit from 5050 that is not the
    // floor. Returns true when the state was changed.
    bool tryFallRecover();

    // --- controller handlers, grouped by what they touch
    void doChangeState(const Controller& controller);
    void doChangeAnim(const Controller& controller);
    // `ChangeAnim2`: pose the *target* with an animation out of this character's
    // file -- the throw idiom. Clears when the target's state moves on, so a
    // victim whose throw ended draws its own art again.
    void doChangeAnim2(const Controller& controller);
    void doVelocity(const Controller& controller);
    void doPosition(const Controller& controller);
    void doHitDef(const Controller& controller);
    // `HitOverride`: arm a slot that redirects a matching incoming hit into the
    // named state, for `time` ticks. The binary's reader shows: attr mask,
    // optional slot index, stateno, time default 1, forceair default 0.
    void doHitOverride(const Controller& controller);
    // `ReversalDef`: the counter side -- arm the capture mask for this move.
    void doReversalDef(const Controller& controller);
    // Attempts to capture an incoming hit with the armed ReversalDef. True when
    // captured -- the caller treats it as "the hit was taken" but nothing about
    // this machine changes except the movereversed flag.
    bool tryReversal(const HitInfo& hit);
    // The armed capture mask, for whatever just got caught by it.
    [[nodiscard]] const ReversalDef& reversalDef() const { return reversalDef_; }
    // `PlaySnd`: hands the (group, sample) pair to the sound sink. The channel,
    // pan, volume and frequency parameters are read by MUGEN and ignored here --
    // the mixer has one stream, and a sound whose volume is not the one its
    // author asked for is a much smaller loss than a sound that never plays.
    void doPlaySnd(const Controller& controller);
    // The spark a landed or blocked hit makes, spawned where the contact was.
    void spawnHitSpark(const HitInfo& hit);
    void doExplod(const Controller& controller);
    // `ModifyExplod`, the controller whose whole description in the reference is that
    // its "syntax is basically the same as Explod" -- and whose difference is that it
    // is not a creation. See `ExplodModify`.
    void doModifyExplod(const Controller& controller);
    // `RemoveExplod`, whose two cases are a named id and "every explod the player owns".
    void doRemoveExplod(const Controller& controller);
    // `GameMakeAnim`, the controller the reference calls "superseded by Explod and ...
    // now considered deprecated" and 37 of the corpus' packs still call 115 times. It is
    // an explod with almost no knobs: an animation out of fightfx, a place, a jitter,
    // and which side of the fighters to draw it on.
    void doGameMakeAnim(const Controller& controller);
    void doProjectile(const Controller& controller);
    void doHelper(const Controller& controller);
    // The `Target*` family: the machine acts on whoever its move last connected
    // with. They are most of how a throw is written -- TargetState poses the
    // victim, TargetLifeAdd costs it life on the slam, TargetBind is how the
    // victim would be dragged without its own bindtotarget.
    void doTargetOp(const Controller& controller);
    // Bind this machine to an anchor: the receiving half of TargetBind. The
    // parameters are read once by the caller and handed over resolved.
    void bindTo(StateMachine* anchor, int time, int facing, float offsetX, float offsetY);
    // `Pause`/`SuperPause`: freeze the match around this machine. The freeze itself
    // lives on the entity sink, because every machine -- both fighters, every
    // helper -- has to read the same one.
    void doPause(const Controller& controller, bool super);
    // `ParentVarSet`/`ParentVarAdd`: a helper writing its parent's var/fvar. On a
    // root they have no meaning and are dropped, as MUGEN does.
    void doParentVar(const Controller& controller, bool additive);
    // `BindToParent`/`BindToRoot`/`BindToTarget`: follow another machine's position
    // for a while. time -1 binds for ever. Applied in step() even through hitpause,
    // because a thrown victim is dragged by exactly this while both fighters freeze.
    void doBind(const Controller& controller, int kind);  // 0 parent, 1 root, 2 target
    // The machine this machine is bound to right now, in its facing frame.
    void applyBind();
    void doHitVelSet(const Controller& controller);
    // The three controllers that spend a fall landing's pending effects. Each is a
    // no-op once its effect has been spent, which is what lets a file that calls
    // them from two states charge the fall only once. See `landingScriptedBy`.
    void doHitFallDamage();
    void doHitFallVel();
    void doFallEnvShake();
    // The spends themselves, shared by the controllers above and by `land()`'s
    // built-in landing. Both are no-ops once the pending effect is gone.
    void applyFallDamage();
    void applyFallShake();
    // Reads the character's file once to learn whether it scripts those three
    // itself, which decides whether `land()` applies them or leaves them to the
    // file. Cheap to re-run and cached against the character pointer, so a round
    // reset or a second fighter sharing the file costs nothing.
    void scanLanding();
    // Reads the character's file once to learn whether it brings its own guard
    // states. Same caching as `scanLanding`. See `scriptedGuardChain_`.
    void scanGuardChain();
    void doAssertSpecial(const Controller& controller);
    void doNotHitBy(const Controller& controller);
    void doHitBy(const Controller& controller);
    void doEnvShake(const Controller& controller);
    void doEnvColor(const Controller& controller);
    // `Trans`: this tick's transparency for the fighter's own body. The controller
    // is documented to hold "for current game tick" only, so the machine stores it
    // for the tick and clears it in step() -- see transThisTick().
    void doTrans(const Controller& controller);
    // `AfterImage` turns the trail on with its own parameters; `AfterImageTime`
    // turns on the state's trail settings for n ticks. The fighter draws the
    // ghosts; the machine only keeps the knobs.
    void doAfterImage(const Controller& controller);
    // `PalFX` and `RemapPal`. The machine records the per-fighter grade; Fighter::draw
    // is what renders it, since the LUT and the grade are the renderer's business.
    // `BGPalFX` and `AllPalFX` are deliberately not on the machine: they grade the whole
    // screen rather than one fighter, so runController forwards them to the fight's two
    // global slots through the entity sink. (There is no PalCopy controller in MUGEN 1.0
    // at all; the token does not exist in the binary's string table.)
    void doPalFx(const Controller& controller);
    void doRemapPal(const Controller& controller);
    // Reads a PalFX-family controller into a slot, in MUGEN's units. PalFX, BGPalFX and
    // AllPalFX share this arithmetic exactly -- only the slot they write to differs --
    // so it is factored out here rather than written once per scope.
    [[nodiscard]] PalFxState readPalFxState(const Controller& controller) const;
    // Where an entity's `pos` lands in this engine's world frame, for a `postype`.
    void resolveEntitySpawn(int type, float offsetX, float offsetY, float* outX, float* outY);
    void doVariables(const Controller& controller);
    // Returns true when the controller's type was recognised and handled.
    bool runController(const Controller& controller);

    void stepPhysics();
    // The landing state to enter when an air state reaches the ground. Picks by
    // how the fighter arrived, since a jump, a knockback and a knockout all
    // land into different states.
    void land();
    void updateAnimation();

    void setAnim(int number, bool resetTime);
    [[nodiscard]] const Animation* animation(int number) const;

    // The frame the current animation is showing and how far into it, in the
    // terms the animation triggers are written in.
    [[nodiscard]] const Animation* currentAnimPtr() const;
    [[nodiscard]] int currentElem() const;
    // Ticks *remaining* in the animation, which is the direction MUGEN's
    // `animtime` counts in.
    [[nodiscard]] long long currentAnimTime() const;
    // True when the animation has run out: it does not loop and is not holding.
    // MUGEN's `AnimTime = 0`, which is what every hand-over in the format waits
    // on. See the definition for why it is not its own predicate.
    [[nodiscard]] bool animationFinished() const;

    // The body's edge in the direction the fighter faces. Read from the current
    // animation's hurt boxes, which is where a character's width actually
    // lives -- a crouching fighter is narrower than a standing one.
    [[nodiscard]] float bodyExtentTowardOpponent() const;

    // The reference's width bar, towards another player's x: the raw `[Size]`
    // width for this fighter's posture (ground/air, front/back by which way it
    // faces) with the Width controller's player bar added. Zero with no
    // character behind the machine. See `separate`.
    [[nodiscard]] int widthToward(float otherX) const;
    // The bar's offset from the x-axis on one side, as the body edge triggers
    // measure it: the edge width, plus the reference's +1 in the air and +2
    // lying down. Front/back are absolute, not facing-flipped.
    [[nodiscard]] int edgeBodyOffset(bool front) const;

    // --- the AI, which plays the character's own commands back
    void beginAiMove(bool jumpFirst);

    // Helpers for the trigger switch, split out to keep it readable.
    [[nodiscard]] Value ownTrigger(std::string_view name, const std::vector<Value>& args) const;
    [[nodiscard]] Value opponentTrigger(std::string_view name,
                                        const std::vector<Value>& args) const;
    [[nodiscard]] Value constTrigger(const std::vector<Value>& args) const;

    // Two x coordinate frames, and the distinction matters because MUGEN uses
    // both:
    //
    //   * Velocity and displacement are in the *local* frame -- +x is the way
    //     the fighter is looking. `velset x = const(velocity.walk.fwd.x)` is one
    //     state and one sign for both directions, `posadd x = 10` is a step
    //     forward, and `vel x > 0` means "moving forward". Every character
    //     relies on this; a set of states written once plays mirrored.
    //   * Position is *absolute*, measured from the middle of the stage.
    //     `pos x > 190` is a wall check, and characters teleport with
    //     `posset x = enemynear, pos x + ifelse(facing = 1, 80, -80)`, which
    //     only lands where they mean it if both positions are absolute.
    [[nodiscard]] float localVelX() const { return velX_ * static_cast<float>(facing_); }
    [[nodiscard]] float worldX(float local) const { return static_cast<float>(facing_) * local; }
    [[nodiscard]] float posX() const { return x_ - stageCenter(); }
    [[nodiscard]] float posY() const { return -y_; }

    // The distance to the opponent along x, positive when they are in front,
    // and the same measured edge-to-edge between the two bodies.
    [[nodiscard]] float p2DistX() const;
    [[nodiscard]] float p2BodyDistX() const;

    // --- the world
    const Character* character_ = nullptr;
    StateMachine* opponent_ = nullptr;
    const StateLibrary* states_ = nullptr;
    const CommandList* commands_ = nullptr;

    // The family. A root has parent_ null and root_ itself; a helper links upward
    // and registers on the root's list. Pointers are weak: the fight owns every
    // machine, and a helper leaving (DestroySelf) removes itself from the list.
    StateMachine* parent_ = nullptr;
    StateMachine* root_ = this;
    bool isHelper_ = false;
    int helperId_ = 0;
    std::string helperName_;
    bool keyCtrl_ = false;
    int pauseMoveTime_ = 0;
    int superMoveTime_ = 0;
    bool destroyed_ = false;
    std::vector<StateMachine*> helpers_;  // whole family's, valid on the root only
    StateMachine* lastTarget_ = nullptr;  // the victim the armed move last connected with
    // The `id` of the HitDef that last *hit* this machine, which is the question a
    // later hit's `chainID` and `nochainID` are asked against. Held on the victim
    // because that is whose history it is: the attacker cannot know what last
    // touched this fighter, and in a team fight it may not have been the attacker
    // at all. -1 is both "nothing has hit me" and "no restriction".
    int lastHitById_ = -1;
    // How many hits this machine has *taken* in the combo it is currently in,
    // which is what `GetHitVar(hitcount)` answers. It is the victim's counter,
    // not the attacker's `hitCount_` above: the two are different numbers with
    // the same name, and the reference keeps them in different places. Reset
    // when a hit arrives while this fighter was *not* already in a hit reaction
    // (that is the start of a new combo, by definition) and bumped on every hit
    // that lands unguarded, which is also what a blocked hit fails to do.
    int hitsTakenInCombo_ = 0;
    // Consecutive ticks this machine has spent *outside* a hit reaction, which
    // is the clock the combo rule above is actually asked against. The
    // reference does not test "am I in a hit reaction right now" but "have I
    // been out of one for more than a single tick", and the difference is
    // visible in play: a victim that spends one free frame between two hits --
    // which is what a move whose hitpause and hitstun end together produces --
    // is still in the same combo, and only from the *second* free frame on does
    // the next hit start a new one. Cleared on every tick spent in a hit
    // reaction, bumped on every tick not spent in one.
    int framesSinceHitReaction_ = 0;
    // Ticks left before a downed fighter is stood up, which is what
    // `GetHitVar(recovertime)` reports. Armed from `[Data] liedown.time` on every
    // tick this fighter is *not* in a hit reaction -- so it reads the file's full
    // number for a fighter on its feet, which is what the key means to a state
    // that asks before the knockdown has happened -- and counted down only while
    // lying in state 5110. It is the engine's clock, not the animation's: the
    // reference's common1.cns 5110 has no ChangeState to 5120 at all, and a
    // knocked-down fighter stands up after this many ticks whether or not its
    // lying animation has finished.
    int liedownTicks_ = 0;
    int playerId_ = 0;

    // A bind in progress: follow bindMachine_'s position by bindOffset_, in the
    // anchor's facing frame, until bindTime_ runs out (never, when negative).
    StateMachine* bindMachine_ = nullptr;
    int bindTime_ = 0;
    int bindFacing_ = 0;  // 0 keep, 1 face anchor's facing, -1 face away
    float bindOffsetX_ = 0.0f;
    float bindOffsetY_ = 0.0f;

    float x_ = 0.0f;
    float y_ = 0.0f;
    float velX_ = 0.0f;
    float velY_ = 0.0f;
    int facing_ = 1;
    float offsetX_ = 0.0f;  // visual only, from the Offset controller
    float offsetY_ = 0.0f;
    float stageMin_ = 90.0f;
    float stageMax_ = 550.0f;
    bool screenBound_ = true;
    bool playerPush_ = true;
    // MUGEN's width bar, the reference's four ints: edge width is how far the
    // width bar may reach towards the screen edge, and player width is added to
    // the `[Size]` width when two bodies are pushed apart (never substituted for
    // it). Both pairs are front/back in the fighter's own frame, stored
    // absolutely -- no facing flip here; the reader picks the side. Zero at rest
    // and cleared at the top of every tick, which is the docs' "for 1 tick".
    int edgeFront_ = 0;
    int edgeBack_ = 0;
    int playerFront_ = 0;
    int playerBack_ = 0;
    bool posFreezeThisTick_ = false;
    // A state change that fired under hitpause. The reference's runner applies
    // a state's entry block (the statedef header, the bookkeeping clears, the
    // animation) only outside the freeze, so an `ignorehitpause` ChangeState
    // moves the state now and has its header applied when the freeze lifts.
    // The number and the zeroed clock are already in place; this says the rest
    // is still owed.
    bool pendingStateEntry_ = false;
    std::optional<TransParams> transThisTick_;
    bool transAddAlphaThisTick_ = false;
    AfterImageParams afterImage_;
    // The PalFX grade currently on the fighter, and the palette selection:
    // palNo_ is the 1-based selection (what `palno` answers) and palRemaps_ is
    // what RemapPal turned into, a map from palette identity to palette
    // identity. Both are sticky across states, exactly as MUGEN's are: a state
    // change does not wash a flash off.
    PalFxState palFx_;
    int palNo_ = 1;
    std::vector<PaletteRemap> palRemaps_;
    // `AttackMulSet`/`DefenceMulSet`: sticky multipliers. Attack scales the damage
    // this machine's HitDefs deal; defence scales what it takes. Both reset to 1
    // on state change? No -- MUGEN keeps them until rewritten, which is what makes
    // a power-up state write one once and leave it.
    float attackMul_ = 1.0f;
    float defenceMul_ = 1.0f;
    // Whether the reaction the last hit put this fighter in is a fall, for
    // `hitfall` and for the built-in table's own splits (5030, 5080, and now the
    // 5000/5010 pair). Written by `applyHit` from the hit's `falls` and
    // overridable by the character's `HitFallSet`.
    bool hitFallFlag_ = false;
    // The fall landing owed by the last hit taken. Each of these is drained by
    // whichever of the two runs first -- the `land()` arrival, or the character's
    // own `HitFallDamage`/`HitFallVel`/`FallEnvShake` controller -- and being
    // *drained* rather than merely read is the whole of the once-only guarantee:
    // the reference's common1.cns calls `HitFallDamage` in state 5100 at time 3
    // and again in 5110 at time 0, so a fall that went 5100 → 5110 and one that
    // went 5100 → 5101 → 5110 would be charged twice and once respectively by an
    // engine that did not consume. `fallShakeTime_` doubles as the answer to
    // `GetHitVar(fall.envshake.time)`, which the reference's `FallEnvShake`
    // contract is written in terms of ("effective only if ... is not zero, and it
    // sets ... to zero after being executed").
    int fallDamagePending_ = 0;
    // The `fall.kill` of the hit that armed that damage. Default 1: a fall the
    // engine was never told about -- or one armed by an older hit with no
    // opinion -- is lethal, which is MUGEN's default and the louder failure.
    bool fallKillPending_ = true;
    float fallVelX_ = 0.0f;
    float fallVelY_ = 4.5f;
    int fallShakeTime_ = 0;
    float fallShakeFreq_ = 60.0f;
    float fallShakeAmpl_ = 0.0f;
    // Whether the character's file scripts the landing effects itself, and the
    // file the answer was read from. See `scanLanding`.
    const Character* landingScriptedBy_ = nullptr;
    bool scriptedFallDamage_ = false;
    bool scriptedFallVel_ = false;
    bool scriptedFallShake_ = false;
    // Whether the character ships the guard chain -- the held states 130/131/132
    // and the 140 the release goes through -- in a common state file of its own.
    // See `scanGuardChain`: when it does, the engine's copies of the reference's
    // guard controllers stand down, because the file's are the reference's and
    // two packs in the corpus deliberately rewrite them. The one guard piece that
    // is the engine's for *every* character is 140's own exit, which the reference
    // file leaves commented out as "Implemented within engine".
    const Character* guardScriptedBy_ = nullptr;
    bool scriptedGuardChain_ = false;
    // The persistent draw angle, in degrees. Sticky like MUGEN's AngleSet.
    float drawAngleDeg_ = 0.0f;

    int stateNumber_ = CommonState::kStand;
    int prevStateNumber_ = CommonState::kStand;
    int stateTime_ = 0;
    bool ctrl_ = true;
    StateType stateType_ = StateType::Stand;
    MoveType moveType_ = MoveType::Idle;
    Physics physics_ = Physics::Stand;

    int anim_ = CommonState::kStand;
    long long animTime_ = 0;
    bool animChanged_ = false;
    int spritePriority_ = 0;
    // Null unless a ChangeAnim2 is posing this fighter with another character's
    // file (the attacker of a throw). Cleared when the state changes.
    const Character* animOverrideChar_ = nullptr;
    // The file this fighter is *running*, when it is not its own: set when a hit
    // writing `p2stateno` places it in a custom state and `p2getp1state` is left at
    // its default of 1, which is how a command grab poses its victim out of the
    // grabber's own states. It survives every state change, including changes made
    // by the borrowed file's own controllers -- the pair travels together until,
    // in the reference's words, "P2 is hit, or a SelfState controller is used to
    // return P2 to his own states". Those two are the only things that clear it.
    const Character* borrowedStateChar_ = nullptr;

    int life_ = 1000;
    int maxLife_ = 1000;
    int power_ = 0;
    bool tookDamage_ = false;

    // Hitstun, in ticks. While it runs the fighter is in a hit state and
    // control is suppressed. It counts the hit's `hittime` down through zero to
    // -1 and stops there, because that is the reference's own layout (`+0x1094`
    // decrements while `>= 0` and `GetHitVar(hittime)` reads the live value), and
    // because `HitOver` is defined as `hittime < 0`: the trigger has to turn true
    // one tick *after* the number reaches zero, not on the tick it does. A
    // fighter that has not been hit sits at -1, which is where the reference's
    // reset branch leaves it.
    int stunTicks_ = -1;
    // Hit states end by returning to state 0 after their animation finishes or
    // their hit time expires, whichever the HitDef asked for.
    int hitTime_ = 0;
    int hitSlide_ = 0;
    float yAccel_ = 0.44f;

    // Combat bookkeeping the triggers read. Move-contact et al are tick
    // counters, not flags: a contact sets them to one and each later tick
    // counts on from there, so a file can ask `movecontact = 3` or open a
    // window around the connect. A state entry spends them again.
    int moveContact_ = 0;
    int moveGuarded_ = 0;
    int moveHit_ = 0;
    int moveReversed_ = 0;
    int hitCount_ = 0;
    int uniqHitCount_ = 0;
    int jugglePoints_ = 0;

    // Hit pause and hit shake, both counted down in step().
    int pauseTicks_ = 0;
    int shakeTicks_ = 0;
    float shakeX_ = 0.0f;
    // Whether the opponent is swinging from within range to be guarded, which is
    // MUGEN's `inguarddist`. Owned by the fight loop.
    bool inGuardDist_ = false;

    // Guard energy, and how long it has been since the last block. A fighter being
    // pressed never gets the rest that recovery waits for, which is the whole of what
    // the meter says.
    int guardEnergy_ = kGuardEnergyMax;
    int guardRestTicks_ = 0;

    // 60 ints and 40 floats, MUGEN's own sizes. (sysfvar is the float half of the
    // system set.) Mutable because triggers read them through const methods and
    // `:=` writes them through one too.
    mutable std::array<int, 60> vars_{};
    mutable std::array<float, 40> fVars_{};
    mutable std::array<int, 5> sysVars_{};
    mutable std::array<float, 5> sysFVars_{};

    // A HitDef is armed by the HitDef controller and stays armed until the
    // state ends, which is what lets a single HitDef cover a whole animation.
    HitDef hitDef_;
    // `[Size] attack.dist` and `proj.attack.dist`: 160 and 90, which is what the
    // reference's own `[Size]` reader falls back to when a file writes neither, and
    // which seventeen packs here write out anyway. The two names exist for exactly the
    // two cases the guard-distance question has -- an attack, and a projectile.
    static constexpr int kDefaultAttackDist = 160;
    static constexpr int kDefaultProjAttackDist = 90;
    // The player's live guard distance, which `inguarddist` reads and which is *not*
    // the armed HitDef's: see `guardDist()`. It outlives any one HitDef, and it is
    // seeded at state entry rather than when a HitDef arrives, because a state in
    // `movetype = A` with nothing armed still has one. The `[Size] attack.dist` default
    // is applied where it is seeded, since that is the player's own constant table.
    int guardDist_ = kDefaultAttackDist;
    // The last HitDef this machine armed, even after the state moved on: the
    // `hitdefattr` chain check asks about what it *did*, not what it is doing.
    HitDef lastUsedHitDef_;
    // The hit that last landed on this fighter, for `gethitvar`.
    HitInfo lastHit_;

    CommandBuffer buffer_;
    InputState lastInput_;

    SoundSink* sound_ = nullptr;
    EntitySink* entities_ = nullptr;
    int side_ = 0;
    // Cleared every tick by step(), then raised again by whatever AssertSpecial runs.
    uint32_t assert_ = 0;
    int invisibleTicks_ = 0;
    int notHitTicks_ = 0;
    int notHitThrowTicks_ = 0;
    std::array<HitOverrideSlot, kHitOverrideSlots> hitOverrides_{};
    ReversalDef reversalDef_;

    int aiLevel_ = 5;
    bool aiControlled_ = false;
    int aiTicks_ = 0;
    // What the computer wants its feet to do between moves. Named rather than
    // numbered because it is a three-way choice and the arithmetic signs read as
    // velocities -- `kAiBack` is a guard as much as it is a retreat, since holding
    // back is the format's only defence.
    enum AiWalk : int { kAiBack = -1, kAiHold = 0, kAiToward = 1 };
    int aiWalk_ = kAiHold;
    uint32_t aiSeed_ = 1;
    // `random` advances this, so it cannot be const even though trigger() is.
    mutable uint32_t randomSeed_ = 1;

    // The AI plays the character's own moves by *playing back its commands*:
    // it picks one of the character's real .cmd entries and feeds the input
    // that fires it, one element a tick. That way the CPU uses whatever move set
    // the character has, including a character it has never seen, instead of a
    // hand-written set of moves the character may not have.
    std::vector<CmdStep> aiScript_;
    size_t aiScriptStep_ = 0;
    int aiScriptTicks_ = 0;

    long long gameTime_ = 0;
    int roundState_ = 2;  // 2 while the round is being fought
    int roundNo_ = 1;
    int roundOutcome_ = -1;  // -1 undecided, 0 lost, 1 won, 2 drawn
    int matchNo_ = 1;
    bool matchOver_ = false;
    // The index `VictoryQuote` recorded for the next victory screen. -1 means
    // random, which is also the state before any controller runs. Root-only in
    // effect: the controller refuses to write it on a helper.
    int victoryQuoteIndex_ = kVictoryQuoteRandom;

    // How many times one tick may change state. A character whose states point
    // at each other in a cycle never leaves its state logic otherwise, and a
    // hang in the fight loop is far worse than a state machine that stops
    // following the file.
    int stateChangesThisTick_ = 0;
    static constexpr int kMaxStateChangesPerTick = 24;

    // Controllers whose type this machine does not implement, recorded so the
    // gap can be reported once per character instead of every tick.
    mutable std::unordered_map<std::string, int> unsupported_;
    // `persistent = n` bookkeeping: when each controller last ran, as a game tick.
    // Cleared on state entry -- the counter is a property of the *stay*, as MUGEN
    // resets it on each new state.
    std::unordered_map<const Controller*, long long> lastRunTick_;

    friend class Fighter;
};

}  // namespace af

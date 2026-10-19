#include "engine/fighter.h"

#include <algorithm>
#include <cmath>

#include "engine/motif.h"
#include "engine/projectile_rules.h"
#include "engine/shadow.h"
#include "core/log.h"

namespace af {

namespace {

// The stage the fight runs on. The procedural arena is drawn wide so the camera
// has room to move; the fighters cannot walk off either end.
constexpr float kStageMin = 90.0f;
constexpr float kStageMax = 550.0f;

// Power gained per landed hit, on the 3000 scale the power bars want.
constexpr int kPowerGain = 90;

// The placeholder's own numbers. It has no character and so no files to read
// them from, and it exists only so the debug arena has something to hit.
constexpr float kPlaceholderWalkFwd = 3.0f;
constexpr float kPlaceholderWalkBack = 2.5f;
constexpr float kPlaceholderJump = 7.8f;
constexpr float kPlaceholderJumpCarry = 0.45f;
constexpr int kPlaceholderPunchDamage = 46;
constexpr int kPlaceholderKickDamage = 64;
constexpr int kPlaceholderPunchTicks = 14;
constexpr int kPlaceholderKickTicks = 20;
constexpr int kPlaceholderHurtStun = 16;

// The placeholder's hurt box, in its own units: as wide and as tall as the box
// the fight screen draws for it.
constexpr float kPlaceholderHalfWidth = 12.0f;
constexpr float kPlaceholderHeight = 64.0f;

// How tall a fighter is drawn, in the screenpack's units.
//
// This is Kung Fu Man's standing art -- 106 pixels in his own 320x240 space --
// and it is the whole of how the roster is brought to one size. Every character
// in it declares the same 320x240 coordinate space, so MUGEN's own scaling has
// nothing to do, and then draws its standing pose at anything from 70 pixels to
// 200. Nothing about the format makes those two characters the same size, and
// nothing about them is: the big one reaches half again as far with half again
// as much body, which is not a match.
//
// What is trusted here is the art, not `[Size] height`. The declared height is
// not reliable enough to scale by -- one character on this roster claims to be
// 150 units tall and draws 90 pixels, another claims 60 and draws 200 -- and
// normalising by it would make that character two and a half times the size of
// the rest.
constexpr float kReferenceStandingHeight = 106.0f;

bool overlap(const Rect4& a, const Rect4& b) {
    return a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3];
}

}  // namespace

void Fighter::setFallbackLook(const std::string& name, Color body, Color trim) {
    fallbackName_ = name;
    bodyColor_ = body;
    trimColor_ = trim;
}

void Fighter::setup(const Character* character, float startX, int facing) {
    // The pointer is handed out for drawing, which uploads textures on first
    // use; the roster itself is fixed once loaded, so the cast is stable.
    character_ = const_cast<Character*>(character);

    machine_.setup(character, facing);
    machine_.setPosition(startX);
    machine_.setStageBounds(kStageMin, kStageMax);
    clearGhosts();

    everTookDamage_ = false;
    matchTicks_ = 0;
    placeholderPhase_ = Phase::Stand;
    placeholderTicks_ = 0;
    placeholderAttackTicks_ = 0;
    placeholderAttackAnim_ = kPunchAnim;
    placeholderHasHit_ = false;
}

void Fighter::setScale(float baseScale) {
    baseScale_ = std::max(0.001f, baseScale);
    scale_ = baseScale_;
    if (character_ != nullptr && character_->standingHeight() > 1.0f) {
        // The art and the collision boxes are scaled together, because they are
        // the same drawing: a box is authored against the sprite it belongs to,
        // and scaling only the pixels would leave every character hitting with
        // the reach of a fighter it no longer is.
        //
        // Motion -- how fast a walk goes, how high a jump reaches -- is
        // deliberately left alone. Those are the character's own numbers in its
        // own units and they stay as its author tuned them; what this fixes is
        // that two fighters can be the same size as each other.
        scale_ = baseScale_ * kReferenceStandingHeight / character_->standingHeight();
    }
    if (character_ != nullptr) {
        AF_LOGD("fighter %s: scale %.3f (art %.0f tall in a %.0f-unit space, declares %.0f tall)",
                character_->name().c_str(), scale_, character_->standingHeight(),
                character_->localCoord().y, character_->sizeHeight());
    }
}

void Fighter::setAiDifficulty(int level) { machine_.setAiLevel(level); }

const std::string& Fighter::name() const {
    return character_ != nullptr ? character_->displayName() : fallbackName_;
}

Fighter::Phase Fighter::phase() const {

    if (character_ == nullptr) return placeholderPhase_;
    if (machine_.knockedOut()) return Phase::KO;
    // The move type is what the character's own states declare they are doing,
    // so the HUD reads the same thing the state machine is acting on.
    switch (machine_.moveType()) {
        case MoveType::Attack: return Phase::Attack;
        case MoveType::Hit: return Phase::Hurt;
        default: break;
    }
    if (machine_.stateType() == StateType::Air) return Phase::Air;
    // MUGEN's walking states are 20 and 21, and a character that keeps its own
    // numbering says so with `statetype`, which the check above already read.
    const int state = machine_.stateNumber();
    if (state == 20 || state == 21) return Phase::Walk;
    return Phase::Stand;
}

// ----------------------------------------------------------------------- input

void Fighter::control(const InputState& input) {
    // Whoever is on the pad is not the computer, which the characters ask about
    // through `ailevel`.
    machine_.setAiControlled(false);
    if (character_ == nullptr) {
        controlPlaceholder(input);
        return;
    }
    machine_.control(input);
}

void Fighter::controlAI(Fighter& opponent) {
    machine_.setOpponent(&opponent.machine_);
    if (character_ == nullptr) {
        // The placeholder has no commands to play back, so it walks in and
        // throws the two moves it has. It is a target, not an opponent.
        InputState input;
        const float gap = opponent.x() - machine_.x();
        const int toward = gap > 0.0f ? 1 : -1;
        if (std::fabs(gap) > 40.0f) {
            input.held = toward > 0 ? Button::kRight : Button::kLeft;
        } else if (--placeholderTicks_ <= 0) {
            placeholderTicks_ = 30;
            input.pressed = Button::kX;
            input.held = Button::kX;
        }
        controlPlaceholder(input);
        return;
    }
    machine_.controlAI();
}

void Fighter::controlPlaceholder(const InputState& input) {
    if (placeholderPhase_ != Phase::Stand && placeholderPhase_ != Phase::Walk) return;

    const int axis = input.horizontalAxis();
    if (input.wasPressed(Button::kA) || input.wasPressed(Button::kB) ||
        input.wasPressed(Button::kC)) {
        placeholderAttackAnim_ = kPunchAnim;
        placeholderAttackTicks_ = kPlaceholderPunchTicks;
        placeholderHasHit_ = false;
        placeholderPhase_ = Phase::Attack;
        machine_.velX_ = 0.0f;
        return;
    }
    if (input.wasPressed(Button::kX) || input.wasPressed(Button::kY) ||
        input.wasPressed(Button::kZ)) {
        placeholderAttackAnim_ = kKickAnim;
        placeholderAttackTicks_ = kPlaceholderKickTicks;
        placeholderHasHit_ = false;
        placeholderPhase_ = Phase::Attack;
        machine_.velX_ = 0.0f;
        return;
    }
    if (input.wasPressed(Button::kUp)) {
        machine_.velY_ = kPlaceholderJump;
        machine_.velX_ = static_cast<float>(axis) * kPlaceholderJump * kPlaceholderJumpCarry;
        machine_.physics_ = Physics::Air;
        placeholderPhase_ = Phase::Air;
        return;
    }
    if (axis != 0) {
        const bool forward = (axis > 0) == (machine_.facing_ > 0);
        machine_.velX_ =
            static_cast<float>(axis) * (forward ? kPlaceholderWalkFwd : kPlaceholderWalkBack);
        placeholderPhase_ = Phase::Walk;
    } else {
        machine_.velX_ = 0.0f;
        placeholderPhase_ = Phase::Stand;
    }
}

// --------------------------------------------------------------------- physics

void Fighter::step(Fighter& opponent) {
    machine_.setOpponent(&opponent.machine_);
    machine_.setGameTime(matchTicks_++);
    machine_.step();
    recordGhost();

    if (character_ != nullptr) return;

    // The placeholder's own clocks: how long an attack lasts, how long a hit
    // stuns, and when it has landed from a jump.
    if (placeholderPhase_ == Phase::Air && machine_.y_ <= 0.0f) {
        placeholderPhase_ = Phase::Stand;
        machine_.physics_ = Physics::Stand;
        machine_.velX_ = 0.0f;
    }
    if (placeholderPhase_ == Phase::Attack && --placeholderAttackTicks_ <= 0) {
        placeholderPhase_ = Phase::Stand;
        machine_.velX_ = 0.0f;
    }
    if (placeholderPhase_ == Phase::Hurt && --placeholderAttackTicks_ <= 0) {
        placeholderPhase_ = machine_.life() <= 0 ? Phase::KO : Phase::Stand;
        // The machine's own hitstun is suspended for a fighter it is not
        // running; clearing it here is what lets the placeholder face its
        // opponent again after a hit. "Clear" is -1, the engine's not-stunned
        // value: the clock counts through 0 to -1 so that `HitOver` is a
        // strictly-less-than test, as it is in the reference.
        machine_.stunTicks_ = -1;
    }
}

// ------------------------------------------------------------------- hitboxes

bool Fighter::overlapsEntityBox(const CollisionBox& box, int facing, float originX, float originY,
                               float floorY) const {
    // The same conversion `worldBox` makes, with the projectile's own position standing in
    // for a fighter's: the boxes are relative to the animation's axis, which the
    // projectile carries. `projectileBoxToWorld` has no scale parameter on purpose, which
    // is what keeps `projscale` out of collision (§7.2).
    const ProjectileRect world =
        projectileBoxToWorld(box.x1, box.y1, box.x2, box.y2, facing, originX, originY, floorY);
    return overlap(Rect4{world.x1, world.y1, world.x2, world.y2}, worldHurtBox(floorY));
}

bool Fighter::attackOverlapsEntityBox(const CollisionBox& box, int facing, float originX,
                                      float originY, float floorY) const {
    // The reverse of overlapsEntityBox: the entity's box is the body (a
    // projectile's Clsn2) and this fighter's Clsn1 boxes are the attack. An
    // armed HitDef is required as well as a boxed frame, so a move whose
    // `hitcount` is spent cannot keep shooting projectiles down; the reference's
    // pass reads a live attack list, which is the same idea.
    if (!machine_.hasArmedHitDef()) return false;
    const Animation* animation = machine_.currentAnimation();
    if (animation == nullptr) return false;
    const int frameIndex = animation->frameAt(machine_.animTime());
    if (frameIndex < 0 || frameIndex >= static_cast<int>(animation->attackBoxes.size())) {
        return false;
    }
    const std::vector<CollisionBox>& boxes =
        animation->attackBoxes[static_cast<size_t>(frameIndex)];
    if (boxes.empty()) return false;
    // No scale on the entity side, for the same reason as above: `projscale` is
    // draw-only. The fighter side does use `worldBox`, which carries its own draw
    // scale, exactly as every melee hit does.
    const ProjectileRect entity =
        projectileBoxToWorld(box.x1, box.y1, box.x2, box.y2, facing, originX, originY, floorY);
    const Rect4 hurt{entity.x1, entity.y1, entity.x2, entity.y2};
    for (const CollisionBox& attack : boxes) {
        if (overlap(worldBox(attack, floorY), hurt)) return true;
    }
    return false;
}

Rect4 Fighter::worldBox(const CollisionBox& box, float floorY) const {
    const float facing = static_cast<float>(machine_.facing()) * scale_;
    const float x1 = machine_.x() + facing * box.x1;
    const float x2 = machine_.x() + facing * box.x2;
    // A collision box is written in the character's own space, where y is
    // negative *upward*: Kung Fu Man's standing body is `Clsn2[0] = -13, 0,
    // 16,-79`, which is 79 units above the feet, not 79 below them. `feet` is
    // the world y of the fighter's feet and the world grows downward from the
    // floor line, so the box's y is *added* -- which is what the renderer does
    // with a frame's `offsetY` when it draws the sprite (see the `pivotY` in
    // `draw`), and a hitbox that disagreed with the drawing would be a hitbox
    // for a fighter nobody can see.
    //
    // Subtracting instead put every box on the far side of the floor line from
    // the fighter it belongs to. Nothing noticed, because attack and hurt boxes
    // went through the same routine and so were mirrored *together*, and a pair
    // of boxes mirrored about a common line still overlap where they did -- but
    // the fighter with no art, whose hurt box is the placeholder's and comes
    // from the other branch below, was on the right side of the line while the
    // one with art was not, so the two never matched.
    const float feet = floorY - machine_.y();
    const float y1 = feet + box.y1 * scale_;
    const float y2 = feet + box.y2 * scale_;
    return Rect4{std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2)};
}

Rect4 Fighter::worldHurtBox(float floorY) const {
    // The placeholder's box, for a fighter whose art supplies none: as wide and
    // as tall as the box the fight screen draws for it, standing on the floor
    // line. It is the one box in the engine written directly in world
    // coordinates rather than converted from a character's own, which is why it
    // is the reference the conversion in `worldBox` has to agree with.
    const Animation* animation = machine_.currentAnimation();
    if (animation == nullptr || animation->hurtBoxes.empty()) {
        const float feet = floorY - machine_.y();
        return Rect4{machine_.x() - kPlaceholderHalfWidth, feet - kPlaceholderHeight,
                     machine_.x() + kPlaceholderHalfWidth, feet};
    }
    const int frameIndex = animation->frameAt(machine_.animTime());
    if (frameIndex < 0 || frameIndex >= static_cast<int>(animation->hurtBoxes.size()) ||
        animation->hurtBoxes[static_cast<size_t>(frameIndex)].empty()) {
        const float feet = floorY - machine_.y();
        return Rect4{machine_.x() - kPlaceholderHalfWidth, feet - kPlaceholderHeight,
                     machine_.x() + kPlaceholderHalfWidth, feet};
    }
    // MUGEN takes the union of every Clsn2 box: a hurt box made of several
    // pieces is one target, and using only the first would let a move pass
    // through the rest of the body.
    const std::vector<CollisionBox>& boxes =
        animation->hurtBoxes[static_cast<size_t>(frameIndex)];
    Rect4 box = worldBox(boxes.front(), floorY);
    for (size_t i = 1; i < boxes.size(); ++i) {
        const Rect4 other = worldBox(boxes[i], floorY);
        box[0] = std::min(box[0], other[0]);
        box[1] = std::min(box[1], other[1]);
        box[2] = std::max(box[2], other[2]);
        box[3] = std::max(box[3], other[3]);
    }
    return box;
}

bool Fighter::tryHit(Fighter& target, float floorY, bool* landed) {
    if (landed != nullptr) *landed = false;
    if (target.life() <= 0) return false;
    // `NotHitBy`: an intro, or the startup of a move that is meant to go through
    // whatever is coming. Nothing can touch the fighter while it lasts.
    if (target.machine_.ignoringHits()) return false;
    // The throw-only window: normal attacks pass, throws do not. It is what
    // `[State 5120, 5] NotHitBy value = , NT,ST,HT time = 12` buys a fighter
    // for the moment after it is back on its feet.
    if (target.machine_.ignoringThrows() && machine_.armedHitIsThrow()) return false;

    // The placeholder attacks with a fixed box in front of it, since it has no
    // animation to read collision boxes from.
    if (character_ == nullptr) {
        if (placeholderPhase_ != Phase::Attack || placeholderHasHit_) return false;
        const float facing = static_cast<float>(machine_.facing());
        const float feet = floorY - machine_.y();
        const Rect4 attack{machine_.x() + facing * 8.0f, feet - 56.0f,
                           machine_.x() + facing * 44.0f, feet - 20.0f};
        if (attack[0] > attack[2]) return false;
        const Rect4 normalised{std::min(attack[0], attack[2]), attack[1],
                               std::max(attack[0], attack[2]), attack[3]};
        if (!overlap(normalised, target.worldHurtBox(floorY))) return false;
        placeholderHasHit_ = true;
        target.takeHit(placeholderAttackAnim_ == kPunchAnim ? kPlaceholderPunchDamage
                                                            : kPlaceholderKickDamage,
                       machine_.facing());
        gainPower(kPowerGain);
        if (landed != nullptr) *landed = true;
        return true;
    }

    // A character's attack is its own HitDef meeting its own animation's Clsn1
    // boxes. The HitDef says what the hit does; the animation says which frames
    // can do it, which is why a move has a startup the hitbox cannot skip. And
    // the `hitflag` says which *condition* of victim it may touch at all: a
    // default MAF attack passes through a fighter already on the floor.
    if (!machine_.hasArmedHitDef()) return false;
    if (!machine_.hitMayReach(target.machine_)) return false;

    const Animation* animation = machine_.currentAnimation();
    if (animation == nullptr) return false;
    const int frameIndex = animation->frameAt(machine_.animTime());
    if (frameIndex < 0 || frameIndex >= static_cast<int>(animation->attackBoxes.size())) {
        return false;
    }
    const std::vector<CollisionBox>& boxes =
        animation->attackBoxes[static_cast<size_t>(frameIndex)];
    if (boxes.empty()) return false;  // a startup or recovery frame

    const Rect4 hurt = target.worldHurtBox(floorY);
    for (const CollisionBox& box : boxes) {
        if (!overlap(worldBox(box, floorY), hurt)) continue;
        // The reaction is built now rather than when the HitDef ran, because
        // which hit state fits depends on where the victim is standing.
        const StateMachine::HitInfo info = machine_.hitInfoFor(target.machine_);

        // An armed ReversalDef on the target captures the hit: the counter
        // super's whole tell. The attacker stops dead for the reversal's pause
        // and learns nothing else; the target's `movereversed` tells its own
        // state machine what happened next tick.
        if (target.machine_.tryReversal(info)) {
            machine_.setPause(std::max(1, target.machine_.reversalDef().pause[0]));
            if (landed != nullptr) *landed = false;
            return true;
        }

        // The victim may still refuse it -- an air hit the juggle allowance cannot
        // pay for does not land -- and if it does, nothing happened: no freeze, no
        // sound, no contact, and the attack goes on looking for one.
        if (!target.machine_.applyHit(info)) break;

        if (info.guarded) {
            machine_.markGuarded();
        } else {
            machine_.markHitLanded(&target.machine_);
            if (info.damage > 0) target.everTookDamage_ = true;
        }
        // The attacker's half of the freeze. Applying it here rather than in
        // applyHit() keeps the two sides apart: the victim froze itself, and the
        // attacker freezes just long enough to sell the connect.
        machine_.setPause(info.attackerPause);
        gainPower(info.getPower >= 0 ? info.getPower : kPowerGain);

        // Corner push. A fighter already against the wall cannot be driven any
        // further into it, so the shove goes back up the attack that caused it and
        // the *attacker* is moved instead. Without this the corner is a place where
        // pressure costs nothing and a jab string is unescapable; with it, cornering
        // someone is worth doing and still has a price. MUGEN's default push is the
        // hit's own velocity scaled by 1.3, which is the value used here.
        //
        // The reference's hitapply decides this from four conditions -- the
        // victim's distance to the wall against the hit's `mindist`/`maxdist`
        // slots, whether the push would leave the sentinel `0xfffffbff` (-1025)
        // untouched, and whether the attacker itself is walled in -- and then
        // moves the attacker by the push slot it chose. This is the one-nudge
        // shape of the same rule: a victim within twelve units of the edge pays
        // the attacker's share once, at contact. The cornering price is the
        // gameplay effect that matters; the exact slot arithmetic is not
        // modelled.
        constexpr float kCornerGap = 12.0f;
        constexpr float kCornerPushFactor = 1.3f;
        const float toward = info.pushX >= 0.0f ? 1.0f : -1.0f;
        const float limit = toward > 0.0f ? target.machine_.stageMax() : target.machine_.stageMin();
        const bool cornered = toward > 0.0f ? target.machine_.x() >= limit - kCornerGap
                                            : target.machine_.x() <= limit + kCornerGap;
        if (cornered) machine_.nudge(-toward * std::fabs(info.pushX) * kCornerPushFactor);

        if (landed != nullptr) *landed = true;
        return true;
    }
    return false;
}

bool Fighter::wouldHit(const Fighter& target, float floorY) const {
    if (target.life() <= 0 || character_ == nullptr) return false;
    if (target.machine_.ignoringHits()) return false;
    if (target.machine_.ignoringThrows() && machine_.armedHitIsThrow()) return false;
    if (!machine_.hasArmedHitDef()) return false;
    if (!machine_.hitMayReach(target.machine_)) return false;

    const Animation* animation = machine_.currentAnimation();
    if (animation == nullptr) return false;
    const int frameIndex = animation->frameAt(machine_.animTime());
    if (frameIndex < 0 || frameIndex >= static_cast<int>(animation->attackBoxes.size())) {
        return false;
    }
    const std::vector<CollisionBox>& boxes =
        animation->attackBoxes[static_cast<size_t>(frameIndex)];
    if (boxes.empty()) return false;

    const Rect4 hurt = target.worldHurtBox(floorY);
    for (const CollisionBox& box : boxes) {
        if (overlap(worldBox(box, floorY), hurt)) return true;
    }
    return false;
}

int Fighter::attackPriority() const {
    return machine_.hasArmedHitDef() ? machine_.attackPriority() : 0;
}

void Fighter::updateGuardDistances(Fighter& a, Fighter& b,
                                   const std::vector<Projectile>& projectiles, float floorY) {
    (void)floorY;
    // `inguarddist` is "the other one is swinging from close enough that holding back
    // would put me into a guard state at all". `trigger.html` defines it in one
    // sentence -- "the player is within guarding distance of an opponent's physical or
    // projectile attack. The guarding distance is the value of the guard.dist parameter
    // of the opponent's HitDef" -- and that sentence is the whole of this function.
    //
    // What it replaced measured the attacker's *live attack boxes*, grown by 24 units,
    // against the defender's body. That is a reach where the reference asks for a
    // distance, and the two answer differently at both ends. A slow spear whose box is
    // not out until frame 30 gave the defender no warning at all on frames 1 to 29, so
    // a block started too late to matter; and a beam already in flight -- which
    // `trigger.html` names outright -- gave none ever, because the fighter that fired
    // it is standing in a recovery state with nothing armed.
    //
    // The comparison itself is the reference's, read off the binary and confirmed
    // instruction by instruction. Two things about it are not what the doc's "within
    // guarding distance" might suggest:
    //
    //   * it is **signed and one-sided**, not a distance: the test is
    //     `0 <= (defender.x - attacker.x) * attacker.facing <= guard.dist * scale`.
    //     An opponent standing *behind* the attacker is out of guarding distance
    //     however close it is, because the move is going the other way. The lower bound
    //     is a real bound and not an artefact of an unsigned compare.
    //   * the scale is the **attacker's**, which is what puts a guard distance written
    //     in the character's own coordinates into the world coordinates everything else
    //     is measured in -- the same factor `worldBox` applies to that fighter's boxes.
    //
    // The melee half is gated on the attacker's `movetype` being A, which is the
    // reference's own test and the second half of what `sctrls.html` says about the
    // parameter ("The effect of guard.dist only takes effect when P1 has movetype = A").
    // Note what that gate does *not* ask: there is no requirement that a HitDef be armed.
    // A super's wind-up, a whiffed attack's recovery, a state that re-dresses its guard
    // distance by `AttackDist` and arms nothing -- all of them answer, because the
    // distance lives on the player (`StateMachine::guardDist()`) and not on the attack.
    // That is the whole point of the number: it is not a hitbox and never claimed to be.
    const auto threatens = [&](const Fighter& attacker, const Fighter& defender, int side) {
        const float toward = (defender.x() - attacker.x()) * static_cast<float>(attacker.facing());
        if (attacker.machine().moveType() == MoveType::Attack) {
            const float reach = static_cast<float>(attacker.machine().guardDist()) * attacker.scale();
            if (toward >= 0.0f && toward <= reach) return true;
        }
        // A beam in flight is an attack, and the distance that matters for it is the one
        // the projectile itself is at -- which is why `[Size]` carries a second number
        // for it, 90 against the melee 160. The projectile's own `facing` answers which
        // way it is going, and the reach is scaled by its *owner's* scale, the same way
        // the melee one is: the number was written in the character's coordinates and
        // the comparison is in the world's.
        //
        // The scan has no movetype gate in the reference -- it runs even when the owner
        // is standing in a recovery state, which is the case the whole branch exists
        // for. It does have the reference's "is this element alive" check, which here is
        // `gone` and nothing more: a projectile playing its removal animation is still
        // in the array and still counted, and second-guessing that with this engine's
        // extra notions of inertness would be inventing a rule the reference does not
        // have.
        for (const Projectile& beam : projectiles) {
            if (beam.gone) continue;
            if (beam.ownerSide != side) continue;
            const float reach = StateMachine::guardDistanceFor(beam.hit, beam.owner, true) *
                                attacker.scale();
            const float gap = (defender.x() - beam.x) * static_cast<float>(beam.facing);
            if (gap >= 0.0f && gap <= reach) return true;
        }
        return false;
    };

    b.machine_.setInGuardDist(threatens(a, b, 0));
    a.machine_.setInGuardDist(threatens(b, a, 1));
}

void Fighter::takeHit(int damage, int fromFacing) {
    if (machine_.life() <= 0) return;

    // A hit that did not come from a HitDef -- the engine test's debug keys --
    // is given a middling one, so it lands and reads the same way.
    StateMachine::HitInfo info;
    info.damage = damage;
    info.attackerFacing = fromFacing;
    info.pushX = static_cast<float>(-fromFacing) * 2.4f;
    info.hitTime = kPlaceholderHurtStun;
    info.stateNumber = CommonState::kHitHigh;
    machine_.applyHit(info);
    everTookDamage_ = true;

    if (character_ == nullptr) {
        placeholderPhase_ = machine_.life() <= 0 ? Phase::KO : Phase::Hurt;
        placeholderAttackTicks_ = kPlaceholderHurtStun;
    }
}

void Fighter::gainPower(int amount) { machine_.gainPower(amount); }

void Fighter::separate(Fighter& a, Fighter& b) {
    StateMachine::separate(a.machine_, b.machine_);
}

// --------------------------------------------------------------------- afterimage

void Fighter::recordGhost() {
    const StateMachine::AfterImageParams& params = machine_.afterImage();
    if (params.ticksLeft <= 0) {
        ghosts_.clear();
        return;
    }

    if (character_ == nullptr) return;
    const Animation* animation = machine_.currentAnimation();
    if (animation == nullptr || animation->frames.empty()) return;
    const int frameIndex = animation->frameAt(machine_.animTime());
    if (frameIndex < 0 || frameIndex >= static_cast<int>(animation->frames.size())) return;
    if (animation->frames[static_cast<size_t>(frameIndex)].blank()) return;
    const AnimFrame frame = animation->frameDrawn(machine_.animTime());

    // The ghost is a frozen copy of everything the live draw uses to place the
    // sprite, so it stays where it was when it was recorded.
    Ghost ghost;
    ghost.group = frame.group;
    ghost.image = frame.image;
    ghost.offsetX = frame.offsetX + machine_.offsetX() + machine_.shakeOffsetX();
    ghost.offsetY = frame.offsetY + machine_.offsetY();
    ghost.x = machine_.x();
    ghost.y = machine_.y();
    ghost.facing = machine_.facing() * (frame.flipH ? -1 : 1);
    ghosts_.push_back(ghost);

    const size_t limit = static_cast<size_t>(std::clamp(params.length, 1, 60));
    while (ghosts_.size() > limit) ghosts_.pop_front();
}

// --------------------------------------------------------------------- drawing

void Fighter::draw(Renderer& renderer, float originX, float floorY,
                   const ShadowStyle& shadow, bool drawShadows) const {
    const float screenX = originX + machine_.x();
    const float feetY = floorY - machine_.y();

    if (character_ == nullptr) return;

    const Animation* animation = machine_.currentAnimation();
    if (animation == nullptr || animation->frames.empty()) return;
    // frameDrawn, not the raw index: Interpolate Offset/Scale actions land here.
    const AnimFrame frame = animation->frameDrawn(machine_.animTime());
    if (frame.blank()) return;

    // A ChangeAnim2 pose is drawn from the attacker's file: the animation's
    // group/image numbers are *that* character's, and its sprites are too.
    Character* bankChar = const_cast<Character*>(machine_.animCharacter());
    const int facing = machine_.facing();
    // The hit shake is added to the character's own visual offset rather than
    // applied to its position: MUGEN's shake never moves the fighter, it only moves
    // the sprite, and a fighter that were really displaced by it would drift.
    const float offsetX = machine_.offsetX() + machine_.shakeOffsetX();
    const float offsetY = machine_.offsetY();
    // Which way the art is mirrored, which is the fighter's facing with the
    // frame's own `H` flag folded in. Shared by the reflection and the body so
    // the two can never disagree.
    const int drawFacing = facing * (frame.flipH ? -1 : 1);

    // ------------------------------------------------------------- shadow
    //
    // The current animation frame, drawn as a silhouette on the floor plane.
    // This is the reference's player shadow (SHADOW-RE §3): same sprite, same
    // group/image, same facing and offsets as the body, squashed vertically by
    // the stage's `[Shadow] yscale` and projected onto the ground.
    //
    // The reference composites it with two blits -- a tinted one at alpha A and
    // a black one at 256-A. The renderer's tint multiplies the texel and its
    // `srcAlpha` then scales the result, so a *single* draw with the stage
    // colour as the tint and A as `srcAlpha` is the same arithmetic for the
    // pixels that survive; the difference is only in the 256-A pass, which
    // exists to darken the art the tinted pass brightened. Recorded as an
    // approximation in PARITY-ROADMAP.md.
    //
    // Gates, in the reference's order: no sprite, drawshadows off, this
    // machine's noshadow, not visible, and a zero alpha (intensity 0 or a fade
    // that has reached the top). `addalpha`'s dst alpha is the only Trans that
    // reaches a shadow (SHADOW-RE §5).
    if (drawShadows && !machine_.asserted(StateMachine::kAssertNoShadow) &&
        !machine_.invisible() && shadow.intensity > 0.0f &&
        std::fabs(shadow.yScale) > 0.0001f) {
        float addAlphaDst = -1.0f;
        if (machine_.transIsAddAlphaThisTick() && machine_.transThisTick().has_value()) {
            addAlphaDst = machine_.transThisTick()->dstAlpha * 256.0f;
        }
        const float alpha = shadowAlpha(shadow, -machine_.y(), addAlphaDst);
        if (alpha > 0.0f) {
            drawSpriteAt(renderer, bankChar->sprites(), frame.group, frame.image,
                         screenX + (frame.offsetX + offsetX) * static_cast<float>(facing) *
                                       scale_,
                         floorY + ((frame.offsetY + offsetY) * scale_ - machine_.y()) *
                                      shadow.yScale,
                         facing * (frame.flipH ? -1 : 1), frame.scaleX * scale_,
                         frame.scaleY * scale_ * std::fabs(shadow.yScale), shadow.color,
                         TransParams{Blend::Normal, alpha, 1.0f}, nullptr,
                         shadow.yScale < 0.0f);
        }
    }

    // ------------------------------------------------------------ palette
    //
    // The fighter's palette state, resolved into the renderer's vocabulary once
    // per draw rather than per sprite: the selected LUT (palno, with RemapPal's
    // redirection on top) and the live PalFX grade. The grade's translation from
    // MUGEN's units into the shader's 0..1 space lives on the state machine,
    // because the global BGPalFX/AllPalFX slots need the same translation and
    // two copies of it would drift.
    const PalFxParams fx = StateMachine::palFxParams(machine_.palFx());

    // Builds the renderer's binding for one sprite, or returns nullptr for a
    // true-colour sprite (PNG24/32), which no table can recolour.
    //
    // LUT choice: a sprite whose own palette slot is not 0 keeps it -- the
    // point of a PNG8 or a dedicated palette node is that the character's palno
    // does NOT reach it. Slot 0 is the character palette by both SFF revisions'
    // conventions, and that is the one palno replaces.
    //
    // RemapPal reaches both cases, because it names palettes rather than a
    // selection: the shared slot is drawn as the selected palette identity
    // `(1, palno)`, and a dedicated palette node is drawn as its own
    // `(group, item)`, so either can be the source of a redirection. The
    // destination is looked up in the animation's palette set, which is where
    // the reference requires both palettes to exist.
    auto bindSprite = [&](Character* palChar, SpriteBank& sb, int group, int image,
                          const PalFxParams& grade, PaletteBind& out) -> const PaletteBind* {
        const Texture* tex = sb.texture(group, image);
        if (tex == nullptr || !tex->indexed()) return nullptr;
        const int own = sb.spritePalette(group, image);
        int lutIndex = own;
        if (own > 0) {
            // A dedicated palette node is remappable as the identity it
            // carries; one with no identity (a v1 palette, an .act append) is
            // not addressable and stays.
            const SpriteBank::PaletteKey key = sb.paletteKeyOf(own);
            if (key.group > 0 && key.item > 0) {
                const PaletteRef source{key.group, key.item};
                const PaletteRef dest = machine_.remapPalette(source);
                if (!(dest == source)) {
                    lutIndex = palChar->paletteIndexFor(dest.group, dest.item);
                }
            }
        } else {
            const PaletteRef palette = machine_.effectivePalette();
            lutIndex = palChar->paletteIndexFor(palette.group, palette.item);
        }
        if (lutIndex < 0) lutIndex = own;  // no selection to apply: the sprite's own entry
        if (lutIndex < 0) return nullptr;
        out.lut = sb.lutTexture(lutIndex);
        if (out.lut == nullptr) return nullptr;
        out.fx = grade;
        return &out;
    };

    // ---------------------------------------------------------- reflection
    //
    // `[Reflection] intensity` (SHADOW-RE.md §0 fact 7, §3.2): the same
    // current-animation sprite as the shadow, mirrored about the ground plane
    // instead of squashed onto it. The reference draws it from the same
    // function as the shadow, under the same `drawshadows` / `noshadow` /
    // visibility gates, and it has no art or tint of its own -- the intensity
    // is the alpha. Only that intensity gates it; the `[Shadow] intensity` and
    // `yscale` above are the shadow's business, not the reflection's.
    //
    // The palette resolution and `bindSprite` above do no drawing, so this is
    // still the draw immediately after the shadow and before any ghost/body.
    // The binding is the body's own (palno, RemapPal, PalFX): the shadow's
    // unbound sprite would make a recoloured fighter's reflection disagree
    // with the fighter standing on it.
    //
    // Geometry: `drawSpriteAt` places a sprite by its axis. The body's axis is
    // at `feetY + (frame.offsetY + offsetY) * scale_`; reflecting that point
    // about `floorY` gives `floorY + (floorY - bodyY)` (equivalently
    // `2 * floorY - bodyY`). Passing `flipV` then turns the art over about
    // that axis, and by the same corner arithmetic `drawSpriteAt` uses for
    // `vfacing`, the sprite lands exactly on the mirror of the body's box --
    // no yscale squash, because a mirror is not a squash. The x, facing and
    // scale are the body's own.
    //
    // Rotation is deliberately not applied: the reference's shadow record is a
    // plain projection of the current sprite (§3.2), and a rotated mirror
    // would need a rotated blit about a reflected pivot, which the notes do
    // not describe.
    if (drawShadows && !machine_.asserted(StateMachine::kAssertNoShadow) &&
        !machine_.invisible() && reflectionActive(shadow)) {
        PaletteBind reflectionBinding;
        const PaletteBind* reflectionPal =
            bindSprite(bankChar, bankChar->sprites(), frame.group, frame.image, fx,
                       reflectionBinding);
        const float bodyAxisY = feetY + (frame.offsetY + offsetY) * scale_;
        drawSpriteAt(renderer, bankChar->sprites(), frame.group, frame.image,
                     screenX + (frame.offsetX + offsetX) * static_cast<float>(facing) * scale_,
                     reflectionY(floorY, bodyAxisY), drawFacing, frame.scaleX * scale_,
                     frame.scaleY * scale_, Color{},
                     TransParams{Blend::Normal, reflectionAlpha(shadow), 1.0f}, reflectionPal,
                     /*flipV=*/true);
    }

    // The ghosts first, oldest to newest, so the trail reads as a trail: each
    // one dimmer than the one that follows it, blended the way the controller
    // asked. `palcolor = 0` and `palinvertall` are PalFX's own `color`/`invertall`
    // and ride the palette path for indexed art; the grey multiply tint stays as
    // the fallback for true-colour sprites.
    if (!ghosts_.empty()) {
        const StateMachine::AfterImageParams& trail = machine_.afterImage();
        const int total = static_cast<int>(ghosts_.size());
        int index = 0;
        for (const Ghost& ghost : ghosts_) {
            const float fade =
                0.55f * static_cast<float>(index + 1) / static_cast<float>(total);
            Color tint{1.0f, 1.0f, 1.0f, fade};
            PalFxParams ghostFx = fx;  // the trail plays under the fighter's own grade too
            if (trail.color == 0) {
                ghostFx.color = 0.0f;
            }
            if (trail.invert) ghostFx.invert = !ghostFx.invert;
            PaletteBind binding;
            const PaletteBind* pal = bindSprite(character_, character_->sprites(), ghost.group,
                                                ghost.image, ghostFx, binding);
            if (pal == nullptr && trail.color == 0) {
                // RGBA fallback: the approximation from before the LUT path existed.
                tint.r = tint.g = tint.b = 0.75f;
            }
            drawSpriteAt(renderer, character_->sprites(), ghost.group, ghost.image,
                         originX + ghost.x + ghost.offsetX * static_cast<float>(facing) * scale_,
                         floorY - ghost.y + ghost.offsetY * scale_, ghost.facing,
                         frame.scaleX * scale_, frame.scaleY * scale_, tint, trail.trans, pal);
            ++index;
        }
    }

    // The body's transparency: a `Trans` controller run this tick outranks the
    // animation's own `.air` code, and when there is none the frame's pair is
    // what draws. (The ghosts above are the trail's own business, not this.)
    const TransParams bodyTrans = machine_.transThisTick().value_or(frame.trans());
    // Rotation is the frame's own `.air` angle plus what the state's Angle*
    // controllers left on the machine, around the axis point the offsets hang
    // from. Zero runs the plain path, which is nearly every frame of a fight.
    const float drawAngle = frame.angle + machine_.drawAngle();
    if (std::fabs(drawAngle) > 0.01f) {
        const Sprite* sprite = bankChar->sprites().find(frame.group, frame.image);
        PaletteBind binding;
        const PaletteBind* pal =
            bindSprite(bankChar, bankChar->sprites(), frame.group, frame.image, fx, binding);
        // bindSprite already touched bank.texture(), so this never re-uploads.
        const Texture* texture = bankChar->sprites().texture(frame.group, frame.image);
        if (sprite != nullptr && texture != nullptr) {
            // Pivot: the character's axis -- pos plus the frame's offsets, the
            // point every other part of the draw hangs from.
            const float pivotX = screenX + (frame.offsetX + offsetX) * static_cast<float>(facing) *
                                             scale_;
            const float pivotY = feetY + (frame.offsetY + offsetY) * scale_;
            // The same corner math as drawSpriteAt, rotated about the pivot.
            const float width = static_cast<float>(sprite->width);
            const float height = static_cast<float>(sprite->height);
            float left;
            if (drawFacing < 0) {
                left = pivotX - (width - static_cast<float>(sprite->axisX)) * frame.scaleX * scale_;
            } else {
                left = pivotX - static_cast<float>(sprite->axisX) * frame.scaleX * scale_;
            }
            const float top = pivotY - static_cast<float>(sprite->axisY) * frame.scaleY * scale_;
            renderer.drawSpriteRotated(*texture, pivotX, pivotY, left, top,
                                       width * frame.scaleX * scale_,
                                       height * frame.scaleY * scale_,
                                       Rect{0.0f, 0.0f, width, height}, drawAngle, Color{},
                                       drawFacing < 0, false, bodyTrans, pal);
        }
        return;
    }
    PaletteBind binding;
    const PaletteBind* pal =
        bindSprite(bankChar, bankChar->sprites(), frame.group, frame.image, fx, binding);
    drawSpriteAt(renderer, bankChar->sprites(), frame.group, frame.image,
                 screenX + (frame.offsetX + offsetX) * static_cast<float>(facing) * scale_,
                 feetY + (frame.offsetY + offsetY) * scale_, drawFacing,
                 frame.scaleX * scale_, frame.scaleY * scale_, Color{}, bodyTrans, pal);
}

}  // namespace af

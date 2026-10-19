#include "engine/entities.h"

#include <algorithm>
#include <cmath>

#include "engine/character.h"
#include "engine/motif.h"
#include "engine/shadow.h"

namespace af {
namespace {

// MUGEN's sprite priorities run from -5 to 5 and everything outside that is clamped, so
// that a character writing `sprpriority = 9999` on a super flash does not end up drawn
// over the HUD.

// Telling a projectile to go, and which animation that means.
//
// Every way a projectile can end -- its time running out, its hits being spent by a
// player, it leaving the screen or the stage or its own height band, or another
// projectile cancelling it -- enters one of the three removal states through here. The
// animation differs per state (§5.3): a bound or player attack plays `projremanim`, a
// cancellation `projcancelanim` (falling through to `projremanim` and then
// `projhitanim`), and an exhausted removable projectile its `projhitanim`. A state whose
// resolved animation is `-1` frees the slot on entry with no animation at all.
//
// A removal state may be re-entered only from `Flying`; the one exception is a
// projectile that was already playing its state-2 removal and then spent its last hit,
// which switches to state 4 as the reference's pending-hit application does.
void enterProjectileState(Projectile& projectile, ProjectileState state) {
    if (projectile.state == state) return;
    if (!projectile.isFlying() &&
        !(projectile.state == ProjectileState::Removal &&
          state == ProjectileState::Hit)) {
        return;
    }
    const ProjectileRemovalAnim selection = resolveProjectileRemovalAnim(
        state, projectile.hitAnim, projectile.hitAnimFromFightFx, projectile.removeAnim,
        projectile.removeAnimFromFightFx, projectile.cancelAnim,
        projectile.cancelAnimFromFightFx);
    projectile.state = state;
    projectile.stateAge = 0;
    if (selection.anim < 0) {
        projectile.gone = true;
        return;
    }
    projectile.anim = selection.anim;
    projectile.animFromFightFx = selection.fromFightFx;
    // The clock restarts so the removal animation plays from its first frame. A
    // projectile that had been flying for 90 ticks would otherwise be 90 ticks into an
    // animation it never started, and most removal animations are short enough that
    // nothing would be seen at all.
    projectile.clock = 0;
}

// A player contact recorded by the fight screen's hit pass is applied here, at the start
// of the next entity update (§3.2), even if a pause would freeze the movement below.
void settleProjectilePendingHit(Projectile& projectile) {
    if (!projectile.pendingHit) return;
    const ProjectileHitApplication applied =
        applyProjectilePendingHit(projectile.hitsLeft, projectile.removeOnHit);
    projectile.hitsLeft = applied.hitsLeft;
    projectile.missTicks = projectile.pendingCooldown;
    projectile.pause = projectile.pendingPause;
    projectile.pendingHit = false;
    if (applied.enterHitState) enterProjectileState(projectile, ProjectileState::Hit);
}

// How long a removal animation is on screen: one pass through it, no matter what its
// loop settings say.
//
// This is deliberately *not* the `finished` rule the rest of this file asks of an
// animation, because under that rule neither removal animation a character in the
// corpus actually names would ever end. `projremanim` is written eleven times across
// three characters here and every one of those animations loops: `dbz_piccolo`'s action
// 3006 is eight one-tick frames with no `LoopStart`, and `dbz_gohan`'s 1016 is a single
// two-tick frame -- and an animation with no `LoopStart` loops from its first element,
// which is why a character's idle animation loops without saying so. A projectile whose
// death animation never finished would sit there for the rest of the round, drawing and
// still counting towards `numproj`, which is a worse reading of `projremanim` than
// "played once". A removal animation with no frames, or one this character does not have,
// is nought ticks long and the projectile goes at once.
// MUGEN's 320x240 reference screen, which is the coordinate space the engine's
// virtual drawing uses too: the edge test is half of it either side of the
// camera (§6.2).
constexpr float kHalfReferenceScreenWidth = 160.0f;

long long removalTicks(const AnimationSet* animations, int anim) {
    if (animations == nullptr || anim < 0) return 0;
    const Animation* animation = animations->find(anim);
    if (animation == nullptr) return 0;
    long long total = 0;
    for (const AnimFrame& frame : animation->frames) total += std::max(0, frame.duration);
    return total;
}

// One removal-state tick (§5.3).
//
// This is a function rather than the tail of the update's `else` branch so that
// a projectile whose bound, edge or age expires in the *flying* branch can fall
// into the removal handler on the same tick -- which is what the reference does:
// state 2 and age 0 are set at `0x47302D`/`0x473034` and control falls straight
// through to `0x473056`, which advances the position by the current velocity
// once more. The first removal tick therefore uses the (just-applied) flight
// velocity, and the next one switches to `remvelocity`.
//
// Entry from a collision pass -- a player attack or a projectile duel -- is a
// different case: that state write happens before this layer's update, so the
// update reaches this helper through the ordinary `else` branch and advances
// exactly once, as the reference's collision-driver-then-update ordering does.
void advanceProjectileRemoval(Projectile& projectile, const AnimationSet* fxAnimations) {
    ++projectile.clock;
    if (projectile.stateAge == 0) {
        projectile.x += projectile.velX;
        projectile.y -= projectile.velY;
        projectile.velX = projectile.remVelX;
        projectile.velY = projectile.remVelY;
    } else {
        projectile.x += projectile.remVelX;
        projectile.y -= projectile.remVelY;
    }
    ++projectile.stateAge;
    const AnimationSet* animations =
        projectile.animFromFightFx
            ? fxAnimations
            : (projectile.owner != nullptr ? &projectile.owner->animations() : nullptr);
    if (projectile.clock >= removalTicks(animations, projectile.anim)) {
        projectile.gone = true;
    }
}

// One object's ground shadow, drawn immediately before the object itself.
//
// The reference draws *all* shadows in three separate passes, before any of the
// bodies; this engine has no such pass and draws an object's shadow with the
// object's own layer draw. The pixels on the floor are the same; what differs
// is which other sprite can cover the shadow, and that is recorded as an
// approximation in PARITY-ROADMAP.md rather than hidden.
//
// Geometry is the player path's: the sprite's own anchor is projected onto the
// floor plane by the stage's yscale, so a shadow sits under the sprite wherever
// the sprite's axis is, and it is squashed by the same factor. `depth` is in
// MUGEN's stage-y direction, where larger is lower; this engine stores height
// above the floor, so it is the negated y.
void drawObjectShadow(Renderer& renderer, SpriteBank& bank, const EntityFrame& frame,
                      const ShadowStyle& style, const ShadowRgb& rgb,
                      bool anyComponentSelectsStage, float x, float y, int facing,
                      float scaleX, float scaleY, bool flipV, float scrollX, float floorY) {
    if (!objectShadowEnabled(rgb)) return;
    const float yScale = style.yScale;
    if (std::fabs(yScale) < 0.0001f) return;
    const float alpha = shadowAlpha(style, -y);
    if (alpha <= 0.0f) return;

    const Color color = resolveObjectShadowColor(rgb, style, anyComponentSelectsStage);
    const float shadowX = x - scrollX + frame.offsetX * static_cast<float>(facing) * scaleX;
    // The body's anchor is `floorY - y + frame.offsetY * scaleY`; the shadow is
    // the ground projection of that point, `floorY + (anchor - floorY) * yScale`.
    const float shadowY = floorY + (frame.offsetY * scaleY - y) * yScale;
    drawSpriteAt(renderer, bank, frame.group, frame.image, shadowX, shadowY, facing, scaleX,
                 scaleY * std::fabs(yScale), color,
                 TransParams{Blend::Normal, alpha, 1.0f}, nullptr,
                 flipV != (yScale < 0.0f));
}
}  // namespace

PostType parsePostType(std::string_view text, PostType fallback) {
    if (text == "p1") return PostType::P1;
    if (text == "p2") return PostType::P2;
    if (text == "front") return PostType::Front;
    if (text == "back") return PostType::Back;
    if (text == "left") return PostType::Left;
    if (text == "right") return PostType::Right;
    return fallback;
}

EntityPaletteChoice chooseEntityPalette(bool ownpal, int spritePalette, int ownerPalette) {
    // A sprite that carries its own non-zero palette is outside palno's reach by
    // the SFF's own convention, exactly as it is in `Fighter::draw`. That answer
    // is the same whether or not the entity asked for `ownpal`, so it is decided
    // first rather than being duplicated in both branches below.
    if (spritePalette > 0) return EntityPaletteChoice{true, spritePalette};

    if (ownpal) {
        // Its own palette: the table entry the sprite decoded against, including
        // entry 0 -- the character's main palette -- which the owner's selection
        // must not replace. A sprite with no palette of its own at all has only
        // the owner's table to fall back on.
        if (spritePalette >= 0) return EntityPaletteChoice{true, spritePalette};
        return EntityPaletteChoice{false, ownerPalette};
    }

    // Inherited: the owner's current selection (palno, with RemapPal on top),
    // which is what makes a PalFX or a costume change reach the entity too. When
    // the owner has no table, the sprite's decoded palette is the last answer.
    if (ownerPalette >= 0) return EntityPaletteChoice{false, ownerPalette};
    return EntityPaletteChoice{true, spritePalette};
}

EntityFrame entityFrame(const AnimationSet* animations, int anim, long long clock) {
    EntityFrame out;
    if (animations == nullptr || anim < 0) {
        out.finished = true;
        return out;
    }
    const Animation* animation = animations->find(anim);
    if (animation == nullptr || animation->frames.empty()) {
        // An animation this character does not have. MUGEN draws nothing and the entity
        // lives out its remove time, which is the forgiving reading and the one that
        // keeps a misspelled anim from leaving a spark on screen for ever.
        out.finished = true;
        return out;
    }

    const int index = animation->frameAt(clock);
    if (index >= 0 && index < static_cast<int>(animation->frames.size())) {
        const AnimFrame& frame = animation->frames[static_cast<size_t>(index)];
        out.group = frame.group;
        out.image = frame.image;
        out.offsetX = frame.offsetX;
        out.offsetY = frame.offsetY;
        out.trans = frame.trans();
    }

    // "Finished" is the same question the state machine asks of a character's animation
    // and it has the same two answers: an animation that loops never finishes, and one
    // whose last element holds for ever never finishes either -- both of those are
    // removed by their `removetime` instead.
    long long total = 0;
    bool holds = false;
    for (const AnimFrame& frame : animation->frames) {
        if (frame.duration < 0) holds = true;
        total += std::max(0, frame.duration);
    }
    const bool loops = animation->loopStart < animation->frames.size();
    out.finished = !holds && !loops && clock >= total;
    return out;
}

// Whether an animation being run at `clock` has reached its **looptime** -- MUGEN's
// `AnimTime = 0`, and the thing `removetime = -2` is defined against ("if rem_time
// is -2, the explod will be removed when its animtime reaches 0").
//
// This is deliberately *not* `EntityFrame::finished`, which asks whether the
// animation has run out for good. An action that loops never runs out for good and
// still reaches its looptime once per pass, and almost every action in the format
// loops, because all that looping takes is the absence of a `LoopStart` line. Read
// as "would it finish for good", a -2 explod with a looping animation is never
// removed at all -- which is what nearly every spark, dust puff and hit effect in
// every character file would have been.
//
// The length of one pass is the sum of *all* the elements, prefix included, which
// is the arithmetic air.html works through in its own example ("the looptime would
// be: 7 + 7 + 7 + 50 + 7 + 7 + 7 + 60 = 152 ticks"). A single element with a
// duration of -1 makes the looptime infinite -- air.html's "no loop" -- so the
// question has no true answer and this says so.
//
// An animation the character does not have counts as reached, which is the same
// forgiving reading `entityFrame` takes and keeps a misspelled `anim` from leaving
// a spark on screen for the rest of the round.
bool entityLoopTimeReached(const AnimationSet* animations, int anim, long long clock) {
    if (animations == nullptr || anim < 0) return true;
    const Animation* animation = animations->find(anim);
    if (animation == nullptr || animation->frames.empty()) return true;

    long long total = 0;
    for (const AnimFrame& frame : animation->frames) {
        if (frame.duration < 0) return false;  // holds for ever: no looptime to reach
        total += std::max(0, frame.duration);
    }
    return clock >= total;
}

// ------------------------------------------------------------------- the layer

void EntityLayer::spawnExplod(const Explod& explod) {
    // MUGEN caps this, because a character whose state loops and spawns an aura every
    // tick would otherwise take the machine down. The cap is generous -- six hundred is
    // far past anything a real move makes -- and it drops the *oldest*, since the newest
    // is what the move just asked for.
    constexpr size_t kMaxExplods = 600;
    if (explods_.size() >= kMaxExplods) explods_.erase(explods_.begin());
    explods_.push_back(explod);
}

// `ModifyExplod`, and the whole of it is that it is not a creation: a field nobody
// named is left alone, including the ones no controller can name -- the clock, and
// which frame of the animation is showing. Only the fields the file actually wrote are
// copied across, which is why every one of them is an `optional` on the way in.
void EntityLayer::modifyExplod(const ExplodModify& modify) {
    for (Explod& explod : explods_) {
        if (explod.owner != modify.owner || explod.gone) continue;
        if (explod.id != modify.id) continue;
        if (modify.anim.has_value()) explod.anim = *modify.anim;
        if (modify.animFromFightFx.has_value()) explod.fromFightFx = *modify.animFromFightFx;
        if (modify.movesPosition) {
            explod.x = modify.x;
            explod.y = modify.y;
            // Re-bound about its new place. The offset is measured off the owner's
            // current position rather than kept from before, so an explod written
            // `pos` and `postype` together really goes where the file said it should
            // instead of being dragged back by the bind on the very next tick.
            explod.bindOffsetX = explod.x - modify.ownerX;
            explod.bindOffsetY = explod.y - modify.ownerY;
        }
        if (modify.velX.has_value()) explod.velX = *modify.velX;
        if (modify.velY.has_value()) explod.velY = *modify.velY;
        if (modify.accelX.has_value()) explod.accelX = *modify.accelX;
        if (modify.accelY.has_value()) explod.accelY = *modify.accelY;
        if (modify.facing.has_value()) explod.facing = *modify.facing;
        if (modify.flipV.has_value()) explod.flipV = *modify.flipV;
        if (modify.scaleX.has_value()) explod.scaleX = *modify.scaleX;
        if (modify.scaleY.has_value()) explod.scaleY = *modify.scaleY;
        if (modify.spritePriority.has_value()) {
            explod.spritePriority = *modify.spritePriority;
        }
        if (modify.onTop.has_value()) explod.onTop = *modify.onTop;
        if (modify.bindTime.has_value()) explod.bindTime = *modify.bindTime;
        if (modify.removeTime.has_value()) explod.removeTime = *modify.removeTime;
        if (modify.setTrans) explod.trans = modify.trans;
        if (modify.ownpal.has_value()) explod.ownpal = *modify.ownpal;
        if (modify.shadow.has_value()) explod.shadow = *modify.shadow;
        if (modify.ignoreHitPause.has_value()) explod.ignoreHitPause = *modify.ignoreHitPause;
        if (modify.pauseMoveTime.has_value()) explod.pauseMoveTime = *modify.pauseMoveTime;
        if (modify.superMoveTime.has_value()) explod.superMoveTime = *modify.superMoveTime;
        if (modify.behindOwner.has_value()) explod.behindOwner = *modify.behindOwner;
        if (modify.removeOnGetHit.has_value()) explod.removeOnGetHit = *modify.removeOnGetHit;
    }
}

// `RemoveExplod`. The reference's `id` "is the ID number of the explods to remove. If
// omitted, removes all explods owned by the player" -- so the two cases are a named id
// and everything, and there is no id-0 reading of the second.
void EntityLayer::removeExplod(const Character* owner, int id, bool all) {
    for (Explod& explod : explods_) {
        if (explod.owner != owner) continue;
        if (!all && explod.id != id) continue;
        // Gone rather than erased: the list is walked by the draw and by `countExplods`,
        // and both skip a `gone` explod already, so the sweep at the end of `update` is
        // the one place removals happen. A controller runs between the fighters' step
        // and that sweep, so an explod removed here is off the screen on this very tick.
        explod.gone = true;
    }
}

void EntityLayer::spawnProjectile(const Projectile& projectile) {
    constexpr size_t kMaxProjectiles = 64;
    if (projectiles_.size() >= kMaxProjectiles) projectiles_.erase(projectiles_.begin());
    projectiles_.push_back(projectile);
}

void EntityLayer::shake(int time, float frequency, float amplitude) {
    // A longer or stronger shake wins, so two at once do not cancel out.
    if (amplitude >= shakeAmplitude_ || time > shakeTicks_) {
        shakeTicks_ = std::max(shakeTicks_, time);
        shakeAmplitude_ = std::max(shakeAmplitude_, amplitude);
        if (frequency > 0.0f) shakeFrequency_ = frequency;
    }
}

int EntityLayer::countExplods(const Character* owner, const StateMachine* ownerMachine,
                              int id) const {
    int count = 0;
    for (const Explod& explod : explods_) {
        // The state owner is the reference's counting key: a fighter and each of
        // its helpers keep separate slot arrays, so a helper's aura must not make
        // its owner's `numexplod` read high (and a mirror-match opponent with the
        // same `Character*` must not either). A null machine falls back to the
        // character-wide reading for engine-spawned effects and host callers.
        if (ownerMachine != nullptr ? explod.ownerMachine != ownerMachine : explod.owner != owner) {
            continue;
        }
        // A `RemoveExplod` earlier in this same tick has already taken it off the
        // screen; `numexplod` has to agree, or a state that clears its auras and then
        // asks whether they are gone would go on believing they are there for one more
        // tick -- and one tick of a `trigger1 = numexplod(...) = 0` is a whole state
        // transition.
        if (explod.gone) continue;
        // An id of -1 means "any", which is how `numexplod` is written when a character
        // only wants to know whether it has any sparks out.
        if (id >= 0 && explod.id != id) continue;
        ++count;
    }
    return count;
}

int EntityLayer::countProjectiles(const Character* owner, const StateMachine* ownerMachine,
                                  int id) const {
    int count = 0;
    for (const Projectile& projectile : projectiles_) {
        // Per state owner, as in the reference's sparse slot arrays
        // (`PROJECTILE-PARAMS-RE.md` §1): a helper's projectile is not the
        // fighter's, even though both share the same `Character*`.
        if (ownerMachine != nullptr ? projectile.ownerMachine != ownerMachine
                                    : projectile.owner != owner) {
            continue;
        }
        // `NumProj` (`0x4733E0`) counts the reference's state 1 only: a
        // projectile in any removal state is no longer "currently active"
        // (`PROJECTILE-PARAMS-RE.md` §4.9). Counting it here kept a `numproj`
        // gate shut through the death animation, which is the window in which
        // characters spawn their next one.
        if (!projectile.isFlying()) continue;
        if (id >= 0 && projectile.id != id) continue;
        ++count;
    }
    return count;
}

void EntityLayer::cancelProjectile(Projectile& projectile) {
    enterProjectileState(projectile, ProjectileState::Cancelled);
}

void EntityLayer::removeProjectileByAttack(Projectile& projectile) {
    enterProjectileState(projectile, ProjectileState::Removal);
}

void EntityLayer::noteProjectileContact(int ownerSide, int projectileId, int code) {
    if (ownerSide < 0 || ownerSide > 1) return;
    recordProjectileContact(projectileContacts_[ownerSide], code, projectileId);
}

ProjectileContactRecord EntityLayer::projectileContact(int ownerSide) const {
    if (ownerSide < 0 || ownerSide > 1) return ProjectileContactRecord{};
    return projectileContacts_[ownerSide];
}

StateMachine* EntityLayer::spawnHelper(const HelperSpec& spec) {
    if (helperHost_ == nullptr) return nullptr;
    return helperHost_->spawnHelper(spec);
}

StateMachine* EntityLayer::playerById(int id) {
    if (helperHost_ == nullptr) return nullptr;
    return helperHost_->playerById(id);
}

// ------------------------------------------------------------------- pauses

void EntityLayer::beginPause(int time, const StateMachine* actor, int moveTime) {
    PauseInfo pause;
    pause.ticks = pause.total = std::max(1, time);
    pause.actor = actor;
    pause.actorMove = std::max(0, moveTime);
    pause.isSuper = false;
    pause.darken = false;
    pause.p2DefMul = 1.0f;
    pause_ = pause;
}

void EntityLayer::beginSuperPause(int time, const StateMachine* actor, int moveTime,
                                  bool darken, float p2DefMul) {
    PauseInfo pause;
    pause.ticks = pause.total = std::max(1, time);
    pause.actor = actor;
    pause.actorMove = std::max(0, moveTime);
    pause.isSuper = true;
    pause.darken = darken;
    pause.p2DefMul = p2DefMul;
    pause_ = pause;
}

void EntityLayer::clear() {
    explods_.clear();
    projectiles_.clear();
    pause_ = PauseInfo{};
    envColor_ = EnvColorState{};
    bgPalFx_ = StateMachine::PalFxState{};
    allPalFx_ = StateMachine::PalFxState{};
    shakeTicks_ = 0;
    shakeAmplitude_ = 0.0f;
    shakeX_ = 0.0f;
    shakeY_ = 0.0f;
    // Per round, not per match. The reference's reset point is not documented;
    // a round boundary is the narrower choice, and it keeps a hit from the last
    // round from answering `ProjHitTime` in the next one.
    projectileContacts_[0] = ProjectileContactRecord{};
    projectileContacts_[1] = ProjectileContactRecord{};
}

void EntityLayer::update(const StateMachine* p1, const StateMachine* p2) {
    const StateMachine* owners[2] = {p1, p2};

    // The freeze counts down here, once a tick, from the fight's own clock. The
    // actor's move allowance drains with it: movetime is "how many of the pause's
    // first ticks the pauser may still act", not a separate timer.
    const bool frozenNow = pause_.ticks > 0;
    if (frozenNow) {
        --pause_.ticks;
        if (pause_.actorMove > 0) --pause_.actorMove;
    }
    if (envColor_.ticks > 0 && !frozenNow) --envColor_.ticks;

    // Projectile-contact clocks are frames since the contact, counted on the
    // fight's ticks. A contact is recorded at 0 during the collision passes
    // earlier in this tick, so the next tick's triggers read 1 ("just made
    // contact"). They advance through a hitpause too: the reference's
    // player-side reset is not proven to be hitpause-gated, and the triggers
    // are commonly used to time a follow-up after an impact.
    advanceProjectileContact(projectileContacts_[0]);
    advanceProjectileContact(projectileContacts_[1]);

    // The global palette grades age like the colour wash and hold under a freeze for
    // the same reason: a super flash that let its own grade tick down would fade
    // during the very freeze it exists to fill. The effect's `elapsed` counter runs
    // with its `time`, so a frozen slot freezes its sine shimmer too. A time of -1 is
    // permanent: it never reaches zero, so the slot is never reset here.
    if (!frozenNow) {
        if (bgPalFx_.active) {
            ++bgPalFx_.elapsed;
            if (bgPalFx_.time > 0 && --bgPalFx_.time == 0) bgPalFx_ = StateMachine::PalFxState{};
        }
        if (allPalFx_.active) {
            ++allPalFx_.elapsed;
            if (allPalFx_.time > 0 && --allPalFx_.time == 0) {
                allPalFx_ = StateMachine::PalFxState{};
            }
        }
    }

    for (Explod& explod : explods_) {
        // Two freezes, two rules -- the fields on `Explod` carry the reasoning.
        // A hitpause holds the explod unless it opted out, and it is the owner's
        // hitpause that counts ("not be updated when the player is in hitpause").
        // The owner here is the *root* of its side, so an explod a helper made
        // reads its root's hitpause rather than the helper's; the layer is handed
        // the two roots and nothing else, and a helper's hitpause is the rare
        // case rather than the rule.
        const StateMachine* ownerMachine =
            owners[explod.ownerSide >= 0 && explod.ownerSide < 2 ? explod.ownerSide : 0];
        // `removeongethit`, and it is checked *before* both freezes rather than
        // among the things the freezes gate. The moment this asks about is the
        // moment a hit lands on the owner, and that is the same moment the
        // owner's own hitpause begins -- so a check placed below the gate would
        // be asking a question it can never see the answer to. An explod written
        // `ignorehitpause = 0` (the non-default) would then outlive the move it
        // was the glow for, which is the one thing this parameter exists to
        // prevent. `tookDamage()` is true for exactly the tick a hit was applied
        // to that machine, and the fight loop steps the fighters before this
        // layer, so the tick it names is this one.
        if (explod.removeOnGetHit && ownerMachine != nullptr && ownerMachine->tookDamage()) {
            explod.gone = true;
            continue;
        }
        if (!explod.ignoreHitPause && ownerMachine != nullptr && ownerMachine->inHitPause()) {
            continue;
        }
        // A running Pause or SuperPause holds it too, unless the explod named a
        // number of the pause's first ticks to keep moving through. `pause_.total`
        // less what is left counts from one on that first tick, which is the same
        // count `movetime` uses.
        const int pauseTick = pause_.total - pause_.ticks;
        const int moveWindow = pause_.isSuper ? explod.superMoveTime : explod.pauseMoveTime;
        if (frozenNow && pauseTick > moveWindow) continue;
        ++explod.clock;

        // A bound explod is carried by its owner: that is what makes a charge aura stay
        // on the character while it moves, and what `bindtime = -1` keeps for as long as
        // the explod lives.
        if (explod.bindTime != 0) {
            const StateMachine* machine =
                owners[explod.ownerSide >= 0 && explod.ownerSide < 2 ? explod.ownerSide : 0];
            if (machine != nullptr) {
                explod.x = machine->x() + explod.bindOffsetX;
                explod.y = machine->y() + explod.bindOffsetY;
            }
            if (explod.bindTime > 0) --explod.bindTime;
        } else {
            explod.velX += explod.accelX;
            explod.velY += explod.accelY;
            explod.x += explod.velX;
            explod.y += explod.velY;
        }

        if (explod.removeTime > 0) {
            if (--explod.removeTime == 0) explod.gone = true;
        } else if (explod.removeTime < -1) {
            // The default, and any negative below -1 the reference leaves
            // unspecified: it goes when its animation has reached its looptime,
            // which is one pass of the animation whether it loops or not. See
            // `entityLoopTimeReached` for why that is not `finished`.
            const AnimationSet* animations =
                explod.fromFightFx ? fxAnimations_
                                   : (explod.owner != nullptr ? &explod.owner->animations() : nullptr);
            if (entityLoopTimeReached(animations, explod.anim, explod.clock)) explod.gone = true;
        }
        // `removeTime == -1` falls through both branches on purpose: it is
        // "displayed indefinitely", so nothing here removes it. The only thing that
        // takes it off the screen is the round starting over, which clears the
        // layer outright.
    }

    for (Projectile& projectile : projectiles_) {
        // A contact the fight screen recorded earlier this tick becomes real here,
        // before any freeze test: a scheduled hit is never dropped (§3.2).
        settleProjectilePendingHit(projectile);

        // Beams freeze in a super flash like everything else; they have no
        // ignorehitpause of their own.
        if (frozenNow) continue;

        // `projremovetime` is the projectile's age limit. Only a positive value
        // expires: -1 is the documented "never", and an expired age enters state 2
        // like a bound does (§5.2). The flight animation ending is deliberately not a
        // removal condition -- a projectile with `projremovetime = -1` stays on its
        // last frame until something else removes it (§5.2, Q5).
        if (projectile.isFlying()) {
            if (projectile.pause > 0) {
                --projectile.pause;
                continue;
            }
            ++projectile.clock;

            projectile.velX = projectile.velX * projectile.velMulX + projectile.accelX;
            projectile.velY = projectile.velY * projectile.velMulY + projectile.accelY;
            projectile.x += projectile.velX;
            // The velocity a character writes is in its own frame, where +y is down, and
            // a projectile's height is kept the other way up -- the same crossing every
            // other y in the engine makes.
            projectile.y -= projectile.velY;

            // Off the stage, off the screen edge, or out of its vertical range. The
            // three axis bounds are velocity-aware: a projectile already outside one
            // survives while it is travelling back in (§6.2). The edge test is
            // position-only. The stage range comes from the fight screen because this
            // layer has no stage of its own.
            const bool outOfBounds =
                projectilePastAxisBound(projectile.x, projectile.velX,
                                        projectileStageLeft_ - projectile.stageBound,
                                        projectileStageRight_ + projectile.stageBound) ||
                // `y -= velY`, so this is the height's actual step.
                projectilePastAxisBound(projectile.y, -projectile.velY, projectile.lowBound,
                                        projectile.highBound) ||
                projectilePastScreenEdge(projectile.x, cameraMidX_, kHalfReferenceScreenWidth,
                                         projectile.edgeBound);
            if (outOfBounds) {
                enterProjectileState(projectile, ProjectileState::Removal);
                // §5.3: the entry tick falls through into the removal handler,
                // so it takes one more flight-velocity step and counts as
                // removal age 0. `gone` means the resolved animation was -1 and
                // the slot has already been freed.
                if (!projectile.gone) advanceProjectileRemoval(projectile, fxAnimations_);
                continue;
            }

            if (projectile.missTicks > 0) --projectile.missTicks;

            if (projectile.removeTime > 0) {
                if (--projectile.removeTime == 0) {
                    enterProjectileState(projectile, ProjectileState::Removal);
                    if (!projectile.gone) advanceProjectileRemoval(projectile, fxAnimations_);
                    continue;
                }
            }
        } else {
            // Removal states play one animation once and then free the slot (§5.3).
            // Their handler has no pause test -- `+0x28` belongs to the state-1 path --
            // so a projectile that was hit on its way out keeps animating. The first
            // tick still travels with the velocity it entered with; from then on
            // `remvelocity` decides, and neither velmul nor accel applies.
            advanceProjectileRemoval(projectile, fxAnimations_);
        }
    }

    explods_.erase(std::remove_if(explods_.begin(), explods_.end(),
                                  [](const Explod& e) { return e.gone; }),
                   explods_.end());
    // Sprite priority decides which of two explods covers the other, so the list is kept
    // in that order and the drawing is a straight walk. Sorted here rather than in the
    // draw because this is where the list is already being changed.
    std::stable_sort(explods_.begin(), explods_.end(), [](const Explod& a, const Explod& b) {
        return a.spritePriority < b.spritePriority;
    });
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                                      [](const Projectile& p) { return p.gone; }),
                       projectiles_.end());

    // The shake, which is a drawn offset rather than a moved camera: the fighters stay
    // where they are and the frame moves under them, which is what MUGEN's EnvShake is.
    if (shakeTicks_ > 0) {
        --shakeTicks_;
        ++shakeClock_;
        const float phase = static_cast<float>(shakeClock_) * shakeFrequency_ / 60.0f *
                            6.2831853f;
        shakeX_ = std::sin(phase) * shakeAmplitude_;
        shakeY_ = std::cos(phase * 0.7f) * shakeAmplitude_ * 0.5f;
        if (shakeTicks_ == 0) {
            shakeX_ = 0.0f;
            shakeY_ = 0.0f;
            shakeAmplitude_ = 0.0f;
        }
    }
}

namespace {
// The renderer's palette binding for one entity sprite, or null when the draw
// has no palette opinion.
//
// The two facts this needs are split across two banks: the sprite's own palette
// node lives in the bank the art came from, while an inherited selection lives in
// the owner character's bank. `chooseEntityPalette` decides which one wins; this
// only turns that decision into a LUT texture and fills the binding.
const PaletteBind* entityPaletteBind(SpriteBank& spriteBank, int group, int image, bool ownpal,
                                     const Character* owner,
                                     const StateMachine* ownerMachine, PaletteBind& out) {
    // Needs the same texture fetch the draw itself will make; false for RGBA
    // art, which no table can recolour.
    const Texture* texture = spriteBank.texture(group, image);
    if (texture == nullptr || !texture->indexed()) return nullptr;

    const int spritePalette = spriteBank.spritePalette(group, image);
    // `ownpal = 0` inherits the owner's *current* selection, so palno and
    // RemapPal are read here at draw time rather than copied at spawn. A
    // hand-built entity with no machine falls back to the character's default.
    int ownerPalette = -1;
    if (owner != nullptr) {
        const PaletteRef palette = ownerMachine != nullptr
                                       ? ownerMachine->effectivePalette()
                                       : PaletteRef{1, owner->defaultPalNo()};
        ownerPalette = owner->paletteIndexFor(palette.group, palette.item);
    }

    // A RemapPal can also name the sprite's *own* palette node as its source.
    // That is a statement about the palette the art uses, not about `ownpal`,
    // so it redirects either way -- and the destination is resolved in the
    // owner's palette set, which is where the reference requires it to exist.
    // Only art out of the player's own sprite file is touched: a `fightfx`
    // spark's palette nodes are the screenpack's, and a character's RemapPal
    // has no business recolouring them.
    EntityPaletteChoice choice;
    bool remapped = false;
    const bool ownerArt =
        owner != nullptr && &const_cast<Character*>(owner)->sprites() == &spriteBank;
    if (ownerArt && ownerMachine != nullptr && spritePalette > 0) {
        const SpriteBank::PaletteKey key = spriteBank.paletteKeyOf(spritePalette);
        if (key.group > 0 && key.item > 0) {
            const PaletteRef source{key.group, key.item};
            const PaletteRef dest = ownerMachine->remapPalette(source);
            if (!(dest == source)) {
                const int index = owner->paletteIndexFor(dest.group, dest.item);
                if (index >= 0) {
                    choice = EntityPaletteChoice{false, index};
                    remapped = true;
                }
            }
        }
    }
    if (!remapped) choice = chooseEntityPalette(ownpal, spritePalette, ownerPalette);
    if (choice.index < 0) return nullptr;

    SpriteBank* tableBank = &spriteBank;
    if (!choice.fromSpriteBank) {
        if (owner == nullptr) return nullptr;
        tableBank = &const_cast<Character*>(owner)->sprites();
    }
    out.lut = tableBank->lutTexture(choice.index);
    if (out.lut == nullptr) return nullptr;
    return &out;
}
}  // namespace

void EntityLayer::drawExplod(Renderer& renderer, const Explod& explod, float scrollX,
                             float floorY) const {
    // Which bank the art comes from is the whole of the fightfx distinction: a spark is
    // drawn from the screenpack's sprite file and an aura from the character's.
    SpriteBank* bank = nullptr;
    const AnimationSet* animations = nullptr;
    if (explod.fromFightFx) {
        bank = fxSprites_;
        animations = fxAnimations_;
    } else if (explod.owner != nullptr) {
        bank = &const_cast<Character*>(explod.owner)->sprites();
        animations = &explod.owner->animations();
    }
    if (bank == nullptr || animations == nullptr) return;

    const EntityFrame frame = entityFrame(animations, explod.anim, explod.clock);
    if (frame.group < 0) return;
    // The explod's own floor shadow, if it asked for one and the two global
    // gates are open. The owning player's `noshadow` deliberately does not
    // reach this (SHADOW-RE §2.3).
    if (drawShadows_) {
        drawObjectShadow(renderer, *bank, frame, shadow_, explod.shadow,
                         /*anyComponentSelectsStage=*/false, explod.x, explod.y,
                         explod.facing, explod.scaleX, explod.scaleY, explod.flipV, scrollX,
                         floorY);
    }
    const float screenX = explod.x - scrollX;
    const float screenY = floorY - explod.y;
    // Its own `trans` if it named one, the animation's colour-add code otherwise.
    const TransParams trans = explod.trans.value_or(frame.trans);
    // `ownpal`: the sprite's own palette node when it asked for one, the owner's
    // current LUT when it did not. See `chooseEntityPalette`.
    PaletteBind paletteBinding;
    const PaletteBind* palette = entityPaletteBind(
        *bank, frame.group, frame.image, explod.ownpal, explod.owner, explod.ownerMachine,
        paletteBinding);
    drawSpriteAt(renderer, *bank, frame.group, frame.image,
                 screenX + frame.offsetX * static_cast<float>(explod.facing) * explod.scaleX,
                 screenY + frame.offsetY * explod.scaleY, explod.facing, explod.scaleX,
                 explod.scaleY, Color{1, 1, 1, 1}, trans, palette, explod.flipV);
}

void EntityLayer::drawBehind(Renderer& renderer, float scrollX, float floorY) const {
    for (const Explod& explod : explods_) {
        // `ontop` outranks the priority comparison outright, so an explod that
        // asked for the top layer is not drawn here even when its sprite
        // priority is below its owner's -- which is the case the reference
        // calls out when it says the parameter "has precedence over
        // sprpriority".
        if (explod.onTop) continue;
        // Behind means behind the fighters, not behind the stage: a charge aura is drawn
        // under its owner so the character reads through it.
        if (!explod.behindOwner) continue;
        drawExplod(renderer, explod, scrollX, floorY);
    }
}

void EntityLayer::drawFront(Renderer& renderer, float scrollX, float floorY) const {
    for (const Explod& explod : explods_) {
        if (explod.onTop || explod.behindOwner) continue;
        drawExplod(renderer, explod, scrollX, floorY);
    }

    for (const Projectile& projectile : projectiles_) {
        // A flying projectile that has just connected draws its hit animation instead of
        // its flight animation, in place, for the few ticks it has left. Once it enters
        // a removal state, `anim` and `animFromFightFx` have already been switched to
        // the animation that state selected (§5.3), and that outranks the hit flash: a
        // blast whose last hit was the one that spent it plays the removal animation,
        // because that is the one still running when the hitstop ends.
        const bool hitFlash = projectile.isFlying() && projectile.missTicks > 0 &&
                              projectile.hitAnim >= 0;
        const int anim = hitFlash ? projectile.hitAnim : projectile.anim;
        // And whichever it is, it comes out of the file that animation's own `F` prefix
        // named -- a screenpack's fightfx or the character's own .air.
        const bool fromFightFx =
            hitFlash ? projectile.hitAnimFromFightFx : projectile.animFromFightFx;
        SpriteBank* bank = nullptr;
        const AnimationSet* animations = nullptr;
        if (fromFightFx) {
            bank = fxSprites_;
            animations = fxAnimations_;
        } else if (projectile.owner != nullptr) {
            bank = &const_cast<Character*>(projectile.owner)->sprites();
            animations = &projectile.owner->animations();
        }
        if (bank == nullptr || animations == nullptr) continue;
        const EntityFrame frame = entityFrame(animations, anim, projectile.clock);
        if (frame.group < 0) continue;
        // `projshadow`, drawn under the projectile's current animation frame.
        // No player's `noshadow` reaches it; only the global gates do.
        if (drawShadows_) {
            drawObjectShadow(renderer, *bank, frame, shadow_, projectile.shadow,
                             /*anyComponentSelectsStage=*/true, projectile.x, projectile.y,
                             projectile.facing, projectile.scaleX, projectile.scaleY,
                             /*flipV=*/false, scrollX, floorY);
        }
        const float screenX = projectile.x - scrollX;
        const float screenY = floorY - projectile.y;
        // `ownpal` for a projectile is the same decision as for an explod: its
        // own sprite palette, or the owner's current selection. See
        // `chooseEntityPalette`.
        PaletteBind paletteBinding;
        const PaletteBind* palette = entityPaletteBind(
            *bank, frame.group, frame.image, projectile.ownpal, projectile.owner,
            projectile.ownerMachine, paletteBinding);
        drawSpriteAt(renderer, *bank, frame.group, frame.image,
                     screenX + frame.offsetX * static_cast<float>(projectile.facing) *
                                   projectile.scaleX,
                     screenY + frame.offsetY * projectile.scaleY, projectile.facing,
                     projectile.scaleX, projectile.scaleY, Color{}, frame.trans, palette);
    }
}

// The `ontop` explods, drawn once everything else in the world has been.
//
// The reference describes the parameter as putting the explod "over all other
// sprites and background layers", and gives it precedence over `sprpriority`.
// The two halves of that are why this is a layer and not just a very high
// priority: a priority orders one sprite against another, and "over all other
// sprites" is not an ordering, it is a place to draw from.
//
// **Where the layer ends is a judgement, and it is recorded rather than
// assumed.** "All other sprites and background layers" is read here as the
// *world* -- the stage's own layers and every fighter, explod and projectile in
// them -- with the HUD and the banners drawn after it, because a life bar that
// a character's comic text can cover is a bar the player cannot read. The other
// reading, that the HUD's art is sprites too and so is covered, is the one the
// literal words allow and the one MUGEN's own draw order may well implement.
// Nothing in the corpus distinguishes them: of the 3469 `ontop = 1` explods
// across the 363 character files in local_chars, the overwhelming majority are
// effects in the middle of the screen. See PARITY-ROADMAP.md, and note that this
// is a display-only distinction -- no state, no collision and no timing depends
// on it.
void EntityLayer::drawTop(Renderer& renderer, float scrollX, float floorY) const {
    for (const Explod& explod : explods_) {
        if (!explod.onTop) continue;
        drawExplod(renderer, explod, scrollX, floorY);
    }
}

}  // namespace af

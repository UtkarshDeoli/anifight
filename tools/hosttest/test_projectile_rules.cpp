// Checks the projectile rules a run on the device cannot show directly: the
// exact projectile-vs-projectile priority exchange, a `projhits = 0` cancel,
// the player-attack attribute/role test and its state-2 outcome, the unscaled
// collision-box conversion, the velocity-aware bounds, the removal-animation
// fallback chain and the deferred player-hit timing.
//
// The layer's update loop and the fight screen's passes are integration code;
// what is pinned here is the arithmetic they are built from, so a change in
// the numbers is caught without a character or a stage.
//
// Build and run from the repo root:
//
//     clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_projectile_rules \
//         tools/hosttest/test_projectile_rules.cpp \
//         app/src/main/cpp/engine/projectile_rules.cpp
//     /tmp/test_projectile_rules

#include <cmath>
#include <cstdio>

#include "engine/projectile_rules.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

void checkNear(float actual, float expected, const char* what) {
    if (std::fabs(actual - expected) < 1e-5f) return;
    std::fprintf(stderr, "FAIL: %s (expected %.4f, got %.4f)\n", what,
                 static_cast<double>(expected), static_cast<double>(actual));
    ++failures;
}

void priority() {
    // §2.4, the three comparisons against the original values.
    {
        // Equal: A loses a hit and a priority point, B loses a hit and a priority
        // point. Both defaults are 1, so this is the ordinary clash.
        const af::ProjectilePriorityOutcome out = af::resolveProjectilePriority(1, 1);
        check(out.aHits == -1, "equal priority: A loses one hit");
        check(out.aPriority == -1, "equal priority: A loses one priority");
        check(out.bHits == -1, "equal priority: B loses one hit");
        check(out.bPriority == -1, "equal priority: B loses one priority");
    }
    {
        // A lower: the lower projectile loses a hit, the higher one a priority point.
        const af::ProjectilePriorityOutcome out = af::resolveProjectilePriority(1, 2);
        check(out.aHits == -1, "lower priority: A loses one hit");
        check(out.aPriority == 0, "lower priority: A keeps its priority");
        check(out.bHits == 0, "lower priority: B keeps its hit");
        check(out.bPriority == -1, "lower priority: B loses one priority");
    }
    {
        // A higher: A keeps its hit but loses a priority point, B loses a hit.
        const af::ProjectilePriorityOutcome out = af::resolveProjectilePriority(3, 1);
        check(out.aHits == 0, "higher priority: A keeps its hit");
        check(out.aPriority == -1, "higher priority: A loses one priority");
        check(out.bHits == -1, "higher priority: B loses one hit");
        check(out.bPriority == 0, "higher priority: B keeps its priority");
    }
    {
        // Negative priorities are ordinary values, not clamped to zero.
        const af::ProjectilePriorityOutcome out = af::resolveProjectilePriority(-2, 1);
        check(out.aHits == -1, "negative priority: the lower one loses a hit");
        check(out.aPriority == 0, "negative priority: it does not gain a point");
        check(out.bHits == 0, "negative priority: the higher one keeps its hit");
        check(out.bPriority == -1, "negative priority: the higher one loses a point");
    }
    {
        const af::ProjectilePriorityOutcome out = af::resolveProjectilePriority(-2, -2);
        check(out.aHits == -1 && out.bHits == -1, "equal negative priorities trade hits");
        check(out.aPriority == -1 && out.bPriority == -1,
              "equal negative priorities trade priority");
    }
}

void duel() {
    // `projhits = 0` still cancels: no "has hits" gate, no `projremove` test.
    {
        const af::ProjectileDuelOutcome out = af::resolveProjectileDuel(0, 1, 1, 1);
        check(out.aCancelled, "a zero-hit projectile is cancelled by a clash");
        check(out.bCancelled, "and the ordinary projectile loses its only hit too");
    }
    // A zero-hit projectile against a high-priority survivor: only the zero one goes.
    {
        const af::ProjectileDuelOutcome out = af::resolveProjectileDuel(0, 1, 2, 5);
        check(out.aCancelled, "zero hits cancels against higher priority");
        check(!out.bCancelled, "the higher-priority survivor stays");
        check(out.bHits == 2, "and keeps every hit it had");
    }
    // A survivor with remaining hits is not cancelled by an equal clash.
    {
        const af::ProjectileDuelOutcome out = af::resolveProjectileDuel(2, 1, 2, 1);
        check(!out.aCancelled && !out.bCancelled, "two-hit projectiles survive equal priority");
        check(out.aHits == 1 && out.bHits == 1, "but each spends one hit");
    }
}

void boxing() {
    // No scale is involved by construction (§7.2): the world box is the raw Clsn
    // record mirrored by facing and placed at the projectile's position.
    {
        const af::ProjectileRect world =
            af::projectileBoxToWorld(10.0f, -20.0f, 30.0f, 0.0f, 1, 100.0f, 50.0f, 210.0f);
        checkNear(world.x1, 110.0f, "box x1 is origin + facing * box x1");
        checkNear(world.x2, 130.0f, "box x2 is origin + facing * box x2");
        checkNear(world.y1, 140.0f, "box y1 is floor - height + box y1");
        checkNear(world.y2, 160.0f, "box y2 is floor - height + box y2");
    }
    {
        // Facing -1 mirrors x but not y.
        const af::ProjectileRect world =
            af::projectileBoxToWorld(10.0f, -20.0f, 30.0f, 0.0f, -1, 100.0f, 50.0f, 210.0f);
        checkNear(world.x1, 70.0f, "facing -1 mirrors the near x");
        checkNear(world.x2, 90.0f, "facing -1 mirrors the far x");
        checkNear(world.y1, 140.0f, "facing does not move y1");
    }
    {
        // A Clsn record may be written either way round; the rect is normalised.
        const af::ProjectileRect world =
            af::projectileBoxToWorld(30.0f, 0.0f, 10.0f, -20.0f, 1, 100.0f, 0.0f, 210.0f);
        check(world.x1 <= world.x2 && world.y1 <= world.y2, "the rect is normalised");
    }
    {
        const af::ProjectileRect a{0.0f, 0.0f, 10.0f, 10.0f};
        const af::ProjectileRect b{9.0f, 9.0f, 20.0f, 20.0f};
        const af::ProjectileRect c{10.0f, 10.0f, 20.0f, 20.0f};
        check(af::projectileRectsOverlap(a, b), "overlapping rects touch");
        check(!af::projectileRectsOverlap(a, c), "edge contact is not an overlap");
    }
}

void bounds() {
    // Outside the lower bound and moving back up: survives (§6.2).
    check(!af::projectilePastAxisBound(-10.0f, 1.0f, 0.0f, 100.0f),
          "below the lower bound but moving back in survives");
    // Outside the lower bound and moving further out, or standing still: removed.
    check(af::projectilePastAxisBound(-10.0f, -1.0f, 0.0f, 100.0f),
          "below the lower bound and moving out is removed");
    check(af::projectilePastAxisBound(-10.0f, 0.0f, 0.0f, 100.0f),
          "below the lower bound with no velocity is removed");
    // Above the upper bound: the mirror image.
    check(!af::projectilePastAxisBound(110.0f, -1.0f, 0.0f, 100.0f),
          "above the upper bound but moving back in survives");
    check(af::projectilePastAxisBound(110.0f, 1.0f, 0.0f, 100.0f),
          "above the upper bound and moving out is removed");
    // Exactly on a bound is still inside it.
    check(!af::projectilePastAxisBound(0.0f, -1.0f, 0.0f, 100.0f),
          "the lower bound itself is inside");
    check(!af::projectilePastAxisBound(100.0f, 1.0f, 0.0f, 100.0f),
          "the upper bound itself is inside");
    // The edge test is position-only, with no velocity escape (§6.2).
    check(af::projectilePastScreenEdge(160.0f + 160.0f + 40.0f + 1.0f, 160.0f, 160.0f, 40.0f),
          "past the right screen edge");
    check(!af::projectilePastScreenEdge(160.0f + 160.0f + 40.0f, 160.0f, 160.0f, 40.0f),
          "the edge bound itself is inside");
    check(af::projectilePastScreenEdge(-41.0f, 160.0f, 160.0f, 40.0f),
          "past the left screen edge");
}

void removalAnimations() {
    // State 2: `projremanim`.
    {
        const af::ProjectileRemovalAnim out = af::resolveProjectileRemovalAnim(
            af::ProjectileState::Removal, 10, false, 20, true, 30, false);
        check(out.anim == 20, "state 2 plays projremanim");
        check(out.fromFightFx, "state 2 carries projremanim's file");
    }
    // State 3: `projcancelanim` first.
    {
        const af::ProjectileRemovalAnim out = af::resolveProjectileRemovalAnim(
            af::ProjectileState::Cancelled, 10, false, 20, true, 30, false);
        check(out.anim == 30, "state 3 prefers projcancelanim");
        check(!out.fromFightFx, "state 3 carries projcancelanim's file");
    }
    // State 3 fallback: `projremanim`.
    {
        const af::ProjectileRemovalAnim out = af::resolveProjectileRemovalAnim(
            af::ProjectileState::Cancelled, 10, false, 20, true, -1, false);
        check(out.anim == 20, "state 3 falls back to projremanim");
        check(out.fromFightFx, "the fallback carries projremanim's file");
    }
    // State 3 fallback: `projhitanim`.
    {
        const af::ProjectileRemovalAnim out = af::resolveProjectileRemovalAnim(
            af::ProjectileState::Cancelled, 10, true, -1, false, -1, false);
        check(out.anim == 10, "state 3 falls back to projhitanim");
        check(out.fromFightFx, "the second fallback carries projhitanim's file");
    }
    // State 4: `projhitanim`.
    {
        const af::ProjectileRemovalAnim out = af::resolveProjectileRemovalAnim(
            af::ProjectileState::Hit, 10, true, 20, false, 30, false);
        check(out.anim == 10, "state 4 plays projhitanim");
        check(out.fromFightFx, "state 4 carries projhitanim's file");
    }
    // Nothing configured: -1, which the layer frees on entry.
    {
        const af::ProjectileRemovalAnim out = af::resolveProjectileRemovalAnim(
            af::ProjectileState::Cancelled, -1, false, -1, false, -1, false);
        check(out.anim == -1, "a cancel with nothing configured frees at once");
    }
}

// The player-attack-vs-projectile half of §4. The attribute model below is the
// one the HitDef parser proves: `affectteam` is Friend = 1, Enemy = 2, Both = 3
// (default Enemy), the player's side field and the projectile's role are
// MUGEN's 1/2 values, and a side field of 2 swaps the mask's low two bits
// before the overlap test. That swap is what makes `Enemy` mean "the other
// team" from either side.
void attackAttribute() {
    // The state gate: only a flying projectile can be shot down. State 2 is
    // deliberately excluded here, even though it may still hit a player (§3.1).
    check(af::projectileHittableByAttack(af::ProjectileState::Flying),
          "a flying projectile is hittable by an attack");
    check(!af::projectileHittableByAttack(af::ProjectileState::Removal),
          "a removal-animation projectile is not");
    check(!af::projectileHittableByAttack(af::ProjectileState::Cancelled),
          "a cancelled projectile is not");
    check(!af::projectileHittableByAttack(af::ProjectileState::Hit),
          "a hit projectile is not");

    // The swap itself touches only bits 0 and 1.
    check(af::swapLowTwoBits(0b0001) == 0b0010, "swap moves bit 0 to bit 1");
    check(af::swapLowTwoBits(0b0010) == 0b0001, "swap moves bit 1 to bit 0");
    check(af::swapLowTwoBits(0b1110) == 0b1101, "swap leaves the upper bits alone");
    check(af::swapLowTwoBits(0) == 0, "swapping nothing is still nothing");

    // The engine's 0/1 sides become MUGEN's 1/2 fields.
    check(af::mugenSideField(0) == 1 && af::mugenSideField(1) == 2,
          "engine sides map to MUGEN's side field");
    check(af::mugenProjectileRole(0) == 1 && af::mugenProjectileRole(1) == 2,
          "engine sides map to the projectile role");

    // P1 (side field 1) with the default Enemy mask hits a P2 projectile
    // (role 2), and not a P1 one.
    check(af::projectileAttackAttributeMatches(af::kAffectTeamEnemy, 1, 2),
          "P1's default attack hits the opponent's projectile");
    check(!af::projectileAttackAttributeMatches(af::kAffectTeamEnemy, 1, 1),
          "P1's default attack passes through its own projectile");

    // P2 (side field 2) swaps Enemy (0b10) to 0b01, so it hits a P1
    // projectile (role 1) and not a P2 one -- the whole point of the swap.
    check(af::projectileAttackAttributeMatches(af::kAffectTeamEnemy, 2, 1),
          "P2's swapped default attack hits the opponent's projectile");
    check(!af::projectileAttackAttributeMatches(af::kAffectTeamEnemy, 2, 2),
          "P2's swapped default attack passes through its own projectile");

    // Friend and Both behave as their masks say. Friend hits the same team,
    // Both hits either, and both follow the same side swap.
    check(af::projectileAttackAttributeMatches(af::kAffectTeamFriend, 1, 1),
          "Friend hits the same team from P1");
    check(!af::projectileAttackAttributeMatches(af::kAffectTeamFriend, 1, 2),
          "Friend does not hit the other team from P1");
    check(af::projectileAttackAttributeMatches(af::kAffectTeamFriend, 2, 2),
          "Friend hits the same team from P2 after the swap");
    check(af::projectileAttackAttributeMatches(af::kAffectTeamBoth, 1, 1) &&
              af::projectileAttackAttributeMatches(af::kAffectTeamBoth, 1, 2),
          "Both hits either role from P1");
    check(af::projectileAttackAttributeMatches(af::kAffectTeamBoth, 2, 1) &&
              af::projectileAttackAttributeMatches(af::kAffectTeamBoth, 2, 2),
          "Both hits either role from P2 after the swap");
}

// §4's outcome: hits--, then state 2 only when exhausted and removable, and
// otherwise a pause from the attacking player's pausetime.
void attackOutcome() {
    // A surviving removable projectile pauses for the attack's pausetime.
    {
        const af::ProjectileAttackOutcome out =
            af::applyPlayerAttackToProjectile(3, true, 8);
        check(out.hitsLeft == 2, "an attack spends one hit");
        check(!out.enterRemoval, "a projectile with hits left keeps flying");
        check(out.pause == 8, "and pauses for the attack's pausetime");
    }
    // The last hit of a removable projectile: state 2, no pause.
    {
        const af::ProjectileAttackOutcome out =
            af::applyPlayerAttackToProjectile(1, true, 8);
        check(out.hitsLeft == 0, "the last hit reaches zero");
        check(out.enterRemoval, "and the removable projectile enters state 2");
        check(out.pause == 0, "the removal state writes no pause");
    }
    // `projremove = 0`: the hit is still spent, but the projectile survives
    // and pauses. It stays flying at zero hits.
    {
        const af::ProjectileAttackOutcome out =
            af::applyPlayerAttackToProjectile(1, false, 8);
        check(out.hitsLeft == 0, "projremove = 0 still spends the hit");
        check(!out.enterRemoval, "projremove = 0 never enters removal from an attack");
        check(out.pause == 8, "and still pauses");
    }
    // There is no "has hits" gate on this pass: an already-spent projectile is
    // still touched, and goes negative rather than clamping.
    {
        const af::ProjectileAttackOutcome out =
            af::applyPlayerAttackToProjectile(0, true, 8);
        check(out.hitsLeft == -1, "hits go negative rather than clamping");
        check(out.enterRemoval, "an already-spent projectile is destroyed");
    }

    // No double-processing: the state the attack writes is state 2, which both
    // the attack gate and the projectile-vs-projectile gate (`isFlying`) refuse
    // for the rest of the tick. This is the invariant that keeps a shot-down
    // projectile out of the duel pass.
    const af::ProjectileAttackOutcome removal =
        af::applyPlayerAttackToProjectile(1, true, 8);
    check(removal.enterRemoval, "the outcome that removes a projectile is state 2");
    check(!af::projectileIsFlying(af::ProjectileState::Removal),
          "the duel pass's flying gate refuses the removal state");
    check(!af::projectileHittableByAttack(af::ProjectileState::Removal),
          "and the attack pass does not touch it twice");
}

void pendingHits() {
    // Not the last hit: both a pause and a cooldown.
    {
        const af::ProjectilePendingTiming out =
            af::projectilePendingHitTiming(3, true, 8, 12);
        check(out.pause == 8, "a surviving hit applies pausetime");
        check(out.cooldown == 12, "a surviving hit reloads projmisstime");
    }
    // Last hit of a removable projectile: neither.
    {
        const af::ProjectilePendingTiming out =
            af::projectilePendingHitTiming(1, true, 8, 12);
        check(out.pause == 0, "the last removable hit applies no pause");
        check(out.cooldown == 0, "the last removable hit applies no cooldown");
    }
    // Last hit of `projremove = 0`: still applies both (§3.2).
    {
        const af::ProjectilePendingTiming out =
            af::projectilePendingHitTiming(1, false, 8, 12);
        check(out.pause == 8, "projremove = 0 still pauses on the last hit");
        check(out.cooldown == 12, "projremove = 0 still cools down on the last hit");
    }
    // Application: hits--, state 4 only for an exhausted removable projectile.
    {
        const af::ProjectileHitApplication out = af::applyProjectilePendingHit(2, true);
        check(out.hitsLeft == 1, "applying a hit decrements projhits");
        check(!out.enterHitState, "a projectile with hits left keeps flying");
    }
    {
        const af::ProjectileHitApplication out = af::applyProjectilePendingHit(1, true);
        check(out.hitsLeft == 0, "the last hit reaches zero");
        check(out.enterHitState, "the last removable hit enters state 4");
    }
    {
        const af::ProjectileHitApplication out = af::applyProjectilePendingHit(1, false);
        check(out.hitsLeft == 0, "projremove = 0 still spends the hit");
        check(!out.enterHitState, "projremove = 0 survives at zero hits");
    }
    // No clamp: a second application on an already-zero projectile goes negative.
    {
        const af::ProjectileHitApplication out = af::applyProjectilePendingHit(0, true);
        check(out.hitsLeft == -1, "hits go negative rather than clamping");
        check(out.enterHitState, "and it still enters state 4");
    }
}

// The player-side contact triple behind `ProjHit` / `ProjGuarded` /
// `ProjContact` and their `*Time` forms. The record lives on the owning side,
// not on the projectile, so it is still readable after the slot is freed.
void projectileContacts() {
    const int maskHit = af::projectileContactMask(af::kProjectileContactHit);
    const int maskGuarded = af::projectileContactMask(af::kProjectileContactGuarded);
    const int maskContact = maskHit | maskGuarded;

    // The codes are the reference's: 1 hit, 3 cancel, with 2 the guarded split.
    check(af::kProjectileContactHit == 1, "a player hit records code 1");
    check(af::kProjectileContactGuarded == 2, "a guard records code 2");
    check(af::kProjectileContactCancelled == 3, "a cancel records code 3");

    // Nothing recorded: every trigger's no-match answer.
    const af::ProjectileContactRecord none;
    check(af::projectileContactTicks(none, maskHit, 0) == -1,
          "no contact: ProjHitTime is -1");
    check(af::projectileContactTicks(none, maskContact, 0) == -1,
          "no contact: ProjContactTime is -1");
    check(!af::projectileContactJustHappened(none, maskHit, 0),
          "no contact: the boolean form is false");

    // A hit: contact code 1. The fight records it at 0; one entity update makes
    // it 1, which is the one tick the boolean form answers.
    af::ProjectileContactRecord hit;
    af::recordProjectileContact(hit, af::kProjectileContactHit, 1234);
    check(hit.active && hit.id == 1234 && hit.ticks == 0,
          "a contact starts at zero ticks on the recording tick");
    check(af::projectileContactTicks(hit, maskHit, 0) == 0,
          "the time form reads zero on the recording tick");
    check(!af::projectileContactJustHappened(hit, maskHit, 0),
          "but the one-tick boolean form does not fire until the next tick");
    af::advanceProjectileContact(hit);
    check(af::projectileContactTicks(hit, maskHit, 0) == 1,
          "one tick later ProjHitTime is 1");
    check(af::projectileContactJustHappened(hit, maskHit, 0),
          "and the boolean form fires");
    check(af::projectileContactTicks(hit, maskContact, 0) == 1,
          "ProjContact includes a hit");
    check(af::projectileContactTicks(hit, maskGuarded, 0) == -1,
          "ProjGuarded does not");
    af::advanceProjectileContact(hit);
    check(af::projectileContactTicks(hit, maskHit, 0) == 2,
          "the clock keeps counting after the one-tick window");
    check(!af::projectileContactJustHappened(hit, maskHit, 0),
          "and the boolean form stops firing");

    // ID filtering. Zero and negative mean "any projectile", per the docs.
    check(af::projectileContactTicks(hit, maskHit, 1234) == 2,
          "the matching id answers");
    check(af::projectileContactTicks(hit, maskHit, 456) == -1,
          "a different id does not");
    check(af::projectileContactTicks(hit, maskHit, 0) == 2,
          "id 0 checks every projectile");
    check(af::projectileContactTicks(hit, maskHit, -3) == 2,
          "a negative id defaults to zero");

    // A guard: code 2. It answers ProjGuarded and ProjContact, not ProjHit.
    af::ProjectileContactRecord guarded;
    af::recordProjectileContact(guarded, af::kProjectileContactGuarded, 77);
    af::advanceProjectileContact(guarded);
    check(af::projectileContactTicks(guarded, maskGuarded, 0) == 1,
          "ProjGuardedTime sees a guard");
    check(af::projectileContactTicks(guarded, maskContact, 0) == 1,
          "ProjContact includes a guard");
    check(af::projectileContactTicks(guarded, maskHit, 0) == -1,
          "ProjHit does not");

    // A cancel: code 3. The docs' ProjContact is "hit or guarded", so a
    // cancellation (projectile-vs-projectile or the player-attack pass) is
    // recorded but not answered by any of the three contact triggers.
    af::ProjectileContactRecord cancelled;
    af::recordProjectileContact(cancelled, af::kProjectileContactCancelled, 9);
    af::advanceProjectileContact(cancelled);
    check(af::projectileContactTicks(cancelled, maskHit, 0) == -1 &&
              af::projectileContactTicks(cancelled, maskGuarded, 0) == -1 &&
              af::projectileContactTicks(cancelled, maskContact, 0) == -1,
          "a cancel answers none of the player-contact triggers");

    // The last contact wins, as one player-side triple implies.
    af::recordProjectileContact(hit, af::kProjectileContactGuarded, 88);
    af::advanceProjectileContact(hit);
    check(hit.id == 88 && hit.ticks == 1, "a new contact replaces the old one");
    check(af::projectileContactTicks(hit, maskHit, 0) == -1,
          "and the old hit no longer answers ProjHitTime");

    // A round reset clears the record; that is the narrower per-round choice.
    hit = af::ProjectileContactRecord{};
    check(!hit.active && af::projectileContactTicks(hit, maskContact, 0) == -1,
          "a round reset forgets the previous round's contact");
}

}  // namespace

int main() {
    priority();
    duel();
    boxing();
    bounds();
    removalAnimations();
    attackAttribute();
    attackOutcome();
    pendingHits();
    projectileContacts();
    if (failures == 0) std::printf("all projectile-rule checks passed\n");
    return failures == 0 ? 0 : 1;
}

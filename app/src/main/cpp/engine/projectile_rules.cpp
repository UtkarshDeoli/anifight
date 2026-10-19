#include "engine/projectile_rules.h"

#include <algorithm>

namespace af {

ProjectileRect projectileBoxToWorld(float boxX1, float boxY1, float boxX2, float boxY2,
                                    int facing, float originX, float originY, float floorY) {
    // No scale is applied, by construction: `projscale` reaches the draw path
    // only (§7.2). The x mirror is the projectile's facing, and the y is added
    // to the floor line because a Clsn y is negative *upward* while the engine's
    // world y grows downward from the floor.
    const float direction = static_cast<float>(facing);
    const float x1 = originX + direction * boxX1;
    const float x2 = originX + direction * boxX2;
    const float axis = floorY - originY;
    const float y1 = axis + boxY1;
    const float y2 = axis + boxY2;
    return ProjectileRect{std::min(x1, x2), std::min(y1, y2), std::max(x1, x2),
                          std::max(y1, y2)};
}

bool projectileRectsOverlap(const ProjectileRect& a, const ProjectileRect& b) {
    return a.x1 < b.x2 && b.x1 < a.x2 && a.y1 < b.y2 && b.y1 < a.y2;
}

ProjectilePriorityOutcome resolveProjectilePriority(int aPriority, int bPriority) {
    // §2.4, in the binary's own order. The comparisons are all against the
    // values the pair entered with; only the deltas below are written.
    ProjectilePriorityOutcome out;
    if (aPriority <= bPriority) --out.aHits;
    if (bPriority <= aPriority) {
        --out.aPriority;
        --out.bHits;
    }
    if (aPriority <= bPriority) --out.bPriority;
    return out;
}

ProjectileDuelOutcome resolveProjectileDuel(int aHits, int aPriority, int bHits,
                                             int bPriority) {
    const ProjectilePriorityOutcome exchange =
        resolveProjectilePriority(aPriority, bPriority);
    ProjectileDuelOutcome out;
    out.aHits = aHits + exchange.aHits;
    out.aPriority = aPriority + exchange.aPriority;
    out.bHits = bHits + exchange.bHits;
    out.bPriority = bPriority + exchange.bPriority;
    out.aCancelled = out.aHits <= 0;
    out.bCancelled = out.bHits <= 0;
    return out;
}

int swapLowTwoBits(int mask) {
    // §4/§2.2: only bits 0 and 1 trade places; the bits above them -- the
    // class/type part of the mask, if a caller ever carries one -- are untouched.
    return (mask & ~0x3) | ((mask & 0x1) << 1) | ((mask & 0x2) >> 1);
}

bool projectileAttackAttributeMatches(int attackAffectTeam, int attackerSideField,
                                      int projectileRole) {
    // §4: compare the attacking player's attribute mask with the projectile's
    // role, swapping the mask's low two bits first when the player's side field
    // is 2. The swap is what makes "enemy" mean the other team from either side;
    // see the header for the parser values and the derivation.
    const int mask = attackerSideField == 2 ? swapLowTwoBits(attackAffectTeam)
                                            : attackAffectTeam;
    return (mask & projectileRole) != 0;
}

ProjectileAttackOutcome applyPlayerAttackToProjectile(int hitsLeft, bool removeOnHit,
                                                      int attackPause) {
    // §4, in the binary's order: spend a hit first, then decide. A projectile
    // whose hits are already <= 0 is still touched by this pass (there is no
    // `projhits > 0` gate here), and it enters Removal if it is removable. When
    // it does not go, the surviving projectile freezes for the attack's
    // `pausetime`; that includes a `projremove = 0` projectile that has just
    // spent its last hit -- it stays flying at zero hits.
    ProjectileAttackOutcome out;
    out.hitsLeft = hitsLeft - 1;
    if (out.hitsLeft <= 0 && removeOnHit) {
        out.enterRemoval = true;
        return out;  // state 2 plays `projremanim`; no pause is written
    }
    out.pause = attackPause;
    return out;
}

int projectileContactTicks(const ProjectileContactRecord& record, int codeMask,
                           int filterId) {
    if (!record.active) return -1;
    // The docs: an omitted id, 0 and a negative id all mean "check every
    // projectile". Only a positive argument filters.
    if (filterId > 0 && record.id != filterId) return -1;
    if (record.code <= 0 || record.code > 30) return -1;
    if ((codeMask & projectileContactMask(record.code)) == 0) return -1;
    return record.ticks;
}

bool projectilePastAxisBound(float position, float step, float lower, float upper) {
    // §6.2: a projectile outside a bound survives while it is travelling back
    // towards the valid region. A zero step is not travelling back in.
    if (position < lower) return step <= 0.0f;
    if (position > upper) return step >= 0.0f;
    return false;
}

bool projectilePastScreenEdge(float x, float cameraMidX, float halfScreenWidth,
                              float edgeBound) {
    return (x < cameraMidX - halfScreenWidth - edgeBound) ||
           (x > cameraMidX + halfScreenWidth + edgeBound);
}

ProjectileRemovalAnim resolveProjectileRemovalAnim(ProjectileState state, int hitAnim,
                                                   bool hitFromFightFx, int removeAnim,
                                                   bool removeFromFightFx, int cancelAnim,
                                                   bool cancelFromFightFx) {
    switch (state) {
        case ProjectileState::Removal:
            // §5.3: `projremanim`. It was already resolved against
            // `projhitanim` at spawn when the file did not name one.
            return ProjectileRemovalAnim{removeAnim, removeFromFightFx};
        case ProjectileState::Cancelled:
            // §2.5 / §5.3: `projcancelanim` -> `projremanim` -> `projhitanim`.
            if (cancelAnim >= 0) {
                return ProjectileRemovalAnim{cancelAnim, cancelFromFightFx};
            }
            if (removeAnim >= 0) {
                return ProjectileRemovalAnim{removeAnim, removeFromFightFx};
            }
            return ProjectileRemovalAnim{hitAnim, hitFromFightFx};
        case ProjectileState::Hit:
            return ProjectileRemovalAnim{hitAnim, hitFromFightFx};
        case ProjectileState::Flying:
        default:
            return ProjectileRemovalAnim{};
    }
}

ProjectilePendingTiming projectilePendingHitTiming(int hitsLeft, bool removeOnHit,
                                                   int hitPause, int missTime) {
    // §3.2: the last hit of a removable projectile is the one that switches it
    // to state 4, so it schedules neither a pause nor a re-hit cooldown.
    if (hitsLeft - 1 <= 0 && removeOnHit) return ProjectilePendingTiming{0, 0};
    return ProjectilePendingTiming{hitPause, missTime};
}

ProjectileHitApplication applyProjectilePendingHit(int hitsLeft, bool removeOnHit) {
    ProjectileHitApplication out;
    out.hitsLeft = hitsLeft - 1;
    out.enterHitState = out.hitsLeft <= 0 && removeOnHit;
    return out;
}

}  // namespace af

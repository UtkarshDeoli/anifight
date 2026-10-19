// The pure half of projectile collision and lifetime.
//
// Everything here is arithmetic that does not need a renderer, a fighter or an
// `EntityLayer`, which is what lets the host suite pin the rules a device test
// cannot see directly: the exact priority exchange, the velocity-aware bounds,
// the removal-animation fallback chain and the deferred player-hit bookkeeping.
// See `docs/mugen-re/PROJECTILE-COLLISION-RE.md`; the section numbers below are
// that document's.
#pragma once

namespace af {

// MUGEN's projectile state (`+0x04`, §5.1). Only `Flying` counts towards
// `numproj` and only it can collide with another projectile; `Removal` (2) can
// still hit players until its animation ends.
enum class ProjectileState : int {
    Flying = 1,
    Removal = 2,    // bounds, edge, age, player attack
    Cancelled = 3,  // projectile vs projectile
    Hit = 4,        // a player hit with `projremove != 0`
};

[[nodiscard]] inline bool projectileIsFlying(ProjectileState state) {
    return state == ProjectileState::Flying;
}

// Whether a player's attack may touch this projectile at all (§4). Unlike the
// projectile-vs-player pass, which accepts states 1 and 2 (§3.1), this pass
// requires the projectile to be `Flying`: once it is on its way out it is no
// longer a thing an attack can shoot down.
[[nodiscard]] inline bool projectileHittableByAttack(ProjectileState state) {
    return state == ProjectileState::Flying;
}

// A world-space collision rectangle, in the same x1,y1,x2,y2 order as Rect4.
struct ProjectileRect {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
};

// Converts one Clsn record to world space for a projectile.
//
// There is deliberately no scale argument (§7.2): `projscale` is a draw
// parameter only, and collision uses the raw position and the unscaled Clsn
// box. `originY` is the projectile's height above the floor, as everywhere in
// the engine, and `floorY` is the floor line.
[[nodiscard]] ProjectileRect projectileBoxToWorld(float boxX1, float boxY1, float boxX2,
                                                 float boxY2, int facing, float originX,
                                                 float originY, float floorY);

[[nodiscard]] bool projectileRectsOverlap(const ProjectileRect& a, const ProjectileRect& b);

// The exact priority exchange for an ordered pair (A, B), §2.4. All four fields
// are deltas: hits lose one, priority loses one. Comparisons are against the
// *original* priorities, which is what makes the equal case lose one of each on
// both sides. There is no clamp and negative priorities are ordinary values.
struct ProjectilePriorityOutcome {
    int aHits = 0;
    int aPriority = 0;
    int bHits = 0;
    int bPriority = 0;
};

[[nodiscard]] ProjectilePriorityOutcome resolveProjectilePriority(int aPriority,
                                                                int bPriority);

// The same exchange applied to a live pair, plus cancellation. A side is
// cancelled when its hits are <= 0 after the exchange -- with no `projremove`
// test and no "must have had hits to begin with" gate, which is why a
// `projhits = 0` projectile still cancels (§2.4, §10.3).
struct ProjectileDuelOutcome {
    int aHits = 0;
    int aPriority = 0;
    int bHits = 0;
    int bPriority = 0;
    bool aCancelled = false;
    bool bCancelled = false;
};

[[nodiscard]] ProjectileDuelOutcome resolveProjectileDuel(int aHits, int aPriority, int bHits,
                                                          int bPriority);

// ---------------------------------------------------------------------------
// Player attack vs projectile — §4
//
// The reference pass (`0x46F3D0`) runs the attacking player's attribute mask
// against the opponent projectile's side/role field. The mask is the HitDef's
// `affectteam`, which the HitDef parser stores as:
//
//     F(riend) = 1, E(nemy) = 2, B(oth) = 3, default 2
//
// (verified in the parser at `0x47DA40`; the same values reach the projectile's
// embedded HitDef block). MUGEN's fields are numbered 1 and 2 rather than this
// engine's 0 and 1, so a side maps to `side + 1` on both ends.
//
// When the *player's* side field is 2, the low two bits of the attack mask are
// swapped before the overlap test. That swap is what makes the same mask work
// from either side: `Enemy` on P1 means "team 2" (0b10), while `Enemy` on P2
// means "team 1" (0b01). Overlap with the projectile's role then admits the
// opponent's projectiles from either side and refuses the player's own.
//
// The HitDef parser supplies this mask (`StateMachine::HitDef::affectTeam`),
// defaulting to `Enemy` when the file writes no `affectteam`. The pure helper
// takes the mask as an argument so the parse and the collision pass stay
// independently testable.
inline constexpr int kAffectTeamFriend = 1;
inline constexpr int kAffectTeamEnemy = 2;
inline constexpr int kAffectTeamBoth = kAffectTeamFriend | kAffectTeamEnemy;

// The engine's 0/1 side as MUGEN's 1/2 field. The reference keeps a player
// side field (`+0xC`) and a projectile role (`+0x8`) separately; a 1v1 match
// makes both the same value, and this engine has no team structure that could
// tell them apart.
[[nodiscard]] inline int mugenSideField(int side) {
    return side == 0 ? 1 : 2;
}
[[nodiscard]] inline int mugenProjectileRole(int ownerSide) {
    return ownerSide == 0 ? 1 : 2;
}

// Swaps bit 0 and bit 1, leaving every higher bit alone. §4 / §2.2.
[[nodiscard]] int swapLowTwoBits(int mask);

// Whether the attacking player's `affectteam` mask matches this projectile.
// `attackerSideField` and `projectileRole` are MUGEN's 1/2 values, not the
// engine's 0/1 sides. The caller is still responsible for the "opponent's
// projectile manager" half of the gate: the reference iterates only the other
// player's projectiles (§4).
[[nodiscard]] bool projectileAttackAttributeMatches(int attackAffectTeam,
                                                    int attackerSideField,
                                                    int projectileRole);

// What one player contact does to a projectile, §4. Hits always lose one; the
// projectile enters state 2 (Removal, `projremanim`) only when the hits are
// exhausted *and* `projremove` is set. Otherwise it survives and its pause is
// set from the attacking player's `pausetime` (`+0xE8`). There is no
// projectile-vs-player pending-hit record here: this contact is applied
// directly, in the same tick.
struct ProjectileAttackOutcome {
    int hitsLeft = 0;
    bool enterRemoval = false;
    int pause = 0;
};

[[nodiscard]] ProjectileAttackOutcome applyPlayerAttackToProjectile(int hitsLeft,
                                                                    bool removeOnHit,
                                                                    int attackPause);

// ---------------------------------------------------------------------------
// Projectile contact bookkeeping — `ProjHit` / `ProjGuarded` / `ProjContact`
//
// The reference keeps the *last* projectile contact on the owning player, not
// on the projectile: player `+0x25C` is the projectile id, `+0x260` the contact
// code and `+0x264` the frames since the contact (`PROJECTILE-COLLISION-RE.md`
// §1). That is why the triggers still work after the projectile slot has been
// freed. Two codes are proven by the binary: 1 for a projectile-vs-player hit
// and 3 for a projectile-vs-projectile cancellation. The guarded code is 2 by
// the trigger table's three-way split (`FINDINGS.md`, cases 62-64: `==1`,
// `==2`, `!=0`); the docs' `ProjGuarded` needs a distinct value and 2 is the
// only one left between the two proven codes.
//
// The player-attack pass (§4) also cancels a projectile, but the documented
// bookkeeping records no code for it. It is recorded as 3 (the cancel code)
// because that is the only cancellation code the reference exhibits; the
// mapping is an interpretation, not a proven binary write.
inline constexpr int kProjectileContactHit = 1;
inline constexpr int kProjectileContactGuarded = 2;
inline constexpr int kProjectileContactCancelled = 3;

// A code mask for `projectileContactTicks`, so a trigger can ask for one code
// (ProjHit, ProjGuarded) or two (ProjContact = hit or guarded).
[[nodiscard]] inline constexpr int projectileContactMask(int code) {
    return 1 << code;
}

struct ProjectileContactRecord {
    // False until the owner's first contact ever. The trigger answers false /
    // -1 rather than "0 ticks ago".
    bool active = false;
    // One of the `kProjectileContact*` values.
    int code = 0;
    // The contact projectile's own `id` (`proj ID`), for the trigger's filter.
    int id = 0;
    // Frames since the contact. The fight records a contact at 0 and the entity
    // layer advances every record once per tick, so the next tick's triggers see
    // 1 -- which is the docs' "just made contact" value.
    int ticks = 0;
};

// A new contact replaces the previous one, as the reference's single player
// triple does.
inline void recordProjectileContact(ProjectileContactRecord& record, int code, int id) {
    record.active = true;
    record.code = code;
    record.id = id;
    record.ticks = 0;
}

inline void advanceProjectileContact(ProjectileContactRecord& record) {
    // Saturate rather than overflow; a contact older than this is never
    // distinguished by any trigger in the corpus.
    if (record.active && record.ticks < 1000000000) ++record.ticks;
}

// The frames-since value a `Proj*Time` trigger returns, or -1 when the last
// contact does not match. `filterId <= 0` checks every projectile id: the docs
// treat an omitted suffix, `ProjX(0)` and a negative argument the same way.
// `codeMask` is one or more `projectileContactMask` bits.
[[nodiscard]] int projectileContactTicks(const ProjectileContactRecord& record,
                                         int codeMask, int filterId);

// Whether the matching contact happened on the tick just passed, which is the
// one-tick window of the boolean trigger forms (`ProjHit = 1`).
[[nodiscard]] inline bool projectileContactJustHappened(
    const ProjectileContactRecord& record, int codeMask, int filterId) {
    return projectileContactTicks(record, codeMask, filterId) == 1;
}

// Whether an axis position is past a bound and not moving back in, §6.2.
//
// `step` is the signed change the position will make this tick in the position's
// own axis, so a projectile outside the lower bound survives while `step > 0`
// and one outside the upper bound survives while `step < 0`. Zero is not moving
// back in, so it is removed. Strict comparisons mean a projectile exactly on a
// bound is still inside it.
[[nodiscard]] bool projectilePastAxisBound(float position, float step, float lower,
                                           float upper);

// The screen-edge test, §6.2. Position-only: unlike the axis bounds it has no
// velocity term.
[[nodiscard]] bool projectilePastScreenEdge(float x, float cameraMidX,
                                           float halfScreenWidth, float edgeBound);

// The animation a removal state selects, §5.3.
struct ProjectileRemovalAnim {
    int anim = -1;
    bool fromFightFx = false;
};

// State 2 uses `projremanim`; state 3 uses `projcancelanim`, falling back to
// `projremanim` and then `projhitanim`; state 4 uses `projhitanim`. An answer of
// -1 means the slot is freed on entry with no animation. `Flying` is not a
// removal state and answers -1.
[[nodiscard]] ProjectileRemovalAnim resolveProjectileRemovalAnim(
    ProjectileState state, int hitAnim, bool hitFromFightFx, int removeAnim,
    bool removeFromFightFx, int cancelAnim, bool cancelFromFightFx);

// The timing a recorded player contact asks of the projectile, §3.2. A hit that
// is not the last one reloads both; the last hit of a removable projectile
// schedules neither, because it is about to enter state 4.
struct ProjectilePendingTiming {
    int pause = 0;
    int cooldown = 0;
};

[[nodiscard]] ProjectilePendingTiming projectilePendingHitTiming(int hitsLeft,
                                                                bool removeOnHit,
                                                                int hitPause, int missTime);

// Applying the pending hit at the start of the next entity update, §3.2:
// `hits --`, then state 4 only when the hits are exhausted and `projremove` is
// set. Hits may go negative; the reference does not clamp.
struct ProjectileHitApplication {
    int hitsLeft = 0;
    bool enterHitState = false;
};

[[nodiscard]] ProjectileHitApplication applyProjectilePendingHit(int hitsLeft,
                                                                bool removeOnHit);

}  // namespace af

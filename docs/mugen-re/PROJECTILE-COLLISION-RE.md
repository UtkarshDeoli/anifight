# Projectile collision, scaling, bounds, and lifetime — MUGEN 1.0 behaviour notes

Read-only interoperability notes for the anifight engine. Scope: collision
rules, priority, scale handling, stage/height/edge bounds, and projectile
lifetime. Companion to `PROJECTILE-PARAMS-RE.md` (parser, spawn, storage,
defaults); this file extends it and does not restate its verified content.

Binary: `mugen.exe` (PE32 i386, image base `0x400000`),
SHA-256 `887d00a04e94cd8df394304822b2a36f956516aa60c113576fd8ba9d3f9a06c4`.

Method: scratch objdump disassembly plus targeted byte searches for
parameter strings. No decompiled text is reproduced here; descriptions are
behavioural, with addresses for re-verification. `rg` was unavailable; grep
was used throughout.

Confidence tags: **HIGH** (directly read, control flow unambiguous),
**MEDIUM** (read but one field/sign is inferred), **LOW** (hypothesis with
some support), **UNRESOLVED** (not located / identity not proven).

---

## 0. Answers in one screen

| # | Question | Answer | Conf. |
|---|----------|--------|-------|
| 1 | `projpriority` default and compare | Default **1**, stored raw (no clamp, no validation). Lower-priority projectile loses **one hit**; higher-priority projectile loses **one priority point**; equal priority: **both lose one hit and one priority point**. Loser enters state **3** (cancel) when its hits reach 0, plays `projcancelanim` (falls back to `projremanim`, then `projhitanim`); if that resolves to `-1` the slot is freed immediately. `projpriority = -1` has no special case — it is simply outranked by any priority `>= 0`. | HIGH |
| 2 | Does `projscale` scale the hit box? | **No.** `projscale` is consumed by the draw path only. Collision uses raw positions (+0x44/+0x48) and unscaled Clsn records. At spawn, `projscale` is multiplied by the character's localcoord factor (and optionally by the owner's draw-scale factors). `proj.doscale` storage is proven, but **the gate that combines it with `projscale` was not located**; see §7 and the correction note. | HIGH for draw-only; UNRESOLVED for `doscale` |
| 3 | Bounds defaults / removal | Docs: `projedgebound` 40/80/160, `projstagebound` 40/80/160, `projheightbound` low -240/-480/-960, high 1/2/4 (240p/480p/720p). Binary: these are scaled by the owner's localcoord factor at spawn and stored as **ints**; leaving a bound triggers state **2** (removal animation, `projremanim`), **not** state 4. If the resolved removal animation is `-1`, the slot is freed immediately with no animation. `projlowbound`/`projhighbound` **do not exist** in this binary; only the pair `projheightbound`. | HIGH for fields/state; MEDIUM for literal defaults |
| 4 | Projectile-vs-projectile conditions | **Both must be state 1.** `projhits = 0` **still collides** (there is no hits gate in this path) and is cancelled immediately because its hits are already <= 0. `projremove`, `projmisstime`, pause, and cooldown are **not** consulted. There is no `projmiss` parameter; the multi-hit delay is `projmisstime`, which is irrelevant to projectile-vs-projectile. | HIGH |
| 5 | Flight anim ends, `projremovetime = -1` | **Not removed.** State 1 has no animation-finished check and no free call. The animation stops on its last frame and the projectile persists until a bound, the edge, a player hit, or an explicit cancel removes it. The removal states (2–4) are the only place animation-end checks and slot frees exist. | HIGH |

---

## 1. Collision driver and call order

The per-tick collision driver occupies `0x46CB00`–`0x46CC89`. Call order
(all addresses are call sites; **HIGH** for order, **MEDIUM** for the exact
outer-loop guards):

| Order | Call | Purpose |
|-------|------|---------|
| 1 | `0x46CB56`–`0x46CB80` → `0x46F3D0` (both directions per player pair) | Player melee/attack vs the opponent's projectiles |
| 2 | `0x46CBB0`–`0x46CBEE` → `0x46F620` (both directions, gated by the target player's masks `+0x1058`/`+0x105C`) | Projectile vs player |
| 3 | `0x46CC06` → `0x46FD80` (per player) | Apply pending projectile-vs-player hits (decrement hits, set pause/cooldown, force state 4) |
| 4 | `0x46CC30` → `0x46FE00` (per player pair) | Projectile vs projectile |
| 5 | `0x46CC60`–`0x46CC74` → `0x46CCA0` (only pairs whose `+0x0C` player ids differ and are non-zero) | Not investigated; likely helper-vs-helper collision. **UNRESOLVED** |

Player list root is the global at `0x11438`; each player's projectile manager
is reached through `player+0x268`.

Collision bookkeeping fields on the owning player (seen in state writes):
`+0x25C` (projectile id), `+0x260` (contact state code), `+0x264`
(associated value, usually 0). Semantic names are **UNRESOLVED**; codes
observed: `1` on a projectile-vs-player hit, `3` on a projectile cancel
(state 3), plus the value 0 for the associated field.

---

## 2. Projectile vs projectile — `0x46FE00`

This function loops over all live projectiles of both managers. Manager data
(reached from `player+0x268`): `+0x04` slot stride (`0x2E0`), `+0x18` array
base, `+0x1C` occupancy bitmap base, `+0x20` iteration cursor, `+0x24` first
live index, `+0x28` live count, `+0x08` non-empty flag.

For every (A = outer player's projectile, B = inner player's projectile) pair:

### 2.1 Eligibility gates

| Gate | A | B | Conf. |
|------|---|---|-------|
| Attack Clsn list at `obj+0x298` → `+0x2C` non-null | required (`0x46FE42`, `0x46FE60`) | used as the test list (`0x46FF11`) | HIGH |
| State `+0x04 == 1` | required (`0x46FE68`) | required (`0x46FE92`) | HIGH |
| Attribute/side overlap | required (`0x46FE9B`–`0x46FEEF`) | symmetric | HIGH |
| Geometry overlap | required (`0x46FEF1`–`0x46FF31`) | symmetric | HIGH |
| `projhits` `+0x1C` | **no gate** | **no gate** | HIGH |
| `projremove` `+0x38` | **no gate** | **no gate** | HIGH |
| pause `+0x28`, cooldown `+0x2C`, `projmisstime` `+0x30` | **no gate** | **no gate** | HIGH |

Consequences, all **HIGH**:

- A state-2 (removal-animation) projectile cannot collide with another
  projectile; both must be state 1.
- A `projhits = 0` projectile still collides. Since its hits are already
  <= 0, the post-collision check immediately sends it to state 3 (cancel).
- A paused projectile (pause `+0x28 > 0`, still state 1) can collide,
  because this path never tests pause.
- A projectile in its post-hit cooldown (`+0x2C > 0`) can collide.
- `projremove` does not prevent cancellation. It is only consumed on player
  hits (state 4) and on player-attack destruction (state 2).

### 2.2 Attribute / side test

Each projectile carries a side/team role at `+0x08` and an attack attribute
mask at `+0x9C`. When `+0x08 == 2`, the low two bits of the attribute mask
are swapped before testing. A collision requires the masks to overlap in
either direction:

- test A.attr against B's role mask, or
- test B.attr against A's role mask.

If neither overlaps, the pair is skipped with no side effects (`0x46FEEF` →
`0x46FF37`, advancing B). Exact bit meanings (the two low bits, and the role
value 2) are **UNRESOLVED**; only the swap and overlap test are proven.

### 2.3 Geometry

Raw float positions are used, with no scale factor and no Clsn scaling:

- delta y = `B+0x48 - A+0x48`
- delta x = `B+0x44 - A+0x44`

Both deltas are converted to ints and passed with B's attack Clsn list to the
shared Clsn test at `0x40B360` (`0x46FEF1`–`0x46FF2C`). A facing-derived flag
is also passed (`+0x34 != 1`). The test result must be non-zero to continue
to the priority block. **HIGH** that no scale is applied; **MEDIUM** for the
exact meaning of the facing flag inside `0x40B360`.

### 2.4 Priority algorithm (exact)

Let `pri` = `+0x0C`, `hits` = `+0x1C`. For the ordered pair (A, B), the
binary performs (`0x46FF71`–`0x46FFEE`):

- If `A.pri <= B.pri`: `A.hits--`.
- If `B.pri <= A.pri`: `A.pri--` and `B.hits--`.
- If `A.pri <= B.pri`: `B.pri--`.

There is no clamp and no special case for negative values. Resulting cases:

| Case | A (outer) | B (inner) |
|------|-----------|-----------|
| `A.pri > B.pri` | priority -1 | hits -1 |
| `A.pri == B.pri` | hits -1, priority -1 | hits -1, priority -1 |
| `A.pri < B.pri` | hits -1 | priority -1 |

This is symmetric in effect: the lower-priority projectile loses a hit, the
higher-priority projectile loses a priority point, and equal priority makes
both lose a hit and a priority point. Default `projhits = 1` therefore means
the lower/equal projectile is always cancelled on first contact, while the
winner survives and is one priority point weaker. **HIGH**

`projpriority = -1` or omitted:

- Omitted: engine default is **1** (definition record `+0x88`, initializer
  verified in `PROJECTILE-PARAMS-RE.md`).
- `-1`: parsed and stored raw at object `+0x0C`; there is no validation
  branch and no clamp (`0x4813AD`). It behaves as an ordinary low priority.
  Two `-1` projectiles are equal and cancel each other. **HIGH**

### 2.5 Cancellation result

For each projectile independently, after the priority update
(`0x46FF83`–`0x46FFEE`):

- If `hits > 0`: set `pause +0x28 = pausetime` (embedded HitDef block
  `+0x94`, field `+0x54`, i.e. live `+0xE8`). The projectile survives.
- If `hits <= 0`: set state **3**, age `+0x24 = 0`, owner `+0x25C`-family
  bookkeeping: `+0x260 = 3`, `+0x264 = 0`. **No check of `projremove`.**

State 3 selects `projcancelanim` (`+0x18`) in the removal-state handler
(§5.3). With no animations configured this resolves to `-1` and the slot is
freed immediately; if `projremanim` was configured, it is used as the cancel
animation. This is the answer to “does the loser play its removal
animation”: it plays the **cancel** animation, which defaults through to
`projremanim` → `projhitanim` → none. **HIGH**

---

## 3. Projectile vs player — `0x46F620` + apply at `0x46FD80`

For each live projectile of the attacker, against each opponent player.

### 3.1 Gates

| Gate | Detail | Conf. |
|------|--------|-------|
| State | `(state - 1) <= 1` unsigned: states **1 and 2** are allowed (`0x46F6C8`) | HIGH |
| Pause | `+0x28 > 0` skips (`0x46F6D1`) | HIGH |
| Attribute | `+0x9C` tested against target `player+0xC`; role `+0x08 == 2` swaps low two bits (`0x46F6DB`–`0x46F6FF`) | HIGH |
| Extra masks | `live+0xA0 & player+0x1058 & player+0x105C` must have at least one low-3 bit and at least one higher bit set (`0x46F705`–`0x46F726`) | MEDIUM / field names UNRESOLVED |
| Cooldown | `+0x2C > 0` skips (`0x46F72C`) | HIGH |
| Geometry | target player boxes (`+0x210/+0x1F8`, `+0x20C`, …) vs raw projectile `+0x44/+0x48`, then Clsn test `0x40B360` (`0x46F736` ff.) | HIGH |
| `projhits` | `+0x1C <= 0` skips (`0x46F79C`) — **`projhits = 0` cannot hit players** | HIGH |
| Hit processing | `0x470030` called before the hits gate; its exact role is **UNRESOLVED** | MEDIUM |

State 2 projectiles **can still hit players**. This is a real behavioural
difference from projectile-vs-projectile: once a projectile enters the
removal animation, it remains dangerous to players until the slot is freed.

### 3.2 Pending-hit protocol

A successful contact does not apply the hit in `0x46F620`; it schedules it.
The pending fields are:

- `+0x2C8` = pending flag (set to 1)
- `+0x2CC` = pending pause
- `+0x2D0` = pending cooldown (misstime)

Values written at `0x46FCF3`–`0x46FD29`, using `hits - 1`:

| Condition | pending pause (`+0x2CC`) | pending cooldown (`+0x2D0`) |
|-----------|--------------------------|------------------------------|
| `hits - 1 > 0` (still has hits) | `+0xE8` (pausetime) | `+0x30` (projmisstime) |
| `hits - 1 <= 0` and `projremove == 0` | `+0xE8` | `+0x30` |
| `hits - 1 <= 0` and `projremove != 0` | 0 | 0 |

`0x46FD80` then applies pending hits per live projectile
(`0x46FDA1`–`0x46FDD5`):

1. `hits--`
2. cooldown `+0x2C = pending cooldown`
3. clear pending flag
4. pause `+0x28 = pending pause`
5. if `hits <= 0` **and** `projremove != 0` → state **4**, age `+0x24 = 0`
6. if `hits <= 0` and `projremove == 0` → projectile survives with hits 0
   (it can never hit a player again because of the `+0x1C` gate)

So on the final hit of a removable projectile, no pause/cooldown is applied
and the projectile switches to the hit animation (state 4). With
`projremove = 0` the final hit still applies the pending pause/cooldown but
the projectile is not removed by hits. **HIGH**

State 4 selects `projhitanim` (`+0x10`); `-1` → free immediately (§5.3).

---

## 4. Player attack vs projectile — `0x46F3D0`

The first collision pass iterates the opponent's projectile manager and
requires `state == 1`. It compares the attacking player's attribute mask
(`+0x11B8`) against the projectile's role `+0x8` (with the same low-two-bit
swap when the *player's* side field `+0xC == 2`), then performs Clsn geometry.
On contact (`0x46F5B2`–`0x46F5D7`):

- `hits--`
- if `hits <= 0` and `projremove +0x38 != 0` → state **2** (removal
  animation), age 0
- otherwise pause `+0x28 = +0xE8` (pausetime)

Note the asymmetry: a projectile destroyed by a player attack uses state
**2** (`projremanim`), while one cancelled by another projectile uses state
**3** (`projcancelanim`). **HIGH** for the state writes; **MEDIUM** for the
player-side attribute plumbing.

---

## 5. State machine and lifetime — `0x472DB0`

### 5.1 States

| State `+0x04` | Meaning | Selected animation |
|---------------|---------|--------------------|
| 1 | Flying | flight animation |
| 2 | Removal (bounds, edge, age, player attack) | `projremanim` `+0x14` |
| 3 | Cancelled (projectile vs projectile) | `projcancelanim` `+0x18` |
| 4 | Hit (player contact, `projremove != 0`) | `projhitanim` `+0x10` |

### 5.2 State 1 per-tick behaviour

- Pause: if `+0x28 != 0`, the tick bypasses movement, bounds, cooldown,
  removetime, and the age increment; a freeze sub-flag (`+0x2C4`) can also
  suppress the animation/pause tick. Exact freeze condition: **MEDIUM**.
- Movement (`0x472E5A`–`0x472EE0`): position += velocity; velocity *= velmul;
  velocity += accel. Facing `+0x34` becomes +1 if vx > 0, -1 if vx < 0, and
  is unchanged if vx == 0.
- Bounds: four axis tests (§6.2), then the screen-edge test.
- Cooldown: `+0x2C` decrements once per tick when positive.
- Age: `+0x3C` limit compared against `+0x24`; `-1` is a sentinel for
  “never”. Expiry → state 2.
- Animation: `0x401B90` advances the flight animation.
- Age and the auxiliary counter `+0x20` then increment together
  (`0x4731B1`–`0x4731B9`), but only when the tick was not paused.

**There is no animation-finished branch in the state-1 path.** The only free
calls in the whole update are in the removal-state handler. This is the
direct evidence for Q5: with `projremovetime = -1`, a projectile is never
removed merely because its flight animation ended.

### 5.3 Removal states 2–4 (`0x47302D`–`0x47315A`)

Entry from a bound/edge/age sets state 2 and age 0 in the same tick
(`0x47302D`, `0x473034`), then falls into the removal handler.

Each removal tick:

1. Position advances by the current velocity (`0x473056`–`0x473095`).
2. Immediate-free checks (`0x473098`–`0x4730B7`):
   - state 2 and `projremanim +0x14 == -1` → free
   - state 3 and `projcancelanim +0x18 == -1` → free
   - state 4 and `projhitanim +0x10 == -1` → free
3. On the first removal tick only (age == 0):
   - velocity = remvelocity (`+0x60/+0x64 ← +0x68/+0x6C`)
   - select the animation by state and run the animation-selection helper
     `0x401C80` (requested animation number passed in a register)
   - first animation-end check (`0x4730FD`): if the current element's
     `+0x8` time value is <= 0 → free
4. Every removal tick: second animation-end check (`0x47312E`): if the
   animation block's `+0x2A4` value equals the element's `+0x8` value →
   free.
5. Common tail (`0x472FFE`): copy sprite priority `+0x2C0 → +0x2BC`, advance
   the animation (`0x401B90`), decrement pause if any, increment age unless
   paused.

Observable consequences (**HIGH** unless marked):

- A removal/cancel/hit animation plays to completion, then the slot is
  freed.
- If the resolved animation number is `-1`, the slot is freed on the first
  removal tick — immediate deletion with no animation. This is the default
  case when a character defines no `projhitanim`/`projremanim`/
  `projcancelanim`.
- On the first removal tick the projectile moves one more step with its
  **flight** velocity; from the next tick it uses `remvelocity`. `velmul`
  and `accel` are not applied in removal states.
- State 1 never auto-frees at animation end (Q5).

The exact semantics of the animation block fields `+0x294` (index/current
element) and `+0x2A4` (value compared to element `+0x8`) are
**UNRESOLVED**, as is whether a looping removal animation can ever satisfy
the second check.

---

## 6. Bounds and edge

### 6.1 Live fields (ints)

| Offset | Meaning | Source at spawn |
|--------|---------|-----------------|
| `+0x80` | `projedgebound` | `ftol(localcoord × def+0x4C)` |
| `+0x84` | left stage bound | `half_stage_width − projstagebound + offsetX` |
| `+0x88` | `projheightbound` low (greatest height) | `ftol(localcoord × def+0x64)` |
| `+0x8C` | right stage bound | `half_stage_width + projstagebound + offsetY` |
| `+0x90` | `projheightbound` high | `ftol(localcoord × def+0x70)` |

Spawn computation (spawn `0x4818C0`–`0x481C00`; record base is `esp+0x20`
at the allocator call, so stack `0xa0/0xa4/0xa8/0xac/0xb0` map exactly to
record `+0x80/+0x84/+0x88/+0x8C/+0x90`). **HIGH**

- `half_stage_width` = integer half of the float global `0x11840`.
- `projstagebound` is read from the definition record `+0x58`, then scaled
  by the owner's localcoord factor; `projedgebound` from `+0x4C`, height
  bounds from `+0x64` and `+0x70`, all scaled the same way.
- The owner's localcoord factor comes from the character record (`+0xA4`
  of the character struct), with a helper/owner indirection through
  `player+0xC3C`, `player+0xC38` and the global table `0x11230`.
- Offset terms use globals `0x1b98`/`0x1b9c` (ints) multiplied by the float
  global `0x1b74`. The exact sign of each offset in the left/right formulas
  is **MEDIUM**; the left bound is a half-width-minus-bound expression and
  the right bound a half-width-plus-bound expression.

The four axis bounds are **game/stage coordinates**. The edge bound is
**screen-relative**, using the float global `0x10ef8` as the screen x
offset and `0x11840` for the half width. **HIGH** for the fields used;
**MEDIUM** for the exact combined expression.

### 6.2 Removal conditions in the update

Each axis check is a compound test, not a simple “inside/outside” test: the
position is compared against the bound and, if outside, the velocity is also
examined (`0x472F00`, `0x472F1E`, `0x472F3E`, `0x472F5E`). A projectile that
is outside a bound but travelling back toward the valid region is kept.
Exact FPU flag/sign mapping is **MEDIUM**, but the velocity involvement is
clear from the paired `fcoms` on `+0x60`/`+0x64`.

The screen-edge test (`0x472F7E`–`0x472FE0`) uses `+0x80`, the half width
`0x11840`, and the screen offset `0x10ef8`; unlike the axis bounds it has no
velocity term.

Any triggered check jumps to `0x47302D`: state **2**, age 0 — the animated
removal path, not state 4. Therefore:

- `projremanim = -1` (effective default) → immediate free, no animation.
- `projremanim` defined → the removal animation plays and the slot frees at
  its end.

### 6.3 No `projlowbound` / `projhighbound`

Byte search of the binary finds only the `projheightbound` parameter string.
`projlowbound` and `projhighbound` do not exist as separate parameters; the
pair is `projheightbound = lowbound, highbound`. **HIGH**

### 6.4 Defaults — docs vs binary

Docs (`sctrls.html`): `projedgebound` and `projstagebound` default to
40/80/160 in 240p/480p/720p; `projheightbound` low defaults to -240/-480/-960
and high to 1/2/4. The binary does not store these literals in the examined
spawn/update paths; it scales whatever value the parameter expression
produces by the owner's localcoord factor. The 40→80→160 and
-240→-480→-960 families are consistent with that scaling if the defaults
live in the controller's default expressions (not located in this pass).
Literal-default confidence: **MEDIUM**. Field, scaling, and state-transition
confidence: **HIGH**.

---

## 7. Scale: `projscale` and `proj.doscale`

### 7.1 Spawn math

`projscale` x/y are float parameters at definition `+0x154`/`+0x160`
(defaults 1.0, per `PROJECTILE-PARAMS-RE.md`). At spawn
(`0x4816FF`–`0x48178C`):

- If `player+0x98 != 0`: multiply by `player+0x7C` (x) and `player+0x80`
  (y), then by the character localcoord factor (`character+0xA4`, with the
  helper indirection).
- Otherwise: multiply by `localcoord × 1.0`, using the helper at `0x465250`.

The results are stored at live `+0x50` (x) and `+0x54` (y). **HIGH** for the
computation and storage; **UNRESOLVED** for the identity of `player+0x98`
(behaviourally it selects whether the owner's own scale factors
`+0x7C/+0x80` are applied).

### 7.2 Draw vs collision

- Draw (`0x4734A0`, and the alternate path around `0x4736AB`): horizontal
  scale = `+0x50 × +0x4C`, vertical = `+0x54 × +0x4C`.
- `+0x4C` is reset to 1.0 every frame by the second update
  (`0x473230`–`0x473235`) for states 1–4. What (if anything) sets it to a
  non-1.0 value between updates is **UNRESOLVED**.
- Collision (`0x46FE00`, `0x46F620`, `0x46F3D0`) reads raw `+0x44/+0x48` and
  passes only int deltas plus Clsn records to `0x40B360`. No scale field is
  read or passed. Therefore `projscale` changes the **sprite only**, not the
  hit box. **HIGH** for the paths examined.

### 7.3 `proj.doscale`

- Parsed as an int in the character-constants record at `+0x308` (parser
  xref `0x467A3A`–`0x467A48`). **HIGH**
- No consumer of the constants `+0x308` field was located in the spawn,
  update, draw, or any of the three collision functions. The gate that
  combines it with `projscale` is **UNRESOLVED**. The MUGEN docs only say the
  final scale is “affected by both” parameters; they do not define the
  combination.

**Correction to `PROJECTILE-PARAMS-RE.md` §4.2.** That note tentatively
identifies a copy at `0x47FF30` (`source+0x308` → active block `+0x148`) as
`proj.doscale`. Re-examination shows `0x47F380`'s source argument is a
controller-specific **HitDef parameter block** (the projectile spawn passes
`definition+0x184`, and the ordinary HitDef controller passes
`controller_block+0x60`), not the character-constants record. The store to
`+0x148` is real, but the numeric coincidence of offset `0x308` does not
prove it is `proj.doscale`. Treat the identity as unsupported, and the
`doscale` gate as unresolved. (Not edited in the companion file, per the
read-only deliverable rule.)

---

## 8. Docs vs binary — differences and confirmations

| Topic | MUGEN 1.0 docs claim | Binary behaviour | Verdict |
|-------|----------------------|------------------|---------|
| `projpriority` equal | “they will cancel” | both hits-- and both priorities--; with default `projhits = 1` both enter state 3 | Confirmed |
| `projpriority` higher | higher cancels lower, higher priority decreases by 1 | lower hits--; higher priority-- | Confirmed exactly |
| `projpriority` negative | not specified | no validation/clamp; ordinary low value | Binary detail |
| `projscale` | affects final scale, jointly with `proj.doscale` | applied only to drawing; collision unscaled; localcoord-scaled at spawn; `doscale` consumer not found | Docs silent on hit boxes; draw-only is a binary finding |
| `projedgebound`/`projstagebound` | distances off screen/stage edge; 40/80/160 | screen/stage-relative int bounds; state 2 removal; velocity-aware for axis bounds | Confirmed (literal defaults MEDIUM) |
| `projheightbound` | least/greatest y; lowbound is greatest height | stored as two ints `+0x88`/`+0x90`; same state-2 removal | Confirmed |
| `projlowbound`/`projhighbound` | occasionally referenced by users/derivatives | **absent from the binary** | Not parameters here |
| `projremanim` | animation “when removed (time/boundaries/etc.)” | state 2 anim; `-1` → immediate free | Confirmed |
| `projcancelanim` | animation “when cancelled by hitting another projectile” | state 3 anim on projectile-vs-projectile; `projremove` ignored | Confirmed |
| `projremove` | removed after it hits; 0 disables | gates state 4 on player hits and state 2 on player-attack destruction; does **not** gate state 3 cancel or bound removal | Confirmed, with scope precision |
| `projhits` | number of hits before removal | gates player hits (`>0` required); **does not** gate projectile-vs-projectile | Binary detail |
| `projmisstime` | re-hit delay for multi-hit projectiles | cooldown `+0x2C` reloaded from `+0x30` after each applied hit; irrelevant to proj-vs-proj | Confirmed |
| `projremovetime` | -1 means never | sentinel comparison; age/limit check; state 2 on expiry | Confirmed |
| Flight anim end | docs do not say it removes the projectile | state 1 has no anim-end check; `projremovetime = -1` persists indefinitely | Binary finding |
| `projmiss` | no such parameter in docs | no such parameter in binary | N/A |

---

## 9. Explicit UNRESOLVED items

1. Identity of `player+0x98` (selects whether owner scale factors
   `+0x7C/+0x80` multiply `projscale`).
2. Consumer/gate of `proj.doscale` (constants `+0x308`); the `+0x148` copy
   is a HitDef-block field, not proven to be `doscale`.
3. What sets live `+0x4C` to a value other than 1.0 (draw multiplier).
4. Exact FPU sign conventions and the role of globals `0x11840`, `0x10ef8`,
   `0x1b98`, `0x1b9c`, `0x1b74` in the bound/edge expressions.
5. Exact semantics of attribute bits, the side/role value 2, and the
   low-two-bit swap.
6. Semantics of player `+0x25C`, `+0x260`, `+0x264` contact bookkeeping and
   of the `0x470030` hit-processing call.
7. Purpose of `0x46CCA0` (called for player pairs with differing non-zero
   `+0x0C` ids).
8. Animation-block fields `+0x294` (index) and `+0x2A4` (compared against
   element `+0x8`), and whether looping removal animations can finish.
9. Literal default expressions for the three bound parameters (scaling
   behaviour is proven).
10. Freeze sub-flag `+0x2C4` semantics during pause.

---

## 10. Engine changes implied

1. **Separate cancellation path.** Projectile-vs-projectile must be its own
   collision pass requiring both projectiles in the flying state, with
   state “cancelled” distinct from both “hit” and “removal”. It must ignore
   `projremove`, `projmisstime`, pause, and cooldown, and must use
   `projcancelanim` (falling back to `projremanim`, then `projhitanim`).
2. **Exact priority rule.** Lower priority loses one hit; higher loses one
   priority point; equal loses one of each on both sides. No clamping;
   negative priorities allowed. Default priority 1.
3. **`projhits = 0` semantics.** Such a projectile still collides with other
   projectiles and cancels immediately, but must never hit players (gate on
   `hits > 0` in the player-hit pass). Do not free `projhits = 0`
   projectiles at spawn.
4. **State-2 projectiles remain dangerous.** The player-hit pass must accept
   states 1 and 2, while the projectile-vs-projectile pass accepts only
   state 1.
5. **Pending-hit model.** Apply hits on the following tick: decrement hits,
   reload the re-hit cooldown from `projmisstime`, set the pause from the
   HitDef pausetime, and force the hit/removal state only when hits are
   exhausted and `projremove != 0`.
6. **Bounds are axis-wise and velocity-aware.** Four stage/height bounds plus
   a separate screen-edge bound; an out-of-bounds projectile moving back in
   survives. All are computed at spawn from localcoord-scaled parameters,
   with the stage half-width from the stage width.
7. **Removal is animated, not instant.** Bound, edge, age, player-attack,
   and cancel removals select the corresponding animation; only a `-1`
   animation causes immediate deletion. Removal states keep advancing
   position: one step with flight velocity, then with `remvelocity`.
8. **No animation-end removal in flight.** Never free a flying projectile
   because its animation ended. With `projremovetime = -1` it must persist
   (frozen on its last frame) until another removal cause applies.
9. **Draw-only scaling.** Apply `projscale` to sprite drawing (times the
   localcoord factor and, when the owner flag is present, the owner's scale
   factors); never to Clsn/hit boxes. Collision uses raw positions and
   unscaled Clsn.
10. **No `projlowbound`/`projhighbound`.** Implement a single
    `projheightbound` pair.

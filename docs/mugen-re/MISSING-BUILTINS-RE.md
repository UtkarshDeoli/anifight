# Missing built-ins: common states 5000-5999 vs `kBuiltins`

Behavior notes for the anifight engine. Date: 2026-10-19.

Sources, all local:

- `app/src/main/cpp/engine/statemachine.cpp` (the built-in table, its clocks, `land()`)
- `app/src/main/cpp/engine/statemachine.h` (`CommonState`, `HitInfo`)
- `app/src/main/cpp/engine/character.h` (`[Data]` defaults, movement/velocity constants)
- `local_mugen/extracted/mugen/data/common1.cns` (stock MUGEN 1.0 common state file; read for semantics only, not copied in)
- `local_mugen/extracted/mugen/docs/cns.html`, `trigger.html`, `sctrls.html`
- `local_mugen/extracted/mugen/chars/kfm/kfm.cns` (a shipped character's `[Data]`)
- `docs/mugen-re/FINDINGS.md` (prior passes; cited by section)

Line numbers below refer to the snapshot read for this note. Quotes are short and attributed to the file and state/controller that writes them.

**Correction to the brief.** The table does *not* have a "timing column". `BuiltinState` (`statemachine.cpp:330-341`) is:

```
int number; StateType type; MoveType moveType; Physics physics; bool ctrl; int anim; int thenState;
```

The comment above it (`statemachine.cpp:316-326`) says so in as many words: "What the table has no column for is *timing*, and that is its one known shortcoming." Timing today is encoded outside the row, in `builtinExitsOnShake()`, `builtinAwaitsGround()`, the `5110` liedown special case, and `animationFinished()`. FINDINGS pass 14 section 14.4 records the absence, not a column. Any proposal below is therefore an addition, not a correction of an existing field.

**Headline findings.**

- The only 5000-5999 states with no `kBuiltins` row are **5035**, **5500** and **5900**.
- States outside that range with no row: **45, 106, 120, 132, 140, 170, 175, 191**. Some are deliberate (120/132/140 are never entered for a file-less character); some are not (45, 106, 191).
- The existing **190** row disagrees with `common1.cns`: the reference's 190 changes to 191 at `Time = 0`, and 191 changes to 0 at `Time = 0`; the engine row holds at 190 (`thenState = -1`).
- **5101** has a row but is unreachable: `land()` bounces directly and routes the second arrival at 5100 to 5110.
- The fall states use `character->gravity()` (`movement.yaccel`, default 0.44) as their gravity, while the reference's 5030/5035/5050/5071/5200 use `GetHitVar(yaccel)` (HitDef `yaccel`, default 0.35; `lastHit_.yaccel` is stored and reported).
- `airjuggle` is **not** a reaction timer. It is a juggle-point pool (`character.h:444`, restore in `land()`), and `[Data] liedown.time` (default 60) is the only reaction-relevant `[Data]` clock, for 5110.

---

## 0. Master table, states 5000-5999

Header notation: `T/M/P` = `type` / `movetype` / `physics`; `C` = `ctrl`; `anim`. "Omitted" means the statedef does not write it. Per `cns.html` the omitted `movetype` defaults to `I` and the omitted `physics` defaults to `N`. "Exit clock" is the clock that actually ends the *hold* in the reference file, not every controller that can redirect it.

| State | Name | Ref header | Entry route (reference / engine) | Exit clock in `common1.cns` | Engine status |
|---|---|---|---|---|---|
| 5000 | Stand get-hit (shaking) | S/H/N, `velset = 0,0` | ground hit, victim standing (`hitInfoFor` ground branch, `statemachine.cpp:5225`) | `HitShakeOver`; split on `GetHitVar(yvel) = 0 && !GetHitVar(fall)` | row; shake clock; physics/type/anim differ |
| 5001 | Stand get-hit (knocked back) | S/H/S | 5000 flat exit | `HitOver` -> 0, `ctrl = 1` | row; anim-driven + stun gate; slide differs |
| 5010 | Crouch get-hit (shaking) | C/H/N, `velset = 0,0` | ground hit, victim crouching, no `forcestand` (`statemachine.cpp:5225-5227`) | `HitShakeOver`; same split -> 5011 / 5030 | row; shake clock; physics/anim differ |
| 5011 | Crouch get-hit (knocked back) | C/H/C | 5010 flat exit | `HitOver` -> 11, `ctrl = 1` | row; anim-driven + stun gate; slide differs |
| 5020 | Air get-hit (shaking) | A/H/N, `velset = 0,0` | air hit, `air.type != 3` (`statemachine.cpp:5136-5137`) | `HitShakeOver` -> 5030 | row; shake clock; physics/anim differ |
| 5030 | Air get-hit (knocked away) | A/H/N, `ctrl = 0` | 5000/5010 launch exit, 5020, 5080 airborne exit | `HitOver` or ground -> 5040/5050 by `HitFall`; `AnimTime = 0` -> 5035 | row; split exists; skips 5035; gravity source differs |
| 5035 | Air get-hit (transition) | A/H/N | 5030 `AnimTime = 0` | `HitOver`, `AnimTime = 0`, ground, or `Time = 0 && Anim != 5035` -> 5040/5050 | **MISSING** |
| 5040 | Air get-hit (recovering in air, not falling) | A/H/N | 5030/5035 non-fall exit | ground -> 52; `HitOver` sets `CtrlSet 1` and `movetype = I`; `!Alive` -> 5050 | row; land -> 52/5100; no ctrl/movetype switch |
| 5050 | Air get-hit (falling) | A/H/N | 5030/5035 fall exit; 5040 death (ref) | ground -> 5100; recovery command -> 5200/5210 | row; `land()` + recovery; gravity source differs |
| 5070 | Tripped get-hit (shaking) | A/H/N, `velset = 0,0` | `ground.type = 3` or `air.type = 3` (`statemachine.cpp:5137, 5228`) | `HitShakeOver` -> 5071 | row; shake clock |
| 5071 | Tripped get-hit (knocked away) | A/H/N | 5070 | ground (`Pos Y >= movement.air.gethit.trip.groundlevel`, 15) -> 5110 | row, but `land()` sends it to **5100** |
| 5080 | Downed get-hit (shaking) | L/H/N, `velset = 0,0` | hit while `stateType == Liedown` (`statemachine.cpp:5273`) | `HitShakeOver`; split on `GetHitVar(yvel)` -> 5081 / 5030 | row; split exists; physics differs |
| 5081 | Downed get-hit (knocked back) | L/H/C | 5080 flat exit | `HitOver` -> 5110 | row; anim-driven + stun gate; friction differs |
| 5100 | Downed get-hit (hit ground from fall) | L/H/N | 5050 ground; 5071 ground (ref); bounce arrival | `Time = 0 && GetHitVar(fall.yvel) = 0` -> 5110; else `AnimTime = 0` -> 5101 | row; bounce handled in `land()`; 5101 unreachable |
| 5101 | Downed get-hit (bounce off ground) | L/H/N | 5100 `AnimTime = 0` with non-zero fall yvel | ground (`Pos Y >= movement.down.bounce.groundlevel`, 12) -> 5110 | row present, **never entered** |
| 5110 | Downed get-hit (lying down) | L/H/N | 5071/5100/5101/5081 | alive: engine-special (`recovertime`); `!alive` + anim/sysvar -> 5150 | row; `liedownTicks_` correct; physics/death timing differ |
| 5120 | Downed get-hit (getting up) | L/I/N | 5110 when `recovertime` expires | `AnimTime = 0` -> 0, `ctrl = 1` | row; anim clock; no invuln windows |
| 5150 | Downed get-hit (lying defeated) | L/H/N, `sprpriority = -3`, `ctrl = 0` | 5110 dead exit | none | row; no exit; no sprpriority field |
| 5200 | Air get-hit (fall recovery on ground; still falling) | A/H/N | 5050 recovery near ground | ground (`Pos Y >= movement.air.gethit.groundrecover.groundlevel`, 10) -> 5201 | row; `land()` -> 5201; gravity source differs |
| 5201 | Air get-hit (fall recovery on ground) | A/H/A, `anim = 5200` | 5200 | none (`physics = A` lands it) | row; entry state supplied in `enterState`; lands to 52 |
| 5210 | Air get-hit (fall recovery in air) | A/I/N, `anim = 5210`, `ctrl = 0` | 5050 recovery in air | ground (`Vel Y > 0 && Pos Y >= 0`) -> 52; `CtrlSet` at `Time = 20` | row; roll hard-coded; uses commented 2002 values |
| 5500 | Continue screen animation | S/omitted(N)/omitted(I), `ctrl = 0`, `anim = 5300`, `velset = 0,0` | continue screen after a loss | none (held) | **MISSING**; engine has no continue screen |
| 5900 | Initialize (at the start of the round) | S/omitted(N)/omitted(I) | round start, before state 0 | `Time = 0` -> 190 if `roundno = 1`, else 0 | **MISSING**; engine resets and enters 0 directly |

### 0.1 Missing rows outside 5000-5999

| State | Name | Ref header | Exit clock | Engine status |
|---|---|---|---|---|
| 45 | Air jump start | A/omitted(I)/N, `ctrl = 0`, `velset = 0,0` | `Time = 2` -> 50, `ctrl = 1` | missing; no air-jump state anywhere |
| 106 | Hop backwards (land) | S/omitted(I)/S, `ctrl = 0`, `anim = 47` | `Time = 7` -> 0, `ctrl = 1` | missing; engine 105 goes straight to 0 |
| 120 | Guard (start) | U/U (leave unchanged) | `AnimTime = 0` -> 130/131/132; stop guarding -> 140 | missing, deliberate (engine never enters 120 for a file-less fighter) |
| 132 | Air guard (guarding) | A/omitted(I)/N | landing -> 130 if still guarding, else 52; stop guarding -> 140 | missing, deliberate |
| 140 | Guard (end) | U/U, `ctrl = 1` | exit "implemented within engine" (the file's `ChangeState` is commented out) | missing, deliberate |
| 170 | Lose (time over) | S/omitted(I)/S, `ctrl = 0`, `anim = 170` | none / round end; `NotHitBy SCA` | missing; round end is engine-side |
| 175 | Draw game (time over) | S/omitted(I)/S, `ctrl = 0` | `Time = 0 && !SelfAnimExist(175)` -> 170; otherwise none | missing; round end is engine-side |
| 190 | Pre-intro | S/omitted(N)/omitted(I), `ctrl = 0`, `velset = 0,0` | `Time = 0` -> 191 | row exists but `thenState = -1`; anim forced |
| 191 | Intro (override point) | S/omitted(N)/omitted(I), `ctrl = 0` | `Time = 0` -> 0 | **MISSING** |

Engine rows with no counterpart in `common1.cns`: **5**, **102**, **180**. They are engine aliases/poses, not substitutes for a reference state; do not remove them on account of this note.

---

## 1. How the engine times a built-in today

The reference's reaction chain has four independent clocks. The engine has three and derives the fourth.

1. **Shake clock** - `shakeTicks_`. Seeded from the HitDef's `pausetime` second value (`statemachine.cpp:5324, 5370`). `GetHitVar(hitshaketime)` reports it (`6225`); `hitshakeover` is `shakeTicks_ == 0` (`6134`). `builtinExitsOnShake()` lists the rows that end on it: 150, 152, 154, 5000, 5010, 5020, 5070, 5080.
2. **Hitstun clock** - `stunTicks_`. Seeded from the resolved hit time (`5404`, `5519`) and decremented only when not in hitpause (`1810`), so shake and hittime are sequential, not concurrent (FINDINGS 19.1). `hitover` is `stunTicks_ < 0 && pauseTicks_ == 0` (`6135`); `GetHitVar(hittime)` reports `stunTicks_` (`6182`). `HitOver` is therefore one tick after the number reaches 0, matching `trigger.html`: "`GetHitVar(hittime) < 0` is equivalent to the HitOver trigger".
3. **Animation clock** - `animationFinished()` is `currentAnimTime() == 0` (`4750`); `currentAnimTime()` returns 0 for a null animation, an empty animation, or a held (`duration = -1`) frame (`4698-4704`). This is the clock for every generic row: `holdOver = ... : animationFinished()` (`1192`).
4. **Liedown clock** - `liedownTicks_`, from `[Data] liedown.time` (`character.h:209, 445`), reported by `GetHitVar(recovertime)` (`6125`), decremented only in `kDowned` with `moveType == Hit` (`1902-1915`). This is the engine-special exit of 5110.

The stun gate is the piece that changes what "animation clock" means for a file-less fighter. `runCurrentState()` (`statemachine.cpp:2014-2039`) runs the built-in table only when `stunTicks_ < 0`, except for the shake states. So every non-shake built-in hand-over is *at least* `HitOver`, even if `animationFinished()` is true on entry. For an animation-less fighter the two clocks coincide at `HitOver`; for a fighter that has art but no `common1.cns`, the hand-over is `max(hitstun, animation length)`, which is not the reference's rule.

The reference often takes the *earliest* of its triggers. 5030 is the clear example: `[State 5030, 6] ChangeState / trigger1 = AnimTime = 0 / value = 5035` fires during hitstun. The engine explicitly chooses not to be early (`statemachine.cpp:2018-2025`: "What it cannot be is *early*"). Implementing 5035 faithfully therefore needs a fourth clock class that is allowed to run while `stunTicks_ >= 0`.

`[Data] airjuggle` does not end any reaction. It is the juggle-point pool (`character.h:444`, default 15), spent by `applyHit` and restored on landing (`land()`, `statemachine.cpp:4469`). The only `[Data]` value that is a reaction timer is `liedown.time`.

Reference clock semantics for the triggers, quoted from `docs/trigger.html`:

- `HitShakeOver`: "returns 1 if the hit shake (the period when he is shaking in place) has ended".
- `HitOver`: "returns 1 when the hittime has expired".
- `GetHitVar(hitshaketime)`: "counts down by 1 for each game tick, and stops when it reaches zero".
- `GetHitVar(hittime)`: "counts down by 1 per game tick ... stops counting down when the value reaches -1".
- `GetHitVar(recovertime)`: "Returns time before player gets up from liedown state ... counts down to 0"; the reference also says it "will count down faster if buttons are hit".

The engine implements no button-mash acceleration of `liedownTicks_`; see section 5.

The hit clocks' sources, quoted from `docs/cns.html`:

- `pausetime`: "The first argument is the time to freeze P1 ... The second is the time to make P2 shake before recoiling from the hit."
- `ground.hittime`: "Time that P2 stays in the hit state after being hit."

---

## 2. Per-state notes

### 5000 - Stand get-hit (shaking)

Header (`common1.cns:873`): `type = S`, `movetype = H`, `physics = N`, `velset = 0,0`; `ctrl` omitted.

Entry: `hitInfoFor()` ground branch chooses 5000 for a standing victim (`statemachine.cpp:5225`). A launch still enters 5000; the reference's `[State 5000, 5] StateTypeSet` turns the victim aerial at `Time = 0` when `GetHitVar(yvel) != 0 || GetHitVar(fall)` or `Pos Y != 0`.

Controllers: dynamic `ChangeAnim` (`5000 + animtype`, or `5010 + animtype` for a Low groundtype, `5030` for Back, `5047 + animtype` for Up/DiagUp when the art exists), anim freeze on later ticks, ForceFeedback, and two exits.

Exit, verbatim (`[State 5000, 6]` and `[State 5000, 7]`):

```
trigger1 = HitShakeOver
trigger1 = GetHitVar(yvel) = 0 && !GetHitVar(fall)
value = 5001
```

```
trigger1 = HitShakeOver
value = 5030
```

Clock: `HitShakeOver` only.

Engine: row `{5000, Stand, Hit, Stand, false, 5000, 5001}`. The shake clock is correct, and `runBuiltinState` overrides 5001 with 5030 when `hitFallFlag_` (`1218-1246`). Disagreements: physics is `Stand` rather than `N` (masked by hitpause: `step()` returns before physics while `pauseTicks_ > 0`); the state type never becomes `A` for a launch; the animation is always 5000 rather than the animtype-selected art.

File-less behavior: hold in 5000 for `victimPause` ticks, then hand to 5001 or 5030. That is exactly the reference for a victim with no art, and the row needs no change except the clock metadata.

### 5001 - Stand get-hit (knocked back)

Header (`common1.cns:949`): `type = S`, `movetype = H`, `physics = S`.

Entry: 5000 flat exit.

Controllers: `HitVelSet x = 1` at `Time = 0`; `ChangeAnim` to `5005 + animtype + (groundtype = 2)*10` when `AnimTime = 0`; `VelMul x = .6` at `Time >= GetHitVar(slidetime)`; at `HitOver`, `VelSet x = 0`, `DefenceMulSet 1`, `ChangeState 0 ctrl = 1`.

Exit clock: `HitOver`.

Engine: row `{5001, Stand, Hit, Stand, false, 5001, 0}`. The hand-over is `animationFinished()` behind the stun gate, so for no art it is `HitOver`, which is right; for art it can be later. The slide is not the reference's: `Physics::Stand` applies `kStandFriction = 0.85` every tick from the first (`statemachine.cpp:32, 4501-4508`), where the reference holds velocity constant until `slidetime` and then multiplies once by .6. Anim and `DefenceMulSet` are not modelled.

File-less behavior: exit to 0 at `HitOver`; no row change needed for the exit, but the slide physics is a disagreement.

### 5010 - Crouch get-hit (shaking)

Header (`common1.cns:987`): `type = C`, `movetype = H`, `physics = N`, `velset = 0,0`.

Entry: ground hit on a crouching victim without `forcestand` (`statemachine.cpp:5225-5227`).

Exits: same two-controller shape as 5000, to 5011 (flat) or 5030 (launch).

Clock: `HitShakeOver`.

Engine: row `{5010, Crouch, Hit, Crouch, false, 5010, 5011}`. Same notes as 5000: physics substitution is harmless under hitpause; anim is fixed at 5010 where the reference uses `5020 + animtype`.

### 5011 - Crouch get-hit (knocked back)

Header (`common1.cns:1059`): `type = C`, `movetype = H`, `physics = C`.

Exit: `HitOver` -> `value = 11`, `ctrl = 1`.

Engine: row `{5011, Crouch, Hit, Crouch, false, 5011, kCrouch}`. The reference's `11` hand-over is correctly represented. Same slide/anim caveats as 5001; `kCrouchFriction = 0.82` applies every tick.

### 5020 - Air get-hit (shaking)

Header (`common1.cns:1097`): `type = A`, `movetype = H`, `physics = N`, `velset = 0,0`.

Entry: air hit whose `air.type != 3` (`statemachine.cpp:5136-5137`). (`air.type = 3` goes to 5070.)

Exit (`[State 5020, 5]`): `trigger1 = HitShakeOver` -> `value = 5030`.

Clock: `HitShakeOver`.

Engine: row `{5020, Air, Hit, Air, false, 5020, 5030}`. Physics substitution is harmless under hitpause. The animation differs: the reference picks `ifelse(GetHitVar(airtype) = 1, 5000, 5010) + GetHitVar(animtype)`, or 5030 for Back, or `5047 + animtype` for Up/DiagUp.

### 5030 - Air get-hit (knocked away)

Header (`common1.cns:1161`): `type = A`, `movetype = H`, `physics = N`, `ctrl = 0`.

Entry: launch exits of 5000/5010, the 5020 shake exit, and the airborne exit of 5080.

Controllers: `ChangeAnim 5030` only when the current anim is outside `[5000,5199]` and `SelfAnimExist(5030)`; `VelAdd y = GetHitVar(yaccel)` every tick; `HitVelSet x = 1, y = 1` at `Time = 0`.

Exit (`[State 5030, 4]`-`[6]`), verbatim-as-notes:

- `triggerall = !HitFall`, `trigger1 = HitOver`, or `trigger2 = Vel Y > 0` and `Pos Y >= Const(movement.air.gethit.groundlevel)` -> `5040`.
- `triggerall = HitFall`, `trigger1 = HitOver`, or ground -> `5050`.
- `trigger1 = AnimTime = 0` -> `5035`.

Clock: earliest of `HitOver`, ground, and `AnimTime = 0`.

Engine: row `{5030, Air, Hit, Air, false, 5030, -1}` plus the special case at `1076-1082`: on `animationFinished()`, `hitFallFlag_ ? 5050 : 5040`. Ground contact is `land()`'s (`4538-4556`). Disagreements:

- The 5035 route is absent.
- For a fighter with art, hand-over can be later than `HitOver` because the table is stun-gated; the reference leaves at `HitOver` regardless.
- Gravity is `movement.yaccel` (0.44), not `GetHitVar(yaccel)` (0.35 default or the HitDef's number).
- The animation is forced to 5030 rather than only switched when the current anim is outside 5000-5199.

File-less behavior: with no 5030 art, the engine enters, holds for the remainder of hitstun, then splits to 5040/5050 at `HitOver`, or `land()` takes it. That is the best available approximation; see section 5 for what the reference does with an animation it cannot draw.

### 5035 - Air get-hit (transition) - MISSING

Header (`common1.cns:1207`): `type = A`, `movetype = H`, `physics = N`; no `ctrl`, no `anim`.

Entry: only `[State 5030, 6]`, `trigger1 = AnimTime = 0` -> `value = 5035`.

Controllers: `ChangeAnim 5035` at `Time = 0` only if `SelfAnimExist(5035)`, the current anim is not `[5051,5059]`, and it is not 5090; `VelAdd y = GetHitVar(yaccel)` every tick.

Exit (`[State 5035, 3]`/`[4]`): for `!HitFall` -> 5040, for `HitFall` -> 5050, on any of:

- `trigger1 = HitOver`;
- `trigger2 = AnimTime = 0`;
- `trigger3 = Vel Y > 0` and `Pos Y >= Const(movement.air.gethit.groundlevel)`;
- `trigger4 = Time = 0` and `Anim != 5035` (no transition art was found).

Clock: earliest of `HitOver`, animation end, ground, or "no transition art" on the entry tick.

Engine: no row, and no route to the number. `land()` has no case for it. A file-less character's correct net behavior is what the engine already produces (5030 -> 5040/5050), because with no 5035 art `trigger4` fires on entry. Implementing it is only observable for a character that has a 5035 animation, and then only if animation-driven exits are allowed before `HitOver`.

### 5040 - Air get-hit (recovering in air, not falling)

Header (`common1.cns:1249`): `type = A`, `movetype = H`, `physics = N`.

Entry: 5030 or 5035 non-fall exit.

Controllers: `ChangeAnim` to 5040 when the transition anim ends; `VelAdd y = GetHitVar(yaccel)` every tick; at `HitOver`, `CtrlSet 1` and `StateTypeSet movetype = I`.

Exit (`[State 5040, 1]`, `[6]`): `!Alive` -> 5050 (`ctrl = 0`); `Vel Y > 0` and `Pos Y >= 0` -> 52.

Clock: ground contact. `HitOver` is not an exit; it is a control/movetype switch.

Engine: row `{5040, Air, Hit, Air, false, 5040, -1}`. `land()` sends it to 52 alive and to **5100** when dead (`statemachine.cpp:4584-4586`). The reference sends a dead victim to **5050** (the fall, which then lands at 5100). The engine never applies the `HitOver` `CtrlSet`/`movetype = I`, so a file-less victim remains `Hit` and uncontrollable until it touches the floor, while the reference gives control back at `HitOver` in mid-air.

### 5050 - Air get-hit (falling)

Header (`common1.cns:1291`): `type = A`, `movetype = H`, `physics = N`.

Entry: 5030/5035 fall exit; the reference's 5040 death exit.

Controllers: transition/coming-down `ChangeAnim`, `VelAdd y = GetHitVar(yaccel)` every tick.

Exit: near-ground recovery command -> 5200 (`Vel Y > 0`, `Pos Y >= movement.air.gethit.groundrecover.ground.threshold`, `CanRecover`, `Command = "recovery"`); mid-air recovery -> 5210 (`Vel Y > movement.air.gethit.airrecover.threshold`); otherwise ground (`Vel Y > 0`, `Pos Y >=` 0 or `movement.air.gethit.groundlevel` depending on anim) -> 5100.

Clock: ground (recovery is input, not a clock).

Engine: row `{5050, Air, Hit, Air, false, 5050, 5100}`; `tryFallRecover()` for the two recovery routes (`1088-1090`); `land()` -> 5100. Net behavior matches; gravity source and animation selection differ.

### 5070 - Tripped get-hit (shaking)

Header (`common1.cns:1344`): `type = A`, `movetype = H`, `physics = N`, `velset = 0,0`.

Entry: `ground.type = 3` on the ground or `air.type = 3` in the air (`statemachine.cpp:5137, 5228`).

Exit (`[State 5070, 2]`): `trigger1 = HitShakeOver` -> `value = 5071`.

Clock: `HitShakeOver`.

Engine: row `{5070, Air, Hit, None, false, 5070, 5071}`. Matches the header and clock. The reference's special-handling note at the top of `common1.cns` says defense increases at `Time = 0`; the engine has no defence multiplier for this state.

### 5071 - Tripped get-hit (knocked away)

Header (`common1.cns:1369`): `type = A`, `movetype = H`, `physics = N`.

Controllers: `HitVelSet x = 1, y = 1`; `VelAdd y = GetHitVar(yaccel)`.

Exit (`[State 5071, 3]`): `Vel Y > 0` and `Pos Y >= Const(movement.air.gethit.trip.groundlevel)` (default 15) -> `value = 5110`.

Clock: ground, at the trip's own threshold.

Engine: row `{5071, Air, Hit, Air, false, 5071, 5110}`, but `land()` routes `kHitTripFall` to **5100** (`statemachine.cpp:4577-4581`), and `land()` triggers at `y <= 0`, five units before the reference's `trip.groundlevel` of 15 (the reference measures from the char's position downward; the engine's floor is 0). The row's `thenState = 5110` is unreachable because the physics landing changes state first. Net destination differs (5100 vs 5110), which skips 5110's `Time = 0` `HitFallDamage`/`VelSet y = 0`/`PosSet y = 0` and instead uses 5100's bounce path.

### 5080 - Downed get-hit (shaking)

Header (`common1.cns:1393`): `type = L`, `movetype = H`, `physics = N`, `velset = 0,0`.

Entry: hit while `stateType == Liedown` (`statemachine.cpp:5273`).

Exit (`[State 5080, 4]`/`[5]`): `HitShakeOver` + `GetHitVar(yvel) = 0` -> 5081; `HitShakeOver` + `GetHitVar(yvel) != 0` -> 5030.

Clock: `HitShakeOver`.

Engine: row `{5080, Liedown, Hit, Stand, false, 5080, -1}` plus the special case at `1084-1088`, which uses `hitFallFlag_` to pick 5030/5081. The split matches. Physics `Stand` is a substitute for `N`; under hitpause it does not tick.

### 5081 - Downed get-hit (knocked back)

Header (`common1.cns:1437`): `type = L`, `movetype = H`, `physics = C`.

Exit (`[State 5081, 4]`): `trigger1 = HitOver` -> `value = 5110`.

Clock: `HitOver`.

Engine: row `{5081, Liedown, Hit, Stand, false, 5081, 5110}`. Clock is `animationFinished()` behind the stun gate (so `HitOver` for no art). Friction is `kStandFriction` (0.85) rather than the reference's `physics = C` (0.82); the reference also does `HitVelSet x = 1` and `VelSet x = 0` at `HitOver`.

### 5100 - Downed get-hit (hit ground from fall)

Header (`common1.cns:1464`): `type = L`, `movetype = H`, `physics = N`.

Entry: 5050 ground; 5071 ground in the reference; the bounce's second arrival.

Controllers: `PosSet y = 0`, `VelSet y = 0`, `VelMul x = .75`, `PosFreeze` every tick, `HitFallDamage` at `Time = 3`, `FallEnvShake`, Explod/Snd, `HitFallDamage`.

Exit (`[State 5100, 7]`/`[12]`): at `Time = 0`, `GetHitVar(fall.yvel) = 0` -> 5110; otherwise `AnimTime = 0` -> 5101.

Clock: `Time = 0` when there is no fall velocity, else the animation.

Engine: row `{5100, Liedown, Hit, Air, false, 5100, 5110}`. `land()` applies fall damage/shake and, when `fallVelY_ > 0`, bounces immediately and re-enters 5100 on the next contact (`4492-4523`). So the engine bounces on the contact tick, where the reference holds 5100 for its animation length and then hands to 5101. The engine's second 5100 arrival routes to 5110 (`statemachine.cpp:4595-4597`), which is the reference's net end point but through the wrong state.

### 5101 - Downed get-hit (bounce off ground) - unreachable

Header (`common1.cns:1554`): `type = L`, `movetype = H`, `physics = N`.

Entry: only 5100 with `GetHitVar(fall.yvel) != 0` at `AnimTime = 0`.

Controllers: `HitFallVel` at `Time = 0` (sets velocity to the HitDef's `fall.xvel`/`fall.yvel`), `PosSet y = movement.down.bounce.offset.y` (default 20), `PosAdd x = offset.x` (default 0), `VelAdd y = movement.down.bounce.yaccel` (default 0.4).

Exit: `Vel Y > 0` and `Pos Y >= movement.down.bounce.groundlevel` (default 12) -> 5110.

Clock: ground.

Engine: row `{5101, Liedown, Hit, Air, false, 5101, 5110}` exists, but nothing changes to 5101 and `land()` never targets it. `builtinAwaitsGround` lists it, which is harmless. Either wire the 5100 -> 5101 route and move the bounce there, or delete the row so the table does not claim behavior it does not have.

### 5110 - Downed get-hit (lying down)

Header (`common1.cns:1600`): `type = L`, `movetype = H`, `physics = N`.

Entry: 5071/5100/5101/5081.

Controllers: `FallEnvShake`, `HitFallDamage`, `PosSet y = 0`, `VelSet y = 0`, `VelMul x = .85`, `VelSet x = 0` below `movement.down.friction.threshold`, anim selection, and one exit:

```
[State 5110, 11] ;If just died
triggerall = !alive
trigger1 = AnimTime = 0
trigger2 = SysVar(0)
trigger3 = Anim = [5110,5119]
value = 5150
```

There is **no alive exit in the file**. The top-of-file comment says: "Hit_liedown (5110): player changes to get-up state (5120) when ready." That is special handling in MUGEN, and the clock is `GetHitVar(recovertime)`, i.e. `[Data] liedown.time`.

Clock: the liedown clock (`liedown.time`, default 60) for the alive exit; `AnimTime = 0` / `SysVar(0)` / anim range for the dead exit.

Engine: row `{5110, Liedown, Hit, Stand, false, 5110, 5120}` and the special case at `1166-1182`: `life_ <= 0` -> 5150 immediately; `liedownTicks_ <= 0` -> re-arm and 5120. The liedown clock is correct. The death route is early: the reference waits for the lying animation to reach its dead frame (or `SysVar(0)`, set when 5081 wants to skip that frame); the engine leaves on the first tick of 5110. For a no-art character `AnimTime` is already 0, so the reference is also immediate in that case. Physics `Stand` vs `N`.

### 5120 - Downed get-hit (getting up)

Header (`common1.cns:1726`): `type = L`, `movetype = I`, `physics = N`.

Controllers: `VelSet x = 0`; `NotHitBy SCA` every tick; at `AnimTime = 0`, `HitFallSet 1`, `NotHitBy , NT,ST,HT` for 12 ticks, `NotHitBy SCA` for 3 ticks, and `ChangeState 0 ctrl = 1`.

Clock: `AnimTime = 0`.

Engine: row `{5120, Liedown, Idle, Stand, false, 5120, 0}`. Clock matches (and for no art it is immediate, like the reference with no art). The `NotHitBy` windows are not modelled, so a file-less character is hittable during get-up where the reference gives it 12 throw-invulnerable and 3 full-invulnerable ticks after the animation ends.

### 5150 - Downed get-hit (lying defeated)

Header (`common1.cns:1781`): `type = L`, `movetype = H`, `physics = N`, `sprpriority = -3`, `ctrl = 0`.

Controllers: anim selection only; no `ChangeState`.

Clock: none.

Engine: row `{5150, Liedown, Hit, None, false, 5150, -1}`. Correctly has no exit. `sprpriority` has no field in `BuiltinState`; the row cannot express it. `changeState` refuses idle targets when `life_ <= 0` (`statemachine.cpp:1636-1643`), which protects this state.

### 5200 - Air get-hit (fall recovery on ground; still falling)

Header (`common1.cns:1838`): `type = A`, `movetype = H`, `physics = N`.

Entry: 5050 recovery near the ground.

Controllers: `ChangeAnim` out of 5035 when it ends; `VelAdd y = GetHitVar(yaccel)`.

Exit (`[State 5200, 3]`): `Vel Y > 0` and `Pos Y >= Const(movement.air.gethit.groundrecover.groundlevel)` (default 10) -> `SelfState 5201`.

Clock: ground at the recovery threshold.

Engine: row `{5200, Air, Hit, Air, false, 5200, kHitFallRecoverGround}`; `land()` routes `kHitFallRecover` to 5201 (`statemachine.cpp:4592-4593`). The engine lands at `y <= 0`, ten units earlier than the reference's threshold; its own comment acknowledges this. Gravity source differs.

### 5201 - Air get-hit (fall recovery on ground)

Header (`common1.cns:1862`): `type = A`, `movetype = H`, `physics = A`, `anim = 5200`.

Entry: 5200.

Controllers (all `Time = 0`): `Turn` if `p2dist X < -20`; `VelSet` to `velocity.air.gethit.groundrecover.x/y` (defaults `-0.15, -3.5`); `PosSet y = 0`; `NotHitBy SCA` each tick; `PalFX` white for 3 ticks; `Explod F60`.

Exit: none. `physics = A` lands it; the reference gives no `ChangeState` at all.

Engine: row `{5201, Air, Hit, Air, false, 5200, -1}`. `enterState` supplies the turn/hop/`PosSet` (`1590-1607`), and the hop is `worldX(-0.15)`, `velY_ = 3.5`, matching the default pair. `land()` -> 52. PalFX/Explod/NotHitBy are not modelled, which is cosmetic apart from the brief invulnerability.

### 5210 - Air get-hit (fall recovery in air)

Header (`common1.cns:1905`): `type = A`, `movetype = I`, `physics = N`, `anim = 5210`, `ctrl = 0`.

Entry: 5050 mid-air recovery command.

Controllers: `PalFX`; `PosFreeze Time < 4`; `Turn` if `p2dist X < -80`; at `Time = 4`, live 1.0 controllers:

```
type = VelMul
trigger1 = Time = 4
x = Const(velocity.air.gethit.airrecover.mul.x)   ; default 0.5
y = Const(velocity.air.gethit.airrecover.mul.y)   ; default 0.2

type = VelAdd
trigger1 = Time = 4
x = Const(velocity.air.gethit.airrecover.add.x)   ; default 0
y = Const(velocity.air.gethit.airrecover.add.y)   ; default -4.5
```

then stick-steered adds (`up` default -2, `down` 1.5, `fwd` 0, `back` -1); `NotHitBy SCA` for 15 ticks; `CtrlSet Time = 20`; `VelAdd y = movement.air.gethit.airrecover.yaccel` (0.35) from `Time >= 4`; ground (`Vel Y > 0`, `Pos Y >= 0`) -> 52, `ctrl = 1`.

Clock: ground; `CtrlSet` is at a fixed `Time = 20`.

Engine: row `{5210, Air, Idle, None, false, 5210, kLand}` plus the hard-coded roll (`1091-1139`): `PosFreeze` for `roll < 4`; at `roll == 4`, `velX_ *= 0.8`, `velY_ *= 0.8`, `velY_ += 4.5`, then `if velY_ < 0: *= 0.5`, `if velY_ < 3: += 2`, `if velY_ < 2: += 1`, stick nudges; gravity `.35` per tick after that; `land()` -> 52.

Disagreements:

- `.8/.8` and the three conditional nudges are the **commented-out** "2002.04.14 behavior" block in `common1.cns` (`type = null;VelMul`, etc.), not the live 1.0 controllers, which use the `airrecover.mul` constants 0.5/0.2. The engine comment at `1107-1110` calls these "the reference's own numbers"; the shipped 1.0 file says otherwise.
- The stick adds are hard-coded to the default constants instead of reading `velocity.air.gethit.airrecover.*` from the character (the defaults happen to match: up +2, down -1.5, back -1, fwd 0).
- `CtrlSet` at `Time = 20` is absent; control returns only on landing.
- `NotHitBy SCA` for 15 ticks is absent; the fighter is hittable during the roll.
- `land()` routes to 52 (correct).

### 5500 - Continue screen animation - MISSING

Header (`common1.cns:2018`): `type = S`; `ctrl = 0`; `anim = 5300`; `velset = 0,0`. `movetype` and `physics` are omitted, so they default to `I` and `N`.

Entry: the continue screen. The reference's own docs list it as "Continue screen animation" (`cns.html` common-state table). It is not reachable from a move or a hit.

Controller:

```
[State 5500, 1] ;Use continue animation if available
type = ChangeAnim
trigger1 = time = 0
trigger1 = SelfAnimExist(5500)
value = 5500
```

Exit: none in the file. The state holds until the engine leaves the continue screen.

Clock: none.

Engine: no row; no continue screen exists anywhere under `app/src/main/cpp/engine/` (grep for "continue" finds only C++ control flow). A row alone would be inert.

File-less behavior: enter 5500 with anim 5300, `ctrl = 0`, at rest, and hold. If the character has anim 5500, use it instead of 5300. Do not auto-exit on `animationFinished()`; `thenState = -1`.

### 5900 - Initialize (at the start of the round) - MISSING

Header (`common1.cns:2032`): `type = S` only. `movetype` defaults to `I`, `physics` to `N`, `ctrl` is omitted and therefore unchanged.

Entry: start of every round, before state 0 (MUGEN's round initialization).

Controllers:

- `VarRangeSet value = 0` (all int vars) when `roundsexisted = 0`;
- `VarRangeSet fvalue = 0` (all float vars) when `roundsexisted = 0`;
- `RemapPal source = 1,1 dest = 1,palno` every entry;
- `ChangeState value = 190` when `roundno = 1`;
- `ChangeState value = 0` otherwise (`trigger1 = 1`).

Exit clock: immediate. Both exits carry `trigger1 = 1` relative to their condition, so 5900 lasts a single tick and exists only to run its initialization controllers. There is no animation and no wait.

Engine: no row. Round initialization today happens in `StateMachine::reset()` (`statemachine.cpp:1318-1363`): all int/float/sys vars are cleared, and the machine enters state 0 directly (`enterState(CommonState::kStand)` at `1362`). `roundNo_` exists (`statemachine.h:1756`, `setRoundNo`), and `roundsexisted` is computed as `roundNo_ - 1` (`statemachine.cpp:6056`). `FightScreen::startRound()` calls `setup()` **before** `setRoundNo(round_)` (`fight_screen.cpp:323-327`), so any 5900 run from `setup()` would see the stale/default round number. Palette remapping is handled outside the state machine (`applyChosenPalettes()` at `fight_screen.cpp:330`).

File-less behavior: run the three initialization effects and hand over to 190 in round 1, otherwise 0, on the same tick. Since the engine already clears vars and sets palettes outside the state machine, a strict row can be added for parity without changing observable behavior for a file-less character, provided the round number is correct. The one behavioral difference is that the engine clears vars **every** round, while the reference clears only when `roundsexisted = 0` (round 1), so character variables otherwise persist across rounds.

### 190 / 191 - Pre-intro and Intro (relevant to 5900)

These are not in 5000-5999 but 5900's round-1 route depends on them, and the engine's 190 row is wrong.

`common1.cns:845`, `[Statedef 190]`: `type = S`, `ctrl = 0`, `velset = 0,0`; `ChangeAnim 190` if `SelfAnimExist(190)`; `ChangeState value = 191` at `Time = 0`.

`common1.cns:862`, `[Statedef 191]`: `type = S`, `ctrl = 0`; `ChangeState value = 0` at `Time = 0`. This is the character's intro override point: a character that wants an intro defines its own `[Statedef 191]`.

Engine: row `{190, Stand, Idle, Stand, false, 190, -1}`. It never routes to 191, and it forces anim 190 even when the character has no such animation. With `thenState = -1` and no controllers, a file-less fighter that ever entered 190 would hold there forever. The engine's round intro is instead a `FightScreen` phase (`RoundIntro`), not a state.

File-less behavior: 190 sets anim 190 only if it exists, then immediately hands to 191, which immediately hands to 0. With no art and no override, the two states last one tick and state 0 is the result.

---

## 3. Disagreements between existing rows and `common1.cns`

Severity: **H** = visible behavior, **M** = timing/velocity difference, **L** = cosmetic or masked.

| # | State(s) | Engine | `common1.cns` | Severity |
|---|---|---|---|---|
| D1 | table | no timing column; clocks encoded in helper functions | per-state triggers are the clocks | H (structural) |
| D2 | 5035 | row exists (:594) and the route is supplied | `AnimTime = 0` entry from 5030; exits on HitOver/anim/ground/no-art | H (fixed) |
| D3 | 5500 | inert row present; continue screen is engine-side | continue-screen hold | M (unreachable today) |
| D4 | 5900 | no state row; `setup()` clears vars (5900's `VarRangeSet`) and enters state 0 with `prevstateno = 5900`; the screen plays 190/191 as round phases | per-round init; vars cleared only when `roundsexisted = 0`; -> 190/0 | H (deliberate simplification, §13 of GAMEPLAY-PARITY) |
| D5 | 190/191 | rows present (:608–609); 190 -> 191 -> 0, `Time = 0` | 190 -> 191 -> 0, both `Time = 0` | H (fixed) |
| D6 | 45 | row present (:474); unreachable until air jump exists | air-jump takeoff, `Time = 2` -> 50 | H (air jump absent for file-less) |
| D7 | 106 | 105 -> 106 (fixed 7 ticks) -> 0 (:582) | 105 -> 106 (fixed 7 ticks) -> 0 | M (fixed) |
| D8 | 120/132/140 | no rows | guard start/hold/end | L (documented deliberate) |
| D9 | 170/175 | no rows | lose/draw poses | L (round end engine-side) |
| D10 | 5071 | ground -> 5110 | ground -> 5110 | H (fixed) |
| D11 | 5040 dead | `!Alive` -> 5050 (via the dead default) | `!Alive` -> 5050 | M (fixed) |
| D12 | 5040 at HitOver | `CtrlSet 1`, `movetype = I` (case 5040 in `runBuiltinState`) | `CtrlSet 1`, `movetype = I` | H (fixed) |
| D13 | 5101 | 5100 holds, then 5101 does the bounce; the engine still bounces at contact in `land()` | 5100 holds, then 5101 does the bounce | M (simplified: contact-time bounce) |
| D14 | 5030/5035/5050/5071/5200 | `GetHitVar(yaccel)` pull in `stepPhysics` | `VelAdd y = GetHitVar(yaccel)` (0.35 default or HitDef value) | M/H (fixed) |
| D15 | 5001/5011/5081 | slide model: free until `slidetime`, then `VelMul x = .6`; 5081 crouch friction | constant slide until `slidetime`, then one `VelMul x = .6`, then `x = 0` at HitOver | M (fixed) |
| D16 | 5000/5010/5020/5030 | anim chosen from `animtype`, `groundtype`/`airtype`, and `SelfAnimExist` | anim chosen from `animtype`, `groundtype`/`airtype`, and `SelfAnimExist` | M (fixed) |
| D17 | 5000/5010 launches | `StateType::Air` at `Time = 0` when yvel/fall/Pos Y | `StateTypeSet statetype = a` at `Time = 0` when yvel/fall/Pos Y | L (fixed) |
| D18 | 5000/5010/5020/5080 shake physics | Stand/Crouch/Air instead of N | `physics = N`, `velset = 0,0` | L (deliberate: pause freezes physics; speed persists through shake) |
| D19 | 5080/5081 | physics N / C | N / C | L/M (fixed) |
| D20 | 5110 | physics N; dead exit gated like the reference | physics N; dead exit waits for anim/SysVar | L/M (fixed) |
| D21 | 5110/5120 | 5120 raises `NotHitBy` 12/3 | 5120 `NotHitBy` 12/3 ticks | M (fixed) |
| D22 | 5150 | `sprpriority = -3` | `sprpriority = -3` | L (fixed) |
| D23 | 5100 | bounce at contact, no hold | HitFallDamage at `Time = 3`; bounce only after the anim | M (simplified: contact-time bounce) |
| D24 | 5210 | live `.5/.2` + add -4.5; CtrlSet 20; NotHitBy 15 | live 1.0 `.5/.2` + add -4.5; CtrlSet 20; NotHitBy 15 | H (fixed) |
| D25 | 190 | `ChangeAnim` only if `SelfAnimExist(190)` | `ChangeAnim` only if `SelfAnimExist(190)` | L (fixed) |
| D26 | 5001/5011 | `slidetime` model present | both present | M (fixed) |
| D27 | non-shake built-ins | `HitOver` is unconditional | `HitOver` is unconditional in 5001/5011/5030/5035/5081 | M (fixed) |

Notes on rows that look like disagreements but are intentional substitutions, not bugs: `Physics::Air` on 5030/5050/5071/5100/5101/5200/5201 replaces the reference's `VelAdd y = GetHitVar(yaccel)` controllers; `Physics::None` on 5070/5150/5210 matches `physics = N`; `MoveType::Idle` on 5120 and 5210 matches the reference's `movetype = I`.

---

## 4. Proposed changes

### 4.1 A clock field instead of `thenState`-only

Add an explicit clock to `BuiltinState` (this is the "timing column" the brief expected) and use it in `runBuiltinState` instead of inferring from helper lists:

```
enum class BuiltinClock { Shake, HitOver, Ground, Animation, Liedown, Immediate, None };
```

Proposed mapping for the reaction states:

| Clock | States |
|---|---|
| `Shake` | 150, 152, 154, 5000, 5010, 5020, 5070, 5080 |
| `HitOver` | 5001, 5011, 5030, 5035, 5081 |
| `Ground` | 5050, 5071, 5100 (non-zero fall velocity), 5101, 5200, 5201, 5210 |
| `Animation` | 5120, 106, 45 (`Time = 2`), 10, 12, 40, 52 |
| `Liedown` | 5110 |
| `Immediate` | 5900, 191, 190 |
| `None` | 5150, 5500, 5201 (lands by physics) |

The reaction states with *multiple* exits need the earliest-trigger rule, not a single enum: 5030 = min(HitOver, ground, AnimTime=0); 5035 = min(HitOver, ground, AnimTime=0, no-art-at-Time-0); 5040 = ground (HitOver is a state switch); 5050 = ground (recovery is input). Encode those as per-state special cases with the clock used only for the fallback.

If reference parity is wanted, the stun gate must permit the animation-driven exits of 5030/5035 before `HitOver`. Today it cannot. That is the single largest structural obstacle to a faithful 5035.

### 4.2 Rows to add or fix (exact initializers)

`BuiltinState` field order: `{number, type, moveType, physics, ctrl, anim, thenState}`.

Add:

```cpp
// 5035: transition. Physics Air substitutes the file's VelAdd y = GetHitVar(yaccel).
{5035, StateType::Air, MoveType::Hit, Physics::Air, false, 5035, -1},

// 5500: continue screen. No exit; the row is inert until a continue screen exists.
// anim 5300 is the statedef's own default; use 5500 when the character has it.
{5500, StateType::Stand, MoveType::Idle, Physics::None, false, 5300, -1},

// 5900: round init. The hand-over is 190 in round 1 and 0 otherwise; special-case it.
{5900, StateType::Stand, MoveType::Idle, Physics::None, false, -1, -1},

// 45: air jump takeoff. Special-case Time = 2 velocity and ctrl = 1.
{45, StateType::Air, MoveType::Idle, Physics::Air, false, 45, 50},

// 106: hop-back landing, fixed 7-tick hold.
{106, StateType::Stand, MoveType::Idle, Physics::Stand, false, 47, 0},

// 191: intro override point. One tick unless the character defines its own 191.
{191, StateType::Stand, MoveType::Idle, Physics::None, false, -1, 0},
```

Fix:

```cpp
// 190: the reference changes to 191 at Time = 0; only set anim 190 when it exists.
{190, StateType::Stand, MoveType::Idle, Physics::None, false, -1, 191},

// 5030: route through 5035 so a character with transition art sees it. If 5035
// is not implemented, leave thenState = -1 and keep the existing split.
{5030, StateType::Air, MoveType::Hit, Physics::Air, false, 5030, 5035},
```

State numbers 120/132/140, 170/175, 5/102/180: leave alone unless the guard/intro/round-end systems change. 120/132/140 are deliberately unreachable for file-less fighters; 170/175 belong to the round-end screen; 5/102/180 are engine aliases with no reference header.

### 4.3 Special cases to add to `runBuiltinState`

- **5035**: on the hold clock (after the stun gate, so effectively `HitOver` for a no-art fighter), `changeState(hitFallFlag_ ? kHitFall : kHitAirRecover, false)`. Also treat `stateTime_ == 0` and no animation 5035 as an immediate hand-over, which is the reference's `trigger4`. This can be folded into the existing 5030 special case by routing 5030 -> 5035 first; `runCurrentState` loops within the tick, so a no-art fighter still reaches 5040/5050 on the same tick.
- **`land()` 5035**: add a case so a transition that touches the floor goes to `hitFallFlag_ ? kHitGround : kLand` (the reference would go 5040/5050 and then land; landing directly to 52 is the same destination for the non-fall case).
- **5900**: at entry (same tick), run the var clear when `roundNo_ - 1 == 0`, apply the palette if the engine exposes it, then `changeState(roundNo_ == 1 ? 190 : 0, false)`. Call it after `setRoundNo()`; `FightScreen::startRound()` currently calls `setup()` first (`fight_screen.cpp:323-327`), so either reorder those two lines or move the 5900 entry into a `startRoundState()` invoked after `setRoundNo`.
- **5500**: in the builtin `enterState` path, choose anim 5500 when the character has it, else 5300. Do not hand over on `animationFinished()`.
- **190**: choose anim 190 only when the character has it; always hand to 191 on the entry tick. **191**: hand to 0 on the entry tick unless the character defines its own 191 (which takes the from-file path anyway).
- **5071**: change `land()`'s `kHitTripFall` target from `kHitGround` to `kDowned` (5110) to match `[State 5071, 3]`.
- **5040 dead**: change `land()`'s `kHitAirRecover` dead target from `kHitGround` (5100) to `kHitFall` (5050). This also gives the death the fall state's gravity and landing rules.
- **5040 at HitOver**: in the builtin hold, when `stunTicks_ < 0` and still in 5040 airborne, set `moveType_ = MoveType::Idle` and `ctrl_ = true` once, matching `[State 5040, 3]`/`[4]`.
- **Fall gravity**: for built-in fall states, use `lastHit_.yaccel` instead of `yAccel_` (`gravity()`); `yAccel_` is still right for a character's own `Gravity` controller. The states are 5030, 5035, 5050, 5071, 5200 (and 5210 keeps its own `movement.air.gethit.airrecover.yaccel` of .35).
- **5210**: replace the `.8/.8` + 2002 nudges with the live 1.0 values read from the character: `mul.x = 0.5`, `mul.y = 0.2`, `add.x = 0`, `add.y = -4.5` (engine signs: y up-positive, so `velY_ += 4.5`), then the stick adds from `velocity.air.gethit.airrecover.{up,down,fwd,back}`. Add `ctrl_ = true` at `stateTime_ == 20` and `NotHitBy SCA` for 15 ticks if the hit-filter model supports it.
- **5101** (optional): either implement the 5100 -> 5101 hold-then-bounce route, or remove the row and the `builtinAwaitsGround` case so the table stops claiming it.

### 4.4 Row-by-row proposed values for the three requested states

| State | type | movetype | physics | ctrl | anim | thenState | hand-over rule |
|---|---|---|---|---|---|---|---|
| 5035 | Air | Hit | Air | false | 5035 | -1 (special) | `hitFallFlag_ ? 5050 : 5040`, on `HitOver`/ground/animation/no-art |
| 5500 | Stand | Idle | None | false | 5300 (5500 if present) | -1 | none |
| 5900 | Stand | Idle | None | false (header omits it) | -1 | -1 (special) | `roundNo == 1 ? 190 : 0` on the entry tick |

---

## 5. What could not be established

- **The reference's exact duration for any state that uses `AnimTime`.** A duration is a property of the character's animation, not of `common1.cns`. For a character with no art there is no duration at all (`currentAnimTime()` returns 0 for a null animation). The table cannot carry a frame count that is correct for every character; it can only carry a clock selector and delegate the length to the AIR.
- **Whether a file-less character's missing hit animation should count as "animation ended".** The reference uses whatever animation the victim is currently in (5000/5010/5020 art, or the previous hit art) and enters 5035 when that ends. The engine forces the row's anim number, so "no 5030 art" is indistinguishable from "a zero-length animation". The stun gate currently hides this by deferring to `HitOver`; a faithful 5035 needs the carried-over animation to be modelled.
- **Contact-frame alignment to +/-1 tick.** FINDINGS 19.2 records that the exact tick on which `HitOver` becomes true relative to the contact frame is "still not established". All clocks here are stated in engine ticks as observed; the relationship to MUGEN's render frame is not proven.
- **Button-mash acceleration of `recovertime`.** `trigger.html` says the liedown timer "will count down faster if buttons are hit". The engine's `liedownTicks_` decrements one per tick with no input term, and no address for the acceleration was established here.
- **Continue-screen behavior.** The engine has no continue screen; there is no evidence in this repo for when 5500 should be entered or how it should be left. The header and hold behavior above are all that the reference file establishes.
- **Round initialization ordering in the reference.** The file proves 5900 runs at round start and routes to 190/0, but the exact point at which MUGEN enters it relative to `roundno`/`roundsexisted` being readable, palette application, and state -1/-2 execution was not established from the binary in this pass. The engine's `setup()` -> `setRoundNo()` order is documented above; reordering is a proposal, not a proven requirement.
- **5101's practical reachability in MUGEN.** The file makes it reachable only from 5100 with a non-zero `GetHitVar(fall.yvel)` at `AnimTime = 0`; whether MUGEN also enters it from engine-special bounce paths was not established. The engine's `land()` bounce is a substitute with the same net end state (5110).
- **`sprpriority = -3` for 5150 and `DefenceMulSet`/defence increases.** The engine's built-in table has no field for sprite priority and no defence multiplier was located for the special states (5070, 5100, 5110, 5120, 5200, 5210 all have their defence restored/set in MUGEN's engine). This pass did not establish whether anifight models defence at all, so those references are recorded as unmodelled rather than as bugs.
- **Rows 5, 102 and 180.** They are not in `common1.cns`; their origin is engine history, not the reference. No conclusion is drawn about whether they should exist.

---

## Appendix A. Reference state line index (`data/common1.cns`)

0:31, 10:61, 11:83, 12:101, 20:113, 40:144, 45:188, 50:233, 51:257, 52:267, 100:301, 105:329, 106:355, 120:390, 130:439, 131:461, 132:483, 140:532, 150:576, 151:612, 152:654, 153:690, 154:732, 155:756, 170:807, 175:821, 190:845, 191:862, 5000:873, 5001:949, 5010:987, 5011:1059, 5020:1097, 5030:1161, 5035:1207, 5040:1249, 5050:1291, 5070:1344, 5071:1369, 5080:1393, 5081:1437, 5100:1464, 5101:1554, 5110:1600, 5120:1726, 5150:1781, 5200:1838, 5201:1862, 5210:1905, 5500:2018, 5900:2032.

All line numbers are the `[Statedef]` lines from `grep -n "\[Statedef"` on this snapshot.

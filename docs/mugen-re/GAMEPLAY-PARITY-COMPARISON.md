# Gameplay parity comparison — the anifight engine against reverse-engineered MUGEN 1.0

Research document. It is the full comparison of **all fighting/gameplay logic** between this
repo's engine (`app/src/main/cpp/engine/`) and the decompiled MUGEN 1.0 reference
(`docs/mugen-re/`), assembled from the session's direct code reads plus the RE doc set.
Every claim carries a `file:line` citation on the side it describes.

Written: 2026-10-19.

Sources consulted:

| Side | Source | Role |
|------|--------|------|
| Reference | `docs/mugen-re/FINDINGS.md` (2,842 lines) | canonical extracted semantics, passes 1–17 |
| Reference | `docs/mugen-re/decompiled/` (48 files, `INDEX.txt`) | decompiled functions, incl. `angr_p9_hitapply.c` |
| Reference | `docs/mugen-re/MISSING-BUILTINS-RE.md` | master 5000–5999 table, timing, D1–D27 disagreement log |
| Reference | `docs/mugen-re/GUARD-CHAIN-RE.md`, `PROJECTILE-COLLISION-RE.md`, `PROJECTILE-PARAMS-RE.md`, `SCREEN-FLOW-RE.md`, `STAGE-COORDINATES-RE.md`, `TEAM-MODES-RE.md`, `SHADOW-RE.md`, `WIDTH-INVESTIGATION.md`, `INERT-CONTROLLERS-SURVEY.md`, `PARITY-ROADMAP.md` | per-subsystem RE |
| Reference | `local_mugen/extracted/mugen/data/common1.cns` | the ground-truth common state text |
| Engine | `app/src/main/cpp/engine/statemachine.cpp` (7,085), `fighter.cpp` (851), `entities.cpp`, `expr.cpp`, `anim.cpp`, `screens/fight_screen.cpp` | the engine |

Golden rule still applies: **behaviour notes only — nothing decompiled goes into `app/`**;
original characters stay inside `docs/` and `local_mugen/`.

Status legend: ✔ matches the reference · △ implements the *effect* with a different model ·
✖ still diverges (doc-tracked D-item) · ○ gap documented, out of scope of this comparison.

---

## 1. Frame loop and tick order

Reference: the per-player frame stepper (caller of `0x48B5F0`) was never decompiled
(FINDINGS pass 6 §4, lines 419–465). What is known: hitpausetime `+0xE70` gates the whole
runner; entry-init is skipped while it counts; `ignorehitpause`-flagged controllers run;
state -1/-2/-3 and the current state all process each tick.

Engine tick order (verified in `screens/fight_screen.cpp:670–795`):

1. world freeze (Pause/SuperPause early return) — `statemachine.cpp:1837–1852` ✔
2. per-fighter `machine_.step()`: bind → own hitpause freeze (`ignorehitpause` controllers
   of -1/-2/-3/current only) → recovery clock → assert flags cleared → `runStateLogic()` →
   `updateAnimation()` → `stepPhysics()` → stun clock decrement (`statemachine.cpp:1829–1972`) ✔
   - entry-init **is** run inside a freeze (via controllers that change state) where the
     reference skips it — N3(c) below
3. helpers step after roots, count snapshotted so helpers born this tick wait
   (`fight_screen.cpp:691–704`) ✔ (MUGEN's "created during this frame" reading)
4. `separate()` pushboxes before hit check (`fight_screen.cpp:709`) ✔
5. `inguarddist` recomputed before hit check (`fight_screen.cpp:714`) ✔
6. melee resolution: `wouldHit` both sides → priority/clash tiebreak with the six-class
   `clashLands` table, loser's HitDef stays armed (`fight_screen.cpp:724–760`) ✔
7. helper targets: enemy root first, then enemy helpers (`fight_screen.cpp:767–794`) ✔
8. player-attack vs projectile pass, `affectteam` mask, uses state-2 path
   (`fight_screen.cpp:796+`; ref `PROJECTILE-COLLISION-RE.md` §4) △ single armed-HitDef
   record means a hit spent on a fighter can't also shoot a beam down on that tick —
   documented approximation in-file
9. projectile-vs-projectile and projectile-vs-player passes (entities; ref §2–§3) ✔

## 2. State-machine core

| Reference behaviour (FINDINGS) | Engine | Status |
|---|---|---|
| Per-tick pass runs -1, -2, -3, then current state; -1 before everything; "started hitstun this tick" ends the turn | `runStateLogic` `statemachine.cpp:2090–2148` | ✔ |
| ChangeState loops into the new state same tick, entry at time 0; loop guard prevents runaway | `runCurrentState` :2150–2216, entry-init in `enterState` :1579–1790 | ✔ |
| Systematic clears at entry: hitcount/uniqhitcount unless `hitcountpersist`, contact code+timer unless `movehitpersist`, hitdef unless `hitdefpersist` | :1599–1624 | ✔ |
| Statedef type/movetype/physics with −1 = keep; sprpriority −1 = keep; velset ×facing; anim −1 = keep | :1640–1672 (`*Unchanged` flags, `animFromHeader`) | ✔ |
| facep2 at entry distance-then-turn | :1668–1671 | ✔ |
| 512-controller cap, per-controller `persistent` counters armed on fire; reset at state entry | :2261–2283 (`lastRunTick_`) | ✔ direct-corpus rule confirmed modulo measurement |
| `&4`/`&3` executor bits = ChangeState fired / stop remaining controllers | :2215 `if (stateNumber_ == before) return;` per guard-loop iteration | ✔ |
| "Valid state" flag / HUD invalid marker | presentation-side, n/a | ○ |
| Movement direction transitions even for a file-less state 0 | `runBuiltinMovement` | ✔ extra-service beyond MUGEN (documented in-file) |
| Ctrl handling of the built-ins' omitted `ctrl`: keeps prior | :1649–1653 with the comment block | ✔ |

Guard/lose/ko continue special case in `changeState`: dead fighters never re-driven to
idle states (`statemachine.cpp:1790+`); engine-side reading of MUGEN's round logic ✔.

## 3. Controller dispatch coverage

Reference dispatch table: FINDINGS pass 6 §5 (lines 481–502). Engine: `runController`
`statemachine.cpp:2301–2865`. Verified coverage = the full pass-6 list **except**:

| Gap | Severity | Note |
|---|---|---|
| `tagin` (551) / `tagout` (552) | M | live only for team modes — S11 in PARITY-ROADMAP (○, team modes) |
| dispatch-slot 213 (unseen in reference) | – | reference itself does not resolve it; n/a |
| `makedust`/`makeanim`/`makepalette`/`displaytoclipboard`/`drawgame`/`zoom`/`bgm`/`appendtoclipboard`/`envadjust`/`explodbindtime`/`removemappedexplods` | L | accepted-and-ignored, matching the corpus shape (INERT-CONTROLLERS-SURVEY) |

Noted-filed-correctly: `nothitby` under `n` (:2408), `offset` under `o` (:2454),
`remappal` under `r` (:2776), with comments why filing mistakes would have silenced them.

ChangeState/SelfState semantics (pass 6 §5): `value` required, `ctrl`/`anim` optional,
absent = keep ✔ (:2865–2886). Two edge divergences: N3(a)/(b) below.

## 4. Trigger coverage

Reference opcode map: FINDINGS pass 6 §3 (lines 354–417). Engine: `ownTrigger`
`statemachine.cpp:6228+` and `expr.cpp`.

Matches ✔ (selected, all verified this pass): stateno/prevstateno/time/ctrl,
statetype/movetype/physics letters, anim/animelem/animtime/animelemtime/animelemno,
animexist/selfanimexist, pos/vel/screenpos/edgedist family (front/back × edge/body),
hitshakeover (`shakeTicks_ == 0` ≙ `+0x1090 ≤ 0`, :6590), hitover (`stunTicks_ < 0`
≙ `+0x1094 < 0`, :6591), hitfall/hitpausetime/canrecover, life/power family, palno,
movehit/moveguarded/movecontact/movereversed (:6577–6580), hitdefattr, statenoexist,
redirections (parent/root/helper/target/enemy / playerid :6180–6200).

Divergences in this area:

- **N1 (M) — fixed 2026-10-19.** move-contact codes are elapsed-tick counters, not
  booleans: `moveContact_`/`moveHit_`/`moveGuarded_`/`moveReversed_` are set to 1 by
  `markHitLanded`/`markGuarded` and incremented each unfrozen tick in `step()`, so
  `movecontact = 2` and `movehit <= 10` read the way the reference's `+0xE90 + 1`
  does. Cleared unless `movehitpersist` at entry ✔.
- **N5 (L) — fixed 2026-10-19.** `AnimExist` asks the file the fighter is running
  (`animCharacter()`, which follows a `p2stateno` borrow) while `SelfAnimExist` always
  asks the victim's own file. Host test: a borrowed file's 500 is visible to `AnimExist`
  and invisible to `SelfAnimExist`, with the reverse for art only in the victim's file.
- `standby` hardcoded false (:6395) — reference trigger exists (0xCD); only touched by
  super-CV style content. Tracked as ○.

## 5. Hit detection

| Item | Reference | Engine | Status |
|---|---|---|---|
| Attack (Clsn1) intersects hurt (Clsn2), armed HitDef required, hitCount budget spent ⇒ no further contact | pass-17 / hitapply callers | `Fighter::wouldHit` `fighter.cpp:444–461`, `attackOverlapsEntityBox` :254–281 | ✔ |
| Box → world mapping: local x ×facing, y *added* on the floor-origin line, draw scale applied | stage coords RE | `worldBox` `fighter.cpp:283–307` (comment documents the mirror bug history) | ✔ |
| projectile boxes vs fighter hurt box; `projscale` draw-only, off collision (§7.2) | PROJECTILE-COLLISION-RE §3, §7 | `overlapsEntityBox` :243–252 | ✔ |
| simultaneous hits: priority number, then class tiebreak table; loser keeps its HitDef armed | sctrls + binary | `wouldHit`+`clashLands` pair at `fight_screen.cpp:724–760` | ✔ |
| ReversalDef capture before damage | hitapply neighbor | `Fighter::tryHit` :397–405, `tryReversal` | ✔ |
| juggle allowance paid in the air | `+0x10D8` family (pass 6) | in `applyHit` juggle block (:5654+) | ✔ |
| guard availability: `inguarddist` from attacker's live `guard.dist`, seeded per-state from `[Size] attack.dist` | pass 13/14 | `enterState` :1605–1617, `updateGuardDistances` `fighter.cpp:471` | ✔ |
| corner push when the victim cannot be driven further | hitapply :583–620, four conditions with the `0xfffffbff` (−1025) "not written" sentinel | `fighter.cpp:424–436`: victim within **12 px** of the edge ⇒ attacker nudged once by `1.3×pushX` | △ simplified: single nudge at contact vs the reference's mindist/maxdist-slot mechanics and sentinel (N7) |

## 6. Hit application and the get-hit chain

`angr_p9_hitapply.c` ↔ engine mapping (verified line-against-line):

| Decompile | Engine |
|---|---|
| ground branch → 5000/5010, `ground.type == 3` (trip) → 5070 | `hitInfoFor` `statemachine.cpp:5241–5654` |
| launch ⇒ fall latch (`forcenofall` respected) | same file |
| forcestand on a crouching victim without fall | same file |
| down → 5080/5081, air → 5020/5030 | same file |
| p2stateno custom state + owner record write | borrowed-state machinery (`changeState` + `SelfState` clears `borrowedStateChar_` :2410–2416) |
| victim `+0x11A8` second guard record | not modelled (unexplained field in decompile; no observed effect) |
| super p2defmul damage math, hitpause max(1,·) floor | `applyHit` :5654+ |
| HitOverride class/type mask, spend, forceair, victim pause | `applyHit` + `hitOverrides_` slots aging (:1961–1963) |
| shake/paustime clocks and `hitover`/`hitshakeover` getters | :6588–6591, clocks in `step()` |

Get-hit *route* coverage (the `kBuiltins` rows the reactions drive):

- 5000/5001, 5010/5011 (→11, not 0) ✔ with in-file reasoning (:554–568)
- 5020→5030 ✔; 5030→5035 waits for the animation even inside stun via the narrow
  exception `statemachine.cpp:2201–2211` ✔ (the reference's own design, comment cites it)
- 5035 splits on `hitFallFlag_` → 5040/5050 ✔ (:1220–1223)
- 5050 fall waits ground, `land()` resolves ✔
- 5070/5071 trip-fall → 5110 ✔
- 5080/5081 ✔ (5081 → 5110 on shake end)
- 5100/5101: bounce launched from `land()` at contact — D13/D23's remaining
  simplification (§10)
- 5110 → 5120 → 0 ✔ route with the reference's physics N, dead exit and 5120's
  `NotHitBy` window (D20/D21 fixed)
- 5150 row present with `sprpriority = -3` (D22 fixed)
- 5200/5201 ✔ incl. entry controllers reconstructed (:1675–1706); 5210 uses the binary's
  live numbers and its CtrlSet/NotHitBy clocks — D24 fixed
- 5500 inert row with the `SelfAnimExist(5500)` preference ✔ (:1734–1737)

`land()` decision tree (statemachine.cpp:4846–4973, incl. :4906–4968) ✔ against the
reference's landing table; guard-to-130/52 question preserved for 132/155 ✔.

## 7. Guard chain

Per `GUARD-CHAIN-RE.md`: the four clocks, two decrements, hitpause gating, sequential
`ctrltime` then `hittime` — engine implements in `applyHit`/`step()`/`scanGuardChain()`
(:3825+). The 150–155 shake rows are timed by the guard shake clock (`guard.pausetime`
second value = `guard.p2shaketime`), not the stun, per `builtinExitsOnShake`
(:418–450, citing pass 17 §17.1) ✔. Guard hand-over carries `velset = 0,0` then
`HitVelSet` re-read (:1337–1363) ✔; height re-answered from input at exit ✔.

## 8. Physics, friction, landing

| Item | Reference | Engine | Status |
|---|---|---|---|
| stand/crouch friction 0.85/0.82, gravity→integrate→friction order | pass 6 §6 :518–523 (open, not decompiled) | `stepPhysics` :4777–4846 applies the same constants in the same order | △ constants match the docs; exact proof pending decompile |
| auto-turn exemptions in hit/attack states | known | :4787+ with exemption list | ✔ |
| physics N still carries velocity | known | :4777 | ✔ |
| air physics gravity | `movement.yaccel` | per-state substitution: the get-hit states fall with `GetHitVar(yaccel)` — `stepPhysics` pulls `lastHit_.yaccel` (default 0.35, or the HitDef's value) for the 5030/5035/5050/5071/5200 family, while `yAccel_` stays the character's `movement.yaccel` for ordinary air movement (`stepPhysics` :5153–5176; `getHitVar(yaccel)` reads the same field :7074) | ✔ **D14 fixed** |
| get-hit slide (5001/5011/5081): `slidetime` free ride → one `VelMul x = .6` → `x = 0` at HitOver | MISSING-BUILTINS §2 | `stepPhysics` (:5200–5221): 5001/5011 keep the hit's velocity untouched until `Time >= GetHitVar(slidetime)`, then `velX *= .6` each tick with the friction threshold; 5081 uses the file's own crouching friction and no `VelMul`. Host test: 8 → 8 → 4.8 over the first three knock-back ticks with `slidetime = 2` | ✔ **D15/D26 fixed** |
| 5030-family fall gravity | `GetHitVar(yaccel)` | same `lastHit_.yaccel` pull as the air-physics row | ✔ **D14 fixed** |
| 5210 roll numbers | binary live values `.5/.2` add −4.5, CtrlSet at Time=20, NotHitBy 15 | `runBuiltinState` (`kHitAirFallRecover`, :1339–1395) uses the live `.5/.2`, the −4.5 add, `CtrlSet 1` at `Time = 20`, `NotHitBy` for 15 ticks, and the entry `PosSet`/turn of 5201 | ✔ **D24 fixed** |
| landed/bounce | 5100 holds, HitFallDamage at Time=3, bounce after the anim | bounce at contact in `land()` | △ **D13/D23 simplified** |
| liedown.time engine clock for 5110 | `[Data] liedown.time` | `liedownTicks_` (:1298–1323) | ✔ deliberate, correct reasoning |

## 9. Built-in state table (kBuiltins, :357–760)

Row-by-row against `common1.cns` + the doc master table:

✔ correct rows (number, type, movetype, physics, ctrl, anim, hand-over):
0, 5, 10/11/12 (types going *with* the transition, commented), 20, 40→50, 45 (row exists;
engine never enters it — air-jump open, D6 route supplied but unreachable), 50, 55, 52,
100/102, 105→106 (fixed 7-tick hold in `runBuiltinState` :1172–1179), 130/131,
150–155 (chain + hand-over targets incl. input-driven 151/153-FAQ), 180, 190/191
(hand-over on `Time = 0`, `SelfAnimExist(190)` respected :1710–1714),
5000/5001, 5010/5011, 5020, 5030, 5035, 5040 (passive row; `land()` supplies 52),
5050, 5070/5071, 5080/5081, 5100/5101, 5110/5120/5150, 5200/5201, 5210, 5500.

D14/D15/D16/D17/D19/D20/D21/D22/D24/D26/D27 are fixed; the remaining divergences are
the structural ones in §10's last row (D4's round-opening simplification included).

**N2 (M) — fixed 2026-10-19.** The real file writes
`[State 50, 2] ChangeAnim value = ifelse((vel x)=0, 41, ifelse((vel x)>0, 42, 43))` and
`[State 50, 3]` re-dresses to `Anim + 3` (44–46) when `Vel y > Const720p(-8)`
(`common1.cns`, statedef 50). `enterState`'s state-50 entry now chooses 41/42/43 from the
facing-relative horizontal velocity when the art exists, and the `kAir` case re-dresses
41–43 to 44–46 once the climb is nearly spent. Host test covers 41/42/43 and the 41→44
re-dress.

Deliberate omissions documented as *correct*: no state-47 row (number invented, :488–494);
D8/D9 rows (120–142, 170/175) as controllers; state 0's `!alive` door.

## 10. Master divergence table (doc-tracked, re-verified this pass)

From `MISSING-BUILTINS-RE.md` §3 — current status after direct code checks:

| Item | Where | Doc claim | Code today | Status |
|---|---|---|---|---|
| D2 | 5035 | row/route missing | row exists :594; route supplied | **fixed** (doc stale) |
| D5/D6/D7 | 45/106/190/191 | no rows | rows exist :474/:582/:608–609 | **fixed** (doc stale) |
| D10 | 5071 | `land()`→5100 | →5110 | **fixed** (doc stale) |
| D11 | 5040 dead | `land()`→5100 | dead default → kHitFall (5100 route) :4966 | **fixed** (doc stale) |
| D14 | 5030/5035/5050/5071/5200 | gravity = `GetHitVar(yaccel)` | `stepPhysics` pulls `lastHit_.yaccel` for the get-hit family (default .35, HitDef value otherwise) | **fixed** |
| D15/D26 | 5001/5011/5081 | slidetime → VelMul .6 | slide model in `stepPhysics` :5200–5221 (5081 keeps crouch friction) | **fixed** |
| D16 | 5000–5030 | animtype/groundtype/SelfAnimExist art pick | `enterState`'s `getHitAnim` implements the file's arithmetic | **fixed** |
| D17 | launches | `StateTypeSet a` at shake end | a launched 5000/5010 sets `StateType::Air` at `Time = 0` (`runBuiltinState`) | **fixed** |
| D19 | 5080/5081 | physics N/C | N / C in the rows; 5080 zeroes its velocity | **fixed** |
| D20 | 5110 | physics N; dead exit waits anim/SysVar | physics N; exit gated the same way | **fixed** |
| D21 | 5110/5120 | 5120 `NotHitBy` 12/3 | 5120 entry raises the same invulnerability window | **fixed** |
| D22 | 5150 | `sprpriority = -3` | set in `runBuiltinState` (:1952); host test reads `spritePriority()` | **fixed** |
| D23/D13 | 5100 | hold, HitFallDamage Time=3, bounce post-anim | 5100 hold and 5101 bounce route; the contact-time bounce in `land()` is the one part still simplified | △ simplified |
| D24 | 5210 | binary live values | live `.5/.2`, −4.5 add, CtrlSet at 20, NotHitBy 15 (:1339–1395) | **fixed** |
| D27 | get-hit hand-overs | HitOver unconditional | 5001/5011/5030/5035/5081 hand over on HitOver | **fixed** |
| D1/D3/D4/D8/D9/D12/D18/D25/D28 | structure/5500/5900/guards/170/5040-exit/velset/liedown/hitpersistence | various | unchanged / deliberate | as documented in MISSING-BUILTINS |

New items found this pass (N-prefix; **N2 is the most gameplay-relevant**):

| Item | Severity | Description |
|---|---|---|
| N1 | M | **fixed** — move-contact triggers are elapsed-tick counters (`step()`), so `movecontact = 2`, `<= 4` windows read |
| N2 | M | **fixed** — state 50 anims 41/42/43 on entry and the `Anim + 3` re-dress 44–46 |
| N3 | L | **fixed (a, b, c)** — (a) `ChangeState ctrl = 0` revokes control, an omitted `ctrl` keeps the header's answer; (b) the state entry block is deferred while `hitpausetime > 0` (`pendingStateEntry_`); (c) only duration −1 holds, all other negatives are zero ticks. (d) was mis-scoped: `keepone` belongs to `TargetDrop`, and the corpus has no `noface`, so there is nothing to parse |
| N5 | L | **fixed** — `AnimExist` follows `animCharacter()`; `SelfAnimExist` always reads the victim's own file |
| N6 | L | **fixed** — `hitshakeover` is `shakeTicks_ <= 0`; host test covers the negative path |
| N7 | △ | corner push exists (fighter.cpp:424–436); still a single 1.3× nudge against the reference's four-condition mindist/maxdist model |

Doc-housekeeping (done 2026-10-19): `MISSING-BUILTINS-RE.md`'s D2/D5/D6/D7/D10/D11
rows and `PARITY-ROADMAP.md`'s corner-push line now match the code.

## 11. Projectiles, helpers, boxes — quick status

- Projectile collision passes, pending-hit protocol, state 1 removal conditions, priority
  duel, `projremanim`, bounds/edge: implemented per `PROJECTILE-COLLISION-RE.md` §2–§6
  (verified earlier passes; §7 `projscale` draw-only on collision ✔ with citation in
  `overlapsEntityBox`).
- Helpers: spawn/parent/root relations, redirects, keyctrl, doom, position after parent
  ✔; team-specific helper relations ○.
- Width/push (WIDTH-INVESTIGATION): `width` controller stores all four slots ✔
  (:2664+); shadow per SHADOW-RE ✔.
- Team modes: ○ open (S11).

## 12. Animation system

✔ `animTime_` counts up; `Animelem`/`AnimTime` read down; `animationFinished()` = `AnimTime = 0`
(:5023–5060, comment block reconciles both directions with the HUD's "Elemno/Time" evidence
from `angr_0044eef0_frame_executor.c` — which is the **debug HUD renderer**, *not* the
animation-advance path; pass-6's animation-advance decompile remains missing).
Looping via modulo (`frameAt`); blank frames consume ticks; frozen ticks counted.
✔ ChangeAnim (restart guard, `elem` skip via pushed clock :2888–2916), ChangeAnim2 borrow
(:5010–5016) cleared at entry (:1588), interpolation flags parsed in anim.cpp.
✔ Negative-duration rule (N3c): only −1 holds forever; every other negative is zero ticks
in `frameAt`/`frameDrawn` (`effectiveDuration`), and `infinite` tests `duration == -1`
only. Host test: an action whose first element is `-2` is not infinite and shows element 1
at tick 0.
N3(d) closed as mis-scoped: the ChangeAnim patch flags `keepone`/`noface` do not exist in
the corpus — `keepone` is a `TargetDrop` parameter, and no `noface` appears at all — so
there is nothing for the parser to carry.

## 13. Round and screen flow

Per `SCREEN-FLOW-RE.md` (freshest doc): intro 190→191→0 ✔; round-specific clearing
is D4's documented engine-side simplification. `setup()` clears the vars (5900's two
`VarRangeSet` controllers) and enters state 0 directly, recording `prevstateno = 5900` for
the first tick. The intro pair 190/191 is played by the screen's own round-announcement
phases, which hold the fighters rather than run their states, so entering 190/191 here
would replay a character's intro a second time (and KFM's 191 is long enough to delay
control). This stays deliberate: the reference's observable first fighting tick is state 0
with `prevstateno = 5900`, which is what this produces.
Win/lose flow `win_flow.cpp`/`victory_flow` ✔; continue screen absent by design (5500
inert row ✔ continue_flow.cpp exists for the flow question).

## 14. Recommended priority order — implemented 2026-10-19

All code items below are in place and covered by `tools/hosttest/test_statemachine.cpp`
where the behaviour is observable from a state machine alone. The host suite is green
(20 test binaries).

1. **N1** — move-contact/movehit tick counters. ✔
2. **D14 + D24** — `GetHitVar(yaccel)` gravity for the get-hit family and 5210's live
   numbers. ✔
3. **D16** — animtype/groundtype art selection in `enterState`'s `getHitAnim`. ✔
4. **N2** — state 50's 41/42/43 and the 44–46 re-dress. ✔
5. **D15/D26** — the `slidetime` slide model. ✔
6. **D21/D22/D17/D20/D27** — guard/get-hit windows, `sprpriority = -3`, launch type,
   friction/dead exit, unconditional HitOver. ✔
7. **N3(a/b/c), N5, N6** — control hand-over, deferred entry during hitpause, negative
   animation durations, `AnimExist` vs `SelfAnimExist`, `hitshakeover <= 0`. ✔
   **N3(d)** closed as mis-scoped (no such flags in the corpus); **N7** remains a
   documented simplification rather than a parity bug.

Remaining known divergences, all deliberate and documented: D4 (round opening — §13),
D13/D23's contact-time bounce in `land()` (§10), N7's single-nudge corner push (§5), and
the structural items in §10's last row.

*Stale-doc repairs* (no code): MISSING-BUILTINS D2/D5/D6/D7/D10/D11; roadmap
corner-push line. ✔

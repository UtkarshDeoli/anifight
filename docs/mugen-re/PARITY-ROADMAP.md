# anifight ↔ MUGEN 1.0 Parity Roadmap

Goal: full MUGEN 1.0 fight-engine parity in `app/src/main/cpp` (from-scratch implementation,
behavior-matched to the reference binary). Canonical surface = the vocabulary/semantics
extracted in `FINDINGS.md` (pass 3 tables). Golden rules still apply:
behavior notes only — **no decompiled code in `app/`**; original chars only in-repo.

Sources for "what MUGEN does": `FINDINGS.md` (anchors + vocab), Ghidra decompiles under
`docs/mugen-re/decompiled/` (semantics verification), Wine black-box runs when timing is
ambiguous.

## Where we are (2026-10-18 inventory)

Solid: file formats (SFF v1+v2 incl. RLE5/PNG, AIR incl. clsn and Interpolate, SND/WAV
PCM, .act, .def family, screenpack, lifebars, round flow), the fight core (HitDef with
attr/hitflag/fall.\*, clsn collision, juggle, guard + chip, hitpause,
corner push, priority clash, projectiles, explods, helpers with the full parent/root
entity graph, targets, PalFX/BGPalFX/AllPalFX, var/fvar/sysvar, `:=`, the CMD matcher
including `>` and `$`, audio params, AngleDraw), and palette selection from the select
screen.

The table this section used to carry — a 14-row ranked gap list — had gone stale against
the sprint records below it, which are the authoritative status: it still listed Pause
(all supers break), the palette pipeline, the CMD matcher, the RLE5/PNG codecs, the audio
param set and the target system as top-ranked *gaps* long after each had landed. It is
replaced by what is genuinely still open, each cross-referenced to the slice that owns it:

| Gap | Why it blocks real characters | Owner |
|-----|-------------------------------|-------|
| **Get-hit state numbers are not MUGEN's** — four invented, two repurposed, the crouch and knocked-back pairs missing | Every hit of every match picks a reaction state, and 34–36 of the 41 packs in the corpus ship their own; see the finding below | **Landed** — the renumbering, the victim-stance chooser, the launch and air-branch fixes, `GetHitVar(fall)`, the shake clock and all three of the guard clocks; what remains is 5035, the `kBuiltins` timing column and what holds a built-in blocker in 151/153, all named in the finding below |
| ~~**`p2getp1state`**~~ — ✅ **landed, FINDINGS pass 19.3** | Throws and grabs that hand the victim a custom state: the victim now runs the *attacker's* state file (`p2getp1state` defaults to 1 whenever `p2stateno` is written), keeps its animation, and returns home on a hit or a `SelfState`. 272 of the corpus's 276 `p2stateno` writes depend on it. | S3/S5 |
| ~~`GetHitVar` tail~~ — ✅ **landed**: `yaccel` and `isbound` are answered alongside the HitDef `yaccel` parameter; the `fall.*`, `hitcount` and `ctrltime` keys landed earlier (see the corrected-rows note) | Characters read these back to size their own reactions; `yaccel` alone is 294 uses across 36 packs, and the reference's own `common1.cns` reads `yaccel`, `ctrltime` and `fall.yvel` inside the get-hit states | S3 |
| Multi-target lists (tag/team and multi-hit grabs) | Tag/team and multi-hit grabs. Note: `TargetBind` has no facing parameter in MUGEN 1.0 — `TargetFacing` is its own controller, and this engine implements it; the old "TargetBind facing param" wording was wrong | S4 |
| ~~RemapPal's group half~~ — ✅ **landed** | Re-colouring one sprite palette node at runtime: `source`/`dest` are `(group,item)` palette identities (not sprite groups), one-arg form means group 1, destinations are not chained, and the map resets each round | S6 |
| ~~`numprojid`, `proj*time`, `projcontact*`~~ — ✅ **landed** (2026-10-19) | `numprojid`/`numprojectileid` counts state-1 entries by plain ID equality; `projremovetime` (default −1), `projremove` (default 1) and the negative-`projmisstime` clamp (→ 2) are parsed and consumed. `ProjHit`/`ProjGuarded`/`ProjContact` and their `*Time` forms read the owning side's last contact, kept on the player rather than the projectile so it survives the slot, with the documented suffix and corpus function spellings; the fight's three collection passes fill it in | S7 |
| ~~Game Over / Default Ending / End Credits storyboard player; VictoryQuote~~ — ✅ **landed** (2026-10-19) | `engine/storyboard.*` + `screens/storyboard_screen.*` play a storyboard's scenes, layers, fades and sounds; `endArcadeRun` and the Win-pop chain reach them. `[Quotes]` + `VictoryQuote` select the victory text. Remaining in that slice: `bgm` (no decoder) and `bg.name` objects |
| Team modes (simul/turns/tag); real AI script | The full-match experience | S11 |
| Zoom controller; fightfx zoom; object (explod/projectile) reflections | Stage zoom and floor reflections | S12 |
| ~~**`sub_47F380`**~~ — ✅ **settled, RE pass 11** (not a gap) | The HitDef active-block *filler*: reads the parsed template (`a0`) and writes the 500-byte block at `player+0x11B0` (`a1`); the template→active offset map is now in `FINDINGS.md` pass 11. It is **not** a palette executor (pass-6 `palfx_executor_b` and pass-7 §4 were wrong). What remains is each field's *runtime consumer*, not the block layout. | RE pass 11 |

**Rows that this inventory corrected, because they were stale or simply wrong:**

- `chainid`/`nochainid` bookkeeping — **landed**, see the S3 note below. The row had
  outlived the work: the parse, the self-contradiction rule and the eligibility gate all
  exist, with regression tests that were shown to bite.
- `down.*` and `airguard.*` — **landed**, see the S3 note below. Both were carried as
  gaps on the strength of the parse alone and the parse was never the missing half: the
  *reaction* is, and it is now written and shown to bite under mutation.
- `projremanim` — **landed**, and it had been wrong twice over rather than missing. It
  was parsed with a flat `-1` fallback where the reference says it falls back on
  `projhitanim` (so a blast that named only a hit animation blinked out of the world
  instead of playing it), and there was no consumer at all: a projectile that was told to
  go simply vanished, whatever animation the file had given it. Both halves are now
  written and pinned by a parse block plus the suite's only real `af::EntityLayer` run.
  Worth recording *what* the removal animation is played for, because the answer is not
  the engine's usual rule: it is played **once**, and the projectile goes when that one
  pass is over. Waiting for the animation to *finish*, as an explod does, would leave
  every real one of them on screen for ever — an animation with no `LoopStart` loops
  from its first element, and all three removal animations a character in the corpus
  actually names are that shape (`dbz_piccolo/piccolo.air` action 3006 is eight one-tick
  frames; `dbz_gohan/P-Gohan.air` 1016 is a single two-tick frame), so a projectile would
  have kept drawing and kept counting towards `numproj` for the rest of the round. The
  other 24 written `projremanim`s in the corpus are explicit `-1`s, which is a character
  saying "no death animation" on purpose, and the fallback is careful not to override
  them.
  Related, and **not** done: the reference's `projremove` ("set to a non-zero value to
  have the projectile be removed after it hits … defaults to 1") is unread — this engine
  always spends `projhits` and always removes, which is the default behaviour and so the
  right thing to be missing. `projcancelanim` remains unread for the same reason as
  before: it belongs to projectile-versus-projectile cancellation, which the engine does
  not implement.
- `Projectile` `F`-prefixed animations — **landed**. This was the last row still carried
  as an open gap on the strength of a *parse* claim, and the parse was only half of it: the
  three parameters (`projanim`, `projhitanim`, `projremanim`) now resolve their prefix like
  an `Explod`'s `anim`, and each of the four places that act on the result had to be taught
  which file to read. Two of those are invisible until a screenpack-drawn projectile has to
  *hit* something — the collision boxes are looked up in the animation's own file, and so is
  the sprite bank at the draw — so the row was understating the work as much as it was
  overstating the gap. See the prefix note below for what is and is not host-tested.
- `envshake.*` on hit time, and `getpower`/`givepower` hit economics — **both were already
  landed** and the rows were simply wrong. `hit.shakeTime` is parsed at `statemachine.cpp`
  and applied through `entities_->shake` where the contact lands; `getpower` reaches the
  attacker at `fighter.cpp:375` and `givepower` the victim in `applyHit`.
- `mindist`/`maxdist`/`snap` — landed except the facing op, so the row narrowed to
  `p1facing`, and **`p1facing` has since landed too**. The row was understated on the
  evidence as well as on the work: the family is not one parameter but a pair, and the
  reason it took a binary read to get right is that the reference does not store what
  the character wrote. `p1facing` is kept as the single bit "is it negative" and
  `p1getp2facing` clears that bit when it is non-zero, so `p1facing = 1` and no
  `p1facing` at all are the *same* no-op — and 44 of the corpus' 51 uses of it write
  exactly that 1. A value-reading implementation would have had to invent a meaning
  for it. The uses that do something are all one idiom, a throw that picks its own
  facing from the direction held (`ifelse(command = "holdfwd", 1, -1)`, and a
  corner-aware variant that throws away from the near wall); 34 of the 36
  `p1getp2facing` lines are explicit zeros. Applied on the attacker's own side, in
  `markHitLanded`, which is where `p1stateno` — its sibling in the same family — was
  already handled. Pinned by eight checks over the parse, the sign, the precedence and
  the no-victim case; three mutations (dropping the sign rule, dropping the
  precedence, inverting the copy direction) each bit exactly the checks they should
  and nothing else. What is *not* verified is the action itself — the consumer of the
  bit is the one part of pass 11 §11.6 still not extracted — so "turn around" is
  implemented as the negation the reference's own documentation words it as, and that
  is recorded as the single MEDIUM item in the finding rather than presented as read
  off the binary.
- `animtype` / `air.animtype` / `fall.animtype`, and the `ground.type` / `air.type` pair —
  **landed**, and the interesting part is that none of them had a row: this was not a
  missing feature but a *wrong answer* to a question the corpus asks constantly. The engine
  parsed `animtype` alone and answered it to every victim in every situation, so
  `GetHitVar(animtype)` — **990 uses across 38 characters**, the most-used trigger of the
  whole hit family — was right only for a victim already on the floor; `GetHitVar(groundtype)`
  (110 uses) was a hardcoded `1`; and `GetHitVar(airtype)` (44) returned the animtype, a
  value out of a different family. `air.type` itself is written 746 times across the corpus
  and was not read at all.
  What the reference does is a three-tier cascade, and its defaults point at *other
  parameters* rather than at constants, which is why neither the old engine nor a
  value-copying version of it could have been right: `air.animtype` defaults to `animtype`,
  `fall.animtype` defaults to `air.animtype` when that is Up **or DiagUp** and to Back
  otherwise, and `air.type` defaults to `ground.type`. The DiagUp half is read off the
  binary rather than the documentation, because the two disagree — the reference's text
  describes a two-way default and `sub_47DA40` writes `if (a1[760] == 4) a1[424] = 4; else
  if (a1[760] == 5) a1[424] = 5; else a1[424] = 3`, carrying DiagUp through
  (`FINDINGS.md` §11.11). The tiers are resolved at *hit* time rather than at parse, because
  which tier applies is a fact about the victim — airborne, airborne and in the hit reaction,
  or grounded — and the third case is not simply "grounded": a hit that hands the victim an
  upward velocity, or writes `fall = 1`, is a hit about to put them in the air, and those
  take the fall animtype too. That is the same condition as the implicit launch rule the
  subsection above settled, so the two agree by construction rather than by coincidence.
  The hit *types* were renumbered to the reference's own — 0 none, 1 high, 2 low, 3 trip —
  which is the numbering a character branches on (`ifelse(GetHitVar(groundtype) = 1, 5000,
  5010)`, in the reference's own `common1.cns`), where the engine had been handing out an
  internal enum. Nineteen checks over the cascade, the defaults, the launch case and all
  three type keys; four mutations (dropping `air.animtype`, collapsing the DiagUp default to
  a two-way one, dropping the launch case, and answering `groundType` for `airtype`) each bit
  exactly the checks they should and nothing else. Not covered by any host test that draws
  anything, which is this project's usual limit.
  **Landed since, and it is a correction rather than an addition:** all six of these names
  are matched by their **first character** and by nothing else. The parser loads one byte of
  the value, lowercases it, subtracts `'b'`, bounds-checks against `0x13` and jumps through a
  twenty-entry table whose only populated slots are `l`->Light, `m`->Medium, `h`->Hard,
  `b`->Back, `u`->Up, `d`->DiagUp (`mugen.exe` 0x47EE4A; `air.animtype` 0x47EE9B and
  `fall.animtype` 0x47F019 are the same scheme; `ground.type` 0x47F067 and `air.type`
  0x47F141 are the same again on `h`/`l`/`n`/`t`). The strings `Light` and `DiagUp` do not
  occur in the binary at all, so matching the spelling — or a prefix of it, which is what
  this engine did — is not the reference's function. The corpus is where the two diverge:
  `animtype = heavy` is written **744 times** and reads as Hard (`h`), where a prefix match
  against the documented names leaves it Light, and `med` (17), `down` (13) and `high` (7)
  are the same mistake in smaller numbers. `parseHitType` was already right on every value
  the corpus writes, by luck of the prefix, and is now first-character too. An unrecognised
  first letter is the reference's "Invalid animtype" error path, which reports the parse as
  failed; this returns Light, the documented default for an omitted parameter, and nothing
  in the corpus reaches that branch — recorded below as the one remaining difference.
  What it leaves open is all one family and was, when this row was written, the largest single
  hole in it. The row has since gone stale in both directions. **Landed since:** the fall
  landing's three effects and the `GetHitVar(hitcount)` counter — `hitcount` (62 uses) and the
  `fall.*` family (`fall.yvel` 133, `fall.damage` 11, and the `fall.envshake.time` key) now
  answer real values, with the reasoning in `FINDINGS.md` passes 14 and 15 below. A re-read of
  `statemachine.cpp` while writing this also finds `GetHitVar(ctrltime)` (`:4410`) and
  `GetHitVar(yaccel)` (`:4414`) answering real values. This row has since gone stale
  itself as well: **`isbound` is answered** (the key test sits beside the bind bookkeeping,
  with the reference's TargetBind rule applied to all three binds), and **`yaccel`'s
  default is settled** — the docs give `.35 in 240p, .7 in 480p, 1.4 in 720p`, `hitapply`
  computes exactly that as `width * 0.35 / 320` at the point of use, and the parser
  registers only the sentinel, so the engine's plain `0.35` default is the reference's own
  number for the one coordinate frame it has. See `FINDINGS.md` pass 18 §18.5. The
  state-numbering half that this row once ended on has since **landed**, `air.type == 3`
  included: the air branch now writes 5070 for a Trip `air.type` exactly as the ground branch
  does for a Trip `ground.type` (`if (v11->field_13c == 3) v12->field_c4c = 5070;` beside
  `v12->field_c4c = 5020;` in `hitapply`'s `LABEL_46ddde` block). See the finding below.
- `const240p`/`480p`/`720p` — **landed**. `timeroundstart` was listed beside them as though
  it were the same kind of gap; it is not a MUGEN 1.0 trigger at all (below).
- **`playerindex` is not a MUGEN 1.0 trigger** and has been dropped from the table. Like
  `timeroundstart`, it is absent from the reference's own alphabetical trigger index
  (`local_mugen/extracted/mugen/docs/trigger.html`), which lists `RoundNo`, `RoundsExisted`
  and `RoundState` as the round triggers that do exist, and no `PlayerIndex`. Both were
  carried in this roadmap as gaps against a target version that never had them. `timeroundstart`
  is still *answered* (with a neutral 0) because a character written for a later build may
  test it, and that is now documented at the site rather than left as a bare stub.

**A defect this inventory found, which no gap row had named — knockback never launched.**

Reading the velocity frame while preparing the `down.*`/`airguard.*` slice turned up a
whole-engine defect that no row above describes, because it was not a missing feature: the
HitDef parser was reading `ground.velocity`'s and `air.velocity`'s y in the *file's* frame
while the rest of the engine works in its own. `cns.html` states the file convention
("a negative y-velocity means he is moving up"), every other velocity entry point already
converts it — `Velocity`, `VelAdd`, `jump`, `fall.yvelocity`, the `vel y` trigger each flip
the sign in one place — and these two did not. The consequence was total rather than
subtle: `launches` reads a positive y as "off the feet", so a launcher's written
`ground.velocity = -8,-4` arrived as a *downward* 4 and **no move in any character file
could put an opponent in the air** — the victim took a standing hit state with a velocity
into the floor. Corpus evidence: `ground.velocity = -8,-4` occurs 37× and
`-4*const(size.xscale),-6` 32×, always paired with an identical `air.velocity`.

Fixed by converting both y values at parse (`statemachine.cpp`, `doHitDef`), and by
correcting `air.velocity`'s omitted-value default from −3 to **0**, which the reference also
states ("Defaults to 0,0 if omitted") and which is load-bearing: with the sign converted, a
default of −3 would have made `launches` true for *every* HitDef ever written. Verified by
parsing a real launcher out of a real file and asking a victim what it got (state 5030, an
upward velocity) — a sign error is invisible in the parse and obvious in the reaction.
Regression-tested; four mutations (each y conversion removed, the default restored to −3,
and `gethitvar(yvel)` left unconverted) were each shown to bite.

Two things this fix deliberately did **not** change, both recorded rather than guessed at.
The first has since been done:

- ~~The launch test is still `pushY > 0.5f`, where the reference and the binary both say
  **non-zero**.~~ **Landed.** `FINDINGS.md` pass 12 read the writer rule straight out of
  the filler — `sub_47F380` sets its "off their feet" flag at `hitdef+0xB4` with
  `if (v29 != 0.0f && !a0[101])`, a comparison against **0.0f** — and found its reader in
  `hitapply`'s standing/crouching branch, where the flag is OR'd with a caller flag and
  picks the victim's *statetype* before the state number comes out of it. So the flag
  gates the state and not merely the velocity, which is the same shape as this engine's
  `HitInfo::air`. The branch now reads `hit.pushY != 0.0f || hit.fall`, which is that
  OR written once.
  Pass 12 also settled the half that made this more than a one-line change: the velocity
  a ground launch carries in the binary is `ground.velocity`'s own pair
  (`field_130`/`field_134`), and this engine was handing over the **`air.velocity`**
  pair. That pair is now the ground one, and the reference backs the binary here rather
  than merely permitting it: the two families are mirrors by construction — a ground
  victim takes `ground.velocity`/`guard.velocity`, an airborne one
  `air.velocity`/`airguard.velocity` — and the *defaults* say so out loud
  (`guard.velocity` "defaults to the x_velocity value of the ground.velocity parameter").
  It also fixed something the test-only version of the change would have introduced: a
  slam with no `air.velocity` written would have arrived in the fall state with no
  velocity at all, since 0,0 is what the air pair defaults to.
  The corpus says how much this moves. Of 2517 `ground.velocity` lines, **1304 are
  single-value** and so have a y of 0 — those are untouched by either half, and they are
  the majority. Of the 1177 that write a y, 709 write it negative (the ordinary
  knock-up), 186 write exactly 0, and **96 write it positive** — a *downward* y, a
  knock-*down*, which the old `> 0.5f` test left standing with a velocity into the
  floor. 89 of those 96 also write `fall = 1` (so they were already being knocked down
  by the other half of the OR), which is what a character writes when it is not sure the
  engine will take the hint; the ones without it are what the test half is for. 87 of
  the 96 are unique HitDefs (the rest are copies in sibling `backup/`-style folders), and
  95 of the 96 pair a *matching positive* `air.velocity` y — the duplication pattern
  that had been hiding the pair question, because on those files the two readings agree.
  Regression-tested, with the two pairs written *differently* in the test's HitDef so a
  reaction carrying the wrong one is visible: three mutations shown to bite (the test
  back to `> 0.5f`; the branch back to the air pair; and the test replaced by an
  always-true one, which fails the five "stays on the ground" controls).
  What is still open from pass 12: the `if (v10) { if (field_134 == 0.0f) field_10b0 *=
  0.66; }` quirk — a `fall` with a *flat* ground velocity gets its x damped to 0.66 —
  and the mystery gate `field_124` on the whole block, which is why the pair was moved on
  the reference's own mirror design rather than on the binary alone. Neither is
  implemented.
- `air.velocity`'s **x** default is still `ground.velocity`'s x, not the 0 the reference
  states. Only the y was load-bearing for the launch defect; the x governs horizontal air
  knockback distances, which no evidence here puts in question. Left alone, flagged here.

**A second defect this inventory found, and it reaches further than the first — the get-hit
state numbers are not MUGEN's.** The knockback defect above was invisible in the parse; this
one is invisible in the suite, because the engine's built-in reactions are consistent with
the engine's own table — every check that asks a victim which state it reached is asking
about the *engine's* numbering, and 5001 answers as consistently as 5010 would. The
reference names the numbers in the state headers of `data/common1.cns`, and the same file
says how a victim is routed. `[Statedef 5000]` is `Stand get-hit (shaking)` with `type = S`
and `[Statedef 5010]` is `Crouch get-hit (shaking)` with `type = C`, so which of the two
applies is the *victim's stance*; and 5000 takes its **animation**, not its state, from the
hit type — `value = ifelse((GetHitVar(groundtype) = 1),5000,5010) + GetHitVar(animtype)`,
which is precisely what `ground.type = Low` means on a standing victim. The knocked-back
states are not the engine's to choose at all: `[State 5000, 6] ChangeState … trigger1 =
HitShakeOver / trigger1 = GetHitVar(yvel) = 0 && !GetHitVar(fall) / value = 5001` hands a
standing shaking victim to 5001 when the shake ends, `[State 5000, 7]` hands a launched one
to 5030, and `[State 5000, 5] StateTypeSet statetype = a` is where a launch becomes an
airborne victim. The binary agrees about the *source* of the number: `hitapply` reads it off
a **statetype** field — `(0 < field_e58 - 1 ? 5010 : 5000)`, i.e. crouching ? 5010 : 5000
(`FINDINGS.md` pass 12 §12.2, whose "not settled" note the reference's `common1.cns` closes)
— with `field_124 == 3`, `ground.type` = Trip, as its one override, to 5070.

**This has since been fixed** — the paragraph below records what landed. It is kept in the
present tense above because it is the *evidence*, and the reading is what the fix rests on;
the drift catalogue it led to (which numbers were right under the name of a neighbour, which
were invented) is gone with the numbers themselves.

**Landed.** The renumbering is in, and it moved more than the numbers. The entry number now
comes from the **victim's stance** — `hitInfoFor` writes 5000 for a standing victim and 5010
for a crouching one, with `ground.type = Trip` as its one override to 5070 — which is
`(0 < field_e58 - 1 ? 5010 : 5000)` from `hitapply` and the reason `kHitLow` is 5010 rather
than the 5001 it used to be. `kHitHighKb = 5001` and `kHitLowKb = 5011` are the two knock-backs
the victim's own 5000/5010 hand over to, and they are *destinations* now rather than things
the engine picks. `kHitAirFall` is 5030 rather than the invented 5090, `kHitFall` is 5050,
`kHitAirRecover` is 5040, `kHitTrip`/`kHitTripFall` are 5070/5071, `kHitLiedownKb` is 5081,
`kHitGround`/`kHitBounce` are 5100/5101, and `kHitFallRecover`/`kHitFallRecoverGround`/
`kHitAirFallRecover` are 5200/5201/5210. The four numbers that appear in no MUGEN state
file — 5090, 5140, 5160, 5170 — are gone.

Three things beyond the numbers had to move with them, and each was a bug in its own right:

- **A launch is not a third reaction.** `hitInfoFor` used to send a grounded victim whose
  `ground.velocity` had a non-zero y straight to 5030, skipping 5000 entirely. The reference's
  ground branch writes the state *first* and never branches on the velocity at all
  (`v12->field_c4c = (0 < v12->field_e58 - 1 ? 5010 : 5000);`, with `field_b4` — `forcestand`,
  whose own default is "1 if the y-velocity of `ground.velocity` is non-zero" — forcing only
  the *statetype*). A launch is that same 5000 with the fall flag set, and 5000 is where
  `[State 5000, 5] / 6 / 7` do their work. Skipping it made every character's own
  `[Statedef 5000]` dead code for exactly the moves it exists for. The air branch had the same
  defect one family over: it is *always* 5020 (`v12->field_c4c = 5020;`, `air.type == 3` the
  only override), and skipping it made `[Statedef 5020]` dead code the same way.
- **`GetHitVar(fall)` was reading `air`.** The two are different questions — `air` is "the
  victim was off the ground", `fall` is "this hit is a knockdown" — and the reference answers
  them from different sources: the flag is resolved once, above the branch, from the HitDef's
  `fall` (`field_78 & 1`) or the caller's own flag (`v10`, still unnamed), with `forcenofall`
  (`+0xB8`) as the explicit clearer. It is what `[State 5000, 6]`,
  `[State 5000, 5]` and `[State 5080, 5]` split on. `HitInfo` carries it as `falls` now, set by
  `hitInfoFor` and reported by the trigger.
- **A hit that says nothing about the fall now leaves it alone.** The prelude above is three
  branches, not two: `fall`/`v10` set the victim's latch, `forcenofall` clears it, and the
  `else` writes *nothing* — so a victim already in a fall keeps it (`angr_p9_hitapply.c:261-269`,
  `FINDINGS.md` §17.2). Recomputing the flag from the incoming HitDef alone, as this did, meant
  a victim juggled out of a knockdown was cleared back to their feet mid-air, because 5030 hands
  over to 5040 rather than 5050 on a latch that has just been dropped. `hitInfoFor` now carries
  `victim.hitFall() && !forceNoFall` into both branches' expression — and only into the *carry*,
  so `forcenofall` can no more un-knock-down the victim than the reference's `else if` can.
- **The shakes had no shake.** These eight states — 150/152/154 and 5000/5010/5020/5070/5080 —
  are all `physics = N`, `velset = 0,0`, with `HitShakeOver` as their only exit; the velocity
  is re-applied *after* the shake by the state they hand over to, through `HitVelSet`. The
  built-in table was timing them on the pose's animation instead, and `runStateLogic` was
  holding the whole table behind `stunTicks_`, so for a character with no art for the pose the
  hold was one tick and for a launcher it lasted the whole of `air.hittime` — the victim hung
  in the pose they were hit in until the flight was over and only then began to fall.
  `builtinExitsOnShake` times those five on `shakeTicks_` (which is `p2shaketime` expired, so
  `HitShakeOver` is already the right model) and exempts them from the stun gate. The three
  *guard* shakes have since joined that set, on the same clock; see the guard-clock row below.

What is left of this slice is smaller and named:

- **5035.** The reference's 5030 hands over to 5035 on `AnimTime = 0` and 5035 splits on
  `HitOver | AnimTime = 0 | ground`, so a file-less character's hop through it is zero-length
  and a character with its own `common1.cns` runs the whole 5030 → 5035 → 5040/5050 chain
  through its own controllers, which this engine already defers to. `kBuiltins` has no 5035
  row for that reason and not by oversight. What does *not* exist is 5035's art, so adding the
  row would only change *when* the hand-over happens for a file-less character, by a tick.
- **`kBuiltins` carries no *timing* column.** A row is a state number, its statetype/physics/
  `ctrl`, an animation and a hand-over *state* — nothing says how long the state lasts, so a
  built-in state hands over the moment its animation runs out. The shakes now have a clock
  because the reference gives them one; the landing chain does not, so a file-less character
  still walks 5050 → 5100 → 5110 → 5120 one state per tick and is on its feet on the tick it
  stops, where a real character lies there for the length of its liedown animation. Filling
  the column means taking the durations from the reference's own `common1.cns`; recorded as
  remaining work in `FINDINGS.md` pass 14 §14.4, not guessed at.
- **The guard clocks — landed.** 150/152/154 are the same three states one family over, and
  their hold is the blocker's *shake* clock — which the binary names outright. `sub_46E7C0`
  feeds the blocker's clocks from the active HitDef as `+0x180` ← `guard.pausetime`'s **second
  value** (`guard.p2shaketime`, the shake), `+0x170` ← `guard.p2hittime`, `+0x174` ←
  `guard.p2slidetime`, `+0x178` ← `guard.p2ctrltime` / `airguard.p2ctrltime`; an *air* block
  writes only the ctrl clock, so `GetHitVar(hittime)` and `(slidetime)` there keep the
  *previous* hit's values. That half is **landed** (`FINDINGS.md` pass 19.4): the air guard
  branch marks `HitInfo::keepStunClock`, `applyHit` leaves `stunTicks_` untouched on that path,
  and the slidetime is carried over from the victim's previous hit, so a blocker who was not
  already stunned reports `hittime = -1` and is held by its shake (`HitShakeOver`) instead. The guard family's defaults are registered in the parser as template-slot defaults,
  documented values confirmed line by line (`angr_0047da40_hitdef_schema_parse.c:647-676`):
  `guard.slidetime` ← `ground.slidetime`, `guard.hittime` ← `ground.hittime`, `guard.velocity` ←
  `ground.velocity`, `guard.ctrltime` ← `guard.slidetime`. All three steps are in:
  1. **`guardHitTime`'s default was wrong** — it defaulted to `hit.slideTime` where the reference
     defaults it to `ground.hittime`, and `guard.slidetime` defaulted to `guard.hittime`, so
     with both unwritten the pair came out as the slide time twice over. Both now follow the
     parser, and the comment that asserted MUGEN defaulted `guard.hittime` to the slide time is
     gone. The corpus is where the old default showed: 2,544 of its HitDefs write `pausetime`,
     600 write `guard.hittime`, 224 `guard.slidetime`.
  2. **`HitInfo.hitTime` for a block is `guard.hittime`**, with `guard.ctrltime` left where it
     belongs in `HitInfo::ctrlTime`. `GetHitVar(hittime)` on a block therefore answers the hit
     clock and `GetHitVar(ctrltime)` the control clock, as the reference's two fields do.
  3. **150/152/154 are on `builtinExitsOnShake`**, so a built-in guard shake is timed on
     `shakeTicks_` — `guard.pausetime`'s second value — exactly as the five get-hit shakes are.
     The reason they had been held back is the belief that most HitDefs write no
     `guard.pausetime` and a shake timed off it would be one tick; the truth is the opposite way
     round and the roadmap had the corpus numbers reversed here: 2,544 of 2,618 HitDefs write
     `pausetime`, which is what `guard.pausetime` inherits from, and for the handful that write
     neither the reference seeds a zero clock and its shake is over on the tick it is entered.
  What the corpus test pins, in the host suite, is the clock *identity* rather than the
  reference's numbers: a HitDef writing `pausetime = 4, 10` beside `guard.ctrltime = 6` holds its
  blocker for eleven ticks — the ten of the shake clock plus the tick it is entered on — and a
  `guard.hittime = 15` beside a `guard.ctrltime = 9` reports 15 through `GetHitVar(hittime)`
  while holding for 15.
  **This question is now settled (`GUARD-CHAIN-RE.md`, `FINDINGS.md` pass 19.1-19.2):** the
  hittime decrement is at `0x46C1B1`, gated on the shake clock being zero (`0x46C1A9`), so the
  reference holds a blocker for **shake then hittime, sequentially**, not for the longer of the
  two; 150 leaves on `HitShakeOver` and 151 on `HitOver`, which is `+0x1094 < 0`. The engine froze
  `stunTicks_` during the freeze already, so its ordering was right; what was wrong was that the
  clock stopped at 0 and `hitover` fired on `== 0`, and that `GetHitVar(hittime)`/
  `(hitshaketime)` reported the HitDef's constants instead of the live counters. Both are fixed
  and the host test now pins the length (11 shake ticks + 13 knock-back ticks for this fixture).

Two smaller questions were recorded here rather than acted on. One has since been settled and
landed; the other has been narrowed and remains open.

- ~~**Does `air.fall` default to `fall`?**~~ **Yes — landed.** The docs say so ("Defaults to
  the same value as `fall`"), the air branch of `hitapply` sets the fall flag again from its
  own field (`if (v11->field_148) v12->field_10c8 = 1;`, the only slot left for `air.fall`,
  which the offset table in `FINDINGS.md` names `+0x148` and flags as "not its name"), and
  the *filler* settles the shape of it: the fall bit is written once, by the `fall` parameter
  alone (`angr_0047f380_at_00480308.c:305`, `if (sub_439260(1)) a1->field_78 |= 1;`), with no
  implicit derivation beside the one `forcestand` gets at `:385-386`. So the two parameters
  are one intent spelled for two statetypes, and `HitDef::airFall` now takes `fall`'s value
  when `air.fall` is not written. Written it still wins the default and loses the flag:
  `fall = 1` is tested first in the prelude, so `air.fall = 0` beside it takes back only the
  default. Three checks, two mutations, both bit.
- **What is `hitapply`'s caller flag `v10`?** Still open, and the earlier reading here —
  that it is the ground pair's y — is **dead**. That derivation belongs to `forcestand`, which
  the filler writes at `:385-386` (`if (ground.velocity.y != 0.0f && !a0[101]) a1->field_b4 =
  1;`), and `field_78`'s fall bit is not derived at all. `v10` is instead `[bp+0x14]`, the
  caller's fifth argument, and the caller *computes* it as a 0/1 from a distance test between
  the two players — `angr_p9_hitresolve.c:244-271` (`v3 = 0;` … `v3 = 1;` … `v3 = 0;`, and
  `if (v23->field_10ee8) v3 = 0;`) — and passes it at `:419`; the other call site passes a
  literal 0 (`angr_p9_pair_writer.c:383`). Since the prelude runs *above* the statetype
  branch, that one expression is the flag for a grounded and an airborne victim alike, so
  this decides both branches at once — and the other two uses of `v10` are consistent with a
  launch rather than a distance: it forces the stand type at `:287` and gives the launch a
  minimum push at `:317` (`x *= 0.66` when the hit is flat, `y` clamped to at most `-6`).
  Naming it needs a dedicated pass on `hitresolve`'s reach test. Until then the air branch
  keeps this engine's own term, `launches` — an upward velocity reads as "off their feet" —
  and that is a deliberate stand-in for a term the artifacts have not yet named rather than a
  reading of the reference. The size of the question: of the **1,466** corpus HitDefs that
  write an upward `air.velocity`, 622 also write a non-zero `ground.velocity` y, 344 write
  `fall = 1`, 88 write `air.fall = 1`, and **671 write none of the three** — so dropping the
  heuristic would be a decision about 46% of the corpus's air launches, on evidence that does
  not yet name the term.


Note the ranked-gap rows that were wrong in a way worth remembering: rows 1 and 2 claimed
the helper/parent/root graph was absent and that its redirects resolved null. They resolve
(`root`, `parent`, `helper(n)`, `ishelper`, `numhelper`, `parentdist`, `rootdist` are all
live), and helpers spawn as full fighters running their spawner's own states.

## Sprint slices

Each slice = implementation in `app/src/main/cpp` + focused diff-check against `FINDINGS.md`
semantics + build (`./gradlew :app:assembleDebug`) + on-device sanity (respect the shared-device
rule: verify our app pid before trusting any device test).

- **S2 Timestop**: ✅ landed (2026-10-18). PauseInfo lives on EntityLayer (one hand on the
  clock, second pause replaces first); Pause/SuperPause controllers real (time/movetime/
  darken/p2defmul/poweradd/unhittable/anim-flash-fightfx/sound); step() gate with actor
  movetime + helper pausemovetime/supermovetime budgets; explods+projectiles freeze except
  `ignorehitpause`; timer holds; darken overlay; p2defmul scales victim damage in applyHit;
  trigger hitpausetime. **Pause verified against the binary (RE pass 9)**: `engine+0x10EBC` is
  the **user pause** latch, a per-frame mirror of byte `0x52B950` (`0x44B51A`/`0x44B521`,
  toggled by the pause key and by `xorb $1,0x52B950` at `0x4149ED`), and while it is set the
  per-frame gameplay update and the palette-effect apply are gated off. It is **not** hitpause
  (`player+0xE70`) and not the round-start hold (`engine+0x11720`); Pass 7's "global
  pause/hitpause freeze" guess was wrong for both engine flags — `engine+0x11700` is the
  round-end **slow-motion** accumulator, not a freeze.
- **S3 HitDef schema (first pass)**: ✅ core landed. attr parse (class/type, normalized text,
  throw=unblockable, projectile flag); hitflag letters (MAF default per binary pass 5,
  `+`/D = downed, subset semantics verified at eligibility gate incl. projectiles);
  fall/air.fall/fall.recover/fall.recovertime/fall.damage/fall.xvelocity/fall.yvelocity
  (bounce on land) + fall.envshake.*; numhits alias; p1stateno/p2stateno throw flow;
  hitdefattr trigger real (class ⊆ probe, type == probe, parser folds `= SCA, NA`);
  canrecover; hitfall + HitFallSet; HitAdd; MoveHitReset (re-arms spent HitDef).
  guard.pausetime←pausetime copy rule verified against binary (already matched).
  `p1facing`/`p1getp2facing` landed since (the pair, read as one bit plus a tri-state;
  see the corrected-rows list). **`p2getp1state` is now landed** (FINDINGS pass 19.3): the
  documented default of 1-when-`p2stateno`-is-written is confirmed in the schema parser, the
  victim borrows the attacker's *whole* state library (the `-1`/`-2`/`-3` lists included) for
  as long as it is in the custom state, draws from the attacker's animations, and returns home
  on a hit or a `SelfState`. Still open from this slice: multi-target lists. `FINDINGS.md`
  §11.11 establishes what the victim-side flag at `+0x10C8`, which the old table read as this
  parameter's name, actually is — the victim's **fall/knockdown reaction flag**, with three
  writers and two readers, of which `p2getp1state` (attacker `+0x148`, confirmed HIGH as a name
  and an offset) is one.
  **Landed since:** the `animtype` / `air.animtype` / `fall.animtype` and `ground.type` /
  `air.type` family — the three-tier cascade, its chained defaults (DiagUp carrying through,
  read off the binary against the documentation), the launch case sharing the implicit-launch
  condition, and all three `GetHitVar` type keys now reporting MUGEN's own numbering. See the
  corrected-rows list for the corpus sizing and the four mutations. It also *sharpened* the
  next gap rather than closing it: with `GetHitVar(groundtype)` finally correct, the one
  thing standing between the corpus's characters and their own low-hit animation was the
  engine's state numbering — which has since **landed** (the finding below), so a character's
  `common1.cns` now receives the reaction its own 5000/5010 draws.
  **Landed since:** `down.*` and `airguard.*` — all five fields parsed on the pass-7
  defaults, and both *consumers* written. `down.velocity`'s y is the switch and not a
  magnitude: zero slides the victim along the floor into the hit-while-down state for
  `down.hittime`, and any non-zero value puts them back in the air for the air clock
  instead, with `down.bounce` deciding whether the landing comes back off the floor (the
  engine's ordinary fall bounces whenever its fall velocity points up, so a downed hit
  that did *not* ask to bounce has to hand over no bounce velocity at all, or the
  parameter would mean nothing). `airguard.velocity`/`airguard.ctrltime` are read by the
  guarded branch *by height*, which is the whole point of the family: an airborne blocker
  was previously shoved by the standing `guard.velocity` for the standing `guard.ctrltime`.
  Regression-tested (parse, both reactions, and a ground control on the same HitDef so the
  checks are about the height rather than the parse), five mutations shown to bite.
  Still not read out of either family: `down.cornerpush.veloff` and
  `airguard.cornerpush.veloff`. A simplified corner push now exists — when the victim is
  within 12 px of the wall, `Fighter::tryHit` nudges the attacker once by `1.3 × pushX`
  (fighter.cpp:424–436) — but it does not model the reference's `cornerpush.*` slots, so
  these two `veloff` spellings have nothing to feed. That whole mechanism is the
  remaining gap; the two parameters are its `down`/`airguard` spellings rather than a
  family of their own.
  **Landed since:** the *guard reaction chain* — the other half of the `guard.*`/
  `airguard.*` slice, and the same defect one state over. A block was written as a single
  built-in row per height, statetype `Stand`/`Air`, `movetype = I`, `physics = S`/`A`, and
  handing the blocker to state 0 when it ended: the blocker slid away from the contact tick
  and was back on its feet (or, for 152, standing *up* out of its crouch) before the guard
  clock had run. `common1.cns` writes six states instead, walked in order — the shake
  (150 standing, 152 crouching, 154 air) holds the blocker still, the knock-back beside it
  (151, 153, 155) is where the push is applied, and each ends in a guard *hold* (130, 131)
  rather than at 0. Three things are load-bearing and all three are now in the table and the
  hand-over: the shake is `movetype = H`, `physics = N` and `velset = 0,0` (so `applyHit`'s
  guarded branch zeroes velocity instead of applying `pushX`, which it kept in `lastHit_`
  for the knock-back's `HitVelSet x = 1`); which knock-back the shake ends in comes from the
  *input* held as the shake ends, not from the posture it was entered in (`value = 151 + 2 *
  (command = "holddown")` in 150 and 152 alike); and the air knock-back's landing chooses
  between the standing guard hold and 52 by "holding back inside guard distance"
  (`[State 155, 6]`/`[State 155, 7]`, the engine's `holdingBack() && inGuardDist_`). Two
  consequential adjustments fell out of it: `canGuard()` now returns early for a guard state
  (the reference's guard-hit states are `movetype = H`, so the movetype/hitstun test must not
  sit above the guard-state exemption, or a blocker could not block the second hit of a
  string), and `isGuardState()` covers the three knock-backs as well as the three shakes.
  Regression-tested end to end (stand 150 → 151 → 130 with the blocker not moving a unit
  during the shake; crouch 152 → 153 → 131 staying crouched; *input*-driven height, a
  stand-blocker that presses down ending in 131 through 153; air 154 hanging under
  `physics = N`, then 155 → 130 with `inGuardDist_` set and 155 → 52 with it clear), and five
  mutations shown to bite. **Recorded as open from this slice:**
  - the guard *hold* does not track height: 130 and 131 do not swap on `holddown` the way the
    reference's own guard-hold states do, so a blocker that crouches mid-block stays in 130's
    posture until it releases;
  - **140** (`kGuardHold`, "the pose a block leaves through") is still never entered — the
    engine owns the release directly (`130`/`131` → `kStand` when back is let go), where the
    reference's 130 hands over to 140 and 140 is written with no exit of its own;
  - **132** (`kGuardAir`) is still never entered either: an airborne blocker that is *not* in
    a reaction holds its block without a state (the entry needs 120 to exist in the file),
    and the air guard hold proper is 132;
  - and one open RE question, deliberately resolved the documented way rather than guessed:
    whether the reference refuses a *new* block while the victim is already in a guard-hit
    state. This engine says yes (the early-return in `canGuard()`, on the strength of the
    shake being `movetype = H`), which is what makes a blocked string stay blocked; the
    binary's `sub_46DB20` was read for it and its guard condition was not pinned down — the
    `v4 = 1` in the earlier decompile note is the HitOverride slot loop, not the guard
    decision (`docs/mugen-re/decompiled/angr_p9_hitresolve.c`).
  **Landed since:** all three of that bullet list's first three open items — the guard
  *hold*, the guard *end* pose, and the air guard hold — which turned out to be one defect in
  three places: the engine answered a question the reference answers in its own file, and
  answered it differently. Read off `data/common1.cns` states 120–155, the chain is one
  sequence of controllers: `120 GUARD (start)` is `type = U, physics = U` and picks its pose
  by posture (`ChangeAnim 120 + (statetype = C) + (statetype = A)*2`), its `Hi to Lo`/`Lo to
  Hi` pair moves the stick between the two guards *in place* (a `StateTypeSet`, not a
  `ChangeState`, so the pose is not restarted), and on `AnimTime = 0` it hands to
  `130 + (statetype = C) + (statetype = A)*2`; the holds 130/131 swap on
  `command = "holddown"` / `command != "holddown"`; and **every** one of 120/130/131/132
  carries the same `Stop Guarding` pair — `command != "holdback" || !inguarddist -> 140`.
  - **130 ↔ 131 swap landed.** Crouching mid-block no longer waits for the release: the hold
    re-reads the posture every tick and changes state to the other hold. It reads the
    *command* rather than the stick (`directionHeld` consults the character's own .cmd first
    and falls back to the raw key only for a command the file does not declare), because that
    is what the reference tests — and one pack in the corpus rebinds `holddown` to a button
    (`mugen_sai/Sai.cmd:371`), so a raw-stick test would swap its guard on the wrong input.
  - **140 landed** (`kGuardHold` renamed `kGuardEnd`, which is what the file calls it: `Guard
    (end)`), and it is now what a release *goes through*: letting back go, or the attack that
    was being blocked ending, leaves 130/131 into 140, which is the one guard state the
    reference writes `ctrl = 1` into. Its own exit is the engine's by the file's own
    admission — `;[State 140, 5] ;Implemented within engine` with `ChangeState / value =
    (statetype = C)*11 + (statetype = A)*51` commented out beside it — so the pose plays out
    and the fighter is then stood up, stood *down* into 11, or put into 51 in the air.
    `130`/`131` → `0` directly, as this did, skips the pose and gives control back a state
    early. Because 140 is not a state a block is *held* in, `isGuardState()` no longer counts
    it, which is also what lets a blockstring come back up through 120 out of it.
  - **132 landed**, and with it both halves of the *physics* rule the family turns on. 120 and
    140 each write the same trio — three `StateTypeSet`s, `trigger1 = Time = 0 && statetype =
    S` / `C` / `A` with `physics = S` / `C` / `A` — because a pose that keeps the posture it
    was entered with still needs the *physics* belonging to that posture; a fixed row in
    `kBuiltins` could say neither. 132 asks the same question with a different answer: `type =
    A, physics = N` with its fall written out by hand (`VelAdd y = Const(movement.yaccel)`
    every tick, and the `sysvar(0)` landing test `(pos y >= 0) && (vel y > 0)`, whose two
    outcomes are 130 while the blocker still holds back inside guard distance and 52
    otherwise). The engine's substitute is `Physics::Air` plus `land()`'s existing call on
    that same pair of triggers — the identical trade 155 already makes for the same reason.
    Entering 120, 132 or 140 now sets `physics` from the state type rather than inheriting it,
    so a fighter that blocked in the air and let go no longer hangs there with its arms coming
    down. That also turned up a *documented-parameter* bug worth recording: `StateTypeSet`'s
    `physics` was parsed and thrown away, so a character whose own 140 carries
    `[State 140, 4]` hung for exactly the same reason. `sctrls.html` is explicit that all three
    of its parameters are optional and each "defaults to no change", which is the sentence the
    bug violated.
  - The gate that keeps all of the above from overruling a file: `scanGuardChain()` sets
    `scriptedGuardChain_` when the character defines **both** 130 and 140 — the state a block
    is *held* in and the state its exit goes through — and the four built-in hand-overs then
    stand down. They are the engine's copy of `common1.cns`'s controllers, so a character that
    ships them has its own; two packs in the corpus rewrite the chain deliberately
    (`mugen_teen_gohan` comments `!inguarddist` out of 130; the dbz pair end in a terminal 131)
    and both are now respected rather than overruled. `140`'s exit stays the engine's for
    everyone, because the file says so in as many words, and so does the guard *entry*, which
    the engine only takes where the character has a 120 to enter.
  **Landed since:** `inguarddist` became the reference's own quantity rather than the
  engine's approximation of it — the last of that batch's open items, and the one that made
  the guard *release* above more eager than the reference's. `trigger.html` defines the
  trigger in a sentence: "the player is within guarding distance of an opponent's physical
  or projectile attack. The guarding distance is the value of the `guard.dist` parameter of
  the opponent's HitDef." Three things follow from it and all three are now the engine's:
  - **The distance is a number, not a reach.** `guard.dist` is parsed off the file and the
    comparison is the reference's own, read off the binary instruction by instruction rather
    than inferred from the docs (see the "consumer" note below). No box is consulted, which is
    what the old reading got wrong in both directions: a slow spear whose box is not out until
    frame 30 gave the defender no warning at all on frames 1–29, and a long attack whose box
    reached far made its owner guard-worthy from the box's edge even before the move came out.
    The corpus makes the difference concrete: 102 write-sites across 8 packs, 90 of them
    `guard.dist = 0` — which is the *deliberate* "this move is not guardable from distance", in
    kfm's template's own words ("This prevents p2 from going into a guard state if close") and
    copied into Gogeta and twelve other packs.
  - **The omitted case is a player variable, and there are two of them.** `sctrls.html` only
    says `guard.dist` "defaults to the value in the player variables"; the names are the pair
    in `[Size]`, `attack.dist` (160) and `proj.attack.dist` (90), which is the two-case
    sentence above twice over. The corpus supplies the gloss on both: seventeen packs write
    the pair, and the ones carrying Japanese comments read `attack.dist = 160 ;相手がガード態勢
    に入る最低の距離` and `proj.attack.dist = 90 ;自分の「Projectile」に対して相手がガード態勢に
    入る最低の距離` — "the minimum distance at which the opponent enters guard posture", and
    the same "against your own Projectile". Both parse under `Const`, both were already
    seeded from Elecbyte's template, and neither had a consumer.
  - **A projectile counts, from its own position and at its own distance.** 90 rather than
    160 is what makes a fireball blockable on approach while its owner stands in a recovery
    state with nothing armed — a case the box-based reading could not see at all. The
    projectile half of the reference's test carries no `movetype` gate at all, which is the
    same point from the other side: the scan runs whether or not the fighter that fired it is
    doing anything. The reach is scaled by the **owner's** art scale and not by the
    projectile's own `projscale`, which is what the reference does — the 90 was written in the
    character's coordinates and the comparison is in the world's, exactly as for the melee
    half. (That a projectile's *collision boxes* are scaled only by `projscale` and not also
    by the owner's fighter scale is a separate question, left where it is: see the `projscale`
    note below. So this engine's guard distance and its projectile boxes are scaled by two
    different factors, and that divergence is the reference's, not a simplification of it.)
  `AttackDist` landed with it, because it is the same number by another route: `sctrls.html`
  calls it "Changes the value of the guard.dist parameter for the player's current HitDef" —
  a *controller*, not a HitDef parameter, which the engine had been ignoring outright. 124
  uses in the corpus, all `trigger1 = 1`, which is what makes "current" work: the ones written
  *before* their state's HitDef still land on it, on the tick after it armed rather than on
  the tick it did, and 14 sites (jolyne 1001 and 1007, s_diavolo 1150/1700/1750) are exactly
  that shape. The other 110 sit in states that arm nothing — a `[State 901, Throw]` block the
  ASB packs copied wholesale out of a throw state and never re-pointed, and they are harmless
  rather than pointless: the controller writes the player's *live* guard distance, so a state
  whose `movetype` is A still answers `inguarddist` with it while it arms nothing at all. The
  gate is the reference's `movetype = A`, and no longer this engine's `hasArmedHitDef`, which
  was the more permissive of the two readings — 38 of the corpus' 2219 HitDef-bearing states
  arm one without writing `movetype = A`, and under the reference's rule those 38 are not
  attacking, so they do not threaten. That is the whole distinction the `movetype` gate draws:
  it asks whether the fighter is *in an attack*, not whether a box is armed.
  Regression-tested in `tools/hosttest/test_statemachine.cpp` (a second guard block: the
  unwritten 160 past a short attack's box and its 200-unit edge, a written 50 and its scaling
  with the fighter, a written 0 over an *overlapping* box, an opponent standing *behind* the
  attacker at five units, a `movetype = A` state with nothing armed, an armed HitDef in a state
  that is not attacking, `AttackDist` writing the player slot outright, and a beam at 30, at
  120, past its target, and at twice the firer's scale), with seven mutations each shown to
  bite: dropping the lower bound on the melee half (1 check), on the projectile half (1),
  dropping the `movetype` gate (1), giving beams the melee 160 (1), unscaling the projectile
  reach (1), an `AttackDist` that writes nothing (3), and the accessor answering 0 while
  nothing is armed (1, the wind-up case), plus a state-entry seed of 0 rather than
  `size.attack.dist` (4). Host-tested only — no device run for this batch.
  **Still open from that batch, flagged not guessed:** 132's own `VelAdd` is approximated by
  the engine's whole `Physics::Air` — fine in effect, but the engine's gravity is
  `[Movement] yaccel` rather than the `Const(movement.yaccel)` the file names, and the file's
  `PosSet y = 0`/`VelSet y = 0` on landing are `land()`'s — and `[State 130, 1]`'s
  `ChangeAnim trigger1 = Anim != 130 value = 130`, the hold's pose *restore*, which fires if
  anything else moved the animation during the hold, is not modelled, since the engine sets
  the pose once on entry and nothing else changes it.
  **The `guard.dist` consumer, settled:** pass 12 recorded the field at HitDef **+0x17C** by
  schema position only, "no consumer seen", MED, which left the whole slice above resting on
  `trigger.html`/`sctrls.html` and on what the corpus says the two player variables mean. A
  pass on the binary has since landed and answered all three of the open questions it left:
  - **The compared quantity is signed and one-sided, not an absolute distance.** One read site
    for the field — `fildl 0x132c(%esi)` at `0x464F79`, inside `sub_464F10`, reached from the
    trigger evaluator as case 36 at `0x486E44` through the wrapper `sub_464EA0` that loops the
    player slots — and the test it feeds is
    `0 <= (self.x − opponent.x) * opponent.facing <= opponent.guard.dist * charScale`.
    No `fabs`. An opponent standing *behind* the attacker is out of guarding distance however
    close it is, because the move is going the other way; the lower bound is a real bound and
    not an artefact of an unsigned compare. The engine now computes exactly that, and the
    "behind at five units" check is the regression test for it.
  - **The bounds are inclusive at both ends** (`<=` on both), so a defender standing exactly at
    the written distance is in.
  - **The omitted case really is the player variable, and the reference carries the same two
    constants.** `guard.dist` is a *player-level* slot (`player+0x132C`) with exactly two
    writers: the state change at `0x48B7FD` seeds it from `player+0x68` — which is
    `size.attack.dist` — whenever `hitdefpersist == 0`, and the HitDef executor at `0x478E88`
    does a single conditional store of `parsed == -1 ? old : parsed`, so a HitDef that omits
    the parameter leaves the slot exactly as it was rather than resetting it. The projectile
    spawn at `0x4817BE` substitutes `player+0x6C` — `size.proj.attack.dist` — in the same way.
    The two constants the reference ships are `0x4A6D74` = 160.0f and `0x4A6D70` = 90.0f, and
    the keys are `attack.dist` at `0x4A27A0` and `proj.attack.dist` at `0x4A27AC`. So the
    engine's `kDefaultAttackDist`/`kDefaultProjAttackDist` pair, its state-entry seeding and
    its "omission leaves it alone" are all the reference's, not a reading of the prose.
  - What the binary adds beyond the prose, and what the engine therefore had wrong even after
    the first rewrite: the gate on the **direct half only** is `movetype == A` (`0x464F31
    cmpl %ecx,0xe5c(%esi)` with `ecx = 1`), the projectile scan runs **regardless of the
    owner's movetype**, and **no HitDef has to be armed** for either. The parameter is a
    distance with a state-entry seed, not a property of an attack.
  **Landed since:** `chainid`/`nochainid`/`nochainid2` — parsed with the pass-11 defaults
  and collision rule, recorded on the victim in `applyHit`, and gated in `hitMayReach`
  against the victim's own last-hit id (so a chain link's history is the victim's, not the
  attacker's). A *block* deliberately does not record an id, since the parameter reads "last
  hit by". Regression-tested, five mutations shown to bite. `envshake.*` at hit time and
  `getpower`/`givepower` were already landed when this note was rewritten.
  **Landed since:** the implicit launch rule — a standing victim goes into the air when
  `ground.velocity`'s y is **non-zero** (not when it is *upward*), and carries
  `ground.velocity`'s own pair rather than `air.velocity`'s. Both halves are the
  reference's and the binary's, and the reasoning, the corpus sizing and the three
  mutations that were shown to bite are in the "defect this inventory found" subsection
  above; the `down.*`/`airguard.*` parse it sits beside is that note's other bullet.
  **Landed since:** the fall landing's ownership and the `GetHitVar(hitcount)` counter — both
  cases where the engine carried a *plausible but wrong* answer rather than a missing one. The
  landing's three effects (fall damage, bounce velocity, screen shake) are **script** in MUGEN:
  the character's own `common1.cns` spends them from 5100/5101/5110, so an engine landing applied
  on top of a file's own doubles the charge *and* drains `GetHitVar(fall.yvel)` before the file
  reads it, skipping the file's own bounce state; the engine's landing now stands down per effect,
  gated on `StateLibrary::usesController` (35 of the 41 packs script at least one). And
  `GetHitVar(hitcount)` is the *victim's* count of hits in the current combo, not the attacker's
  standing `hitcount` trigger; the engine answered a flat 0, so damage scalers
  (`Enemy,GetHitVar(HitCount)`) multiplied by zero and combo counters never left their first hit.
  Both are implemented in `app/src/main/cpp/engine/statemachine.cpp`; the reference evidence, the
  sign convention and the corpus sizing are in `FINDINGS.md` passes 14 and 15. **Remaining,
  flagged not guessed:** the built-in landing still has no *duration* — `kBuiltins` has no timing
  column — so a character with no `common1.cns` walks the landing chain a state per tick and is
  standing on the tick it stops; see pass 14 §14.4.
  **RE pass 11 settled the *field-layout* half of four of these** (from the active-block
  filler `sub_47F380`) — see `FINDINGS.md` pass 11 §11.4–11.8: chainID/nochainID/nochainID2
  at block **+0x18/+0x1C/+0x20** (default −1, collision rule zeros all three); damage,
  getpower and givepower at **+0x38/+0x3C**, **+0x40/+0x44** and **+0x48/+0x4C**
  (hit/guard pairs); mindist/snap at **+0x110/+0x114** and maxdist/snap at **+0x118/+0x11C** (default
  −1025); the envshake record as 5 ints at **+0x1DC..+0x1EC** (+ptr at +0x1F0) copied to
  `engine+0x10F28..`. What remains open for each is its *runtime consumer* (the chain test,
  the meter arithmetic, the placement ops), not the block layout.
  RE pass 7 pinned the `down.*`/`airguard.*` field set from the parser at `0x47DA40`, with
  record offsets and defaults: `down.velocity` (def = `air.velocity`), `down.bounce` (def 0),
  `down.hittime` (def 20), `down.cornerpush.veloff` (def = `air.cornerpush.veloff`),
  `airguard.velocity` (def = `air.velocity`), `airguard.ctrltime` (def = `guard.ctrltime`),
  `airguard.cornerpush.veloff` (def = `guard.cornerpush.veloff`). Four names this engine
  was treating as HitDef fields are **not**: `down.bounce.offset`/`.yaccel`/`.groundlevel`
  and `down.friction.threshold` are movement constants. `p2getp1state` is **not** "default 0":
  Pass 7 read the slot's initial value, but the schema parser registers its default
  conditionally — it pushes pending defaults `-1, -1, 0` and, on finding `p2stateno`, pushes
  **1** before parsing it (`FINDINGS.md` pass 19.3). The effective default is therefore 1
  whenever `p2stateno` is written, and 0 (ignored) otherwise, exactly as the docs state.
  RE pass 9 pinned what happens at *hit time* (`sub_46D2D0`, `0x46D8FC–0x46D97D`, one straight
  line). The active HitDef block is 500 bytes inline at `player+0x11B0`; the two statenos are
  read from **+0x84** (applied to the **attacker**, through `ChangeState` at `0x46AF10`) and
  **+0x88** (applied to the **victim**, followed by `ctrl(+0xE64) = 0`), each meaning "no
  change" when the value is `-1`. The owner record pair is a **separate, `+0x8C`-gated copy**
  (`sub_48BB50`, called at `0x46D929`): attacker `+0x1240..0x124C` →
  victim `+0xC38/+0xC3C/+0xC40/+0xC44`, with that record's id echoed to victim `+0x9B0` and the
  128-byte command buffer at victim `+0x9B4` cleared whenever the id changes. `p2getp1state`
  does **not** drive that copy: it sets `victim+0x10C8 = 1` in `sub_46DB20`, and `+0x10C8` is
  what the placement/knockback path then tests — so both are needed for parity.
  `ChangeState` resolves its owner through the chain `+0xC3C` else `+0xC34` before writing
  prevstateno/stateno/time-in-state. Field **+0x13C is not a state**: it is a small
  fall/animation enum — `sub_46DB20` copies it to `victim+0x107C` and forces state 5070 when it
  is 3 — so Pass 7's `p2stateno`/`p1stateno` template offsets (+0x13C/+0x130) are superseded by
  these +0x84/+0x88 reads.
- **S4 Target system**: ✅ landed (2026-10-18). lastTarget_ on contact (landed, not guarded);
  target redirect + bare-target existence trigger; numtarget; TargetAdd (playerid lookup),
  TargetBind (victim bindTo via shared applyBind path), TargetDrop, TargetFacing,
  TargetLifeAdd (currently never kills — kill param TODO), TargetPowerAdd, TargetState,
  TargetVelSet/Add; sweep clears references on destroy. Open: multi-target lists,
  TargetBind facing param.
- **S5 Combat edge (first pass)**: ✅ landed — HitFallSet+hitfall, fall bounce/damage,
  HitAdd, MoveHitReset, **HitOverride** (attr-mask slot array, stateno redirect, time/forceair
  defaults per binary), **ReversalDef** (capture with masks, pausetime pair, movereversed
  trigger live), AttackMulSet/DefenceMulSet, ChangeState `anim` param (binary pass 6),
  statedef *persist flags (movehitpersist/hitcountpersist/hitdefpersist), ChangeAnim2
  (attacker-file pose onto target, sprite bank follows), snap, mindist/maxdist, p2facing, p1facing/p1getp2facing,
  envshake.* at impact, getpower/givepower.
  Still open here: chainid/nochainid bookkeeping inside juggle logic, down.* family,
  airguard.* family, p2getp1state — RE pass 9 settled its mechanism (`victim+0x10C8 = 1` plus
  the separate `+0x8C`-gated owner-record copy, see S3), so what is left is wiring it across
  files.
- **S2.5 extras**: ignorehitpause controller flag honoured during hitpause (filtered -1/-2/-3
  + current-state controllers run while frozen); persistent=n per-state-entry gating;
  PlayerPush/ScreenBound/PosFreeze/Gravity real; EnvColor overlay (under + over);
  palno=1, roundno wired; AfterImage trails (ghost ring in Fighter, knobs from the
  AfterImage/AfterImageTime controllers, fade+blend, palette grading approximated);
  roundstate/win/lose/drawgame/winperfect/ingame wired from the fight loop.
- **S6 Palettes**: ✅ landed — indexed-upload + LUT palette pipeline (GL_R8 sprite indices,
  256x1 RGB LUT per palette, `uPalette`/`uUsePalette`/`uPalMul`/`uPalAdd`/`uPalColor`/
  `uPalInvert`) driving PalFX/RemapPal/palno and ACT character palettes. Verified on device:
  SFF v1 and v2 characters both render in correct full colour on the select screen and in a
  fight, 59.5–59.9 fps. **SFF v1 `.act` index reversal** — a v1 sprite's own palette and the
  character's `.act` hold the same 256 colours permuted by an exact full reversal
  (`sprite[i] == act[255-i]`, 256/256 for 25 of the 27 roster characters that have both);
  `character.cpp` now flips a v1 `.act` on load. Evidence and a re-runnable check:
  `tools/palcheck.py`. **BGPalFX/AllPalFX landed** — the two engine-global grades live on
  `EntityLayer` and are reached through `EntitySink`, applied around the stage and then the
  fighters/effects in `fight_screen.cpp`; host-pinned in `test_statemachine.cpp`. Honest
  limits: only the indexed/LUT path is graded (true-colour sprites, `fillRect` gauges and
  fonts are not), the procedural test arena is `fillRect`-based so BGPalFX is invisible in
  it, and stacking a global grade with a fighter's own is exact only when one is identity.
  **Selecting an alternate palette from the select screen landed** (the data and the
  `palno` lookup existed, but nothing reached them from the UI):
  `Character::paletteChoices`/`palNoForChoice` fold
  the two sources of alternates (a `pal1..pal12` set of `.act` files, or the palette nodes
  an SFF v2 carries) into one 0-based choice numbering, skipping empty slots, so a UI
  cannot offer the same picture twice; `defaultPaletteChoice` puts the cursor on the
  colour `pal.defaults` names rather than on the first alternate. `Start` held turns the
  d-pad into the picker (`Start` alone confirms on *release*, and a `Start` that was
  chording does not also confirm; the six attack buttons still confirm on the press,
  since `InputState` carries no touch coordinates and a tappable swatch is not available).
  The choice travels as a choice index to `SelectScreen::launch`, is resolved to a palNo
  against the cell there, and is re-applied by `FightScreen::applyChosenPalettes` after
  **every** `Fighter::setup` — setup resets `palNo_` to `pal.defaults` and runs at the top
  of each round, so an earlier application would have survived exactly one round. A
  `randomselect` roll resets to the rolled character's own default, since the player never
  saw its alternates. **Open:** RemapPal's group half; and this is build- and
  host-verified only — no device run yet.
- **PalFX semantics corrected against the binary** (RE pass 7, `docs/mugen-re/FINDINGS.md`):
  `time` is a **hold**, not a fade — full strength for the span, then the slot resets to
  identity; there is no interpolation (the linear decay this engine had was invented and has
  been removed, along with `PalFxState::total`). The grade is 8-bit integer, not float:
  `clamp(mul * ((colorBlend(C) ^ invertmask) + add) >> 8, 0, 255)`, so the order is
  **color → invert → add → mul** (the shader had mul and add first and inverted last), the
  luma for `color` is MUGEN's fixed point `(5R+7G+4B)/16` rather than Rec.601, `invertall`
  is a per-channel `XOR 0xFF`, and `mul` is 0..256 with 256 the identity. Three separate
  slots exist: `palfx` per player, `bgpalfx` and `allpalfx` global to the engine — all three
  are implemented here (see the S6 entry above; the two globals landed after this pass).
  There is no `PalCopy` controller in the binary. RE pass 9 added the **scope match**: an effect
  is applied per draw object iff **any** dword of `(entryKey & selector)` is non-zero, scanning
  the array at `engine+0x80C0` (header `{+0x0 base, +0x8 count}`, stride `0x18`, entry
  `{+0x0 drawobj, +0x4..+0x14 = 5-dword layer key}`). The BG selector is literally
  `{0x100,0,0,0,0}` (built at `0x4473A5–0x4473BD`), so the BG test is `entryKey[0] & 0x100`
  (that bit 8 of key dword 0 *means* "background plane" is MEDIUM confidence), and the
  per-player selector is `player+0x1424..+0x1434`, read at `0x44745D–0x44748E`. AllPalFX is the
  **screen-global** path (`a6 = 1` at `0x4474E1`) and does no matching at all — it writes
  `0x52B34C` through `sub_413D50` (`+0x4/+0x8/+0xC`) and `sub_413D80` (`+0x10/+0x14/+0x18`),
  each flagging the render state dirty only when a cached value actually changes, so the two
  globals are a different code path rather than "a scope with every bit set". The effect's own
  aging stops under round-end slow motion (`engine+0x11700`) and the apply is skipped entirely
  under user pause (`engine+0x10EBC`).
- **Animation parameter prefixes** (RE pass 10, `docs/mugen-re/FINDINGS.md`): the one-letter
  prefix that says which file a controller's animation comes from was being ignored, and
  because this engine's expression parser turns an unknown identifier into trigger `0`, the
  prefixed forms silently became **animation 0** — every character's stand. Two conventions
  exist and they are mirror images: `Explod`'s `anim` / `Projectile`'s `projanim` use `F` for
  the screenpack's fightfx (bare = the character's own), while a `HitDef`'s `sparkno` and a
  `SuperPause`'s `anim` are the other way round (bare = fightfx, `S` = the character's own).
  Fifteen `Explod` sites across ten roster characters write the `F` form — mostly super-jump
  landing dust — and each was drawing a second, additive copy of the fighter instead of the
  dust. That is the far-left gold artefact seen on the device: `postype = back` is resolved
  once to a fixed world point rather than tracking the screen, so the copy slid to the screen
  edge as the camera moved. `animTarget`/`AnimPrefix` in `statemachine.cpp` now read the
  prefix off the raw text; a malformed prefix draws nothing rather than animation 0 of the
  wrong file. Wired into `Explod` and the `SuperPause` flash (the latter had been forcing
  fightfx unconditionally, so every `S`-prefixed flash — `S3102`, `S6039`, `S-1` — resolved
  to 0 as well). Pinned by a 19-check regression block in
  `tools/hosttest/test_statemachine.cpp`, verified to bite (8 failures against a copy of the
  engine with the prefix branch disabled), and one of those checks corrected this very note:
  a *malformed* prefix does **not** leave the animation unnamed — `anim = Fxyz` is a bare
  four-letter lower-case identifier, which MUGEN answers as a string literal, so it lands on
  animation 1 of the character's own file. The property the fix secures is narrower, and is
  the one the bug was about: it can no longer land on animation **0**, the stand, out of the
  wrong file.
  **`Projectile` was the half of this that was left open, and it is now closed.** All three
  of a projectile's animations — `projanim`, `projhitanim`, `projremanim` — take the same
  `F` convention, are resolved one at a time, and each carries its own "which file" flag on
  the `Projectile` because nothing stops a character from mixing them. Four consumers had to
  follow, which is why this was not the one-line fix it looks like: the flight animation's
  own "has it played out" test and the removal animation's clock, both in
  `EntityLayer::update`; the sprite bank *and* the animation set at the draw, which now pick
  file and art together the way the explod draw already did; and — the one that actually
  decides whether a screenpack-drawn projectile can hit anything — the flight animation's
  collision boxes in the fight screen, which were being looked up in the character's file
  whatever the prefix said. `projremanim`'s fallback on `projhitanim` carries the file along
  with the number, so naming only a prefixed `projhitanim` is enough. Regression-tested at
  the parse (four combinations of prefixed and bare across the three parameters, plus the
  fallback) and at the layer, where a fightfx removal animation of a *different length* from
  the character's same-numbered one says which file was read; three mutations shown to bite.
  Not covered by a host test, because neither a draw nor the fight screen has one: the sprite
  bank and the collision-box lookup are compile-verified and want a device look.
- **The generated fightfx pack was too small, and mislabelled.** With the prefix fixed and
  the dust no longer drawing a second copy of the fighter, the effect it *should* have drawn
  turned out not to exist either: `gen.py` wrote seven animations (0-3, 10-12) against
  MUGEN's nineteen, and labelled 10/11/12 the guard sparks when Elecbyte's guard spark is
  **40** (10/11/12 are blood). The generator now emits nineteen — 0-3, 4, 10-12, 30, 40,
  60-62, 70-72, 100-102 — from a single table feeding both the SFF and the AIR writer, so an
  AIR reference can no longer name a sprite the SFF lacks; sizes and frame counts follow the
  reference pair's own dimensions, and the art is drawn by the generator, never copied.
  Still absent from the reference set: `120` (floor dust) and `130` (power up), which
  nothing in the roster reaches. Two of the nineteen, `4` and `30`, have no counterpart in
  the reference at all and are our own shapes. Verified by running the Gradle task
  (`:app:assembleScreenpack`) and re-parsing its output, plus a census of what the roster
  asks for: **every bare `sparkno` now resolves**, and the only unresolved `anim = F<n>` in
  the whole roster is `dbz_gogeta`'s `F221`/`F222` — two `[State -2]` KO banners gated on
  `p2life = 0`, which Elecbyte's own pack does not define either, so drawing nothing there is
  the reference-correct answer rather than a gap.
- **Projectile collision, bounds and lifetime** ✅ landed (2026-10-19). The RE
  is `PROJECTILE-COLLISION-RE.md`; the pure rules live in
  `projectile_rules.{h,cpp}` and `test_projectile_rules` pins them. What changed
  from the deviations the RE listed:
  - a projectile-vs-projectile pass now exists (both state 1, exact
    `projpriority` exchange, defaults and negatives included); a spent side
    enters state 3 and plays `projcancelanim` → `projremanim` → `projhitanim`;
    `projhits = 0` cancels but never hits a player;
  - the player-hit pass accepts states 1 and 2, and records the contact as
    pending; `EntityLayer::update` applies it at the start of the next tick
    (`hits--`, cooldown from `projmisstime`, pause from the HitDef, state 4 only
    when exhausted and `projremove != 0`);
  - collision no longer applies `projscale`; hit boxes use raw positions and
    the unscaled Clsn record;
  - a flight animation ending no longer removes the projectile: state 1 has no
    animation-end check, so `projremovetime = -1` persists;
  - removal states move (one step with the entry velocity, then
    `remvelocity`) and play one pass of their animation before freeing;
  - bounds are four velocity-aware axis tests plus a position-only screen-edge
    test, fed the stage range by `FightScreen`;
  - `numproj` still counts state 1 only.
  Still open from the same RE, by design: the projectile-vs-projectile
  attribute/role bit test (§2.2) is approximated by opposite `ownerSide`
  because collision boxes carry no attribute mask; the player-attack-vs-
  projectile pass (§4) is landed and now reads the HitDef's parsed `affectteam`
  (`F`/`E`/`B`, default `E`); removal animation duration keeps the one-pass
  rule rather than the binary's element-time check; and the player-attack
  contact records code 3 because the reference documents no code of its own
  for that pass.
- **S7 Expression completion**: ✅ core landed — `:=` writes (Assign node, right-assoc,
  var/fvar/sysvar/sysfvar), sysfvar store added, selectored redirects (`helper(4000)`,
  `playerid(n)`, `target(id)`), `&&`/`||` non-short-circuit to match the stack machine
  (matters now that `:=` exists), gameflow trigger surface (numpartner/standby/teammode/
  ingame/timeroundstart/roundsexisted/roundsremaining/matchno/matchover), screenpos x/y via
  sink-provided stage view, gethitvar tail (xveladd/yveladd/type/airtype/hitshakeover/
  slidetime/hitshaketime). remaining: `timeroundstart` and `const240p`/`480p`/`720p` — the
  three `const*` triggers are now **landed** (converting by the ratio of widths, so a 720p
  offset lands where its author meant), leaving only `timeroundstart`, which is not a MUGEN
  1.0 trigger and is answered 0 for compatibility rather than implemented.
- **S7 Projectile tail** ✅ landed (2026-10-19), from `PROJECTILE-PARAMS-RE.md`
  §§4.2–4.10:
  - `numprojid` / `numprojectileid`. The reference (`0x473440`) applies `numproj`'s
    state-1 filter and then a **plain equality** against the projectile's own ID
    (`cmpl %edx, (%ecx)`); it does not clamp a negative argument to 0, so the
    engine counts nothing for one rather than every projectile. The host test pins
    both the ID match and the negative case.
  - `numproj` itself was counting projectiles that were already playing their
    removal animation. The reference counts state 1 only (`cmpl $1, 0x4(%ecx)` at
    `0x473401`), so a `numproj` gate no longer stays shut through the death
    animation — which is exactly the window in which a character respawns one.
  - `projremovetime` (default −1, never aged out) and `projremove` (default 1)
    are parsed. `projremove = 0` still decrements `projhits` on a connection
    (`decl 0x1C(%eax)` at `0x46FDA9`) but does not take the projectile out of the
    world (`cmpl $0, 0x38(%eax)` at `0x46FDC9`), so a beam that has spent every
    hit keeps flying and drawing.
  - A negative `projmisstime` is answered as 2, which is the reference's own
    clamp (`PROJECTILE-PARAMS-RE.md` §4.3).
  - **Still open:** the removal-on-animation-finish rule. The engine takes a
    projectile out when its non-looping flight animation ends even with
    `projremovetime = -1`; no reference evidence for that has been found. It is
    kept because removing it would make many corpus projectiles immortal, and is
    recorded here rather than presented as read off the binary.
  - **Per-state-owner counting** ✅ landed (2026-10-19). `numproj`, `numprojid`
    and `numexplod` asked the entity layer for a `Character*` count, so a
    helper's projectiles and explods were counted with its owner's — and in a
    mirror match both fighters share a `Character*`, so each counted the other's.
    The reference's manager is a sparse slot array per player struct (helper
    included), so the queries now carry the asking `StateMachine*`: the spawn
    paths already record `ownerMachine`, and the count matches on it. A null
    machine keeps the character-wide reading for engine-spawned effects and for
    callers that have no state owner. The host suite adds a two-machine,
    one-character case that pins the split for both projectiles and explods.
    `GameMakeAnim`, the pause flash and hit sparks now carry the spawning
    machine too, so they remain part of that machine's own `numexplod` answer.
- **S7 Gameflow triggers** ✅ landed (2026-10-19), from `SCREEN-FLOW-RE.md` §4
  and `TEAM-MODES-RE.md` §2:
  - `matchno` and `matchover` were constants (1 and false). Both now live on the
    state machine and are driven by `FightScreen`: the match index is
    `ladderStep_ + 1` for an arcade run and 1 for versus/engine test, set on both
    roots (and helpers) every round; `matchover` becomes true only once the
    match is decided and the deciding round has played out into `MatchOver`
    (the nearest equivalent to the reference's state-180 gate, which this
    engine does not force). This is slice 2 of SCREEN-FLOW-RE.md's order; the
    Win and Continue screens that follow it are now built, and the Game
    Over / ending / credits screens play through the storyboard player (all
    below).
  - `teammode = single` was **false**. The trigger returned the text `"single"`
    while the right-hand keywords compiled as unknown bare identifiers to 0, so
    the string-vs-number fallback compared 1 with 0. `teammode` now returns the
    single-mode code 0, and the parser compiles the reserved TeamMode keywords
    `single`/`simul`/`turns` to 0/1/3 (`TEAM-MODES-RE.md` §2.3). Four corpus
    characters that gate states on `teammode = single` now run those states.
- **Screen-flow data + menu geometry** ✅ landed (2026-10-19):
  - `[Victory Screen]`, `[Win Screen]`, `[Continue Screen]`, `[Game Over
    Screen]`, `[Survival Results Screen]`, `[Default Ending]`, `[End Credits]`,
    `[Option Info]`, `[Demo Mode]` and `[Music]` now parse into typed records,
    together with their `VictoryBGdef`/`OptionBGdef` layers (slice 1 of
    `SCREEN-FLOW-RE.md`). No screen consumes them yet. The host suite pins the
    fields against an in-memory section; the four shipped motifs were read by
    hand to confirm the keys and defaults (`data/system.def` Victory `time=300`,
    portrait `9000,2`; `data/kfm` has no `[Victory Screen]` and keeps defaults).
  - Title menu labels are drawn one ascent above `menu.pos.y`, because MUGEN's
    menu position is the text **baseline**, not the glyph top: the shipped
    motif's `menu.boxcursor.coords = -58,-10,57,2` frames a ten-ascent cell
    exactly from the baseline. Before the lift the cursor box sat a row above the
    item it marked; the shipped screenpack's coords now run `-40,-9,39,2` for its
    6x9 font. Verified on device with ENGINE TEST selected.
  - `title.fadeout.time` now delays the screen switch: confirm starts a black
    overlay that reaches full alpha at the configured tick and only then leaves,
    instead of cutting on the keypress.
- **Victory Screen (slice 2)** ✅ landed (2026-10-19), device-verified:
  - `VictoryScreenView` draws the `[Victory Screen]` record: `VictoryBGdef` layers
    when present (otherwise black), the winner's portrait `p1.spr` (default
    `9000,2`) from the winner's own SFF, honouring `p1.offset`, `p1.facing`,
    `p1.scale` and the `p1.window` clip; when the character has no portrait the
    current animation frame is fitted and centred in that window instead.
  - `p1.name.text` (default `%s`) and `winquote.text` (default `%i`) go through
    `formatVictoryText`, which implements MUGEN's `%s`/`%i`/`%%` and leaves other
    conversions literal; `textwrap = w`/`n` is honoured in `winquote.window`.
  - `VictoryTiming` reproduces `time`/`fadein.time`/`fadeout.time`: the card is
    readable for `time` ticks, skippable once the fade-in completes, and lives
    `time + fadeout.time` ticks in total. Arcade only: it runs before the next
    opponent is set up, or before `ArcadeClear` on the last match, and the final
    match shows it too. The engine's own `matchover` stays true throughout.
  - Device: match 4 of an 8-fight arcade run showed the black card with the KFM
    `9000,2` portrait, "Kung Fu Man" in font 2 and "Winner!  -  match 4" in
    font 1; the next match was set up 165 ticks later (150 hold + 15 fadeout).
- **Win Screen (slice 2)** ✅ landed (2026-10-19), device-verified:
  - `WinScreenView` draws the `[Win Screen]` card once per cleared arcade run,
    after the final Victory card and gated by `win.enabled` (compiled default
    true). It clears to black, freezes the winner's current animation frame and
    fits it to 72% of the screenpack width and 82% of its height (the honest
    substitute for the reference's forced state 180, which this engine does not
    do), then draws `wintext` through `drawElement` so its font, bank, offset,
    alignment, scale and alpha all apply.
  - The shipped motif defines no `[Win Screen]` section, so the view supplies
    MUGEN's compiled defaults for the omitted `wintext`: "Congratulations!" in
    font 2, centred at 159,70, `displaytime = -1`, `layerno = 2`.
  - `WinScreenTiming` adds `fadein.time` + `displaytime` + `fadeout.time`; the
    fade runs 1→0 over the fade-in, holds, and 0→1 over the fade-out. With the
    shipped defaults (32/300/64) the card lives 396 ticks (6.6 s). It has no
    skip input: a stray press must not cut the run-cleared card short.
    `Phase::Win` holds `ArcadeClear` back until the card pops, and the fight
    restores its wide window on the first update afterwards.
  - Device: winning the single-match run showed the black card with the frozen
    KFM win pose and "Congratulations!"; the log reported 396 ticks and ARCADE
    CLEAR drew immediately after it.
- **Continue Screen + Game Over gate (slice 2)** ✅ landed (2026-10-19),
  device-verified for the timeout/decline path:
  - A lost arcade match now pushes `ContinueScreenView` when
    `continueScreen.enabled`; accepting resumes the *same* match
    (`setupMatch(false)`: same `ladderStep_`/`matchno`, no VS card) with
    unlimited continues, and decline or timeout ends the run. Exhibition and
    engine-test losses keep their old pop.
  - The five `[Continue Screen]` elements (`continue.*`, `yes.*`, `yes.active.*`,
    `no.*`, `no.active.*`) draw at the section's `pos` (default 159,40). The
    shipped pack has no section, so big-motif defaults fill in: "CONTINUE?" at
    0,0, YES at −80,60, NO at +80,60 in font 1, active variants font bank 3.
    Left/right move the selection; Start or any attack confirms.
  - The countdown is **600 ticks (10 s at the fixed 60 Hz logic tick)**, the
    documented interpretation — no system.def key exists and no value was
    traced. Expiry is silent (the section has no timer element) and takes the
    NO path. The clock advances before input is read.
  - Decline/timeout reaches the Game Over gate. It was logged and skipped when
    this slice landed; the storyboard player (next bullet) now plays
    `gameover.def` there, and the run returns to the title (the arcade stack
    pops fight + select together), not to character select, once it pops.
  - Device (2026-10-19): the shipped `gameover.def` drew the wordmark, "GAME
    OVER" and "TRY AGAIN" with its fade, then returned to the title. That
    return exposed a latent title-menu bug — `TitleScreen::leaving_` stayed
    set from the original ARCADE confirm, so the first title tick re-ran
    `activate()` and pushed a fresh select screen. `activate()` now clears the
    leave state; the device re-run lands on a live title menu.
  - Device: idle loss showed the black CONTINUE? card with YES/NO and the
    controls; the log reported the prompt with 600 ticks, the expiry at exactly
    600 ticks (10.0 s after the prompt), the decline and the return to title.
    The accept path is wired and syntax-checked but the device tap landed after
    expiry, so it is not yet device-exercised.
- **Storyboard player (slice 6)** ✅ landed (2026-10-19), host-tested:
  - `engine/storyboard.{h,cpp}` parses `[Info] localcoord`, `[SceneDef]`
    `spr`/`snd`/`font0..font9`/`startscene`, and each `[Scene N]`: `end.time`,
    `fadein.time`/`fadeout.time` (plus colours), `clearcolor`, `layerall.pos`,
    `bgm`/`bgm.loop`, `bg.name`, `layer0..99` (`anim`, `text`, `font`, `offset`,
    `starttime`, `endtime`, `textdelay`, and the extension `scale`), `sound0..99`
    (`value`, `starttime`, `volumescale`, `pan`) and inline `[Begin Action N]`
    animations through the existing AIR parser.
  - `StoryboardTiming` gives each scene `[start, start + end.time)` and runs
    two linear fades (in 1→0 at the start, out 0→1 at the end; the stronger ramp
    wins and picks its colour). `layerall.pos`, `clearcolor` and `bgm` are
    sticky across scenes, `startscene` seeks the clock, and the total is the sum
    of the scenes. There is deliberately no skip input: the reference defines
    none.
  - `screens/storyboard_screen.*` loads the storyboard's own sprite/sound/font
    files, draws `clearcolor`, layers 0..99 bottom-to-top at
    `layerall.pos + offset`, text lines split on literal `\n` with
    `textdelay` reveal, fades over the finished scene, and plays due `soundN`
    one-shots. Alignment follows the storyboard convention (1 left, 0 centre,
    −1 right). Sound banks are cached for the process because the mixer keeps
    raw bank pointers for the engine's life.
  - `FightScreen` gained `GameOver`, `DefaultEnding` and `EndCredits` phases.
    `endArcadeRun` pushes the Game Over storyboard when enabled and returns to
    the title when it pops; the Win card now feeds `[Default Ending]` once, then
    `[End Credits]`, then `ArcadeClear`. A missing or unplayable storyboard logs
    and keeps the old fallback, so an enabled switch cannot trap the player.
  - The shipped pack now carries an original `gameover.def` (text and timing
    only; its animation is the pack's own `system.sff` wordmark, group 10 image
    0) and `system.def` enables `[Game Over Screen]`. Covered by
    `tools/hosttest/test_storyboard.cpp`, which also parses the shipped file.
  - Storyboard sound parameters now reach the mixer: `Mixer::playFromFull` plays
    a bank-scoped command with volume and pan, `volumescale/100` maps to gain
    (clamped at 1.0; the docs allow no amplification, so >100 is reported once)
    and `pan/127` maps the docs' -127..127 range to the mixer's -1..1. Covered
    by `tools/hosttest/test_mixer.cpp`, which drives real rendered PCM.
  - Deliberately unsupported and logged once: `bgm` (this engine has no music
    decoder), `bg.name` background objects, and `clearcolor = -1` drawing the
    previous framebuffer (it becomes black). Object-level `[Reflection]` is
    still S12's gap.
- **VictoryQuote + `[Quotes]`** ✅ landed (2026-10-19), host-tested:
  - `Character::loadCns` reads `victory0`..`victory99` from `[Quotes]` into a
    sparse table; values are unquoted, only an exact numeric key is accepted,
    and a later constants document without the section does not erase earlier
    quotes. Localised `[ja.Quotes]`-style sections are ignored (no language
    selection exists).
  - The `VictoryQuote` controller evaluates `value` (default −1) and records an
    index only when the character actually wrote that quote; −1, an unwritten
    index and 0..99 out of range all leave the random marker. Helpers are a
    hard no-op. The selection resets per match (`setupMatch`), not per round.
  - `VictoryScreenView` resolves the winner's selection first, then a random
    present quote (uniform over written indices, drawn from the winner
    machine's own RNG stream), then the screenpack's `winquote.text` fallback
    through `formatVictoryText`. A character quote is drawn verbatim, so `%` is
    literal.
  - Tests: `tools/hosttest/test_victory_flow.cpp` and extensions to
    `test_statemachine.cpp` cover parsing, sparse/edge indices, merge
    preservation, roll mapping, explicit/missing/-1 resolution, literal `%`,
    helper no-op and round-reset preservation.
- **RemapPal (group half)** ✅ landed (2026-10-19), host-tested:
  - `source`/`dest` are palette-node identities `(group,item)`, not sprite
    groups; the legacy one-argument form means group 1. Destinations are applied
    one level only (they are not chained), the map is keyed by source so a
    repeated controller replaces its entry, and it is cleared in the round's
    `setup()`.
  - `SpriteBank` now records each SFF v2 palette node's group/item;
    `Character::paletteIndexFor` keeps group 1 on the old palno/`.act` lookup and
    resolves other groups through the SFF v2 palette nodes. The fighter's LUT
    binding applies the remaps to `(1,palno)` and to its dedicated sprite
    palette nodes, and inherited entity draws resolve `effectivePalette()`.
    Fightfx palettes are deliberately excluded.
  - Fixed on the way: `remappal` was dispatched under `case 'p'`, which can
    never match a type starting with `r`, so the controller had never run.
- **Parallel roster and stage loading** ✅ landed (2026-10-19). `Game::beginLoad`
  sizes both result vectors once, then a pool of 1–4 threads (one per core, capped
  at four) fills one slot per entry from a shared atomic index: no two threads write
  the same element, and the containers are not resized while the pool runs. Hidden
  roster entries take no slot and are not counted in the loading total, so the bar
  can reach its own end. Nothing is published until the last slot is filled, which
  keeps the cell order exact and lets the loading screen stay on the counters it
  already reads. The single `worker_` the class owned before is still the only
  thread `~Game` joins: it joins its own helpers before raising `loadFinished_`.
- **`PosSet` / `PosAdd` / `Offset` on one axis** — **fixed** (2026-10-19). The
  engine read `x` and `y` as the first and *second* items of one list, but they are
  two separate single-valued keys: `posadd y = -8` writes one number under `y`, so
  the second-item read returned the fallback 0 and every y-only position change in
  the corpus silently did nothing. Pinned by a two-tick `PosAdd` host test and
  shown to bite under mutation.
- **S8 CMD parity**: ✅ landed + host-tested (196/196). `>` strict interleave, `s` start
  symbol wired end to end, `$` audited-loose. Host suite (tools/hosttest/test_expr_cmd.cpp)
  found three real matcher bugs, all fixed and pinned by tests: `/hold` vs `/N$`-counted
  windows (holdTicks dropped on the `~N$D` form; window now closes at the matched tick),
  and the canonical smooth roll D,DF,F — direction elements now count as pressed on a
  *direction-set change*, so rolls match while `F,F` still needs a real second tap.
- **S9 Assets**: ✅ codecs landed (subagent): RLE5 (format 3) bounds-safe decoder by SFF 2.0
  spec; real PNG8/24/32 decoder over NDK zlib (filters 0-4, multi-IDAT, 1188 local sprites
  decoded, 0 failures). ✅ AIR Interpolate (Offset/Scale/Angle lerped at draw time,
  loops wrap). Remaining: ACT palettes (rolls into S6).
- **S13 What a hit may take, and who wins a clash** ✅ landed (2026-10-18). The HitDef's
  three refusals of the last point of life, and its tiebreak class. All host-tested and
  shown to bite (5 mutations, each restored from a `/tmp` copy afterwards).
  - `kill` (default 1), `guard.kill` (default 1) and `fall.kill` (default 1) were each
    parsed and then thrown away; the engine killed regardless, so the parameter set a
    character writes to juggle an opponent down to nothing *except* dead did nothing.
    `applyHit` now clamps at 1 for the hit and for the chip damage of a block, and
    `applyFallDamage` clamps at 1 for the fall. `fall.kill` needed a *carrier*: the fall
    lands ticks after the HitDef that caused it has been overwritten, so the flag travels
    with the damage into `fallKillPending_` beside `fallDamagePending_` rather than being
    read back off the attacker when it is finally needed — the same reasoning `HitInfo`
    already used for `id`.
  - `priority = hit_prior, hit_type`. The number was read and the class was not. The class
    is a **name** (`priority = 4, Miss`) and cannot go through `secondParam`, which
    evaluates its item as an arithmetic expression — a bare `Miss` reaches the expression
    parser as an unknown identifier and comes back 0, leaving every class at its default.
    It comes off the raw text instead, via a `secondComponent` helper that respects nesting;
    an unrecognised spelling falls back on `Hit` rather than on the worse of the two, since
    read as `Dodge` a typo would take the attack *and* its opponent out of every clash.
  - The tie table itself is `StateMachine::clashLands(mine, theirs)`, a pure constexpr
    function rather than inline fight-loop logic, so the reference's six rows are
    host-testable one side at a time. The rows are (Hit,Hit) both land, (Hit,Miss) the hit
    lands and the miss does not, (Hit,Dodge) neither, (Dodge,Dodge) neither, (Dodge,Miss)
    neither, (Miss,Miss) neither — which reduces to "a hit lands unless it is not a `Hit`,
    or the other side is a `Dodge`". The asymmetry is the content: `Miss` yields to a hit,
    `Dodge` takes the hit out with it. A tie that produces no hits leaves both HitDefs
    enabled, which is what falling through without calling `tryHit` already does.
  - **The first version of this rule was wrong and the doc caught it.** The engine had been
    carrying "only `Hit` can win a clash, every other pairing takes both out", which is
    *almost* the table — it gets (Hit,Hit), (Dodge,\*) and (Miss,Miss) right and gets
    (Hit,Miss) wrong, turning "the hit lands" into "neither lands". Worth recording as a
    near-miss: a plausible paraphrase of a six-row table is not the table. Read
    `sctrls.html`'s `priority` entry (line ~1394 in the extracted copy) rather than
    re-deriving it.
  - `fight_screen.cpp`'s clash pass gained a `Fighter::attackPriorityClass()` forwarder
    (`Fighter` holds a `StateMachine` rather than deriving from one, so every machine
    accessor the fight loop wants needs forwarding by hand — the first build of this failed
    on exactly that).
  - `hitonce` was assessed and deliberately **not** implemented: its wording ("the HitDef
    only affects one opponent... all other targets will be dropped") is about the *target
    list* under a multi-target HitDef rather than about further hits from the same attack,
    and the corpus in `local_chars` is a 1v1 roster that cannot tell the two readings apart.
- **Explod freeze gates split in two** ✅ landed (2026-10-18). `Explod` carried one flag,
  `ignoreHitPause`, answering two different freezes as if they were one question, and its
  default was 0 where the reference's default for `ignorehitpause` is **1** (sctrls.html's
  Explod entry). The consequence was a real visual bug in both directions: every explod got
  superpause behaviour (a hit spark hung where the reference plays it) and none of them got
  the hitpause one (a super flash played on where the reference holds it). The two gates are
  now separate and separately documented on the struct:
  - a *hitpause* — the per-fighter freeze a landed hit hands both fighters — is governed by
    `ignorehitpause`, default 1, and read from the **owner side's root** machine
    (`owners[ownerSide]->inHitPause()`), which is the reference's "not be updated when the
    player is in hitpause";
  - a running Pause/SuperPause — the world freeze — is governed by `pausemovetime` /
    `supermovetime`, default 0, counted from the pause's first tick as
    `pause_.total - pause_.ticks`, which is the same count `movetime` uses.
  The deprecated `supermove` is deliberately left **unread** rather than guessed at (its
  relationship to `supermovetime` is ambiguous in the documentation), and the test asserts
  that `supermove = 1` reads as nothing so the decision is pinned rather than accidental.
  Verified by a real controller parse (parameters through a real `StateMachine`) *and* a
  real `EntityLayer` run (5-tick hitpause, world `Pause`, `SuperPause`, the other-side
  owner case, and an aura running through a 3-tick flash); four mutations shown to bite.
  **The first attempt at this test did not bite** and that is worth remembering: it asserted
  `makeExplod("default").ignoreHitPause`, which only tests the *struct default* and passes
  however the controller reads its parameter. The assertion had to become a real parse
  through a real machine before reverting the parse default failed it.
- **`[Size] xscale`/`yscale` is a SPRITE-ONLY draw scale** (research finding, not yet
  implemented). It never touches clsn boxes, physics, offsets or the axis; `localcoord` is
  what scales everything. A Helper's `size.xscale` inherits from its *spawner* when
  unspecified and the inheritance **chains** (a helper's helper inherits from the helper),
  with a `-FLT_MAX` sentinel in the reference meaning "inherit". Corpus scale: 35 files at
  character level (mostly 1), 311 files / 5860 lines at Helper level, extremes .03 and 2.
  The character-level pair is currently parsed into the constants table only, so
  `const(size.xscale)` answers; nothing applies it to a draw yet. Shadow behaviour under a
  non-unit scale is unresolved in the docs and will need a device look.

- **`removetime` is three numbers, and the two negatives are not the same one** ✅ landed
  (2026-10-18). MUGEN's Explod entry: a positive `rem_time` is a lifetime in ticks, `-1`
  is "displayed indefinitely", and `-2` — the documented **default** — is "removed when
  its animtime reaches 0". The engine seeded the field with `-1` and treated every
  negative as one case, which made the default and "for ever" the same answer, so the
  **392** explods in `local_chars` that write `removetime = -1` for something meant to
  outlive its animation died with it. Default is now `-2`, `-1` falls through both
  branches, and anything below `-1` takes the animation rule.
  - The animation rule needed a **new predicate**, and this is the finding worth keeping:
    "removed when its animtime reaches 0" is not `EntityFrame::finished`. `AnimTime = 0`
    is `trigger.html`'s "equal to the animation action's looptime, ie. the end of the
    action has been reached", and a **looping** action reaches it once per pass. Almost
    every action in the format loops, because all that looping takes is the absence of a
    `LoopStart` line — so read as "would it finish for good", the default explod with a
    looping animation is **never removed at all**. That is the shape of the bug the state
    machine already fixed once for itself (`animationFinished`, whose comment records the
    same reasoning) and which `entityFrame` still carried. `entityLoopTimeReached` is now
    the shared answer: one pass is the sum of *all* the elements, prefix included, which
    is the arithmetic air.html walks through in its own example ("7 + 7 + 7 + 50 + 7 + 7 +
    7 + 60 = 152 ticks"); a single element of duration `-1` makes the looptime infinite
    (air.html's "no loop") and the answer is therefore "no". An animation the character
    does not have counts as reached, the same forgiving reading `entityFrame` takes.
  - Pinned by host tests at the parse (default `-2`, `-1` kept, `40` kept) and on a real
    `EntityLayer` with a two-element animation that really does complete a pass: the
    default explod goes, the `-1` one does not, the timed one spends its ticks and then
    goes. Three mutations shown to bite (default reverted, rule reverted to `finished`,
    `-1` folded back into the negative range).
  - **Open, and deliberately not changed:** a *projectile*'s `projremovetime` defaults to
    `-1`, which MUGEN defines as "the projectile will not be removed" by time — and the
    engine instead removes it when its flight animation finishes (`entities.cpp`, the
    `< 0` branch). Those are different rules in both halves: the default should mean
    *no time-based removal at all*, and the engine's extra condition can kill a flying
    projective after a single pass of a short animation. Changing it is a behaviour
    change to every projectile in the roster and wants corpus evidence (how many
    projectiles have a `projanim` shorter than their life) plus a device look, so it is
    recorded here rather than guessed at.

- **The Explod set's placement half: `vfacing`, `random`, `removeongethit`, `ontop`** ✅
  landed (2026-10-18). Four parameters of the Explod set that had **no reader at all**, found
  by reading the param set out of the binary (`FINDINGS.md:190-193`) against the corpus.
  Counted properly this time — parameter lines across the **363** character files in
  `local_chars` (`.cns`/`.air`/`.cmd`/`.st`/`.def`, comments stripped), which is a wider and
  more honest scope than the per-Explod-controller tally an earlier pass used:
  `ontop` 4446 (of which **`ontop = 1` 3469**), `removeongethit` 3814 (`= 1` **3064**),
  `vfacing` 2609 (`= -1` **54**, plus ~54 more written as an `ifelse(random …)` that picks
  `-1` about half the time), `random` 463 (of which **349 are `0,0`** — a deliberate no-op,
  so ~114 real uses). None of these is an exotic corner.
  - **`vfacing` was parsed by nobody, and the field it should have filled did not exist.**
    It is the *vertical* mirror and it is an independent choice from `facing`: a falling
    droplet that faces away from its owner still has to fall the way it was drawn. Added as
    `Explod::flipV`, plumbed through `drawSpriteAt`'s new trailing `bool flipV`, and the
    corner arithmetic in `motif.cpp` now mirrors about the axis on the other axis too
    (`top = flipV ? y - (height - axisY) * scaleY : y - axisY * scaleY`) rather than
    hard-coding `false` into `Renderer::drawSprite`.
  - **`random = rand_x, rand_y` displaces the *bind point*, not the spawn.** The reference's
    worked example is the specification: "if `pos = 0,0` and `random = 40,80`, then the
    explod's x location will be a random number between -20 and 19, and its y location will
    be a random number between -40 and 39" — so the low end is `-(rand / 2)` and the span is
    `rand`, which is **not** a symmetric range about zero. Both halves are separate draws off
    the machine's own stream, the same stream the `random` trigger reads. The corpus's widest
    is `500,500` and most are `30,30`/`60,60`, so a range wider than the 0..1023 generator
    would repeat rather than cover — noted in the code rather than left as a silent ceiling.
  - **`removeongethit` is checked *above* the explod's freeze gates, and that placement is
    the whole finding.** The tick the owner is hit is the same tick the owner's own hitpause
    begins, so a check sitting under the gates would be asking a question whose answer it can
    never see. The engine's answer is `StateMachine::tookDamage()` — true for exactly the tick
    a hit was applied, set by `applyHit` and cleared at the top of `step()` — and the fight
    loop's order (fighters first, then `entities_.update`) is what makes that the *current*
    tick at the layer.
  - **`ontop` is a layer, not a priority**, which is what "precedence over sprpriority" forces
    once it is read literally: a priority orders one sprite against another *within* a layer.
    So `EntityLayer` gained a third pass, `drawTop`, and `drawBehind`/`drawFront` both skip an
    `ontop` explod. `doExplod` also clears `behindOwner` for one, so the filter inside
    `drawBehind` is unreachable through the parse — it is kept as the layer's own contract and
    a host test pins it with a hand-built explod that asks for both, because a rule that only
    holds because of an invariant enforced elsewhere is a rule that gets deleted by accident.
  - **Open, and recorded rather than assumed: how far up "over all other sprites and
    background layers" reaches.** Read here as the *world* (the stage's own layers and every
    fighter, explod and projectile), with the HUD and the banners drawn after it, because a
    life bar a character's comic text can cover is a bar the player cannot read. The literal
    words also allow the other reading — the HUD's art is sprites too — and MUGEN's own draw
    order may well implement it. Nothing in the corpus distinguishes them: the overwhelming
    majority of the 3469 `ontop = 1` explods are mid-screen effects. This is display-only —
    no state, no collision and no timing depends on it — so it is a candidate for a device
    look rather than a blocker.
  - Pinned by host tests at three levels: the parse (each parameter read off a real
    controller through a real `StateMachine`, with the defaults asserted as absent), the draw
    *call* (a new `draw_recorder` shim records what `drawSpriteAt` was asked for, and the three
    passes are told apart by the x each explod was spawned at), and the layer's timing (a real
    victim takes a real hit with `victimPause = 4` so the freeze gates are genuinely closed,
    and the explod still goes the same tick). **Ten mutations shown to bite**, including the
    two that did not bite first time and had to be fixed: the y half of `random` (zero is
    inside both windows, so only a displacement assertion on y itself catches it) and the
    `drawBehind` `onTop` filter (unreachable through the parse, hence the hand-built case).
  - **The test fixture trap worth remembering, because it silently faked four passes:**
    `Character::loadBehaviour` **merges** rather than replaces — the `.cns` and `.cmd` are
    parsed into one state library by design — so a state 300 loaded once per parameter set
    ends up holding *every* earlier run's controller, all firing on the same tick, with the
    sink handing back the **first** run's explod. The first version of the fixture reused one
    character and every positive assertion was reading an explod that had been given no
    parameters; it was caught by instrumenting the fixture, not by reasoning about it. The
    fixture now keeps one character per run and asserts its own spawn count.
- **The way out of a fall: recovery (5200 / 5201 / 5210)** ✅ landed (2026-10-18). The engine
  could only leave a fall by reaching the floor, so `fall.recover` was a parameter the corpus
  writes that nothing could honour and a launched victim was helpless however well it read
  the screen. `common1.cns` reaches it through two controllers inside 5050 — `[State 5050, 4]
  ;Recover near ground` and `[State 5050, 5] ;Recover in mid air` — both gated on `alive`,
  `CanRecover` and the `recovery` command, and both landing the victim in a state the
  reference defines further down the file.
  - **The corpus says both halves are load-bearing.** All **42** pack `common1.cns` files
    under `local_chars` write both hand-overs, and **41** of them carry 5200's own `SelfState
    5201` as well. **5071 — the trip's fall — carries neither**, in the reference or in any
    pack, so recovery is scoped to 5050: a tripped fighter stays tripped. That asymmetry is
    the kind of thing worth having read rather than assumed, because "the fall's recovery"
    sounds like it belongs to every fall.
  - **The three states, out of the reference's own files.** 5200 (`HITFALL_RECOVER`) is
    `type = A, movetype = H, physics = N`, changes its animation to `5050+var(10)` and adds
    `GetHitVar(yaccel)` a tick; 5201 (`HIT_FALLRECOVER`) is where it lands — it turns on
    `p2dist X < -5`, `VelSet x = -.15, y = -3.5`, `PosSet y = 0`, `NotHitBy SCA time = 1` and
    a 3-tick `PalFX add = 128,128,128`, and has **no exit controller at all** (confirmed
    across every pack: the only one that writes anything writes `2147483647`). 5210
    (`HITFALL_AIRRECOVER`) is the mid-air roll: `physics = N`, `PosFreeze value = 4`, and at
    `Time = 4` the lift (`VelMul x = .8, y = .8`, `VelAdd y = -4.5`, then the three
    conditional nudges that straighten a fast fall out into a rise) plus the `holdup` /
    `holddown` / `holdfwd` / `holdback` stick quartet, with gravity `.35` a tick from there
    on and `ChangeState 52` when it reaches the ground.
  - **`Time = 4` is the fifth tick, not the fourth**, because MUGEN's state clock starts at
    0 and `PosFreeze value = 4` therefore covers clocks 0–3. The engine's roll reads
    `stateTime_` the same way (`roll < 4` freezes, `roll == 4` lifts), which is why the test
    asserts the freeze lasts four ticks *and* that the lift arrives on the one after them.
  - **The window had to be moved onto the hit's clock, and that is the substantive change.**
    `fall.recovertime` is "the minimum time before the player can recover from a fall", and
    this engine's built-in table is deliberately held still while `stunTicks_ > 0`, so a
    launched victim's fall *begins* only after its hitstun — a window measured from the fall
    state would open that much later than the reference's and put recovery out of reach of a
    default hit entirely. So `HitInfo` gained `fallRecoverTicks`, armed in `hitInfoFor`
    beside the parsed `fallRecoverTime` and counted down one a tick in `step()`. It is
    decremented *after* the hitpause early-return, so a pause does not spend the window —
    which the test measures: an armed clock of 3 opens `CanRecover` on exactly the **4th**
    tick after the hit, one hitpause tick behind three clock ticks.
  - **One deliberate deviation, recorded.** The reference's 5200 is `physics = N` and
    unclamped, and hands over to 5201 ten units *under* the floor (`Pos Y >= 10`, where
    positive pos y is below it — the corpus's `Pos Y >= 0` 432 / `Pos Y >= 10` 220 / `Pos Y
    >= 25` readings are what settled that sign). The engine gives it `Physics::Air` and lands
    it at the fight's own floor, routing 5200 → 5201 through `land()` — the same engine
    choice already made for 5050 → 5100, and the reason `builtinAwaitsGround` had to learn
    about both recovery states: 5200 and 5210 must not hand over to their `thenState` while
    they are still in the air.
  - **Cosmetic gaps, left deliberately rather than faked**: the `PalFX add = 128,128,128`
    blink and `GameMakeAnim 60` dust the reference writes beside the hop are not supplied
    (the engine has neither a per-fighter palette-add it can raise without a controller to
    read one from, nor an engine-driven `MakeAnim` yet), and 5210's `CtrlSet 20` is skipped
    with the rest of the control-time bookkeeping it belongs to.
  - **Nineteen mutations, all shown to bite**, and the two that escaped the first draft are
    the reason the assertions are shaped the way they are: loosening the mid-air window
    (`Vel Y > -1`) to "any upward motion" was invisible until a test pressed the buttons on a
    victim that was still *rising*, and dropping the roll's `VelAdd y = -4.5` was invisible
    until the victim entered the roll from a hard fall (its two conditional nudges are enough
    to raise a slow one, so only a low, fast entry tells the two apart). The others cover the
    near-ground window's bound and its place ahead of the mid-air one, `CanRecover` losing
    either half of its test, the clock never running or never being armed, `nohardcodedkeys`,
    the fallback binding's `x`+`y` being a pair rather than either, `builtinAwaitsGround`,
    `land()`'s 5200 row, the hop's lift and its direction, the freeze count, the roll's
    gravity (`.35`, distinct from the `.44` the same fighter's fall runs at), and the stick's
    nudge. Engine restored byte-identically after each.
  - Two facts about the fixture that the next slice will want: a `StateMachine` with
    `character_ == nullptr` has **no** state logic at all (`step()` skips it), so a fall cannot
    be observed on a bare machine — the victim here carries a one-state `.cns` — and
    `worldX(-0.15f)` on a player-1 machine is **+0.15**, because `facing` is MUGEN's -1 for the
    left player. The first draft of the hop's assertion read `velX() < 0` and was wrong for
    exactly that reason; it now asserts .15 a tick *against* `facing()`.

- **One draw's transparency, from all three sources** ✅ landed (2026-10-18). MUGEN writes
  transparency as a *pair* of alphas, and until this slice the engine could only say which
  of three blend modes a draw used: the destination half of every pair in the corpus
  — `AS128D128`, `A1`, `Trans = addalpha, time, 256-time` — was parsed into `AnimFrame` and
  then dropped at the draw. Now the pair reaches the blender.
  - **The arithmetic, from the reference's own documentation rather than from inference.**
    `air.html` gives it twice: "`AS256D256` is equivalent to just `A`", "a shorthand for
    `AS256D128` is `A1`", and — the sentence the whole slice rests on — "`AS128D128 ;<-- Mix
    50% source with 50% dest`". So the result is `src*srcA + dst*dstA`. `sctrls.html`'s
    `Trans` entry supplies the controller side: exactly six types (`default` "does nothing",
    `none` "disables transparency", `add` "full additive", `addalpha` "(alpha must be
    specified)", `add1` "at 256,128", `sub` "full subtractive"), `alpha = source, dest` on
    MUGEN's 0..256 scale with "If omitted, defaults to 256,0", and the fade example
    `trans = addalpha; alpha = time, 256-time`. The decompiled `Trans` dispatch
    (`decompiled/angr_00473ed0_controller_dispatch.c:3150-3260`) agrees on all six codes and
    on the `(256, 0)` default.
  - **`add` is AS256D256 and not AS256D0, and that is the one reading worth spelling out.**
    The decompile's pre-pushed defaults are `(-1, 256, 0)`, which read naively makes `add`
    additive with a *zero* destination — a mode in which nothing behind the sprite is ever
    kept. Two things say otherwise: the documentation's own equivalence above, and the fact
    that `parseBlend` has always drawn a bare `A` as full additive, so the other reading
    would silently darken every `trans = add` in the corpus. The `(256, 0)` pair is the
    `alpha` parameter's default, exactly as `sctrls.html` states, and the engine reads it
    that way.
  - **The two factors cannot use the same mechanism, which is why the destination half
    needed GL.** The source alpha multiplies the sprite's own alpha, and
    `fragColor = texel * vColor` means it rides the vertex tint: `TransParams::srcAlpha` goes
    through `transTint`. The destination factor has no per-vertex home — it is a property of
    the blend state, not of the quad — so `dstAlpha < 1` goes through `glBlendColor(0,0,0,dstAlpha)`
    with `GL_CONSTANT_ALPHA`, and `dstAlpha >= 1` stays on `GL_ONE` so that every `add` and
    `sub` in the format, and every draw that predates the pair, never touches `glBlendColor`
    at all. `BlendState` now carries the constant itself, and the batch cache compares the
    whole state, so two draws that differ only in the destination alpha are two states and
    cannot be conflated.
  - **And the destination factor made a colour key necessary.** With `dstAlpha < 1` the
    destination term applies to *every* fragment the quad covers, so the transparent corners
    of a sprite would darken the scene behind them. The fragment shader gained
    `if (fragColor.a <= 0.0) discard;`, which is MUGEN's own colour-key rule and is
    behaviour-preserving for every existing draw (Normal keys on the source alpha already;
    Additive and Subtractive were `GL_ONE`).
  - **Three sources, ranked rather than merged.** The ranking is the substance: a `Trans`
    controller run this tick outranks the animation's own `.air` code, and an absent
    controller means "inherit", not "Normal" — which is why the override lives in a
    `std::optional<TransParams>` and not in a `TransParams` with a default. The same
    distinction is why `Explod::trans` is optional: an explod that wrote no `trans` must let
    its animation's `A` draw, and defaulting it to Normal would cancel it. An `AfterImage`
    trail carries its own pair (defaulting to `add`, the controller's documented default),
    and a Projectile draws as its own frame asks — its hardcoded `Blend::Additive` is gone,
    which is what lets the two thirds of `fightfx.air`'s frames that carry no code draw as
    plain sprites.
  - **`Trans` holds for one tick, and the engine has to clear it.** `sctrls.html`: "Overrides
    the player's animation transparency parameters for current game tick". So `step()` resets
    the slot beside `posFreezeThisTick_` each tick, and `default` — "does nothing" — resets
    it rather than setting it. A character that wants a fade re-triggers every tick, exactly
    as the reference's own example does. `AfterImage`'s parameter list at `FINDINGS.md:195`
    (`0x47bc33`: timegap, framegap, trans, **alpha**, palcolor, …) also carries `alpha`,
    which the controller's prose documentation does not mention, so the trail's `trans` is
    parsed through the same six-type read with the same `(256, 0)` default.
  - **Corpus check, so the reading is not just the documentation's.** Across the 415
    `.air`/`.def` files in `local_mugen` and `local_chars`, all 732 distinct blend codes
    begin with `A` or `S` — there is no normal-blending code carrying an `S`/`D` pair — which
    is why `blendStateFor` lets a Normal draw ignore the destination factor rather than
    inventing a meaning for a form the format does not have.
  - **Twenty-six mutations, every one caught.** They cover all six types read as each other,
    the unrecognised value that must be dropped rather than guessed at, the 0..256 scaling
    and both clamps, each of the three rankings (body over frame, explod over animation,
    trail over body) and the clearing of the one-tick slot, plus the GL table: the constant
    reaching the state, a `full` comparison that 1.0 would fail, the source alpha scaling the
    colour instead of the alpha, and a batch cache that conflated two destination alphas.
    Engine restored byte-identically after each; host suite `12 ran, 0 skipped, 0 failed`,
    Android build green.
  - **Gaps left honestly.** (a) `applyBlend`'s three GL calls are the one part no host test
    can reach — there is no GL in the host build — so the *decision* to use a constant alpha
    and *which* constant is asserted on `BlendState`, and only the calls themselves are
    unverified; a device run is what closes that. (b) The `discard` and the removed
    hit-spark hardcode both change what appears on screen and are host-tested only through
    the pair that reaches the draw call, so the sparks need a device look. (c) Explods and
    projectiles still pass `palette = nullptr`, so `ownpal` (8804 uses) remains open — now
    **closed**, see the `ownpal` slice below.
  - **The HUD's own `trans` — ✅ landed (2026-10-19).** `defs.cpp`'s `blendFromTrans` was
    the last caller folding a pair down to a bare mode. `DisplayElement` and `BgElement`
    now carry a whole `TransParams`, so a screenpack's `trans = add1` reaches the blender
    with its documented `256,128` pair instead of drawing at full destination — the visible
    bug, since `mugen1`'s and `big`'s `system.def` use `add1` for title and select layers.
    The parser (`transFromDef`) reads `add`, `add1`, `sub`, `addalpha`/`subalpha`, an
    inline pair after the type (`addalpha, 256, 0`), and the separate `alpha` key in either
    the normalised 0..1 spelling this engine already shipped or MUGEN's 0..256; an
    explicit `alpha` overrides the type's default pair for every type. Two adjacent parse
    bugs were found while wiring it: `alpha` and `window` were read as *unprefixed* keys
    inside `readElement`, so `p1.face.window` — the corpus' own portrait clip — never
    reached the element. Both are now prefix-aware. Host coverage is in
    `tools/hosttest/test_select_config.cpp` (synthetic `fight.def`/`system.def`), and the
    `add1` and alpha-pair checks were each shown to fail under mutation. What remains
    unverified is the same GL half as (a); and `applyAlpha` is now text-only, since every
    sprite and animation path passes the pair through `TransParams`.

- **`ModifyExplod`, `RemoveExplod`, `GameMakeAnim`, `PowerSet` — the four controllers that
  reached nothing at all** ✅ landed (2026-10-18). This batch started from a measurement rather
  than a suspicion. Splitting every corpus file into its `[State ...]` sections and reading what
  each one's `type =` names — a method that has to exclude the five single letters, `S`/`C`/`A`/
  `L`/`U`, or the `[Statedef]` headers drown the histogram — gives 87 distinct controller names
  in `local_chars`. Three of them were **entirely absent** from the dispatch and a fourth was a
  one-word stub, which between them is a character file in four writing something the engine
  silently dropped:

  | Controller | Sections | Files | Packs | What the corpus does with it |
  |---|---|---|---|---|
  | `ModifyExplod` | 149 | 22 | 19 | Always with an `id`; `scale` 82, `pos`+`postype` 76/77, `pausemovetime` 43, `ignorehitpause` 34, `trans` 32, `alpha` 27, `sprpriority` 15, `facing` 14, `ontop` 11, `removetime` 11, `ownpal` 9, `supermovetime` 9, `anim` 7, `bindtime` 3, `vfacing` 2, `removeongethit` 2, `vel` 1; `index` **0**, `angle` **0**, `shadow` **0** |
  | `RemoveExplod` | 930 | 120 | 48 | `id` written in 912 and omitted in 18 |
  | `GameMakeAnim` | 118 | 42 | 37 | `pos`, `under`, `random` |
  | `PowerSet` | 50 | 20 | 10 | `value` only |

  The documentation is thin on all four and it matters which way: `sctrls.html` gives `PowerSet`
  one sentence, `GameMakeAnim` five plus a deprecation notice ("superseded by Explod and is now
  considered deprecated"), `RemoveExplod` two (the second of which — "If omitted, removes all
  explods owned by the player" — is the whole of the unnamed case), and `ModifyExplod` **three
  sentences and no parameter list at all**, merely "syntax is basically the same as Explod" plus
  "the pos and postype parameters must be specified together". So for `ModifyExplod` the
  parameter set had to come from the corpus rather than from the prose, and the corpus is
  unambiguous about the one parameter that matters for a design decision: a later reference
  added an `index` for picking one of several explods sharing an `id`, and it is used **zero**
  times in 149 sections while `id` is used in all 149. It is therefore **deliberately not
  implemented**, recorded as an absence with its evidence rather than guessed at: the numbering
  a reader would have to invent (oldest first? does a new explod displace one at that index?) is
  not recoverable from anything on hand, and the documented reading of `id` is "all explods
  carrying it", so a file that wrote an index would get *more* explods modified than it asked
  for rather than a differently-chosen one. `angle`, `ownpal` and `shadow` are also unhandled
  inside `ModifyExplod`, and honestly: `angle` and `shadow` appear zero times, and `ownpal`
  needs the palette half of the draw that explods do not yet have (see the previous bullet's
  gap (c)) — the other 8 sites of it are the palette slice's, not this one's.
  - **The shape of the work.** The first three are about entities, and a state machine does not
    own entities — it *asks* the fight to — so `EntitySink` gained `modifyExplod` and
    `removeExplod` beside the `spawnExplod` it already had, and the layer implements them. That
    split is what the tests exploit: the state-machine half is tested on the *ask* (which explod
    was reached, which fields were named) and the layer half on a real `af::EntityLayer` with no
    state machine in the picture at all, so a layer that ignored the request cannot hide behind
    a machine that asked for the right thing.
  - **`ModifyExplod` is a controller that is almost entirely omission.** Every field arrives as
    an `optional` (plus one bare `setTrans` flag, because `trans = none` is a *value* and an
    absent `trans` is a different answer — the same two-level "nothing" the Explod parse
    already had). The one non-obvious consequence, which the reference does not state and the
    engine had to decide: an explod that is **moved** must be re-bound about its new place, or
    the bind it already had drags it straight back on the next tick. So a position modification
    carries *where the owner stood at that moment* along with the new `pos`, and the layer
    recomputes the bind offset from it. That is the one place the two halves of the controller
    cannot be separated.
  - **`GameMakeAnim` is an Explod with a fixed parameter set**, and the one thing that is not
    the same is that its `value` has no `F` prefix to write — there is nowhere else it could
    come from — so it resolves straight into the fight effects. `under` is its `behindOwner`
    ("If under_flag is 1, the animation is drawn behind the character sprites. Defaults to 0"),
    and `random` there is **one number drawn twice, once per axis**, unlike `Explod`'s
    `random = x, y`; the doc's "displaced in the x and y directions by (different) random
    amounts… as large as half of rand_amt" is the reason. Its `bindtime` is written out as 1
    rather than left at the struct's zero, which means something else to the layer — a spark a
    hit throws off stays where it was thrown rather than following the fighter home.
  - **`PowerSet` writes the meter; `PowerAdd` adds to it.** The test had to be built to be able
    to tell them apart at all: the fighter is put at 400 by a `PowerAdd` before the block that
    says 1000, so a write reads 1000 and an add would read 1400. (The first version of the test
    started from 0 and passed under both, which is exactly the kind of check that looks like
    coverage and is not.) The clamp is the same one `poweradd` uses, because the two have to
    agree about what a power total can be.
  - **Fifteen mutations, every one caught**, engine restored byte-identically after each:
    `ModifyExplod` not dispatched (4 checks), an `anim` invented for a block that named none
    (1), `ontop` not outranking `sprpriority` (1), the id/omitted inversion in `RemoveExplod`
    (2), `PowerSet` adding rather than writing (1, after the test was rebuilt to be able to
    tell), `GameMakeAnim` ignoring `under` (1), not going through fightfx (1), unbound (1),
    ignoring `random` (1), using the full spread instead of half (1), and on the layer side:
    ignoring the owner and matching on `id` alone (2), writing fields it was not told to (1,
    three fields at once), a moved explod keeping its old bind (1), `RemoveExplod` ignoring the
    named id (2), and `countExplods` not agreeing with a removal earlier in the same tick (2).
    Suite `12 ran, 0 skipped, 0 failed`. Host-tested only — no device run for this batch, and
    the shared device was held by another session throughout (`adb shell pidof dev.anifight`
    returned 10450).
  - **Still open in this area, flagged not guessed:** `ModifyExplod`'s `angle`, `ownpal` and
    `shadow`; the `Explod` parameter set's own `angle`/`ownpal`; and the rest of the
    recognised-but-inert vocabulary — `width` (141 sections, 21 packs), `makedust` (42, 15),
    and the ones with no corpus presence at all (`envadjust`, `explodbindtime`, `makeanim`,
    `makepalette`, `zoom`, `removemappedexplods`), which are surveyed in
    `docs/mugen-re/INERT-CONTROLLERS-SURVEY.md`.

- **Stage coordinate translation**: ✅ landed (2026-10-19). A stage's numbers are
  read in its own `[StageInfo] localcoord` space and translated into the fight
  window's units before use; `zoffset`, `[PlayerInfo]` starts/bounds,
  `[Camera]` bounds, `[Bound]` margins, `[Shadow] fade.range` and every
  background layer now go through the same factor, and `xscale`/`yscale`
  multiply on top. The fight window is fixed-height (240) and device-width, so
  the factor is the **height** ratio, not the reference's width ratio — the
  deviation and why it preserves the author's composition are written up in
  `docs/mugen-re/STAGE-COORDINATES-RE.md`, along with the top-centre `start`
  placement, the absolute-coordinate `window` clip, and the `spr =
  stages/x.sff` resolution fix that had blacked out three corpus stages.
  Verified on device at 1280×720 (`The Roof`), 960×720 (`Chamber of Souls`),
  640×480 (`Red Moon`) and unchanged at 320×240. Still open in that area:
  stage-embedded `[Begin Action]`, parallax `xscale` pairs/`yscalestart`,
  vertical camera, `overdraw*`/`cut*`, `tilespacing`/`windowdelta`, OLD
  `hires = 1`.

## Known verification gaps (2026-10-18)

- Device install: the APK is ~1.4 GB (99% of it assets/mugen user packs, stored
  uncompressed for zero-mmap reads), so a full install needs a lot of free space and is
  slow. **Space is no longer the blocker it was** — checked 2026-10-18, `/data` on
  RZ8N22ALBMN had 6.1 GB free (it had ~0.99 GB when this note was written). What blocks a
  device run now is contention: the shared device had `dev.anifight` up under *another*
  session's control (pid 10450, 12 minutes of uptime, logging live at 59.9 fps on a 320x240
  menu), and installing over a running instance would kill that session's test. Check
  `adb shell pidof dev.anifight` and how long it has been up before installing, and prefer
  the slim dev build (`-Panifight.devChars=...`) when only a few characters are needed.
  **Re-checked later the same day, still occupied**: `pidof dev.anifight` is that same
  process (pid 10450) at 1 h 10 m of uptime, so nothing device-facing has been verified
  since the note was written. What is waiting on a device run, every item host-tested only:
  the knockback launch fix, the implicit-launch rule and the ground-velocity pair,
  `down.*`/`airguard.*`, `chainid`, `projremanim` and the projectile `F`-prefix resolution,
  `p1facing`/`p1getp2facing`, the `!= 0` knockdown test, and the
  `animtype`/`air.animtype`/`fall.animtype` and hit-type family. The host suite covers the
  logic, but no host test drives a `Renderer` or the fight screen, so what a device run adds
  is specifically that all of it arrives on screen: the reactions a real character's own
  `common1.cns` draws from the new `GetHitVar` values, the sparks, and the facing flips. The
  trans batch above adds itself to that list and for the same reason — no host test can call
  `applyBlend` — and it is the one that most needs eyes, since what it changes is how every
  additive frame in the corpus looks.

- tools/hosttest/test_statemachine.cpp (and siblings) predate today's interfaces; a
  follow-up should extend their build lines (fighter.cpp, entities.cpp, palette stubs).
- MUGEN's evaluator never short-circuits &&/|| — ours now matches (eager), which
  matters under `:=` writes: `x || (var(0) := 1)` always writes, as the binary does.
- **S10 Audio**: ✅ landed — PlaySnd full param set (channel exclusivity replacing the voice,
  loop, volume 0-255 scale, pan/abspan center-scale mapping, freqmul playback rate),
  StopSnd/SndPan channel commands on the mixer's command ring.
- **S11 Game flow**: team modes and real AI script population remain. ✅ Win /
  Continue / Game Over / Default Ending / End Credits screens and the
  storyboard player; ✅ VictoryQuote and `[Quotes]`.
  ✅ **Stage select** (2026-10-19, anifight extension): MUGEN picks a match's
  stage from the character's select.def list and has no screen for it, but this
  roster names no stages, so every match landed in the first extra stage.
  `screens/stage_select_screen.*` now lists every loaded stage under
  `[Info] displayname` (defaulting to `name`, the key MUGEN documents for that
  list), confirms with `A`/Start and backs out with `B`; `FightScreen` takes the
  choice as an explicit stage and keeps it for the whole arcade run. The screen
  replaces itself with the fight, so the return-to-title pop count is unchanged.
  Host-tested displayname parsing and device-verified against the 17-stage
  build (selection reached the fight's own `stage` log line).
- **S12 Rendering extras**: ✅ AngleDraw (drawSpriteRotated, pivot at the character axis,
  AngleSet/Add/Mul sticky; .air frame angle adds at draw).
  ✅ **Shadows** landed — stage-owned `[Shadow] color/intensity/yscale/fade.range`
  and `[Reflection] intensity` parsed; `[Config] drawshadows` is the global kill
  switch; every fighter and helper draws its current animation frame as a
  ground-projected silhouette squashed by `yscale`; Explod `shadow` and
  Projectile `projshadow` draw per-object silhouettes with the documented colour
  sentinels (`r == -1` for Explod, any component for Projectile); per-machine
  `noshadow` hides only that machine's own shadow while `globalnoshadow` hides
  every pass. Known approximations: (a) the reference's two-pass tinted/black
  blit is drawn as one sprite pass with the stage colour as tint and the
  post-intensity alpha as `srcAlpha` — the pre-intensity tint pre-scale cannot
  be expressed by a renderer that multiplies tint by texel; (b) `globalnoshadow`
  is engine-global in the reference and is approximated by OR-ing the flag
  across both fighters and every helper; (c) object shadows are drawn with their
  own layer pass rather than in the reference's separate object-shadow passes,
  which changes only what can cover a shadow; (d) `size.shadowoffset` is
  accepted-but-inert; (e) ✅ **reflections landed 2026-10-19** for fighters and
  helpers — `[Reflection] intensity` draws the current animation frame mirrored
  about the ground plane at `intensity/256` alpha under the same
  `drawshadows`/`noshadow`/visibility gates, with the body's palette binding and
  no `yscale` squash; object (explod/projectile) reflections are still open.
  Tests: `tools/hosttest/test_stage_shadow.cpp` (helpers) and
  `tools/hosttest/test_reflection_draw.cpp` (the actual draw call).
  remaining: zoom (screen + FightFX zoom factors), object reflections.
- **`sub_47F380`** (`0x47F380`) — ✅ **settled, RE pass 11** (was "open RE item, highest
  value"). It is the **HitDef active-block filler**, not a palette executor: the state
  controller reads the parsed template (`a0`) and writes the 500-byte block at
  `player+0x11B0` (`a1`), which the HitDef executor calls at `0x478E88–0x478EC9` before
  setting `hitdefBlock+0x0 = 1`. Pass 11 (`FINDINGS.md`) recovers the full
  template→active map and confirms every offset pass 9 had named (+0x84 p1stateno,
  +0x88 p2stateno, +0x8C owner-record gate, +0x148 p2getp1state, +0x00 armed), and names
  +0x13C as `air.type`. The INDEX label is corrected from the pass-6 guess
  `palfx_executor_b` to `hitdef_fill`. **Remaining:** each field's runtime consumer
  (the chain test, the meter arithmetic, the placement ops) and the envshake per-slot
  naming / tail-record write, listed in pass 11 §11.9.

## Verification discipline

- Every semantic choice cites either (a) a decompiled-function note in
  `docs/mugen-re/decompiled/` + FINDINGS.md, or (b) a Wine black-box observation log.
- Per-sprint: `./gradlew :app:assembleDebug` must pass; `run` skill launches on the shared
  device only after pid check.
- Roadmap state is updated here as slices land.

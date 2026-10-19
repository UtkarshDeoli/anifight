# MUGEN 1.0 Binary Reverse Engineering — Finding Ledger

Non-commercial interoperability research on the reference binary (`local_mugen/mugen.zip`).
Purpose: ground anifight's from-scratch engine semantics in observed MUGEN behavior/structure.
**Rule: no decompiled/recompiled code ever enters `app/`. Behavior notes only.**

## Target identity

| Field | Value |
|---|---|
| File | `mugen.exe`, 694,272 bytes |
| SHA-256 | `887d00a04e94cd8df394304822b2a36f956516aa60c113576fd8ba9d3f9a06c4` |
| Format | PE32, x86 (i386), GUI subsystem |
| Compiler | MSVC 9.0 (Visual Studio 2008), linker 9.0 |
| Built | 2011-01-18 (= MUGEN 1.0 release) |
| Packing | none (plain unpacked image) |
| License | Elecbyte, non-commercial use only, no modification/redistribution (Appendix A) |

Sections: `.text` RVA 0x1000 (0x996fc), `.rdata` 0x9b000, `.data` 0xa9000, `.rsrc` 0x12d000. ImageBase 0x400000. Entry 0x99653.

## Imports (what the engine is made of)

- Rendering: SDL 1.2 + SDL_gfx via software surfaces, or OpenGL+GLEW+GLU (two render paths)
- Audio: SDL_mixer (mikmod/libmpg123 are transitive deps)
- Text: SDL_ttf (freetype transitive)
- Runtime: MSVCR90, pthreadVC2, zlib1, libresample, WINMM
- **No libpng import in mugen.exe itself** → PNG handled inside SDL_image; SFF v1/v2 sprite decode is hand-rolled inside `.text` (relevant to our SFF parser parity)

## Disassembly status

- Full `objdump` (Apple LLVM 21) dump: 195,990 instructions → `/tmp/mugen_peek/mugen_disasm.txt` (scratch)
- 2,246 strings cataloged in `.rdata`; 1,744 have ≥1 code xref (2,257 total refs)
- xref tooling: `/tmp/mugen_peek/xref.py` (scratch; rebuild if /tmp is wiped)

## Anchors into the core fight-engine subsystems

(String VA → code reference; confirmed by disassembly inspection)

| Subsystem | Anchor string | Code xref |
|---|---|---|
| Per-frame state controller executor | `"State no: %i; Ctrl: %i; Type: %i; Movetype %i; Time: %i"` | 0x44f33f |
| Trigger expression evaluator (redirects: `target,…`, `enemy,…`, `partner,…`, `helper,…`) | `"…in redirected trigger from player…"` + 6 "Illegal … in retargeted trigger" strings | 0x443db6 (region ~0x443c00–0x444000) |
| Trigger compiler/parser | `"Invalid trigger: %s"`, `"Missing trigger code %i"`, `"trigger%i"`, `"triggerall"`, `"trigger1"` | 0x4859c6, 0x48883e, 0x477ce5… |
| Expression counter (load-time) | `"%i expressions (%i on trigger lines)"` | — |
| CNS parser section scanner | `"statedef "` cluster | 0x48a90c–0x48abc3; `"ctrl in statedef should be 1 or 0"` @ 0x48b8d6 |
| HitDef controller | `"hitdef"` 0x474a25, `"hitpausetime"` 0x48375c, `"hitdefattr"`, `"hitdefpersist"` | region ~0x474000–0x484000 |
| Vel/Pos controllers | `"velset"` 0x474924, `"posadd"` 0x47488b, `"targetvelset"` 0x476021, `"hitvelset"` 0x474da7 | — |
| hitpause plumbing | `"ignorehitpause"` | 0x478156, 0x47cb32 |
| Controller param reader | `"ctrl.time"` | 0x453ba9 |

Observed in executor region (0x44f213–0x44f33f): per-state debug HUD formatting
(player name, state no, ctrl, type, movetype, time-in-state) — confirms the
executor iterates the player's current-state controller list each frame and the
hitpause gate (`cmpl $0x0, 0xc48(%esi)` / `fld1` scaling) lives adjacent at 0x44f288.

## Tooling gaps

- REA MCP not registered in this Claude Code session; `rea doctor` running in background.
- No Ghidra/Hopper/radare2 on host. Current capability: objdump disassembly + string-xref
  only → good for structure/semantics notes, not pseudocode.
- Optional upgrade: `brew install --cask ghidra` for headless decompile of selected
  functions (e.g., trigger evaluator @ 0x443db6 region). Needs user approval; large install.

## Next passes

1. Function-boundary map of 0x440000–0x490000 (fight engine core), then name functions.
2. Reconstruct trigger evaluator operand dispatch (the redirect + expression opcodes).
3. HitDef field layout from `"hitdefattr"`/`"hitpausetime"` readers → confirm against
   anifight's `HitDef` struct semantics.
4. Physics: gravity/friction/veladd application order — locate via `"posadd"` xrefs.
5. Optional: black-box runs under Wine to cross-check timing semantics (hitpause, juggle).

## Pass 5 — decompiled semantics (angr 10, 2026-10-18)

angr pseudocode for 14 anchored functions → `docs/mugen-re/decompiled/angr_*.c`
(Ghidra recovered boundaries but its decompiler has no mac_arm_64 build; angr's is
pure Python + unicorn. Rough output, semantics-legible.)

**Confirmed behaviors (diff-relevant):**

- *HitDef defaults* (hitdef_schema_parse @ 0x47DA40):
  - `guard.pausetime` absent ⇒ copies `pausetime` **both components**; written with one
    value ⇒ second component copies pausetime's victim component.
  - `kill` = `guard.kill` = `fall.kill` = 1 by default. `hitonce` defaults to
    `(attr & 0x1C0) != 0` — hypers/heavies get hitonce=1 without being asked.
  - `hitflag` absent ⇒ ORs in `0x3E` (=61/62 bits = M|A|F defaults). Both hitflag and
    guardflag *accumulate* with OR — `+`/`-` operators modify rather than replace.
  - `animtype` letter map: l=0 m=1 h=2 b=3 u=4 d(iagup)=5. The *map* matches
    anifight's `parseAnimType`; the **matching rule does not** — the reference
    compares one lowercased character, not a prefix of the name. See pass 16.
    `air.animtype` absent ⇒ copies `animtype`.
  - Struct offsets (DWORD indices into the HitDef record): id +8, chainid +20,
    air.juggle +104, numhits +212, p1stateno +304, p2getp1state +328, forcenofall +392,
    fall.damage +412, fall.xvelocity +428, fall.yvelocity +440, fall.recover +452,
    fall.recovertime +464, p1facing +608, p1getp2facing +620, mindist +632, maxdist +656,
    p2facing +680, ground.hittime +696, ground.slidetime +708, air.hittime +764,
    fall +776, down.bounce +824, down.hittime +836, guard.dist +932,
    pausetime (victim,actor) @ +188/+200, guard.pausetime @ +944/+956, animtype +376,
    air.animtype +760, fall.animtype +424, yaccel +1028 (float), guardflag +280 (OR),
    hitflag +284 (OR; default |62).
- *HitOverride* (0x47D820, was mislabeled "hitdef attr reader"): attr bitfield via the
  same parser HitDef uses; stateno optional(-1), `time` default 1, `forceair` default 0.
- *Helper* (0x481EC0): helpertype normal=0/player=1/proj=2 (proj needs build flag
  sub_40e400(4)); facing default 1; stateno default 0; id/name/pos optional.
- *SuperPause trigger latch* (0x4824B0): `pausebg` default 1; time/movetime/
  endcmdbuftime remain unset (-1) until written.
- *Debug HUD* (0x44EEF0 is the overlay printer, not the executor): confirms player
  object +0xC4C = state no, and adds +0xE54 = time-in-state(+1), +0xE58 = statetype,
  +0xE5C = movetype, +0xE64 = ctrl, +0xC38/+0xC3C = "in <player>'s state" — the
  custom-state owner pointer pair behind TargetState/P2GetP1State.

Open questions for later passes:
- helper redirect self-exclusion rule (expression_eval @ 0x482B00 region)
- exact hitpause scaling factor at executor gate 0x44F288 (fld1 scaling observed pass 2)
- Pause/SuperPause field execution (not just latching) — where the freeze is applied


## Pass 2 — object layout discoveries (hand-read asm, pre-Ghidra)

Anchor-region analysis of the trigger warning printers (0x443d10 / 0x443d80):

**Player object layout (partial, confirmed):**
- `+0x04` — player ID/index int (the `(%i)` in warnings)
- `+0x20` — player name C-string (i.e. 32-byte header precedes name)
- `+0xC4C` — current state number
- `+0xC48` — (from executor @ 0x44f288) hitpause/ctrl gate flag

**Engine-global at `0x52b40c`** (single global pointer, dereferenced everywhere):
- `+0x10EA8` — current trigger line number (used in "%-4i:" warnings)
- `+0x12058` — cached value (usage TBD)
- `+0x80A8` — referenced right before hitpause scaling in executor (TBD: frame timing?)

Also: 0x443d80 is a fastcall-style (args in eax/ecx/edx) tail helper — VC9 custom-calling-convention
error reporter shared by the redirect evaluator; 0x443dd0 begins the operand-list iterator
(`fld1`, `cmpl 0x14(%edi)` bound check, 4-byte stride array walk) = expression evaluation core.

## Pass 4 — Ghidra structure map (2026-10-18)

Headless analysis ran clean: `ghidra_12.1.4_PUBLIC` (SHA-256 verified against the official
release digest) + Adoptium Temurin 21, `analyzeHeadless` + `AnalyzeAnchors.java`
(docs/mugen-re/ghidra/). Project cached at /tmp/mugen_peek/ghidra-proj (10 s analysis).

**Decompiler unavailable on this host**: Ghidra's decompiler is a native binary and the
12.1.4 release ships only `win_x86_64`/`linux_x86_64` builds of it — no `mac_arm_64`.
So no pseudocode from Ghidra here; semantics verification continues via angr (pip) or
hand-reading. Structure/recovery data is still valid.

Real function boundaries now confirmed (Ghidra recovery, not prologue heuristics):
- `0x473ED0` (0x3C4E bytes): the controller-dispatch megafunction — the whole
  `type = X` chain from `tagin` to `null` lives in one function (a single stricmp chain).
- `0x451C20` (0x25EA): controller parameter reader / evaluator family (`ctrl.time` etc).
- `0x473ED0` contains: changestate…hitdef…velset… (anchors 0x47401e–0x477ce5 all inside).
- `0x47D820` (0x13B): HitDef `attr` reader. `0x47DA40` (0x174B): HitDef field schema parse
  (attr→envshake chain, the pass-3 ordered list).
- `0x4824B0` (0x16C): SuperPause param reader. `0x481EC0` (0x4D3): Helper param reader
  (helpertype→size.shadowoffset).
- `0x482B00` (0x2CDA): expression/eval region containing the redirect dispatch.
- `0x44EEF0` (0x57A): per-frame executor (contains the hitpause gate 0x44F288 and the
  HUD debug formatter 0x44F33F).
- `0x4861E0`/`0x48A900`/`0x48AB00`/`0x48B5F0`: trigger compiler, CNS statedef scanner,
  section attach, ctrl validator.
- `0x477C80` (0x549): triggerall/trigger%i/persistent/ignorehitpause reader.

## Pass 3 — canonical vocabularies (string→code xref extraction, 2026-10-18)

Recovered by scanning every code reference into `.rdata` (1,981 strings with refs, improved
resolver catching both `$0x…` immediates and bare `0x…` operands). Raw map:
`/tmp/mugen_peek/all_resolved_strings.txt` (rebuildable via scratch scripts; /tmp is volatile).
These lists define the exact surface anifight must implement for character compatibility.
All names are case-insensitive `stricmp` chains, address-ordered below.

### Controllers (dispatch chain 0x47401e–0x47780b, ~80 entries)

tagin, tagout, changestate, selfstate, ctrlset, turn, changeanim, changeanim2, playsnd,
stopsnd, sndpan, lifeset, lifeadd (kill, absolute params), powerset, poweradd, posset, posadd,
posfreeze, velset, veladd, velmul, gravity, hitdef, reversaldef, hitby, nothitby, hitvelset,
hitfallset, hitfalldamage, hitfallvel, hitadd, attackmulset, defencemulset, statetypeset,
statetype, movetype, physics, hitoverride (slot param), movehitreset, victoryquote,
assertspecial (flag/flag2/flag3), projectile, width (edge/player params), sprpriority,
angledraw, angleset, angleadd, anglemul, trans (none/default/add1/addalpha + alpha),
screenbound, movecamera, targetbind, bindtotarget, targetstate, targetpoweradd, targetlifeadd,
targetvelset, targetveladd, targetfacing, targetdrop (excludeID, keepone, attackdist),
playerpush, varset, varadd, varrangeset (first/last + value/fvalue), varrandom, offset,
envshake, fallenvshake, envcolor, gamemakeanim, afterimage, afterimagetime, pause, superpause,
makedust (pos2, spacing), palfx, bgpalfx, allpalfx, remappal (source, dest), explod,
removeexplod, explodbindtime, modifyexplod, forcefeedback, helper, destroyself, parentvarset,
parentvaradd, bindtoparent, bindtoroot (time, facing), displaytoclipboard, appendtoclipboard,
clearclipboard, null.

Trigger-line machinery (0x477ce5+): `triggerall`, `trigger%i` (1..n), `persistent`,
`ignorehitpause`; warnings "Need at least 1 trigger".

Explod/ModifyExplod param set (0x47c6cc–0x47ce00): anim, pos, vel(ocity), accel, random,
supermove, supermovetime, pausemovetime, removetime, scale, postype, bindtime, sprpriority,
ontop, facing, vfacing, shadow, ownpal, removeongethit, ignorehitpause, trans, alpha.
PalFX param set (0x47ce3a–0x47d1xx): time, mul, add, sinadd, invertall, color.
AfterImage params (0x47bc33–0x47c1xx): timegap, framegap, trans, alpha, palcolor,
palinvertall, palbright, palcontrast, palpostbright, paladd, palmul, time, length (max enforced).
EnvShake params (0x47c1ed–0x47c4xx): time, dir, ampl, freq, phase.
Helper params (0x482000–0x482460): helpertype (`proj`), facing, stateno, keyctrl, ownpal,
pausemovetime, supermovetime, plus size.* params (xscale, yscale, ground.back/front,
air.back/front, height, proj.doscale, head.pos, mid.pos, shadowoffset).
SuperPause params (0x4824b4–0x482a00): time, movetime, endcmdbuftime, pausebg, anim, pos,
darken, p2defmul, poweradd, unhittable, sound.
PlaySnd params (0x47d44b region): freqmul, loop, abspan (+ channel, volume, lowpriority
elsewhere), with validation warnings for channel/groupno/sndno/freqmul.

### HitDef field schema (parse order 0x47d865–0x47f200) — canonical checklist

attr, affectteam, chainid, nochainid (incl. nochainID2 in warnings), sparkno, guard.sparkno,
sparkxy, pausetime, guard.pausetime, kill, guard.kill, fall.kill, hitonce, numhits, damage,
getpower, givepower, priority, hitsound, guardsound, ground.slidetime, ground.hittime,
ground.velocity, air.velocity, air.juggle, air.hittime, snap, mindist, maxdist,
p1sprpriority / sprpriority, p2sprpriority, p1facing, p1getp2facing, p2facing, p1stateno,
p2stateno, p2getp1state, forcestand, forcenofall, fall.damage, fall.xvelocity, fall.yvelocity,
fall.recover, fall.recovertime, yaccel, down.velocity, down.bounce, down.hittime,
guard.slidetime, guard.hittime, guard.velocity, guard.ctrltime, guard.dist,
ground.cornerpush.veloff, air.cornerpush.veloff, down.cornerpush.veloff,
guard.cornerpush.veloff, airguard.cornerpush.veloff, airguard.velocity, airguard.ctrltime,
fall, air.fall, guardflag, hitflag, animtype, air.animtype, fall.animtype, ground.type,
air.type, envshake.*, fall.envshake.*, (hitID / p2shaketime validated via warnings).
ReversalDef at 0x474afc: requires `reversal.attr`.

Validation warnings confirm types/ranges: negative hitID/chainID/nochainID/juggle points,
negative p2shaketime/pausetime, illegal sound group.

### Triggers (0x482e34–0x4841xx, both halves of one chain)

Redirects (0x482b9e–0x482dxx): parent, root, helper, target, partner, enemy, enemynear,
playerid. Core (0x482e34+): time, statetime, animtime, animelem, anim, vel, pos, screenpos,
timemod, p2dist, p2bodydist, p2life, winperfect, win, lose, drawgame.
Long-name half (0x483000+): matchover, gametime, roundno, roundstate, matchno,
roundsexisted, teamside, teammode, ishometeam, frontedgedist, backedgedist,
frontedgebodydist, backedgebodydist, life, lifemax, power, powermax, alive, command, ailevel,
hitcount, uniqhitcount, movehit, moveguarded, movecontact, movereversed, inguarddist,
stateno, prevstateno, p2stateno, statetype, p2statetype, movetype, p2movetype, ctrl,
hitdefattr, fvar, sysvar, sysfvar, random, numproj, numprojid, projcanceltime, projhittime,
projhit, projguardedtime, projguarded, projcontacttime, projcontact, p1name, name, p2name,
p3name, p4name, authorname, numpartner, numenemy, facing, hitfall, hitshakeover, hitover,
hitvel, hitpausetime, canrecover, animelemno, animexist, selfanimexist, playeridexist, palno,
numexplod, numtarget, gethitvar, const, const240p, const480p, const720p, stagevar,
tickspersecond, ishelper, numhelper, parentdist, rootdist, standby, gamewidth, gameheight,
ifelse, asin, acos, atan, floor, ceil, animelemtime, cond.
Win/Lose take an optional `time` param (0x484092/0x4840e6). Per-version gating exists:
"Trigger not valid for mugenversion of character" (mugenversion field enforcement).

GetHitVar field names (0x4841da–0x4845xx): xveladd, yveladd, type, animtype, airtype,
groundtype, damage, hitcount, fallcount, hitshaketime, hittime, slidetime, …(chain continues;
exhaustive tail pending Ghidra pass).

### Notes for anifight parity work

- The expression evaluator has a fixed stack ("Expression stack overflow" @ 0x482a9a
  boundary) — operand iterator begins 0x443dd0 (4-byte-stride array, count at +0x14 of frame
  object). Redirect resolution is a named dispatch over the 8 redirects above.
- `p3name`/`p4name`, `teammode`, `matchno`, `ishometeam`, `standby` confirm full simul/turns
  team-mode trigger surface in the engine (not just versus).
- `tickspersecond`, `const240p/480p/720p`, `stagevar` are the coordinate-space API — relevant
  to our localcoord handling (stage code must expose stagevar + const scaling).

## Pass 6 — runtime evaluator + per-frame state runner semantics (2026-10-18)

Re-reads of the angr set with pass-5 cross-refs. **Identity corrections first** (files kept,
only the names were wrong):

- `angr_004861e0_trigger_compile_a.c` (0x4861E0) is **not a compiler** — it is the *runtime
  trigger/redirect evaluator*: redirect resolution at entry, then one giant opcode switch
  (`Missing trigger code %i` default at line 2079). 2150 lines of runtime semantics.
- `angr_0048b5f0_cns_ctrl_validator.c` (0x48B5F0) is **not load-time validation** — it is
  the *per-frame state runner*: hitpause gate, statedef param application on state entry,
  persistent/ignorehitpause machinery, and the controller trigger→execute loop.
- `angr_00451c20_param_reader.c` (0x451C20) is the fight.def/lifebar loader, not a
  controller param reader.
- `angr_00454230_unknown_454230.c` (0x454230) is engine resource teardown (TTF_CloseFont,
  surface frees) — resolve as shutdown.
- `angr_00477c80_trigger_line_reader.c` (0x477C80) confirmed as the triggerall/trigger%i/
  persistent/ignorehitpause block reader.

### 1. Redirect runtime resolution (evaluator entry, 0x4861E0 lines 163–337)

The compiled node carries: opcode @ +0x0C, **redirect code @ +0x10** (v19->field_10),
comparison code @ +0x14, arg/axis @ +0x1C, float-result flag @ +0x18, negate flag @ +0x28.

Execution order per redirect code (v21 = field_10), each first evaluating the optional
`(expr)` argument via `sub_439260(1)`:

| code | redirect | arg clamp | lookup call | failure warning |
|---|---|---|---|---|
| 1 | parent | none (no arg allowed) | player's +0x147C parent ptr (:177) | "has no parent" (:180) |
| 2 | root | none | player's +0x1480 root ptr (:197) | sub_443cd0 silent-warn (:200) |
| 3 | helper | <0 → 0 (:214-218) | `sub_450b00(self, n)` (:219) | "has no helper %i" (:222) |
| 4 | target | <−1 → −1 (**target(id) exists**, id = HitDef hit ID; −1 = any) (:234-239) | `sub_465130()` (:240) | "has no target with hit ID %i" (:243) |
| 5 | partner | <0 → 0 (:259-263) | `sub_464990(n)` (:264) | "has no %i-th partner" (:267) |
| 6 | enemy | <0 → 0 (:274-279) | `sub_464a50(self, n)` (:280) | "has no %i-th enemy" (:283) |
| 7 | enemynear | <0 → 0 (:295-300) | `sub_464b20()` (:301) | "has no %i-th nearest enemy" (:304) |
| 8 | playerid | <0 → 0 (:314-319) | `sub_464c10()` (:320) | "tried to access non-existent player with ID %i" (:323) |

- **Failed redirect → returns 0**: on any lookup failure the evaluator pushes a bottom
  result (tag 4, value 0 — `g_52b440[idx]=4; g_52b640[idx]=0;` :187-188, or the same via
  `v17=4; v15=0; sub_482a90(); return 0.0;` e.g. :225-228) and aborts that node's
  evaluation. The expression does NOT abort wholesale; the bottom value rides the stack
  (observed-in-pseudocode). Downstream comparisons against bottom come out false
  (inferred from tag-4 checks in callers of sub_488e20/sub_488ec0, e.g. :1889).
- **"Illegal … in retargeted trigger" warnings**: emitted *from inside this evaluator*.
  v5 = (redirected entity != asking entity) is computed at :346 (`v5 = v23 != v22`); error
  paths then pick `sub_443d80()` (retargeted reporter) vs `sub_443cd0()` (plain reporter),
  e.g. out-of-range var/fvar/sysvar/sysfvar at :1879-1886, :1929-1936, :1949-1952.
  `sub_443d10` is the "Player %s (%i) in state %i: %s" formatter. Neither printer is
  decompiled yet (0x443CD0/0x443D10/0x443D80 region).
- **parent/root take no argument**: compile side — expression_eval.c :538-549 both
  `goto LABEL_482d92` (comma+trigger-name read) directly; a `(` after parent/root fails
  the comma check → compile error (return 0 at :621). helper/target/partner compile the
  optional `(...)` via sub_438c40 (:558/:567/:577); enemy/enemynear/playerid compile a
  constant n via sub_438d20(v2) (v2 = 0 for enemy/enemynear, −1 for playerid,
  :592-604, :3834) then also accept `(...)`. **playerid requires the argument**:
  `if (v10 == 8) return 0;` at expression_eval.c :3856.
- **helper self-exclusion: UNRESOLVED.** From a helper, `helper` calls
  `sub_450b00(asking_entity, n)` (:219); whether that accessor enumerates the root's
  helper list, the asker's own sub-helpers, or self-excludes lives inside 0x450B00,
  which is not in the decompile set. Next-pass target (this decides anifight's helper
  redirect self-rule).

### 2. Evaluation order, stack, short-circuit

- **Expression stack is a global pair of parallel arrays**: `g_52b640` (values) +
  `g_52b440` (tags), depth counter `g_4a905c`. **Capacity 128** — push allowed only while
  top < 127, else `sprintf("Expression stack overflow.")` (evaluator :183-191; the same
  guard lives in sub_482a90, called on every push). Tags observed: 0 = int, 1 = float,
  4 = bottom/error; 2/3 appear as alternates inside ifelse/pop helpers (angr typing noise
  possible on 2/3).
- **No short-circuit in && / ||** (inferred, strong): the runtime opcode switch contains
  *zero* logical-operator cases (jump opcodes would appear here); operators are stack
  operations applied by the postfix walker after both operands are pushed. Corroboration:
  **ifelse (opcode 0x131 = 305) evaluates all three arguments eagerly** — case 305 pops
  three already-computed values via sub_437db0 (:1276-1287) and merely selects one
  (:1313-1344). **cond (opcode 0x190 = 400) is lazy** — case 400 evaluates the condition
  subtree, then evaluates exactly ONE branch subtree via sub_4393b0 (:2019-2076), and
  propagates eval-failure tags. Compile side: ifelse → 0x131 (expression_eval.c :1065-1068),
  cond → 0x190 (:1121-1124). Matches MUGEN 1.0/1.1 documented behavior.
- **postfix comparison application**: after the trigger body, the node's comparison op
  (field_14) is applied — kinds 0..5 = ==/!=/>/>=/</<=, kinds 10..13 = interval forms
  (10: node∈[lo,hi] inclusive both; 12: lo<node≤hi; 13: lo<node<hi; 11 → 0,
  likely angr-restructured; :2086-2130) with `lo`/`hi` from node +0x1C/+0x20. Negation
  flagged nodes apply `node = 1 − node` (:2131-2132). This comparison block only runs for
  the int-valued trigger opcodes listed at :2084.
- **Trigger-line grouping** (line reader, 0x477C80): per-line table, 16 B stride —
  {+0 used flag, +4 serial, +8 group index, +12 line-in-group} (:326-331). triggerall
  compiled separately and marked group −1 via sub_4030f0(&v9, −0x1, v72) (:372).
  Semantics = (all triggerall) AND (OR over trigger%i groups, AND within group);
  group-level short-circuit is inside the trigger-set runner sub_486040 (not decompiled).
  Defaults: `persistent = 1` (:382-385), `ignorehitpause = 0` (:386-389).
  "Need at least 1 trigger" when no trigger lines at all (:396).

### 3. Trigger opcode map (compile, 0x482B00 :647-1124) — canonical for anifight

time/statetime=0, animtime=1, animelem=2, anim=3, pos=4, screenpos=5, vel=6, timemod=7,
p2dist=8, p2bodydist=9, p2life=10, frontedgedist=frontedgebodydist=0xB(distinct arg),
backedge…=0xC, life=0xF, lifemax=0x10, power=0x11, powermax=0x12, alive=0x13, command=0x14,
ailevel=0x19, hitcount=0x1E, uniqhitcount=0x1F, movehit=0x20, moveguarded=0x21,
movecontact=0x22, movereversed=0x23, inguarddist=0x24, stateno=0x28, prevstateno=0x29,
p2stateno=0x2A, statetype=0x2B, p2statetype=0x2C, movetype=0x2D, p2movetype=0x2E,
ctrl=0x2F, hitdefattr=0x30, random=0x33, numproj=0x3C, projhit/projguarded/projcontact
cluster=0x3E-0x40 (proj*time variants 0x151-0x153), p1name/name=0x5A, p2name=0x5B,
p3name=0x5C, p4name=0x5D, authorname=0x5E, numpartner=0x5F, numenemy=0x60, id=0x61,
facing=0x63, hitfall=0x64, hitshakeover=0x65, hitover=0x66, hitvel=0x67,
hitpausetime=0x68, canrecover=0x69, palno=0x6B, winperfect=0x6C, win=0x6D, lose=0x6E
(behind sub_40e400 feature gates 3/4 — angr association, verify), drawgame=0x6F,
matchover=0x70, gametime=0x71, roundno=0x72, roundstate=0x73, matchno=0x74,
roundsexisted=0x75, teamside=0x78, teammode=0x79, ishometeam=0x7A, numexplod=0x96,
numtarget=0xA0, gethitvar=0xB4, const=0xB5, stagevar=0xB6, tickspersecond=0xBE,
ishelper=0xC9, numhelper=0xCA, parentdist=0xCB, rootdist=0xCC, standby=0xCD,
gamewidth=0xD2, gameheight=0xD3, animexist=0x14C, selfanimexist=0x14D, playeridexist=0x14E,
animelemno=0x15C, numprojid=0x154, const240p=0x15D, const480p=0x15E, const720p=0x15F,
projcanceltime=0x15B, var=0x157(?), fvar=0x158, sysvar=0x159, sysfvar=0x15A, pi=0x12C,
e=0x12D, ifelse=0x131, log=0x136, sin..ceil=0x140-0x14A, animelemtime=0x14B, cond=0x190.

Runtime confirmations from the evaluator switch (offsets are player-struct bytes):
- vel x returns **facing × stored x-vel**: case 6 reads facing *(+0x1E8) × float *(+0x204);
  vel y at +0x208 (:545-556). Confirms internal vel is facing-relative on read
  (pos x/y at +0x1F4/+0x1F8 — case 4, camera-adjacent global at g_52b40c+69368).
- life=+0x1B8 (case 15 :676-678), p2life reads OTHER player +0x1B8 via pair cache
  (case 10 :596-607) — cross-confirms life offset.
- stateno=+0xC4C (case 40 :756), prevstateno=+0xC50 (case 41 :759), p2stateno reads the
  cached p2 struct +0xC4C (case 42 :762-774) — cross-confirms pass-2 +0xC4C.
- statetype=+0xE58, movetype=+0xE5C, ctrl=+0xE64 (cases 43/45/47 :775/:791/:807) —
  cross-confirms the HUD fields of pass 5.
- time-in-state=+0xE54 raw (case 0 :463-465; HUD prints it minus 1 → counter starts at 1
  on entry).
- **hitpausetime = +0xE70** (case 104 :938-940) — the very counter gating the runner below.
- hitfall=+0x10C8 (case 100 :915-918), hitshakeover = (+0x1090 ≤ 0) (case 101),
  hitover = (+0x1094 < 0) (case 102), hitvel x = facing × +0x10B0 / y = +0x10B4
  (case 103 :925-937). canrecover uses +0x10D8/+0x10DC/+0x10E0 (case 105 :941-945).
- hitcount = +0x13F8, uniqhitcount = +0x13FC (cases 30/31 :727-732).
- movehit/moveguarded/movecontact/movereversed (cases 32-35 :733-752) gate on a contact
  code at **+0xE8C** (values 1, 2, 4 discriminate guarded/contact/reversed-ish — exact
  name↔code pairing has angr-artifact risk) and return frames-since from **+0xE90** + 1.
- hitdefattr (case 48 :810-822): valid only while movetype==1(A); reads stored attr at
  **+0x11BC**, masks low 3 bits (S/C/A class) AND high bits (attack type) of the compiled
  bitmask — matching the documented SSA/NA/SA/… syntax split.
- projhit/projguarded/projcontact (cases 62-64 :830-871): projectile-contact state code at
  **+0x260** (≠0/==1/==2/==3 variants), proj ID filter at +0x25C vs node arg, time from
  +0x264. field_28 negate flag honored.
- random = rng() % 1000 → **0-999 inclusive** (case 51 :824-826, sub_4599c0).
- command (case 20 :691-702): per-command "matched" byte array at player +0x9B4+idx;
  full re-eval sub_460200 when needed. Command-buffer machinery (buffer len, window)
  lives in the CMD parser — NOT in the decompile set; the "120-frame default buffer"
  question stays open (next-pass: decompile CMD compile/matcher region, sub_460200 area).
- const(varname) (case 181 :1147-1174): values in a per-player array from +0x50
  (36+4*n within the stat block pre-0x2C region), g_49d27c marks float-vs-int entries,
  result scaled by localcoord scale at (CNS-data +0xA4) via sub_4652a0/sub_40f160.
- const240p/480p/720p (cases 349-351 :1978-2018): 480p divides by 640.0, 720p by 1280.0
  after multiplying by a game-width global — the documented coordinate-space conversions.
- tickspersecond is a plain global (case 190 :1207-1209, g_52b848).
- palno is stored 0-based, returned +1 (case 107 :946-948, +0x1420).
- ishelper (case 201 :1210-1212) = (entity-type field +0x1C == 1) — entity type codes
  0=player, 1=helper (proj=2 per helper_params pass 5). Parent/root pointers at
  **+0x147C / +0x1480** confirmed by the redirect table above.

### 4. Hitpause + controller frame loop (0x48B5F0, the per-frame runner)

- **Gate: player+0xE70 (hitpausetime counter).** The entire body — state-entry init AND
  the controller scan — is under `if (*(idx + 3696) <= 0)` (:84 onward); when the counter
  is > 0 only controllers with their **ignorehitpause** record flag (controller record
  field_c, 16-byte records) run (:228: `(!hitpause || v25->field_c)`), and the runner
  entry-init block is skipped entirely.
- **persistent machinery**: per-controller counters live in the PLAYER object at
  +0xC54 (3156), one byte per controller slot, cleared to 0 on every state entry
  (:121-129). A controller whose controller-record field_8 (persistent value) > 0 re-arms
  its counter to that value after firing (else 1); each run frame the counter decrements
  (clamped ≥ 0) only when the controller was eligible this frame (:233-251).
  512-controller cap per state: "state %i can only have max of %i controllers" (:130-135).
- **Controller execution**: sub_486040 evaluates the controller's compiled trigger set;
  on true, sub_4781d0 executes the controller body. Return bits: **&4 → "a state change
  happened this frame" flag** (v6, returned from the runner), **&3 → stop processing
  further controllers** (return immediately) (:236-241). This is the MUGEN rule "once a
  ChangeState/SelfState fires, remaining controllers of the old state don't run this
  frame" (inferred mapping — the executor bits are observed, their consumers aren't).
- **State-entry application** (:84-210, all skipped during hitpause): on entry the runner
  * clears hit bookkeeping: +0x13F8/+0x13FC (hitcount/uniqhitcount) unless
    hitcountpersist (:103-107), +0xE8C/+0xE90 (movehit/contact code+timer) unless
    movehitpersist (:108-112), +0x11BC/+0x11B0 (hitdefattr storage) unless
    hitdefpersist (:113-118);
  * sets "valid state" flag +0xE98 = 1 (0 when the statedef lookup failed, :86-100) —
    +0xE98 is the HUD "(invalid)" gate;
  * applies statedef type→+0xE58, movetype→+0xE5C, physics→+0xE60 **with −1 = keep
    previous** (:137-142);
  * facep2: evaluates the statedef flag, computes signed x-distance to the opponent
    (sub_464cd0), flips facing (sub_46b410) when behind (:143-148; the `v7 &&` guard there
    reads angr-confused — treat the exact precondition as artifact-prone, but the
    distance-then-turn sequence is the auto-face rule, executed AT STATE ENTRY);
  * sprpriority→+0x11E4, −1 = keep (:149-157);
  * statedef `velset x,y`: float params via sub_439330, sentinel = −FLT_MAX
    (−3.4028234663852886e+38, read through sub_439330 = "param absent"), **x is
    multiplied by facing (+0x1E8) before store** → +0x204/+0x208 (:160-177);
  * statedef `anim`: int param, −1 = keep; on apply → anim change (sub_46b0c0) and
    **+0xC48 cleared** (:178-188). +0xC48 is also the HUD dimming flag for the anim line
    (frame HUD :182-206) — pass-2's "+0xC48 hitpause gate" reading should be refined:
    the actual hitpause counter is +0xE70; +0xC48 is an anim/state-entry bookkeeping
    flag (its executor-side use at 0x44F288 remains from the hand-read pass).
- **Pause/SuperPause freeze of other players, p2defmul application point: NOT in the
  decompile set.** The SuperPause latch reader (0x4824B0) covers time/movetime/
  endcmdbuftime (−1 = unset) and pausebg (default 1) only — the anim/pos/darken/
  p2defmul/poweradd/unhittable/sound params live past 0x482530 and the freeze/defmul
  application is runtime code elsewhere (likely the player frame-step around the
  sub_4649xx-0x464Cxx accessor family). Next-pass targets.

### 5. ChangeState semantics (compile side; runtime NOT decompiled)

- Controller type codes: changestate=1, selfstate=2 (:1036, :1167). Both compile the
  same param set: `value` (required, → record +24), `ctrl` (optional, → +36), `anim`
  (optional, → +48), with **sub_438d20(−1) sentinel defaults for ctrl and anim**
  (:1040-1043 vs :1171-1174) — pattern consistent with the runner's −1 = keep convention
  above: omitted ctrl ⇒ leave ctrl unchanged; omitted anim ⇒ keep current anim.
- tagin=551 / tagout=552 exist as full controllers in the dispatch chain
  (params stateno/partnerstateno/ctrl, :1082-1132).
- Self-change / negative-state guards: happen at runtime (state lookup sub_4032f0 →
  0xFFFFFFFF ⇒ runner takes the invalid branch, 0x48B5F0 :82-97). The ChangeState
  execution handler itself (where any "changed to negative state" guard would print)
  is not in the set. Controller-record layout from the parse chain: type @ +16,
  compiled `value` @ +24, `ctrl` @ +36, `anim` @ +48.
- **Controller dispatch table** (parse chain in 0x473ED0, name→type code): 1 changestate,
  2 selfstate, 3 turn, 4/5 varset/varadd (tail of chain), 6 varrandom, 7 varrangeset,
  20 ctrlset, 10 lifeset, 11 lifeadd, 12 powerset, 13 poweradd, 21 posset, 22 posadd,
  23 posfreeze, 24 velset, 25 veladd, 26 velmul, 27 **gravity (a controller, not physics
  — adds yaccel to vel)**, 32 changeanim, 33 changeanim2, 34 playsnd, 35 stopsnd,
  36 sndpan, 28 offset, 30 hitby & nothitby (same code pair — two param variants),
  37 hitdef, 38 reversaldef, 39 projectile, 40 width, 50 screenbound, 51 sprpriority,
  60 angledraw, 61 angleset, 62 angleadd, 63 anglemul, 65 trans, 70 targetbind,
  71 bindtotarget, 72 targetstate, 73 targetpoweradd, 74 targetlifeadd, 75 targetvelset,
  76 targetveladd, 77 targetfacing, 78 targetdrop, 90/91 playerpush area (pairing
  tentative — angr nesting artifacts around the stricmp chain tails), 100 hitfallset,
  101 hitvelset, 102 hitfalldamage, 103 hitfallvel, 104 hitadd, 105 attackmulset,
  106 defencemulset, 110 statetypeset, 120 hitoverride, 130 movehitreset,
  140 victoryquote, 150 assertspecial, 200 envshake, 201 fallenvshake, 202 envcolor,
  203 gamemakeanim, 210 afterimage, 211 afterimagetime, 212 pause, 214 superpause
  (213 slot unseen — displaytoclipboard or clip*?), 215 makedust, 216 palfx/bgpalfx/
  allpalfx (shared record, sub-discriminated by +0xB8 field, st_473ed0_3.field_b8),
  217 remappal, 220 explod, 221 removeexplod, 222 explodbindtime, 223 modifyexplod
  (has a build-flag gate sub_47c5d0(a2,1)), 300 helper, 301 destroyself,
  304 parentvarset, 305 parentvaradd, 312 clearclipboard, 320 bindtoparent,
  321 bindtoroot, 0 null (and forcefeedback shares record code 0 with a different path —
  check). Type codes live at controller record +16.

### 6. Misc sharp observations

- **Value/physics cross-reference now triple-confirmed**: facing +0x1E8, vel x +0x204,
  vel y +0x208 (runner velset write ×facing; evaluator case 6 read ×facing).
- movetype/statedef codes: movetype values seen gating hitdefattr (=1 → A-valid);
  statedef type/movetype/physics are int3 at statedef-record +0x04/+0x08/+0x0C with −1.
- The "Expression stack overflow" cleanup at evaluator :182 encodes per-opcode operand
  counts: opcodes 320-351 pop 1, 310 (log) pops 2, 305 (ifelse) pops 3 — matching the
  case handlers' sub_488ec0/sub_437db0 pop patterns; 400 (cond) counts 2…3 per branch.
  (observed; arithmetic rendered oddly by angr but the table shape is legible).
- win/lose behind sub_40e400(3)/(4) feature gates at compile (expression_eval.c
  :695-701) — same gate function as the `proj` helpertype (helper_params) and
  winperfect neighbor. Whether these are mugenversion or build flags: unresolved
  (sub_40e400 not decompiled).
- Friction constants (0.85 stand / 0.82 crouch) and the per-frame
  gravity→integrate→friction order: NONE of the decompiled functions contains them
  (no 0x3F59…/0x3F51… float bit patterns appear anywhere in the set). Physics timing
  remains an open question — next-pass target: the per-player frame stepper that calls
  into the runner (0x48B5F0 caller), located via xref of +0xE70 (hitpause decrement)
  and +0x204/+0x208 (vel integration).

### Pass-6 next targets (not yet decompiled)

1. sub_450B00 — helper redirect accessor (resolves Q1 self-exclusion).
2. 0x443CD0 / 0x443C40 / 0x443D10 / 0x443D80 / 0x443E80 — warning printer family
   (the six "Illegal … in retargeted trigger" strings) and the fatal "Panic: aborting".
3. sub_439260 / sub_4393B0 / sub_486040 / sub_482A90 — postfix stack machine +
   trigger-set group short-circuit + command trigger eval.
4. The per-player frame stepper calling 0x48B5F0 — physics order (gravity/integrate/
   friction/face), hitpause decrement of +0xE70, pause/superpause freeze, p2defmul.
5. ChangeState/SelfState execution handler (controller type 1/2 runtime).
6. CMD parser + sub_460200 region — command buffer length/window defaults.

## Pass 7 — palette FX math, AllPal/BGPal scope, HitDef down/airguard, p2getp1state (2026-10-18)

Decompiles: `angr_0047cdb0_at_0047cdb0.c` (PalFX param reader), `angr_0047d050_at_0047d050.c`
(slot writer), `angr_00472470_at_00472470.c` (per-frame apply + age), the four channel
appliers `angr_00472020/472100/472200/4722d0` (add / mul / invertall / color) and
`angr_004723a0` (identity reset), the pixel blitter `angr_00414320_at_00414320.c`
(+ packer `angr_004142c0`), the controller dispatch `angr_00476a0d_at_00476d25.c`
(fn 0x476A0D) and the HitDef parser re-dump `angr_0047da40_at_0047e6e7.c`.
All addresses are file VAs (ImageBase 0x400000).

### PalFX record / slot layout (0x47CDB0 reader, 0x47D050 slot writer)

Reader (0x47CDB0) fills a heap record (alloc 0xD8 in the dispatch at 0x476D25/0x476D9B/
0x476E0F). Accepted names and their record slots:

| param | names | record offset | default | type |
|---|---|---|---|---|
| time | `time` / `palfx.time` | +0x04 | (required) | int |
| color | `color` / `palfx.color` | +0x10 | **256** | int |
| add | `add` / `palfx.add` | +0x1C,+0x28,+0x34 | 0 | 3× int |
| mul | `mul` / `palfx.mul` | +0x40,+0x4C,+0x58 | **256** | 3× int |
| sinadd | `sinadd` / `palfx.sinadd` | +0x64,+0x70,+0x7C (RGB), +0x94 (period) | 0 | 4× int (R,G,B,period) |
| invertall | `invertall` / `palfx.invertall` | +0x88 | 0 | int |

Bare names (`time`, `mul`, …) and prefixed names (`palfx.time`, …) are both accepted;
which spelling is looked up is selected by the reader's third arg (the `bgpalfx`/`allpalfx`
callers look up the prefixed form). Each entry is a 0xC-stride {value, present-flag, expr}
triple.

The per-frame *applied* slot (0x43C bytes? no — 60-byte active head, zeroed by
`memset(slot,0,60)` at 0x47D050) has this layout (confirmed by `angr_00472470` reads):
`+0x00` elapsed, `+0x04` time, `+0x08` color, `+0x0C/+0x10/+0x14` add R/G/B,
`+0x18/+0x1C/+0x20` mul R/G/B, `+0x30` invertall flag, `+0x34` sinadd present flag,
`+0x38` expired/done flag. `mul` warning fires if any of +0x18/+0x1C/+0x20 out of range;
`color` warning fires if <0 or >256.

### 1. `PalFX.time` semantics — HOLD, not fade (0x472470)

```
v5 = (int)slot->[+0x04]                 ; time
if (!v5) return;                        ; time==0  -> controller is a no-op
if (engine+0x11700) age_enable = 0;     ; global freeze disables aging
if (engine+0x10EBC) return;             ; global gate: skip entirely this frame
if (slot->[+0x00] >= time && time != -1) {
    memset(slot,0,60); slot[+0x08]=0x100;         ; color -> 256
    slot[+0x18]=slot[+0x1C]=slot[+0x20]=0x100;    ; mul   -> 256
    slot[+0x38]=1;                                 ; done
    return;
}
... apply channels ...
if (age_enable) slot->[+0x00] += 1;     ; the only aging tick
```

- `time` is a **hold duration in frames**: while `elapsed < time` the FX stays at full
  strength; when `elapsed >= time` the slot is snapped to identity (mul=256, color=256,
  add=0, time=0) and marked done. **No interpolation / no linear fade.**
- `time == -1` → never expires (permanent). `time == 0` → no-op. Other negatives warn
  ("invalid PalFX time", reader 0x47D050 checks `< -1`).
- This **contradicts the current implementation assumption** of "linear fade over `time`
  with mul easing to 1.0 and add easing to 0".

### 2. PalFX math — 8-bit integer, not 0..1 (blitter 0x414320)

The real per-pixel pipeline lives in `sub_414320` (called from the draw path at 0x413B1C
with the struct base `lea 0xc(%ebx),%esi`; the `invert` flag is struct+0x20 = drawobj+0x2C
and source pixels are 4-byte RGBA: R,G,B,A):

```
L    = (5*R + 7*G + 4*B) >> 4                       ; luma (0..255)
blend= color==256 ? C : (color*C + (256-color)*L) >> 8   ; per channel
v    = (blend ^ invertmask) + add                   ; invertmask = 0xFF if invertall
out  = clamp( (mul * v) >> 8 , 0, 255 )             ; mul==256 -> identity
pack to 16-bit: ((outR&0xfff8)*32 | outG)&0xfffc)*8 | outB>>3
```

- **`color`** (0..256) is a **greyscale blend toward luminance**, in 8-bit fixed point:
  `color=256` → original; `color=0` → pure luma (`0x414320` case `v13==0`); intermediate
  → `(color*C + (256-color)*L)>>8`; `color>256` → treated as no-op (falls into the
  `mul/add`-only path). Direction confirmed: higher color = *less* grey.
- **`mul`** is 0..256 with **256 = identity**, applied as `(mul*channel)>>8`
  (`sub_472100`, 0x472100; guard `if (v11==0x100 && y==0x100 && z==0x100) return`).
- **`add`** is a signed 8-bit offset added to the (post-invert) channel (`sub_472020`).
- **`invertall`** toggles draw-object +0x2C (`sub_472200`: `obj[0x2C] = !obj[0x2C]`); at
  pixel time it XORs each channel with `0xFF` (a full negative), i.e. `255 - v`, applied
  **before** `add` and `mul`.
- Order is `((invert^blend)+add)*mul >> 8`, **all integer in 0..255**, with `mul` 0..256.
  This **contradicts** the "`rgb*mul+add` in 0..1 space, mul 0..256" half of the
  assumption (mul is 0..256, but there is no normalised 0..1 arithmetic anywhere).

### 3. `sinadd` phase/period + its own counter (0x472470 → 0x47254E)

Sine handler (0x47254E):
```
fildl elapsed                       ; numerator = frame counter
fidivl period                       ; / period  (4th sinadd value)
fmull 0x4a6cc8                      ; * pi   (<- 8-byte double pi)
fadd  st,st(0)                      ; *2     => 2*pi*elapsed/period
call  0x49a6c6                      ; sine
... scale by each RGB amplitude, ftol (0x499690), feed into the add path (sub_472020)
```
- Phase = `2*pi * elapsed / period`, i.e. **a full period every `period` frames**.
- `elapsed` is the **effect's own `+0x00` counter** (the same field aged by
  `slot->[+0x00] += 1`), **not** the global frame clock.
- The counter only advances when `age_enable` is non-zero, and `age_enable` is forced to
  **0 while `engine+0x11700` is set** — i.e. the counter **freezes during the global
  pause/hitpause window**. So sinadd is phased on the effect's own elapsed, frozen under
  pause. This **confirms** the current "sinadd phased on the effect's own elapsed
  counter" assumption; it additionally freezes with the global pause flag.
- `engine+0x10EBC` (a second global gate) suppresses application entirely for the frame.

### 4. AllPalFX / BGPalFX scopes (dispatch 0x476D25–0x476F0x)

The four palette controllers are matched by name in one dispatch block, each allocating a
fresh 0xD8 record, parsing it via 0x47CDB0, and stamping a **scope byte at record+0x00**
before storing the record pointer at `controller_this+0x60`:

| controller | name VA | record scope byte | slot (writer 0x47D050) |
|---|---|---|---|
| `palfx` | 0x4A36C4 | `0` | **caller player + 0x1438** (per-player) |
| `bgpalfx` | 0x4A36CC | `1` | **engine + 0x10E14** (global) |
| `allpalfx` | 0x4A36D4 | `2` | **engine + 0x10E50** (global) |
| `remappal` | 0x4A36E0 | (n/a) | params `source` 0x4A36EC, `dest` 0x4A36F4 |

`sub_47D050` decides the destination from that byte: `==1` → `eng+0x10E14`, `==2` →
`eng+0x10E50`, else (0) → the player pointer passed in (player+0x1438). So:

- **PalFX is per-player** (each player owns a slot at player+0x1438).
- **AllPalFX and BGPalFX are two distinct *global* slots** on the engine object
  (0x10E50 / 0x10E14) — AllPalFX is a shared global, not per-player.
- The per-frame executors that push a record into its slot are `0x4782A0` (reads
  `0x60(%ebx)` = the controller's record ptr, `angr_004782a0_at_0047a889.c`) and
  `0x47F380` (`angr_0047f380_at_00480308.c`).

### 5. `PalCopy` — does not exist

There is no `palcopy` token anywhere in `.rdata`. The palette-family controllers are only
`palfx`, `bgpalfx`, `allpalfx`, `remappal`. `remappal` is a *palette-index remap*
(`source, dest`), which is unrelated to PalFX parameter copying. If anifight exposes a
`PalCopy` concept it has no 1.0 counterpart; the closest engine behaviour is the
record→slot copy done by 0x47D050.

### 6. `down.*` / `airguard.*` HitDef fields (parser 0x47DA40)

**These ARE HitDef fields** (parsed in the `down.*`/`airguard.*` block 0x47E8D4–0x47ED5D),
with record offsets and defaults read directly from the parse code:

| field | name VA | record offset(s) | default |
|---|---|---|---|
| `down.velocity` | 0x4A42E8 | +0x350 (x), +0x35C (y) | copies `air.velocity` |
| `down.bounce` | 0x4A42F8 | +0x338 | 0 |
| `down.hittime` | 0x4A4304 | +0x344 | **20** (0x14) |
| `down.cornerpush.veloff` | 0x4A4394 | +0x368 | copies `air.cornerpush.veloff` |
| `airguard.velocity` | 0x4A43E0 | +0x3D4 (x), +0x3E0 (y) | copies `air.velocity` |
| `airguard.ctrltime` | 0x4A43F4 | +0x3EC | copies `guard.ctrltime` (+0x398) |
| `airguard.cornerpush.veloff` | 0x4A43C4 | +0x3F8 | copies `guard.cornerpush.veloff` (+0x3C8) |

`down.velocity` / `airguard.velocity` fall back to **`air.velocity`** (parsed to
+0x314/+0x320 at 0x47E307). Related fields in the same block: `guard.slidetime`,
`guard.hittime`, `guard.velocity`, `guard.ctrltime`, `guard.dist`, `air.cornerpush.veloff`
(+0x32C), `ground.cornerpush.veloff`, `guard.cornerpush.veloff` (+0x3C8).

**These are NOT HitDef fields** — they are *movement/config constants* loaded by the
"movement constants" loader (error string "Can't load movement constants" @ 0x4A2A40,
code near 0x4681BD), **not** hitdef params:
`down.bounce.offset` (0x4A2B7C), `down.bounce.yaccel` (0x4A2B90),
`down.bounce.groundlevel` (0x4A2BA4), `down.friction.threshold` (0x4A2BBC).

(`yaccel` 0x4A2A18 is also a hitdef field, +0x404 — but `down.bounce.yaccel` is the
constant, a different string.)

### 7. `p2getp1state` (parser 0x47DA40, runtime read 0x46D749)

- Field `p2getp1state` (0x4A4270) parses into hitdef **+0x148 (328)** — consistent with
  Pass 5. Its default is set explicitly just before the optional override:
  `leal 0x148(%ebp); pushl $0x0; call 0x438D20` (0x47E66E–0x47E676) → **default 0**.
  (Note: this differs from the commonly-quoted "default 1"; the binary sets 0. The two
  neighbours `p1stateno` +0x130 and `p2stateno` +0x13C default to **-1**.)
- Runtime: the field is read at hit time as `cmpl $0x0, 0x148(%edi)` (edi = active hitdef)
  in the hit-resolution path (0x46D749), gating a falling/airborne branch together with
  the victim's `statetype==3` (player+0xE58, per Pass 5).
- Semantic (medium confidence — the state-placement routine itself is not in the current
  decompile set): `p2stateno` is the state P2 is placed into; `p2getp1state` controls
  whether P2's **state-owner pointer pair** (player+0xC38/+0xC3C, the "in <player>'s
  state" pointer behind TargetState/P2GetP1State, Pass 5) is repointed at P1, so that P1
  can subsequently drive P2 with `target, changestate`. With the field 0, P2 enters
  `p2stateno` in its own state table without P1 retaining control. Runtime handler not
  decompiled → flag for a later pass.

### Pass-7 next targets

1. State-placement routine for `p2stateno`/`p2getp1state` (the code that writes
   player+0xC38/+0xC3C at hit time) — resolves Q7 end-to-end.
2. Confirm `engine+0x11700` and `engine+0x10EBC` identities (pause vs hitpause vs
   rounds-freeze) by xref.
3. The `sub_413D50`/`sub_413D80` render-state path (the `a6` branch of 0x472470) — how a
   PalFX applies to a specific draw layer (screen vs BG vs character).

## Pass 8 — .act index order vs the sprite bank (file-format finding, 2026-10-18)

Not a decompile result: this one is settled by the shipped files themselves, and it
overturns the assumption the palette pipeline was built on.

A character's `pal1..pal12` `.act` files and the palettes the sprite bank itself
carries hold **the same colours in opposite index order**. For every character in the
roster that has both a readable bank palette and an `.act`:

    bank[i] == act[255 - i]     for all 256 entries

exactly -- e.g. dbz_goku, dbz_gohan, mugen_hinata, mugen_levi and 20 others at
256/256 reversed and 0/256 direct. The property holds against an SFF v1 sprite's own
inline PCX palette and against an SFF v2 palette node alike (dbz_vegeta, dbz_vegetto
and mugen_izuku_midoriya are each 256/256 reversed), so it belongs to `.act` files
rather than to either sprite layout. A handful of characters sit lower (Kira 162/256,
dbz_vegeta_ssg 1/256) -- for those the `.act` is a genuinely different palette rather
than a copy of the bank's, so the reversal is a ceiling not an equality, but the
direction is never in doubt.

**Which side is authoritative is not a matter of taste.** Feeding one SFF v1 sprite's
raw indices through each candidate palette and looking at the result settles it:
the bank's own palette gives the correct golden-haired SSJ Goku, the `.act` as it
lies gives blue/green confetti -- exactly the artefact the device showed -- and the
`.act` reversed reproduces the bank palette pixel for pixel. The sprite indices are
authored in the bank's order, so an `.act` has to be flipped to be indexed by them.

`tools/palcheck.py` re-runs the comparison for any character folder; the engine flips
an `.act` on load in `Character`'s palette block.

Method note: this could not be settled by reading the binary alone, and it did not
need to be. Wine is unavailable here, so the black-box comparison against MUGEN
itself that this project normally reaches for was not an option -- but the files carry
their own answer, and a rendering of the same indices under each candidate palette is
a more direct test than either. Worth remembering when a format question looks like it
needs the binary: check whether the shipped data already decides it.

The same holds for a character's alternate palettes, which is a stronger test than the
first one. mugen_hinata ships six `.act` files: `1.act` is a copy of the bank's palette
(256/256 reversed, 0/256 direct), and `2.act`..`6.act` are recolours differing from it in
exactly six entries (250/256 reversed). If the order did not need flipping those recolours
would read ~250/256 *direct*; instead direct is 0/256 for all six. Rendering sprite 0
through the flipped alternates gives the clean recolours an author drew, and through the
unflipped ones gives an inverted face. So the flip is not a property of one file, or of a
character's default palette, but of the `.act` format as these tools write it.

## Pass 9 — p2stateno placement, the pause vs slow-motion flags, and the palette-effect scope match (2026-10-18)

Scope: the three questions Pass 7 left open. Answers first, evidence after.

| # | Question | Answer | Confidence |
|---|---|---|---|
| Q1 | the code that writes P2's state and the owner pair at hit time | `sub_46D2D0` 0x46D8FC–0x46D97D, a 4-step straight-line chain (details in 9.2); `sub_46AF10` is `ChangeState`; `sub_48BB50` copies a 4-dword owner record | HIGH for the mechanics |
| Q2 | `engine+0x11700` and `engine+0x10EBC` | 0x10EBC = the **user pause** latch (a mirror of byte `0x52B950`); 0x11700 = the **round-end slow-motion** accumulator | HIGH |
| Q3 | what the `a6` branch of 0x472470 draws through | `a6=1` = AllPalFX only, and it writes the **global** screen palette state on `0x52B34C` (no layer matching at all); `a6=0` = BGPalFX and the per-player slot, applied per layer by `any(entryKey[i] & selector[i]) != 0` over the engine+0x80C0 array | HIGH |

New decompiles for this pass: `decompiled/angr_p9_*.c` (13 files, all INDEX'd).

### 9.1 The active HitDef block (needed for Q1, and a correction to how we read it)

The engine keeps a **500-byte (0x1F4) active-HitDef block inline at player+0x11B0** — not a
pointer: `sub_46D2D0` opens with `leal 0x11b0(%ebp),%edi / movl %edi,0x68(%esp)`
(0x46D308–0x46D30E) and uses that address as the struct base throughout. `sub_46A6B0` inits it
with `pushl $0x1f4 / leal 0x11b0(%edi),%edx / pushl %ebx / pushl %edx / call memset`
(0x46AB45–0x46AB57); adjacent to it, the same routine fills `*(player+0x11A0)[i] = -1` for the
whole array and memsets `player+0x11AC` (0x46AB10–0x46AB42), so +0x11A0/+0x11AC/+0x11B0 are one
group (target list / buffer / hitdef block). The block is **armed** by the HitDef controller's
executor: at 0x478E88–0x478EC9 the executor calls `sub_47F380` with `(ctrlParamBlock=0x60(%ebx),
out=&player->hitdefBlock, player)` and then sets `movl $0x1,(%esi)` (i.e. `hitdefBlock+0x0 = 1`)
at 0x478EAC, clearing `+0x11B4` when the controller's id field equals 0x26 (0x478EB2–0x478EB8).
`hitdefBlock+0x0` is the "no hitdef armed" flag: tested at 0x46D32E, 0x44CEC1, 0x460567,
0x460797, 0x46D0F8, 0x46EA73, 0x46F41F, 0x471781; zeroed at 0x46F3AA, 0x46F547, 0x48B6FA and in
`sub_46EB80`.

Offsets that matter for hit resolution (block base = player+0x11B0, so block+N = player+0x11B0+N):

| Block offset | Player offset | Meaning (as read at hit time) |
|---|---|---|
| +0x00 | +0x11B0 | armed flag (1 = active HitDef) |
| +0x04 | +0x11B4 | "already applied to a target" latch (0x46EBB6/0x46EBCD) |
| +0x0C | +0x11BC | the HitDef `attr` word (ANDed against candidates) |
| +0x30 | +0x11E0 | gate for the "apply now" path (0x46D328/0x46D337) |
| **+0x84** | **+0x1234** | **p1stateno** — `!= -1` ⇒ ChangeState(*attacker*) |
| **+0x88** | **+0x1238** | **p2stateno** — `!= -1` ⇒ ChangeState(*victim*) |
| **+0x8C** | **+0x123C** | **install-owner-record flag** — `!= 0` ⇒ repoint P2's owner pair |
| **+0x90..0x9C** | **+0x1240..+0x124C** | the staged 4-dword owner record (source of the copy) |
| +0x0A4 | +0x1254 | amount added to the hit-counter record (+0x14 of it) |
| +0x118/+0x11C | +0x12C8/+0x12CC | the two float reaction amounts |
| +0x13C | +0x12EC | a **small enum** (0/1/2/3), *not* a stateno — see 9.5 |
| +0x148 | +0x12F8 | `p2getp1state` (matches Pass 7) |
| +0x1C4 | +0x1374 | duration/counter fields used by the victim reaction |

Bonus (same code path, useful for our own hit test): `sub_46D2D0` scans an **8-entry list at
victim+0x1100, stride 0x14**, each entry `{+0 valid, +4 attr word, +8 stateno}` (the loop ends
at +0x11A0). The match test is verbatim `t = entry.attr & hitdef.attr; accept iff
(t & 7) != 0 && (t & 0xFFFFFFF8) != 0` — i.e. at least one of the low three bits (statetype
S/C/A) and at least one bit above bit 2 (attack/guard type) must agree. The entry's `+8` is a
state the victim can be pushed into on the other branch (0x46D9D1–0x46D9EA).

### 9.2 Q1 — the state-placement chain at hit resolution

All of the following is one straight-line block in `sub_46D2D0`, entered when the local flag at
`0x1C(%esp)` is 0 (`cmpl $0x0,0x1c(%esp); jne 0x46D9D1` at 0x46D8F1). Registers: `edi` = the
attacker's hitdef block, `ebp` = the attacker (proved by `movl %ecx,0xe74(%ebp)` at 0x46D9FE
vs the decompile's `a0->field_e74`), `esi` = the victim.

1. **Attacker self-state** (0x46D8FC–0x46D909)

       movl 0x84(%edi),%eax ; cmpl $-0x1,%eax ; je skip
       movl %ebp,%edi ; call 0x46AF10
   → `if (hitdefBlock+0x84 != -1) ChangeState(attacker, hitdefBlock+0x84)`.
   Condition: **exactly `-1` means "no self state change"**; any other value is used as a state
   number. This is `p1stateno`.

2. **Owner-record install, then statetype** (0x46D912–0x46D932)

       cmpl $0x0,0x8c(%edi) ; je skip
       movl 0x68(%esp),%edi ; movl 0x6c(%esp),%esi ; addl $0x90,%edi ; call 0x48BB50
       movl $0x2,0xe5c(%esi)
   → `if (hitdefBlock+0x8C != 0) { sub_48BB50(src = attacker+0x1240, dst = victim);
   victim->statetype(+0xE5C) = 2 }`.
   `sub_48BB50` is register-passed (no pushes at the call site) and does exactly:

       dst+0xC38 = src[0] ; dst+0xC3C = src[1] ; dst+0xC40 = src[2] ; dst+0xC44 = src[3]
       if (dst+0x9B0 != src[2]) { memset(dst+0x9B4, 0, 128); dst+0x9B0 = src[2]; }

   (`angr_p9_statectx_apply.c`; the same field mapping is inlined in `sub_46EB80` at
   0x46F233–0x46F272 with `src = attacker+0x1240..0x124C`, `dst = victim`, plus
   `victim->statetype = 2`.) Note the **source is the attacker's own** +0x1240..+0x124C block,
   i.e. the record is staged on the attacker by the same code that fills the hitdef block
   (9.1) and then handed to the victim. `+0xC38`/`+0xC3C` are the state-owner file pointers,
   `+0xC40` is the "in <player>'s state" id (echoed to `+0x9B0`, which Pass 5 tied to the
   state/cmd set), `+0xC44` its companion — and the 128-byte command buffer at `+0x9B4` is
   dropped whenever the owning id changes.

3. **Victim state + ctrl** (0x46D93C–0x46D97D)

       cmpl $-0x1,0x88(%edi) ; je done
       sub_470140(victim, 0) ; sub_446320(victim->field_4)
       ChangeState(victim, *(0x88(%ecx))) ; victim->ctrl(+0xE64) = 0
   → `if (hitdefBlock+0x88 != -1) { ...; ChangeState(victim, hitdefBlock+0x88); victim ctrl = 0 }`.
   Condition: **`-1` is the default and means "P2 is not moved"** (the documented
   `p2stateno`-omitted case); any other value places P2. Two companion calls precede it whose
   roles are not established here (`sub_470140(player,0)` also runs in the per-frame hitdef
   driver at 0x46D337; `sub_446320` receives the player's `+0x4` index field).

4. **Palette-effect hand-off** (0x46D9A0-ish, per the decompile): when
   `hitdefBlock+0x1CC > 0` (or `== -1`), 15 dwords are copied from `attacker+0x1350` to
   `victim+0x1438` — the victim inherits the attacker's `palfx` slot. HIGH on the copy, MEDIUM
   on which hitdef field enables it.

**`sub_46AF10(player, stateNo)` is `ChangeState`** — confirmed by raw disassembly
(0x46AF10–0x46AF70):

    46af13  movl %eax,%esi                  ; esi = new state
    46af22  movl 0xc3c(%edi),%eax ; jne …   ; owner statefile: use +0xC3C, else…
    46af2c  movl 0xc34(%edi),%eax           ; …fall back to +0xC34
    …       call 0x4032F0 (probe the state table through that owner)
    46af53  movl 0xc4c(%edi),%eax
    46af59  movl %eax,0xc50(%edi)           ; prevstateno(+0xC50) = stateno
    46af5f  movl %esi,0xc4c(%edi)           ; stateno(+0xC4C) = newState
    46af65  movl $0x0,0xe54(%edi)           ; time-in-state(+0xE54) = 0

So the engine's ChangeState resolves the state **through the owner chain first** and only then
writes prevstateno/stateno/time-in-state. That is the single most load-bearing detail for our
`TargetState`/`p2stateno` implementation: the ordering of the four steps above (P1's own state,
then the owner repoint, then P2's state, then ctrl=0) is observable, and step 2 must happen
*before* step 3 or P2's state lookup uses the wrong owner.

### 9.3 Q2 — `engine+0x10EBC` is the pause latch; `engine+0x11700` is round-end slow motion

Every xref, re-verified in the raw listing this pass:

**`engine+0x10EBC` = user pause (latched mirror of byte `0x52B950`)**

- Written once per frame from that byte: `movsbl 0x52b950,%ecx` (0x44B51A) then
  `movl %ecx,0x10ebc(%eax)` (0x44B521) at the top of the input/event handler `sub_44B4C0`.
- `0x52B950` itself: `xorb $0x1,0x52b950` (0x4149ED — an unconditional bit toggle, the debug
  key path), `movb $0x1` at 0x44BD18 and `movb $0x0` at 0x44BD54 (the two arms of the key-code
  302 handler — i.e. the **pause key toggles it**, the branch is taken on the current
  0x10EBC value, cf. 0x44BD23/0x44BD8B), `movb $0x0` at 0x44B5B1 and 0x44B666 (other exits of
  the same handler), and `%bl/%cl` stores at 0x458BE0 / 0x459452 (round-start init, storyboard).
- Copied/cleared wholesale by round-start init (`sub_458D80`: 0x458BDA/0x458BE0),
  `sub_458BB0`, and the storyboard player (`sub_459A10`: 0x45944A, 0x459EEA, 0x45A04C;
  the storyboard's own loop reads `g_52b40c[69308]` = +0x10EBC and mirrors `0x52B950` into it).
- **Effects while set:** `sub_472470` returns *immediately* when it is set
  (`if (g_52b40c->field_10ebc) return;`, decompiled 0x4724xx) → no palette effect is applied at
  all; the frame-work guards `cmpl …,0x10ebc(…)` at 0x43DD57, 0x4470E5, 0x44712A, 0x44714D,
  0x44715E, 0x4474FE, 0x447528, 0x4484D4, 0x448512, 0x44863E, 0x448844, 0x448C72, 0x448D82,
  0x449234, 0x44936C, 0x449388, 0x44BD23, 0x44BD8B, 0x44C569, 0x44CFA7, 0x44E497, 0x44E718,
  0x44F856 sit across the whole per-frame gameplay update (0x4470xx–0x44Fxxx: player update,
  animation, the palette-effect drivers, camera zoom, the round machine). This is the *global
  pause*, not hitpause.

**`engine+0x11700` = round-end slow-motion accumulator**

- Written **only** by the round machine `sub_449BC0` (0x44A3F1, 0x44A6B6, 0x44A817, 0x44A82C,
  0x44A85B, 0x44A884) together with its span `0x116FC` (0x44A3EB, 0x44A6B0, 0x44A811, 0x44A826,
  0x44A855, 0x44A87E) — the round-end / KO sequence sets span = 500 and value = 500, then
  200 / 100 / 50 / 25 as the sequence advances — and **cleared** by round-start init
  (`sub_458D80`, `movl %esi,0x11700(%ebx)` / `movl %esi,0x116fc(%ebx)` at 0x458DEC/0x458DF2).
- Updated once per frame by `game_update` at 0x447120–0x447147:
  `movl 0x116fc(%esi),%ecx ; movl 0x11700(%esi),%eax ; … movl %eax,0x11700(%esi)` — the
  sawtooth `if (value > 0) value -= 100; else value += span`, so `value == 0` marks a frame on
  which the game actually advances and non-zero marks a skipped frame. Guarded by
  `cmpl %ebx,0x10ebc(%esi)` at 0x4470E5/0x44712A/0x44714D/0x44715E, so pause wins over slow-mo.
- **Effects while non-zero:** `sub_472470` does `if (field_11700) a7 = 0;` → the palette
  effect's own elapsed counter stops ageing (0x4724FF); tests at 0x447191, 0x447530, 0x44851E,
  0x448D8F, 0x449241 (the state-advance block), 0x44A412, 0x44CFB0, 0x44E4A4, 0x44E722 select
  the "hold this frame" behaviour.

**Pass 7's guess is wrong for both fields** and must not be carried forward. Neither is
hitpause: hitpause is the *per-player* counter at player+0xE70 (Pass 5), and neither is the
round-start freeze (that is the hold counter at engine+0x11720 driven by `sub_458D80`, which
resets players to state 5900).

### 9.4 Q3 — how a palette-effect scope meets the layer being drawn

The three effect records are fed to one driver by three call sites in the same routine
(0x44739B, 0x44745D, 0x4474B7), and the argument order is
`sub_472470(record, sel0..sel4, a6, a7)` where `a6` = "global" and `a7` = "age this frame"
(established from the pushes: `pushl $1 ; pushl $eax ; subl $0x14,%esp ; <5 dwords> ;
pushl <record>` ⇒ arg0 = record, args 1–5 = the 20-byte selector block, arg6 = a6, arg7 = a7):

| Call site | Record | Selector passed | `a6` |
|---|---|---|---|
| 0x4473C0 | `engine+0x10E14` (bgpalfx) | `{0x100,0,0,0,0}`, built literally at 0x4473A5–0x4473BD | **0** |
| 0x447497 | `player+0x1438` (per-player palfx) | `player+0x1424,+0x1428,+0x142C,+0x1430,+0x1434` (read at 0x44745D–0x44748E) | **0** |
| 0x4474E1 | `engine+0x10E50` (allpalfx) | 5 dwords assembled from stack locals (0x3C/0x40 and 0x60/0x64/0x68(%esp)) — not resolved to a constant | **1** |

So the `a6` branch is precisely the **AllPalFX-vs-the-other-two** split, not an abstract
"global vs per-entry" mode:

- **`a6 = 1` (AllPalFX, engine+0x10E50).** No layer is matched at all. The record's parameters
  are written into the single screen palette state on the render-state object `0x52B34C`:
  `sub_413D50` sets the triple `{+0x4,+0x8,+0xC}` and `sub_413D80` the triple `{+0x10,+0x14,+0x18}`,
  each flagging the render state dirty only when one of its three values actually changes
  (`field_48 = 1; field_50 = 0;`), and the driver separately compares
  `0x52B34C+0x20` vs the record's `+0x30` and `0x52B34C+0x0` vs the record's `+0x8` with the
  same dirty flagging. That is the whole of `sub_413D50`/`sub_413D80` — see the correction in
  9.5.
- **`a6 = 0` (BGPalFX, player palfx).** The record is handed to `sub_472020` (add),
  `sub_472100` (mul), `sub_472200` (invertall), `sub_4722D0` (color). Each walks the palette
  **entry array at `engine+0x80C0`** — header `{+0x0 = entry base pointer, +0x8 = count}`,
  entries of **stride 0x18 (24 bytes)**, entry `{+0x0 = pointer to the draw object,
  +0x4..+0x14 = the entry's 5-dword layer key}` — and the predicate is, verbatim,

          for (i = 0; i < 5; i++)
              if (entryKey[i] & selector[i]) { apply; break; }

  i.e. **apply to this draw object iff *any* of the five dwords of `(entryKey & selector)` is
  non-zero** (decompiled `sub_472020`, the `do { … } while ((v14 += 4, v14 < 20))` loop; the
  other three apply routines have the same shape). Applied as `drawObj+0x10/+0x14/+0x18 +=
  r/g/b`, plus `drawObj+0x54 = (drawObj+0x54 & ~4) | 1`. Note the per-entry early-out when the
  selector's tail dwords are all zero, and that the "match" reduces with OR (any dword), not
  AND-all — relevant if we ever expose layer masks.
  - `entryKey[0] & 0x100` is therefore the background test: the BG selector is literally
    `{0x100,0,0,0,0}`. **HIGH** that the BG selector is that constant; MEDIUM that bit 8 of key
    dword 0 means "background plane".
  - `player+0x1424..+0x1434` is confirmed as the 5-dword selector that scopes a *player's* own
    palette effect (Pass 5's read is exact).

`a7` = "age the effect this frame": the driver skips ageing under slow motion and (per the
decompile) does not advance `record+0x0` when it is 0; the per-player call site computes it
from a `player+0x228+4*i` slot (0x44744A–0x44745D, meaning not established — LOW).

### 9.5 Corrections and contradictions with earlier passes

1. **Pass 7's `p2stateno` offset is contradicted.** Pass 7 records `p2stateno` parsing into
   hitdef **+0x13C** (with `p1stateno` at +0x130). At hit time the engine reads the two statenos
   from **+0x84** and **+0x88** (0x46D8FC / 0x46D93C, both `cmpl $-0x1`), and the field at
   **+0x13C** is read as a *small enum*, not a state: `sub_46DB20` does
   `victim+0x107C = hitdef+0x13C` and, when `hitdef+0x13C == 3`, forces `victim stateno = 5070`
   (the air-hit state). Either the parse-time template and the 500-byte active block have
   different layouts (the +0x148 coincidence for `p2getp1state` argues for a shared tail), or
   the +0x13C mapping is wrong. **Next function to decompile: `sub_47F380` (0x47F380)**, called
   by the HitDef controller executor at 0x478EA4/0x478EA9 as
   `(ctrlParamBlock, &player->hitdefBlock, player)` — that is the code that fills the active
   block, and it settles template→active offset mapping in one read. Note `sub_47F380` is
   currently INDEX'd as `palfx_executor_b` from a Pass-6 guess; that label is now doubtful.
2. **Pass 7's Q2 guess ("global pause/hitpause freeze") is wrong** — see 9.3. 0x10EBC is the
   pause latch over byte 0x52B950; 0x11700 is round-end slow motion.
3. **Pass 7's `p2getp1state` semantic survives, refined.** `p2getp1state` (active block +0x148)
   is *not* the gate on the owner-record copy: the copy is gated by the hitdef's own +0x8C
   (0x46D912). What `p2getp1state` does at hit time is set the victim's own flag
   `victim+0x10C8 = 1` (`if (v11->field_148) v12->field_10c8 = 1`, in `sub_46DB20`), and
   `+0x10C8` is what the placement/knockback path then tests
   (`if (v24->field_10c8 || hitdefBlock+0x10 & 1 || …)`, `sub_46D2D0`). So "P2's state belongs to
   P1" is implemented as: p2getp1state ⇒ victim+0x10C8 = 1, plus the separate +0x8C/+0x1240
   record hand-off. Both must be implemented to match.
4. **Self-correction to my own Pass-9 draft framing of Q3.** `sub_413D50`/`sub_413D80` are *not*
   the per-layer draw path: they are the **screen-global** (a6=1, AllPalFX) parameter setters on
   `0x52B34C` and do no matching. Matching happens only on the a6=0 path, inside
   `sub_472020`/`sub_472100`/`sub_472200`/`sub_4722D0`.
5. Pass 8's `.act` reversal finding is untouched by this pass and still stands.

### 9.6 What is still open

- `sub_47F380` (0x47F380): the template→active-block filler (see 9.5.1). Highest value.
- The 5-dword selector handed to `sub_472470` for AllPalFX (built from stack locals at
  0x4474C0–0x4474DE) — whether it is a constant screen selector or derived per frame.
- `sub_470140(player, 0)` and `sub_446320(player->field_4)`, called on every `p2stateno`
  placement, and `sub_446320`'s argument convention.
- What `player+0x11A0` (pointer, array filled with -1 by `sub_46A6B0` at 0x46AB10–0x46AB1D for
  `engine+0x11120` entries) and `player+0x11AC` are, and how they relate to the 8-entry,
  stride-0x14 list at +0x1100.
- The exact per-field defaults inside the active block: it is `memset` with the `%ebx` byte at
  0x46AB45 (`sub_46A6B0`), and the value of `%ebx` there was not resolved, so the `-1` defaults
  seen at read time may come from that fill or from `sub_47F380`.

## Pass 10 — animation parameter prefixes: which file a controller's `anim` names (behaviour finding, 2026-10-18)

Not a decompile pass: this one fell out of the shipped character files plus the
engine's own expression parser, and it explains a rendering artefact that had been
seen twice on the device and could not be reproduced on demand.

### The defect

MUGEN lets an animation parameter carry a one-letter prefix saying which of two files
the animation comes from, and the engine was ignoring it -- `firstParam(controller,
"anim", ...)` handed the raw text to `Expr::parse`. That is not a parse failure. An
identifier is a legal expression, so `F72` parses cleanly into a `Trigger` node named
`F72`; and an identifier the engine does not know falls through
`trigger` -> `ownTrigger` -> `opponentTrigger` to `return Value::number(0.0f)`
(`statemachine.cpp`). So `anim = F72` did not draw nothing -- it quietly became
**animation 0**.

Animation 0 is the one animation every character in the roster has: their stand.

### The two conventions are mirror images

Confirmed against the installed roster rather than from documentation, because the
numbers themselves are the evidence:

| parameter | convention | evidence |
|---|---|---|
| `Explod`'s `anim`, `Projectile`'s `projanim` | an `F` prefix names a **fightfx** animation; a bare number is the character's own | the prefixed ones in this roster are `F60` and `F72` -- fightfx's dust range -- while the bare ones run to 1000, 5800, 7400 |
| a `HitDef`'s `sparkno`, a `SuperPause`'s `anim` | the other way round: a bare number is **fightfx**, an `S` prefix is the character's own | super pauses are written bare as `100`, `30`, `101`, `102` and with the prefix as `S3102`, `S6039`, `S-1` |

Fifteen `Explod` sites across ten characters write the `F` form. The most common shape
is a super-jump landing dust:

    [State 5401, Spark]
    type = Explod
    trigger1 = Time = 0
    anim = F72
    pos = 0, floor(screenpos y) - 50
    postype = back
    sprpriority = 3

### The visible symptom

The dust explod therefore drew the **character's own standing animation** instead of
the screenpack's dust, and it did so with the explod's `Blend::Additive`, so a golden
Super Saiyan reads as a bright gold spiky silhouette. `postype = back` is resolved once
by `resolveEntitySpawn` into a fixed *world* point (`stageCenter()`), not a
screen-relative one, so as the camera moves that point slides -- which is why the shape
appeared clipped at the very edge of the screen with the fighters elsewhere. It also
explains why it looked transient and would not sit still to be photographed.

This is the far-left gold artefact. The chain is complete and each link is checkable in
the repo: the file text, the parser path, the constant-zero fallback, and the two
`spawnExplod` call sites.

### Fix

`animTarget(controller, key, context, convention)` reads the prefix off the **raw**
text, with an `AnimPrefix` enum naming the two conventions at the call site rather than
a bare bool. A parameter that is not a plain prefixed number (`anim = var(5)`, or an
expression) is still evaluated the ordinary way and keeps its convention's default file.

What a *malformed* prefix does is worth stating precisely, because the first draft of
this note got it wrong and the regression test caught the difference. A malformed
prefix is **not** "unnamed, draws nothing". `anim = Fxyz` is not a prefixed number, so it
falls through to the expression evaluator -- and there MUGEN's rule that "an unknown
identifier is a string literal" applies: a bare lower-case word of four letters or fewer
is answered as text (`ownTrigger`, the same fallback that makes `statetype = A` work), and
a non-empty string's numeric value is 1. So `Fxyz` lands on animation **1** of the
character's own file, not on 0 and not on -1. A parameter that fails to *parse* outright
(`anim = F-`) does come back as -1. The property the prefix handling actually secures is
narrower and is the one that matters: it can no longer land on animation **0**, the
stand, out of the wrong file. The regression block in `tools/hosttest/test_statemachine.cpp`
pins exactly that (`anim = Fxyz` must not be 0 and must not claim fightfx) rather than
asserting -1, which the engine does not produce.

Wired into the `Explod` controller and the `SuperPause` flash. The `SuperPause` case was
wrong in the other direction and had been since it was written: it forced
`fromFightFx = true` unconditionally, so the bare-number majority was right by accident
and every `S`-prefixed flash was resolved to `0` -- i.e. fightfx animation 0, not the
character's animation.

**Also wired, since (2026-10-18):** `Projectile`'s `projanim`/`projhitanim`/`projremanim`.
This was left open above with the note that "nothing observable turns on it" in this
roster, and while that is still true of the roster it was wrong about the size of the
job: each of the three parameters carries its own prefix, so each needed its own flag on
the `Projectile` (`animFromFightFx`, `hitAnimFromFightFx`, `removeAnimFromFightFx` --
nothing stops a character mixing them, and `projremanim`'s fallback on `projhitanim` has
to carry the file along with the number), and then all four places that act on an
animation had to be taught which file to read: the layer's removal clock, the layer's
flight-animation finish test, the layer's draw -- which selects bank *and* animation
together, as the explod draw already did -- and the fight screen's collision box lookup,
which is the one that decides whether a screenpack-drawn projectile can hit anything at
all and which had been reading the character's file whatever the prefix said.

Regression coverage: four combinations of prefixed and bare across the three parameters
plus the inherited-file fallback, asserted at the parse through the recording sink, and a
layer check that gives fightfx a same-numbered removal animation of a *different length*
from the character's, so the tick count itself says which file was read. Three mutations
each bit the intended checks and only those (dropping the `animTarget` calls fails five;
dropping the fallback's file copy fails one; making the removal branch read the owner's
file unconditionally fails the tick-count check). What is **not** covered by a host test
is the draw path and the fight screen -- no host test drives a `Renderer` or a live
match -- so those two are compile-verified only and are on the device-verification list.

### Follow-on: the pack had seven animations where MUGEN's own has nineteen

Fixing the prefix removed the wrong sprite, which immediately exposed a second problem --
the effect the prefix points *at* did not exist either. `tools/screenpack/gen.py` drew
only 0, 1, 2, 3, 10, 11 and 12, and it mislabelled 10/11/12 as guard sparks (Elecbyte's
guard spark is **40**; 10/11/12 are the blood sparks). The generated pack now holds
nineteen animations -- 0-3, 4, 10-12, 30, 40, 60-62, 70-72, 100-102 -- generated from one
table-driven list feeding both the SFF writer and the AIR writer, so an AIR reference can
no longer name a sprite the SFF lacks. Sizes, frame counts and blend modes were taken from
the reference pair's own dimensions and frame counts; **the art is drawn by the
generator**, as `gen.py`'s header requires. Two members of the reference set are still
absent -- `120` (floor dust) and `130` (power up), which nothing in the roster reaches --
and two of ours have no counterpart in the reference at all: `4` (the hardest hit spark)
and `30` (the super-pause flash). Those last two are our own shapes, flagged as such in
the generator, and their numbering is a bet on the conventional screenpack range rather
than a reading of Elecbyte's file.

A census of what the installed roster actually asks for, against the regenerated pack:

| Form | Result |
|---|---|
| `anim = F<n>` on an `Explod` | only `F221` and `F222` unresolved -- two `[State -2]` KO banners in `dbz_gogeta`, gated on `p2life = 0` |
| bare `sparkno = <n>` on a `HitDef` | **none** unresolved |

`221`/`222` were not in Elecbyte's `fightfx.air` either, so under the reference pack they
draw nothing as well; that is the reference-correct answer, and inventing art for them
would be inventing an effect the character never had.

### Method note

The lesson worth keeping: "an unrecognised value" and "a value that failed to parse"
are not the same thing, and only the second is safe. `Expr::parse` returning a valid
tree for `F72` is what turned a missing-effect bug into a wrong-sprite bug -- and a
wrong sprite is far harder to attribute than a missing one, because it looks like art.
When a prefixed form is accepted in one controller and silently reinterpreted in
another, the two should share a reader.

## Pass 11 — `sub_47F380` is the HitDef active-block filler: the template→active offset map, chainID/nochainID, damage/getpower/givepower, snap/mindist/maxdist and the envshake record (behaviour finding, 2026-10-18)

Pass 9 (§9.5.1, §9.6) left one question on the table: *what is `sub_47F380`
(`0x47F380`), and how does the parse-time HitDef record map into the 500-byte
active block at `player+0x11B0`?* This pass answers it. The primary artifact is
`decompiled/angr_0047f380_at_00480308.c` (483 lines), cross-read against the
parser `decompiled/angr_0047da40_hitdef_schema_parse.c` and the two hit-time
consumers Pass 9 already dumped (`angr_p9_hitresolve.c` = `sub_46D2D0`,
`angr_p9_hitapply.c` = `sub_46DB20`).

### 11.1 `sub_47F380` is the HitDef filler, not a palette executor

Four independent things in the artifact settle its identity.

1. **Its `a1` is the active block.** Every write targets `a1->field_NN`. The
   field at `+0xC` is the attr word — the consumer reads it into `victim+0x10A4`
   (`sub_46DB20` `angr_p9_hitapply.c:251`) and masks it for class/type
   (`sub_46D2D0` `angr_p9_hitresolve.c:277` `node->field_4 & v25->field_c`, and
   `:447` `v25->field_c & 448`). `+0x78` is the hitflag word (`a1->field_78 = a0[71]`,
   executor `:266`; consumed `angr_p9_hitapply.c:261` `field_78 & 1` and
   `angr_p9_hitresolve.c:386`). `+0x140`/`+0x124`/`+0x13C` are the anim-type enums
   Pass 9 traced into `victim+0x1078`/`+0x1080`/`+0x107C`. In short `a1` is
   `player+0x11B0`.
2. **`a0` is the parser's record.** `sub_47DA40(int a0, void *a1)` writes named
   HitDef fields to `a1+byteoffset`; `sub_47F380` reads `a0[N]` and writes the
   active block. `a0[1]` → `a1+0xC` (attr); `a0[70]` → `a1+0x74` (guardflag,
   parser `+280`); `a0[71]` → `a1+0x78` (hitflag, parser `+284`); `a0[173]` →
   `a1+0x124` (ground.type, parser `+692`); `a0[189]` → `a1+0x13C` (air.type,
   parser `+756`). Roughly a dozen such coincidences fix the template→active
   direction; the two records do **not** share offsets.
3. **It names HitDef fields in its own warning strings.** `:169` "assigned
   negative hitID", `:177/:185/:193` "negative chainID"/"nochainID"/
   "nochainID2", `:208` "illegal juggle points", `:222/:230` p2shaketime/
   pausetime, `:240/:247/:255/:262` sound/gsound group/no, `:272` "hit priority",
   `:280/:287/:298` p1stateno/p2stateno/p2bindtime, `:319` recovertime,
   `:327/:335` "illegal spark no"/"gspark no", `:373/:380` ground.p2hittime/
   p2slidetime, `:398` air.p2hittime, `:414` down.p2hittime, `:429/:436/:443`
   guard.p2hittime/slidetime/ctrltime, `:451` guard.p2shaketime, `:458`
   guard.p1pausetime, `:473` airguard.p2ctrltime. No palette executor would name
   `p1stateno`, `down.p2hittime` or `guard.p2ctrltime`.
4. **The call site is the HitDef controller.** Pass 9 recorded the wrapper at
   `0x478E88–0x478EC9` calling it `(ctrlParamBlock, &player->hitdefBlock, player)`
   then setting `hitdefBlock+0x0 = 1`. The declared signature is exactly
   `(unsigned int *a0, st_47f380_0 *a1, st_47f380_3 *a2)`, and `a2` is a player:
   `a2->field_4` = player index, `&a2->padding_8[24]` = player name, `a2->field_c4c`
   = stateno — the fields the warnings print.

So the pass-6 INDEX label `palfx_executor_b` was a guess, and **Pass 7 §4
("AllPalFX / BGPalFX scopes") is wrong where it grouped `0x47F380` with
`0x4782A0` as "per-frame palfx executors"**: `0x4782A0` is the palette side,
`0x47F380` is the HitDef fill.

The executor's own body writes `+0x08 .. +0x19C`; the tail `+0x1A0 .. +0x1F4` is
written by its last three calls — `sub_47D050(a0 + 260)` (`:479`) then
`sub_47C440()` twice (`:480–481`) — which is why the block is 500 bytes and why
the declared struct stops at `0x19C`.

### 11.2 The template → active offset map

`Src` column: **[T]** = copied from template `a0[N]`; **[P]** = evaluated int param
(`sub_439260(n)`); **[F]** = evaluated float param (`sub_439330()` + optional
`ftol`=`sub_499690`); **[helper]** = written by the trailing calls. Line numbers
are into `angr_0047f380_at_00480308.c`.

| Active | Field | Src | Note (default/clamp) | Conf |
|---|---|---|---|---|
| +0x00 | armed flag | — | not written here; wrapper sets `=1` | HIGH (pass 9) |
| +0x08 | (attr-pair low) | [T] `a0[0]` `:160` | template `+0x0` (parser `affectteam`) | MED |
| +0x0C | **attr** | [T] `a0[1]` `:161` | → `victim+0x10A4`; class/type mask consumed | HIGH |
| +0x14 | **hitID** | [P] `:163` | `<0 → 0` + warn `:169` | HIGH |
| +0x18 | **chainID** | [P] `:171` | `<-1 → -1` + warn `:177` | HIGH |
| +0x1C | **nochainID** | [P] `:179` | `<-1 → -1` + warn `:185` | HIGH |
| +0x20 | **nochainID2** | [P] `:187` | `<-1 → -1` + warn `:193` | HIGH |
| +0x24 | bool param | [P] `:198` | consumed as the damage-flag arg (see 11.5) | MED |
| +0x28 | bool param | [P] `:199` | ditto | MED |
| +0x2C | bool param | [P] `:200` | unnamed | LOW |
| +0x30 | bool param | [P] `:201` | the "apply" gate (pass 9); a HitDef param | MED |
| +0x34 | **juggle points** | [P] `:202` | `<-1 → -1` + warn `:208`; zeroed on hit (`hitresolve:389`) | HIGH |
| +0x38 | **damage (hit)** | [P] `:210` | fed to `sub_46E760` (`hitapply:621`, `hitresolve:262`) | MED-HIGH |
| +0x3C | **damage (guard)** | [P] `:211` | fed to `sub_46E760` (`hitresolve:248`) | MED-HIGH |
| +0x40 | **getpower (hit)** | [P arg0] `:212` | `attacker_power += block+0x40` (`hitresolve:424`) | HIGH |
| +0x44 | **getpower (guard)** | [P arg0] `:213` | `attacker_power += block+0x44` (`hitresolve:499`) | HIGH |
| +0x48 | **givepower (hit)** | [P arg0] `:214` | `victim_power += block+0x48` (`hitapply:627`) | HIGH |
| +0x4C | **givepower (guard)** | [P arg0] `:215` | pair with 0x48 | MED-HIGH |
| +0x50 | **p2shaketime** | [P] `:216` | `<0 → 0` + warn `:222`; → `victim+0x1090` (`hitapply:583`) | HIGH |
| +0x54 | **pausetime** | [P] `:224` | `<0 → 0` + warn `:230`; `-1` on first frame `:232` | HIGH |
| +0x58 | **numhits** | [P] `:234` | added to hit counters (`hitresolve:426–429`) | MED |
| +0x5C | hitsound **group** | [P] `:235` | `<-1 → -1` + warn `:240` | HIGH |
| +0x60 | hitsound **no** | [P] `:242` | `<-1 → -1` + warn `:247` | HIGH |
| +0x64 | (flag) | [T] `a0[62]` `:249` | unnamed | LOW |
| +0x68 | gsound **group** | [P] `:250` | `<-1 → -1` + warn `:255` | HIGH |
| +0x6C | gsound **no** | [P] `:257` | `<-1 → -1` + warn `:262` | HIGH |
| +0x70 | (word) | [T] `a0[69]` `:264` | unnamed | LOW |
| +0x74 | **guardflag** | [T] `a0[70]` `:265` | parser `+280` | HIGH |
| +0x78 | **hitflag** | [T] `a0[71]` `:266` | parser `+284` | HIGH |
| +0x78 bit0 | "hit downed" bit | [P] `:305–306` | param → `field_78 |= 1` | MED-HIGH |
| +0x7C | **hit priority** | [P] `:267` | `<0 → 0` + warn `:272` | HIGH |
| +0x80 | (sparkxy hi?) | [T] `a0[75]` `:274` | unnamed | LOW |
| +0x84 | **p1stateno** | [P] `:275` | `<-1 → -1` + warn `:280` | HIGH (pass 9) |
| +0x88 | **p2stateno** | [P] `:282` | `<-1 → -1` + warn `:287` | HIGH (pass 9) |
| +0x8C | owner-record install | [P bool] `:289` | if set → `sub_48BC00()` `:292` | HIGH (pass 9) |
| +0xA4 | **p2bindtime** | [P] `:293` | `< -1 → **1**` + warn `:298` (default 1, not 0) | HIGH |
| +0xA8 | (int) | [P] `:300` | unnamed | LOW |
| +0xAC | (int) | [P] `:301` | unnamed | LOW |
| +0xB0 | **animtype** | [T] `a0[94]` `:302` | → `victim+0x1078` (`hitapply:272`) | MED-HIGH |
| +0xB4 | bool | [P] `:303` | also set when ground.velocity.y≠0 `:387–388` | MED → **pin** |
| +0xB8 | bool | [P] `:304` | → `victim+0x10C8 = 0` (`hitapply:265`) — a *clearer* of the fall flag | MED |
| +0xC0 | (int) | [P] `:307` | → `victim+0x10CC` (`hitapply:563`) | MED |
| +0xC4 | **fall.animtype** | [T] `a0[106]` `:308` | → `victim+0x1078` when `victim+0x10C8` (see 11.11) | MED-HIGH |
| +0xC8 | **scale x** | [F] `:309` | → `victim+0x10D0` (`hitapply:540`) | MED-HIGH |
| +0xCC | **scale y** | [F] `:311` | → `victim+0x10D4` (×p2 const) | MED-HIGH |
| +0xD0 | (int) | [P] `:313` | → `victim+0x10D8` (`hitapply:548`) | MED |
| +0xD4 | **recovertime** | [P] `:314` | `<0 → **4**` + warn `:319`; → `victim+0x10E0` | HIGH |
| +0xF0 | **sparkno** | [P] `:322` | `<-2 → -2` + warn `:327` | HIGH |
| +0xF4 | (F-flag) | [T] `a0[141]` `:321` | consumed `hitresolve:454` | MED |
| +0xF8 | **gsparkno** | [P] `:330` | `<-2 → -2` + warn `:335` | HIGH |
| +0xFC | (F-flag) | [T] `a0[145]` `:329` | consumed `hitresolve:505` | MED |
| +0x100 | (int) | [P] `:337` | damage-scale candidate (`hitresolve:321/359`) | LOW |
| +0x104 | (int) | [P] `:338` | damage-scale candidate (`hitresolve:373`) | LOW |
| +0x108 | p1facing-family bool | [P] `:339` | `= (param<0)`; cleared to 0 `:353` when the next param ≠0 | MED |
| +0x10C | p1facing-family tri-state | [P] `:340–355` | clamped to −1/0/1, default 0 | MED |
| +0x110 | **mindist/snap x** | [P] `:356` | default −1025 (see 11.6) | MED |
| +0x114 | **mindist/snap y** | [P] `:357` | ditto | MED |
| +0x118 | **maxdist/snap x** | [P] `:358` | ditto | MED |
| +0x11C | **maxdist/snap y** | [P] `:359` | ditto | MED |
| +0x120 | **p2facing** | [P] `:360` | out of [−1,1] → 0 + warn `:365` | HIGH |
| +0x124 | **ground.type** | [T] `a0[173]` `:367` | → `victim+0x1080` (`hitapply:259`) | HIGH |
| +0x128 | **ground.p2hittime** | [P] `:368` | `<0 → 0` + warn `:373` | HIGH |
| +0x12C | **ground.p2slidetime** | [P] `:375` | `<0 → 0` + warn `:380` | HIGH |
| +0x130 | **ground.velocity x** | [F] `:382` | → `victim+0x10B0` (`hitapply:299`) | HIGH |
| +0x134 | **ground.velocity y** | [F] `:384` | → `victim+0x10B4` (`hitapply:311`) | HIGH |
| +0x138 | **ground.velocity z** | [F] `:389` | → `victim+0x11A4` (`hitapply:316`) | HIGH |
| +0x13C | **air.type** | [T] `a0[189]` `:391` | → `victim+0x107C`; `==3 → 5070` | HIGH |
| +0x140 | **air.animtype** | [T] `a0[190]` `:392` | → `victim+0x1078` (`hitapply:455`) | MED-HIGH |
| +0x144 | **air.p2hittime** | [P] `:393` | `<0 → 0` + warn `:398`; → `victim+0x1094` | HIGH |
| +0x148 | **p2getp1state** | [F→ftol] `:401` | *setter* of `victim+0x10C8 = 1` (`hitapply:452`), which is not its name — see 11.11 | HIGH |
| +0x14C | **air.velocity x** | [F] `:403` | → `victim+0x10B0` (`hitapply:432`) | HIGH |
| +0x150 | **air.velocity y** | [F] `:405` | → `victim+0x10B4` (`hitapply:444`) | HIGH |
| +0x154 | **air.velocity z** | [F] `:407` | → `victim+0x11A4` (`hitapply:448`) | HIGH |
| +0x158 | **air.fall** (bool) | [P] `:408` | consumed `hitapply:411` | MED |
| +0x15C | **down.p2hittime** | [P] `:409` | `<0 → 0` + warn `:414`; → `victim+0x1094` | HIGH |
| +0x160 | **down.velocity x** | [F] `:417` | → `victim+0x10B0` (`hitapply:375`) | HIGH |
| +0x164 | **down.velocity y** | [F] `:419` | → `victim+0x10B4` (`hitapply:393`) | HIGH |
| +0x168 | **down.velocity z** | [F] `:421` | → `victim+0x11A4` (`hitapply:399`) | HIGH |
| +0x16C | (float) | [F] `:423` | unnamed 4th down velocity | LOW |
| +0x170 | **guard.p2hittime** | [P] `:424` | `<0 → 0` + warn `:429` | HIGH |
| +0x174 | **guard.p2slidetime** | [P] `:431` | `<0 → 0` + warn `:436` | HIGH |
| +0x178 | **guard.p2ctrltime** | [P] `:438` | `<0 → 0` + warn `:443` | HIGH |
| +0x17C | **guard.dist** | [P] `:445` | unnamed | MED |
| +0x180 | **guard.p2shaketime** | [P] `:446` | `<0 → 0` + warn `:451` | HIGH |
| +0x184 | **guard.p1pausetime** | [P] `:453` | `<0 → 0` + warn `:458`; `-1` first frame `:460` | HIGH |
| +0x188 | **guard.velocity x** | [F] `:462` | → guarded-branch velocity | HIGH |
| +0x18C | **guard.velocity y** | [F] `:464` | ditto | HIGH |
| +0x190 | **guard.velocity z** | [F] `:466` | ditto | HIGH |
| +0x194 | **airguard.p2ctrltime** | [P] `:468` | `<0 → 0` + warn `:473` | HIGH |
| +0x198 | **airguard.velocity x** | [F] `:475` | unnamed consumer | MED |
| +0x19C | **airguard.velocity y** | [F] `:478` | scaled/defaulted `hitapply:513–524` | MED-HIGH |
| +0x1A0..+0x1DC | **embedded PalFX** | [helper] | 15 dwords → `victim+0x1438` (`hitresolve:437–445`) | HIGH |
| +0x1DC..+0x1EC | **envshake record** | [helper] | 5 ints → `engine+0x10F28..` (see 11.8) | MED-HIGH |
| +0x1F0 | envshake ptr | [helper] | → `engine+0x10F3C` | LOW |

### 11.3 The offsets Pass 9 named are confirmed

All six offsets Pass 9 pinned hold up, and three of them get a name upgrade:

- **+0x84 = p1stateno**, **+0x88 = p2stateno** — the executor writes them at
  `:275`/`:282` under exactly those warnings. Confirmed.
- **+0x8C = the owner-record install flag** — `:289–292` (`if set → sub_48BC00()`).
  Pass 9's read-time gate (`sub_48BB50` at `0x46D929`) is the same field. Confirmed.
- **+0x148 = p2getp1state** — `:401` (`ftol(sub_439330())`, default 0 as Pass 7 §7
  found in the template). Confirmed; and here it is genuinely a *different* field
  from +0x8C, as Pass 9 argued.
- **+0x00 = armed** — the executor does not write `+0x0`; the wrapper sets it
  (`hitresolve:232/519–521` reads and clears it). Confirmed.
- **+0x13C is not a state**: it is **`air.type`** (`a0[189]` = parser `+756`). Pass 9
  called it "a small fall/animation enum"; the parser's `air.type` case values
  are `n=0/h=1/l=2/t=3`, and both consumers copy `+0x13C` into `victim+0x107C`
  and force state 5070 on `==3` (`hitapply:423`). The name is `air.type`.

The resolution of Pass 9's central puzzle: the parser and the active block are
**two different layouts**, and `sub_47F380` is the map between them. Pass 7 §6/§7
quoted *template* offsets (p1stateno `+0x130`, p2stateno `+0x13C`, p2getp1state
`+0x148`); Pass 9 read the *active* block. Both are right for their own struct.
Pass 9's phrasing "Pass 7's +0x13C/+0x130 are superseded" is misleading — nothing
was superseded; they were never the same record. Only one coincidence survives:
p2getp1state happens to sit at `+0x148` in *both* structs.

### 11.4 chainID / nochainID / nochainID2 — settled (closes an S3 item)

Fill-time (`:171–197`): three int params land at **+0x18 (chainID)**, **+0x1C
(nochainID)**, **+0x20 (nochainID2)**; each `<-1` clamps to `-1` with its own
warning. Then a collision rule: if `chainID == nochainID || chainID == nochainID2`
and `chainID != -1`, **all three are set to `-1`** via `memset(&field_18, -1, 12)`
(`:195–197`). Default is `-1` for all three (the absent param evaluates to `-1`).
This is the whole of what MUGEN's chain-limit bookkeeping needs for the field
layout; the *runtime* juggle/chain test that consumes +0x18/+0x1C/+0x20 lives in
the target-eligibility path and was not part of this pass. HIGH on the offsets,
defaults and collision rule (exact warnings); the *consumer* is the remaining half
of the S3 "chainid/nochainid" item.

### 11.5 damage / getpower / givepower — settled (closes an S3 item)

The executor reads six consecutive params after juggle (`:210–215`):

```
+0x38 = sub_439260(1)   +0x40 = sub_439260(0)   +0x48 = sub_439260(0)
+0x3C = sub_439260(1)   +0x44 = sub_439260(0)   +0x4C = sub_439260(0)
```

The hit-time code consumes them exactly as MUGEN's {damage, getpower, givepower}
each-as-a-pair (hit, guard):

- `+0x38`/`+0x3C` → the damage function `sub_46E760(block+0x38, block+0x24)`
  (`hitapply:621`, `hitresolve:262`) on the hit path and
  `sub_46E760(block+0x3C, block+0x28)` (`hitresolve:248`) on the guard path. So
  `+0x38` = hit damage, `+0x3C` = guard damage (MED-HIGH; the value function itself
  is not decompiled, so which is hit vs guard rests on which branch calls it).
- `+0x40`/`+0x44` → **the attacker's meter**: `sub_46B070(*a0->field_1c8 +
  block+0x40)` on a clean hit (`hitresolve:424`) and `+= block+0x44` on a guarded
  hit (`hitresolve:499`). `a0->field_1c8` is the power pointer (clamped to
  `a0->field_1cc` at `:500–503`, i.e. powermax). So `+0x40` = getpower (hit),
  `+0x44` = getpower (guard). HIGH.
- `+0x48`/`+0x4C` → **the victim's meter**: `sub_46B070(*v12->field_1c8 +
  block+0x48)` (`hitapply:627`). `+0x48` = givepower (hit); `+0x4C` = givepower
  (guard) by the pair. HIGH on `+0x48`, MED-HIGH on `+0x4C`.

The parser stores damage at template `+0x74/+0x80`, getpower at `+0x8C/+0x98`,
givepower at `+0xA4/+0xB0`, but the executor never reads those template slots —
all six arrive as evaluated params. A useful cross-check: the guarded branch also
picks `block+0x184` (guard.p1pausetime) as the attacker's pause
(`hitresolve:493`) whereas the hit branch uses `block+0x54` (pausetime, `:423`) —
the same hit/guard pairing the MUGEN schema implies, which is why the pair
assignment above is trusted.

The four boolean params at `+0x24/+0x28/+0x2C/+0x30` (`:198–201`) sit immediately
before this run; `+0x24` and `+0x28` are the second argument to `sub_46E760`
above, so they are flags on damage (plausibly hitflag/guardflag-adjacent
conditions), and pass 9's `+0x30` "apply gate" is one of them — i.e. it is a
*character-set HitDef param*, not a pure engine latch. Their public HitDef names
could not be recovered from this artifact.

### 11.6 snap / mindist / maxdist — settled to active offsets (closes an S3 item)

Four int params at **+0x110/+0x114/+0x118/+0x11C** (`:356–359`). The parser
(`angr_0047da40_hitdef_schema_parse.c`) sets the template defaults with four
`sub_438d20(-0x401)` calls (value **−1025** = `0xFFFFFBFF`), and when `snap` is
given it writes the `snap` x/y into *both* the mindist pair and the maxdist pair
(`sub_438e30` copies `+632→+656` and `+644→+668`), while `mindist` writes only
`+632/+644` and `maxdist` only `+656/+668`. The consumer confirms the split: the
`hitapply:590–620` block clamps the victim's X/Y offset with four sentinel-tested
ints — `+0x110`/`+0x114` as a *minimum* pair (`if (val > |dx|) shift`, `:596–605`)
and `+0x118`/`+0x11C` as a *maximum* pair (`:612–620`), each skipped when the slot
is `0xFFFFFBFF`. Hence **+0x110/+0x114 = mindist-or-snap (x,y)** and
**+0x118/+0x11C = maxdist-or-snap (x,y)**, default −1025. Confidence MEDIUM: the
slot order is inferred from the consumer (the executor reads these as params, not
as template copies, so the positional correspondence is not read straight off the
artifact). The `+0x108`/`+0x10C` p1facing-family params (bool + tri-state; which is
`p1facing` vs `p1getp2facing` is not pinned) are read just before the clamp and are
the facing op that pairs with them (MEDIUM).

**Since pinned (2026-10-18).** The pair is `(p1facing, p1getp2facing)`, and the
encoding is exact — it is the one place in the fill that does not store what it reads.
`angr_0047f380_at_00480308.c:339–355`:

```c
a1->field_108 = (int)sub_439260(1) < 0;   /* p1facing  -> a single bit */
v25 = sub_439260(1);                      /* p1getp2facing */
if (v25 < 0) v25 = -1; else if (v25 > 0) v25 = 1; else goto LABEL;
a1->field_108 = 0;                        /* non-zero getp2facing CLEARS the bit */
LABEL: a1->field_10c = v25;
```

So **+0x108 = (p1facing < 0) ∧ (p1getp2facing == 0)** and **+0x10C =
clamp(p1getp2facing, −1, 1)**. Two consequences worth keeping. (a) The *order* of the
pair is settled, and not just by the filler: the template layout runs p1facing
(+608), p1getp2facing (+620), mindist (+632/+644), maxdist (+656/+668), p2facing
(+680) — the same order the filler pops them in, with the four sentinel-tested
dist ints between the pair and +0x120, which is what the parser's `else if` chain
(`angr_0047da40_hitdef_schema_parse.c:537–547`) skips over because it tests parameter
names rather than walking the template. (b) **`p1facing` is a sign, not a value.** A
character writing `p1facing = 1` and one writing nothing are stored identically, so
they must behave identically; only a negative value turns the attacker around. The
docs ("Set to -1 to make P1 turn around") agree, but the *corpus* is where it
matters: 51 `p1facing` lines, 44 of them `= 1` — a no-op that a value-reading
implementation would have to invent a meaning for. The remaining uses are the
interesting ones and all three are the same idiom, a throw that picks its own facing:
`ifelse(command = "holdfwd", 1, -1)` (`mista.cns`, `Giorno.cns` and their AI copies)
and `ifelse(frontedgebodydist > backedgedist, 1, -1)` (the corner-aware variant, which
throws away from the near wall). `p1getp2facing` is 36 lines, 34 of them explicit
zeros. The consumer is still not extracted, so the *action* taken on the bit is read
from the doc (turn around = negate the attacker's facing) rather than off the binary;
that is the one part of this that stays at MEDIUM.

**A caveat on `p2facing`, which is next door and is *not* a mirror of this.** The one
consumer line we do have for it (`angr_p9_hitapply.c:248`) is not a sign test but an
equality test against a computed value —
`if (p2facing == -(v8 * victim->field_1e8)) victim->field_1ec = -victim->field_1e8;` —
which reads as "act only when the written value already names the direction the
computation wants", i.e. `0` and the *unmatched* of `±1` both do nothing. The engine does
not read it that way: there, `1` is the ordinary turn toward the attacker (identical to
leaving the parameter out), `-1` is the turn away, and `0` keeps the victim's facing. We
have left the engine alone, because the corpus is lopsided in a way the equality reading
cannot explain — 323 lines write `p2facing = 1` and 6 write `-1`, and under the equality
reading half of those 323 would flip the victim. Either `field_1e8` is not the victim's
facing, or the comparison is doing something else; the consumer wants a proper read
before either reading is called settled.

### 11.7 down.* / airguard.* — fill-time offsets

Fill-time confirms Pass 7 §6's *template* set and gives the active offsets:

- **down.***: `+0x15C` = down.p2hittime (`<0→0`, warn `:414`); `+0x160/+0x164/+0x168`
  = down.velocity x/y/z (floats, `:417/:419/:421`), with `+0x16C` a fourth unnamed
  float (`:423`). The consumer uses `+0x160/164/168` as the down velocity triple
  and the `-FLT_MAX` sentinel for "absent" (`hitapply:375–400`). HIGH on `+0x15C`
  and `+0x160–0x168`; LOW on `+0x16C`.
- **airguard.***: `+0x194` = airguard.p2ctrltime (`<0→0`, warn `:473`); `+0x198/+0x19C`
  = airguard.velocity x/y (floats, `:475/:478`). The consumer only exercises
  `+0x19C` (defaulted to `0.35/320` or scaled at `hitapply:513–524`), so `+0x198`
  is MED.
- Also newly pinned by warning string: guard.velocity x/y/z = `+0x188/+0x18C/+0x190`
  (`:462–467`), guard.dist = `+0x17C` (`:445`).

Pass 7 §6's *parser* defaults (down.velocity def = air.velocity, down.bounce def 0,
down.hittime def 20, airguard.ctrltime def = guard.ctrltime, …) are template-side
and unaffected; the executor's fill-time defaults are the `→0`/`→−1` clamps listed
above. Both are true for their own struct.

### 11.8 The HitDef envshake record

The active block ends with a **5-int record at +0x1DC/+0x1E0/+0x1E4/+0x1E8/+0x1EC
plus a pointer at +0x1F0** — exactly the 0x18 bytes that take the block from
`+0x1DC` to the 500-byte boundary. On a successful hit `hitapply:525–534` copies
all six into the engine globals `g_52b40c+0x10F28..+0x10F3C`, gated on
`field_1E0 != 0`. Two facts make the identity "envshake" well-supported rather
than guessed:

- five ints matches EnvShake's parameter count (time/freq/ampl/phase/dir);
- the **expression evaluator** recognises exactly five identifiers —
  `fall.envshake.time`, `.dir`, `.ampl`, `.freq`, `.phase`
  (`angr_00482b00_expression_eval.c:2677/2717/2758/2798/2840`) — which is the
  HitDef-record redirect form, i.e. the HitDef carries a named envshake record of
  five values.

What is **not** settled: the hit-time write gate tests `+0x1E0`, not the first
slot, so the per-name slot order is not derivable from the consumer; the `+0x1F0`
pointer's meaning is unknown; and the *parse* of the envshake fields is not in
`sub_47DA40`'s body (it reads no envshake string), so it is in the parser's
unexamined tail (`sub_47C1E0`, called at parser end) and/or the executor's
`sub_47C440` helpers. Existence, count, engine destination and "reaches the
victim/attacker via the engine globals" are MED-HIGH; per-slot naming is not
settled. Also note `fall.envshake.*` is an *expression* identifier, not a
controller-dispatched string: the `envshake`/`fallenvshake` names in the dispatch
(`angr_00473ed0_controller_dispatch.c:4025`→type 200, `:4087`→type 201) are the
controllers; the field reads go through the evaluator.

### 11.9 What could not be settled

- **The hit/guard ordering within each pair.** The split of `+0x40/+0x44` and
  `+0x48/+0x4C` (and `+0x38/+0x3C`) rests on *which branch* consumes each (clean hit
  vs guarded) and on `sub_46E760`'s not-decompiled body for damage; the pair
  ordering is MED-HIGH, read from usage, not from a name.
- **The four boolean params +0x24/+0x28/+0x2C/+0x30** have no warning string, so
  their public HitDef names are unknown. `+0x30` is a character-set param that
  gates the apply path.
- **The exact template→active correspondence for param-stream fields.** For fields
  copied from the template (`a0[N]` → `a1+off`) the map is read directly; for
  fields delivered as evaluated params (statenos, damage/power, snap/mindist/
  maxdist, the hittime/velocity families) the *active* offsets are fixed but their
  correspondence to the parser's textual order is not, because the executor's
  param vector is not in textual order (p1stateno is early here, late in the
  parser). Where that matters (snap/mindist/maxdist) confidence is MEDIUM.
- **envshake**: per-slot naming, the `+0x1F0` pointer, and the parse site (see 11.8).
- **The tail records' byte layout.** `+0x1A0..+0x1DC` is the embedded PalFX (15
  dwords; Pass 7 has the PalFX layout), `+0x1DC..+0x1F4` the envshake record — but
  both are written by the trailing helper calls (`sub_47D050`/`sub_47C440`), which
  this pass did not decompile, so the tail's exact fill is inferred from the
  consumers, not read from the writer.
- **Unnamed-slot identities**: `+0x08`, `+0x64`, `+0x70`, `+0x80`, `+0xA8`, `+0xAC`,
  `+0x100`, `+0x104`, `+0x16C` — source known (template copy or param), MUGEN name
  not tied to any string. `+0x100`/`+0x104` are used as two damage-scale values in
  `hitresolve:321/359/373` and may be a scale pair, LOW. `+0x17C` is called
  guard.dist by schema position only (a param at `:445`, no consumer seen), MED.
- **p2bindtime's default.** The executor clamps `< -1 → 1` (`:293–298`), i.e. the
  fill default is **1**, not 0 — recorded here because the clamp is unusual (every
  neighbouring clamp goes to 0 or −1).

### 11.10 What this corrects

- **Pass 6 INDEX label** `0x47f380 palfx_executor_b` — wrong; corrected to the
  HitDef filler (see the INDEX fix and 11.1).
- **Pass 7 §4** — `0x47F380` is not a per-frame palfx executor; only `0x4782A0`
  is on the palette side.
- **Pass 9 §9.5.1 / §9.6** — the open question is closed: `sub_47F380` is the
  template→active filler, the two records are different layouts, and the six
  offsets Pass 9 named are all confirmed. Pass 9's "supersedes" framing of Pass 7
  is softened: Pass 7 quoted the *template*, Pass 9 the *active* block.
- **Roadmap S3 "Still open"** — chainid/nochainid (field layout, defaults, collision
  rule), getpower/givepower (active offsets), mindist/maxdist/snap (active offsets)
  and the down.*/airguard.* *active* offsets are now settled; what remains for each
  is its runtime consumer (chain test, meter arithmetic, placement ops), not the
  block layout.
- **Pass 9 §9.6 / Pass 10 §10.2, on `+0x10C8`** — the arrow this table used to draw
  for `+0x148` ("`p2getp1state` → `victim+0x10C8 = 1`") read a *writer* as a *name*.
  `+0x10C8` is the victim's fall-reaction flag with three writers, of which
  `p2getp1state` is one. See §11.11.

### 11.11 `victim+0x10C8` is the fall reaction, not `p2getp1state` — and the animtype trio's defaults

A re-read of the four artifacts that mention `victim+0x10C8` closes the question
Pass 9 §9.6 opened and corrects the arrow this table drew for `+0x148`.

**`+0x10C8` has three writers and two readers; only one writer is `p2getp1state`.**
In `angr_p9_hitapply.c` `sub_46DB20` (v11 = the attacker's active HitDef, v12 = the
victim):

- `:263` `v12->field_10c8 = 1;` when `hitflag & 1` — the "hit downed" bit of the
  hitflag word (this table, `+0x78 bit0`) — or the knockdown flag `v10` handed down
  from `sub_46D2D0`;
- `:267` `v12->field_10c8 = 0;` when the HitDef's `+0xB8` is set, so that parameter
  exists to *suppress* the fall;
- `:453` `if (v11->field_148) v12->field_10c8 = 1;` — `p2getp1state`.

It is read as the switch between the two airborne animtypes (`:454`, `if
(!v12->field_10c8) field_1078 = air.animtype; else field_1078 = fall.animtype;`),
read the same way on the ground branch (`:269`–`:278`), and read by both knockback
paths (`hitresolve:386`, `pair_writer:384`) to decide whether the victim takes the
hit's x knockback at all. It also forces a hair of downward velocity when it is set
and the hit has carried no vertical knockback (`:345`, `field_10b4 = -0.001f`),
which is exactly what a knockdown on a flat velocity needs.

**So `+0x148` is `p2getp1state` — name and offset stand, HIGH — but `+0x10C8` is
not.** It is "this victim is being knocked into a fall". `p2getp1state ⇒
victim+0x10C8 = 1` was too strong a reading, and it matters for the roadmap's
`p2getp1state` item, whose first recorded prerequisite was "resolve the semantics of
`+0x10C8`": resolved, and what `p2getp1state` does at hit time is *nothing beyond*
setting this flag in this routine. The state/animation handover Pass 9 named
(`+0x8C` → `sub_48BC00`, and the `+0x12F8` record) is a different mechanism and is
still open.

**The animtype trio and its defaults.** `sub_47DA40` writes `animtype` at template
`+376`, `air.animtype` at `+760`, `fall.animtype` at `+424`, `ground.type` at `+692`
and `air.type` at `+756`; `sub_47F380` copies them into the active block's `+0xB0`,
`+0x140`, `+0xC4`, `+0x124` and `+0x13C`, which is the map this table already had.
The defaults are explicit in the parser and are *chained* rather than constant:

- `air.animtype` defaults to `animtype`;
- `fall.animtype` defaults to `air.animtype` when that is **4 (Up) or 5 (DiagUp)**,
  and to **3 (Back)** otherwise;
- `air.type` defaults to `ground.type`.

The middle rule is a documentation defect worth writing down, because the engine now
follows the code: the parameter's own page says "Defaults to Up if air.animtype is
Up, or Back otherwise", while the parser carries DiagUp through as DiagUp.

**And the choice, at hit time** (`sub_46DB20`): a victim on the ground takes
`animtype`, or `fall.animtype` when `+0x10C8` is set; a victim in the air takes
`air.animtype`, or `fall.animtype` when `+0x10C8` is set. The ground branch's flag
comes from the downed bit or from the knockdown decision, which is why a *standing*
victim hit by a launcher already reads the fall animtype — they are airborne by the
time it is read, so a reader that consults only their pre-hit state gets it wrong.
`victim+0x1078` (animtype), `+0x107C` (`air.type`) and `+0x1080` (`ground.type`) are
what `GetHitVar(animtype)`/`(airtype)`/`(groundtype)` answer, which is why resolving
the three lands directly in those triggers.

**Open.** `air.type == 3` (Trip) forces the victim into state **5070** in the
airborne branch (`hitapply:423`). The engine reaches 5070 only from `ground.type`
being Trip, so an airborne victim told `air.type = Trip` is one state off. Five
corpus uses write `air.type = Trip`, so it is small — but it is real, and with the
offset and the consumer now in hand it is cheap to close. It is not cheap *alone*,
which is why it is not closed yet: 5070 is one of the numbers the engine's built-in
reaction table keys on, and §12.2 below shows the reference's own `common1.cns` choosing
the ground reaction from the *victim's stance* (`if (field_124 == 3)` being the one
override, to 5070 — the Trip case). The roadmap carries this with the rest of the
state-numbering finding rather than as its own one-line change.

## Pass 12 — the implicit launch flag `hitdef+0xB4`: its writer rule and its reader site (behaviour finding, 2026-10-18)

Pass 11 recorded `+0xB4` in the HitDef *active* block as an unnamed bool, "also set
when ground.velocity.y≠0" (MED). Chasing that down for the knockback work turns it
into the engine's **implicit launch flag**, with a writer rule that is unambiguous.

### 12.1 The writer — `sub_47F380` (`angr_0047f380_at_00480308.c`)

Two sites write it, and the second is the one that matters:

```c
a1->field_b0 = a0[94];                          // +0xB0 animtype
a1->field_b4 = -(-(0 < sub_439260(1)));         // :303  -- b4 = (param > 0), a plain bool
a1->field_b8 = -(-(0 < sub_439260(1)));         // :304
...
sub_439330();                                    // advance to the next float param
a1->field_130 = (float)v21;                      // ground.velocity x
sub_439330();
v29 = (float)v21;                                // ground.velocity y
*(float *)&a1->field_134 = v29;
if (v29 != 0.0f && !a0[101])                     // :385-386  <-- the rule
    a1->field_b4 = 1;
sub_439330();
*(float *)&a1->field_138 = v29;                  // the y copied again
```

So the rule is **`ground.velocity`'s y specified non-zero**, and the comparison in the
binary is against **0.0f**, not against a threshold and not against "greater than".
`a0[101]` is a template *token* index (the raw token array the filler reads), sitting
between `a0[94]` (animtype) and `a0[106]` (fall.animtype); `!a0[101]` therefore clears
the flag when some neighbouring switch is set, which is consistent with a character
that already wrote the explicit form. Which token `a0[101]` is has not been established
(LOW confidence), and nothing in the engine depends on that yet.

The `field_128`/`field_12c`/`field_130`/`field_134`/`field_138` run around it reads as
`ground.p2hittime`, `ground.p2slidetime`, then the `ground.velocity` x, y and the y a
second time — the duplicate at `+0x138` is MUGEN holding the launch velocity in two
places, one for the corner-push and one for the victim.

### 12.2 The reader — `hitapply` (`angr_p9_hitapply.c:287`)

```c
v19 = v12->field_e58;                            // the victim's statetype
if (v19 == 1 || v19 == 2)                        // standing or crouching
{
    if (v11->field_124 && !v11->field_bc)
    {
        v25 = v12->field_c4c;
        v12->field_c50 = v25;                    // prevstateno <- stateno
        v4 = 0;
        if (v11->field_b4 || v10)                // <-- b4, OR'd with a caller flag
            v12->field_e58 = 1;
        v12->field_c4c = (0 < v12->field_e58 - 1 ? 5010 : 5000);
        if (v11->field_124 == 3) v12->field_c4c = 5070;
        ...
        *(float *)&v12->field_10b0 = v11->field_130 * v8;   // the x, scaled
        *(float *)&v12->field_10b4 = v11->field_134;        // the y
        *(float *)v12->padding_11a4 = v11->field_138;       // and the duplicate
        if (v10) { if (v11->field_134 == 0.0f) field_10b0 *= 0.66; ... }
```

Two things are settled and one is not.

**Settled:** `field_b4` is read in the *ground* branch (victim standing or crouching),
it is OR'd with a second, caller-supplied flag (`v10`), and the pair decides the
victim's `field_e58` before the state number is picked out of it. It is therefore a
"this hit takes the victim off their feet" flag in exactly the sense the reference
describes for `ground.velocity`, and it gates the *state*, not just the velocity — the
same shape as this engine's `HitInfo::air` plus its launch branch. The whole block is
additionally gated on `field_124` (which is `a0[173]`, a template token — a
character-set switch, not a MUGEN public name) and on `!field_bc`.

**Settled:** the velocity a launch carries is `ground.velocity`'s own pair
(`field_130`/`field_134`), with the x scaled by `v8`. This is worth recording because
it is *not* what this engine does: `hitInfoFor`'s grounded-launch branch hands the
victim the **`air.velocity`** pair. Which of the two is right for a *standing* victim
hit by a `ground.velocity` launch is therefore an open question against the binary,
and the `if (v10)` block that scales the x by 0.66 when the y is zero suggests the two
pairs are not meant to be the same value.

**Settled, but from the reference rather than from the binary: the enum behind
`field_e58` runs `1 = stand, 2 = crouch`, and only that reading makes the line sensible.**
`v19 == 1 || v19 == 2` is then "standing or crouching" — the natural branch — and
`0 < field_e58 - 1 ? 5010 : 5000` is "crouching ? 5010 : 5000". The enum's *numbers* are
not read off an enum table in the binary (that is the MEDIUM part); what settles which is
which is `local_mugen/extracted/mugen/data/common1.cns`, whose own state headers name the
two numbers and whose contents say what each is for:

- `[Statedef 5000]` is `Stand get-hit (shaking)`, `type = S`, and `[Statedef 5010]` is
  `Crouch get-hit (shaking)`, `type = C`. A crouching state is not somewhere a standing
  victim can be put without popping them upright, so `5010 : 5000` **is** the victim's
  stance and not the hit's.
- `[Statedef 5000]`'s first controller reads the hit type for its **animation** and not
  for its state: `type = ChangeAnim / trigger1 = Time = 0 / trigger1 = GetHitVar(animtype)
  != [3,5] / value = ifelse((GetHitVar(groundtype) = 1),5000,5010) + GetHitVar(animtype)`.
  So `ground.type = Low` on a standing victim means "draw the crouch-hit animation", which
  is the whole reason the hit *type* exists as a separate choice — and it is why reading a
  state number off `ground.type` (as this engine does for Low) cannot be right.
- Then `if (field_b4) field_e58 = 1` collapsing a launchable hit onto the *standing* number
  is not a defect in the reading either, because **5000 is the state that handles a
  launch**: `[State 5000, 5] StateTypeSet / trigger1 = GetHitVar(yvel) != 0 ||
  GetHitVar(fall) / statetype = a`, followed by `[State 5000, 7] ChangeState / trigger1 =
  HitShakeOver / value = 5030` and `[State 5000, 6] … value = 5001` for the flat knockback
  (`GetHitVar(yvel) = 0 && !GetHitVar(fall)`). So the number the engine picks at hit time
  is always a *shaking* state; 5001, 5011, 5030 and the rest of the knocked-away family are
  reached from inside the character's own 5000/5010, not chosen here.
- `if (field_124 == 3) field_c4c = 5070` is the one override, and it is consistent with the
  hit type being read for a *state* only in the Trip case: `field_124` is `ground.type` by
  pass 11's template→active map (`+692 → +0x124`, §11.2 above), and 3 is Trip.
  `[Statedef 5070]` is `Tripped get-hit (shaking)`.

Which leaves the branch's numbers exactly as they are written in the decompilation above,
with one consumer question now open in their place: this engine applies `ground.type`
itself for *both* Low and Trip, and sends a launched victim straight to **5030** instead of
5000. Both are recorded in the roadmap's state-numbering finding.

**Who owns which transition** — the same file settles it, and it is the reason the numbers
have to be right rather than merely consistent. Nothing in `common1.cns` anywhere
`ChangeState`s *into* 5000, 5010, 5020, 5070 or 5080 — those five are entered by the engine
when a hit lands — and everything after that is the character's own file handing over:

| entered by the engine | the file hands over to | when |
|---|---|---|
| 5000 (S, shaking) | 5001 / 5030 | `HitShakeOver`, and `GetHitVar(yvel) = 0 && !GetHitVar(fall)` splits flat knockback (5001) from a launch (5030) |
| 5010 (C, shaking) | 5011 / 5030 | same split |
| 5020 (A, shaking) | 5030 | `HitShakeOver` |
| 5070 (A, shaking) | 5071 | `HitShakeOver`; 5071 then falls into 5110 |
| 5080 (L, shaking) | 5081 / 5030 | `HitShakeOver`, split on `GetHitVar(yvel)` |

and the rest of the chain is 5030 → 5035 (on `AnimTime = 0`) → 5040/5050 (split on
`HitFall`), 5050 → 5100 (landing) or 5200/5210 (`CanRecover` + the `recovery` command),
5100 → 5101/5110, 5101 → 5110, 5110 → 5150 (only `!alive`), 5120 → 0 with `ctrl = 1`,
5210 → 52. Two of those the file says out loud are the engine's and not its own:
5110 → 5120 carries the comment "player changes to get-up state (5120) when ready" and the
guard-end controller is commented out with `;Implemented within engine`. So the engine's job
at hit time is the *entry* number, the hit data the file reads back (`GetHitVar`, and the
`HitShakeOver`/`HitOver`/`HitFall`/`CanRecover` triggers), and the velocity — not the
sequence. Which is why **27 of the 41 packs in `local_chars/` ship their own `common1.cns`**
(the other 14 are this project's generated characters and Kung Fu Man): getting the entry
number right hands the whole reaction back to the character, which is the point.

### 12.3 What this means for this engine (recorded, not yet acted on)

`statemachine.cpp` currently decides a ground launch with `hit.pushY > 0.5f` — a
threshold nobody chose, on a value already converted to the engine's up-positive
frame. Both the reference (`cns.html`: "ground.velocity: ... If y_velocity is not
zero, P2 will be knocked into the air") and the binary (`v29 != 0.0f`) say the test is
**non-zero**. The two differ on the corpus's *downward* ground velocities
(`ground.velocity = -1,15` and `-10,10`): under `> 0.5f` a downward-y hit leaves the
victim standing with a velocity into the floor, and under `!= 0` it is a knockdown.

The change is small — one parse flag, one consumer — but it moves the single most
common path in the engine, and §12.2 leaves two things open that the branch would need:
which velocity pair a ground launch carries, and what the implicit flag does when the
character also wrote `fall = 1`. It is therefore recorded here with the writer rule
pinned and deliberately left for its own slice and its own test. See the roadmap's S3
note and the "defect this inventory found" subsection.

**Since acted on (2026-10-18).** §12.2's pair question was settled by the reference
rather than by the binary's gated block: `guard.velocity` "defaults to the x_velocity
value of the ground.velocity parameter" while `airguard.velocity` defaults from the air
pair, so MUGEN's two families are mirrors by construction — a ground victim takes
`ground.velocity`, an airborne one `air.velocity`, even when the hit puts them in the
air. `statemachine.cpp`'s grounded-launch branch now reads `hit.pushY != 0.0f ||
hit.fall` and hands over `hit.pushX`/`hit.pushY` instead of the air pair. A corpus
survey (2517 `ground.velocity` lines) sized the change: 1304 single-value lines have no
y and are untouched, 709 negative-y launches were already launches, and **96** write a
positive — *downward* — y that the old `> 0.5f` test left standing. Still not
implemented, and flagged rather than guessed: the `if (v10) { if (field_134 == 0.0f)
field_10b0 *= 0.66; }` damping for a `fall` on a flat ground velocity, and the
`field_124` gate on the whole branch. See the roadmap's "defect this inventory found".

## Pass 13 — the liedown timer: `[Data] liedown.time`, the 5110 handler, and who owns the get-up (behaviour finding, 2026-10-18)

Pass 12 walked the hit-reaction chain and had to stop at two transitions the character's own
file says are not the character's: 5110 → 5120 ("player changes to get-up state (5120) when
ready") and the guard-end controller ("`;Implemented within engine`"). This pass closes the
first one: where the countdown lives, what arms it, when it ticks, what it fires, and the one
gate that suppresses it. Five answers, each with the address it came from.

The load-bearing prerequisite is a single constant: **the engine's movetype encoding is
I = 0, A = 1, H = 2**, read straight out of the state-header parser's switch
(`0x475133`–`0x475192`, with the error string `State movetype "%s" not valid` @ `0x4A3274`).
Every guard in this pass is a `cmpl $2` meaning "in a hit reaction", so this mapping is what
makes the reset block below readable at all — see §13.4.

### 13.1 The state dispatch inside the per-frame player update (`0x46B440`)

The player's per-tick update is one function (`0x46B440`, `ebp` = player). After the shared
work it dispatches on stateno with a two-level table: a range test
(`subl $0x13EC,%eax; cmpl $0x6E,%eax` @ `0x46B9B3`) selects 5100–5210, a byte selector table
@ `0x46C4A0` feeds a target table @ `0x46C48C`. The table's whole content:

| stateno | handled at | what it does |
|---|---|---|
| 5100 | `0x46B9CF` | halve the countdown if `+0x108C > 0`; `+0x1E4 *= fall.defence_up`; invincibility when ≤ 10 |
| 5110 | `0x46BA21` | **tick the countdown, and change to 5120 at 0** |
| 5120 | `0x46BA6D` | restore `+0x1E4 = 100/defence` at time-in-state 0 |
| 5200, 5210 | `0x46BA99` | `+0x1E4` scaling only |
| everything else in range | `0x46B8B3` | default |

So **5080, 5081, 5101, 5140, 5150, 5160 and 5170 have no engine handler at all** — they run
purely as CNS states. HIGH (table bytes read). This is also the first structural answer to the
question "which states does the engine get a fighter up from": the set of states the engine
knows about by number at get-up time is exactly {5100, 5110, 5120, 5200, 5210}.

### 13.2 The engine's own writers of 5120 — three, and only two states feed them

Grepping every immediate `0x1400` (5120) in the binary leaves exactly one that stores into a
player's stateno; the rest are table entries or locals (`0x469AB9`, `0x44B730`). The engine
puts a player into 5120 from:

| # | site | source state | condition | effect |
|---|---|---|---|---|
| 1 | `0x46BA4F` | **5110** | countdown ≤ 0 **and** gate `+0xE80 != 0` (`0x46BA2D`) **and** time-in-state `+0xE54 > 0` (`0x46BA35`) | re-arm `+0x10A0 = player+0x78`, then `ChangeState(player, 5120)` (`0x46AF10`) |
| 2 | `0x46C12D` | **5150** (the defeated lie) | `engine+0x4A00 == 3` (round over) **and** stateno == 5150 **and** `+0xE54 >= 0x5A` (90 ticks) | stateno := 5120 directly; time-in-state := −1; **life := lifemax**; gate := 1; `+0x106C`/`+0x1068` := 0 |
| 3 | `0x44B735` | **5150** (round (re)start sweep) | for every active player (`engine+0x11230`, count `engine+0x11434`) still in 5150 | `ChangeState(player, 5120)`; life := lifemax; gate := 1; `+0x1F0` := 1; `+0x106C`/`+0x1068` := 0 |

Answers to the question as asked: **5110 yes; 5150 yes (twice); 5080 no; 5140/5160/5170 are
not states in this build.** The reference's own data agrees — `common1.cns` has `[Statedef
5080]`, `[Statedef 5081]`, `[Statedef 5100]`, `[Statedef 5101]`, `[Statedef 5110]`,
`[Statedef 5120]`, `[Statedef 5150]`, and **no `[Statedef 5140]`, `[Statedef 5160]` or
`[Statedef 5170]` at all** — 5140 and 5170 appear there only as *animation* numbers (5140 is
the K.O. lie animation used by 5150, and 5170 the downed anim in the air-recovery bank).
HIGH for the sites; HIGH for the file, from reading it.

Note the asymmetry that matters downstream: path 1 is an ordinary `ChangeState`, so
prevstateno becomes 5110 and the character's file resumes control in 5120 — but paths 2 and 3
write stateno *directly*, bypassing `ChangeState`, and set time-in-state to a value they
choose (−1 and whatever the sweep leaves). A character file cannot express either path: stock
5150 declares `ctrl = 0` and contains no `ChangeState` that leaves. So a K.O.'d fighter
standing up is 100% engine, 0% character.

### 13.3 The countdown is a player field seeded from `[Data] liedown.time` — not from a HitDef parameter

**The field.** `GetHitVar(recovertime)` reads player `+0x10A0`: `"recovertime"` is in the
GetHitVar name cluster @ `0x4A5108` (`0x4A50A8`–`0x4A51D4`, alongside xveladd, yveladd,
fallcount, hitshaketime, hittime, slidetime, ctrltime, isbound, fall.*), and the handler at
`0x487889` loads `+0x10A0`. MED-HIGH (the name table and the load are seen; the handler was
matched by its field, not by a decompiled name).

**Every writer of `+0x10A0` in the binary is four sites, and three of them read
`player+0x78` first:**

| site | writes | source |
|---|---|---|
| `0x46AAB9` | round reset (`0x46A6B0`) | `player+0x78` |
| `0x46B7E0` | the not-in-hit-reaction refresh (every frame) | `player+0x78` |
| `0x46BA42` | re-arm immediately before the 5120 change | `player+0x78` |
| `0x46BA62` | the decrement (`decl`, inside 5110) | itself |

**`player+0x78` is `[Data] liedown.time`.** The chain, all seen:

1. The `[Data]` block parser `0x4675B0` (single caller `0x46666A`) reads the key with the
   int reader `0x433730` and stores it at **charstruct+0x2E8** (`"liedown.time"` @ `0x4A2680`,
   store at `0x4676B9`–`0x4676CC`). Sibling keys in the same parser confirm the block:
   life → `+0x2C0`, power → `+0x2C4`, attack → `+0x2C8`, airjuggle → `+0x2D4`,
   defence → `+0x2E0`, `fall.defence_up` → `+0x2E4`, `liedown.time` → `+0x2E8`. HIGH.
2. At character load, `0x468F41` block-copies the constants region to the live player:
   `leal 0x2C0(%ebp),%esi; leal 0x50(%ebx),%edi; movl $0x58,%ecx; rep movsl` — a 0x160-byte
   move from charstruct+0x2C0 to player+0x50. charstruct+0x2E8 therefore lands on
   `player+0x78`. HIGH (the arithmetic is inside the instruction).

`player+0x78`'s neighbours are the constants the docs name, which is the cross-check:
`+0x50` life, `+0x54` power, `+0x58` attack, `+0x64` airjuggle, `+0x70` defence,
`+0x74` fall.defence_up, `+0x78` liedown.time — and `+0x70` is the divisor used to build the
damage scale (`+0x1E4 = 100.0/defence`), which is exactly what `[Data] defence` should do.

**`fall.recovertime` is a different field with a different consumer.** The HitDef parameter
string `"fall.recovertime"` @ `0x4A42D4` (negative-value warning "specified negative
recovertime" @ `0x4A46C0`; the parser clamps `<0 → 4`, Pass 11 §11.2) is copied by `hitapply`
into the **victim**, not into the player's own countdown: active hitdef `+0xD0` → victim
`+0x10D8`, active hitdef `+0xD4` → victim `+0x10E0`, with victim `+0x10DC` zeroed
(`0x46E3BF`/`0x46E3C5`/`0x46E3D5`) and then incremented once per frame at `0x46C1CC`. That
triple is the fall-*recovery* window consumed by the CanRecover boolean at
`0x4875F6`–`0x487615` (`+0x10D8 != 0 && +0x10DC >= +0x10E0`; Pass 9's case 105). So the
reference has **two independent timers**, and the one the user-facing docs describe under
`GetHitVar(recovertime)` is the liedown one, not the HitDef one.
**There is no `ground.recovertime` string in this build** (MED — from the full resolved
string table, not a hand check of every HitDef key).

**Default when `[Data] liedown.time` is omitted: there is none in this path, and a missing key
is fatal to the character load.** The int reader `0x433730` (find-line `0x433410` +
`atoi` via `*0x49B1BC`) returns 0 *only* when the key is absent, and the `[Data]` parser treats
a 0 return as failure: `0x4676D1` → `0x4682A2` prints `Can't load Data constants`
(`0x4A269C`) and returns failure; the caller inside the character loader
(`0x466672`–`0x4666DA`) then prints `\nError in %s` (`0x49C848` + the def filename) and returns
0. So the doc's "default 60" is **not implemented** in this build: 60 is simply what the
shipped `mugen.cfg` common data writes. MED-HIGH (reader, failure branch and both call sites
seen; I have not audited every other loader in the exe). Two consequences worth recording:
`liedown.time = 0` is legal and gets the fighter up on the *second* tick in 5110 (the handler
requires `+0xE54 > 0`), and an engine that defaults the constant silently diverges from a
reference that refuses to load the character.

### 13.4 When the countdown runs: armed every non-hit frame, frozen by the hit, ticking only in 5110

This is the part the docs' one-line description ("counts down to 0 for each game tick") gets
wrong, because the guard is easy to misread. In the per-frame update, `0x46B6CF` tests
`cmpl $2, +0xE5C` — movetype **== H** — and the *otherwise* branch (`0x46B7B2`–`0x46B806`) is a
reset block that **re-arms the countdown to `player+0x78` at `0x46B7E0`** and additionally
clears `+0x1084`, `+0x10BC`, `+0x1070`, `+0x10A4`, `+0x10C8` (the fall reaction, §11.11),
`+0x108C`, `+0x1090`, `+0x10EC`, sets `+0x10E4 = 1` and `+0x1E4 = 100.0/defence`. The
in-a-hit-reaction branch (`0x46B6DC`) integrates air acceleration from `+0x10A8`/`+0x10AC` into
`+0x1F4`/`+0x1F8`, decrements `+0x1090`, and applies accumulated damage from `+0x1084` — and
**does not touch `+0x10A0` at all**.

Consequence: outside a hit reaction the liedown timer is continuously re-armed and therefore
always reads `liedown.time`; the instant a hit puts the fighter in movetype H the re-arming
stops and the value freezes; and the only decrement in the entire binary is the `decl` inside
the 5110 handler. So the honest answer to "when does the countdown start" is: **it is armed
before the hit and starts ticking on the first tick inside state 5110** — not at hit time, and
not in 5080/5081/5100/5101, which never decrement it. HIGH for the two code paths; the
"hitapply seeds nothing" half is a negative result over the four known writers, so MED-HIGH.

Three further engine edits to the same counter are worth recording because they are invisible
in any character file:

- **Entering 5100 while already knocked down halves it.** `+0x108C > 0` at
  `0x46B9D1`–`0x46B9F9` → `sarl +0x10A0` (arithmetic halve), then `+0x1E4 *= fall.defence_up`
  (`player+0x74`) and `+0x108C++`. Since `+0x108C` is zeroed by the not-in-hit refresh, this is
  "knocked down again while down".
- **Being hit while already falling scales it.** In the same region that handles a hit landing
  on a victim already in movetype H (`0x465C86`–`0x465CE7`) the countdown is multiplied:
  `+0x10A0 = (int)(+0x10A0 × 0.75)` (`0x4A6D80`) when the victim's statetype is L, else
  `(int)(+0x10A0 × 0.9)` (`0x4A6D78`) when `+0x108C > 0` (float path via `ftol` `0x499690`).
- **≤ 10 in 5100 means invincibility.** `0x46B9FF`–`0x46BA1C`: when `+0x10A0 <= 10`,
  `+0x1058 := 0` and `+0x1060 := -1`. This is precisely the stock file's comment "player
  becomes invincible if within 10 ticks of getting up". MED (the pair are the not-hit-by
  attribute mask and its timer — the round reset writes `0xFFF` into `+0x1058`/`+0x105C`).

**No button-mash term exists.** The only decrement of `+0x10A0` in the binary is the 5110
`decl` (`0x46BA61`–`0x46BA62`); there is no input read on that path. The doc sentence "will
count down faster if buttons are hit" is **not** implemented here — the only "faster" the
engine has is the two multiplicative rules above. HIGH as a negative result.

### 13.5 At zero: an ordinary `ChangeState`, and the engine does not set `ctrl`

The 5110 handler's firing sequence is: re-arm `+0x10A0 = player+0x78` (`0x46BA3D`–`0x46BA42`),
then `movl $0x1400,%eax; movl %ebp,%edi; call 0x46AF10` (`0x46BA4F`). `0x46AF10` is Pass 9's
ChangeState and it writes exactly three fields — prevstateno (`+0xC50`) = stateno, stateno
(`+0xC4C`) = 5120, time-in-state (`+0xE54`) = 0 — plus the owner-record debug path. **It does
not write ctrl (`+0xE64`).** Because the countdown is re-armed *before* the change,
`GetHitVar(recovertime)` reads `liedown.time` again during the get-up animation; that is what
makes the reference value meaningful to a character file (Pass 9's case 105 is the *other*
timer, see §13.3).

`ctrl` is written only by the per-state header application (`0x48B8C0`–`0x48B907`): the state
record's ctrl expression is evaluated, clamped to 0/1, and **an absent parameter (−1) means
"keep the current value"** (`0x48B8FC`–`0x48B907`). Stock 5120's header declares statetype,
movetype and physics only — no `ctrl` — so the fighter carries in whatever it had, and the hit
states set `ctrl = 0` (KO path `0x46BDEE`; hit-apply path clears it too). Control is handed
back not by the engine but by the file: `[State 7]` of stock 5120 is
`ChangeState value = 0 ctrl = 1` at `AnimTime = 0`, and `ctrl` is a parameter of the
ChangeState controller (`"ctrl"` @ `0x4A2FFC` and `"value"` @ `0x4A2FF0` adjacent in that
controller's parameter table, `0x4740A0`/`0x474270`). So the answer to "does the new state get
ctrl?" is **no, not from the engine** — and a state whose header omits the parameter inherits
the previous value rather than resetting to 0, which is the trap for anyone re-implementing
this. HIGH for ChangeState and the header applier; MED for the exact write site of the
`ctrl =` controller parameter (the parameter exists and is parsed; I did not isolate its store).

### 13.6 What suppresses the get-up: exactly one engine gate, and it is about being K.O.'d

`+0xE80` is the gate, and the handler consults it at `0x46BA2D` before the state change. Its
**complete writer set in the binary is four sites** — which is the strongest statement in this
pass:

| site | value | context |
|---|---|---|
| `0x46BDE2` | **0** | the KO block: also sets global `0x4A9020 = 1`, `ctrl = 0`, `+0xE84 = 0`, and `ChangeState(player, 5030)` when movetype != H |
| `0x46C147` | 1 | the K.O. get-up: 5150 → 5120 after 90 ticks, with life := lifemax |
| `0x44B759` | 1 | the round (re)start sweep out of 5150 |
| `0x46A9CF` | 1 | the round reset (`0x46A6B0`, which also sets stateno 0, statetype 1, movetype 0, physics 1, ctrl 1, `+0x1058`/`+0x105C` 0xFFF, and re-arms `+0x10A0 = +0x78`) |

So the gate means **"this player is not the defeated one"**, and nothing else. While it is 0,
the 5110 handler's countdown parks at 0 without a state change — the fighter lies there — and
the only ways back up are the K.O. get-up and the round sweep. That is exactly how a dead
fighter is prevented from standing up, and it is the *only* such mechanism.

The other candidates in the question are negative results:

- **`HitFallSet` / the fall-reaction flag `+0x10C8`: not consulted.** The handler reads only
  `+0x10A0`, `+0xE80` and `+0xE54`; `+0x10C8` is set by `hitapply` (§11.11) and by the KO block,
  and *cleared* by the not-in-hit refresh — it never appears on the get-up path. HIGH
  (the handler's full instruction list).
- **Throws / binds: no check found.** A bound victim is running the thrower's
  `p2stateno` and so is not in 5110 to begin with; there is no throw-specific test in the
  get-up path. Flagged LOW-MED: I did not resolve the `isbound` GetHitVar field
  (`"isbound"` @ `0x4A5144`) to confirm nothing else writes the gate.
- **KO sound marks:** while the gate is 0, `+0xE84` counts frames and the engine plays a
  sound at 20 and 50 (`0x46BEAB`/`0x46BEF3` → `0x473C40`, `ecx = 0xB`). MED; tangential, but it
  confirms the gate-off counter is a K.O. sequence timer rather than a second get-up timer.

### 13.7 What this means for this engine (recorded, not yet acted on)

Three concrete divergences, all in `app/src/main/cpp/engine/`, none of them actioned here.

1. **`kBuiltins` hardcodes a chain the reference keeps in the character's file, and its
   numbers for the defeated lie are wrong.** `statemachine.cpp` (~lines 275-291) builds
   5110 → 5120 and 5150 → 5160 → (−1) as built-in transitions, with `kDeadLiedown = 5160`
   (`statemachine.h:88`) and `kHitLiedown = 5080` (`:81`). The reference has no statedef 5160
   at all — the defeated lie is **5150**, it declares `ctrl = 0`, it does not return, and the
   engine stands that fighter up by writing 5120 directly after 90 ticks of the K.O.
   slow-motion. Under this engine's numbering a portable `common1.cns` that says
   `!alive → 5150` lands on a state the engine treats as its own terminal dead lie, so a
   dead fighter never gets up. Two separate things to decide: the numbering, and who owns
   the stand-up.
2. **The liedown duration is a constant the engine already parses and never reads.**
   `character.h:209` registers `{"data.liedown.time", "60", false}`. The reference's get-up
   timing is exactly that value, counted down while in 5110 (with the halve-on-re-knockdown
   and the ×0.75/×0.9 on being hit while falling), then a `ChangeState` to 5120.
   `HitDef::fallRecoverTime` is the *other* field — `statemachine.cpp:2773` defaults it from
   `fall.recovertime` and `:4160` consumes it only in `canrecover` — and must not drive the
   get-up. Two independent timers is the reference's design; this engine currently has one.
3. **The gate is a real state, not an edge case.** "K.O.'d → can't get up" and "defeated
   fighter stands up 90 ticks into the K.O." are both engine behaviours that no character
   file can express, because stock 5150 has no path out. If the engine's K.O. handling sets
   a flag, it should be cleared by the same two events the reference uses (the K.O. stand-up
   and the round reset) and by nothing else — that is the whole rule.

**Open, flagged, not guessed:** (a) whether `+0xE80` has writers beyond the four above
(the list is complete for this binary's disassembly, but the `isbound`/bind fields were not
cross-checked); (b) the store site of the ChangeState controller's `ctrl =` parameter;
(c) the name of the trigger at `0x4875F6` (called `canrecover` throughout this file — it is the
*fall-recovery* test on `+0x10D8`/`+0x10DC`/`+0x10E0`, **not** the liedown timer).

## Pass 14 — the fall landing, and who spends it (behaviour finding, 2026-10-18)

A fall's landing owes three effects: the **fall damage** (`fall.damage`), the **bounce
velocity** (`fall.xvelocity`/`fall.yvelocity`) and the **screen shake** (`fall.envshake.*`).
In MUGEN the engine applies none of the three. They are *script* — spent by the character's
own common state file from its ground and airborne landing states 5100/5101 and 5110, through
the `HitFallDamage`, `HitFallVel` and `FallEnvShake` controllers. **27 of the 41 packs in
`local_chars/` ship a `common1.cns` that does exactly this.** The engine's own `land()`
(`statemachine.cpp:3116`) is the stand-in for the file that ships none — the remaining packs —
and it is meant to be *only* the stand-in.

### 14.1 The contract that lets the file and the engine coexist: each effect is *spent*

What lets a character's own landing and the engine's built-in one share the floor without
fighting is that each effect is **consumed**, not merely read — and the reference's own common
file shows the duplication is real and intended. `HitFallDamage` is called from **two** states
(5100 at time 3 and 5110 at time 0) and `FallEnvShake` from the same two, and `FallEnvShake`'s
own documentation says it "sets `GetHitVar(fall.envshake.time)` to zero after being executed".
The reference is therefore relying on the second call finding nothing left to spend: a file
that scripts its landing gets each effect exactly once however many states ask for it, and a
file that scripts none still gets the engine's.

The engine mirrors that with a pending/spend split. `doHitFallDamage`, `doHitFallVel` and
`doFallEnvShake` (`statemachine.cpp:2244`–`2279`) are the **spending** controllers — the ones
a character's file calls — and `applyFallDamage`/`applyFallShake` (`:2251`/`:2267`) are the
actual application, factored out because `land()` applies the same thing as the built-in
default. The pending amounts live on the fighter (`fallDamagePending_`, `fallVelX_`/
`fallVelY_`, `fallShakeTime_`; `statemachine.h:1176`–`1193`) and are zeroed as they are spent,
so a second call is a no-op. `fallShakeTime_` doubles as the live answer to
`GetHitVar(fall.envshake.time)`, which is the key the contract is written in — it counts down
as the controller spends it rather than snapshotting the hit's value.

Who owns the landing is decided **per effect**, and the decision reads the file itself.
`scanLanding` (`statemachine.cpp:2294`) runs once per character and is cached against the
library pointer, so a shared library is not re-scanned every round; it asks
`StateLibrary::usesController` (`cns.cpp:353`, declared at `cns.h:243`) — a case-insensitive
scan of every state's controller list for MUGEN's own spelling (`hitfalldamage`, `hitfallvel`,
`fallenvshake`) — and sets `scriptedFallDamage_`/`scriptedFallVel_`/`scriptedFallShake_`.
`land()` then stands down for *exactly* the effects the file declares, not for all three: a
pack that writes `HitFallDamage` and nothing else keeps the engine's bounce and its own damage
timing.

Corpus weight, by the same census (a *pack* is a top-level folder under `local_chars/`; a
*use* is a `type = <Controller>` line in a `.cns`, `.cmd` or `.st` file, matched
case-insensitively): `HitFallDamage` **88 uses across 35 packs**, `FallEnvShake` **88 across
35**, `HitFallVel` **41 across 35**. That `HitFallDamage` and `FallEnvShake` come out *equal*
is not a coincidence and is worth recording in its own right: the files write the two as a
pair — one shake per damage charge — so per pack the shake count matches the damage count,
and the bounce (`HitFallVel`) count is half of them.

### 14.2 The failure mode is a *doubled* landing, not a missing one

This is what makes the per-effect gate load-bearing rather than cosmetic. An engine landing
applied on top of a file's own does not merely charge the fall twice. `land()` bounces at the
moment of contact and *drains* the fall velocity, and a file's own 5100 reads
`GetHitVar(fall.yvel)` to choose between sliding and bouncing — so the drained value reads 0,
the file takes the slide branch, and the whole bounce state, its animation and its lift are
skipped. The doubled damage is the visible half; the skipped bounce is the half that changes
what the match looks like.

### 14.3 The sign convention, and why it never moves a branch

`fall.yvelocity` defaults to **-4.5**, upward in the character's own frame. The engine stores
the landing's bounce velocity **up-positive**: `hitInfoFor` copies `info.fallVelY = hit.fallVelY`
(`statemachine.cpp:3474`) from a value read as `-firstParam("fall.yvelocity", ..., -4.5f)`
(`:2928`), and `GetHitVar(fall.yvel)` negates it back on the way out (`:4425`) so the character
reads it in its own frame.

What makes the sign safe is how the corpus uses the key: `GetHitVar(fall.yvel)` is read **only**
as `= 0` or `!= 0` — **133 uses across 35 packs, every one of them a zero test** — to choose
the slide or the bounce branch in the 5100/5110 ground states. So the sign never selects a
branch; it only has to agree with what the number means, which the negation guarantees.

### 14.4 What is still missing: the built-in landing has no duration

The engine's substitute landing has a real limitation, and it is a *timing* one rather than a
missing effect. `kBuiltins` (`statemachine.cpp:285`–`324`) is a table of rows carrying a state
number, statetype, movetype, physics, `ctrl`, an animation and a *next* state — and **no
duration column**. A built-in state hands over when its animation runs out (`runBuiltinState`,
`statemachine.cpp:528`, testing `animationFinished()`), and for a character with no
`common1.cns` — hence no get-hit animations — "the animation has run out" is true the moment
the state is entered. Such a character therefore walks the landing chain (5040 → 5050 → 5110 →
5120) one state per tick via `runCurrentState`'s guard loop (`:1134`): it lands, bounces once,
and is standing again on the tick it stops, where a real character lies there for the length
of its liedown animation. Giving the table real durations means taking them from the common
state file the reference ships; that is separate work and is not guessed at here.

## Pass 15 — `GetHitVar(hitcount)` is the victim's counter (behaviour finding, 2026-10-18)

`GetHitVar(hitcount)` is the **victim's** count of hits taken in the combo it is currently in.
It is *not* the standalone `hitcount` trigger, which is the attacker's own count of how many
times the move it is running has connected (`statemachine.cpp:4319`) — the two share a name and
count different things from opposite sides of the fight. The reference's documentation is
explicit about which one this is: "Returns the number of hits taken by the player in current
combo". The corpus splits along exactly that line.

### 15.1 The corpus needs the victim's number, and a flat 0 is the dangerous wrong answer

`GetHitVar(hitcount)` is read **62 times across 7 packs**, in two families, and both want the
number as seen *by the victim*:

- an attacker scaling its damage by the punishment it has already dealt
  (`Enemy,GetHitVar(HitCount)` — Kira, Giorno and Vegeta), and
- a victim gating its own AI while it is being hit (Vegetto, Diavolo, Jolyne, Mista).

The engine answered a flat 0 before this was implemented, which is the dangerous kind of wrong
answer rather than a merely absent one: a damage scaling that multiplies by the count
multiplies by zero, and a combo counter that reads the victim's count never leaves its first
hit.

### 15.2 The reference's three rules, read off `hitapply`

The decompiler artifacts put the field at **victim+0x1088** and give it three rules, all in
`hitapply` (`docs/mugen-re/decompiled/angr_p9_hitapply.c:570`–`579`; the struct declares
`field_e5c` at `:120`, `field_1088` at `:134`):

| rule | the artifact | meaning |
|---|---|---|
| increment | `field_1088 = field_1088 + 1` | once per hit, on the path a hit actually travels |
| reset | `if (field_e5c != 2 ...) field_1088 = 0` | zeroed only when the victim was **not already** in a hit reaction (movetype H is 2) |
| then set | `field_e5c = 2` | the hit puts the victim into H, so the *next* hit does not reset |

Guarded hits never reach this path at all — the block branch returns earlier — which is the
other half of the same rule: blocking a hit does not advance the combo the attacker is
building. And because the field is zeroed only when the victim was not *already* in movetype H,
a combo that keeps the victim in the hit reaction counts up across it, while a fresh hit that
lands after a recovery starts the count again at one.

### 15.3 Where the engine keeps it

The engine keeps this in its own field, deliberately distinct from the attacker-side
`hitCount_`/`uniqHitCount_`, because the two are different numbers with the same name:
`hitsTakenInCombo_` (`statemachine.h:1134`, whose comment at `:1128` spells out the split).
It is answered at `statemachine.cpp:4441` under `GetHitVar`'s `hitcount`. Two sites touch it:

- the round reset (`statemachine.cpp:672`), beside `lastHitById_ = -1;` — a new round erases
  the combo exactly as it erases what last hit the fighter; and
- `applyHit`'s unguarded path (`statemachine.cpp:3769`–`3786`), right where
  `lastHitById_ = resolved.id;` is written, as `if (moveType_ != MoveType::Hit)
  hitsTakenInCombo_ = 0; ++hitsTakenInCombo_;`.

The ordering there is load-bearing and the code says so: the reset test reads `moveType_`
**before** `moveType_ = MoveType::Hit;` runs later in the same function, because that
assignment is what the "was I already in a hit reaction" test would otherwise be reading. The
reset-and-increment being the same two lines is what makes the first hit of a combo read 1
rather than 0, which is the value the `Enemy,GetHitVar(HitCount)` scalers index off.

### 15.4 Open: a second guard at victim+0x11A8

The decompiled condition is `field_e5c != 2 && field_11a8 > 1`, not the movetype test alone,
and the artifact's field table declares `int field_11a8` (`angr_p9_hitapply.c:165`) with **no
writer anywhere in the decompiled set** — a scan of every `.c` under
`docs/mugen-re/decompiled/` finds the name only on this one read. What it counts, and what it
gates, is unestablished. The engine implements the movetype half of the guard and leaves this
one; recorded here rather than guessed at.

## Pass 16 — the enumerated HitDef names are matched by one character (behaviour finding, 2026-10-18)

### 16.1 The matching rule, and why a prefix is not it

All six enumerated parameters of a HitDef — `animtype`, `air.animtype`, `fall.animtype`,
`ground.type`, `air.type`, and by the same code path the `priority` class beside its number —
are matched on the **lowercased first character of the value** and on nothing else.

```
47ee4a: movsbl (%eax), %eax        ; first byte of the value
47ee4d: calll  0x40e380            ; sub_40E380 == tolower (IAT 0x49b198 = MSVCR90 tolower)
47ee52: addl   $-0x62, %eax        ; subtract 'b'
47ee55: cmpl   $0x13, %eax         ; outside [b..u] -> error
47ee5e: movzbl 0x47f2c4(%eax),%edx
47ee65: jmpl   *0x47f2a8(,%edx,4)   ; 20-entry jump table
```

Resolving `0x47f2a8` against the index bytes at `0x47f2c4` gives the only six populated slots:
`b`->Back (3), `d`->DiagUp (5), `h`->Hard (2), `l`->Light (0), `m`->Medium (1), `u`->Up (4).
Every other first letter takes the error path: the `Invalid animtype` string (which lives at
`0xa303c`, beside `Invalid air.animtype` and `Invalid fall.animtype`) is copied into the engine
error buffer and the function returns 0.

The same three-line scheme is at `0x47EE9B` for `air.animtype` and `0x47F019` for
`fall.animtype`, and again at `0x47F067` for `ground.type` (key offset `'h'`, table `0x47f308`:
`h`->High 1, `l`->Low 2, `n`->None 0, `t`->Trip 3) and `0x47F141` for `air.type`. The decompiled
C agrees at `angr_0047da40_hitdef_schema_parse.c:799-830` (animtype), `:832-864`
(air.animtype), `:877-909` (fall.animtype), `:911-935` (ground.type) and `:938-962` (air.type),
each a `switch` on `sub_40E380()` with cases `98,100,104,108,109,117`.

**Corroboration, and the reason a spelling match is impossible rather than merely absent:** a
byte-level scan of `mugen.exe` finds no `Light`, `DiagUp`, or standalone `Back`/`Up` anywhere in
the image. `Medium` and `Hard` occur only inside the difficulty-menu table at
`0xa02bc`/`0xa02c4`; `Back` occurs only within `Backspace` at `0x9cbe4`. There is no string
constant the parser could have compared against.

**What it costs to get wrong, and how it was got wrong here.** `parseAnimType` matched a
*prefix* of the documented names — `"ligh"`, `"medi"`, `"hard"`, `"back"`, `"diag"`, `"up"` —
which is the same function for every value the documentation names and a different one for
everything else. The corpus writes `animtype = heavy` **744 times**, and the reference reads
its `h` and calls it Hard while a prefix match against `hard` falls through to Light: the
victim's reaction animation is the medium one instead of the hard one, on 744 hits across the
corpus. `med` (17) reads as Light instead of Medium, `down` (13) as Light instead of DiagUp,
`high` (7) as Light instead of Hard.

`parseHitType` had the same prefix shape but no divergence: `low`, `trip` and `none` are
written 2,177 / 491 / 31 (ground) and 594 / 273 / 5 (air) times and the prefix and the first
letter agree on all of them, and `hard` (4 uses) reaches the `default:` arm as High either way.
It is now first-character too, so the two families read alike.

`animtype` is reported to a character as `GetHitVar(animtype)` — **990 uses across 38
characters**, the most-used trigger in the whole hit family — so this was not a cosmetic
difference.

**An unrecognised first letter is not silently Light in the reference**: it is `Invalid
animtype` and a parse that reports failure. This engine returns Light, which is the documented
default for an *omitted* parameter and the harmless choice for a name the author got wrong,
and no value in the corpus reaches the branch. Recorded as the one remaining difference rather
than implemented, because whether the reference discards the single controller or aborts the
whole HitDef parse is not established (the parse returns 0 either way, and a 0/Light has
already been written to the slot when it does).

### 16.2 Correction: the fall bit is not derived, and `v10` is not the ground velocity's y

Pass 12 recorded, and the roadmap repeated, that `hitapply`'s prelude flag
(`if (v11->field_78 & 1 || v10) v12->field_10c8 = 1;`) takes its second term from the ground
velocity's y. **That reading is wrong**, and the filler is what settles it.

The hitdef filler writes the parameter block from the parsed template in slot order, and in
the run `forcestand`, `forcenofall`, `fall`, … the writes are
(`angr_0047f380_at_00480308.c:302-306`):

```c
a1->field_b4  = -(-(0 < sub_439260(1)));   /* forcestand  */
a1->field_b8  = -(-(0 < sub_439260(1)));   /* forcenofall */
if (sub_439260(1))
    a1->field_78 = a1->field_78 | 1;       /* fall        */
```

So the fall bit is written by the `fall` parameter and by nothing else: there is **no implicit
derivation of `fall`**. The implicit derivation in that region is `forcestand`'s, twenty lines
further on at `:385-386` — `if (v29 != 0.0f && !a0[101]) a1->field_b4 = 1;`, where `v29` is the
ground velocity's y just stored at `+0x134` — which is exactly the rule the documentation gives
for `forcestand` ("Normally defaults to 0, but if the y_velocity of the `ground.velocity`
parameter is non-zero, it defaults to 1"). That rule belongs to `forcestand` alone.

`v10` is instead the caller's fifth argument (`[bp+0x14]`, `angr_p9_hitapply.c:235`, read and
never written in that function). The caller computes it:

```c
v3 = 0;                                       /* angr_p9_hitresolve.c:244 */
...
if (sub_470030() && v24->field_e68 && !a0->field_257) {
    v26 = sub_46e760(a0->field_11ec, a0->field_11d8);
    if ((int)(v24->field_1b8 - v24->field_1084 - v26) > 0) v7 = 1;
    else if (v23->field_11bac) { v3 = 1; ... }
} else {
    v27 = sub_46e760(a0->field_11e8, a0->field_11d4);
    if ((int)(v24->field_1b8 - v24->field_1084 - v27) <= 0 && v24->field_1c != 1) { v3 = 1; ... }
}
if (v23->field_10ee8) v3 = 0;
```

and passes it at `:419` (`sub_46db20(v25, *(float *)&a0->field_1f4, *(float *)&a0->field_1f8,
v2, v3)`). The other call site passes a literal `0` (`angr_p9_pair_writer.c:383`,
`sub_46db20(v27, …, v10, 0)`). Every term in the computation is a distance or a position —
`sub_46e760` of two pairs of `a0`'s fields against `v24`'s `field_1b8` less its `field_1084` —
and none of them is a velocity. What the flag *names* is not established; the code around its
other two uses is consistent with "this hit takes the victim off their feet" rather than with
a distance ("`if (v10) field_e58 = 1;`" forces the standing type at `:287`, and `if (v10)` at
`:317` gives the launch a minimum push — `x *= 0.66` when the hit is flat, `y` clamped to at
most `-6`), which is why the roadmap's engine-side stand-in for it is an upward velocity.

**Consequence for the engine, unchanged and now recorded as a heuristic rather than a
reading:** the flag is resolved *above* the statetype branch, so one expression decides it for
a grounded and an airborne victim alike, and the engine's ground branch already spells that
expression as `pushY != 0.0f || fall`. The air branch adds `fall` and `air.fall` (see the
roadmap) and keeps `launches` — an upward velocity — as the stand-in for the unnamed second
term. It is not the reference's rule and is not claimed to be.

## Pass 17 — `forcestand`/`forcenofall`, the `a0` template view, and the fall flag's carrier (behaviour finding, 2026-10-18)

### 17.1 `a0` is the parsed HitDef template read as 32-bit words

Pass 16 quoted the filler's implicit-default line as

```c
/* angr_0047f380_at_00480308.c:385-388 */
v29 = (float)v21;                       /* the ground velocity's y, stored just above */
*(float *)&a1->field_134 = v29;
if (v29 != 0.0f && !a0[101]) a1->field_b4 = 1;
```

and left `a0[101]` unidentified. It is now settled by the schema parser, which sets
it and only it, for this one parameter, and only when the parameter is actually
present in the controller:

```c
/* angr_0047da40_hitdef_schema_parse.c:564-572 */
v62 = sub_433410(v50, "forcestand");
if (v62) {
    *(unsigned int *)&a1[404] = 1;                    /* byte 404 = word 101 */
    if (!sub_438b20(v62, a1 + 380, v23, 0, 1)) return 0;   /* the value, byte 380 */
}
if (!sub_438b20(sub_433410(v50, "forcenofall"), a1 + 392, v23, 0, 1))
    return 0;                                        /* no flag written beside it */
```

So `a0` is the parsed template viewed as an array of 32-bit words — the parser's
byte-addressed `a1[404]` and the filler's `a0[101]` are the same four bytes (404/4 =
101) — and **slot 101 means "the file wrote `forcestand`"**. The rest of the index
space is confirmed against the parser rather than merely being consistent with it. The
parser is byte-addressed throughout, so a parameter's template byte offset over four is
its word index:

| parameter | parser byte | word | evidence |
|---|---|---|---|
| `hitflag` | 284 | 71 | `a1[284] \|= v87`, else `\|= 62` (`:789-796`) |
| `guardflag` | 280 | 70 | `a1[280] \|= v86` (`:786-787`) |
| `forcestand` | 380 (value) + 404 (written flag) | 95 / **101** | (`:565-572`) |
| `forcenofall` | 392 | 98 | no flag beside it (`:572`), and default 0 |
| `animtype` | 376 | 94 | `a1[376] = 0` then the six-way switch (`:798-820`) |
| `fall.animtype` | 424 | 106 | derived from 760, then its own switch (`:865-889`) |
| `air.animtype` | 760 | 190 | `a1[760] = a1[376]` then its own switch (`:831-853`) |
| `ground.hittime` | 696 | 174 | (`:452`) |
| `fall` | 540 | 135 | written, and registered as `air.fall`'s default (`:770-775`) |
| `air.fall` | 776 | 194 | `sub_438c40(v84)` takes back that default (`:776-781`) |

The filler's own reads line up with those indices where it reads the template at all
(`a0[94]` → `+0xB0`, `a0[106]` → `+0xC4`, `a0[173]` → `+0x124`, `a0[189]` → `+0x13C`,
`a0[190]` → `+0x140`, `a0[101]` → the guard on the implicit default at `:385-388`),
while the parameter *values* it writes come from its sequential reader
(`sub_439260`); the left-hand offsets are the **active** block's, which is what
`hitapply` reads as `v11->field_b0` and so on. The string table at `0xa2b00` is a third
order again — the parameters' interned-name emission order, `forcestand` immediately
before `forcenofall`, `animtype` twelve words before `air.animtype` — and is not either
index space. A reading that walks any two of the three as if they were one array lands
on the wrong parameter; `+0xB0` = 176 = 4×44 is the kind of coincidence that invites it.

One further piece, because it is the same run of code and the next slice needs it: the
guard family's defaults are all registered here as template-slot defaults, and the
binary's answers are the documented ones exactly —

```c
/* angr_0047da40_hitdef_schema_parse.c:647-676 */
guard.slidetime (908)  <- ground.slidetime (708)
guard.hittime   (896)  <- ground.hittime   (696)
guard.velocity  (884)  <- ground.velocity (720)
guard.ctrltime  (920)  <- guard.slidetime  (908)
```

(§17.2's `field_78` is the *active* block's flag word: the filler folds the `fall`
parameter into its bit 0 — `if (sub_439260(1)) a1->field_78 = a1->field_78 | 1;`
(`:305-306`) — and that is the bit `hitapply` tests as `v11->field_78 & 1`. The
template's `fall` is a plain value at byte 540; the active block keeps a bit.)


### 17.2 The prelude, read as three branches rather than two

```c
/* angr_p9_hitapply.c:261-269 */
if (v11->field_78 & 1 || v10) { v12->field_10c8 = 1; }      /* fall, or the caller's flag */
else if (v11->field_b8)       { v12->field_10c8 = 0; }      /* forcenofall */
v18 = v12->field_10c8;
if (!v18) v12->field_1078 = v11->field_b0;                 /* animtype */
else      v12->field_1078 = v11->field_c4;                 /* fall.animtype */
```

`v12->field_10c8` is the victim's own persistent **"in a fall" latch**, not the hit's
verdict about this frame: the `else` writes nothing at all, so when neither `fall` nor
`forcenofall` is written the victim's existing value survives. That preservation is
the entire reason `forcenofall` exists as a parameter — without the carry there is
nothing for it to override — and the latch is read back two lines later to choose
between `animtype` and `fall.animtype`, which is why a carried fall also carries the
fall's animation.

`forcestand` then appears once, in the same function's grounded branch, *after* the
victim's posture has been read:

```c
/* angr_p9_hitapply.c:279-288 */
v19 = v12->field_e58;                 /* 1 = standing, 2 = crouching */
if (v19 == 1 || v19 == 2) {
    if (v11->field_124 && !v11->field_bc) {        /* ground.type != None */
        ...
        if (v11->field_b4 || v10) v12->field_e58 = 1;
        v12->field_c4c = (0 < v12->field_e58 - 1 ? 5010 : 5000);
```

`field_b4` is `forcestand` and `field_b8` is `forcenofall`, so the two reads land as:
the fall latch is decided above the state-type branch (one expression for a grounded
and an airborne victim alike), and the stand-vs-crouch reaction is decided below it by
the victim's own posture, with `forcestand` able to force it back to standing. That is
also, incidentally, why `ground.type` — `field_124`, slot 173 — is the type read in the
ground branch and `air.type` — `field_13c`, slot 189 — in the air one.

### 17.3 The implicit default, and what the corpus does with it

The filler reads the template's value first and then overwrites it, guarded by the
written flag:

```c
a1->field_b4 = -(-(0 < sub_439260(1)));        /* :303  forcestand, as written */
...
a1->field_134 = v29;                           /* :385  ground.velocity's y */
if (v29 != 0.0f && !a0[101]) a1->field_b4 = 1;/* :388  ...or 1, if the file said nothing */
```

which is the documentation's rule exactly — "Normally defaults to 0, but if the
y_velocity of the ground.velocity parameter is non-zero, it defaults to 1" — including
its ordering: an explicit `forcestand = 0` beside a non-zero y stays 0.

**Corpus:** across 2,571 `ground.velocity` writes, `forcestand` is written 4 times and
`forcenofall` once. All four `forcestand` writes are `= 1`, and all four sit beside a
`ground.velocity = -3` — a single value, so a y of zero, so a default of 0 and a write
that changes nothing. The parameter's explicit form is therefore dead weight in this
corpus and its *implicit* form governs every launcher in it; the four files that spell
it out are not the ones it matters for.

### 17.4 Correction: `field_b4` was mis-named in passes 12 and 16

Pass 12 introduced `hitdef+0xB4` as an "implicit launch flag", and pass 16 kept the
name while correcting what writes it. It is `forcestand`. The naming mattered: a
"launch flag" suggests the engine has an opinion about launches that the file cannot
express, which invites inventing one, whereas the parameter is a documented one with a
documented default whose only reader is a three-line stand-vs-crouch choice. Pass 12's
§12.1 rule — non-zero `ground.velocity` y, "no upward sense to it" — was right and is
now attributed to the right parameter.

### 17.5 What the engine does with it

Implemented in `statemachine.cpp`/`statemachine.h` as `HitDef::forceStand` and
`HitDef::forceNoFall`:

- `forceStand` is the explicit value when the controller writes `forcestand` and
  `ground.velocity`'s y being non-zero otherwise, which is §17.3's rule with the
  written flag kept rather than reasoned about.
- The ground branch's state number is the victim's posture — `5010` crouching, `5000`
  otherwise — unless `forceStand`, matching §17.2's `field_e58` write in front of the
  `5010`/`5000` choice.
- `carriedFall = victim.hitFall() && !hit.forceNoFall` is `field_10c8`'s `else` branch,
  and it feeds both branches' `falls` expression. `hitFall()` is gated on the victim
  still being in a hit reaction, which is the state the latch lives in.
- The carry is deliberately reachable **only** through the latch: `fall`, `air.fall`
  and `launches` are tested alongside it, so `forcenofall` cannot un-knock-down a victim
  the current hit is knocking down, exactly as the prelude's `else if` cannot.

The behavioural bug this fixes: a victim juggled out of a knockdown. Before, `falls`
was recomputed from the incoming HitDef alone, so the second hit of an air combo
cleared the latch, `5030` handed over to `5040` instead of `5050`, and the victim
landed on their feet in mid-air.

**Not established, and not needed here:** what `v10` is computed from. It is passed by
the caller (§16.2) and every term in it is a position or a distance, but the two places
it is *used* are `if (field_b4 || v10) field_e58 = 1;` and the fall latch, both of which
are what a launch implies, and the third (`:317`, a minimum push for a flat hit) is
consistent with the same reading. The engine's stand-in — an upward velocity — is
therefore left as it stands and is still marked a heuristic.

## Pass 18 — the three guard clocks (behaviour finding, 2026-10-18)

### 18.1 The parser's guard-family registrations, and what the engine had wrong

Reading `angr_0047da40_hitdef_schema_parse.c:647-676` for pass 17's index space turned
up four default registrations the engine had never implemented, in the block that runs
after a HitDef's own parameters are read:

| slot | byte | default it is registered from |
|---|---|---|
| `guard.slidetime` | 908 | `ground.slidetime` (708) |
| `guard.hittime`   | 896 | `ground.hittime` (696) |
| `guard.velocity`  | 884 | `ground.velocity` (720) |
| `guard.ctrltime`  | 920 | `guard.slidetime` (908) |
| `air.animtype`    | 760 | `animtype` (376) |
| `fall.animtype`   | 424 | `air.animtype` (760) when Up(4)/DiagUp(5), else Back(3) |

and `fall` (540) registered as `air.fall`'s (776) default, with `hitflag` (284)
defaulting to `|= 62`. Only the `guard.ctrltime` ← `guard.slidetime` step was what this
engine already did; the other three were not. `guard.hittime` was parsed at
`statemachine.cpp:4067` (old numbering) and **read nowhere at all** — a parameter 600
HitDefs in the corpus write, writing into nothing — and its default was `hit.slideTime`
where the reference defaults it to `ground.hittime`. `guard.slidetime` defaulted to
`guard.hittime`, so with both unwritten the pair came out as the slide time twice over.
The old comment on that line asserted that MUGEN defaulted `guard.hittime` to the slide
time, "looks like a mistake and is what MUGEN actually does" — it is not, and the
comment was the whole of the evidence for it.

### 18.2 A block's four clocks, and which two the engine confuses

`sub_46E7C0`'s callers write the blocker's clocks from the active HitDef:

- `+0x1090` **shake** ← `+0x180`, `guard.pausetime`'s second value (`guard.p2shaketime`)
- `+0x1094` **hittime** ← `+0x170` (`guard.p2hittime`)
- `+0x1098` **slidetime** ← `+0x174`
- `+0x109C` **ctrltime** ← `+0x178` / `airguard.p2ctrltime`; an *air* block writes only
  this one, leaving the other three as the previous hit left them (recorded in pass 11,
  not implemented: this engine's `stunTicks_` is assigned from `HitInfo::hitTime`
  unconditionally, and carrying the victim's existing hitstun through a block is a
  different change).

A block is a hit with two extra clocks and its own state family, and the engine's
`HitInfo` has exactly two clock fields. It was seeding **both** of them from
`guard.ctrltime`, so `GetHitVar(hittime)` on a block answered the control clock, and the
held time came out of the same number. Now: `HitInfo::hitTime` ← `guard.hittime`
(reported as `hittime`, and what `stunTicks_` is seeded from) and `HitInfo::ctrlTime` ←
`guard.ctrltime` (reported as `ctrltime`). That is the mapping the reference's two
fields have: `GetHitVar(hittime)` reads the former, `GetHitVar(ctrltime)` the latter.

### 18.3 The shakes are timed on the shake clock, and my corpus numbers were reversed

150/152/154 now sit in `builtinExitsOnShake`, so a built-in guard shake is timed on
`shakeTicks_` — `guard.pausetime`'s second value, which is also what the reference feeds
`+0x1090` — as the five get-hit shakes already were.

They had been held back on the reasoning recorded in pass 12/pass 16's roadmap row:
"241 HitDefs write `guard.pausetime` against 2,544 that write `pausetime`, so a guard
shake timed off the wrong clock is one tick for 90% of the corpus". **That is backwards,
and the mistake was reading a count of explicit writes as a count of HitDefs the default
would not reach.** `guard.pausetime` does not default to zero: it defaults to `pausetime`
itself, so the 2,544 HitDefs that write `pausetime` are the ones whose guard shake is
*correct* under the shake clock, and the 74 that write neither are the only ones whose
shake is zero-length — which is also what the reference gives them, since the clock it
seeds is zero and their shake is over on the tick it is entered. The correct reading of
the write counts is: 2,544 write `pausetime` (inherited), 241 write `guard.pausetime`
explicitly, 600 write `guard.hittime`, 224 write `guard.slidetime`.

### 18.4 What the host suite pins, and what it deliberately does not

The clock *identity* is what is checked, not the reference's numbers, because a test
that asserts "11 ticks" for a HitDef the reference has not been read on is a test that
asserts this engine's arithmetic:

- `pausetime = 4, 10` beside `guard.ctrltime = 6` — the two numbers deliberately differ —
  holds the blocker for eleven ticks (the ten of the shake clock plus the tick it is
  entered on), with zero travel, then hands it to 151. A shake measured off the ctrl
  clock, or off a single tick, fails that count.
- The push is not applied during the shake: travel is measured *before* each tick runs,
  so the tick the shake ends on — the one the push lands on, in 151 — is not counted.
- `guard.hittime = 15` beside `guard.ctrltime = 9` reports 15 through
  `GetHitVar(hittime)`, holds the blocker for 15, and reports 9 through
  `GetHitVar(ctrltime)`; and a HitDef writing only `ground.hittime = 17` reaches the same
  field as 17, which is 18.1's inheritance measured rather than assumed.
- Pressing down on the tick the shake *ends* takes a standing blocker through 153 into
  131, not through 151 into 130 — the height is read when the shake ends, not when it was
  entered.

**Open, and the next slice:** a built-in blocker that has left its shake sits in
151/153 until `stunTicks_` expires, because `runStateLogic` keeps the built-in table
behind the stun gate, so the built-in guard chain holds for shake + `guard.hittime`. The
reference's guard knock-back is the character's own `common1.cns` ended by its own
action, and while `sub_46E7C0` is known to *write* the clocks, its decrement site has not
been found — so whether the reference holds a blocker for shake + hittime, for the longer
of the two, or only for the file's action is unestablished. The host test asserts the
chain and the push rather than the length, for that reason. An RE pass on `sub_46E7C0`'s
attribution, the `+0x1090`/`+0x1094` tick-down, and their readers is the work that settles
it.

### 18.5 The `yaccel` open item, closed: the default is real and it lives at the point of use

The `GetHitVar` tail row carried two questions. `isbound` is answered in the engine
already (`statemachine.cpp`, the key test beside the bind bookkeeping), so the row was
stale there. `yaccel` was the live one — "whether its parse and default are exactly the
reference's" — and it now has an answer from both sides.

**The documentation** (`local_mugen/extracted/mugen/docs/sctrls.html`, the HitDef
section): "`yaccel = accel (float)` — Specifies the y acceleration to impart to P2 if the
hit connects. **Defaults to .35 in 240p, .7 in 480p, 1.4 in 720p.**" So the parameter's
default is not free: it is one number per resolution, in a fixed ratio of 1 : 2 : 4.

**The binary** reproduces exactly that, and it does so at the *use* site rather than in
the parser. `angr_p9_hitapply.c:513-522`:

```
if (v11->field_19c == -FLT_MAX) { v4 = <resolution width>; v33 = v4 * 0.35 / 320.0; }
else                           { v33 = v11->field_19c; scale(v33); }
v12->field_10b8 = v33;
```

- the active block's yaccel is `+0x19C`, and the *victim's* resulting fall acceleration
  lands in `+0x10B8` — the field the falling states' own `VelAdd` ends up reading through
  `GetHitVar(yaccel)`;
- an unset yaccel is the sentinel, and it is resolved to `0.35 × (width / 320)`, which is
  `.35` at 320, `.7` at 640 and `1.4` at 1280 — the three documented numbers to the digit;
- a written one goes through the same scaling helper the velocity fields use, so the
  file writes 320p units and the engine scales them, exactly as `fall.yvelocity` is
  scaled at parse time (`-4.5 / field_a4`).

And the parser, read for pass 17's index space, registers **no** default for the
template's `yaccel` slot (`a1 + 0x404`) — `sub_438d50(-FLT_MAX)` immediately before its
`sub_438b20`, where the parameters around it get real numbers registered. That is the
sentinel, and it is why the default had to be looked for at the use site at all.

**What this means for the engine: nothing changes, and the reason is the resolution.**
`hit.yaccel = firstParam(controller, "yaccel", *this, 0.35f)` is the 240p default, and this
engine has one coordinate frame rather than three, so there is no width to scale by and
`0.35` is the reference's own number for the frame the engine uses. The corpus writes no
sentinel at all — 380 `yaccel` mentions across the packs in `local_chars/`, and every
written value is an ordinary number (`.36`, `.44`, `.4`, `.6`, `.65`, `.45`, `.8`), so
even a character that meant "leave it to the engine" spells it by omitting the line. The
open question is closed with a *check* rather than a change: the engine's default was
already the documented one, and it was the only one of the three.

## Pass 19 — the guard chain's length, the live hit clocks, and the `p2stateno` borrow (behaviour finding, 2026-10-18)

Three answers, one of them a correction to this engine's own behaviour. The disassembly
behind each is in `GUARD-CHAIN-RE.md` (guard clocks) and `WIDTH-INVESTIGATION.md` is the
sibling survey, not this pass.

### 19.1 A block is held for shake **then** hittime, not for the longer of the two

Only two decrements exist in the whole binary, both in the per-frame player update at
`0x46B440`:

- `+0x1090` (shake) at `0x46B732`, `if > 0`, inside the `movetype == H` branch;
- `+0x1094` (hittime) at `0x46C1B1`, `if >= 0`, reached by both the `movetype == H` and
  the reset branch, and **gated on `+0x1090 == 0`** (`0x46C1A9` / `0x46C1AF`).

`+0x1098` (slidetime) and `+0x109C` (ctrltime) have no decrement site anywhere: they are
write-once constants that `common1.cns` compares against `Time`, not counters.

So the shipped chain's length is `guard.pausetime[1] + guard.hittime`, counted one after
the other: 150 leaves on `HitShakeOver`, 151 leaves on `HitOver`. It is not
`max(shake, hittime)` — a `pausetime = 0,10` hit with `guard.hittime = 15` holds for 25.
This engine already froze `stunTicks_` during the freeze (the decrement is below the
hitpause early-out), and its shake clock is the same number as the freeze, so the
*ordering* was already right; what was wrong was the boundary, below. The host test now
pins the observable length: 11 shake ticks, then 13 knock-back ticks (12 countdown values
plus the hand-over), for `pausetime = 4,10` and the default `guard.hittime = 12`.

### 19.2 `hittime` counts through 0 to -1, and both hit clocks are live

`GetHitVar(hittime)` reads `+0x1094` (getter at `0x487868`) and `GetHitVar(hitshaketime)`
reads `+0x1090` (`0x48785D`); `HitOver` is `+0x1094 < 0` (`setl` at `0x4875AC`) and
`HitShakeOver` is `+0x1090 <= 0` (`setle` at `0x48759A`). Because the decrement runs while
the value is `>= 0` and state logic runs *before* it, the clock reads `H … 1, 0, -1` and
`HitOver` turns true one tick after the number reaches zero.

The engine had three defects against that, all now fixed:

1. `GetHitVar(hittime)` returned `lastHit_.hitTime` — the number the HitDef wrote, frozen
   forever. A character's `Time = GetHitVar(hittime)` therefore compared against a
   constant. It now returns the live `stunTicks_`.
2. `GetHitVar(hitshaketime)` returned `lastHit_.victimPause`, likewise frozen. It now
   returns the live `shakeTicks_`.
3. `stunTicks_` stopped at 0 and `hitover` was `stunTicks_ == 0`. It now counts to -1 and
   stops (`if (stunTicks_ >= 0) --stunTicks_`), `hitover` is `stunTicks_ < 0`, and the
   built-in reaction hold uses `>= 0` so it covers the tick the clock reads zero — which
   is exactly the tick the reference spends before `HitOver`. A fighter that has not been
   hit sits at -1 (the reference's reset branch clears `+0x1094` to 0 and the same frame's
   common decrement takes it to -1). `fighter.cpp`'s placeholder reset was updated with
   it.

The ±1 frame alignment — whether the contact frame counts against the shake — is still
not established (GUARD-CHAIN-RE §1.5); the test pins the engine's own ordering, which is
the reference's controller-before-decrement order.

### 19.3 `p2stateno` is a state in the **attacker's** file, and the pair travels together

The parser registers `p2getp1state`'s default on the branch where `p2stateno` is found:
the schema parser pushes pending defaults `-1, -1, 0`, parses `p1stateno` → `a1+304`, and
on `p2stateno` pushes `1` before parsing it into `a1+316`, then parses `p2getp1state` into
`a1+328`. That is the documented rule ("Defaults to 1 if the `p2stateno` parameter is
used. Ignored otherwise"), and the consequence is that the state number is resolved in
the attacker's file: "P2 will retain P1's states and animation data until P2 is hit, or a
SelfState controller is used to return P2 to his own states." The reference's debug
overlay annotates such a fighter with `(in <name> <n>'s state rc)` — running another
player's state record is a first-class per-player state.

The engine used to resolve the number in the victim's own file. The corpus makes the
size of that mistake plain: of 276 literal `p2stateno` writes in `local_chars/`, 272 name
a state defined in the writer's own folder (the 4 that do not are `dbz_vegeta_ssg`'s
`P2Stateno = 42732`), and exactly one write asks for `p2getp1state = 0`. Every throw
therefore dropped its victim into the unknown-state fallback mid-move: stand, idle,
`ctrl = 1`. Now:

- `HitDef::p2GetP1State` defaults to true iff `p2stateno` was written, and is forced false
  when it was not (the documented "ignored otherwise");
- `HitInfo::stateSourceChar` carries the attacker's `Character` to the victim (the static
  path a projectile lands on has no attacker machine, so it travels with the hit, like
  `attackerX`);
- `StateMachine::borrowedStateChar_` latches the file, and **all** state resolution —
  the state table, `-1`/`-2`/`-3` controller lists, `statenoexist`, the landing/guard-chain
  capability caches — goes through `stateLibrary()`, while all art goes through
  `animCharacter()`;
- it survives state changes made by the borrowed file, and is cleared by exactly the two
  things the docs name: any hit that lands (`applyHit`) and a `SelfState` controller.
- the projectile hit path passes `beam.owner` as the source character.

What is deliberately *not* borrowed is the command list: `commands_` stays the player's
own (`setup` takes it from `character_`, not from `stateCharacter()`), because a CMD file
and its buffer belong to the player, not to the state record. A `command = "..."` trigger
inside a borrowed state therefore reads the victim's input, which is what a throw escape
written in the attacker's 9990 needs.

Host tests pin the default, the borrowed header (airborne / `movetype = H` / no control)
versus the fallback, the clear-on-hit, the `SelfState` return, the explicit
`p2getp1state = 0` opt-out, and the ignored-without-`p2stateno` case.

### 19.4 An air block leaves the hitstun and slidetime clocks alone

The block reaction writes the blocker's four clocks from the active HitDef, but not all
four on both paths. The ground path writes `+0x1090` (shake) and then `+0x1094`
(`guard.hittime`), `+0x1098` (`guard.slidetime`) and `+0x109C` (`guard.ctrltime`). The
air path writes `+0x1090` and `+0x109C` (`airguard.ctrltime`) and a velocity pair, and
stops there: `+0x1094` and `+0x1098` are **not written at all** (`0x46E8D9`-`0x46E950`,
`GUARD-CHAIN-RE.md` §1.2). Two corollaries follow, and neither is a rounding detail:

- an airborne blocker keeps whatever hitstun it already had, and a blocker who was *not*
  already stunned keeps `-1`. `hitover` is therefore true for the whole block, and the
  pose is held by `HitShakeOver` instead — which is exactly 154's own exit. Seeding the
  hittime from the guard clock, as this engine did, invented a stun the reference never
  starts.
- `GetHitVar(slidetime)` in an air block reports the *previous* hit's number, because
  the slot is a write-once constant that this path does not touch.

The engine used to set `info.hitTime = hit.airGuardCtrlTime` in the air branch of
`hitInfoFor` and then seed `stunTicks_` from it unconditionally in `applyHit`, so a
blocked air hit reported the control clock through `GetHitVar(hittime)` and reported
`guard.slidetime` through `GetHitVar(slidetime)`. Now `HitInfo::keepStunClock` marks the
air path; `applyHit` skips the seed on that path in both branches (the guarded branch and
the shared non-guard line), and the air branch copies the victim's previous
`lastHit_.slideTime`. The ground block is unchanged and still seeds its hitstun.

Host tests pin `keepStunClock` on the air path and its absence on the ground one, the
preserved slidetime, and — after a real air block driven through `applyHit` — that
`GetHitVar(hittime)` is `-1` while `GetHitVar(ctrltime)` is `airguard.ctrltime`. The check
is not vacuous: the first build, before the guarded branch's seed was made conditional,
failed on exactly that assertion.

### 19.5 A screenpack element's `trans` carries its pair too

The blend-pair slice (`PARITY-ROADMAP.md`, "One draw's transparency, from all three
sources") reached the character/animation side and left `defs.cpp`'s `blendFromTrans`
folding the screenpack HUD's `trans` back into a bare `Blend`. That made one corpus-visible
error: `add1` is documented in `air.html` as the shorthand for `AS256D128`, and
`mugen1/system.def` and `big/system.def` both use `trans = add1` for title and select
layers. Drawn as a bare additive mode the destination factor was `1` instead of `0.5`,
i.e. every such layer was twice as opaque as the reference draws it.

`DisplayElement` and `BgElement` now carry `TransParams`; `transFromDef` reads `add`,
`add1`, `sub`, `addalpha`/`subalpha`, an inline pair (`addalpha, 256, 0`) and the separate
`alpha` key. The inline pair wins; an explicit `alpha` overrides the type's default pair;
an unrecognised word is dropped to a normal draw rather than guessed at. The `alpha` key's
units are ambiguous in the format as this corpus writes it — no screenpack in
`local_mugen` writes one — so a pair with no value above 1 is read as this engine's
existing normalised spelling and any larger value as MUGEN's 0..256.

Two adjacent parse bugs were found while wiring it. In `readElement`, `alpha` and
`window` were read as flat keys while every other key of an element is prefixed with the
element name. `local_mugen/.../fight.def` writes `p1.face.window = 0,0, 739,959`, so the
portrait clip never reached the element at all; both reads are now prefix-aware. The
background-element path (`[TitleBG n]`/`[SelectBG n]`) was already unprefixed and is
unchanged.

`motif.cpp`'s draw paths now pass the pair through (`element.trans`, or the frame's own
when its animation overrides the element); `applyAlpha` survives only for text, which
draws through a tint with no blend state of its own. Host coverage lives in
`tools/hosttest/test_select_config.cpp` against synthetic documents; the `add1`
destination and the alpha-pair cases were each shown to fail under mutation, and the
engine was restored byte-identically afterwards. The GL calls remain the one part the
host build cannot reach, as they were for the slice above.

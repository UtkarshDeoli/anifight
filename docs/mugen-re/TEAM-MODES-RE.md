# Team modes RE — select config, flow, triggers, and the anifight gap

Reference: `mugen.exe` (MUGEN 1.0, PE32 i386, MSVC 9.0, image base `0x400000`),
read from `/tmp/mugen_peek/mugen/mugen.exe` and its objdump text
`/tmp/mugen_peek/mugen_disasm.txt`. All addresses are VAs. The shipped 1.0
documentation and data were read read-only from
`local_mugen/extracted/mugen/`; character-corpus counts come from
`local_chars/` (41 top-level character directories, `*.cns`, `*.cmd`, `*.def`,
`*.st`, read-only). Nothing from either directory is copied here.

Prior binary work is in `docs/mugen-re/FINDINGS.md` (trigger opcode map,
redirect accessors, controller dispatch) and the decompiled excerpts in
`docs/mugen-re/decompiled/`. This note adds only behavior and addresses.

Confidence: **HIGH** = binary and docs agree, or direct code evidence;
**MED** = one source or a partial trace; **LOW** = inference, no doc or corpus
support.

## 0. Answers in one screen

| question | answer | confidence |
|---|---|---|
| Does MUGEN 1.0 have `team.menu`, `team.menu.item.*`, `team.1`, `team.2`? | **No.** None occurs in shipped `system.def`, `select.def`, the docs, or `local_chars/`. The team menu is `[Select Info]`'s `teammenu.move.wrapping` plus `p1/p2.teammenu.*`. The selected *mode* lives in the menu/game state, not in a roster file. | HIGH |
| Where is team size chosen? | In the team-mode menu: Single is 1, Simul is 2 simultaneous, Turns is 1 active plus up to 3 standby. In Turns, left/right changes the number of members. No `select.def` field sets team size. | HIGH |
| How is a team mode encoded? | The `TeamMode` right-hand keyword is compiled to: `single` = **0**, `simul` = **1**, `turns` = **3**. Value **2 is unassigned**; `tag` is not accepted by the 1.0 compiler. | HIGH for 0/1/3; MED for 2 reserved |
| Member counts and rounds? | Single: one character, 2 rounds. Simul: two characters at once, team has 2 rounds. Turns: up to 4 characters, one active at a time, each member has 1 round. Starting life is adjusted by the number of players on each side. | HIGH (readme) |
| Team-only config keys? | `mugen.cfg`: `Team.1VS2Life = 150`, `Team.LoseOnKO = 0`, plus `team.AIramp.start/end`. `select.def`: `[Options] team.maxmatches`, which is Team Battle ladder length, not team size. | HIGH keys; MED exact consumers |
| What does `TeamMode` return? | Comparison-only trigger: `TeamMode [oper] mode`, oper `=` or `!=`, result boolean 1/0. Other operators are invalid. In Survival the enemy side is `turns`. | HIGH |
| What is `NumPartner`? | Value trigger: number of partners. Neutral players and normal helpers are not counted. | HIGH docs; MED binary count path |
| What is `partner`? | A redirect, not a numeric trigger. It resolves to the player's partner; an invalid destination yields `bottom`, which a condition treats as 0. | HIGH docs |
| What are `standby`, `TagIn`, `TagOut`? | Not documented in 1.0. The binary still parses `standby` as trigger opcode `0xCD` and `TagIn`/`TagOut` as controllers `0x227`/`0x228` with a per-player latch at **+0x22B4**. No tag mode is selectable, so this path is unreachable in normal 1.0 play. | MED existence; LOW semantics |
| Is there an `assist` trigger/controller? | **No.** It is absent from the resolved string table, the trigger compiler, the docs, and the corpus. | HIGH |
| `matchno` and `roundsexisted`? | `matchno` = opcode `0x74`; always 1 in versus, increments per Arcade/Team Arcade match. `roundsexisted` = opcode `0x75`; number of rounds the player has existed for, first round 0. | HIGH |
| What must an engine track? | Per side: ordered member list, active index, each standby member's own life/meter/state, and (for tag) a switch latch/cooldown. See §2.5. | HIGH as design requirement |
| What does anifight do today? | Single-side only: team triggers are hardcoded, the `partner` redirect always yields null, selection creates one fighter per side, member arrays do not exist, and `team.maxmatches` is parsed but never read. `teammode = single` is concretely false. See §4. | HIGH code evidence |

## 1. Select and roster configuration

### 1.1 The keys that exist in MUGEN 1.0

| key / section | location | role |
|---|---|---|
| `teammenu.move.wrapping` | `data/system.def:213` | whether the team-mode list wraps |
| `p1.teammenu.*` | `data/system.def:214-234` | P1 team-menu layout: `pos`, `selftitle.*`, `enemytitle.*`, `move/value/done.snd`, `item.offset/spacing/font/active.font/active2.font`, `item.cursor.offset/anim`, `value.icon.offset/spr`, `value.empty.icon.offset/spr`, `value.spacing` |
| `p2.teammenu.*` | `data/system.def:236-256` | same block, mirrored |
| `[Simul Lifebar]`, `[Turns Lifebar]` | `data/fight.def:70`, `:121` | two-member and relay lifebar layouts |
| `[Simul Face]`, `[Turns Face]` | `data/fight.def:14220`, `:14259` | portrait layouts per member |
| `[Simul Name]`, `[Turns Name]` | `data/fight.def:14307`, `:14324` | name layouts per member |
| `[Options] team.maxmatches` | `data/select.def:168` | maximum Team Battle matches, split by opponent `order` tier, exactly like `arcade.maxmatches` |
| `Team.1VS2Life` | `data/mugen.cfg:16` | starting-life adjustment for unequal side sizes; shipped value 150 |
| `Team.LoseOnKO` | `data/mugen.cfg:17` | shipped 0, i.e. losing one simul member does not immediately lose the round |
| `team.AIramp.start/end` | `data/mugen.cfg:386-388` | Team-mode AI ramping |

The `value.icon` / `value.empty.icon` pair is the filled/empty member-count
display in the team menu; `value.spacing` lays those icons out. The mode list
itself is generated by the engine, not by a select.def entry.

Binary corroboration that this is a config-screen feature: the labels
`Team Mode Config` (`0x4A1548`), `Single VS Simul Advantage` (`0x4A15C8`) and
`TEAM MODE CONFIG` (`0x4A16A0`) are referenced from the config/options UI at
`0x45B8B7`, `0x45B92E`, `0x45C8D6`. This is screen text, not a parsing path.

### 1.2 The keys that do **not** exist

- No `team.menu`, `team.menu.item.*`, `team.1`, or `team.2` anywhere in the 1.0
  distribution or corpus. Those names are not the 1.0 format.
- No `tag` mode. The compiler accepts only `single`, `simul`, `turns`; the
  readme lists only those three team modes.
- No team-size field in `select.def`. The per-character parameters are `order`
  (1-10), `music`, `includestage`, and `hidden` (`data/select.def:53-60`).
  `order` selects which ladder tier an opponent belongs to; it does not change
  team size.
- `[Options] team.maxmatches` is ladder length. It uses the same ten
  order-tier buckets as `arcade.maxmatches`.

Anifight's parser reflects this: `RosterEntry` has only `defPath`, `stages`,
`isRandomCell`, `hidden` (`app/src/main/cpp/engine/defs.h:371-376`), and the
per-line parameter loop handles only `hidden` (`defs.cpp:626-640`). `order` and
`music` are ignored. `team.maxmatches` is parsed (`defs.cpp:657-663`) and
immediately logged as not implemented (`defs.cpp:676-678`), with no consumer
(`defs.h:388-390`).

### 1.3 Mode-menu flow (shipped readme behavior)

- Team modes first ask for the mode: up/down chooses it, and modes such as Turns
  use left/right to set the number of team members. A confirm key accepts.
- In **Team Arcade**, after a side's characters are chosen, that side also
  chooses the opponent's team mode.
- **Team Co-op** forces Simul: P1 picks a character, P2 picks the partner, then
  P1 chooses the opponent's team mode.
- **Survival** is an endless opponent stream; team size changes starting life,
  damage taken per member, and post-round healing.
- **Turns order switching** is allowed immediately before match load and after
  losing a round, before the next round loads. Hold forward/back to rotate one
  member; with four living members, hold up to rotate by two. Only members who
  have not been KO'd participate.

### 1.4 Corpus usage

Counts over the 41 top-level character directories, case-insensitive, restricted
to `*.cns`, `*.cmd`, `*.def`, `*.st`:

| surface | occurrences | files | characters |
|---|---:|---:|---:|
| `teammode` comparison (any value) | 45 | 25 | 13 |
| `NumPartner` | 104 | 6 | 3 |
| `partner,` redirect | 177 | 6 | 4 |
| `roundsexisted` | 88 | 39 | 30 |
| `standby` | 0 | 0 | 0 |
| `TagIn` / `TagOut` | 0 | 0 | 0 |
| `assist` | 0 | 0 | 0 |
| `matchno` | 0 | 0 | 0 |

`teammode` value breakdown: `= single` 12 uses (4 characters), `= simul` 4 uses
(2 characters), `!= simul` 16 uses (3 characters), `= turns` 13 uses (6
characters). The characters are: Giorno, Kira, dbz_gogeta_ssj4, dbz_vegeta,
dbz_vegeta_ssj4, dbz_vegetto_ssj, jolyne_asb, mista, mugen_black,
mugen_g6_naruto_sennin, mugen_izuku_midoriya, mugen_naruto_ashura_jus,
s_diavolo.

The raw word `simul` appears in 5 characters (7 files, 21 uses), including the
`= simul` / `!= simul` comparisons above. Do not count the word `turns` by
itself: an earlier raw count matched the English word "returns". Only
`teammode = turns` is real usage.

No `select.def`, `mugen.cfg`, or `system.def` occurs anywhere under
`local_chars/`, and no corpus file sets `team.maxmatches`, a `Team.*` key, a
`teammenu.*` key, or a per-character `order`. The only team-related settings in
the corpus are the character-state comparisons counted above.

## 2. Game-flow semantics per mode

### 2.1 Single

One character per side. The side that wins two rounds wins the match; the
shipped readme states "Your character has 2 rounds." `roundsexisted` counts the
rounds the character has existed for, starting at 0. Anifight's
`roundsWon_[2]`/`winsNeeded` model (`fight_screen.cpp:351-365`) matches this
case, and is the only case it describes.

### 2.2 Simul

Two characters per side are on screen and active at the same time. Each has an
individual lifebar/face/name, selected by the `[Simul ...]` sections. The team
loses when its members are out; the shipped `Team.LoseOnKO = 0` is consistent
with one KO not ending the round, though the exact consumer was not traced.
Starting life is scaled for unequal side sizes (`Team.1VS2Life`).
`NumPartner` is 1 for each member of a two-member side; `partner, ...` resolves
to the other member. Both members can hit and be hit; the round result is a team
result, not a per-character result.

### 2.3 Turns

One active member per side; up to three standby members. When the active member
is KO'd, the next living member joins. Each member has one round. Team order is
the selection order and can be rotated in the two windows described in §1.3.
The match ends when a side has no living members. The docs define
`roundsexisted` for the single-player case only; a new Turns member starting
its own count at 0 is the natural reading, but it is an inference, not a traced
engine rule. The readme does not say
whether power is shared or per-member; the single power bar per side in the HUD
suggests shared, but the binary team struct was not traced, so treat meter
carry-over as **LOW/UNRESOLVED** until a trace or play test decides it.

### 2.4 Tag (binary-only, not a 1.0 mode)

There is no tag entry in the team menu and no `tag` keyword in the `TeamMode`
compiler. The binary nevertheless contains a tag-style controller pair:

- **TagIn** is controller type `0x227`. Its compiler branch is reached from the
  name `tagin` at `0x4740FB`; it stores the type at `0x474133`, reads
  `stateno` into controller **+0x18**, initializes **+0x24** and **+0x30** to
  `-1`, then reads `partnerstateno` into **+0x24**, and handles the common
  `ctrl` parameter. Its executor at `0x47AEF7` takes the second argument as a
  gate: when nonzero it changes self to `stateno`, changes the partner to
  `partnerstateno`, and clears the partner's latch
  (`0x22B4(partner) = 0`, store at `0x47AF72`). It returns 5.
- **TagOut** is controller type `0x228`. Its branch is reached from `tagout` at
  `0x4741AA` and stores the type at `0x4741DC`, then takes the common `ctrl`
  parameter. Its executor at `0x47B0BC` sets `0x22B4(self) = 1` and returns 5.
  It does not itself swap members; the latch is the request that a team layer
  would consume.

`selfstate` is a separate standard controller (type 2) whose branch is adjacent
at `0x4741E8`; it is not a TagOut parameter. Do not attribute its `value`/`ctrl`
parameters to TagOut.

Because no tag mode is reachable, the exact switch input, cooldown, and the
consumer of the `+0x22B4` latch are **LOW/UNRESOLVED**. What is proven is that
the name strings, types, parameter reads, executors, and latch exist.

### 2.5 State a team-capable engine must track

| state | why | modes |
|---|---|---|
| Ordered member list per side | selection order defines Turns entry order and rotation | Simul, Turns |
| Active index per side | which member is on screen / receiving input | Turns, Tag |
| Standby member records | each has its own life, power, state, animation, facing, and round history while inactive | Turns, Tag |
| KO/elimination state | stop offering a KO'd member; match ends when the list is empty | Simul, Turns |
| Round/entry counter per member | `roundsexisted` is per-player; a fresh member starts at 0 | Turns |
| Team latch/cooldown | `TagOut` sets self `+0x22B4`; `TagIn` clears the partner's; a team layer must consume it | Tag |
| Side life scaling and `LoseOnKO` | unequal Simul sizes and early-loss policy | Simul |
| Team result, not per-character result | `MatchOver`, win poses, and round counters are team-scoped | Simul, Turns |

## 3. Triggers and controllers

### 3.1 Surface table

| surface | kind | opcode / encoding | semantics | evidence | confidence |
|---|---|---|---|---|---|
| `TeamMode` | comparison-only trigger | `0x79` | compares current team mode against a compile-time mode keyword; returns 1/0; oper `=` or `!=` only | name/opcode emit `0x4830CB`->`0x4830DA`; RHS parser `0x4854D4`; runtime comparison block `0x48864F`; docs `trigger.html:3153` | HIGH |
| `NumPartner` | value trigger | `0x5F` | number of partners; neutral players/helpers excluded | name/opcode emit `0x483674`->`0x483683`; docs `trigger.html:2151` | HIGH docs/emit; MED count path |
| `Matchno` | value trigger | `0x74` | 1 in versus; increments per match in Arcade/Team Arcade | emit `0x483074`->`0x483083`; docs `trigger.html:1876` | HIGH |
| `Roundsexisted` | value trigger | `0x75` | rounds the player has existed for; first round 0 | emit `0x483091`->`0x4830A0`; docs `trigger.html:2872` | HIGH |
| `TeamSide` | value trigger | `0x78` | 1 = left side, 2 = right side | emit `0x4830AE`->`0x4830BD`; docs `trigger.html:3185` | HIGH |
| `IsHomeTeam` | value trigger | `0x7A` | 1 if the player's side is the home team; in versus, P1/left; in arcade, the computer | name string `0x4A4C78`; docs `trigger.html:1742` | HIGH docs; MED emit |
| `standby` | value trigger | `0xCD` | not documented; inferred as "this member is not the active one" | name string `0x4A4F44`; emit `0x4839AF` | LOW |
| `partner` | redirect | none | resolves to the player's partner; invalid destination yields `bottom` (condition 0) | redirect list `cns.html:1616-1690`; accessor `0x464990` (FINDINGS.md) | HIGH docs; MED binary |
| `TagIn` | controller | `0x227` | `stateno`, `partnerstateno`, common `ctrl`; changes self and partner state; clears partner latch | dispatch `0x4740FB`/`0x474133`; executor `0x47AEF7` | MED existence; LOW gate/consumer |
| `TagOut` | controller | `0x228` | common `ctrl`; sets self latch `+0x22B4`; the switch itself is external | dispatch `0x4741AA`/`0x4741DC`; executor `0x47B0BC` | MED existence; LOW semantics |
| `assist` | absent | none | no 1.0 trigger or controller | absent from resolved strings, compiler, docs, corpus | HIGH |

### 3.2 `TeamMode` encoding, exactly

The trigger compiler scans the right-hand side for the first `=` and checks the
byte before it: `!` sets the negation flag, everything else is an equality.
That is why only `=` and `!=` are legal; there is no `<`, `>`, or arithmetic
path. The mode token is trimmed and lowercased, then matched:

| keyword | compiled value | evidence |
|---|---:|---|
| `single` | 0 | string `0x4A52F0`; store `0x4855DC` |
| `simul` | 1 | string `0x4A52F8`; store `0x48562C` |
| `turns` | 3 | string `0x4A5300`; store `0x485680` |

An unknown token reports `Invalid TeamMode %s in %s` (`0x4A5308`, print at
`0x4856A1`). The runtime block at `0x48864F` includes `0x79` among the triggers
whose node operand is compared as an integer, and the operator jump table at
`0x488668` implements equality/inequality. The result is boolean, so this is
**not** a general numeric trigger that can be used bare.

Docs-only detail: in Survival, `TeamMode = turns` is true on the enemy side
(`trigger.html:3153`).

### 3.3 `partner` and `NumPartner`

`NumPartner` counts only partners, not normal helpers and not neutral players.
Its documented use is exactly the team query:

```
trigger1 = NumPartner = 1
trigger1 = partner, life < 200
```

`partner` is a redirect. A side with no partner has no valid destination, so the
redirect yields `bottom`; condition contexts treat `bottom` as 0
(`cns.html:1616-1690`). This matches anifight returning null for the redirect:
in single mode that is the correct answer. It is only a missing feature once a
side actually has more than one member.

### 3.4 `standby` and the tag latch

`standby` compiles to `0xCD` but has no 1.0 documentation and no corpus use.
The semantic below is an inference from the tag-style fields, not a trace:
standby would be true for a team member that is not currently active. The only
latch-like field located is player **+0x22B4**, set by TagOut and cleared by
TagIn on the partner. No other reader of that field was located; a team layer
would have to consume it. Mark both **LOW** until a
tag-capable trace or play test exists.

### 3.5 `assist` is not a 1.0 surface

`assist` is absent from the resolved string table and from the trigger compiler
chain. The corpus has zero uses. Characters written for engines that do have an
assist mechanic cannot be detected here; there is nothing to implement.

## 4. anifight today: parsed, consumed, inert

### 4.1 Audit

| surface | status | file:line | note |
|---|---|---|---|
| `numpartner` | answered | `statemachine.cpp:6281` | always `0`; correct only while every match is single |
| `standby` | answered | `statemachine.cpp:6282` | always false; no team state exists |
| `ishometeam` | answered | `statemachine.cpp:6283` | always false; no home/away side model |
| `teammode` | answered, wrong type | `statemachine.cpp:6284` | returns the text `"single"`, but the trigger is comparison-only |
| `matchno` | answered | `statemachine.cpp:6285` | always 1; fine for versus, no arcade increment |
| `roundsexisted` | consumed | `statemachine.cpp:6287` | `roundNo_ - 1`; correct for single, not per-member for Turns |
| `partner` redirect | stubbed | `statemachine.cpp:6072-6073` | always null; correct in single, no partner entity exists |
| `TagIn`/`TagOut` | unimplemented | `statemachine.cpp:2270-2274` | unknown controller types are logged once as "not implemented" |
| team selection | absent | `select_screen.h:78`, `select_screen.cpp:240,264` | only `p1Cell_`; `launch(p1Cell_, ...)` creates one character per side |
| member storage | absent | `fight_screen.h:210-213` | fixed `characters_[2]`, `fighters_[2]`, `roundsWon_[2]` |
| team round logic | absent | `fight_screen.cpp:351-365` | `endRound` increments one of two `roundsWon_` counters; no relay |
| `team.maxmatches` | parsed, inert | `defs.cpp:657-663`, `:676-678`; `defs.h:388-390` | logged as "not implemented"; no reader |
| `select.def` `order` / `music` / `includestage` | not parsed | `defs.cpp:626-640` | only `hidden` is read |
| `system.def` team menu | not parsed | `motif.cpp`, `motif.h` (no `teammenu` or team keys) | the select screen knows only the roster grid |
| `[Simul/Turns Lifebar/Face/Name]` | not parsed | no `Simul`/`Turns` handling in `app/src/main/cpp` | HUD is code-drawn; `fight_screen.cpp:981-987` says the `[LifeBar]`/`[PowerBar]` path is no longer called |
| model comment | acknowledged | `statemachine.h:22-23` | team modes are "the one population this model does not describe" |

### 4.2 The concrete `TeamMode` bug

> **Fixed 2026-10-19.** `teammode` now answers the single-mode code 0, and the
> expression parser compiles the reserved right-hand keywords `single`, `simul`
> and `turns` to 0, 1 and 3. `teammode = single` is true and `teammode != simul`
> is true in this single-only engine; `= simul` and `= turns` are false. Host
> tests pin all three forms. The analysis below records the state as found.

`teammode` returns `Value::text("single")` at `statemachine.cpp:6284`. The
expression parser has no special case for the TeamMode right-hand side: a bare
identifier becomes a `Trigger` node (`expr.cpp:790-792`). At evaluation, the
unknown-trigger fallback treats identifiers of at most four lowercase letters
as string constants and everything longer as numeric zero
(`statemachine.cpp:6655-6669`). Therefore:

- `single` (6 letters), `simul` (5), and `turns` (5) all evaluate to **0**.
- A string compared with a number falls back to numeric comparison
  (`expr.cpp:900-912`), and any non-empty string converts to **1**
  (`expr.h:47`).

Net results with the current engine:

| expression | current result | correct result in single |
|---|---|---|
| `teammode = single` | **false** (1 = 0) | true |
| `teammode != simul` | **true** (1 != 0) | true |
| `teammode = simul` | false | false |
| `teammode = turns` | false | false |

So four characters that gate states on `teammode = single` currently have those
states suppressed, while the three characters using `teammode != simul` happen
to be correct in single mode. Changing the return value alone will not fix this;
TeamMode must be recognized in the expression parser and its right-hand keyword
compiled to the 0/1/3 mode code, with `=`/`!=` only.

### 4.3 Consequences

Team triggers in `state -1` intros and win/lose states are common; the four
`= single` characters are the immediate correctness loss. Team modes are not
needed to *load* any of the 41 corpus characters: all are playable in single.
The gap is team play, team HUD, and correct gating, not character loading.

## 5. Implementation order

Ranked by corpus reach and by how much of the engine each behavior forces open.

| order | behavior | corpus reach | engine surfaces needed |
|---:|---|---|---|
| 1 | Correct `TeamMode` keyword semantics with single as the default | 45 comparisons in 13 characters; **4 characters wrong today** (`= single`) | `expr.cpp` parser special case; `statemachine.cpp` trigger/compare; tests for `= single`, `= simul`, `= turns`, `!= simul` |
| 2 | Simul mode | 2 characters `= simul`, 3 `!= simul`; 4 use `partner,` (177 uses); 3 use `NumPartner` (104 uses); 5 mention `simul` | `fight_screen.h` fixed `[2]` member arrays -> per-side lists; entity population; `StateMachine::redirect("partner")`; `numpartner`; teamside/ishometeam; `[Simul Lifebar/Face/Name]`; `Team.1VS2Life` / `Team.LoseOnKO`; select-screen partner pick |
| 3 | Turns mode | 6 characters `= turns`; the 4 `partner,` characters also matter; `roundsexisted` per member | select-screen team count and order; member list and active index; KO handoff; per-member round/entry counters; standby member records; `[Turns Lifebar/Face/Name]`; order rotation |
| 4 | Tag mode | **0 corpus uses**; not a 1.0 mode | `TagIn`/`TagOut` in `runController`; `standby`; `+0x22B4` latch and cooldown; switch input; active-index swap. Only worth doing for later-engine compatibility, not for this corpus |
| 5 | `assist`, `matchno` | 0 corpus uses | `assist` does not exist in 1.0; `matchno` is already 1 and only needs arcade/team-arcade match counters if those modes are added |

`roundsexisted` is already answered (`statemachine.cpp:6287`) and is used by 30
of 41 characters. It is correct for single; the Turns work in rank 3 is what
would make its per-member meaning matter.

## 6. Evidence map

| address / location | what it proves |
|---|---|
| `0x482B00` region | trigger-name compiler; the string-to-opcode chain |
| `0x483074` -> `0x483083` | `matchno` -> `0x74` |
| `0x483091` -> `0x4830A0` | `roundsexisted` -> `0x75` |
| `0x4830AE` -> `0x4830BD` | `teamside` -> `0x78` |
| `0x4830CB` -> `0x4830DA` | `teammode` -> `0x79` |
| `0x483674` -> `0x483683` | `numpartner` -> `0x5F` |
| `0x4839A0` -> `0x4839AF` | `standby` -> `0xCD` |
| `0x4854D4` | `TeamMode` right-hand-side compiler: operator scan, lowercasing, keyword match |
| `0x4855DC`, `0x48562C`, `0x485680` | mode values 0, 1, 3 for single, simul, turns |
| `0x4A5308` / `0x4856A1` | `Invalid TeamMode %s in %s` |
| `0x48864F`, `0x488668` | runtime integer comparison block that includes `0x79` |
| `0x473ED0` region | controller-name dispatch |
| `0x4740FB`, `0x474133` | `tagin`/type `0x227`, params `stateno`, `partnerstateno`, `ctrl` |
| `0x4741AA`, `0x4741DC` | `tagout`/type `0x228`, common `ctrl` |
| `0x47AEF7`, `0x47AF72` | TagIn executor; changes states; clears partner `+0x22B4` |
| `0x47B0BC` | TagOut executor; sets self `+0x22B4 = 1` |
| `0x464990` | `partner` redirect accessor (per `FINDINGS.md`) |
| `local_mugen/.../readme.txt:206-217`, `:237-252` | mode list, member counts, rounds, life scaling, Turns order switching |
| `local_mugen/.../docs/trigger.html:1742,1876,2151,2872,3153,3185` | IsHomeTeam, Matchno, NumPartner, Roundsexisted, TeamMode, TeamSide |
| `local_mugen/.../docs/cns.html:1616-1690` | redirect list, `partner`, `bottom` |
| `local_mugen/.../data/system.def:213-256` | team-menu keys |
| `local_mugen/.../data/fight.def:70,121,14220,14259,14307,14324` | Simul/Turns HUD sections |
| `local_mugen/.../data/select.def:157-168` | `[Options]`, `arcade.maxmatches`, `team.maxmatches` |
| `local_mugen/.../data/mugen.cfg:16-17,386-388` | team-only config |
| `local_chars/` | 41 characters; usage counts in §1.4 |

### What this note does not prove

- Where the runtime reads the current team mode from (player/team struct offset):
  only the comparison block and the compiled mode operand are pinned.
- The exact `NumPartner` counting loop: docs and opcode emit are pinned; the
  entity scan was not traced.
- `standby` semantics: no docs; inferred only.
- Tag mode flow, switch input, cooldown, and the consumer of `+0x22B4`: no mode
  is reachable in 1.0, so these remain LOW/UNRESOLVED.
- Whether Turns shares power or gives each member its own meter: the readme does
  not say, and the team struct was not traced.

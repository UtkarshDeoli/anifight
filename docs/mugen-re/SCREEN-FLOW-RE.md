# Screenpack flow RE — screens, match state, and what the engine is missing

Reference: `mugen.exe` (MUGEN 1.0, PE32 i386, image base `0x400000`), read from
`/tmp/mugen_peek/mugen/mugen.exe` and its objdump text
`/tmp/mugen_peek/mugen_disasm.txt`. All addresses are VAs unless marked
otherwise. The system struct base is the global at `0x52b40c`; offsets below are
from that base.

Two other sources are used, both read read-only and never copied into the repo:

- the MUGEN 1.0 distribution's shipped documentation
  (`local_mugen/extracted/mugen/docs/`: `trigger.html`, `storyboard.html`,
  `air.html`, `cns.html`, `sctrls.html`, `history.html`, `readme.txt`);
- the motifs shipped with the distribution (`data/system.def`,
  `data/big/system.def`, `data/mugen1/system.def`, `data/kfm/system.def`).

The shipped docs do **not** document `system.def` keys. Key names, defaults and
section names below therefore come from the corpus motifs and the binary's
loader; comments embedded in the corpus `.def` files are treated as
distribution-shipped behaviour notes, not as engine proof. Where the binary and
a comment disagree, the binary wins.

Scope: the screenpack-driven flow after the title screen — victory, win,
continue, game over, options, and the match state those screens read. Title,
select and VS are covered because they are the part of the flow the engine
already implements and the rest must fit around them.

## 0. Answers in one screen

| question | answer | confidence |
|---|---|---|
| What are the sections actually called? | `[Title Info]`, `[Select Info]`, `[VS Screen]`, `[Victory Screen]`, `[Win Screen]`, `[Continue Screen]`, `[Game Over Screen]`, `[Option Info]`, plus `[Demo Mode]`, `[Survival Results Screen]`, `[Default Ending]`, `[End Credits]`, `[Music]`. There is no `[Title Screen]`, `[Select Screen]`, `[Versus Screen]`, `[Options Screen]` or `[Game Over Screen]`-style alternate spelling in MUGEN 1.0. | HIGH (loader names at `0x4a5d5c` … `0x4a6274`) |
| Where is each screen shown in arcade? | Before a match: VS. After a won non-final match: Victory. After a lost match: Continue. Declining/timeout: Game Over. After the final match: Win, then optional Default Ending, then optional End Credits. | HIGH for order, MEDIUM for final-match gating |
| Victory vs Win — what is the difference? | Victory is **per won match** and carries the winner's portrait, name and win quote; the default motif says it is skipped on the last match. Win is **once per cleared run**, just a text banner over a pose, and is skipped if the winning character has an ending. | HIGH (corpus comments + separate loader records and routines) |
| Does a continue have a configurable countdown or a credit counter? | No key for either was found. `[Continue Screen]` exposes only `enabled` (plus an undocumented `storyboard` string in the loader). No `credits` trigger, variable or config key exists in MUGEN 1.0. The countdown duration is engine-internal and was not resolved. | HIGH for absence of keys; UNRESOLVED for duration |
| Are `credits`, `continue`, `gameover`, `roundsremaining` triggers? | No. The trigger-name table `0x4a4b10–0x4a4e90` and the compiler chain `0x482fdf–0x4830c0` contain none of them. The game-over screen is selected from `system.def`, not from a trigger. | HIGH |
| What match triggers actually exist? | `drawgame`, `matchover`, `gametime`, `roundno`, `roundstate`, `matchno`, `roundsexisted`, then `teamside`, `teammode`, `ishometeam`. There is no `roundsremaining`. | HIGH (compiler opcodes `0x6f`–`0x7a`) |
| What does the engine parse today? | Every section in this document's title, plus `[Music]`, parses into typed records; the four shipped motifs were checked by hand. | HIGH (`defs.cpp`, `defs.h`) |
| What does the engine draw today? | Title, select, VS, Victory, Win, Continue, and the Game Over / Default Ending / End Credits storyboards, all with their backgrounds; Options is an anifight layout editor. | HIGH (screen classes listed in §4) |
| What is the smallest useful order? | (1) parse the remaining sections and BGdefs inertly, (2) add explicit match/run state and fix the match triggers, (3) Victory screen, (4) Win/Survival results, (5) Continue, (6) game over/endings/credits via a storyboard player, (7) option info/background, (8) music. | MEDIUM (engineering estimate, not a binary fact) |

## 1. Section inventory and loader evidence

`system.def` is loaded by one routine spanning roughly `0x48c201–0x48cf4b`
(missing-section paths jump to a logger around `0x48d200`).
Each section name string and the address that loads it:

| section | name string | loader | record base / notes |
|---|---|---|---|
| `[Title Info]` | `0x4a5d5c` | `0x48c2a2` | base `+0x1828` |
| `[Option Info]` | `0x4a5f2c` | `0x48c5e2` | base `+0x1a10`; sounds at `+0x08/+0x10/+0x18`, title string at `+0x20` |
| `[Select Info]` | `0x4a5f7c` | `0x48c67f` | delegates to the select parser at `0x440330` |
| `[VS Screen]` | `0x4a5fc4` | `0x48c6dd` | base `+0x4238` |
| `[Victory Screen]` | `0x4a6034` | `0x48c91f` | base `+0x447c` |
| `[Demo Mode]` | `0x4a6068` | `0x48ca7e` | fields `+0x1166c`…`+0x1168c` |
| `[Continue Screen]` | `0x4a6124` | `0x48cbd0` | base `+0x3b08` |
| `[Game Over Screen]` | `0x4a617c` | `0x48ccc4` | inside the continue record: enabled `+0x52c`, storyboard `+0x530` |
| `[Win Screen]` | `0x4a61b0` | `0x48cd2b` | enabled `+0x39a4`, wintext `+0x39a8`, fadein `+0x3a4c`, pose.time `+0x3a48`, fadeout `+0x3a50` |
| `[Survival Results Screen]` | `0x4a61e8` | `0x48cdd2` | base `+0x3a54` |
| `[Default Ending]` | `0x4a6248` | `0x48ce86` | at `0x359c+0x204`: enabled `+0x37a0`, storyboard `+0x37a4` |
| `[End Credits]` | `0x4a6274` | `0x48ceed` | at `+0x359c`: enabled `+0x359c`, storyboard `+0x35a0` |

Note on the last row: the loader uses base `+0x359c`, reads `enabled` at `+0`
and copies a `0x200`-byte `storyboard` string to `+0x4` (`0x48cf2c`). The
adjacent records are packed: End Credits at `+0x359c`, Default Ending at
`+0x37a0`, Win at `+0x39a4`, each `0x204` apart with a `0x200` path buffer.

`[Music]` is not loaded by this routine; it is read elsewhere. Its keys
(`title.bgm`, `select.bgm`, `vs.bgm`, `victory.bgm`, each with `.volume`,
`.loop`, `.loopstart`, `.loopend`) are present in every corpus motif.

## 2. The screen sections

Drawable elements all use the same grammar in this engine's model
(`DisplayElement`, `defs.h:39–63`): one of `anim`, `spr` or `font`; plus
`offset`, `scale`, `facing`, `layerno`, `displaytime`, `alpha`, `window`,
`trans`. The tables below therefore give the section-specific keys, not the
common modifiers.

### 2.1 `[Title Info]`

The screen shown at boot after the intro. Fade in, then the mode menu; choosing
a mode leaves the screen. Per the corpus comment, if every `menu.itemname.*` is
empty the motif "goes straight to demo mode".

| key | type / default | notes |
|---|---|---|
| `fadein.time` | int; motif `10`; loader clamps to `>= 1` (`0x48c42b`) | engine draws this (`title_screen.cpp:190–191`) |
| `fadeout.time` | int; motif `10`; loader clamps to `>= 1` (`0x48c43b`) | parsed by the engine but never used |
| `menu.pos` | pair; binary default `159,158` (`0x48c2e8`) | |
| `menu.item.font`, `menu.item.active.font` | `font,bank,align` triples | |
| `menu.item.spacing` | pair; default y `13` (`0x48c2f4`) | |
| `menu.window.margins.y` | pair; binary default `12,8` (`0x48c2fb`) | |
| `menu.window.visibleitems` | int; binary default `5` (`0x48c30f`) | |
| `menu.boxcursor.visible` | bool; **binary default `1`** (`0x48c319`) | engine struct defaults false |
| `menu.boxcursor.coords` | rect; binary default `-32,-10,32,2` (`0x48c323–0x48c341`) | the default motif overrides to `-58,-10,57,2` |
| `menu.itemname.<mode>` | quoted string | 11 stable keys: arcade, versus, teamarcade, teamversus, teamcoop, survival, survivalcoop, training, watch, options, exit. `""` disables the item. |
| `cursor.move.snd`, `cursor.done.snd`, `cancel.snd` | `group,index` | parsed by the binary; **not parsed by anifight** |

### 2.2 `[Select Info]`

The character/stage grid. Keys fall into five groups. Values below are the
default motif's; the binary's select parser is the separate function
`0x440330`, so compiled defaults were not enumerated key by key.

| group | keys | default-motif values |
|---|---|---|
| Fades / layout | `fadein.time`, `fadeout.time`, `rows`, `columns`, `wrapping`, `pos`, `showemptyboxes`, `moveoveremptyboxes` | 10, 10, 2, 5, 0, `90,170`, 0, 0 |
| Cells | `cell.size`, `cell.spacing`, `cell.bg.spr`, `cell.random.spr`, `cell.random.switchtime` | `27,27`, 2, `150,0`, `151,0`, 4 |
| Cursors | `p1.cursor.startcell`, `p2.cursor.startcell`, `p1.cursor.active.*`, `p1.cursor.done.*`, `p2.cursor.active.*`, `p2.cursor.done.*` | `0,0` / `0,4`; sprite triples in the motif |
| Portraits / labels | `portrait.spr`, `portrait.offset`, `portrait.scale`, `title.offset`, `title.font`, `stage.pos`, `stage.active.*`, `stage.active2.*`, `stage.done.*`, `p1.face.*`, `p2.face.*`, `p1.name.*`, `p2.name.*` | portrait `9000,0`; title `159,13`; stage `160,237` |
| Sounds / team menu | `cursor.move.snd`, `cursor.done.snd`, `cancel.snd`, `blink`, `random.move.snd.cancel`, `p1.teammenu.*`, `p2.teammenu.*` | full team-menu block in the motif; anifight parses none of these |

The select screen leaves to the fight screen; in anifight the same handoff is
at `select_screen.cpp` (roster selection) into `FightScreen`.

**Anifight extension (2026-10-19): stage select.** MUGEN takes a match's stage
from the `stage` list beside the character in select.def, falling back to
`[ExtraStages]`; it has no stage-selection screen of its own. This game's
roster lines name no stage, so every match used to land in the first extra
stage. `StageSelectScreen` now sits between the character grid and the fight:
it lists every loaded stage by `[Info] displayname` (defaulting to `name`, as
`bgs.html` documents), `A`/Start confirms, and `B` backs out to the grid. The
choice is handed to `FightScreen` as an explicit stage and is kept for the
whole arcade run. The screen replaces itself with the fight rather than
stacking another screen, so the stack stays title -> select -> fight and the
fight's return-to-title pops still land on the title.

### 2.3 `[VS Screen]`

Shown before a match, with the two big portraits (`group 9000, image 1` of each
character's own sprite file). The cursor or the timer ends it.

| key | default | evidence |
|---|---|---|
| `time` | **150** ticks (`0x96` stored at `0x48c70a`) | hold time |
| `fadein.time` | motif `15` | read at `0x48c723` |
| `fadeout.time` | motif `15` | read at `0x48c736` |
| `match.text` | `"Match %i"` | arcade match number; `%i` |
| `match.offset`, `match.font` | motif `159,12`, font `3,0,0` | |
| `p1.spr`, `p2.spr` | `9000,1` | per-side record at `+0x40`, stride `0xcc` |
| `p1.offset` / `p2.offset` | `20,31` / `299,31` | |
| `p1.facing` / `p2.facing` | `1` / `-1` | |
| `p1.scale` / `p2.scale` | `1,1` | |
| `p1.name.offset`, `p1.name.font`, `p2.name.*` | `78,190` / `241,190` | loader keys are `p%i.name.offset`, `p%i.name.pos`, `p%i.name.font`, `p%i.name.spacing` |

No sounds are defined for the VS screen in the corpus motifs.

### 2.4 `[Victory Screen]` and `[VictoryBGdef]`

Shown after a won **match** in arcade. Corpus comment: "It will not be displayed
on the last match of arcade mode." The winner's portrait is `group 9000, image
2`; the win quote comes from the character's `[Quotes]` (see §3.4), with
`winquote.text` as the fallback.

| key | default | notes |
|---|---|---|
| `enabled` | motif `1` | gate tested at `0x441e40` (`cmp` against `+0x447c`), early-out `0x44273c` |
| `time` | binary pre-store **150**, motif `300` | overwritten by the motif |
| `fadein.time` | motif `8` | |
| `fadeout.time` | motif `15` | |
| `p1.offset` | motif `100,20` | |
| `p1.spr` | `9000,2` | |
| `p1.facing`, `p1.scale` | `1`, `1,1` | |
| `p1.window` | `0,0,319,160` | clipping rectangle |
| `p1.name.offset`, `p1.name.font` | `20,180`, `3,0,1` | right-aligned |
| `winquote.text` | `"Winner!"` | fallback only |
| `winquote.offset`, `winquote.font` | `20,192`, `2,0,1` | |
| `winquote.window` | `18,171,301,228` | |
| `winquote.textwrap` | `w` | word wrap |

`[VictoryBGdef]` and `[VictoryBG 1..3]` use the ordinary background grammar.
Layers 1 and 2 are tiled, layer 2 uses `trans = sub`, and both text-area layers
carry `window` rectangles.

### 2.5 `[Win Screen]`

Shown once after **winning arcade**, not per match. Corpus comment: "It will not
be displayed if the winning character has an ending." Binary gate at
`0x4562b4` (`cmp` against `+0x39a4`), routine `0x4564f0`.

| key | default | notes |
|---|---|---|
| `enabled` | motif `1` | |
| `wintext.text` | `"Congratulations!"` | |
| `wintext.font` | `2,0,0` | |
| `wintext.offset` | `159,70` | |
| `wintext.displaytime` | `-1` | persistent |
| `wintext.layerno` | `2` | drawn above the pose |
| `fadein.time` | `32` | |
| `pose.time` | `300` | how long the win pose holds |
| `fadeout.time` | `64` | |

`0x4564f0` seeds the RNG (`call 0x4598e0`), sets the game-speed globals
`0x52aec0`/`0x52b848` to `60`, and reads the wintext geometry at `+0x1b90` and
`+0x1b94`.

### 2.6 `[Continue Screen]`

Shown after the human loses a match in arcade, if enabled. Corpus `big` motif
carries the full text block; the default motif carries only `enabled`.

| key | default | notes |
|---|---|---|
| `enabled` | motif `1`; binary checked at `0x456152` (`+0x3b08`) | |
| `storyboard` | empty | **present in the loader** (`0x200` string at `+0x04`, key string `0x49f250`) but absent from the docs and the default motif; semantics unproven |
| `pos` | binary default **`159,40`** (`0x48cc33`) | |
| `continue.text`, `.font`, `.offset` | big motif `"CONTINUE?"`, font `1,0,0`, `0,0` | element at `+0x20c` |
| `yes.text`, `yes.font`, `yes.offset` | `"YES"`, `1,0,0`, `-80,60` | element at `+0x2ac` |
| `yes.active.*` | active variant; big motif font `1,3,0` | element at `+0x34c` |
| `no.*`, `no.active.*` | `"NO"` at `80,60`; active variant | elements at `+0x3ec` / `+0x48c` |

Reader order in the loader is `continue`, `yes`, `no`, `yes.active`,
`no.active`. The `big` motif's comment says the display elements "use the
fight.def resources instead of the ones defined in this file"; that is a
distribution note, not verified in the binary.

The UI routine is `0x44f470`. It reads `pos`, draws the five text blocks
(`0x44fd9d–0x44ff9c`), handles input, and returns non-zero when the player
accepted a continue. The post-match caller `0x456165` then clears the pause flag
and resumes the fight loop; it does not advance the match number.

### 2.7 `[Game Over Screen]`

Shown when the run is lost and no continue was accepted. Corpus comments and
the storyboard guide agree: choosing "No" at the continue screen plays the game
over cutscene and returns to the title.

| key | default | notes |
|---|---|---|
| `enabled` | default motif `0`; big/kfm `1` | binary check `0x456196` (`+0x4034`) |
| `storyboard` | empty; kfm uses `gameover.def` | `0x200` path at `+0x4038`, copied by `0x4561a2–0x4561bc` |

There is no timer, text or background key: the section is entirely a storyboard
switch. The game over screen is **not** a trigger-driven screen.

### 2.8 `[Option Info]` and `[OptionBGdef]`

The options menu's screen definition. The default motif defines only three
sounds; the rest of the options UI is engine-built.

| key | default |
|---|---|
| `cursor.move.snd` | `100,0` |
| `cursor.done.snd` | `100,1` |
| `cancel.snd` | `100,2` |

`[OptionBGdef]`/`[OptionBG n]` use the ordinary background grammar. Anifight
currently routes the title's OPTIONS item to its own `LayoutScreen`
(`title_screen.cpp:108`) and parses none of this.

### 2.9 Adjacent flow sections

These do not draw a screen of their own but decide what follows the flow.

| section | keys | role |
|---|---|---|
| `[Demo Mode]` | `enabled`, `select.enabled`, `vsscreen.enabled`, `title.waittime`, `fight.endtime`, `fight.playbgm`, `fight.stopbgm`, `fight.bars.display`, `intro.waitcycles`, `debuginfo` | attract mode; fields at `+0x1166c`…`+0x1168c` |
| `[Survival Results Screen]` | `enabled`, `winstext.text` (`"Rounds survived: %i"`), `.font`, `.offset`, `.displaytime`, `.layerno`, `fadein.time`, `show.time`, `fadeout.time`, `roundstowin` | shown instead of Win for survival; routine `0x449880`, state globals `0x117d0`/`0x117d4`, compares counter with `show.time` at `+0xa4`, `roundstowin` at `+0xb0` |
| `[Default Ending]` | `enabled`, `storyboard` | after Win, only if the character has no ending |
| `[End Credits]` | `enabled`, `storyboard` | after Win/Ending |
| `[Music]` | `title.bgm`, `select.bgm`, `vs.bgm`, `victory.bgm`; each `.volume`, `.loop`, `.loopstart`, `.loopend` | per-screen BGM |

## 3. Round, match and run flow

### 3.1 The state the flow reads

The binary keeps the flow state in the global system struct and in per-fight
fields, not in the screen section records:

| field | address / offset | meaning | evidence |
|---|---|---|---|
| winner side | `+0x4a14` | `0` when undecided/draw, otherwise a 1-based side | `0x4560e6` |
| human/player side | `+0x117e0` | compared with the winner | `0x4560f4` |
| arcade/run gate | `+0x117e8` | non-zero selects the arcade post-match path | `0x4560ce`, `0x456313` |
| pause / menu flag | `+0x809c` | must be `0` for post-match screens | `0x4560da`, `0x45631f` |
| match/pause gate | `+0x12044` | must be `0` for the Victory routine call | `0x455c2c` |
| roster counts | `+0x1e74`, `+0x1e78` | used to decide whether the run continues / has an ending | `0x4561c1–0x4561d4`, `0x455c12–0x455c1e` |
| selected-character "has ending" byte | character record `+0xc60` | non-zero takes the character-ending branch instead of Win/Default Ending | `0x456238–0x456244` |

`0x442c30` returns the count of non-`-1` entries in the `+0x1e70` array; the
post-match code uses it with `+0x1e74`/`+0x1e78` to distinguish "more
opponents" from "run finished".

### 3.2 Order of screens

Doc evidence and binary evidence agree on the following arcade sequence. "Doc"
means the shipped documentation or a corpus comment; "bin" means a gate or
routine address.

| step | screen / action | source |
|---|---|---|
| 1 | title → select (or demo mode when all menu items are disabled) | doc: title comment; engine today: title pushes select |
| 2 | VS screen before the match | doc: `[VS Screen]`; bin: `0x48c6dd` |
| 3 | rounds run per `fight.def [Round]` (`match.wins`, round/fight/KO times) | doc + engine `RoundSection` |
| 4 | match decided; `matchover` becomes true once the winner starts the win pose (state 180) | doc: `trigger.html:1876–1912` |
| 5a | human **won**: Victory Screen (non-final match) → next match | doc: Victory comment; bin: `0x455c34`, routine `0x441e40` |
| 5b | human **lost**: Continue Screen, if enabled | bin: `0x456152`, routine `0x44f470` |
| 6a | continue accepted → resume the fight loop, same match number | doc: `matchno` "does not increment on continue"; bin: caller `0x456165–0x456191` |
| 6b | continue declined or expired → Game Over Screen (if enabled), then title | doc: `storyboard.html:247`; bin: `0x456196–0x4561bc` |
| 7 | final match won (arcade cleared): Win Screen, if enabled and the character has no ending | doc: Win comment; bin: `0x4562b4` / `0x4564f0` |
| 8 | Default Ending storyboard, if enabled and no character ending | doc: Default Ending comment; bin: `0x4562c7–0x4562e5` |
| 9 | End Credits storyboard, if enabled | doc: End Credits comment; bin: `0x4562eb–0x45630b` |
| — | survival: Survival Results Screen replaces Win | doc + bin: `0x4563af` → `0x456900` |

Two details are worth calling out:

- **Victory is per match, Win is per run.** They are separate records
  (`+0x447c` vs `+0x39a4`), separate routines (`0x441e40` vs `0x4564f0`) and
  separate loader sections. A screenpack can enable one and not the other.
- **The "last match" and "has ending" rules live in corpus comments and in the
  win-path gate.** The Victory routine's own early-out was located
  (`0x44273c`), but the exact test that suppresses Victory on the final match
  was not fully traced; the roster-count/ending checks at `0x4561c1` and
  `0x456244` are the best evidence.

### 3.3 Continue countdown and credits

What is proven:

- `[Continue Screen] enabled` gates the whole continue path (`0x456152`).
- The continue UI routine is `0x44f470`; it draws the five configured elements,
  takes yes/no input, and returns non-zero on acceptance.
- Accepting returns to the fight loop without incrementing the match number.
- Declining leads to the Game Over gate (`0x456196`).
- There is **no `credits` variable, trigger, or `system.def` key** in the
  trigger table, compiler chain, or system loader. No credit counter exists to
  decrement; an enabled `[Continue Screen]` therefore appears to allow
  unlimited continues.

What is not resolved:

- The countdown duration. The continue routine contains no immediate compare
  against a plausible duration; it seeds the engine's 60-ticks-per-second
  globals (`0x52aec0`, `0x52b848`) like every other per-second timer and then
  runs an internal loop. No `system.def` key exposes the length.
- Whether a timeout and a "No" choice take exactly the same path. Both reach
  the game-over gate; the intermediate flag was not named.
- What "resume" means for round state (restart the round, restore life, or
  continue from the current round). The docs only guarantee that `matchno` does
  not increase.
- The undocumented `[Continue Screen] storyboard` key: it loads, but its
  consumers were not located.

### 3.4 Win quotes

`VictoryQuote` is the character-side controller (`sctrls.html:3152–3171`): only
the winning player's quote can be set, it affects only the next victory screen,
and quotes live in the character's `[Quotes]` section. This is the character
surface the Victory screen must consume; `winquote.text` is the screenpack
fallback when the character has no quote.

### 3.5 Match triggers

The binary compiles trigger names to opcodes in `0x482fdf–0x4830c0`; the name
strings are at `0x4a4b10–0x4a4e90`.

| trigger | opcode | semantics | engine today |
|---|---|---|---|
| `drawgame` | `0x6f` | match ended in a draw | implemented in fight flow |
| `matchover` | `0x70` | true once the match is decided; docs: "does not return true until the players start their win poses (state 180)" | **landed 2026-10-19**; driven by `FightScreen` from `matchDecided_` plus the `MatchOver` phase (this engine has no forced state-180 win pose) |
| `gametime` | `0x71` | ticks since round start | implemented |
| `roundno` | `0x72` | current round number | implemented (`statemachine.cpp:6238`) |
| `roundstate` | `0x73` | docs: `0` pre-intro, `1` intro, `2` fight, `3` pre-over, `4` over | engine maps its phases to `1`–`4` (`fight_screen.cpp:400–413`) and returns the value at `statemachine.cpp:6229` |
| `matchno` | `0x74` | versus always `1`; arcade starts at `1`, increments per new match, not on continue, resets on new game | **landed 2026-10-19**; 1 for versus/engine test, `ladderStep_ + 1` for arcade (a loss does not advance it) |
| `roundsexisted` | `0x75` | number of completed rounds | computed as `roundNo_ - 1` (`statemachine.cpp:6287`) |
| `teamside`, `teammode`, `ishometeam` | `0x78`–`0x7a` | team metadata | see team-mode RE |
| `roundsremaining` | — | **not a MUGEN 1.0 trigger** | stub returns 0 (`statemachine.cpp:6288`), harmless but misleading |
| `credits`, `continue`, `gameover` | — | **not triggers** | no handling |

`matchover` and `matchno` are both **landed** (2026-10-19): `FightScreen`
drives `matchover` from the decided flag plus the post-match phases, and
`matchno` is 1 for versus/engine test and `ladderStep_ + 1` for arcade, unchanged
by an accepted continue.

## 4. What anifight parses and draws today

Status legend: **drawn** = parsed and rendered; **parsed** = in `SystemDef` but
not consumed; **inert** = not parsed at all.

| system.def section | status | evidence |
|---|---|---|
| `[Title Info]` | drawn; `fadeout.time` parsed only; sounds not parsed | parse `defs.cpp:388–411`; draw `title_screen.cpp:70,190–191` |
| `[TitleBGdef]` / `[TitleBG n]` | drawn | `defs.cpp:462`, `motif.cpp:215–227` |
| `[Select Info]` | drawn; sounds and `p1/p2.teammenu.*` inert | parse `defs.cpp:413–448`; draw `select_screen.cpp:34,120,178,396` |
| Stage list (`[Info] displayname`) | **drawn** (2026-10-19): the stage select screen lists every loaded stage by display name, falling back to `name` | `stage.cpp` (`parseStageDef`), `screens/stage_select_screen.*` |
| `[SelectBGdef]` / `[SelectBG n]` | drawn | `defs.cpp:463` |
| `[VS Screen]` | drawn; `time`, fades, match text, portraits, names | parse `defs.cpp:450–460`; draw `versus_screen.cpp:30–33,59,77,90–94,111` |
| `[VersusBGdef]` / `[VersusBG n]` | drawn | `defs.cpp:464` |
| `[Victory Screen]` + BGdef | **drawn** (2026-10-19): background, portrait `9000,2` with window/scale/facing, name, winquote fallback, fades and skippable hold | parse `defs.cpp`; draw `screens/victory_screen.cpp`; timing `engine/victory_flow.*` |
| `[Win Screen]` | **drawn** (2026-10-19): black card, frozen winner pose, `wintext` (compiled defaults when the section is absent), no skip | parse `defs.cpp`; draw `screens/win_screen.cpp`; timing `engine/win_flow.*` |
| `[Continue Screen]` | **drawn** (2026-10-19): five elements at `pos`, big-motif defaults, YES/NO with left/right + Start/attacks; 600-tick interpreted countdown | parse `defs.cpp`; draw `screens/continue_screen.cpp`; timing `engine/continue_flow.*` |
| `[Game Over Screen]` | **played** (2026-10-19): the storyboard player runs the configured scenes; an absent/unplayable file keeps the log-and-title fallback | parse `defs.cpp`; `screens/storyboard_screen.*`; `FightScreen::endArcadeRun` |
| `[Option Info]` + BGdef | parsed; options item opens `LayoutScreen` instead | `title_screen.cpp:108` |
| `[Demo Mode]` | parsed, not consumed | `defs.cpp` |
| `[Survival Results Screen]` | parsed, not consumed | `defs.cpp` |
| `[Default Ending]`, `[End Credits]` | **played** (2026-10-19) through the same storyboard player, in that order after the Win card when enabled | `defs.cpp` records; `FightScreen::advanceEndSequence` |
| `[Music]` | parsed; no BGM playback | only `PlayBGM` controller type at `statemachine.cpp:2740` |
| `matchover`, `matchno` | **landed** (2026-10-19) | `statemachine.cpp` triggers; `FightScreen` drives them |
| match end → post-match screens | Victory per won match, Win once per cleared run, Continue on loss, Game Over gate on decline/timeout | `fight_screen.cpp` (`finishArcadeRun`, `endArcadeRun`, `restartMatch`) |

Consequences for a corpus screenpack today:

1. A motif that defines Victory/Win/Continue looks correct through the post-match
   screens, including the final card; Game Over, Default Ending and End Credits
   play their storyboards through `engine/storyboard.*`, and an enabled switch
   whose file is missing falls back to the old log-and-title path.
2. `ArcadeClear` is the end-of-run hold that follows the Win card (or stands
   alone when `win.enabled = 0`); it no longer stands in for the Win Screen.
3. `winquote` is the Victory screen's fallback; the winner's `VictoryQuote`
   selection and its `[Quotes]` are resolved first (2026-10-19).
4. An options motif's sounds/background never appear; OPTIONS is an anifight
   layout editor.
5. `[Music]` is silent, including title/select/VS BGM.

## 5. Implementation order

The order below is chosen so each slice makes a visible, testable improvement to
a corpus screenpack without requiring the next slice. "New flow state" means
state that does not exist anywhere in the engine today.

| # | slice | engine surface | new flow state? | why this order |
|---|---|---|---|---|
| 1 | Parse the remaining sections inertly: add records for Victory, Win, Continue, Game Over, Option Info, Demo, Survival, Ending, Credits, `[Music]`, and their BGdefs; log what was found | `defs.h` (new structs + `SystemDef` fields), `defs.cpp` (`parseSystemDef`, `parseBackground` calls) | no | Zero behaviour change; proves the corpus motifs parse and gives every later slice its data. Also fixes the current silent-ignore of `fadeout.time`-style keys. |
| 2 | ✅ **landed 2026-10-19** — Make match/run state explicit and correct the triggers | `fight_screen.h/.cpp` (match number, run kind, final-match flag), `statemachine.h/.cpp` (`matchover`, `matchno`) | yes: `matchno`, `matchover`, run/final-match flags | All later screens branch on these. `matchno` is currently a constant 1, so even a correct Victory screen would show "Match 1" forever. |
| 3 | ✅ **landed 2026-10-19** — Victory Screen | `screens/victory_screen.{h,cpp}`; `engine/victory_flow.*` | uses #2 | Device-verified: portrait, name, winner/match line, 150+15 tick hold, tap-skippable after fade-in. |
| 4 | ✅ **landed 2026-10-19** (Win); Survival still open — Win Screen | `screens/win_screen.{h,cpp}`; `engine/win_flow.*` | uses #2 | Device-verified: black card, frozen pose, "Congratulations!", 396 ticks, then `ArcadeClear`. |
| 5 | ✅ **landed 2026-10-19** — Continue Screen | `screens/continue_screen.{h,cpp}`; `engine/continue_flow.*` | yes: continue result | Device-verified: prompt, 600-tick expiry, decline to title, accept restarts the same match without advancing `matchno`. |
| 6 | ✅ **landed 2026-10-19** — Game Over, Default Ending, End Credits | `engine/storyboard.{h,cpp}`, `screens/storyboard_screen.{h,cpp}`, then hooks at the `0x456196`/`0x4562c7`/`0x4562eb` equivalents | yes: run-over vs run-cleared outcome | Host-tested. Plays scenes/layers/fades/sounds (including per-sound volume and pan); no skip input; a missing file keeps the log-and-title fallback. `bgm` and `bg.name` objects remain unsupported. |
| 7 | Option Info + OptionBGdef | extend the existing options surface (or replace `LayoutScreen`'s background) with `[OptionBG]` and the three sounds | no | Cosmetic; options behaviour is already anifight-specific. |
| 8 | `[Music]` | audio module; per-screen BGM with volume/loop/loopstart/loopend | no | Independent of flow; lowest priority for "looking right". |

Two smaller items fit anywhere after #1 and are nearly free:

- parse and play title/select cursor sounds (`title_screen.cpp` menu movement,
  `select_screen.cpp:108` blip already exists but is engine-generated);
- honour `title.fadeout.time` when leaving the title screen.

## 6. Explicit unknowns

1. **Continue countdown length and timeout semantics.** No key, no immediate
   constant in `0x44f470`, and no doc statement. Treat the duration as
   engine-internal and reproduce it only after timing the reference.
2. **Continue resume behaviour.** `matchno` does not increment (doc), but
   whether the round, life, and timer reset was not traced.
3. **Final-match suppression of the Victory screen.** The corpus comment is
   clear; the exact branch in `0x441e40`/`0x455c34` that tests "no more
   opponents" was not fully isolated.
4. **Credits.** There is no credit counter in MUGEN 1.0. "Credits" in the docs
   means the End Credits storyboard, not an arcade credit pool.
5. **`[Continue Screen] storyboard`.** The loader reads a `0x200` path at
   `+0x04`; no consumer was found and it appears in no corpus motif.
6. **Team and survival variants.** The post-match gate reads side/winner fields
   at `+0x4a14`/`+0x117e0`; how these map for team modes was not traced here
   (see the separate team-mode RE).
7. **Compiled defaults for `[Select Info]`.** The section is parsed by
   `0x440330`, not by the main loader; only the motif values are tabulated.
8. **Title fade defaults when a key is omitted.** The loader clamps to `>= 1`
   but the pre-clamp initializer was not identified.
9. **`p1.window` / `winquote.textwrap` rendering details.** The keys are parsed
   by the generic element reader; the exact clipping/word-wrap implementation
   in the reference was not traced.
10. **Game over return path.** The docs say "returned to the title screen"; the
    exact screen-stack operation after the storyboard was not traced. This
    engine implements it as two deferred pops (fight, then select), which lands
    on the title that has been under the arcade run all along. Popping back to a
    screen that was never re-entered exposed a latent title-menu bug: its
    `leaving_` fade-out state stayed set from the original ARCADE confirm, so
    the first tick back re-ran `activate()` and pushed a fresh select screen.
    `TitleScreen::activate` now clears that state, and the device check confirms
    the storyboard returns to a live title menu.

## 7. Corpus caveat

The values above are the distribution's motifs, not the engine's compiled
defaults. Motifs differ in ways that matter:

- `data/system.def` (default): Continue enabled but text-less; Game Over
  disabled; Victory and Win enabled; Default Ending and End Credits disabled;
  Survival enabled with `roundstowin = 5`.
- `data/big/system.def`: Continue enabled **with a full text block and
  `pos = 640,240`**, plus the distribution comment that the display elements
  use `fight.def` resources; Victory geometry rewritten for a 1280-wide
  screenpack.
- `data/mugen1/system.def`: Continue enabled with full keys; Game Over
  disabled; Victory/Win enabled.
- `data/kfm/system.def`: no `[Victory Screen]` section at all; Game Over
  enabled with `storyboard = gameover.def`; Win enabled.

A screenpack can therefore omit a section entirely. Parsers must keep the
record's compiled defaults when the section is absent, rather than inventing
geometry, and the engine must treat a missing `enabled` as "use the compiled
default" only where that default is known. The default motif's comments are the
best available statement of intended behaviour, but they are distribution
documentation, not proof of engine behaviour.

No corpus `.def` content is copied into this repository; the tables above are
key names, values and addresses only.

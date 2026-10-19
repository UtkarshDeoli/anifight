# The roster

Every fighter in this game is a community character. There are no generated
characters any more — `tools/characters/gen.py` and its Gradle tasks are gone —
so the grid is exactly what you download.

`tools/screenpack/data/select.def` reserves a grid cell for each character
below. A cell is only a *slot*: it holds a fighter once a character folder is
sitting in `local_chars/`, which is gitignored and packed into your APK at build
time. Nothing on this list ships in the repository, and nothing here is
downloaded for you.

**The roster is Dragon Ball and nothing else.** That is a decision about the
fight rather than about taste, and the reasoning is in `select.def` itself: these
characters are edits of edits published over twenty years, and their art arrives
at every scale from 70 pixels to 200 for the same declared height. Left alone the
biggest is half again the size of the smallest and reaches half again as far, and
the two are not a match. The engine normalises the art so they are the same size
(see below), and keeping the roster to one series keeps the *look* consistent
too -- a 1998 hand-drawn sprite and a 2015 redrawn one in the same match read as
a bug even when they measure the same.

Twenty-one famous-name entries used to fill the rest of the grid -- Ronald
McDonald, Peter Griffin, Goku, Naruto, Mario, Sonic, Rare Akuma and the rest.
None of their folders is obtainable from a free-to-use source, so every one of
them loaded as an empty cell: a grid two thirds black boxes, and a cursor that
had to be told to step over them. They are gone.

Five more used to sit below them, from amarimono's MUGEN work
(`github.com/amarimono-mugen/products`). Those did load -- JoJo characters, and
good ones -- and were dropped for being the wrong series rather than for being
wrong.

On a fresh clone every cell but Kung Fu Man is empty. That is the intended
state, not a broken install: `Game::loadWorld()` pushes an empty cell for a
folder it cannot open so the grid never renumbers itself around whatever is
installed, `system.def` sets `moveoveremptyboxes = 0` so the cursor steps over
those cells instead of stopping on them, and the select screen draws a plate only
for a cell that is *reserved* -- one inside the roster with nothing in it -- so
the cells past the last entry are blank rather than black.

**Kung Fu Man is the one character a stock install is expected to have**, since
it ships with MUGEN itself, and select.def lists it first for that reason. With
no characters at all the select screen has nothing to land on and cannot start
a match — so if you remove `local_chars/kfm/`, add something else in its place.

## The list

### Kung Fu Man

| Entry | Who | Where it comes from |
|---|---|---|
| `kfm/kfm.def` | Kung Fu Man — Elecbyte's sample fighter | Ships with MUGEN itself |

He is the one character a stock install is expected to have, and he is also the
height the whole roster is measured against: `kReferenceStandingHeight` in
`engine/fighter.cpp` is his standing art, 106 pixels in a 320x240 space, and
every other fighter is scaled to draw the same. A roster without him has nothing
to be the size *of*.

## Dragon Ball

Thirteen more, collected from Mugen Free For All's Dragon Ball series thread
(`mugenfreeforall.com/topic/51078-dragon-ball-series-part-1/`). That thread
indexes characters by *author* rather than by name, so the author is part of
the entry here: several of these are edits of edits, and the thread is the only
place that records whose is whose.

| Entry | Who | Author |
|---|---|---|
| `dbz_goku/dbz_goku.def` | SSJ Goku | SSJTrunks |
| `dbz_gohan/dbz_gohan.def` | P-Gohan (SSJ2) | Barrado |
| `dbz_vegeta/dbz_vegeta.def` | Vegeta (base) | HiddenSage121 |
| `dbz_gogeta/dbz_gogeta.def` | Gogeta | Supermystery |
| `dbz_piccolo/dbz_piccolo.def` | Piccolo | Ssonic |
| `dbz_gogeta_ssj4/dbz_gogeta_ssj4.def` | Xeno Gogeta SSJ4 | Orochi Shin |
| `dbz_vegeta_ssj4/dbz_vegeta_ssj4.def` | Vegeta SSJ4 | Ryon, edited by Kenjiro44 |
| `dbz_vegeta_bal/dbz_vegeta_bal.def` | Balance SSJ4Vegeta | Linkgamer2004 |
| `dbz_vegeta_ssg/dbz_vegeta_ssg.def` | Vegeta SSG | Sustina, edited by GOD984 |
| `dbz_vegetto/dbz_vegetto.def` | Vegetto, transforming | ThiaggoAssis |
| `dbz_vegetto_ssj/dbz_vegetto_ssj.def` | Super Vegetto SSJ | Mr. 4x3l |
| `dbz_vegetto_ssb/dbz_vegetto_ssb.def` | Vegetto | Adoriendson Tenorio |
| `dbz_yamcha/dbz_yamcha.def` | Yamcha | Mr. 4x3l |

Three things about these particular files:

- **Two defs are misleadingly named, and the naming is the author's.** The
  `dbz_vegeta_bal` archive calls itself "Balance SSJ4Vegeta" but points at
  `SSGSS_Vegeta.sff` — SSGSS is Super Saiyan God Super Saiyan, i.e. Blue, not
  SSJ4. `dbz_vegetto` likewise ships a `Goku.sff` under the name "Vegetto
  Transform SSB". The `[Info] displayname` is what the game shows, so these
  appear on the grid as the author wrote them.
- **The def files were renamed.** Several arrived with spaces and accented
  characters in their filenames; each was renamed to `<folder>.def` so the
  folder, the `select.def` line and the file all read the same. Nothing inside
  a def refers to its own filename, so this is safe.
- **Two are SFF v2** (`dbz_vegeta`, `dbz_vegetto`), the rest v1. The engine
  reads both; v2 is the same sprite bank with a node table instead of inline
  PCX data.

The three Vegettos are separate characters rather than one fighter with three
forms. They were published independently, have unrelated sprites, animation
sets and move lists, and share no code — so the grid shows three cells, and
that is the honest representation of what they are.

### They are all the same size now, and it is not an accident

Kung Fu Man's standing sprite is 106 pixels tall in a 320x240 space. Among the
Dragon Ball thirteen, the same figure runs from 70 (`dbz_gohan`) to 200
(`dbz_vegetto_ssj`) -- a factor of nearly three -- while nearly all of them
declare MUGEN's canonical `size.height = 60`.

Nothing in the format makes those two characters the same size, and nothing about
them was: the big one reached half again as far with half again as much body,
which is not a match but a mismatch pretending to be a difficulty curve. Left
alone, the largest of the thirteen stood at nearly twice the height of the
smallest.

So a fighter is scaled to draw at the reference height. The scale applies to the
art and to the collision boxes together, because a box is authored against the
sprite it belongs to -- scaling only the pixels would leave every character
hitting with the reach of a fighter it no longer is. Motion is deliberately left
alone: how fast a walk goes and how high a jump reaches are the character's own
numbers and stay as its author tuned them. See `Fighter::setScale`.

`[Size] height` is *not* what this scales by. It is not reliable enough: on this
roster one character claims 150 units and draws 90 pixels while another claims 60
and draws 200, and normalising by the claim would make the first of those two and
a half times the size of the rest. The art is what is measured, because the art
is what a player sees.

### Case matters, and it bit one of these

`dbz_piccolo`'s def asks for `piccolo.sff`, `piccolo.air`, `piccolo.cns` and
`piccolo.cmd`, but the archive shipped them capitalised (`Piccolo.sff` …).
Windows does not care, so the character works under MUGEN; Android's asset
system is case-sensitive, so on device all four lookups failed and the fighter
loaded with default stats and no sprites. The files were renamed to lower case
to match the def. If you add a character and it appears with the right name but
the wrong body, check this first.

## Installing one

1. Download the character. The long-running hubs are **MUGEN Archive**,
   **Mugen Free For All** and **Infinity MUGEN Team**; the **MUGEN Database**
   wiki is the index of what exists and who made it.
2. Unpack it into `local_chars/` so the folder name matches the entry above.
3. `./gradlew assembleDebug`. Gradle's `assembleLocalChars` task copies
   everything in `local_chars/` into the APK's `mugen/chars/`, so the folder
   rides into your build without ever being committed.
4. The cell fills in. Portrait, animations and stats all come from the
   character's own files.

### What a character folder is

A MUGEN character is a folder of files the engine's loader reads at runtime.
Nothing about one is compiled or generated:

- `.def` — identity and the file list. Its `[Info]` block is where the author
  and the character's name live.
- `.sff` — the sprites: the move art, and the two portraits in group 9000 that
  the select grid and the HUD's face plates draw.
- `.air` — the animations, one action per move, with `Clsn2` hurt boxes per
  frame and `Clsn1` attack boxes on the active frames.
- `.cns` — the constants: `[Data]` life/attack/defence, `[Size]`, `[Velocity]`
  and `[Movement]`. The engine reads these, so stat differences are real.
- `.cmd` — the command table.

### The folder name has to match

Character packs disagree about what to call themselves — `goku/`, `Goku/`,
`GokuZ2/`, `goku_z2/` are all in circulation, and the entry above only matches
one. Rename the folder to the entry, or edit the entry to the folder. Either
is a one-line change and nothing else in the engine cares.

## Two things worth knowing before you download twenty of them

- **Scripted states are not interpreted yet.** The move set is hand-written
  C++ parameterised by each character's `.cns` constants, which are read. A
  community character loads, draws, and fights with its own stats, animations,
  hit boxes and portraits — but the states scripted in its `.cns` are not run,
  so its special moves do not exist here yet. That is the next project and it
  is large. What you get today is the character's look, numbers and normals.
- **The grid holds 40, and the list fills all 40.** Entries past the 40th have
  no cell to land on. `system.def` calls it 10 columns by 4 rows, sized to a
  320x240 screen, and that is now a hard limit rather than a roomy one:
  neither axis can grow. A fifth row would start at y 138 and the on-screen pad
  claims everything from y 140 down, and eleven columns would run off the right
  edge. Going past 40 means smaller cells, which means redrawing the
  screenpack's cell sprites — a screenpack change, not a roster one.
- **The APK is large and getting larger.** The sprite sheets are the whole
  cost: the Dragon Ball fighters alone are ~270 MB of a ~326 MB debug build,
  because Extreme Butoden rips carry a thousand-plus sprites each. `.snd` and
  music files were stripped on the way into `local_chars/` back when nothing
  read them, and that is the only trimming that happens.

  That trimming has since been paid for: the engine reads `.snd` now, so the
  characters' voices and effects are gone, and the source archives they were
  stripped from are not kept anywhere. Measured, 18 of the 19 characters declare
  a `sound = ` their folder does not contain. The engine prefers a character's
  own bank the moment one exists (`Mixer::playFrom` searches it first), so
  dropping the real files back in is the whole of the fix — see the note on
  sounds below.

## Sounds

The screenpack ships no synthesised audio. `fight.def` names `fight.snd` and
`common.snd`, and `system.def` names `system.snd`; `tools/screenpack/gen.py`
copies the player's own copies of those three files from `local_sounds/` into the
build byte for byte. If `local_sounds/` is absent, no bank is written and the
engine is simply silent for the pairs it cannot resolve — a stand-in is not a
substitute for the authors' recordings.

A character that ships its own `.snd` is heard ahead of the shared bank with no
configuration at all, because `Mixer::playFrom` looks in the character's bank
before the screenpack's. To restore a character's voice and effects, drop its
original `.snd` beside its other files in `local_chars/`.

## What the engine implements, and what it does not

The fight engine follows MUGEN's own controller reference (`docs/sctrls.html` in a
MUGEN distribution, which is the authoritative spec), and the roster is a good measure
of which parts matter: the counts below are how often the nineteen installed
characters use each controller between them.

Implemented: `ChangeState`, `ChangeAnim`, `CtrlSet`, `VelSet/Add/Mul`,
`PosSet/PosAdd/Offset`, `Var*`, `PowerAdd`, `PlaySnd`, `StopSnd`, `HitDef`, `HitVelSet`,
`NotHitBy`/`HitBy`, `AssertSpecial` (the shadow, invisibility, the three guard
refusals, `nohardcodedkeys`, `nojugglecheck`), `Explod`, `Projectile`, `EnvShake`,
`Turn`, `StateTypeSet`, `SprPriority`, `LifeAdd`/`LifeSet`, `Null`.

**Not implemented, and used a lot:**

| controller | uses here | what is missing |
|---|---|---|
| `Helper` | 793 | MUGEN's third kind of entity: a second fighter with its own state machine. Summoned characters, some beams, and most transformation effects are built on it |
| `AfterImage`, `AngleDraw`, `PalFX`, `Trans` | 446 | trails, rotation, palette flash. The renderer draws no rotated quads yet |
| `HitOverride`, `TargetState`, `TargetBind` | 325 | invulnerable windows that redirect a hit, and the machinery that carries a thrown opponent |
| `SuperPause` | 111 | the whole-engine freeze a super's first frame runs behind |
| `ScreenBound`, `Width`, `Depth` | ~130 | per-character stage bounds and body width; every fighter is pushed apart by one constant |

So a beam written as a `Projectile` works -- it flies, it hits, it makes a spark -- and
a beam written as a `Helper` does not. That is the largest remaining gap, and it is
infrastructure (a second entity kind with its own state machine) rather than a tweak.

## Hit sparks, and the fightfx file

A HitDef's plain `sparkno = 2` refers not to the character's animations but to the
screenpack's **`fightfx.air`**; the `S` prefix is what means "this character's own".
This screenpack had declared `fightfx.sff` and `fightfx.air` from the beginning and the
generator wrote neither, so every hit landed without a spark -- which is most of what
made the fight look like two sprites passing through each other. The generator now
writes both, with Elecbyte's action numbering (0 light, 1 medium, 2 heavy, 3 hardest,
10/11/12 the guard sparks) and our own art: a starburst that opens and fades, drawn
additively, spawned where the contact was.

## The fight screen's HUD, and where its numbers come from

The in-fight gauges are Shin Budokai 2's, not MUGEN's. Every position, colour and
proportion in `FightScreen::drawHud` was measured off that game's own promotional
screenshots and scaled from the PSP's 480×272 to this screenpack's 320×240, so a
number in that function is where the element really is in the game being copied:

| element | is |
|---|---|
| health plate | a slanted plate carrying seven small squares — how many *health bars* are left — over the health bar |
| health bar | one continuous run, **anchored at the outer edge and eaten from the centre end** (the opposite of the usual convention), green, with the damage just taken drawn yellow behind it and the recoverable part of it red |
| ki gauge | a smaller plate at the outer corner holding seven diagonal slashes and the count of filled ones; lavender when full, and the count turns red at zero |
| portrait, name | the character's own face art and its name, between the two plates |
| guard bar | a thin white-outlined teal bar under the cluster, draining as blocks are made |
| timer | a red-rimmed capsule at the top centre, and nothing at all in the story mode, which is timeless |
| damage counter | "N HITS!" and "N dmg." in the open screen below, on the attacker's side |

Two of those are drawn against quantities MUGEN has no equivalent of, and both are
real rather than decorative. The yellow trail is the damage just taken, eaten away
over about a second. The red part of it — damage taken *behind a guard* — drains back
into the bar if the fighter is not hit again before it settles, which is what makes a
blocked exchange recoverable rather than permanent.

**There are no round-win markers, and that is deliberate.** Shin Budokai has none
anywhere on its HUD; rounds are announced between them, in full-screen lettering
("PERFECT!!", "YOU WIN!!", "K.O."), which is what this engine does too. Adding pips
would have been inventing an element rather than copying one.

The screenpack's own `[LifeBar]`, `[PowerBar]` and `[WinIcon]` sections are therefore
no longer drawn. Their drawing code is still in `fight_screen.cpp`, marked as no
longer called, because it is the MUGEN-compatible half of the HUD and nothing in this
repository's history has it.

## The on-screen pad, and where its format comes from

The touch controls take their *format* from PPSSPP's Android pad, which is the
most-used touch fighting-game control scheme there is. What is worth taking from it is
not its art but the way it describes a layout: every control is a position as a
**fraction of the screen**, a scale and a show flag, kept in flat settings keys, with
`-1` meaning "never configured" so that a fresh install computes defaults and a saved
layout always wins. Positions in screen fractions rather than in the game's own
coordinate space is what lets a layout survive a change of aspect ratio, a rotation, or
a different phone — which matters here because this engine letterboxes a 320×240
screenpack into whatever the device reports.

The layout lives in `[TouchControls]` in the app's settings file, with PPSSPP's own key
names (`ActionButtonCenterX`, `DPadX`, `TouchButtonOpacity`, `StickyTouchDPad`,
`TouchGliding`, `DisableDpadDiagonals`, …), and **Options → Control layout** in the game
is a live editor for it: drag a control to move it, Start for resize mode, and the
editor's own Reset and Done buttons. It edits the real controls rather than a picture
of them, and writes the layout out when a drag ends.

The behaviours are PPSSPP's too, because they are what a player who has used that pad
expects: the d-pad is read from the **angle** of the thumb rather than from which arm
is nearest, so a diagonal comes out of a slide; there is a dead zone in the middle of
the cross; a sticky d-pad keeps a direction when the thumb drifts off it; gliding lets
a finger roll from one attack button onto the next; and the six attacks are **one**
cluster with a centre and a spacing, so moving them moves all six.

## Credit

Credit belongs where the author put it: the `[Info] author` field of the
character's own `.def`, which is where a real MUGEN character keeps it. This
repository claims nothing about any character on this page.

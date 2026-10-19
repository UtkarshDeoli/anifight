# Anifight v2 — the starter roster

2026-10-16, revision after v1's empty game.

## What this is

The ten starter characters, and the engine changes needed to fight them. It
completes v1's arc: the select screen's grid was built to point at nothing;
now it points at ten fighters.

## What was built

- **`engine/character`** — loads a character folder: the `.def`, its `.sff`
  sprite bank, `.air` animations and the handful of `.cns` constants the
  move set needs (`[Data]` life/attack/defence, `[Velocity]` walk and jump,
  `[Movement]` yaccel, `[Info]` localcoord). Missing art degrades; a missing
  `.def` is a skipped slot, not a failed boot.
- **`engine/fighter`** — the built-in move set: stand/walk/jump/punch/kick,
  MUGEN-convention action numbers, hit detection from the `.air` CLSN boxes,
  hitstun, knockback, power gain and KO. Every number it uses comes from the
  character's files, so stat differences are real. A fighter without a
  character is the placeholder box, which still takes hits exactly like a
  real one.
- **Select screen** — portraits in occupied cells, a locked pick shown with
  the done cursor, versus mode's second pick, and `randomselect` cells that
  roll at confirm time.
- **Fight screen** — real fighters when the roster supplies them (player on
  the pad, CPU on the other side), placeholder boxes in the engine test, a
  camera that sits between the fighters and clamps at the stage's ends, and
  the HUD's face plates now draw the character's group-9000 portrait.
- **`tools/characters/gen.py`** — the ten characters, drawn programmatically,
  written as complete MUGEN folders at build time (Gradle task, same pattern
  as the screenpack).
- **AIR parser fix** — `Clsn*Default` boxes now apply to every frame until a
  per-frame declaration, instead of only the first frame. Community packs
  rely on that.

## What is deliberately not here yet

- **A `.cns` state machine.** The move set is hand-written C++ parameterised
  by character constants. Community packs that script custom behaviour in
  their `.cns` will load, draw and fight with their constants and animations,
  but their scripted states are not interpreted yet. That is the next project
  and it is large.
- **Device-storage character folders.** Today characters load from the APK;
  reading a user's own `chars/` folder from device storage is on the v2 list.
- **Stages from files.** `select.def` names `stages/training.def`; the fight
  still draws the procedural stage, which is why roster lines carry no stage.
- **Player two on a pad.** Versus mode's second pick is made with the same
  pad; the fight itself is one human against the CPU until a second input
  source exists on one screen.

## Decisions recorded

- **The ten starters are original archetypes, not licensed characters.** This
  project ships only what its generators draw. Community characters belong in
  an uncommitted local folder, credited in the place their authors chose.
- **Action numbers follow MUGEN's conventions** (0/20/40/200/210/5000/5150)
  so packs using them read correctly as the support widens.
- **One shared palette of named colours** across all ten characters keeps the
  generator simple; SFF v1 embeds the palette per PCX, so a future character
  palette per fighter is a local change.

## The trap the real character file caught (2026-10-16)

The v1 doc said it: a reader and a writer built from the same belief will
agree with each other and both be wrong. Kung Fu Man -- the first real
third-party SFF v2.01 file this engine parsed -- found exactly that, twice
over:

- **LZ5's offset high bits were mis-accumulated.** `d & (0xC0 >> k)` masks
  *after* shifting, which for k >= 2 reaches down into the token's run-length
  bits and pollutes the offset; the correct reading is `(d & 0xC0) >> k`,
  mask first. Any sprite whose back-references had nonzero high bits garbled;
  small ones whose references stayed short looked *right*, which is why the
  bug hid until a full-body sprite arrived.
- **Extended back-references copied one byte short** (lastByte + 2 rather
  than the reference implementations' n+1-counting, i.e. lastByte + 3 exact).

Both are pinned by `tools/hosttest/test_sff_codecs.cpp`, which decodes real
sprities from a community character file and compares checksums produced by
an independent decoder. Run from the repo root:

    clang++ -std=c++20 -O1 -I app/src/main/cpp -o /tmp/test_sff_codecs \
        tools/hosttest/test_sff_codecs.cpp app/src/main/cpp/gfx/sff_codecs.cpp
    /tmp/test_sff_codecs local_chars/kfm/kfm.sff

The `Clsn*Default` parser fix that same file required is recorded in the v1
doc's traps -- `Clsn2Default` was silently applying to the first frame only
before it.
# MUGEN 1.0 Shadow Rendering — Behavior Notes

Non-commercial interoperability research on the reference binary
(`local_mugen/extracted/mugen/mugen.exe`, SHA-256
`887d00a04e94cd8df394304822b2a36f956516aa60c113576fd8ba9d3f9a06c4`,
PE32, MSVC 9.0, built 2011-01-18; identity/details in `FINDINGS.md`).
**Behavior notes only; no code from the binary enters `app/`.**

Method: static analysis of the objdump listing
`/tmp/mugen_peek/mugen_disasm.txt` (195,998 instructions, VAs below are
image VAs with ImageBase 0x400000), plus the shipped MUGEN 1.0 docs for
intended semantics (`local_mugen/extracted/mugen/docs/`). Scratch files
under `/tmp` are not part of the repository and may need regeneration.

Scope: shadows only — player shadows, the `[Shadow]` / `[Reflection]`
stage groups, the `noshadow` / `globalnoshadow` assertions, Explod
`shadow`, Projectile `projshadow`, and the Trans interaction.

---

## 0. Answer table

| # | Fact | Confidence | Evidence |
|---|---|---|---|
| 1 | There is **no per-player shadow color or alpha field**. Player shadow appearance is owned entirely by the stage. | HIGH | draw fn 0x470AC0 takes only `player`, `&engine+0x1C24`, `&engine+0x1C38`; no player shadow color is read anywhere. String inventory has no player shadow-color key. |
| 2 | `[Data] shadow` does **not exist** in MUGEN 1.0. The only character-side shadow-related key is `[Size] shadowoffset` (and the `size.shadowoffset` Helper parameter). | HIGH | exhaustive string/xref sweep; `shadowoffset` 0x4A27E4 parsed at 0x467A8B; no `[Data]` shadow parser. |
| 3 | Player shadow color comes from stage `[Shadow] color = r,g,b` (0–255 each; higher = less of that color), default 0,0,0. | HIGH | stage parser 0x457A4B–0x457AC6; packed via 0x411170 (SDL_MapRGB wrapper) into `engine+0x1C28`. Docs `bgs.html:390-395`. |
| 4 | Stage `[Shadow] intensity` default 128, stored at `engine+0x1C24`. | HIGH | `movl $0x80,0x154(%ebp)` at 0x457A37 (base `engine+0x1AD0` ⇒ 0x1C24); docs `bgs.html:387-389`. |
| 5 | Stage `[Shadow] yscale` default 0.4, stored at `engine+0x1C2C`; negative values flip the shadow to the other side. | HIGH | default float 0x4A6D30 = `0x3ECDCCCC` (0.4f) at 0x457A1F–0x457A2E; key 0x4A107C parsed at 0x457A81–0x457A8E; docs `bgs.html:396-399`. |
| 6 | Stage `[Shadow] fade.range = top_y, bot_y` controls a linear alpha ramp stored at `engine+0x1C30`/`+0x1C34`; omitted = no fade. | HIGH | key 0x4A1098 parsed at 0x457A91–0x457AAB; alpha ramp at 0x470F42–0x470F90 (player) and 0x473887–0x4738D7 (objects). |
| 7 | `[Reflection]` in the shipped 1.0 binary parses **`intensity`**, not the `reflect` flag documented in `bgs.html`. Default 0 (off); reflection is drawn only when intensity > 0. | HIGH | section 0x4A10A4 at 0x457ACF; key read is 0x4A108C (`intensity`) at 0x457AE7–0x457AEE, stored `engine+0x1C38`; draw gate `(*arg3 > 0)` at 0x470E00-ish/0x473749. `bgs.html:411-417` is stale; `bg-tut.html:354-358` documents `intensity` correctly. |
| 8 | `[Config] drawshadows` controls the global switch `engine+0x118B8`, default **1**. | HIGH | default `movl $1,0x118B8` at 0x48D993; key 0x4A65F8 parsed at 0x48DA6A–0x48DA71. |
| 9 | `AssertSpecial` `noshadow` suppresses **only that player/helper's own shadow**, not the shadows of its projectiles/explods. `globalnoshadow` suppresses every shadow pass. | HIGH | noshadow code 4 → `player+0x254` (mapper 0x4899CE, executor 0x4792CD); gate only in the player loop at 0x447891. globalnoshadow code 6 → `engine+0x1165E` (mapper 0x489762, executor 0x4792C3); gate 0x44785F skips all three loops. Docs `sctrls.html:505-508`. |
| 10 | Asserts are one-tick flags: the byte arrays are cleared and re-set every tick. | HIGH | player assert bytes cleared at 0x46A7F7–0x46A803 in the same per-player update that resets Trans (0x46AB5A); executor sets `engine+0x11658[code]` or `player+0x250[code]` to 1 at 0x4792C3/0x4792CD. |
| 11 | Shadows are drawn in three passes: player/helper shadows, per-player object-list shadows, global object-list shadows. | HIGH | draw routine 0x4477B7/0x447840: 0x470AC0 per player, then 0x473570 per `player+0x268`, then 0x445060 for `engine+0xFCB0`. |
| 12 | A player shadow is the player's **current animation sprite**, vertically scaled by the stage yscale, projected onto the ground plane. It has no independent art. | HIGH | 0x470AC0 reads `player+0x1418` (anim record) and sprite index at anim `+0x2C`; yscale `fmuls 0x8(%ebx)` at 0x470EC6. Docs `bg-tut.html:346-352`. |
| 13 | Player shadow alpha = `[Shadow] intensity` × `fade.range` ramp; blending needs two blits (tint at alpha A, then complement 256−A). | HIGH | alpha math 0x470E60–0x470FE2; calls 0x4193C0 at 0x471039 and 0x418A30 at 0x47105E-ish. |
| 14 | `Trans` type 3 (`addalpha`) changes the player shadow's starting alpha to `256 − dst_alpha` (`player+0x13EC`). Other Trans types are ignored by the shadow path. | HIGH | `cmpl $3,0x13E4(%ebp)` at 0x470F08; `subl 0x13EC(%ebp),%ecx` at 0x470F26. Trans parser 0x47953B–0x4795AF; docs `sctrls.html:2879-2905`. |
| 15 | There is **no `shadow` state controller**. The string `"shadow"` (0x4A3F24) is only the Explod parameter, not a controller name. | HIGH | only xref to 0x4A3F24 is the Explod parser at 0x47CA84; controller name dispatch has no `shadow` entry. |
| 16 | `Clsn` boxes / frame data do not affect shadow geometry. No Clsn access appears in any shadow function. | HIGH | no Clsn reads in 0x470AC0, 0x473570, 0x473600, 0x445060, 0x4450D0, 0x4189F0, 0x418A30. |
| 17 | Sprite axis is not read by the shadow draw itself; whether it affects the final blit origin depends on the shared blit helpers 0x4189F0/0x418A30 (not disassembled to completion). | LOW | shadow functions pass sprite index, position and scales only; the axis is consumed inside the blit helper at best. |
| 18 | Explod `shadow = r,g,b`: default 0,0,0 = no shadow; `r == -1` = use stage color; out-of-range emits `"Illegal shadow color"`. Result stored per global object at `+0x70`. | HIGH | parser keys 0x47CA84 (`shadow` 0x4A3F24) with template +0x104/+0x110/+0x11C; runtime eval 0x445E35–0x445F2A, error string 0x49F904 at 0x445EB2, store `+0x70` at 0x445F2A. Docs `sctrls.html:1006-1011`. |
| 19 | Projectile `projshadow = r,g,b`: default 0,0,0 = no shadow; **any** component equal to −1 selects the stage color; out-of-range emits `"illegal shadowcolor"`. The packed color lands in the per-player object element at `+0x40`. | HIGH | parser 0x48101B–0x481047 (template +0xC4/+0xD0/+0xDC); validate/pack 0x48123F–0x4812B2, error 0x4A49BC at 0x4812D3; spawn template +0x40 → element `+0x40` via 0x472CB0 → memcpy helper 0x4030F0. Docs `sctrls.html:2442-2448`. |
| 20 | Object shadow color sentinels at draw time: `0` = skip, `-1` = stage `[Shadow] color`, else the packed explicit color. | HIGH | per-player list at 0x473599–0x4735B1; global list at 0x445081–0x44509A. |
| 21 | Object shadows obey `drawshadows` and `globalnoshadow`, but **not** the owning player's `noshadow`. | HIGH | 0x473681 and 0x44515C check 0x118B8; the second/third loops (0x4478E1–0x447914) have no `player+0x254` check. |
| 22 | Helpers are players in the same player table (`engine+0x11230`), so a Helper's own shadow is drawn by the player path with the helper's anim/position and the stage's shadow parameters. | HIGH | 0x44787A–0x4478B2 iterates `engine+0x11230` via `engine+0x11430/+0x11434`; no helper-specific shadow branch. |
| 23 | Helper `size.shadowoffset` is parsed (`[Size] shadowoffset`) into a constants slot `+0x31C`, but **no shadow-draw consumer was found**; its observable effect is unresolved. | LOW | parse 0x467A8B–0x467A9C (key 0x4A27E4); no read in 0x470AC0/0x473600/0x4450D0; the only `+0x31C` reads are 0x46588A/0x465896/0x465920/0x465A32 in the per-frame routine at 0x4654D0. Docs `sctrls.html:1270-1274`. |
| 24 | No stage/player/state parameter other than these can change shadow color, scale, or alpha. Per-state control exists only through `AssertSpecial` (`noshadow`/`globalnoshadow`) and Explod/Projectile parameters. | MEDIUM-HIGH | exhaustive shadow-key string sweep; no per-state shadow record. |

---

## 1. Where the shadow model lives

### 1.1 There is no player shadow state

An exhaustive sweep of the resolved 1.0 strings yields every shadow-related
literal:

| String | VA | Role |
|---|---|---|
| `Shadow` | 0x4A1084 | stage `[Shadow]` section |
| `intensity` | 0x4A108C | stage `[Shadow]` and `[Reflection]` key |
| `fade.range` | 0x4A1098 | stage `[Shadow]` key |
| `Reflection` | 0x4A10A4 | stage `[Reflection]` section |
| `size.shadowoffset` | 0x4A20A0 | Helper parameter |
| `shadowoffset` | 0x4A27E4 | character Constants `[Size]` key |
| `shadow` | 0x4A3F24 | Explod parameter |
| `projshadow` | 0x4A4918 | Projectile parameter |
| `Illegal shadow color` | 0x49F904 | Explod runtime error |
| `illegal shadowcolor` | 0x4A49BC | Projectile runtime error |
| `globalnoshadow` | 0x4A5920 | AssertSpecial |
| `noshadow` | 0x4A5994 | AssertSpecial |
| `drawshadows` | 0x4A65F8 | `mugen.cfg [Config]` |

There is no `shadowcolor`, no player `shadow` key, and no `[Data] shadow`.
The draw function confirms it structurally: `0x470AC0` receives exactly
`player`, `&engine+0x1C24` (the stage `[Shadow]` record) and
`&engine+0x1C38` (`[Reflection] intensity`) — no per-player color/alpha
argument exists (call site 0x4478A3–0x4478B2).

### 1.2 Stage parse sites

The stage parser region (function at 0x4570A0; stage record base is
`engine+0x1AD0`, see `FINDINGS.md`) writes the complete shadow model:

| Stage field | Engine VA | Default | Key / string | Parse site |
|---|---|---|---|---|
| `[StageInfo] resetbg` | `engine+0x1C0C` | 1 | `resetbg` 0x4A1074 | 0x4579BA–0x4579D1 |
| `[StageInfo] yscale` | `engine+0x1C10`, `+0x1C14` | 1.0 | `yscale` 0x4A107C | 0x4579D6–0x457A1A |
| `[Shadow] intensity` | `engine+0x1C24` | **0x80 (128)** | `intensity` 0x4A108C | default 0x457A37; key 0x457A6B–0x457A79 |
| `[Shadow] color` | `engine+0x1C28` (packed) | 0,0,0 | `color` (via `r,g,b`) | 0x457A4B–0x457AC6, packed by 0x411170 |
| `[Shadow] yscale` | `engine+0x1C2C` | **0.4f** (float 0x4A6D30) | `yscale` 0x4A107C | 0x457A1F–0x457A2E; key 0x457A81–0x457A8E |
| `[Shadow] fade.range` | `engine+0x1C30` (top), `+0x1C34` (bottom) | 0 (off) | `fade.range` 0x4A1098 | 0x457A91–0x457AAB |
| `[Reflection] intensity` | `engine+0x1C38` | 0 (off) | `intensity` 0x4A108C inside `Reflection` 0x4A10A4 | 0x457ACF–0x457AF3 |

The color triple is packed with `0x411170`, a thin wrapper around SDL's
`SDL_MapRGB` (0x4991C8) using the current screen surface at `0x52B3AC`;
therefore `engine+0x1C28` is a display-format pixel value, not an RGB word.
The same helper packs explosion/projectile colors later.

`[Shadow] intensity` is **not clamped** by the parser: any integer is stored.
The draw path treats `>= 0x100` as full alpha (0x470F92, 0x47391D), so
out-of-range values degenerate rather than error.

Docs quotes:

- `bgs.html:387-389` — intensity 0 (lightest) to 256 (darkest), default 128.
- `bgs.html:390-395` — color r,g,b 0–255; higher = less of that color;
  default 0,0,0.
- `bgs.html:396-399` — yscale default 0.4; negative puts the shadow on the
  other side of the player.
- `bgs.html:400-408` — fade.range top_y, bot_y; invisible above top, fully
  visible below bottom, linear between; omitted = always visible.
- `bgs.html:411-417` — documents `reflect = 0/1`; **the 1.0 binary does not
  read `reflect`** (no such literal in the image) and reads `intensity`
  instead. `bg-tut.html:354-358` documents the binary's behavior.

---

## 2. Global switches and gating

### 2.1 `DrawShadows`

`[Config] drawshadows` is parsed at 0x48DA5A–0x48DA71 into
`engine+0x118B8`; the default is 1 (`movl $1,0x118B8(%eax)` at 0x48D993).
All three draw functions gate on it:

- player shadow: `cmpl $0,0x118B8(%edi)` at 0x470B5D, skip at 0x470B7B;
- per-player objects: 0x473681;
- global objects: 0x44515C.

### 2.2 `globalnoshadow`

`AssertSpecial(globalnoshadow)` is mapped at 0x489762–0x48979B to
**code 6, scope 0** (`movl %eax,(%esi)` with eax=0; `movl $6,(%edi)`).
The executor at 0x479294 writes the assert byte:

```
0x4792B9  testl %eax,%eax        ; scope
0x4792BB  jne   0x4792CD         ; non-zero => player-local
0x4792BD  movl  0x52B40C,%edx
0x4792C3  movb  $1,0x11658(%edx,%esi)   ; engine+0x11658 + code
...
0x4792CD  movb  $1,0x250(%esi,%ebp)     ; player+0x250 + code
```

Code 6 therefore lands at `engine+0x1165E`. The draw routine tests it at
0x44785F and jumps to 0x44791F, skipping **all** shadow passes.

### 2.3 `noshadow`

`AssertSpecial(noshadow)` maps at 0x4899CE–0x489A05 to **code 4, scope 1**
(`movl $4,(%edi)`; `movl $1,(%esi)`), landing at `player+0x254`. The
per-player reset clears it with the rest of the assert block
(0x46A7F7–0x46A803: dwords at `+0x250`/`+0x254`, byte at `+0x258`), so it
is re-asserted each tick.

Only the first (player/helper) shadow loop tests it:

```
0x447891  cmpb $0,0x254(%eax)    ; noshadow
0x447898  jne  0x4478C0          ; skip this player's own shadow
```

The per-player object loop (0x4478E1–0x4478F7) tests only the alive flag
`player+0x1B0`; the global object pass (0x44790E) has no per-player test.
Consequence: **`noshadow` does not hide a player's projectiles or explods.**

### 2.4 Visibility and player table

The first loop iterates the player table `engine+0x11230` between indices
`engine+0x11430` and `engine+0x11434` (0x44786C–0x4478CA). For each entry
it requires:

- `engine+0x1B0` non-zero (alive/active flag; not further identified),
- `player+0x254` == 0 (`noshadow`),
- `player+0x1F0` non-zero (visible flag; driven by the intro blink at
  0x46C020–0x46C0EF),
then calls 0x470AC0.

The second loop walks the same table and calls `0x473570(player+0x268)` for
each live player. The third calls `0x445060(engine+0xFCB0)`.

Draw order per frame: player/helper shadows → per-player object shadows →
global object shadows.

---

## 3. Player shadow draw — 0x470AC0

Signature (cdecl, call site 0x4478A3–0x4478B2):
`draw_player_shadow(player, &stage_shadow, &stage_reflection)`.

### 3.1 Early-outs

1. `engine+0x118B8` == 0 → return (0x470B5D).
2. `*(player+0x1418) + 0x2C == -1` → return: the current animation has no
   sprite (0x470B81–0x470B91).
3. Optional owner indirection: if `player+0x141C != -1` and
   `player+0xC48 != 0`, the draw object is replaced by
   `engine+0x11230[player+0x141C]` (0x470B97–0x470BAB). This is the
   palette/sprite owner used by Helpers, not a shadow color.

The visibility and `noshadow` gates are in the caller, not here.

### 3.2 Geometry

The sprite is the player's current animation frame (`player+0x1418`,
sprite index `+0x2C`). Position/scale is assembled from:

- `player+0x7C` and `player+0x80` (position), multiplied by the character
  scale `*(player)+0xA4` (0x470BB9–0x470C17);
- the per-frame draw-transform fields `player+0x13A8`, `+0x13AC`,
  `+0x13B0` (the same block is memset at 0x46AB5A and re-seeded with 1.0 at
  0x46AB80/0x46AB90 — the field semantics were not independently named);
- the ground-plane projection constants
  `engine+0x10F14 × engine+0x1B74 + engine+0x10EFC` and `engine+0x80C8`,
  used identically in the object path (0x4737A2–0x4737D7), so it is the
  shared floor projection, not player data;
- the vertical scale from the stage record: `fmuls 0x8(%ebx)` at 0x470EC6,
  where `ebx = &engine+0x1C24`, i.e. `[Shadow] yscale`. A negative yscale
  flips the projection, matching `bgs.html:396-399`.

The same block is used for the reflection: when `*(&engine+0x1C38) > 0`,
0x4189F0 is called with mirrored geometry (0x470E49; object path 0x4737E2).

### 3.3 Alpha, color and blending

`ebx = &engine+0x1C24`; fields in that record are:
`+0x00` intensity (`engine+0x1C24`), `+0x04` packed color (`+0x1C28`),
`+0x08` yscale (`+0x1C2C`), `+0x0C` fade top (`+0x1C30`),
`+0x10` fade bottom (`+0x1C34`).

1. Base alpha `A = 0x100` (0x470F17).
2. If `player+0x13E4 == 3` (Trans type `addalpha`), `A = 0x100 − player+0x13EC`
   (0x470F08–0x470F2C).
3. If `engine+0x1C30 < engine+0x1C34` (top < bottom), the player depth value
   `player+0x1F8` is ramped (0x470F42–0x470F90). The ramp **multiplies** the
   alpha saved in step 2 rather than replacing it:
   - depth ≤ top ⇒ `A = 0` (0x470F55);
   - depth ≥ bottom ⇒ `A` unchanged (0x470F6A → 0x470F92);
   - otherwise `A = (depth − top) × A / (bottom − top)` (0x470F79–0x470F8B).
   So `addalpha` and `fade.range` compose multiplicatively. `player+0x1F8` is
   MUGEN's stage-y depth (larger is lower); this engine stores height above the
   floor, so the value it compares is the negation of that height.
4. If `A < 0x100`, the packed RGB is multiplied component-wise by `A` and
   shifted right 8 (0x470F9A–0x470FDB). This pre-scale uses the
   **pre-intensity** `A` from step 3.
5. Intensity is then applied to the alpha, not to the tint: `A = A × intensity
   >> 8` (`imull 0x34(%esp),%ecx` at 0x470FBA, `sarl $8`). The intensity is the
   stage record's `+0x00` field, saved to that stack slot earlier in the
   function. If `A ≥ 0x100` after step 3, the whole scale block is
   skipped (0x470F98 → 0x470FE4): the tint is left unscaled and the blit alpha
   is the intensity value itself.
6. If the resulting tint differs from the un-tinted color **or** `A != 0x100`,
   the shadow is drawn in two passes:
   - 0x4193C0 with the scaled tint and alpha `A` (0x471039);
   - 0x418A30 with color 0 and alpha `0x100 − A` (0x47105E region).
   If `A == 0x100` and the tint is unchanged, the shadow is skipped
   (0x470FEC–0x471004), since a fully opaque black shadow would be a plain
   sprite copy.

This engine's approximation of the composite: one sprite draw with the stage
colour as the tint and the post-intensity `A` as `srcAlpha`. The reference's
first pass tints a sprite by the pre-intensity `A` while its coverage uses the
post-intensity `A`, and the second pass covers the rest; a renderer that
multiplies tint by texel cannot express both alphas in one draw, so the tint is
not pre-scaled. Recorded in `PARITY-ROADMAP.md` S12.


The two blit helpers (0x4193C0 / 0x418A30) do float coordinate/clip and
alpha arithmetic only. They contain no Clsn, frame-data, or state reads
(prologue inspected; 0x418A30 does not reference any player/anim struct
beyond the surface passed to it).

---

## 4. Object shadows (projectiles, explods, helpers' objects)

### 4.1 Per-player object list — 0x473570 → 0x473600

`0x473570(list)` walks the list owned by the player (`player+0x268`). It
skips empty slots, and for live elements:

- live/element state at `element+0x04` must be 1..4 (`decl; cmpl $3; ja`,
  0x473590–0x473597);
- color at `element+0x40`:
  - `0` → no shadow (0x47359C);
  - `-1` → replace with `engine+0x1C28` (stage color, 0x4735A6–0x4735AB);
- calls `0x473600(element, &engine+0x1C24, color, &engine+0x1C38)`.

The element slot stride is 0x2E0 bytes (allocator setup at 0x472C20–0x472C65;
`element_size` at list `+0x04`). The append helper 0x472CB0:

- allocates a slot via 0x403280,
- memcpy's a full element template from the caller's stack via 0x4030F0,
- then copies 14 dwords (0x38 bytes) from `player+0x1418` into
  `element+0x288` (0x472D09–0x472D16).

That anim-record overlay is why `element+0x2B4` equals the animation record's
`+0x2C` sprite index. `0x473600` uses it as a gate:

```
0x473698  movl 0x2B4(%ebp),%eax
0x4736A2  cmpl $-1,%eax
0x4736A5  je   0x473A3C          ; shadow uses sprite index -1 => skip
```

(It is **not** an alpha/intensity field.)

### 4.2 Object draw — 0x473600

- `engine+0x118B8` gate at 0x473681.
- Sprite/anim from `element+0x298` (anim) and the copied sprite index.
- Facing at `element+0x34`, y at `element+0x48`, and the element transform
  fields `+0x4C/+0x50/+0x54/+0x58`.
- Ground projection identical to the player path
  (`engine+0x10F14 × engine+0x1B74 + engine+0x10EFC`, `engine+0x80C8`),
  yscale from the stage record (`0x8(%edx)`, 0x473839).
- `fade.range`: compares `element+0x48` to stage `+0x0C` (top) and `+0x10`
  (bottom) at 0x47386C–0x4738D7 and derives the same 0..0x100 ramp.
- Reflection when reflection intensity > 0 (0x473749).
- Two-pass blend exactly like the player path: 0x4193C0 at 0x473972,
  0x418A30 at 0x4739DB with `0x100 − alpha` (the alpha value is derived
  from the stage intensity/fade; the element contributes only color and
  sprite).

### 4.3 Global object list — 0x445060 → 0x4450D0

Entries are 0x108 bytes (`addl $0x108,%ebx`, 0x4450B6).

- empty slot (`entry+0x00 == 0`) → skip (0x44507C);
- shadow color at `entry+0x70`: `0` skip, `-1` → `engine+0x1C28`,
  otherwise use as packed (0x445081–0x44509A);
- calls `0x4450D0(entry, &engine+0x1C24, color, &engine+0x1C38)`.

`0x4450D0` repeats the same gates and math: `0x118B8` at 0x44515C, anim
pointer at `entry+0x80`, sprite index `anim+0x2C` at 0x445175, yscale,
fade.range, reflection and the two-pass blend.

### 4.4 Explod `shadow`

The Explod parameter parser reads keys around 0x47CA50–0x47CB43:

- `vfacing` 0x4A3F1C → template +0x128/0x12C/0x14C (0x47CA53–0x47CAC0),
- `shadow` 0x4A3F24 → template +0x104/+0x110/+0x11C (0x47CA84 region),
- `ownpal` 0x4A3F2C → template +0x128 (0x47CAE8),
- `removeongethit` 0x4A3F34 → template +0x134 (0x47CB0F).

At object update/creation, 0x445E35–0x445F2A evaluates the three values:

```
0x445E47  cmpl $0x80000000,%esi   ; first value absent => leave +0x70 alone
0x445E7B  cmpl $-1,%esi ; jl error
0x445E84  cmpl $-1,%ecx ; jl error
0x445E89  cmpl $-1,%eax ; jl error
0x445E8E  cmpl $0xff,%esi ; jg error
... same for g and b
0x445EA5  testl %edi,%edi ; je 0x445EEA
0x445EB2  pushl $0x49F904          ; "Illegal shadow color"
0x445EE5  cmpl $-1,%esi            ; r == -1?
0x445EEA  movl $-1,0x70(%ebx)      ; stage-color sentinel
0x445EF3  ... SDL_MapRGB(...)
0x445F22  call 0x4991C8
0x445F2A  movl %eax,0x70(%ebx)     ; runtime shadow color
```

Notes:

- default is 0,0,0, and 0 means "no shadow" at the draw site;
- the runtime field is the global object's `+0x70`;
- only `r == -1` selects the stage color. The validation accepts `g == -1`
  or `b == -1` too, but those are passed to SDL_MapRGB as ordinary values;
  docs (`sctrls.html:1006-1011`) document only `shad_r = -1`;
- values below −1 or above 255 emit `"Illegal shadow color"` (0x49F904)
  and fall back to `+0x70 = -1` (stage color).

### 4.5 Projectile `projshadow`

The Projectile parser reads `projshadow` 0x4A4918 at 0x48101B–0x481047 into
template +0xC4/+0xD0/+0xDC. At spawn, 0x48123F–0x4812B2 evaluates and packs:

```
0x48123F  eval template +0xC4 -> esi (r)
0x481251  eval template +0xD0 -> edi (g)
0x481265  eval template +0xDC -> eax (b)
0x481279  any < -1              -> error
0x481288  any > 0xFF            -> error
0x48129F  any == -1             -> 0xFFFFFFFF sentinel
0x4812AE  push b; push r; edx=g; call 0x411170   ; SDL_MapRGB wrapper
0x4812BA  movl %eax,0x60(%esp)  ; packed color in the element template
0x4812CA  ... "illegal shadowcolor" 0x4A49BC at 0x4812D3
```

The element template starts at stack `+0x20`; `+0x60` is template offset
`+0x40`, i.e. `element+0x40`. The spawn call
`0x472CB0(player+0x268, &template, ...)` reaches 0x4030F0, which memcpy's
`list->element_size` bytes from the template into the new element. Therefore
the projectile's packed shadow color really does become `element+0x40`,
the field read by 0x473570. (This closes the "live draw field" gap recorded
in `PROJECTILE-PARAMS-RE.md` §4.7.)

Sentinel behavior is stricter than the docs: **any** of r/g/b equal to −1
produces the `-1` "use stage color" sentinel, not only `r`. Values outside
−1..255 emit `"illegal shadowcolor"`.

---

## 5. Trans interaction

`Trans` is a per-tick controller. Its parameters are read at template
offsets +0x18/+0x24/+0x30 and stored at:

- `player+0x13E4` type (default −1),
- `player+0x13E8` src alpha (default 0x100, clamped 0..0x100),
- `player+0x13EC` dst alpha (default 0, clamped 0..0x100).

Parser: 0x47953B–0x4795AF. Per-tick reset: 0x46AB5A memsets 0x48 bytes at
`player+0x13A8`, then 0x46AB86 writes `-1` to `+0x13E4`, `0x100` to
`+0x13E8`, and 0 to `+0x13EC` (0x46AB96–0x46ABA0).

Shadow interaction: only type 3 (`addalpha`) is consulted, and only the
**dst alpha** is used, as `A = 0x100 − dst` before the fade ramp. Other
types (`add`, `add1`, `sub`, default) do not affect the shadow. This means a
fully transparent normal actor (`trans = addalpha,256,0`) keeps a full
shadow; `trans = addalpha,0,256` removes it.

Docs: `sctrls.html:2879-2905` (default/none/add/addalpha/add1/sub;
src,dst 0–256, defaults 256,0).

---

## 6. Animation `Clsn`, frame data, and sprite axis

- No shadow function reads collision boxes. 0x470AC0, 0x473570, 0x473600,
  0x445060, 0x4450D0, 0x4189F0 and 0x418A30 contain no access to the Clsn
  arrays or frame-data structs.
- The shadow is a copy of the **current sprite**, so the animation frame
  naturally selects *which* pixels are shadowed; frame-data timing/Clsn does
  not move or size the shadow.
- The sprite axis is not read as a separate field in the shadow functions.
  The blit helpers receive the sprite index, position and scale; whether the
  axis is applied inside 0x4189F0/0x418A30 was not established (their full
  coordinate math was not decoded). The safe engine rule is: treat the shadow
  like the normal sprite's blit origin with a vertical scale applied, not like
  a Clsn-derived rectangle.

---

## 7. Helpers

Helpers live in the same player table (`engine+0x11230`) and therefore take
the first shadow path with their own anim, position, facing, draw transform
and `player+0x254` assert byte. Their shadows use the same stage parameters;
there is no helper-specific shadow color or intensity.

`[Size] shadowoffset` (Helpers, key 0x4A20A0; character Constants key
0x4A27E4) is parsed at 0x467A8B–0x467A9C into a constants slot at `+0x31C`.
No read of that slot exists in any shadow function; see §8.

---

## 8. What could not be established

1. **`shadowoffset` consumption.** The key is definitely parsed (0x467A8B),
   but there is no `+0x31C` read in any shadow path. The only other accesses
   to `+0x31C` are in the per-frame routine around 0x4654D0
   (0x46588A/0x465896/0x465920/0x465A32), where the value is compared with a
   non-shadow value; whether the Constants parser writes the live player or a
   template copied elsewhere was not traced. Do **not** guess a horizontal
   offset from this.
2. **Sprite axis inside the blit helpers.** 0x4189F0/0x418A30 were only
   inspected for what they read; their axis handling and exact blend
   equation are not decoded.
3. **Field names for the draw transform** (`player+0x13A8/+0x13AC/+0x13B0`)
   and **`player+0x1F8`** (the depth value used by `fade.range`). Their roles
   are inferred from use, not from a named accessor.
4. **The alive flag `player+0x1B0`** is required by the draw loop but was not
   identified beyond "non-zero".
5. **Explod `g == -1` / `b == -1`.** Validation accepts them and only `r` is
   tested for the stage-color sentinel; the resulting SDL_MapRGB behavior was
   not run. The docs only promise `shad_r = -1`; matching the docs is safe.
6. **Per-tick clearing of `engine+0x11658`.** Player assert bytes are clearly
   cleared per tick in 0x46A7F7–0x46A803; for the global array only a
   round/init clear at 0x4592E2 was observed, so the exact lifetime of
   `globalnoshadow` beyond "asserted while the controller runs" is MEDIUM.
7. **Whether `noshadow` should hide a helper's projectiles.** The binary
   answer is no (object loops have no per-player assert test); this is a
   behavior choice the engine must copy if parity matters.

---

## 9. What this engine must do

- [ ] Keep shadow appearance **stage-owned**: `[Shadow] color`, `intensity`,
      `yscale`, `fade.range`; do not invent per-player shadow color/alpha in
      character data or `[Data]`.
- [ ] Parse `[Shadow]`: intensity default 128 (0–256 by docs, no binary
      clamp), color r,g,b default 0,0,0 (higher value = less color), yscale
      default 0.4 (negative flips), fade.range top,bot default off.
- [ ] Parse `[Reflection] intensity` (not `reflect`), default 0; draw
      reflections only when intensity > 0. Treat `bgs.html`'s `reflect` as
      stale.
- [ ] Honor `[Config] drawshadows` (default on) as a global kill switch.
- [ ] Render every player/helper's shadow from its **current animation
      sprite**, vertically scaled by `[Shadow] yscale`, projected onto the
      ground plane; skip when the current sprite index is −1.
- [ ] Apply the alpha chain: base 0x100 → replace with `0x100 − dst_alpha`
      when `Trans` type is `addalpha` → multiply by the `fade.range` ramp
      (depth vs top/bottom, 0 above top, 256 below bottom, linear between) →
      pre-scale RGB by `alpha/256` and draw the shadow with the two-pass
      alpha blit (tint at A, black at 256−A).
- [ ] `AssertSpecial(noshadow)` hides only that player/helper's own shadow;
      `AssertSpecial(globalnoshadow)` hides all player, helper, projectile and
      explod shadows.
- [ ] Draw object shadows in the same order as the reference: players →
      per-player object list → global object list.
- [ ] Per-player object elements: color `0` = no shadow, `-1` = stage
      `[Shadow] color`, otherwise the packed explicit color.
- [ ] Explod `shadow = r,g,b`: default 0,0,0 = none; `r == −1` = stage color;
      out-of-range → the same `"Illegal shadow color"` diagnostic and stage
      color fallback. Store the resolved color per explod object.
- [ ] Projectile `projshadow = r,g,b`: default 0,0,0 = none; **any**
      component −1 → stage color; out-of-range → `"illegal shadowcolor"`;
      store the packed color on the projectile object (`+0x40` equivalent).
- [ ] Do not gate projectile/explod shadows on the owning player's
      `noshadow`; only `globalnoshadow` (and the global drawshadows switch)
      suppress them.
- [ ] Never derive shadow geometry from Clsn boxes or frame data.
- [ ] Leave `size.shadowoffset` unimplemented (or accepted-but-inert) until
      its consumer is established; do not guess an offset.

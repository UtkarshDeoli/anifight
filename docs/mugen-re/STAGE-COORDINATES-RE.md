# Stage coordinate spaces — behaviour notes

How a stage's numbers become screen positions in this engine, what the reference
documentation says, and where the two deliberately differ. Written after the
large-stage work (2026-10-19) that made `The Roof` (1280×720), `Chamber of Souls`
(960×720) and `Red Moon` (640×480) render at the right size and put their floors
and fighters where their authors drew them.

Golden rule applies: this is a behaviour note. No decompiled or recompiled code
is involved, and nothing here is copied into `app/`.

## 1. What a stage's numbers mean

From `local_mugen/extracted/mugen/docs/bgs.html` (StageInfo Group):

> `localcoord = width, height` — Dimensions of the coordinate space of the stage.
> Defaults to 320, 240 if omitted.
>
> `xscale = xscale` — Horizontal scaling factor of offsets, velocities, sprites
> and animations.
>
> `yscale = yscale` — Vertical scaling factor of offsets, velocities, sprites
> and animations.

Every other number in the `.def` — `zoffset`, `start`, `delta`, `velocity`,
`tile`, `tilespacing`, `window`, `[Camera]` bounds, `[PlayerInfo]` starts and
bounds, `[Bound]` margins, `[Shadow] fade.range` — is in that local space. The
parser keeps them that way (`StageDef` stores raw stage units); translation is a
consumer-side concern, which is why the same `StageDef` can be drawn into a menu
preview and a fight window of any size.

`localcoord` is sanitised to 320×240 when missing or non-positive. A stage that
writes `localcoord = 0,0` is malformed, and leaving the translation infinite
would be worse than treating it as the default.

## 2. What the reference does, and what this engine does

`coordspace.html` states the rule and then an important limitation:

> The translation scaling factor of vectors between coordinate spaces is the
> ratio of the target coordinate space's width and the source coordinate
> space's width. For example, a character made for a 320x240 coordinate space
> will be scaled by a factor of 4 to operate in a 1280x720 coordinate space.
>
> [Stages] … Stages contain additional parameters to control cropping if the
> game aspect is shorter than the stage's coordinate space aspect, and to
> control overdraw if the game aspect is taller than the stage's aspect.
> **The height parameter of localcoord is not used.**

So the reference translates stages by **width** ratio into a fixed game
coordinate space (mugen.cfg `GameWidth`/`GameHeight`), and handles aspect
mismatch with `cuthigh`/`cutlow` and `overdrawhigh`/`overdrawlow`.

This engine's fight window is different by construction: `kWindowHeight = 240`
is fixed (the screenpack's authored height) and the width follows the device's
aspect (`clamp(240 × deviceAspect, 320, 560)`; 502 on the 2340×1080 test device).
The width is therefore an output, not an invariant, and a width-ratio
translation would change the vertical composition on every device:

- `The Roof` at 1280×720 into a 502×240 window: width ratio 502/1280 = 0.392,
  so `zoffset = 670` lands at **263**, below the bottom of a 240-high screen —
  the ground would be off-screen.
- Height ratio 240/720 = 0.333 puts it at **223**, the same fraction of the
  window the author drew (670/720 = 93%).

The engine therefore translates by **height ratio**:

```cpp
sourceHeight = localCoord.y > 0 ? localCoord.y : 240;
base = viewportHeight / sourceHeight;
translation.x = base * (scaleX > 0 ? scaleX : 1);
translation.y = base * (scaleY > 0 ? scaleY : 1);
```

`stageTranslation()` in `engine/stage.cpp`. The result is applied to every
stage-unit quantity, including `xscale`/`yscale`, which MUGEN defines as extra
drawing scale on top ("Horizontal/Vertical scaling factor of offsets, velocities,
sprites and animations").

This is a deliberate adaptation, not a claim that MUGEN does it this way. It
chooses the reference's *intent* — the stage keeps the composition its author
drew, with the floor at the same fraction of the screen — for a game window
whose aspect is an input rather than a constant. `xscale`/`yscale` still layer
on top exactly as documented.

Resulting factors seen in the local corpus (240-high window):

| localcoord | factor | examples |
|-----------|--------|----------|
| 320×240 | 1.0 | Training Ground, Sunset Rooftop, Konoha Destroyed, City/Bridge/Military/Nature (XX'DB') |
| 640×480 | 0.5 | Red Moon |
| 880×720 | 1/3 | Witch's Ruins |
| 960×720 | 1/3 | AP_Playground, Chamber of Souls |
| 1280×720 | 1/3 | The Roof, Training Room 720 (`stage0-720`) |

## 3. Placing a background layer

`BackgroundPlacement` (`engine/motif.h`) carries the mapping once and every
element goes through it:

```cpp
struct BackgroundPlacement {
    Vec2 origin;               // screen point that `anchor` maps to
    Vec2 anchor;               // stage-space point that maps to `origin`
    Vec2 scale{1.0f, 1.0f};    // stage units -> screen units
};
```

The fight draws with `origin = (virtualWidth/2, 0)`, `anchor =
(localCoord.x/2, 0)`, `scale = stageScale_`. Menus call `drawBackground` with
the default placement (origin 0,0, anchor 0,0, scale 1,1) and are unchanged.

`start`, per `bgs.html`:

> Specifies the background element's starting position with respect to the top
> center of the screen (positive y values go downward). The background
> element's axis (the one specified for the designated sprite in the SFF) is
> placed at this starting position.

So a layer draws at:

```
x = origin.x + (start.x + frameOffset.x) * scale.x
    - camera.x * delta.x + velocity.x * ticks * scale.x
    - sprite.axis.x * scale.x
y = origin.y + (start.y + frameOffset.y) * scale.y
    - camera.y * delta.y + velocity.y * ticks * scale.y
    - sprite.axis.y * scale.y
```

`start` is already centre-relative, so no anchor subtraction is needed for it.
The sprite axis is a stage-space length and scales. `delta` is documented as
"how many pixels the background element should scroll for each pixel of camera
movement" — the camera is passed in fight/screen units, so `camera * delta`
needs no extra scale. `velocity` is stage pixels per tick and does scale.

`window` is different: `bgs.html` says it is in screen coordinates "considering
the upper left corner of the screen as (0,0)". That is an *absolute* stage-space
rectangle, so the anchor must be subtracted:

```
clipX1 = origin.x + (w.x1 - anchor.x) * scale.x
clipY1 = origin.y + (w.y1 - anchor.y) * scale.y
```

Tiled layers fold their position into one tile (`fmod`) before repeating so the
offset cannot grow without bound.

Animated layers (`type = anim`) go through the same placement: the frame's
AIR offset is stage pixels and scales, and the frame's own `trans` wins over the
element's, matching the HUD animation rule.

## 4. The non-drawing consumers

A stage's local space is not only art. The fight screen resolves these once per
match in `setupMatch()` (`engine/screens/fight_screen.cpp`):

- **Floor**: `floorY_ = zoffset * translation.y`. `zoffset` is documented as
  "the vertical distance of the ground level from the top of the screen".
- **Fighter starts**: `startX = kStageOriginX + p1startx * translation.x`.
  `kStageOriginX` (320) is this engine's arena middle; the camera and all world
  x positions are in the same left-origin space.
- **Camera travel**: `cameraBounds()` maps `[Camera] boundleft/boundright`
  around `kStageOriginX` with `translation.x`.
- **Walk limits**: the camera ends plus the window half-width, pulled in by
  `[Bound] screenleft/screenright * translation.x`, then intersected with
  `[PlayerInfo] leftbound/rightbound * translation.x`. A contradictory pair
  falls back to the camera's own ends rather than welding a fighter in place.
  The same pair is handed to helpers when they spawn.
- **Shadow fade**: `[Shadow] fade.range` is a pair of player *y* coordinates and
  therefore scales with `translation.y`. `[Shadow] yscale` is a unitless factor
  and does not.
- **Projectile stage bounds**: `projstagebound` defaults are the camera ends,
  translated with `translation.x`.
- **Camera origin**: the camera position is the midpoint of the translated
  starts, and `drawStage` passes `scrollX - kStageOriginX` to the background.

## 5. Sprite file resolution (a bug this work fixed)

MUGEN resolves `[BGdef] spr` from the data root, not from the .def's folder.
Stages in `stages/` legally write either spelling:

```ini
spr = roof.sff          ; bare
spr = stages/roof.sff   ; fully qualified
```

The engine used to prepend the .def's folder unconditionally, so the second form
asked for `stages/stages/roof.sff`. Three corpus stages (`The Roof`,
`Chamber of Souls`, and another using the qualified form) loaded as black
screens with only a log warning. `resolveStageFile()` now returns a reference
that already starts with the folder unchanged, takes absolute paths as written,
and still resolves a bare name inside the folder. Pinned by
`tools/hosttest/test_stage_shadow.cpp`'s "sprite file resolving" block.

## 6. Verification

Host (`tools/hosttest/run_all_tests.sh`): 20 ran, 0 skipped, 0 failed. The
"stage coordinate space" block checks defaults (320×240 → 1,1), 1280×720 →
1/3, the roof floor 670 → 223, the roof starts ±280 → ±93.3, 640×480 → 0.5,
`xscale`/`yscale` multipliers, and the zero-localcoord fallback.

Device (RZ8N22ALBMN, 502×240 fight window), from `adb logcat` plus screenshots:

| stage | localcoord | log | result |
|-------|-----------|-----|--------|
| The Roof | 1280×720 | `scale 0.333,0.333 floor 223, camera -50..67` | art fills the window, moon/railing visible, fighters grounded |
| Chamber of Souls | 960×720 | `scale 0.333,0.333 floor 223, camera -300..300` | full chamber art, fighters grounded |
| Red Moon | 640×480 | `scale 0.500,0.500 floor 225, camera -47..48` | full cave art, fighters grounded |
| Sunset Rooftop | 320×240 | `scale 1.000,1.000 floor 176, camera -190..190` | unchanged from before the work |

## 7. Known gaps in this area (not fixed here)

- **Stage-embedded `[Begin Action n]`** is not parsed. `Roof` fans/Grua, the
  Chamber's mouthsouls and Red Moon's river waves use `type = anim` with an
  action written in the stage `.def`; the engine looks only in the screenpack
  `AnimationSet`, so those layers are skipped. (Red Moon's large flat magenta
  rectangle is a `trans = add1` "cover" layer, not this gap.)
- **Parallax `xscale = a,b`** and `yscalestart`/`yscaledelta` (bgs.html
  "Parallax" section) are parsed neither as pairs nor applied. `xscale`/`yscale`
  here are single `[StageInfo]` numbers only.
- **Vertical camera**: `verticalfollow`, `floortension`, `boundhigh`,
  `boundlow` are not implemented; the camera is horizontal only.
- **`overdrawhigh`/`overdrawlow`** and **`cuthigh`/`cutlow`** are not
  implemented. They exist to adapt a stage's aspect to the game's; the
  height-ratio choice above removes the immediate need for the local corpus,
  but a stage with authored crop hints would be drawn without them.
- **`tilespacing`** and **`windowdelta`** are not applied (tiling repeats at
  the sprite's own size).
- **OLD `hires = 1`** stages that predate `localcoord` (320×240 art intended to
  be doubled) still need a rule.

#!/usr/bin/env python3
"""Generates Anifight's stage art and stage .def files.

A MUGEN stage is a `.def` naming a sprite file and a list of `[BG n]` layers,
plus the numbers that make it a place rather than a picture: where the floor is,
how far the camera may travel, and where the two fighters start. All of that is
written here, so each `stages/<id>.def` and its `.sff` are two views of one
description and cannot drift apart.

Like `tools/screenpack/gen.py`, the SFF/PCX writer is reimplemented here rather
than imported from it, so that each generator is an independent reading of the
format it writes.

Usage:
    python3 tools/stages/gen.py [output_dir]

Writes <output_dir>/<id>.{def,sff}, which the Gradle asset step copies into the
APK under mugen/stages/.
"""

from __future__ import annotations

import os
import struct
import sys

# ---------------------------------------------------------------------------
# Palette
# ---------------------------------------------------------------------------
#
# Every stage gets its own palette, but they all lay it out the same way: the
# same meaning at the same index, so the drawing code is shared and only the
# colours differ. In SFF v1 the palette lives in each image block rather than in
# the header, so two stages can be entirely different colours and still be the
# same format.
#
# Index 0 is the transparent colour, which is a convention rather than something
# the file records.

TRANSPARENT = (255, 0, 255)

OUTLINE = 1
FAR = 2
FAR_WINDOW = 3
NEAR = 4
NEAR_WINDOW = 5
FLOOR_LIGHT = 6
FLOOR_DARK = 7
FLOOR_LINE = 8
FLOOR_SHADOW = 9
ACCENT = 10
SKY_START = 11
SKY_STEPS = 32


class Skin:
    """One stage's colours."""

    def __init__(self, outline, far, far_window, near, near_window, floor_light, floor_dark,
                 floor_line, floor_shadow, accent, sky_top, sky_mid, sky_low):
        self.slots = [None] * SKY_START
        self.slots[OUTLINE] = outline
        self.slots[FAR] = far
        self.slots[FAR_WINDOW] = far_window
        self.slots[NEAR] = near
        self.slots[NEAR_WINDOW] = near_window
        self.slots[FLOOR_LIGHT] = floor_light
        self.slots[FLOOR_DARK] = floor_dark
        self.slots[FLOOR_LINE] = floor_line
        self.slots[FLOOR_SHADOW] = floor_shadow
        self.slots[ACCENT] = accent
        self.sky_top, self.sky_mid, self.sky_low = sky_top, sky_mid, sky_low
        # Filled in by the stage that owns this skin; the sky needs to know how
        # much of the screen it is spanning.
        self.floor_y = 190


def palette_for(skin: Skin) -> list:
    """A stage's 256 colours.

    The sky is a ramp rather than a handful of stops because a sky is a gradient
    in every MUGEN stage ever drawn; having the steps in the palette rather than
    interpolating at draw time keeps the art and the engine equally simple.
    """
    palette = list(skin.slots)
    palette[0] = TRANSPARENT

    for i in range(SKY_STEPS):
        t = i / (SKY_STEPS - 1)
        if t < 0.62:
            u = t / 0.62
            a, b = skin.sky_top, skin.sky_mid
        else:
            u = (t - 0.62) / 0.38
            a, b = skin.sky_mid, skin.sky_low
        palette.append(tuple(int(a[c] + (b[c] - a[c]) * u) for c in range(3)))

    while len(palette) < 256:
        palette.append((0, 0, 0))
    assert len(palette) == 256, len(palette)
    return palette


# ---------------------------------------------------------------------------
# Canvas
# ---------------------------------------------------------------------------


class Canvas:
    """An indexed-colour image, one byte per pixel."""

    def __init__(self, width: int, height: int, fill: int = 0):
        self.w = width
        self.h = height
        self.px = bytearray([fill]) * (width * height)

    def set(self, x: int, y: int, c: int) -> None:
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y * self.w + x] = c

    def rect(self, x: int, y: int, w: int, h: int, c: int) -> None:
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, c)


# ---------------------------------------------------------------------------
# PCX and SFF v1
# ---------------------------------------------------------------------------


def pcx_rle_row(row: bytes) -> bytes:
    """PCX run-length encoding for one scanline."""
    out = bytearray()
    i = 0
    while i < len(row):
        run = 1
        while i + run < len(row) and row[i + run] == row[i] and run < 63:
            run += 1
        if run > 1 or row[i] >= 0xC0:
            out.append(0xC0 | run)
            out.append(row[i])
        else:
            out.append(row[i])
        i += run
    return bytes(out)


def pcx_block(canvas: Canvas, palette: list) -> bytes:
    header = bytearray(128)
    header[0] = 0x0A
    header[1] = 5
    header[2] = 1
    header[3] = 8
    struct.pack_into("<HHHH", header, 4, 0, 0, canvas.w - 1, canvas.h - 1)
    struct.pack_into("<HH", header, 12, 72, 72)
    header[65] = 1
    struct.pack_into("<H", header, 66, canvas.w)

    body = bytearray()
    for y in range(canvas.h):
        body += pcx_rle_row(bytes(canvas.px[y * canvas.w:(y + 1) * canvas.w]))
    flat = bytes(v for rgb in palette for v in rgb)
    return bytes(header) + bytes(body) + b"\x0c" + flat


class Sprite:
    def __init__(self, group: int, image: int, canvas: Canvas, axis_x: int = 0, axis_y: int = 0):
        self.group = group
        self.image = image
        self.canvas = canvas
        self.axis_x = axis_x
        self.axis_y = axis_y


def write_sff_v1(path: str, sprites: list, palette: list) -> None:
    """Writes SFF revision 1: a 512-byte header, then a linked list of 32-byte
    subheaders each followed by its own PCX image."""
    blobs = [pcx_block(s.canvas, palette) for s in sprites]

    header = bytearray(512)
    header[0:12] = b"ElecbyteSpr\x00"
    header[0x0C:0x10] = bytes([0, 0, 0, 1])
    struct.pack_into("<I", header, 0x10, len({s.group for s in sprites}))
    struct.pack_into("<I", header, 0x14, len(sprites))
    struct.pack_into("<I", header, 0x18, 512)
    struct.pack_into("<I", header, 0x1C, 32)
    comment = b"Anifight stage\x00"
    header[0x24:0x24 + len(comment)] = comment

    body = bytearray()
    offset = 512
    for index, (sprite, blob) in enumerate(zip(sprites, blobs)):
        sub = bytearray(32)
        next_offset = 0 if index + 1 == len(sprites) else offset + 32 + len(blob)
        struct.pack_into("<I", sub, 0x00, next_offset)
        struct.pack_into("<I", sub, 0x04, len(blob))
        struct.pack_into("<h", sub, 0x08, sprite.axis_x)
        struct.pack_into("<h", sub, 0x0A, sprite.axis_y)
        struct.pack_into("<H", sub, 0x0C, sprite.group)
        struct.pack_into("<H", sub, 0x0E, sprite.image)
        body += sub
        body += blob
        offset += 32 + len(blob)

    with open(path, "wb") as f:
        f.write(bytes(header) + bytes(body))


# ---------------------------------------------------------------------------
# The art
# ---------------------------------------------------------------------------
#
# Laid out for a 320x240 screen. A stage's floor position appears here *and* in
# the .def as zoffset, because those are one decision seen twice: a stage whose
# art and zoffset disagree has its fighters standing in the air.

STAGE_W, STAGE_H = 320, 240


def make_sky(skin: Skin, floor_y: int) -> Canvas:
    """A full-screen gradient, one palette step per band.

    The ramp is compressed into the sky band rather than spread over the whole
    image, so the warm end lands on the horizon instead of below the floor where
    nothing can see it. A stage with a high floor therefore gets a compressed
    ramp, which is part of what makes a rooftop read as being up in the sky.
    """
    c = Canvas(STAGE_W, STAGE_H)
    span = max(1, floor_y)
    for y in range(STAGE_H):
        t = min(1.0, y / float(span))
        c.rect(0, y, STAGE_W, 1, SKY_START + int(t * (SKY_STEPS - 1) + 0.5))
    return c


def make_skyline(seed: int, height: int, floors: int, colour: int, window: int) -> Canvas:
    """A repeating band of buildings.

    Deterministic rather than random: the same stage must come out of the
    generator on every machine, or the generated art would differ between builds
    for no reason anyone could point at.
    """
    c = Canvas(STAGE_W, height)
    x = 0
    state = seed
    while x < STAGE_W:
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        width = 18 + (state >> 8) % 26
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        tall = height - 14 - (state >> 8) % max(1, height - 24)

        c.rect(x, tall, width, height - tall, colour)
        c.rect(x, tall, width, 2, OUTLINE)

        # Windows in a grid, offset per building so the band does not read as
        # one repeated stamp.
        step = max(6, floors * 2)
        offset = 3 + (state >> 16) % 4
        for wy in range(tall + 5, height - 4, step):
            for wx in range(x + offset, x + width - 3, 5):
                c.rect(wx, wy, 2, 2, window)
        x += width + 2
    return c


def make_floor(tile_w: int, tile_h: int) -> Canvas:
    """One tile of floor, repeated horizontally by the layer's `tile` setting."""
    c = Canvas(tile_w, tile_h)
    c.rect(0, 0, tile_w, tile_h, FLOOR_LIGHT)
    # A lit strip at the very top of the floor is the horizon line; everything
    # below it darkens toward the camera, which is what gives the plane depth.
    c.rect(0, 0, tile_w, 2, FLOOR_LINE)
    for y in range(2, tile_h, 4):
        c.rect(0, y, tile_w, 2, FLOOR_DARK)
    # A seam down the middle, so the tiling reads as paving rather than stripes.
    c.rect(tile_w // 2, 2, 1, tile_h - 2, FLOOR_SHADOW)
    c.rect(0, tile_h - 6, tile_w, 6, FLOOR_SHADOW)
    return c


def make_parapet(height: int) -> Canvas:
    """A railing band whose bottom edge is the roof line.

    Drawn in front of the distant city and behind the fighters, which is what
    puts the fight on top of a building rather than in front of one.
    """
    c = Canvas(STAGE_W, height)
    rail = max(0, height - 14)
    c.rect(0, rail, STAGE_W, 3, ACCENT)
    c.rect(0, height - 4, STAGE_W, 4, NEAR)
    for x in range(2, STAGE_W, 12):
        c.rect(x, rail, 2, height - rail, NEAR)
    return c


# ---------------------------------------------------------------------------
# Stages
# ---------------------------------------------------------------------------
#
# Each stage is a floor position, a camera width, a palette and a list of
# layers. The layer list is what the .def is generated from, so a stage's shape
# is described once and written to both files.
#
# `delta` is the whole of the parallax: 0 is pinned to the screen, 1 is pinned
# to the stage, and anything between is a layer at some distance. `tile` repeats
# a layer across the screen, which is what lets a 320-wide image cover a camera
# that moves.


class Layer:
    def __init__(self, name, group, start, delta, tile=(1, 0)):
        self.name = name
        self.group = group
        self.start = start
        self.delta = delta
        self.tile = tile

    def spriteno(self) -> str:
        return "%d,0" % self.group


class Stage:
    def __init__(self, stage_id, name, skin, floor_y, bound_left, bound_right,
                 far_top, far_h, near_top, near_h, parapet_h, note,
                 shadow=((0, 0, 0), 90, 0.5)):
        self.stage_id = stage_id
        self.name = name
        self.skin = skin
        self.floor_y = floor_y
        self.bound_left = bound_left
        self.bound_right = bound_right
        self.note = note
        self.shadow = shadow
        skin.floor_y = floor_y

        self.sprites = [
            Sprite(0, 0, make_sky(skin, floor_y)),
            Sprite(1, 0, make_skyline(17, far_h, max(1, far_h // 34), FAR, FAR_WINDOW)),
            Sprite(2, 0, make_skyline(91, near_h, max(1, near_h // 30), NEAR, NEAR_WINDOW)),
        ]
        self.layers = [
            Layer("sky", 0, (0, 0), (0.1, 0.1)),
            Layer("far", 1, (0, far_top), (0.35, 0.35)),
            Layer("near", 2, (0, near_top), (0.6, 0.6)),
        ]

        if parapet_h > 0:
            self.sprites.append(Sprite(3, 0, make_parapet(parapet_h)))
            self.layers.append(
                Layer("roof", 3, (0, floor_y - parapet_h + 4), (0.9, 0.9)))

        floor_group = len(self.sprites)
        self.sprites.append(Sprite(floor_group, 0, make_floor(32, STAGE_H - floor_y + 10)))
        self.layers.append(Layer("floor", floor_group, (0, floor_y), (1.0, 1.0)))

        self.palette = palette_for(skin)


TRAINING = Stage(
    "training", "Training Ground",
    skin=Skin(
        outline=(16, 14, 22),
        far=(58, 54, 104), far_window=(232, 200, 120),
        near=(34, 32, 64), near_window=(255, 176, 96),
        floor_light=(86, 78, 104), floor_dark=(52, 46, 66),
        floor_line=(120, 110, 140), floor_shadow=(26, 22, 38),
        accent=(196, 150, 84),
        sky_top=(14, 18, 48), sky_mid=(74, 58, 112), sky_low=(208, 120, 96),
    ),
    floor_y=190, bound_left=-160, bound_right=160,
    far_top=88, far_h=105, near_top=110, near_h=85, parapet_h=0,
    note="A night street: two skylines, a wide floor and a low horizon.",
    shadow=((0, 0, 0), 110, 0.5),
)

ROOFTOP = Stage(
    "rooftop", "Sunset Rooftop",
    skin=Skin(
        outline=(38, 22, 34),
        far=(92, 58, 96), far_window=(255, 214, 150),
        near=(58, 36, 68), near_window=(255, 236, 190),
        floor_light=(138, 112, 116), floor_dark=(96, 74, 84),
        floor_line=(214, 176, 156), floor_shadow=(64, 46, 58),
        accent=(244, 190, 120),
        sky_top=(48, 34, 96), sky_mid=(198, 96, 112), sky_low=(255, 190, 118),
    ),
    floor_y=176, bound_left=-190, bound_right=190,
    far_top=52, far_h=120, near_top=104, near_h=72, parapet_h=34,
    note="A rooftop at sunset: the floor sits higher, the camera travels "
         "further, and a railing is what the fighters stand behind.",
    shadow=((60, 24, 40), 80, 0.42),
)

STAGES = [TRAINING, ROOFTOP]


# ---------------------------------------------------------------------------
# The .def
# ---------------------------------------------------------------------------


def write_stage_def(path: str, stage: Stage) -> None:
    lines = [
        "; Anifight stage -- generated by tools/stages/gen.py.",
        ";",
        "; Edit the generator, not this file. The floor position here and the art",
        "; in the sprite file are one decision: the generator writes its floor_y",
        "; here as zoffset, and a stage whose art and zoffset disagree has",
        "; fighters standing in the air.",
        ";",
        "; " + stage.note,
        "",
        "[Info]",
        'name = "%s"' % stage.name,
        'author = "Anifight"',
        "localcoord = 320, 240",
        "",
        "[Camera]",
        "startx = 0",
        "starty = 0",
        "; How far the camera may travel either side of the stage origin. The",
        "; fight screen clamps to these, so they are the stage's width as far as",
        "; play is concerned -- a camera wider than the art would show the edge of",
        "; the world.",
        "boundleft = %d" % stage.bound_left,
        "boundright = %d" % stage.bound_right,
        "boundhigh = -25",
        "boundlow = 0",
        "verticalfollow = 0",
        "tension = 100",
        "",
        "[PlayerInfo]",
        "p1startx = -70",
        "p1starty = 0",
        "p2startx = 70",
        "p2starty = 0",
        "",
        "[Bound]",
        "screenleft = 15",
        "screenright = 15",
        "",
        "[StageInfo]",
        "zoffset = %d" % stage.floor_y,
        "autoturn = 1",
        "resetbg = 1",
        "localcoord = 320, 240",
        "",
        "[Shadow]",
        "; How the fighters' shadows are drawn. This belongs to the stage rather",
        "; than to the character: the same fighter casts a harder shadow on a",
        "; night street than on a rooftop at sunset, and nothing about the",
        "; fighter changes.",
        "color = %d,%d,%d" % stage.shadow[0],
        "intensity = %s" % stage.shadow[1],
        "yscale = %s" % stage.shadow[2],
        "",
        "[BGdef]",
        "spr = %s.sff" % stage.stage_id,
        "debugbg = 0",
        "",
    ]

    for layer in stage.layers:
        lines += [
            "[BG %s]" % layer.name,
            "type = normal",
            "spriteno = %s" % layer.spriteno(),
            "start = %d,%d" % layer.start,
            "delta = %s,%s" % layer.delta,
            "tile = %d,%d" % layer.tile,
            "trans = none",
            "",
        ]

    with open(path, "w") as f:
        f.write("\n".join(lines))


def main() -> int:
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "data"
    )
    os.makedirs(out_dir, exist_ok=True)

    for stage in STAGES:
        write_sff_v1(os.path.join(out_dir, stage.stage_id + ".sff"), stage.sprites,
                     stage.palette)
        write_stage_def(os.path.join(out_dir, stage.stage_id + ".def"), stage)
        print("  %s.def" % stage.stage_id)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

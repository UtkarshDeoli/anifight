#!/usr/bin/env python3
"""Generates Anifight's default screenpack.

Writes system.sff, fight.sff and the two bitmap fonts as real MUGEN files, from
art this script draws itself. Nothing here is derived from Elecbyte's data
folder, which is copyrighted and is not redistributed with this project.

This script is deliberately written against the published format specs rather
than against the C++ loader in app/src/main/cpp/gfx/. The two are independent
implementations of the same spec on purpose: if the writer were written by
reading the reader, a shared misreading of the format would produce files that
round-trip perfectly and that no real MUGEN tool could open. Keeping them
separate means the round-trip test actually tests something.

Usage:
    python3 tools/screenpack/gen.py [output_dir]

Defaults to tools/screenpack/data/, which is what the Gradle asset step copies
into the APK.
"""

from __future__ import annotations

import math
import os
import shutil
import struct
import sys
from collections import namedtuple

# ---------------------------------------------------------------------------
# Palette
# ---------------------------------------------------------------------------
#
# Index 0 is the transparent colour. In MUGEN's 8-bit sprites that is a
# convention rather than something the file records -- every decoder, ours
# included, hardcodes it -- so the actual RGB there is irrelevant. Magenta is
# used anyway so that a bug which loses the transparency is immediately obvious
# rather than subtly wrong.

TRANSPARENT = (255, 0, 255)

PALETTE = [
    TRANSPARENT,      # 0  transparent
    (255, 255, 255),  # 1  white
    (200, 208, 224),  # 2  light grey
    (140, 150, 170),  # 3  mid grey
    (80, 88, 104),    # 4  dark grey
    (28, 30, 40),     # 5  near black
    (232, 72, 72),    # 6  red
    (140, 32, 32),    # 7  dark red
    (96, 216, 128),   # 8  green
    (40, 120, 64),    # 9  dark green
    (96, 160, 255),   # 10 blue
    (40, 70, 150),    # 11 dark blue
    (255, 216, 96),   # 12 yellow
    (240, 152, 48),   # 13 orange
    (110, 226, 226),  # 14 cyan
    (208, 112, 224),  # 15 magenta
]

# A 48-step ramp from deep navy to pale blue, used for every background. Having
# a ramp in the palette rather than computing gradients at runtime keeps the
# generated art and the engine both simple.
RAMP_START = 16
RAMP_STEPS = 48
for _i in range(RAMP_STEPS):
    _t = _i / (RAMP_STEPS - 1)
    PALETTE.append((
        int(10 + 42 * _t),
        int(12 + 56 * _t),
        int(32 + 120 * _t),
    ))

# Metal ramp for the bars: dark at the ends, bright in the middle.
METAL_START = RAMP_START + RAMP_STEPS
METAL_STEPS = 32
for _i in range(METAL_STEPS):
    _t = _i / (METAL_STEPS - 1)
    _v = int(40 + 180 * (1.0 - abs(2 * _t - 1)))
    PALETTE.append((_v, int(_v * 0.94), int(_v * 0.78)))

# The bars' art is exactly as wide as the fill it represents, because the engine
# draws a partly-drained bar by cropping the sprite rather than scaling it. These
# widths are therefore not free: they must equal |range.x| in fight.def, and the
# two bars must leave a gap in the middle wide enough for the timer.
LIFEBAR_W, LIFEBAR_H = 115, 12
POWERBAR_W, POWERBAR_H = 99, 6

# Fill the rest of the 256 entries so every index a decoder might reference is
# defined rather than reading past the end of the palette.
while len(PALETTE) < 256:
    PALETTE.append((0, 0, 0))

assert len(PALETTE) == 256, len(PALETTE)


def palette_bytes() -> bytes:
    return bytes(v for rgb in PALETTE for v in rgb)


# ---------------------------------------------------------------------------
# Canvas
# ---------------------------------------------------------------------------


class Canvas:
    """An indexed-colour image, stored as one byte per pixel."""

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

    def frame(self, x: int, y: int, w: int, h: int, c: int) -> None:
        for xx in range(x, x + w):
            self.set(xx, y, c)
            self.set(xx, y + h - 1, c)
        for yy in range(y, y + h):
            self.set(x, yy, c)
            self.set(x + w - 1, yy, c)

    def hline(self, x: int, y: int, w: int, c: int) -> None:
        for xx in range(x, x + w):
            self.set(xx, y, c)

    def vline(self, x: int, y: int, h: int, c: int) -> None:
        for yy in range(y, y + h):
            self.set(x, yy, c)

    def blit(self, src: "Canvas", ox: int, oy: int) -> None:
        for y in range(src.h):
            for x in range(src.w):
                c = src.px[y * src.w + x]
                if c != 0:
                    self.set(ox + x, oy + y, c)


# ---------------------------------------------------------------------------
# PCX: the image format SFF v1 stores
# ---------------------------------------------------------------------------


def pcx_rle_row(row: bytes) -> bytes:
    """Run-length encodes one scanline.

    Runs never cross a row boundary. The decoder resets at `bytesPerLine` and
    drops any part of a run that would overflow the row, so an encoder that let
    runs span rows would produce an image sheared further left on every line.
    """
    out = bytearray()
    i = 0
    n = len(row)
    while i < n:
        value = row[i]
        run = 1
        while i + run < n and row[i + run] == value and run < 63:
            run += 1
        # A lone byte still needs the run prefix if its value looks like a run
        # marker, otherwise the decoder reads it as the start of one.
        if run > 1 or value >= 0xC0:
            out.append(0xC0 | run)
            out.append(value)
        else:
            out.append(value)
        i += run
    return bytes(out)


def pcx_block(canvas: Canvas) -> bytes:
    """A complete 8-bit PCX: 128-byte header, RLE pixels, 768-byte palette."""
    header = bytearray(128)
    header[0] = 0x0A              # manufacturer, the PCX magic
    header[1] = 5                # version
    header[2] = 1                # encoding: RLE
    header[3] = 8                # bits per pixel
    struct.pack_into("<hhhh", header, 4, 0, 0, canvas.w - 1, canvas.h - 1)
    header[0x41] = 1             # one colour plane
    struct.pack_into("<H", header, 0x42, canvas.w)   # bytes per line
    struct.pack_into("<H", header, 0x44, 1)          # palette info

    body = bytearray()
    for y in range(canvas.h):
        row = bytes(canvas.px[y * canvas.w:(y + 1) * canvas.w])
        body += pcx_rle_row(row)

    return bytes(header) + bytes(body) + palette_bytes()


# ---------------------------------------------------------------------------
# SFF v1
# ---------------------------------------------------------------------------


class Sprite:
    def __init__(self, group: int, image: int, canvas: Canvas,
                 axis_x: int = 0, axis_y: int = 0):
        self.group = group
        self.image = image
        self.canvas = canvas
        self.axis_x = axis_x
        self.axis_y = axis_y



# ------------------------------------------------------------- hit sparks
#
# MUGEN keeps its hit sparks in a `fightfx.air`/`fightfx.sff` pair of their own, in the
# screenpack rather than in any character, and a HitDef's plain `sparkno = 2` refers to
# *that* file: the `S` prefix is what means "this character's own animation". So a
# screenpack with no fightfx is one where every hit lands without a spark, which is what
# this one was.
#
# The numbering is Elecbyte's, read off the real fightfx.air: 0 light, 1 medium, 2 heavy
# and 3 a fourth for the hardest hits. 10/11/12 are the three *blood* sparks, not the
# guard spark -- a guard is 40, further down. The art is ours: a starburst that opens and
# fades, drawn additively, which is what the real one is.
#
# An entry is (canvas size, spoke reach, cool palette, what it is). The reach lengthens
# the spokes; `cool` swaps the hot white/yellow/orange palette for cyan and blue, which
# is the look the 10/11/12 group has always had here.
SPARK_ACTIONS = {
    0: (40, 2, 0, "light hit spark"),
    1: (40, 3, 0, "medium hit spark"),
    2: (40, 4, 0, "strong hit spark, the one a bare `sparkno = 2` means"),
    3: (40, 5, 0, "very strong hit spark"),
    4: (52, 6, 0, "the hardest hit spark, for characters that name it"),
    10: (40, 2, 1, "weak blood hit spark"),
    11: (40, 3, 1, "medium blood hit spark"),
    12: (40, 4, 1, "strong blood hit spark"),
}
SPARK_FRAMES = 6


def make_spark(size: int, reach: int, cool: int) -> list:
    """One spark: a starburst that opens outward and fades, six frames of it."""
    core_colour = 14 if cool else 1        # white, or cyan
    spoke_colour = 10 if cool else 12      # yellow, or blue
    tip_colour = 14 if cool else 13        # orange, or cyan
    centre = (size - 1) * 0.5
    frames = []
    for index in range(SPARK_FRAMES):
        canvas = Canvas(size, size)
        progress = index / float(SPARK_FRAMES - 1)
        radius = 2.0 + (size * 0.5 - 2.0) * progress * (1.0 + 0.2 * reach)
        for spoke in range(8):
            angle = spoke * (math.pi / 4.0)
            length = radius * (1.0 if spoke % 2 == 0 else 0.68)
            steps = max(1, int(length))
            for step in range(steps):
                if index > SPARK_FRAMES // 2 and step > steps * 0.7:
                    continue
                colour = spoke_colour if step < steps * 0.6 else tip_colour
                x = int(round(centre + math.cos(angle) * step))
                y = int(round(centre + math.sin(angle) * step))
                canvas.set(x, y, colour)
                if index < SPARK_FRAMES // 2:
                    canvas.set(x + 1, y, colour)
                    canvas.set(x, y + 1, colour)
        core = max(0, int(3 - 2.5 * progress))
        for dy in range(-core, core + 1):
            for dx in range(-core, core + 1):
                if dx * dx + dy * dy <= core * core + 1:
                    canvas.set(int(centre + dx), int(centre + dy), core_colour)
        frames.append(canvas)
    return frames


# The guard spark is not a starburst. A blocked hit is met with a flat wash of pale,
# cool light at the point of contact, wide and low, which opens sideways and fades --
# drawn additively like the other sparks, but kept in the cyan end of the palette.
GUARD_SPARK_W, GUARD_SPARK_H = 64, 24
GUARD_SPARK_FRAMES = 5


def make_guard_spark() -> list:
    """A flat, cool flash that spreads sideways across a block."""
    frames = []
    centre_x = (GUARD_SPARK_W - 1) * 0.5
    centre_y = (GUARD_SPARK_H - 1) * 0.5
    for index in range(GUARD_SPARK_FRAMES):
        canvas = Canvas(GUARD_SPARK_W, GUARD_SPARK_H)
        progress = index / float(GUARD_SPARK_FRAMES - 1)
        # The flash reaches full width almost at once and then thins and dims, so it
        # reads as a snap of light rather than something that swells and recedes.
        half_w = 8.0 + (GUARD_SPARK_W * 0.5 - 8.0) * min(1.0, progress * 1.6)
        half_h = 7.0 - 4.0 * progress
        core = 0.34 - 0.30 * progress
        for y in range(GUARD_SPARK_H):
            for x in range(GUARD_SPARK_W):
                dx = (x - centre_x) / half_w
                dy = (y - centre_y) / half_h
                d = dx * dx + dy * dy
                if d > 1.0:
                    continue
                if d < core:
                    canvas.set(x, y, 1)        # white core
                elif d < 0.66:
                    canvas.set(x, y, 14)       # cyan
                else:
                    canvas.set(x, y, 10)       # blue rim
        frames.append(canvas)
    return frames


def paint_ring(canvas: Canvas, cx: float, cy: float, outer: float, inner: float,
               crest: int, body: int, tail: int, jag: float = 0.0,
               lobes: int = 8) -> None:
    """Paints one frame of an expanding ring.

    A ring is a ring whether it spreads across a floor or bursts off a wall; only the
    point it is measured from and how ragged its edge is differ, so both shockwave
    families share this. `jag` wobbles the outer edge into `lobes` points, which is
    what makes a wall break look thrown rather than drawn with a compass.
    """
    for y in range(canvas.h):
        for x in range(canvas.w):
            dx = x - cx
            dy = y - cy
            distance = math.hypot(dx, dy)
            edge = outer
            if jag:
                angle = math.atan2(dy, dx)
                edge = outer * (1.0 + jag * 0.5 * (1.0 + math.sin(angle * lobes)))
            if distance > edge or distance < inner:
                continue
            if distance > edge - 2.0:
                canvas.set(x, y, crest)
            elif distance > inner + 2.0:
                canvas.set(x, y, body)
            else:
                canvas.set(x, y, tail)


def shockwave_colours(progress: float):
    """The crest, body and tail colours for one frame of a shockwave.

    The first half of the effect is lit -- a hot crest over a warmer body -- and the
    second half is bare dust, which is the colour the ring cools to as it spreads.
    """
    if progress < 0.5:
        return 12, 13, 6      # yellow crest over orange, red shadow
    return 2, 3, 4            # pale dust over grey


def make_ground_shockwave(width: int, height: int, frame_count: int) -> list:
    """A ground shockwave: a dome of dust widening across the floor.

    The base of the sprite is the floor, so the ring is measured from the bottom edge
    and only its upper half lands on the canvas. That is what makes it read as
    spreading along the ground rather than as a ball floating in the air.
    """
    frames = []
    cx = (width - 1) * 0.5
    base = height - 1
    for index in range(frame_count):
        canvas = Canvas(width, height)
        progress = index / float(frame_count - 1)
        outer = (0.42 + 0.58 * progress) * (width * 0.5)
        inner = outer * (0.12 + 0.68 * progress)
        crest, body, tail = shockwave_colours(progress)
        paint_ring(canvas, cx, base, outer, inner, crest, body, tail)
        frames.append(canvas)
    return frames


def make_wall_shockwave(size: int, frame_count: int, jag: float) -> list:
    """A wall-hit shockwave: a ragged ring bursting off the impact point.

    Round rather than flattened, because it is thrown from a point in the air instead
    of spread along a floor, so it is measured from the sprite's centre.
    """
    frames = []
    centre = (size - 1) * 0.5
    for index in range(frame_count):
        canvas = Canvas(size, size)
        progress = index / float(frame_count - 1)
        outer = (0.32 + 0.54 * progress) * (size * 0.5)
        inner = outer * (0.34 + 0.42 * progress)
        crest, body, tail = shockwave_colours(progress)
        paint_ring(canvas, centre, centre, outer, inner, crest, body, tail, jag)
        frames.append(canvas)
    return frames


def floor_axis(width: int, height: int) -> tuple:
    """The bottom centre of a sprite: where a floor-standing effect is pinned."""
    return (width // 2, height - 1)


def make_super_glow(size: int, frame_count: int) -> list:
    """A super effect: a large radial glow that flares and falls away.

    Drawn additively, so the white centre is the brightest thing on screen and the
    blues only light the stage rather than covering it. The flare peaks a third of the
    way through and eases off, which is what gives the effect a rise and a tail
    instead of an even fade.
    """
    frames = []
    centre = (size - 1) * 0.5
    reach = size * 0.5
    for index in range(frame_count):
        canvas = Canvas(size, size)
        progress = index / float(frame_count - 1)
        flare = max(0.12, 1.0 - abs(2.0 * progress - 0.6))
        for y in range(size):
            for x in range(size):
                level = flare - math.hypot(x - centre, y - centre) / reach
                if level > 0.72:
                    canvas.set(x, y, 1)        # white core
                elif level > 0.50:
                    canvas.set(x, y, 14)       # cyan
                elif level > 0.30:
                    canvas.set(x, y, 10)       # blue
                elif level > 0.14:
                    canvas.set(x, y, 11)       # dark blue rim
        frames.append(canvas)
    return frames


# One fightfx action: its number, its frames, the ticks each frame is held for, its
# drawtype, its axis and what it is.
#
# `blend` is the AIR drawtype: "A" is additive, which every glowing effect wants, and
# "" is the plain normal blend the dust and rocky shockwaves are drawn with. `ticks` is
# per frame and unequal on purpose -- short at the start so the effect snaps open, long
# at the end so it settles, which is how a real fightfx animation gets its timing.
FxAction = namedtuple("FxAction", "number frames ticks blend axis comment")


def build_fightfx_actions() -> list:
    """The whole fightfx set, in one place so the SFF and the AIR cannot disagree."""
    actions = []

    def add(number, frames, ticks, blend, axis, comment):
        assert len(frames) == len(ticks), (number, len(frames), len(ticks))
        actions.append(FxAction(number, frames, ticks, blend, axis, comment))

    for number in sorted(SPARK_ACTIONS):
        size, reach, cool, label = SPARK_ACTIONS[number]
        add(number, make_spark(size, reach, cool), [2] * SPARK_FRAMES,
            "A", (size // 2, size // 2), label)

    add(30, make_super_glow(128, 6), [2, 2, 3, 2, 3, 3], "A", (64, 64),
        "super pause")
    add(40, make_guard_spark(), [1, 1, 2, 2, 3], "A",
        (GUARD_SPARK_W // 2, GUARD_SPARK_H // 2), "guard spark, a flat cool flash")

    # Ground shockwaves, small to strong: a low dome of dust pinned to the floor by an
    # axis on the sprite's bottom edge.
    add(60, make_ground_shockwave(96, 28, 5), [2, 1, 1, 2, 2], "",
        floor_axis(96, 28), "small ground shockwave")
    add(61, make_ground_shockwave(128, 36, 7), [2, 1, 1, 2, 2, 3, 3], "",
        floor_axis(128, 36), "medium ground shockwave")
    add(62, make_ground_shockwave(176, 48, 11),
        [2, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3], "",
        floor_axis(176, 48), "strong ground shockwave")

    # Wall-hit shockwaves, light to strong: a ragged ring off the point of impact,
    # measured from the centre because the impact is in the air.
    add(70, make_wall_shockwave(72, 3, 0.10), [2, 2, 3], "", (36, 36),
        "light wall-hit shockwave")
    add(71, make_wall_shockwave(96, 5, 0.14), [2, 1, 2, 2, 3], "", (48, 48),
        "medium wall-hit shockwave")
    add(72, make_wall_shockwave(136, 8, 0.20), [2, 1, 2, 2, 2, 3, 3, 3], "",
        (68, 68), "strong wall-hit shockwave")

    # Super effects, level 1 to 3, plus the pause that precedes one.
    add(100, make_super_glow(96, 8), [3, 2, 3, 2, 3, 2, 3, 3], "A", (48, 48),
        "level 1 super effect")
    add(101, make_super_glow(144, 10), [3, 2, 3, 2, 3, 2, 3, 2, 3, 3], "A",
        (72, 72), "level 2 super effect")
    add(102, make_super_glow(192, 12), [3, 2, 3, 2, 3, 2, 3, 2, 3, 3, 3, 3],
        "A", (96, 96), "level 3 super effect")

    actions.sort(key=lambda action: action.number)
    return actions


def build_fightfx_sff(actions: list) -> list:
    sprites = []
    for action in actions:
        for image, canvas in enumerate(action.frames):
            sprites.append(Sprite(action.number, image, canvas,
                                  action.axis[0], action.axis[1]))
    return sprites


def fightfx_air_text(actions: list) -> str:
    lines = [
        "; Hit sparks and impact effects, in the screenpack rather than in a character: a",
        "; HitDef's plain `sparkno = 2` refers here, and the `S` prefix is what means a",
        "; character's own animation. The numbers are Elecbyte's, the art is not.",
    ]
    for action in actions:
        lines.append("")
        if action.comment:
            lines.append("; %s" % action.comment)
        lines.append("[Begin Action %d]" % action.number)
        # A trailing `A` is the additive blend, which is what makes a spark or a glow
        # light the stage instead of sitting on top of it; a shockwave carries no
        # drawtype, so the dust it throws reads as solid ground rather than light.
        for image, ticks in enumerate(action.ticks):
            if action.blend:
                lines.append("%d,%d, 0,0, %d, ,%s"
                             % (action.number, image, ticks, action.blend))
            else:
                lines.append("%d,%d, 0,0, %d" % (action.number, image, ticks))
    lines.append("")
    return "\n".join(lines)

def write_sff_v1(path: str, sprites: list[Sprite]) -> None:
    """Writes SFF revision 1.

    Layout: a 512-byte header, then for each sprite a 32-byte subheader
    immediately followed by that sprite's PCX block. Each subheader stores the
    file offset of the next one, forming a singly-linked list; the last stores
    zero.
    """
    blobs = [pcx_block(s.canvas) for s in sprites]

    header = bytearray(512)
    header[0:12] = b"ElecbyteSpr\x00"
    # The version word at 0x0C is four bytes and only the last one matters: 1
    # selects revision 1, 2 selects revision 2.
    header[0x0C:0x10] = bytes([0, 0, 0, 1])
    groups = len({s.group for s in sprites})
    struct.pack_into("<I", header, 0x10, groups)
    struct.pack_into("<I", header, 0x14, len(sprites))
    struct.pack_into("<I", header, 0x18, 512)   # first subheader
    struct.pack_into("<I", header, 0x1C, 32)    # subheader size
    header[0x20] = 0                            # per-sprite palettes
    # Slice assignment resizes a bytearray when the lengths differ, which would
    # silently shrink the header and shift every offset after it. Write the
    # comment as a length-matched slice to keep the header exactly 512 bytes.
    comment = b"Anifight screenpack\x00"
    header[0x24:0x24 + len(comment)] = comment

    body = bytearray()
    offset = 512
    for index, (sprite, blob) in enumerate(zip(sprites, blobs)):
        sub = bytearray(32)
        next_offset = 0
        if index + 1 < len(sprites):
            next_offset = offset + 32 + len(blob)
        struct.pack_into("<I", sub, 0x00, next_offset)
        struct.pack_into("<I", sub, 0x04, len(blob))
        struct.pack_into("<h", sub, 0x08, sprite.axis_x)
        struct.pack_into("<h", sub, 0x0A, sprite.axis_y)
        struct.pack_into("<H", sub, 0x0C, sprite.group)
        struct.pack_into("<H", sub, 0x0E, sprite.image)
        struct.pack_into("<H", sub, 0x10, 0)     # linked index, unused
        sub[0x12] = 0                            # no shared palette
        body += sub
        body += blob
        offset += 32 + len(blob)

    with open(path, "wb") as f:
        f.write(bytes(header) + bytes(body))


# ---------------------------------------------------------------------------
# Bitmap font, 5x7, and the FNT v1 container
# ---------------------------------------------------------------------------
#
# The classic 5x7 glyph set, column-major: five bytes per character, bit 0 of
# each byte being the top row and bit 6 the bottom. Only the printable ASCII
# range is needed -- MUGEN's bitmap fonts are ASCII-only by construction.

FONT_5X7 = {
    0x20: (0x00, 0x00, 0x00, 0x00, 0x00),  # space
    0x21: (0x00, 0x00, 0x5F, 0x00, 0x00),  # !
    0x22: (0x00, 0x07, 0x00, 0x07, 0x00),  # "
    0x23: (0x14, 0x7F, 0x14, 0x7F, 0x14),  # #
    0x24: (0x24, 0x2A, 0x7F, 0x2A, 0x12),  # $
    0x25: (0x23, 0x13, 0x08, 0x64, 0x62),  # %
    0x26: (0x36, 0x49, 0x55, 0x22, 0x50),  # &
    0x27: (0x00, 0x05, 0x03, 0x00, 0x00),  # '
    0x28: (0x00, 0x1C, 0x22, 0x41, 0x00),  # (
    0x29: (0x00, 0x41, 0x22, 0x1C, 0x00),  # )
    0x2A: (0x14, 0x08, 0x3E, 0x08, 0x14),  # *
    0x2B: (0x08, 0x08, 0x3E, 0x08, 0x08),  # +
    0x2C: (0x00, 0x50, 0x30, 0x00, 0x00),  # ,
    0x2D: (0x08, 0x08, 0x08, 0x08, 0x08),  # -
    0x2E: (0x00, 0x60, 0x60, 0x00, 0x00),  # .
    0x2F: (0x20, 0x10, 0x08, 0x04, 0x02),  # /
    0x30: (0x3E, 0x51, 0x49, 0x45, 0x3E),  # 0
    0x31: (0x00, 0x42, 0x7F, 0x40, 0x00),  # 1
    0x32: (0x42, 0x61, 0x51, 0x49, 0x46),  # 2
    0x33: (0x21, 0x41, 0x45, 0x4B, 0x31),  # 3
    0x34: (0x18, 0x14, 0x12, 0x7F, 0x10),  # 4
    0x35: (0x27, 0x45, 0x45, 0x45, 0x39),  # 5
    0x36: (0x3C, 0x4A, 0x49, 0x49, 0x30),  # 6
    0x37: (0x01, 0x71, 0x09, 0x05, 0x03),  # 7
    0x38: (0x36, 0x49, 0x49, 0x49, 0x36),  # 8
    0x39: (0x06, 0x49, 0x49, 0x29, 0x1E),  # 9
    0x3A: (0x00, 0x36, 0x36, 0x00, 0x00),  # :
    0x3B: (0x00, 0x56, 0x36, 0x00, 0x00),  # ;
    0x3C: (0x08, 0x14, 0x22, 0x41, 0x00),  # <
    0x3D: (0x14, 0x14, 0x14, 0x14, 0x14),  # =
    0x3E: (0x00, 0x41, 0x22, 0x14, 0x08),  # >
    0x3F: (0x02, 0x01, 0x51, 0x09, 0x06),  # ?
    0x40: (0x32, 0x49, 0x79, 0x41, 0x3E),  # @
    0x41: (0x7E, 0x11, 0x11, 0x11, 0x7E),  # A
    0x42: (0x7F, 0x49, 0x49, 0x49, 0x36),  # B
    0x43: (0x3E, 0x41, 0x41, 0x41, 0x22),  # C
    0x44: (0x7F, 0x41, 0x41, 0x22, 0x1C),  # D
    0x45: (0x7F, 0x49, 0x49, 0x49, 0x41),  # E
    0x46: (0x7F, 0x09, 0x09, 0x09, 0x01),  # F
    0x47: (0x3E, 0x41, 0x49, 0x49, 0x7A),  # G
    0x48: (0x7F, 0x08, 0x08, 0x08, 0x7F),  # H
    0x49: (0x00, 0x41, 0x7F, 0x41, 0x00),  # I
    0x4A: (0x20, 0x40, 0x41, 0x3F, 0x01),  # J
    0x4B: (0x7F, 0x08, 0x14, 0x22, 0x41),  # K
    0x4C: (0x7F, 0x40, 0x40, 0x40, 0x40),  # L
    0x4D: (0x7F, 0x02, 0x0C, 0x02, 0x7F),  # M
    0x4E: (0x7F, 0x04, 0x08, 0x10, 0x7F),  # N
    0x4F: (0x3E, 0x41, 0x41, 0x41, 0x3E),  # O
    0x50: (0x7F, 0x09, 0x09, 0x09, 0x06),  # P
    0x51: (0x3E, 0x41, 0x51, 0x21, 0x5E),  # Q
    0x52: (0x7F, 0x09, 0x19, 0x29, 0x46),  # R
    0x53: (0x46, 0x49, 0x49, 0x49, 0x31),  # S
    0x54: (0x01, 0x01, 0x7F, 0x01, 0x01),  # T
    0x55: (0x3F, 0x40, 0x40, 0x40, 0x3F),  # U
    0x56: (0x1F, 0x20, 0x40, 0x20, 0x1F),  # V
    0x57: (0x3F, 0x40, 0x38, 0x40, 0x3F),  # W
    0x58: (0x63, 0x14, 0x08, 0x14, 0x63),  # X
    0x59: (0x07, 0x08, 0x70, 0x08, 0x07),  # Y
    0x5A: (0x61, 0x51, 0x49, 0x45, 0x43),  # Z
    0x5B: (0x00, 0x7F, 0x41, 0x41, 0x00),  # [
    0x5C: (0x02, 0x04, 0x08, 0x10, 0x20),  # backslash
    0x5D: (0x00, 0x41, 0x41, 0x7F, 0x00),  # ]
    0x5E: (0x04, 0x02, 0x01, 0x02, 0x04),  # ^
    0x5F: (0x40, 0x40, 0x40, 0x40, 0x40),  # _
    0x60: (0x00, 0x01, 0x02, 0x04, 0x00),  # `
    0x61: (0x20, 0x54, 0x54, 0x54, 0x78),  # a
    0x62: (0x7F, 0x48, 0x44, 0x44, 0x38),  # b
    0x63: (0x38, 0x44, 0x44, 0x44, 0x20),  # c
    0x64: (0x38, 0x44, 0x44, 0x48, 0x7F),  # d
    0x65: (0x38, 0x54, 0x54, 0x54, 0x18),  # e
    0x66: (0x08, 0x7E, 0x09, 0x01, 0x02),  # f
    0x67: (0x0C, 0x52, 0x52, 0x52, 0x3E),  # g
    0x68: (0x7F, 0x08, 0x04, 0x04, 0x78),  # h
    0x69: (0x00, 0x44, 0x7D, 0x40, 0x00),  # i
    0x6A: (0x20, 0x40, 0x44, 0x3D, 0x00),  # j
    0x6B: (0x7F, 0x10, 0x28, 0x44, 0x00),  # k
    0x6C: (0x00, 0x41, 0x7F, 0x40, 0x00),  # l
    0x6D: (0x7C, 0x04, 0x18, 0x04, 0x78),  # m
    0x6E: (0x7C, 0x08, 0x04, 0x04, 0x78),  # n
    0x6F: (0x38, 0x44, 0x44, 0x44, 0x38),  # o
    0x70: (0x7C, 0x14, 0x14, 0x14, 0x08),  # p
    0x71: (0x08, 0x14, 0x14, 0x18, 0x7C),  # q
    0x72: (0x7C, 0x08, 0x04, 0x04, 0x08),  # r
    0x73: (0x48, 0x54, 0x54, 0x54, 0x20),  # s
    0x74: (0x04, 0x3F, 0x44, 0x40, 0x20),  # t
    0x75: (0x3C, 0x40, 0x40, 0x20, 0x7C),  # u
    0x76: (0x1C, 0x20, 0x40, 0x20, 0x1C),  # v
    0x77: (0x3C, 0x40, 0x30, 0x40, 0x3C),  # w
    0x78: (0x44, 0x28, 0x10, 0x28, 0x44),  # x
    0x79: (0x0C, 0x50, 0x50, 0x50, 0x3C),  # y
    0x7A: (0x44, 0x64, 0x54, 0x4C, 0x44),  # z
    0x7B: (0x00, 0x08, 0x36, 0x41, 0x00),  # {
    0x7C: (0x00, 0x00, 0x7F, 0x00, 0x00),  # |
    0x7D: (0x00, 0x41, 0x36, 0x08, 0x00),  # }
    0x7E: (0x08, 0x08, 0x2A, 0x1C, 0x08),  # ~
}

GLYPH_CODES = sorted(FONT_5X7)


def build_glyph_strip(cell_w: int, cell_h: int, scale: int):
    """Lays every glyph out left to right in one strip.

    Returns (canvas, map_lines) where map_lines are `[map]` records of the form
    `<char> <offset> <width>`. The offset is explicit rather than implied so
    that the reader's running cursor and this writer's arithmetic are checked
    against each other rather than assumed to agree.
    """
    glyph_w = 5 * scale
    glyph_h = 7 * scale
    strip = Canvas(cell_w * len(GLYPH_CODES), cell_h)

    map_lines = []
    for index, code in enumerate(GLYPH_CODES):
        cell_x = index * cell_w
        # Centre the glyph in its cell.
        origin_x = cell_x + (cell_w - glyph_w) // 2
        origin_y = (cell_h - glyph_h) // 2

        columns = FONT_5X7[code]
        for col in range(5):
            bits = columns[col]
            for row in range(7):
                if bits & (1 << row):
                    for sy in range(scale):
                        for sx in range(scale):
                            strip.set(origin_x + col * scale + sx,
                                      origin_y + row * scale + sy, 1)

        # Every glyph is written as a `0xNN` escape rather than a literal.
        #
        # A map record is `char offset width`, and the character position is a
        # byte, not a token -- so a literal cannot express all of them. Space
        # would split the record; `;` would read as a comment and take the glyph
        # with it; `[` would read as a section header and end the map there. The
        # escapes cost nothing to read and remove the whole class of problem,
        # which matters because the map is a table for a program, not prose.
        map_lines.append("0x%02X %d %d" % (code, cell_x, cell_w))

    return strip, map_lines


def write_fnt_v1(path: str, cell_w: int, cell_h: int, scale: int) -> None:
    """Writes a revision-1 bitmap font.

    Layout: a 64-byte header, then an 8-bit PCX holding the glyph strip, then a
    plain-text blob of `[map]` and `[def]` sections.
    """
    strip, map_lines = build_glyph_strip(cell_w, cell_h, scale)
    pcx = pcx_block(strip)

    text = "[map]\n"
    text += "\n".join(map_lines)
    text += "\n\n[def]\n"
    text += "size = %d,%d\n" % (cell_w, cell_h)
    text += "spacing = 0,0\n"
    text += "offset = 0,0\n"
    # One colour per bank: the generated font has a single bank and the screens
    # tint it at draw time instead of switching banks for colour.
    text += "colors = 1\n"
    blob = text.encode("ascii")

    header = bytearray(64)
    header[0:12] = b"ElecbyteFnt\x00"
    struct.pack_into("<H", header, 0x0C, 1)          # version
    struct.pack_into("<H", header, 0x0E, 0)          # sub-version
    struct.pack_into("<I", header, 0x10, 64)         # PCX offset
    struct.pack_into("<I", header, 0x14, len(pcx))   # PCX length
    struct.pack_into("<I", header, 0x18, 64 + len(pcx))
    struct.pack_into("<I", header, 0x1C, len(blob))
    header[0x20:0x40] = b"Anifight generated font".ljust(32, b"\x00")

    with open(path, "wb") as f:
        f.write(bytes(header) + pcx + blob)


# ---------------------------------------------------------------------------
# Art
# ---------------------------------------------------------------------------


def make_title_bg() -> Canvas:
    """A vertical ramp with a subtle diagonal band, 320x240."""
    c = Canvas(320, 240)
    for y in range(240):
        shade = RAMP_START + int((y / 239) * (RAMP_STEPS - 1))
        for x in range(320):
            c.set(x, y, shade)
    # A few diagonal streaks so the background is not a flat wash.
    for band in range(6):
        start = band * 70
        for i in range(200):
            x = start + i
            y = 240 - i
            for t in range(3):
                c.set(x, y - t, min(RAMP_START + RAMP_STEPS - 1, RAMP_START + 30))
    return c


def make_ground_tile() -> Canvas:
    """A 32x24 tile of dim floor, repeated across the bottom of the title."""
    c = Canvas(32, 24)
    for y in range(24):
        shade = RAMP_START + 8 + int((y / 23) * 10)
        for x in range(32):
            c.set(x, y, shade)
    c.hline(0, 0, 32, RAMP_START + 34)
    for x in range(0, 32, 8):
        c.vline(x, 0, 24, RAMP_START + 24)
    return c


def make_logo() -> Canvas:
    """The game's wordmark, drawn as a chunky blocky title."""
    c = Canvas(160, 46)
    # A plate behind the text.
    for y in range(46):
        shade = METAL_START + int(abs(y - 23) / 23 * (METAL_STEPS - 1) * 0.6)
        c.hline(2, y, 156, min(shade, METAL_START + METAL_STEPS - 1))
    c.frame(0, 0, 160, 46, 1)
    c.frame(1, 1, 158, 44, 12)

    # "ANIFIGHT" in a big blocky face built from rectangles, scaled 5x from a
    # 5x7 source so it matches the fonts.
    word = "ANIFIGHT"
    scale = 4
    glyph_w, glyph_h = 5 * scale, 7 * scale
    gap = 2
    total = len(word) * (glyph_w + gap) - gap
    ox = (160 - total) // 2
    oy = (46 - glyph_h) // 2
    for i, ch in enumerate(word):
        columns = FONT_5X7[ord(ch)]
        bx = ox + i * (glyph_w + gap)
        for col in range(5):
            bits = columns[col]
            for row in range(7):
                if bits & (1 << row):
                    c.rect(bx + col * scale, oy + row * scale, scale, scale, 1)
    return c


def make_footer_bar() -> Canvas:
    c = Canvas(320, 40)
    for y in range(40):
        c.hline(0, y, 320, RAMP_START + 6)
    c.hline(0, 0, 320, RAMP_START + 30)
    c.hline(0, 1, 320, 3)
    return c


def make_select_bg() -> Canvas:
    """A tiled starfield for the select screen."""
    c = Canvas(64, 64)
    for y in range(64):
        shade = RAMP_START + int((y / 63) * 12)
        for x in range(64):
            c.set(x, y, shade)
    # Deterministic pseudo-random stars: a fixed generator keeps the generated
    # file byte-identical between runs, which matters for reproducible builds.
    seed = 12345
    for _ in range(40):
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
        x = seed % 64
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
        y = seed % 64
        c.set(x, y, 1 if (seed & 3) == 0 else 2)
    return c


def make_select_panel() -> Canvas:
    c = Canvas(240, 180)
    for y in range(180):
        c.hline(0, y, 240, RAMP_START + 2)
    c.frame(0, 0, 240, 180, RAMP_START + 18)
    c.frame(1, 1, 238, 178, RAMP_START + 10)
    return c


# The roster grid's cells. They were 27x27 for a ten-column grid; with eleven
# columns they are 24x24 at a 26-pixel pitch, which is what fits 41 entries -- the
# roster the MUGEN install brought -- inside the 320-pixel screen without a fifth row.
CELL_SIZE = 24


def make_cell_bg() -> Canvas:
    """The square behind every roster cell."""
    n = CELL_SIZE
    c = Canvas(n, n)
    for y in range(n):
        c.hline(0, y, n, RAMP_START + 4)
    c.frame(0, 0, n, n, 3)
    c.frame(1, 1, n - 2, n - 2, RAMP_START + 14)
    return c


def make_random_cell() -> Canvas:
    n = CELL_SIZE
    c = Canvas(n, n)
    for y in range(n):
        c.hline(0, y, n, RAMP_START + 8)
    c.frame(0, 0, n, n, 12)
    c.frame(1, 1, n - 2, n - 2, 3)
    # A question mark, so the cell reads as "random" at a glance.
    columns = FONT_5X7[ord("?")]
    for col in range(5):
        bits = columns[col]
        for row in range(7):
            if bits & (1 << row):
                c.rect(9 + col * 2, 7 + row * 2, 2, 2, 1)
    return c


def make_cursor(color: int, filled: bool) -> Canvas:
    c = Canvas(CELL_SIZE + 4, CELL_SIZE + 4)
    if filled:
        c.rect(0, 0, CELL_SIZE + 4, 2, color)
        c.rect(0, 29, 31, 2, color)
        c.rect(0, 0, 2, 31, color)
        c.rect(29, 0, 2, 31, color)
    else:
        c.rect(0, 0, 31, 2, color)
        c.rect(0, 29, 31, 2, color)
        c.rect(0, 0, 2, 31, color)
        c.rect(29, 0, 2, 31, color)
        c.rect(4, 4, 23, 23, RAMP_START + 6)
        c.frame(4, 4, 23, 23, color)
    return c


# The four layers of a bar are not interchangeable; MUGEN gives each one a
# different job, and the engine reproduces it:
#
#   bg0, bg1   the backing, drawn whole. This is the empty channel a drained bar
#              leaves behind, so it is where the frame and the dark track go.
#   mid        the band of life between the character's current life and its
#              "mid life" -- the pale trailing edge a bar shows just after a hit.
#   front      the bar itself, and the layer that drains. Bright metal goes here.
#
# Putting the bright metal in bg1 instead produces a bar that looks full forever,
# which is the mistake this comment exists to prevent.


def make_bar(width: int, height: int, top: int, bottom: int) -> Canvas:
    """A horizontal metal bar: bright along its top edge, dark along the base."""
    c = Canvas(width, height)
    for y in range(height):
        t = y / max(1, height - 1)
        shade = top + int((bottom - top) * t)
        c.hline(0, y, width, max(0, min(255, shade)))
    return c


def make_bar_plate(width: int, height: int) -> Canvas:
    """bg0: the empty channel, and the bar's frame."""
    c = Canvas(width, height)
    c.rect(0, 0, width, height, 5)
    c.frame(0, 0, width, height, METAL_START + 10)
    return c


def make_bar_inset(width: int, height: int) -> Canvas:
    """bg1: a second backing layer, drawn whole under the bar.

    Only ever seen on the drained part of a bar, so it is a shadow rather than
    anything bright.
    """
    c = Canvas(width, height)
    c.hline(1, 1, width - 2, 4)
    return c


def make_bar_lost(width: int, height: int, top: int, bottom: int) -> Canvas:
    """mid: the pale band of life only just lost.

    Drawn whole by this engine, but MUGEN clips it to the gap between the
    character's life and its mid life. With no characters to ask there is no such
    gap, so it never appears -- which is why the engine leaves it out entirely
    rather than approximating it.
    """
    return make_bar(width, height, top, bottom)


def make_bar_fill(width: int, height: int, top: int, bottom: int) -> Canvas:
    """front: the bar. The engine crops this one, so its width carries the drain.

    The one-pixel transparent margin keeps the backing's frame visible around it
    even at full life.
    """
    c = make_bar(width, height, top, bottom)
    c.hline(0, 0, width, 0)
    c.hline(0, height - 1, width, 0)
    c.vline(0, 0, height, 0)
    c.vline(width - 1, 0, height, 0)
    return c


def make_lifebar_plate() -> Canvas:
    return make_bar_plate(LIFEBAR_W, LIFEBAR_H)


def make_lifebar_inset() -> Canvas:
    return make_bar_inset(LIFEBAR_W, LIFEBAR_H)


def make_lifebar_lost() -> Canvas:
    return make_bar_lost(LIFEBAR_W, LIFEBAR_H, 1, 2)


def make_lifebar_fill() -> Canvas:
    return make_bar_fill(LIFEBAR_W, LIFEBAR_H, METAL_START + 28, METAL_START + 6)


def make_powerbar_plate() -> Canvas:
    return make_bar_plate(POWERBAR_W, POWERBAR_H)


def make_powerbar_inset() -> Canvas:
    return make_bar_inset(POWERBAR_W, POWERBAR_H)


def make_powerbar_lost() -> Canvas:
    return make_bar_lost(POWERBAR_W, POWERBAR_H, RAMP_START + 40, RAMP_START + 34)


def make_powerbar_fill() -> Canvas:
    return make_bar_fill(POWERBAR_W, POWERBAR_H, RAMP_START + 46, RAMP_START + 20)


def make_face_bg() -> Canvas:
    c = Canvas(26, 26)
    c.rect(0, 0, 26, 26, 5)
    c.frame(0, 0, 26, 26, METAL_START + 14)
    c.frame(1, 1, 24, 24, METAL_START + 4)
    return c


def make_win_icon(kind: str) -> Canvas:
    """A small round token per way of winning a round."""
    c = Canvas(12, 12)
    color = {
        "n": 2, "s": 10, "h": 6, "throw": 13, "c": 14,
        "t": 15, "suicide": 4, "teammate": 9, "perfect": 12,
    }.get(kind, 3)
    c.rect(0, 0, 12, 12, color)
    c.frame(0, 0, 12, 12, 1)
    c.rect(3, 3, 6, 6, 5)
    return c


def make_vs_plate() -> Canvas:
    c = Canvas(200, 90)
    for y in range(90):
        c.hline(0, y, 200, RAMP_START + 3)
    c.frame(0, 0, 200, 90, 12)
    return c


# ---------------------------------------------------------------------------
# Assembly
# ---------------------------------------------------------------------------


def build_system_sff() -> list[Sprite]:
    sprites = []

    def add(group, image, canvas, ax=0, ay=0):
        sprites.append(Sprite(group, image, canvas, ax, ay))

    add(1, 0, make_title_bg())
    add(1, 1, make_ground_tile())
    add(10, 0, make_logo())
    add(11, 0, make_footer_bar())

    add(100, 0, make_select_bg())
    add(100, 1, make_select_panel())

    add(150, 0, make_cell_bg())
    add(151, 0, make_random_cell())

    # Cursors are drawn around a cell, so their axis is the cell's centre.
    const_axis = CELL_SIZE // 2 + 2
    add(160, 0, make_cursor(6, True), const_axis, const_axis)
    add(161, 0, make_cursor(8, True), const_axis, const_axis)
    add(170, 0, make_cursor(10, True), const_axis, const_axis)
    add(171, 0, make_cursor(9, True), const_axis, const_axis)

    add(200, 0, make_vs_plate())
    return sprites


def build_fight_sff() -> list[Sprite]:
    sprites = []

    def add(group, image, canvas, ax=0, ay=0):
        sprites.append(Sprite(group, image, canvas, ax, ay))

    # [Lifebar] p1.pos anchors the bar at its inner end, because the engine keeps
    # the remaining life against `pos` and drains toward the far end of range.x.
    # So the axes sit at the right edge, where p1's bar meets the timer.
    add(10, 0, make_lifebar_plate(), LIFEBAR_W, 0)
    add(11, 0, make_lifebar_inset(), LIFEBAR_W, 0)
    add(12, 0, make_lifebar_lost(), LIFEBAR_W, 0)
    add(13, 0, make_lifebar_fill(), LIFEBAR_W, 0)

    add(40, 0, make_powerbar_plate(), POWERBAR_W, 0)
    add(41, 0, make_powerbar_inset(), POWERBAR_W, 0)
    add(42, 0, make_powerbar_lost(), POWERBAR_W, 0)
    add(43, 0, make_powerbar_fill(), POWERBAR_W, 0)

    # The portrait backing draws around the cell, so its axis is its centre.
    add(50, 0, make_face_bg(), 13, 13)

    for image, kind in enumerate(
        ["n", "s", "h", "throw", "c", "t", "suicide", "teammate"]
    ):
        add(100 + image, 0, make_win_icon(kind))
    add(110, 0, make_win_icon("perfect"))

    return sprites


# ---------------------------------------------------------------------------
# Sound
# ---------------------------------------------------------------------------
#
# fight.def names two banks --
#
#     snd = fight.snd
#     common.snd = common.snd
#
# -- and system.def names a third, system.snd. The real Elecbyte files are
# copyrighted and are not redistributed here, so this generator writes no
# sound of its own: every `.snd` in the built screenpack is the player's own
# copy from local_sounds/, copied byte for byte. When a file is not supplied,
# no bank is written and the engine is simply silent for those pairs -- a
# synthesised stand-in is not a substitute for the authors' recordings.

SOUND_FILES = ("system.snd", "common.snd", "fight.snd")


def install_supplied_sounds(directory: str, out_dir: str) -> None:
    """Copies the player's real MUGEN banks into the build, unmodified."""
    for name in SOUND_FILES:
        source = os.path.join(directory, name)
        target = os.path.join(out_dir, name)
        if not os.path.isfile(source):
            # Remove a bank left by an earlier build, so deleting local_sounds/
            # cannot silently keep shipping the old copy.
            if os.path.exists(target):
                os.remove(target)
            print("  %-16s not supplied (no generated stand-in)" % name)
            continue
        shutil.copyfile(source, target)
        print("  %-16s %8d bytes (supplied)" % (name, os.path.getsize(target)))



# ---------------------------------------------------------------------------
# Characters and stages live in their own generators
# ---------------------------------------------------------------------------
#
# The roster used to be drawn here, back when the screenpack was the only
# generator. It moved out when it grew to ten fighters. A character is a
# folder under chars/ and a stage is a folder under stages/, both siblings of
# this screenpack's data/ rather than part of it, so each keeps its own
# generator and the three remain independent readings of the same formats.

def main() -> int:
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "data"
    )
    os.makedirs(out_dir, exist_ok=True)

    write_sff_v1(os.path.join(out_dir, "system.sff"), build_system_sff())
    write_sff_v1(os.path.join(out_dir, "fight.sff"), build_fight_sff())
    # The hit sparks and impact effects, in their own bank because that is where the
    # format puts them: a HitDef's plain `sparkno = 2` refers to fightfx, and the `S`
    # prefix is what means "this character's own animation". A screenpack with no
    # fightfx is one where every hit lands without a spark. The action set is built
    # once and handed to both writers so the SFF and the AIR cannot disagree.
    actions = build_fightfx_actions()
    write_sff_v1(os.path.join(out_dir, "fightfx.sff"), build_fightfx_sff(actions))
    with open(os.path.join(out_dir, "fightfx.air"), "w", encoding="utf-8") as handle:
        handle.write(fightfx_air_text(actions))

    # 6x9 at 1x, and 12x18 at 2x, matching the names system.def references.
    write_fnt_v1(os.path.join(out_dir, "f-6x9.fnt"), 6, 9, 1)
    write_fnt_v1(os.path.join(out_dir, "f-12x18.fnt"), 12, 18, 2)

    # The sound banks are the player's own copy of MUGEN's files. Nothing is
    # synthesised: a checkout without local_sounds/ ships none, and the engine is
    # silent where a pair has no real recording.
    supplied = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
        os.path.dirname(os.path.abspath(out_dir)), "..", "..", "local_sounds"
    )
    install_supplied_sounds(supplied, out_dir)

    for name in sorted(os.listdir(out_dir)):
        path = os.path.join(out_dir, name)
        if os.path.isfile(path):
            print("  %-16s %8d bytes" % (name, os.path.getsize(path)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

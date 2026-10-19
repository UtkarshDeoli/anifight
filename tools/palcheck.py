#!/usr/bin/env python3
"""Compare an SFF v1 sprite's own palette against a character's .act file.

    python3 tools/palcheck.py [character-folder ...]

The engine reads a character's `pal1..pal12` .act files as palettes, and the
index order they use is not the order a sprite bank's own palettes use. This
prints the evidence: for each character, how many of the 256 entries of the
bank's first palette match the .act directly, and how many match it reversed.

The answer for every character in the roster that has both is the same --
256/256 reversed, 0/256 direct -- which is why character.cpp flips an .act on
load rather than indexing it as it lies. It holds against an SFF v1 sprite's
own palette and against an SFF v2 palette node alike, so the reversal belongs
to .act files rather than to either sprite layout. Run this again if a
character ever renders in the wrong colours; a character with its own
convention will show up here as a low reversed count rather than a mysterious
palette bug.

For v1 the sprite examined is sprite 0, whose PCX always starts at 512 + 32
(the 32-byte subfile header). Reading v1's header chain instead would be more
general but the chain in these files is unreliable past the first entry, and
sprite 0 is enough to settle the question. For v2 the palette node table in the
header is read directly.
"""

import glob
import os
import sys
import zlib
import struct

HEADER = 512
SUBFILE_HEADER = 32


def sprite0(path):
    """(w, h, pixel indices, 768-byte palette or None) for an SFF v1 file."""
    with open(path, "rb") as f:
        b = f.read()
    # The version is the high byte of the 4-byte word at 0x0C.
    if b[0x0F] != 1:
        return None
    h = HEADER + SUBFILE_HEADER
    if b[h] != 0x0A:  # PCX manufacturer byte
        return None

    xmax = b[h + 8] | (b[h + 9] << 8)
    ymax = b[h + 10] | (b[h + 11] << 8)
    w, hh = xmax + 1, ymax + 1
    nplanes, bpl = b[h + 65], b[h + 66] | (b[h + 67] << 8)
    total = nplanes * bpl * hh

    px, j, n = bytearray(), h + 128, len(b)
    while len(px) < total and j < n:
        c = b[j]
        j += 1
        if c >= 0xC0:  # run-length encoded
            run = c & 0x3F
            if j >= n:
                break
            px.extend(bytes([b[j]]) * run)
            j += 1
        else:
            px.append(c)

    # PCX stores its 256-colour palette after the pixels, behind a 0x0C marker.
    pal = b[j + 1 : j + 1 + 768] if j < n and b[j] == 0x0C else None
    return w, hh, px, pal


def write_png(path, w, h, rgba):
    """Render indices through `rgba` so a decode can be looked at directly."""

    def chunk(t, d):
        body = t + d
        return struct.pack(">I", len(d)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + bytes(rgba[y * w * 4 : (y + 1) * w * 4]) for y in range(h))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 6))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def render(sprite, palette, path, flip=False):
    """Write sprite0 as a PNG; index 0 is the transparent key, shown magenta."""
    w, hh, px, _ = sprite
    rgba = bytearray(w * hh * 4)
    for y in range(hh):
        for x in range(w):
            idx = px[y * w + x]
            src = (255 - idx if flip else idx) * 3
            o = (y * w + x) * 4
            if idx == 0:
                rgba[o : o + 4] = bytes([255, 0, 255, 255])
            else:
                rgba[o : o + 4] = bytes([palette[src], palette[src + 1], palette[src + 2], 255])
    write_png(path, w, hh, rgba)


def v2_palette0(path):
    """(colours, 768-byte palette or None) for an SFF v2 file's first node."""
    with open(path, "rb") as f:
        b = f.read()
    if b[0x0F] != 2:
        return None

    def u32(o):
        return struct.unpack_from("<i", b, o)[0]

    def u16(o):
        return struct.unpack_from("<H", b, o)[0]

    nodes, count, ldata = u32(0x2C), u32(0x30), u32(0x34)
    if count <= 0 or nodes <= 0:
        return None
    # A palette node is group, number, colours, link, then a data offset and
    # length: 16 bytes.
    colors = u16(nodes + 0x04)
    if colors == 0:
        return None
    off, length = u32(nodes + 0x08), u32(nodes + 0x0C)
    if off == 0:
        off = ldata
    stride = 4 if length >= colors * 4 else 3  # v2 stores RGBA, or RGB when short
    raw = b[off : off + colors * stride]

    rgb = bytearray()
    for c in range(colors):
        rgb += raw[c * stride : c * stride + 3]
    return colors, bytes(rgb)


def main(folders):
    print(f"{'character':30s} {'bank palette 0':>15s}  {'direct':>9s}  {'reversed':>9s}")
    for d in folders:
        name = os.path.basename(d.rstrip("/"))
        sffs = sorted(glob.glob(os.path.join(d, "*.sff")))
        acts = sorted(glob.glob(os.path.join(d, "*.act")))
        if not sffs:
            print(f"{name:30s} {'-':>15s}  no .sff")
            continue

        with open(sffs[0], "rb") as f:
            version = f.read(0x10)[0x0F]

        if version == 1:
            s = sprite0(sffs[0])
            if s is None:
                print(f"{name:30s} {'-':>15s}  sprite 0 unreadable")
                continue
            w, hh, _, pal = s
            label = f"v1 sprite {w}x{hh}"
        elif version == 2:
            v = v2_palette0(sffs[0])
            if v is None:
                print(f"{name:30s} {'-':>15s}  no v2 palette nodes")
                continue
            _, pal = v
            label = "v2 node 0"
        else:
            print(f"{name:30s} {'-':>15s}  unsupported SFF version {version}")
            continue

        if pal is None:
            print(f"{name:30s} {label:>15s}  carries no palette")
            continue
        if not acts:
            print(f"{name:30s} {label:>15s}  no .act")
            continue

        with open(acts[0], "rb") as f:
            act = f.read()[-768:]
        direct = sum(1 for k in range(256) if pal[k * 3 : k * 3 + 3] == act[k * 3 : k * 3 + 3])
        rev = sum(1 for k in range(256) if pal[k * 3 : k * 3 + 3] == act[(255 - k) * 3 : (255 - k) * 3 + 3])
        print(f"{name:30s} {label:>15s}  {direct:>6d}/256  {rev:>6d}/256")


if __name__ == "__main__":
    args = sys.argv[1:] or sorted(glob.glob("local_chars/*/"))
    if not args:
        raise SystemExit("no character folders found (run from the repo root)")
    main(args)

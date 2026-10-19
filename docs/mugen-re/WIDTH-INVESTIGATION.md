# MUGEN 1.0 width subsystem — investigation

Scope: read-only reverse engineering of `MUGEN 1.0` (`local_mugen/extracted/mugen/mugen.exe`,
Win32 PE32 i386, image base `0x400000`) plus the shipped 1.0 docs, to answer five questions about
the **width bar** the engine currently no-ops. Notes only; no extracted code or data is copied
into `app/`.

Method / artifacts:

- Raw disassembly: `objdump -d -M intel` dump of `mugen.exe` at `/tmp/mugen_dis.txt`
  (~196k lines). All `file:line` references below are lines in *that* dump; every claim is
  keyed by virtual address (VA) so it can be re-checked without the dump. VAs are file-image
  addresses (i.e. `0x400000 + RVA`).
- Decompilations under `docs/mugen-re/decompiled/` (angr), used only as a cross-check for
  record layouts and trigger-name → opcode mapping.
- Official docs under `local_mugen/extracted/mugen/docs/`.

## Struct/field map used throughout

Player (`edi`/`esi`/`ebp` in the snippets below), verified by the sites that read and write them:

| offset | meaning |
|---|---|
| `+0x1e8` | `facing` (1 or −1) |
| `+0x1f4` / `+0x1f8` | `x` / `y` (floats) |
| `+0x220` / `+0x224` | screen-space **left** / **right** bound for this player (ints, see Q2) |
| `+0xe58` / `+0xe5c` | `statetype` / `movetype` |
| `+0x84/+0x88/+0x8c/+0x90` | the four raw `[Size]` widths: ground.back / ground.front / air.back / air.front (ints) |
| `+0x240/+0x244` | **edge** width: front / back (ints, absolute — see Q2/Q5) |
| `+0x248/+0x24c` | **player** width: front / back (ints, used **additively** — see Q3) |

`[player+0x0]` is the character-constants struct. In it: `+0xa4` `size` scale (float, default 1.0),
`+0xb8` version/compat int, `+0x2ec/+0x2f0` `xscale`/`yscale` (floats, default 1.0),
`+0x2f4/+0x2f8/+0x2fc/+0x300` the four `[Size]` widths, `+0x304` `height`.

Note a recurring idiom: widths are rounded to ints by
`signed-int → float → *[const+0xa4] → +0.5 (qword 0x4a6ba0) → floor (call 0x49a644) → ftol (call 0x499690)`,
i.e. **round-half-up after scaling**, and the operand order gives `round_half_up(scale * width)`.

---

## Q1 — Player width at rest: keys, spellings, defaults, ground/air selection, signedness

**Answer.** The four widths live in the character's `[Size]` block under the exact keys
`ground.back`, `ground.front`, `air.back`, `air.front`, plus `height`. They are parsed as
**integers**. All four widths are **required** — a missing one is a hard load error. `height` is
optional and defaults to 60.0. At runtime the applicable raw width is selected by
`statetype == 3` (air) → `air.*`, everything else → `ground.*`, then by `facing == 1` → the
`front` value, else the `back` value. Values are stored as signed ints and converted with
`signed` loads (`fild`), then scaled by the character's `size` scale.

**Evidence — the parser** (`0x46792b`–`0x4679b7`, dump lines 131372-131415). Each key is passed
with its destination field address and a string:

```
46792b: lea edx,[ebx+0x2f4]              ; ground.back  -> const+0x2f4
467938: mov eax,0x4a272c                ; "ground.back"
46793f: call 0x433730
467947: test eax,eax
467949: je   0x468290                   ; missing -> ERROR
46794f: lea eax,[ebx+0x2f8]  ; "ground.front" (0x4a2738)  -> const+0x2f8, also required
46796d: lea ecx,[ebx+0x2fc]  ; "air.back"     (0x4a2748)  -> const+0x2fc, also required
46798b: lea edx,[ebx+0x300]  ; "air.front"    (0x4a2754)  -> const+0x300, also required
4679a9: lea eax,[ebx+0x304]  ; "height"       (0x4a277c)  -> const+0x304, NO test/je follows
```

Strings read out of the binary (via the PE section table, not the dump):
`0x4a272c "ground.back"`, `0x4a2738 "ground.front"`, `0x4a2748 "air.back"`,
`0x4a2754 "air.front"`, `0x4a2760 "Can't load Size constants"`, `0x4a277c "height"`,
`0x4a2708 "Size"`, `0x4a2710 "Can't access Size constants"`.

The error path `0x468290` (`468295: push 0x4a2760` then `call [0x49b1c4]`, an engine log/print)
emits `"Can't load Size constants"` for any of the four missing keys. A missing `[Size]` section
itself is caught earlier — `0x4677e6` loads `"Size"` (`0x4a2708`), `0x4677f5 jne`, and
`0x4677f7: push 0x4a2710` → `"Can't access Size constants"`. Immediately after, the scale
defaults are written: `467801: fld1; 467809: fst [ebx+0x2f0]; 467811: fstp [ebx+0x2ec]`
(`xscale`/`yscale` = 1.0, `0x467801`-`0x467813`).

**Evidence — which raw width applies**, from the push code (`0x46cea9`-`0x46cee2`):

```
46cdcf: mov ebp,0x1              ; ebp == 1 throughout, so "cmp ecx,ebp" == "facing == 1"
46ceaf: mov ebx,0x3
46ceb4: cmp [edi+0xe58],ebx      ; statetype == 3 ?
46ceba: jne 0x46ced0             ; no -> ground
46cebc: cmp ecx,ebp              ; facing == 1 (ebp==1) ?
46cebe: jne 0x46cec8
46cec0: mov eax,[edi+0x90]       ; air.front
46cec8: mov eax,[edi+0x8c]       ; air.back
46ced4: mov eax,[edi+0x88]       ; ground.front
46cedc: mov eax,[edi+0x84]       ; ground.back
```

**Signedness**: `fild [esp+0x14]` on the dword (`0x46cefc`, and again `0x46cf7c`) is the signed
integer→float conversion, so a negative width is representable and would shrink/invert the bar;
there is no unsigned clamp visible at the parse site or the consumers. Values reach the
consumers as ints; the float conversions happen at use time, not at parse time.

**Docs**: the 1.0 docs never document the `[Size]` width keys outside the tutorial example
(`local_mugen/extracted/mugen/docs/tutorial3.html:225-228`, `ground.back = 15`,
`ground.front = 16`, `air.back = 12`, `air.front = 12`, `height = 60` with the comment
"Generally fine at 60"), so **the docs give no defaults for the four widths** — and the
reference agrees with that reading in the only sense that matters: there is no default, a
missing key is fatal. The docs *do* document the scale keys
(`docs/cns.html:299-301`: "In `[Size]`, you can use `xscale` and `yscale` to change the width
and height of your character") — see the disagreement note below.

**Confidence: high** for keys, requirement, storage offsets, the statetype/facing selection and
signedness. **Unsettled (low)**: the site that copies `const+0x2f4..+0x300` into the player's
`+0x84..+0x90` was not located — the push code and the `size.*` triggers read the player copy,
but the copy step itself is not in the blocks I traced. It is presumably part of the player
construction/`ChangeState`-to-self re-init, and it is a plain int copy.

**Docs vs reference (Q1)**: `cns.html:299-301` says `xscale`/`yscale` change the width and height
of the character. In the binary, `xscale`/`yscale` are read at `0x467052`-`0x467072` and written
into the **draw globals** `[0x4a9cc4]`/`[0x4a9cc8]`, whereas every width computation multiplies
by `[const+0xa4]` — a different field (`size`). So the docs' claim that `xscale`/`yscale` scale
widths is at best indirect; the width path uses `+0xa4`. Treat `xscale/yscale = draw geometry`
vs `+0xa4 = width/height scale` as the reference's actual behaviour.

---

## Q2 — Edge width at rest: persistence, the clamp, and the stage bound

**Answer.** Edge width **starts at 0 and is 0 unless the `Width` controller sets it**; it is not
derived from `[Size]`. It is a per-player pair of ints (`+0x240` front, `+0x244` back) that is
written only by the `Width` applier (absolute store, no facing flip) and cleared only by the
player-reset blocks. The screen-edge clamp is

```
x ∈ [ leftBound  + round_half_up(scale * edgeWidthTowardLowX),
      rightBound − round_half_up(scale * edgeWidthTowardHighX) ]
```

where the "toward low x" width is `facing == 1 ? edgeBack : edgeFront` and the "toward high x"
width is `facing == 1 ? edgeFront : edgeBack`. Equivalently: the width bar spans
`[x − wBack, x + wFront]` (mirrored when facing left) and the clamp keeps that span inside
`[leftBound, rightBound]`. `leftBound`/`rightBound` come from camera state, **not** from the
stage `.def` bounds directly.

**Evidence — the clamp** (`0x46c650`-`0x46c77e`, dump lines 136909-136998). Gate:

```
46c657: mov eax,[0x52b40c]                ; engine
46c65c: cmp [eax+0x10f40],1              ; movement-constraint enable
46c663: jne 0x46c672
46c665: cmp [edi+0xe80],0   ; je 0x46c779 -> skip the whole clamp
46c672: mov ebp,[edi+0x1e8]              ; ebp = facing
46c678: cmp ebp,-1 ; jne 0x46c689
46c67d: mov ecx,[edi+0x240]              ; facing == -1 -> EDGE FRONT  (low-x side)
46c689: mov edx,[edi+0x244]              ; facing == +1 -> EDGE BACK   (low-x side)
   ; fild; fmul [esi+0xa4]; +0.5; floor; ftol  -> ebx = round(scale*that)
46c6c2: cmp ebp,1 ; jne 0x46c6d3
46c6c7: mov eax,[edi+0x240]              ; facing == +1 -> EDGE FRONT (high-x side)
46c6d3: mov ecx,[edi+0x244]              ; facing == -1 -> EDGE BACK
   ; -> esi = round(scale*that)
46c70a: call 0x46c5b0                    ; computes the two screen bounds
46c70f: mov edx,[edi+0x220] ; add edx,ebx ; fild; fld [edi+0x1f4]; fcom st(1)
46c733: test ah,0x41 ; jne 0x46c740
46c738: fstp [edi+0x1f4]                 ; x = lo  (if x <= lo)
46c742: mov eax,[edi+0x224] ; sub eax,esi
46c766: test ah,0x5 ; jp 0x46c777
46c76b: fstp [edi+0x1f4]                 ; x = hi  (if x > hi)
```

The `test ah,0x41` / `test ah,0x5` pairs are the x87 `fcom` + `fnstsw` idioms for "`x <= lo`" and
"`x > hi`" respectively (C0|C3 set / C0||C2 clear). Note the low side adds the edge width to the
left bound and the high side subtracts from the right bound — an edge width of 0 is exactly a
no-op, which is why the controller is safe to ignore in states that never use it.

**Evidence — the bounds** (`0x46c5b0`-`0x46c64b`, dump lines 136856-136904):

```
leftBound  = ftol( (float)(int)[eng+0x1c20]*[eng+0x1b74]/[player+0x13a8] + [eng+0x10ef8]
                    - (int)(float)[eng+0x11840]/2 )
rightBound = ftol(            [eng+0x10ef8]
                    - (float)(int)[eng+0x1c1c]*[eng+0x1b74]/[player+0x13a8]
                    + (int)(float)[eng+0x11840]/2 )
```

with `eng+0x11840` = screen width float (640 from the config), `eng+0x10ef8` = camera x offset,
`eng+0x1b74` = zoom, `eng+0x1c1c`/`+0x1c20` = the camera half-width terms, `player+0x13a8` the
bound divisor. There is **no read of a stage `boundleft`/`boundright` field anywhere in this
function** (`grep` of the body finds only those engine globals). So the stage bound participates,
if at all, only transitively — via whatever moves the camera and sets `+0x220`/`+0x224`.
**Confidence: high** for the expression and the field roles; **medium** for the "stage bound is
only transitive" claim (it is a negative finding over this function, not a proof about the
camera writer).

**Evidence — where `+0x240/+0x244` are written** (complete list from a scan of every store to
those offsets in `.text`): `0x46a7df`-`0x46a7f1` and `0x46a7e5`/`0x46a7eb` (player init block,
storing 0), `0x46b593`-`0x46b5a5` (inside the per-tick player update at `0x46b440`, storing 0 —
see Q5), and `0x4793e9`-`0x47940d` (the `Width` applier). Reads are `0x46c650` (this clamp),
`0x486969`/`0x486a41` (the 0xb/0xc triggers), `0x470715` (a second clamp caller) and
`0x471441`. So **at rest the edge width is 0** and only `Width` ever makes it nonzero. There is
no `[Size]`-derived initialization of these fields.

**Docs**: `sctrls.html:3177-3178`: "Width — Temporarily changes the size of the player's width
bar for 1 tick. Useful for controlling the "pushing" behavior when the player makes contact
with another or with the sides of the screen."
`sctrls.html:3184-3187`: "edge = edgewidth_front, edgewidth_back (int, int) — Sets the player's
edge width in front and behind. Edge width determines how close the player can get to the edge
of the screen. These parameters default to 0,0 if omitted." The docs' "default to 0,0" matches
the reference's zero-fill at player init. The docs do **not** say that edge width scales with
`[Size]`, and indeed the reference scales it by the character's `size` only (see Q1 note).

**Confidence: high.**

---

## Q3 — The push-apart rule: exact arithmetic

**Answer.** It is genuinely **width-dependent — there is no gap constant**. The pushing rule is
a two-part rule operating on the horizontal axis only:

1. Width of a player toward the other one:
   `W = round_half_up( scale * ( rawSizeWidthTowardOpponent + playerBarWidthTowardOpponent ) )`
   where `rawSizeWidthTowardsOpponent` is `air.*`/`ground.*` per `statetype == 3`, front/back per
   facing, and `playerBarWidthTowardsOpponent` is `+0x248` (front) or `+0x24c` (back). The bar
   value is **added** to the raw `[Size]` value, not substituted for it.
2. Let `dx = xRight − xLeft` (after ordering the two players so that `xLeft <= xRight`), and
   `S = W_left + W_right`.
   - If `S > dx`: `push = (S − dx) * 0.5`; `xLeft −= push`, `xRight += push` (split equally about
     the midpoint — no mass/facing weighting).
   - Independently, if `dx < (int)screenWidth / 32` (`= round(640/32) = 20` at 640), the pair is
     recentred about the midpoint: `xLeft = mid − (int)screenWidth/64` (`−10`),
     `xRight = mid + (int)screenWidth/64`, where `mid = (xLeft + xRight) * 0.5`. The comparison in
     the second branch uses the **pre-recentre** `dx` (the value is computed once, earlier).

There is no minimum-gap floor and no clamp of the resulting positions in this function (unlike
the edge clamp, this pass does not re-apply the screen bounds).

**Evidence** (`0x46cdc0`-`0x46d0c7`, dump lines 137526-137746).

Vertical gate (`0x46cdc9`): if the right player's statetype is 1 (standing) and
`left.y − right.y > round(scale * [left+0x94])` → return (no push between a standing player and
one already above its height). `+0x94` is `height`.

Ordering (`0x46ce42`-`0x46cea7`):

```
46ce63: fucompp
46ce65: fnstsw ax
46ce67: test ah,0x44
46ce6a: jp 0x46cea9        ; not equal -> keep order
   ; equal: tie-break on facing pair, then movetype==1 (+0xe5c), then statetype==3
46cea3: mov eax,edi ; mov edi,esi ; mov esi,eax   ; swap: edi becomes the left/low-x player
```

The `test ah,0x44; jp` is "one of C0/C2 set" ⇒ the compared pair is not equal; when it *is*
equal the code falls through to the facing/movetype/statetype tie-break. So after this block
**`edi` is the low-x player** (`left`) and `esi` the high-x one (`right`).

Per-player width (`0x46cea9`-`0x46cf22`, shown in Q1) is `fild` of
`raw[Size width] + [0x248/0x24c]`, `fmul [const+0xa4]`, `+0.5`, `floor`, `ftol` — for `edi` the
`front` bar value is `+0x248` when `facing == 1` and `+0x24c` when `facing == −1`; mirrored for
`esi` (`0x46cf61`-`0x46cf6e`). The 0x24x term is added at `0x46cef6`/`0x46cf74`
(`add ecx,eax` where `eax` came from `+0x84..+0x90` and `ecx` from `+0x248`/`+0x24c`).

Branch A (`0x46cfa7`-`0x46d03c`) — the recentre. Note the screen width is loaded as
`lea ebx,[engine+0x11840]`, so `[ebx]` is the screen width:

```
46cfa7: fld [esi+0x1f4] ; fsub [edi+0x1f4]      ; dx = right.x - left.x
46cfd4: fild <ftol(dx)> ; fmul qword [0x4a6c88] ; fld qword [0x4a6ca0] ; fdiv st(1),st
46cff0: fcomp st(2) ; test ah,0x41 ; jne 0x46d065   ; branch A iff dx >= (int)W/32
46cff9: fld [edi+0x1f4] ; fadd [esi+0x1f4] ; fmul qword [0x4a6ba0]   ; mid = (L+R)*0.5
46d013: fld st(0) ; fld [ebx]                   ; [ebx] = screen width
   ; ... ftol, fmul qword [0x4a6c80], fdiv by 4 (st(4)) -> (int)W/64
46d038: fsubp st(2),st                          ; one coordinate = mid - (int)W/64
46d03c: fstp [edi+0x1f4]                        ; left.x = mid - (int)W/64  (branch A)
```

The `fadd qword [0x4a6ba0]` (0.5) + `floor` + `ftol` again is round-half-up. In the same block
the second store (into `esi`, the right player) is `mid + (int)screenWidth/64`, and `0x4a6c88`
is the `1/32` factor while `0x4a6c80` is `1/64`.

Branch B (`0x46d065` onward) — the push itself:

```
46d065: ... fild [esp+0x18]  (W_left) ; fild [esp+0x1c] (W_right) ; fadd
   ; sum = W_left + W_right ; compare sum against the ORIGINAL dx (fcom st(1))
   ; if (sum > dx): push = (sum - dx) * 0.5
   ;   left.x  -= push ;  right.x += push
```

The two stores are `fsubp`/`faddp` against `[edi+0x1f4]`/`[esi+0x1f4]`; `[0x4a6ba0]` (0.5) is
applied to the difference first, so each side moves by exactly `(sum − dx) / 2`.

**Confidence: high** for the arithmetic, the additive bar term, the equal split and the `dx`
ordering. **Medium** for the exact tie-break rule when `xLeft == xRight` (the facing-pair /
`movetype == 1` / `statetype == 3` ladder at `0x46ce6c`-`0x46cea7` is a compiler-flattened
boolean chain; I read it as "swap so that the player facing right/away ends up as `left`", but
the two mirrored halves use different registers for the constant 3, so I will not claim the
exact predicate).

**Docs**: nothing in the 1.0 docs states the push arithmetic; `sctrls.html:3178` only calls the
`Width` controller "useful for controlling the "pushing" behavior when the player makes contact
with another". `tutorial3.html:222-223` comments the `[Size]` widths with "These affect how
close another character can get to yours." So the docs and the reference agree at the level of
"wider = stands further apart", and the docs are silent on the formula (no disagreement).

---

## Q4 — the `0xb`/`0xc` trigger evaluators

**Answer.** `0xb` = `FrontEdgeDist` / `FrontEdgeBodyDist`, `0xc` = `BackEdgeDist` /
`BackEdgeBodyDist`; the docs' "body" variants are the **same opcodes with `num2` (the trigger
node's `+0x24` field) set to 1**. What `num2 = 1` changes: the reference point moves from the
player's x-axis to the **end of the width bar** — it adds `round_half_up(scale * edgeWidth)` on
the relevant side (`+0x240` front for `0xb`, `+0x244` back for `0xc`) — and, in both variants, it
also adds a small statetype constant (+1 if `statetype == 3`, +2 if `statetype == 4`). The bound
is the **facing-side screen bound**: `0xb` uses `+0x224` (right) when `facing == 1`, `+0x220`
(left) otherwise; `0xc` is the mirror. The returned value is divided by the owner's own scale
via `0x4652a0`, so it is expressed in the same units as a `[Size]` width rather than scaled
pixels.

**Evidence — opcode mapping** (`docs/mugen-re/decompiled/angr_00482b00_expression_eval.c:743-760`):

```c
else if (!_stricmp(&v23, "frontedgedist"))     { v49 = 0xb; }
else if (!_stricmp(&v23, "backedgedist"))      { v49 = 0xc; }
else if (!_stricmp(&v23, "frontedgebodydist")) { v49 = 0xb; num2 = 1; }
else if (!_stricmp(&v23, "backedgebodydist"))  { v49 = 0xc; num2 = 1; }
```

This is decisive: the body variants are not separate opcodes, they are the plain ones with a
flag. `num2` in the compiled tree is what the evaluator reads at `+0x24`.

**Evidence — the `0xb` evaluator** (`0x486969`-`0x486a3c`, dump lines 170609-170667). Prelude
shared by both opcodes:

```
486974: mov eax,[edx+0x24]          ; num2 ("body")
486977: xor edi,edi
486979: test eax,eax ; je 0x486993  ; offset = 0 for the plain variant
48697d: mov ecx,[ebp+0xe58]
486983: cmp ecx,3 ; jne 0x48698b
486988: lea edi,[ecx-2]            ; statetype == 3 -> +1
48698b: cmp ecx,4 ; jne 0x486993
486990: add edi,2                  ; statetype == 4 -> +2
486993: cmp [edx+0xc],0xb
486997: jne 0x486a41               ; else -> the 0xc path
48699d: test eax,eax ; je 0x4869db  ; only the body variant adds the width
4869a4: mov edx,[ebp+0x240]          ; EDGE FRONT
   ; round(scale*edgeFront) added to edi  ->  add edi,eax at 0x4869d9
4869db: mov esi,[ebp+0x1e8]         ; facing
4869e1: cmp esi,1 ; jne 0x4869f2
4869e6: mov ecx,[ebp+0x224]          ; facing == +1 -> RIGHT bound
4869f2: mov edx,[ebp+0x220]          ; facing == -1 -> LEFT bound
4869fc: mov eax,esi ; imul eax,edi  ; facing * offset
486a06: fild [esp+0x44] ; fadd [ebp+0x1f4]      ; x + facing*offset
486a14: fisub [esp+0x3c]                        ; - bound
486a23: call 0x4652a0                           ; scale conversion
486a28: neg esi ; fimul [esp+0x5c] ; ftol       ; * (-facing)
```

So `FrontEdgeDist/BodyDist = ftol( (x + facing*offset − boundFront) * (−facing) / scale )`, i.e.
for `facing == 1`: `rightBound − x − offset`; for `facing == −1`: `x − offset − leftBound`
(front points at lower x). With `offset = 0` this is the plain distance from the x-axis; with
`num2 = 1` it is `edgeFront (+ statetype adj)` further in.

**Evidence — the `0xc` evaluator** (`0x486a41`-`0x486af6`, dump lines 170668-170716):

```
486a41: test eax,eax ; je 0x486a99     ; only the body variant
486a45: mov ecx,[ebp+0x1e8] ; mov eax,ecx ; neg eax
486a4f: cmp ecx,eax ; jne 0x486a5f     ; "facing == -facing" -- always false
486a53: mov ecx,[ebp+0x240]             ; EDGE FRONT (dead branch)
486a5f: mov edx,[ebp+0x244]             ; EDGE BACK  (always taken)
   ; edi += round(scale*edgeBack)
486a99: mov eax,[ebp+0x1e8]
486aa3: cmp eax,1 ; jne 0x486ab4
486aa8: mov ecx,[ebp+0x220]             ; facing == +1 -> LEFT bound
486ab4: mov edx,[ebp+0x224]             ; facing == -1 -> RIGHT bound
   ; value = ftol( (x - facing*edi - bound) / scale * facing )
```

i.e. for `facing == 1`: `x − edgeBack − leftBound` (`BackEdgeBodyDist`), and `x − leftBound`
for the plain `BackEdgeDist`. The `cmp ecx, eax` where `eax = -facing` is a dead test (it can
only be true if `facing == 0`), so the 0xc body variant is *always* back-side; presumably the
source was a mirrored copy of the 0xb code and the compiler could not prove the branch dead.

**Docs** — `docs/trigger.html:682-696` (BackEdgeBodyDist): "BackEdgeBodyDist gives the distance
from the back of the player, as determined by the end of his width bar, to the back edge of the
screen." with the example `trigger1 = BackEdgeBodyDist < 30`; `trigger.html:1240-1254`
(FrontEdgeBodyDist): "gives the distance between the front of the player (as determined by the
front edge of his width bar) and the edge of the screen."; `trigger.html:701-716`
(BackEdgeDist): "distance between the x-axis of the player and the edge of the screen behind of
the player"; `trigger.html:1259-1274` (FrontEdgeDist) the mirror. The reference **agrees** with
the docs on all four: x-axis vs width-bar-end reference point, and facing-side screen edge.
The docs do not mention the statetype `+1/+2` and do not say which underlying field supplies the
bar end — those are reference-only details.

**Confidence: high** for the expressions, the reference point and the bound; **high** for the
`num2` mechanism (the expression compiler is explicit); **medium** for the purpose of the
`+1`/`+2` statetype constant (observed, unexplained — it is not derivable from the docs, and it
is added to *both* variants identically).

---

## Q5 — the `Width` controller: record, consumer, and the "1 tick" lifetime

**Answer.** `Width` compiles to a controller record with tag **40 (0x28)** and four 12-byte
parameter slots, in this order:

| record offset | slot # | `Width` keyword | player field |
|---|---|---|---|
| `+0x18` | 0 | `edge` first value = edge**front** | `+0x240` |
| `+0x24` | 1 | `edge` second value = edge**back** | `+0x244` |
| `+0x30` | 2 | `player` first value = player**front** | `+0x248` |
| `+0x3c` | 3 | `player` second value = player**back** | `+0x24c` |

All four slots are pre-set to the constant 0 (`sub_438d20(0)` ×4), so omitted parameters mean
"set to 0", matching the docs' "default to 0,0". The `value` alias copies the two `edge` slots
into the two `player` slots. The consumer is the tag-40 handler at `0x4793e9`, which evaluates
each slot as an integer expression (`call 0x439260`) and stores it **absolutely** into the four
player fields — there is **no facing flip** (the front value always goes to `+0x240`/`+0x248`
regardless of facing; the facing-relative selection happens at the *read* sites, not here).

**Evidence — record construction** (`docs/mugen-re/decompiled/angr_00473ed0_controller_dispatch.c`,
the Width case): `*(unsigned int *)&v14[16] = 40;`; the four slots are freed and
re-initialised with `sub_438c40(...)` / `sub_438d20(..., 0)`; `edge` is parsed with
`sub_488ff0(..., "ii", ...)` into `v14+24` (0x18) and `v14+36` (0x24); `player` into `v14+48`
(0x30) and `v14+60` (0x3c); the `value` alias ends with `sub_438e30(v14+48, v407);
sub_438e30(v14+60, v406);` — i.e. the edge slots are *copied* into the player slots. The slot
stride of 0x0c at `+0x18, +0x24, +0x30, +0x3c, +0x48, +0x54` is confirmed by the dispatcher's
per-slot initialisation at `0x473ed0`.

**Evidence — the applier** (`0x4793e9`-`0x479449`, dump lines 153472-153502):

```
4793eb: lea edx,[ebx+0x18] ; call 0x439260 ; mov [ebp+0x240],eax   ; edge front
479400: lea edx,[ebx+0x24] ; call 0x439260 ; mov [ebp+0x244],eax   ; edge back
479415: lea edx,[ebx+0x30] ; call 0x439260 ; mov [ebp+0x248],eax   ; player front
47942a: lea edx,[ebx+0x3c] ; call 0x439260 ; mov [ebp+0x24c],eax   ; player back
47943d: xor eax,eax ; ret
```

`0x439260` is the int expression evaluator (`eax` = player, `edx` = slot, `push 1`), so the
controller **reads live expressions** at execution time, not just constants (a character may
write `player = 0, const(240)`).

**Lifetime — what is settled and what is not.** Settled:

- The effect is visible from the moment the controller runs until it is cleared; both
  consumers (`0x46c650` clamp, `0x46cdc0` push, `0x486969` triggers) read the fields directly
  with no copy or snapshot, so the value written is what the next reader sees.
- No unconditional per-tick clear of `+0x240/+0x244/+0x248/+0x24c` exists anywhere in `.text`:
  a scan of every store to those four offsets yields only the player-init block
  (`0x46a7df`-`0x46a7f1`, `ebx = 0`), the block `0x46b593`-`0x46b5a5` inside the per-tick player
  update `0x46b440` (also storing 0 from `ebx`), and the applier `0x4793e9`. There is no
  save/restore pair (no read-then-store-elsewhere-then-write-back) anywhere.
- **The leading candidate for the "1 tick" restore is `0x46b593`-`0x46b5a5`**, inside
  `0x46b440`. Its guard at `0x46b52a` is `cmp [char+0xb8], [engine+0x408]` (`jae 0x46b538`) or
  `cmp edi,0; je` — `engine+0x408` is set once to `0x1000000` at `0x454346`. If that guard is
  the "normal character, not a helper" test (helpers would otherwise get their width zeroed
  every tick), the block is exactly "zero the width bar at the top of the player's own update",
  which reproduces the docs' "for 1 tick": the controller writes, later readers in the same tick
  see it, and the next tick's update clears it.
- I could not prove the guard's per-tick validity or that this block runs once per player per
  tick (as opposed to once per state change / on some other trigger). **Confidence: medium.**
  This is the one part of Q5 I am explicitly leaving as "closest candidate, unproven" rather
  than a reading.

**Docs**: `sctrls.html:3177-3178` — "Temporarily changes the size of the player's width bar for
1 tick"; `sctrls.html:3184-3199` for the four parameters and the `value` shorthand
("This is a shorthand syntax for setting both edge width and player width simultaneously. This
may only be used if the edge and player parameters are not specified."). The docs' phrase
"Sets the player's width in front and behind" is the one place docs and reference **disagree in
spirit**: the docs say a player width *determines how close the player can get to other
players*, which reads like a replacement, while the reference **adds** `+0x248/+0x24c` to the
raw `[Size]` width (Q3) and never replaces it. Implement the additive behaviour.

**Confidence**: record layout, slot→field mapping, `value` aliasing, absolute store, no facing
flip, and the absence of any other writer — **high**. The 1-tick restore mechanism — **medium**
(candidate identified, guard unproven).

---

## What the engine does today (for the implementer, read-only observations)

- `app/src/main/cpp/engine/statemachine.cpp:2571` — `if (type == "width") return true;` — the
  controller is accepted and ignored.
- No `backedgedist` / `frontedgedist` exist at all; only the body variants are implemented, and
  those are computed from **hurt boxes**, not the width bar:
  `statemachine.cpp:5792-5798` use `bodyExtentTowardOpponent()` (`statemachine.cpp:5599`, which
  falls back to a hard-coded `12.0f` and walks `animation->hurtBoxes`), and they measure against
  `stageMin_`/`stageMax_` rather than the screen bounds.
- The push rule uses a constant: `statemachine.cpp:5586 constexpr float kMinGap = 26.0f;`,
  `:5589 const float overlapAmount = kMinGap - distance;` — axis-based, not width-based, and
  clamped to the stage rather than measured from the width bar.
- `statemachine.cpp:4443` clamps `x_` to `stageMin_`/`stageMax_` gated on `screenBound_`
  (`:2317`), with no edge-width terms.
- `app/src/main/cpp/engine/fighter.cpp:35 constexpr float kPlaceholderHalfWidth = 12.0f;` — the
  placeholder half-width used wherever the width bar should be.

To match the reference, the engine needs: a per-fighter pair of edge widths (front/back) and a
pair of player-bar widths (front/back), both ints, zero at rest; the `[Size]` widths as the base
that the player-bar values are **added** to; the clamp
`x ∈ [leftBound + wLow, rightBound − wHigh]` with `wLow/wHigh` chosen by facing; width-based
pushing `(S − dx)/2` plus the `dx < screenWidth/32` recentre; the four edge triggers with the
`num2` body variant; and the per-tick clear.

## Unsettled / no evidence found

1. **The one-tick restore mechanism** for `+0x240/+0x244/+0x248/+0x24c`. No unconditional clear
   found; closest (and only) candidate `0x46b593`-`0x46b5a5` inside the per-tick player update
   `0x46b440`, guarded at `0x46b52a` by `[char+0xb8] >= [engine+0x408] (0x1000000)` or
   `edi == 0`. Not proven per-tick. Medium.
2. **The copy step** `const+0x2f4..+0x300` → `player+0x84..+0x90` was not located. Low; it is
   certainly a plain int copy somewhere in player construction/re-init.
3. **The `+1 (statetype == 3) / +2 (statetype == 4)` constant** added inside the body variants
   of `0xb`/`0xc`, and the **dead `facing == -facing`** test in the `0xc` body variant. Observed
   in bytes; purpose unknown. Medium (behaviour), unsettled (intent).
4. **Whether the stage `.def` `boundleft`/`boundright` reach the edge clamp at all**, or only
   through the camera fields (`engine+0x10ef8`, `+0x1b74`, `+0x1c1c`/`+0x1c20`). No stage-bound
   read inside `0x46c5b0`. Medium.
5. **The exact equal-x tie-break** in the push ordering (`0x46ce6c`-`0x46cea7`): the
   facing-pair / `movetype == 1` / `statetype == 3` ladder is a flattened boolean chain and I
   will not claim its precise predicate. Medium.
6. **`0x4652a0`'s exact semantics** (used by the edge triggers): read as "convert a value into
   the owner player's scale", i.e. divide by `[[owner]+0xa4]`. For `scale == 1` it is a no-op,
   so this only matters for scaled characters. Medium.

# Guard chain RE — who writes the blocker's clocks, who ticks them, and who ends the chain

Reference: `mugen.exe` (MUGEN 1.0, PE32 i386, MSVC 9.0, image base `0x400000`,
sha256 `887d00a0…3f9a06c4`), read from `/tmp/mugen_peek/mugen/mugen.exe` and its
objdump text `/tmp/mugen_peek/mugen_disasm.txt`. All file offsets below are
VAs.

This pass answers the three questions left open by FINDINGS pass 18 §18.4
("whether the reference holds a blocker for shake + hittime, for the longer of
the two, or only for the file's action is unestablished") and by
`decompiled/INDEX.txt`'s entry for `0x46E7C0`.

Two independent sources are used: the binary, and the MUGEN 1.0 distribution's
own shipped documentation, read read-only from
`local_mugen/extracted/mugen/docs/` (never copied into the repo). Where the two
disagree, the binary wins and the disagreement is recorded.

## 0. Answers in one screen

| question | answer | confidence |
|---|---|---|
| Who writes the four victim clocks? | The **block-reaction branch inside the function at `0x46E7C0`** (`guard_clocks`), entered only from hitresolve's *guarded* branch at `0x46DA1D`. The ledger's attribution is **correct** (the four writes at `0x46E8E4/E943/E94F/E978/E9D1` fall inside `0x46E7C0 + 0x2A3`). They are not inline in hit-application. | HIGH |
| Who decrements them? | Two decrements only, both in the per-frame player update at `0x46B440`: **`+0x1090` at `0x46B732`** (inside the `movetype == H` branch, `if > 0`) and **`+0x1094` at `0x46C1B1`** (common path, `if >= 0 && +0x1090 == 0`). **`+0x1098` and `+0x109C` have no decrement site in the whole binary** — they are write-once constants. | HIGH |
| Is the decrement gated by hitpause? | Yes: both decrements are downstream of the hitpause early-out `0x46B6C5 → 0x46C231`. Separately, while `+0x1090 > 0` the engine force-clears the player's own hitpause pair at `0x46B467/0x46B46D`, so the shake clock always ticks. | HIGH |
| Does hittime tick during the shake? | **No.** It is frozen until `+0x1090` reaches 0. The decisive question therefore resolves to **"shake + hittime, sequential"**, not `max`. | HIGH |
| What do `hitshakeover` / `hitover` compile to? | `hitshakeover` ≡ `+0x1090 <= 0` (`setle` at `0x48759A`); `hitover` ≡ `+0x1094 < 0` (`setl` at `0x4875AC`), opcodes `0x65`/`0x66`. | HIGH |
| Does anything engine-side end 150/151/152/153? | **No.** The engine writes statenos `150/152/154` (`0x46E8CF/E8C3/E8B7`) and nothing else in the guard family; it has no write of `151/153/155` or `130/131` anywhere. The chain is the character's own `common1.cns`. The engine *does* own one transition — state `140`'s return to idle, which the shipped `common1.cns` leaves commented out as "Implemented within engine". | HIGH |

## 1. Q1 — who writes and who decrements the four clocks

### 1.1 The writes: the `sub_46E7C0` attribution is correct

`/tmp/mugen_peek/ghidra_funcs_46_49.txt` gives the boundaries:

```
FUNC 0x46d2d0 size=0x841   -> 0x46D2D0..0x46DB11   hitresolve
FUNC 0x46db20 size=0xbd8   -> 0x46DB20..0x46E6F8   hitapply (clean-hit reaction)
FUNC 0x46e760 size=0x56    -> 0x46E760..0x46E7B6   damage helper
FUNC 0x46e7c0 size=0x2a3   -> 0x46E7C0..0x46EA63   guard_clocks  <-- the four writes
FUNC 0x46eb80 size=0x849   -> 0x46EB80..          pair writer
```

The guard-reaction code that writes the blocker's four clocks runs
`0x46E880 – 0x46E9FD`, which is **inside** `0x46E7C0`'s range and *after* the
`0x46DB20` clean-hit function has ended. So the ledger's attribution
("the block-reaction path around `sub_46E7C0`") is right, and no correction is
needed.

Caveat worth recording: the artifact `decompiled/angr_p9_hitapply.c` is labelled
`0x46db20`, but it also contains the guard-reaction writes (its `field_1090`
write near `:584`, the `field_1094/field_1098` pair near `:356-357`). That file
therefore covers both functions — angr did not split at `0x46E7C0`. Anyone
citing it should cite the region, not the label.

The path, in order (all `%esi` = blocker):

- `0x46E880 – 0x46E8B2` — branch on the blocker's statetype `+0xE58`:
  `1 → stateno 0x96 (150)`, `2 → 0x98 (152)`, `3 → 0x9A (154)`, else the
  internal error string at `0x4A2F7C`.
- `0x46E8D9 – 0x46E8E4` — `+0x1090 ← hitdef+0x180` (**guard.pausetime's second
  value**), unconditional for all three statetypes.
- `0x46E906 / 0x46E910 / 0x46E916 / 0x46E91C / 0x46E922 / 0x46E928 / 0x46E92E` —
  `+0xE68 = 1`, `movetype = 2 (H)`, `+0x21C = 0`, `+0xE54 (time-in-state) = 0`,
  `+0xE64 (ctrl) = 0`, and **`+0xE74 = 0`, `+0xE70 = 0`** (the blocker's own
  hitpause pair is cleared).
- `0x46E934 – 0x46E992`, only when statetype is S or C —
  `+0x1094 ← hitdef+0x170`, `+0x1098 ← hitdef+0x174`, `+0x109C ← hitdef+0x178`,
  plus the guard.velocity pair scaled into `+0x10B0`/`+0x11A4`.
- `0x46E995 – 0x46E9ED`, only when statetype is A —
  `+0x109C ← hitdef+0x194` (**airguard.ctrltime**) and the airguard.velocity
  pair; **`+0x1094`/`+0x1098` are not touched**, so an air block inherits the
  previous hit's numbers. Confirms pass 18 §18.2.
- `0x46E9F0 – 0x46E9FD` — `+0x1084 += sub_46E760(hitdef+0x28, hitdef+0x3C)`,
  i.e. the guard.damage pair is accumulated.

The only call site is hitresolve, `0x46DA1D`, in the branch that first writes
the **attacker's** pending pause `+0xE74 ← hitdef+0x184` (guard.pausetime's
first value) at `0x46D9FE`. So "this was a block" is decided in hitresolve, and
the block reaction is built there.

### 1.2 The decrements — two, and only two

`+0x1090`, `0x46B728`:

```
46b728  movl 0x1090(%ebp),%eax        ; inside the movetype==H branch of 0x46B440
46b72e  cmpl %ebx,%eax                ; ebx == 0
46b730  jle  0x46b739                 ; skip if <= 0
46b732  decl %eax
46b733  movl %eax,0x1090(%ebp)
```

`+0x1094`, `0x46C1A1` (the common path, reached by *both* the `movetype == H`
branch and the reset branch):

```
46c1a1  cmpl %ebx,0x1094(%ebp)        ; ebx == 0
46c1a7  jl   0x46c1b7                 ; skip if hittime < 0
46c1a9  cmpl %ebx,0x1090(%ebp)
46c1af  jne  0x46c1b7                 ; skip if the shake clock is nonzero
46c1b1  decl 0x1094(%ebp)
```

`grep` over the whole disassembly for `0x1098(` and `0x109c(` returns two
writes and one read each — `0x46E1ED`/`0x46E94F` + `0x487873` and
`0x46E978`/`0x46E9D1` + `0x48787E`. **There is no decrement of slidetime or
ctrltime anywhere in the binary.** They are constants written once at block
entry and read back as `GetHitVar(slidetime)` / `GetHitVar(ctrltime)` for the
rest of the reaction. This is why the shipped `common1.cns` compares them
against `Time` (`Time = GetHitVar(slidetime)`) rather than waiting for them to
reach zero: in 151 and 153, `Time` restarts at 0, so the slide stops
`slidetime` ticks *into the knock-back state*, and control returns `ctrltime`
ticks into it — not `slidetime`/`ctrltime` ticks after the block.

Two behavioural corollaries fall out of §1.1 + §1.2:

- `+0x1090` is cleared to 0 by the reset branch at `0x46B7EC` whenever
  `movetype != H`, so it is a "during the reaction only" counter.
- `+0x1094` is cleared to 0 only in the reset branch, so once a reaction ends it
  is left at `-1` and `hitover` reads true while the player is not being hit —
  which is the documented meaning.

### 1.3 Hitpause gating

At the top of `0x46B440`:

```
46b45f  cmpl %ebx,0x1090(%ebp)   ; ebx == 0
46b465  jle  0x46b473
46b467  movl %ebx,0xe70(%ebp)    ; clear own hitpause
46b46d  movl %ebx,0xe74(%ebp)    ; clear own pending hitpause
```

then the hitpause flag is derived from `+0xE70 > 0` (`0x46B489`) and stored at
`0x34(%esp)`; then at `0x46B6C5`:

```
46b6c5  cmpl %ebx,0x34(%esp)
46b6c9  jne  0x46c231            ; whole body skipped, and 0x46C231 > 0x46C1B1
```

So during a hitpause neither clock ticks — the `+0x1090` decrement at
`0x46B732` and the `+0x1094` decrement at `0x46C1B1` are both inside the skipped
range. And while the shake clock is running, the player cannot be hitpaused at
all, because `0x46B467` re-clears it every frame. Net effect for a block: the
attacker is frozen for `guard.pausetime`'s first value (written at `0x46D9FE`);
the blocker is not frozen, its controllers run, and its shake clock ticks every
frame.

### 1.4 The decisive question: sequential, not max

Because `+0x1094` is gated on `+0x1090 == 0`, and because the shipped
`common1.cns` leaves 150 on `HitShakeOver` and 151 on `HitOver`, the reference's
guard chain length is **shake + hittime, counted one after the other**:

- 150 lasts while `guard.pausetime`'s second value counts down (`S` ticks), then
  `ChangeState 151` fires on the tick `HitShakeOver` turns true.
- 151 lasts while `hittime` counts `H, H-1, … 0, -1`; `HitOver` (i.e.
  `hittime < 0`) fires the tick after the value passes zero, then
  `ChangeState 130`.
- Total held time is therefore ≈ `S + H`, plus the entry tick and the boundary
  tick (`S + H + 1` or `+2` depending on where the contact falls in the frame
  order).

It is *not* `max(S, H)`: a HitDef with `pausetime = 0,0` and `guard.hittime = 15`
still holds the blocker for the full 15, and one with `pausetime = 0,10` and
`guard.hittime = 15` holds for 25.

### 1.5 What is not established (Q1)

- The exact ±1 frame alignment: whether the contact frame itself counts against
  `S`, i.e. whether the total is `S+H+1` or `S+H+2`. I derived the ordering
  (controllers at `0x46B652` run *before* the shake decrement at `0x46B732`,
  which runs before the hittime decrement at `0x46C1B1`) from instruction
  positions, not from a live run. The two boundary ticks are within one frame
  of each other, so this matters only for exact tick-count tests.
- Whether a *second* hit during the shake restarts both clocks from the new
  HitDef. §1.1's writes are unconditional (not "only if smaller"), so almost
  certainly yes, but the interaction with the `+0xE68`/damage path was not
  traced.
- Whether `+0xE70`/`+0xE74` can ever be non-zero for a player that is inside a
  shake — I only found the two clears (`0x46B467`, and the hit/block writes at
  `0x46E518`/`0x46E92E`), never a set, for that case.

## 2. Q2 — what reads `+0x1090` and `+0x1094`

### 2.1 Compile side: name → opcode

`decompiled/angr_00482b00_expression_eval.c:949-969`, the `_stricmp` chain that
turns a trigger's name into its opcode:

```
"hitfall"       -> 0x64      (:949)
"hitshakeover"  -> 0x65      (:953)
"hitover"       -> 0x66      (:957)
"hitpausetime"  -> 0x68      (:965)
"canrecover"    -> 0x69      (:969)
```

This confirms FINDINGS pass 6 §3's opcode numbers. Note that this file contains
**no** reference to `+0x1090`/`+0x1094` beyond a struct declaration — the
constant lives in the runtime handler, not the compiler.

### 2.2 Runtime side: opcode → expression

The runtime handlers are in the function whose entry is `0x4861E0`
(`FUNC 0x4861e0 size=0x2672`, `decompiled/INDEX.txt`), laid out in opcode order
in one `0x4875xx` block — which is itself the best confirmation of the mapping:

```
0x487557  hitfall        movl 0x10c8(%ebp)      -> result
0x487571  (ctrl family)  movl 0x1e8(%ebp)
0x48757c  (ctrl)         movl 0xe64(%ebp)
0x487587  hitfall? (0x64) movl 0x10c8(%ebp)
0x487592  hitshakeover   xorl %edx,%edx ; cmpl %edx,0x1090(%ebp) ; setle %dl
0x4875a4  hitover        xorl %eax,%eax ; cmpl %eax,0x1094(%ebp) ; setl  %al
0x4875b6  hitvel         arg 0 -> 0x1e8 * 0x10b0 (facing), arg 1 -> 0x10b4
0x4875eb  hitpausetime   movl 0xe70(%ebp)
0x4875f6  canrecover     0x10d8 != 0 && 0x10dc >= 0x10e0
```

So, verbatim:

- **`hitshakeover`** evaluates `+0x1090 <= 0`, signed, via `setle` → 1 when the
  shake clock has run out (and also when the player is not in a reaction at
  all).
- **`hitover`** evaluates `+0x1094 < 0`, signed, via `setl` → 1 when the hittime
  clock has passed zero.

The `hitvel` handler also confirms `+0x10B0` is the x-velocity *as facing-signed*
(the `0x1e8` field is the facing/mirror factor) and `+0x10B4` the y-velocity,
without a facing factor.

### 2.3 GetHitVar getters

In the same function, the get-hit-variable jump table's arms read:

```
0x48785d  +0x1090   -> GetHitVar(hitshaketime)
0x487868  +0x1094   -> GetHitVar(hittime)
0x487873  +0x1098   -> GetHitVar(slidetime)
0x48787e  +0x109c   -> GetHitVar(ctrltime)
0x487889  +0x10a0   -> GetHitVar(recovertime)
```

matching the name blob at `0x4A50D8/0x4A50E8/0x4A50F0/0x4A50FC`.

### 2.4 The only other readers: the cosmetic shake

`grep` finds exactly two more readers of `+0x1090`, both in the position/draw
path:

- `0x470970` (inside `0x470860`, size `0x25e`): branches on
  `movetype == 2 && +0x1090 > 0` before loading two floats from `+0x13B4`/`+0x13B8`
  and computing an offset whose sign alternates with the parity of
  `+0xE54` (`andl $0x80000001`).
- `0x470BB2` (inside `0x470AC0`, size `0x5ea`): branches on `+0x1090 != 0` in a
  similar offset computation.

So the "shake" is a jitter applied to where the blocker is *drawn*; it does not
touch the player's position, velocity, animation or state. Nothing else in the
binary reads `+0x1090`.

### 2.5 What is not established (Q2)

- I did not decode the exact jitter formula (amplitude source `0x11840` of the
  engine global at `0x52B40C` is an unread float; I only read the branch
  conditions and the surrounding float loads). The claim "cosmetic draw offset"
  is HIGH on the branch, MEDIUM on the precise arithmetic.
- Whether `0x470860` and `0x470AC0` are the two players' draw passes or two
  layers is not established; only that both are position computations.
- Which opcode value belongs to `0x487557`/`0x487571`/`0x48757C` is not
  established — I only pinned the four that matter by their opcode order
  relative to `hitfall` (`0x64`) and the name table.

## 3. Q3 — does anything engine-side end 150/151/152/153?

### 3.1 The engine's per-frame state table

`0x46B882`, in the per-frame player update, is a second dispatch table on
`stateno`, covering states 40–155 (`subl $0x28; cmpl $0x73`). Its byte selector
lives at `0x46C418` (116 bytes) and its 5-entry target table at `0x46C404`:

```
0x46c404 -> 0x46b895, 0x46b8ad, 0x46b8ec, 0x46b8fd, 0x46b8b3
```

Decoded byte-for-byte from the PE:

| states | index | handler | what the handler does |
|---|---|---|---|
| 40 | 0 | `0x46B895` | `if (time-in-state == 0) +0x21C = +0x10C` |
| 120 | 1 | `0x46B8AD` | `+0xE68 = 1` |
| 130, 131, 132 | 2 | `0x46B8EC` | `+0xE68 = 1`, `+0x1E4 = 100.0/defence` |
| 140 | 3 | `0x46B8FD` | `+0xE68 = 1`, `+0x1E4 = 100.0/defence`, and **if the anim has ended, ChangeState** |
| **150–155** | 1 | `0x46B8AD` | `+0xE68 = 1` |
| everything else in 40–155 | 4 | `0x46B8B3` | nothing (pop the FP stack) |

Two things follow immediately.

1. **There is no handler that takes a player out of 151/153/155.** The guard
   knock-back states fall into the same trivial slot as 150/152/154: the engine
   only re-asserts the block flag there.
2. **The engine does re-assert `+0xE68 = 1` for the whole guard family, every
   frame** — 120, 130, 131, 132, 140 and 150–155. `+0xE68` is the "this player is
   blocking" latch: hitresolve tests it at `0x46D35F` and, when set, computes the
   incoming hit's damage from the **guard.damage pair** (`hitdef+0x28`/`+0x3C`,
   i.e. active `+0x11D8`/`+0x11EC` via `sub_46E760`) instead of the hit damage.
   It is set on block application (`0x46E906`), cleared on a clean hit
   application (`0x46E4D0`) and on a full player reset (`0x46A9AB`).
   Behavioural consequence: **a blocker that is inside the shake/knock-back
   states 150–155 keeps blocking**, without holding back, for as long as the
   file keeps it there.

### 3.2 The one transition the engine does own: state 140

`0x46B8FD` (states 130–132's sibling for 140) reads the current anim's element
count and duration through `+0x1418`, and when `time-in-state >= duration`
(`0x46B926`, `jl` skip) it does:

- statetype S → inline change to state 0, `time-in-state = 0`,
  `prevstateno = old stateno` (`0x46B962`);
- statetype C → `ChangeState(11)` (`0x46B948`);
- statetype A → `ChangeState(51)` (`0x46B95B`).

This is exactly the controller the shipped `common1.cns` leaves commented out:

```
;[State 140, 5] ;Implemented within engine
;type = ChangeState
;trigger1 = AnimTime = 0
;value = (statetype = C)*11 + (statetype = A)*51
```

A direct, verifiable match between the file and the binary.

### 3.3 There is no engine write of 151/153/155 or 130/131

A search for the immediates `0x97`, `0x99`, `0x9B` and `0x82`/`0x83` shows the
only `movl $0x97, %ecx` instances (`0x410133`, `0x4101D4`, `0x417822`,
`0x41B224`) are `rep movsl` block copies (a 0x97-dword memcpy), not statenos, and
there is no immediate `0x99`, `0x9B`, `0x82` or `0x83` store at all. The engine's
only guard-family stateno stores are the three in §1.1:
`movl $0x9a/%0x98/%0x96, 0xc4c(...)`.

### 3.4 The chain really is `common1.cns`'s

Read from `local_mugen/extracted/mugen/data/common1.cns` (read-only; not copied
into the repo):

- `[Statedef 120]` guard start → `ChangeState 130 + (C) + (A)*2` on
  `AnimTime = 0`; `→ 140` when `command != "holdback"` or `!inguarddist`.
- `[Statedef 130]` / `[Statedef 131]` / `[Statedef 132]` — the three guard holds;
  each changes to its sibling on a `holddown` change and to `140` on release.
  (The changelog dates the move of exactly this release handling into the file:
  see §3.5.)
- `[Statedef 140]` guard end, `ctrl = 1`, engine-implemented exit (§3.2).
- `[Statedef 150]` stand guard hit (shaking), `movetype = H`, `velset = 0,0`,
  `ChangeAnim 150`, and `ChangeState 151 + 2*(command = "holddown")` on
  `HitShakeOver` — i.e. **151 or 153 depending on whether down is held at the
  tick the shake ends**.
- `[Statedef 151]` stand guard hit (knocked back): `HitVelSet x` at `Time = 0`;
  `VelSet x = 0` at `Time = GetHitVar(slidetime)` or `HitOver`;
  `CtrlSet 1` at `Time = GetHitVar(ctrltime)`; `ChangeState 130 ctrl = 1` on
  `HitOver`.
- `[Statedef 152]` crouch shaking → `151 + 2*(holddown)` on `HitShakeOver`;
  `[Statedef 153]` mirrors 151 and ends at 131.
- `[Statedef 154]` air shaking → 155 on `HitShakeOver`; `[Statedef 155]` air
  knocked away → 130 when it lands with `holdback` and `inguarddist`, else 52.

So every arrow after the engine's initial `150/152/154` is the file's, driven by
the two engine triggers and the two engine getters answered in §2. The engine's
contribution is the clocks, the state entry, the trigger expressions, and the
`+0xE68` flag.

### 3.5 The distribution's own changelog agrees

`local_mugen/extracted/mugen/docs/history.html`, the **14 April 2002** entry,
under "*** Changed from previous version ***":

> common1.cns: states 120,130,131,132: added guard-release handling (mugen exe
> no longer does this internally)

This is a second, independent statement that the exe stopped ending the guard
states itself and that `common1.cns` does it instead — which is exactly what the
1.0 binary's vestigial `0x46B8AD`/`0x46B8EC` slots for 120/130/131/132 show: the
hooks remain, the release logic is gone.

### 3.6 What is not established (Q3)

- Whether a character that ships **no** common1.cns falls back to the shipped
  one, or to engine defaults, was not traced here; the corpus question is
  separate from the binary question.
- Nothing was found that would end 150–155 from the engine, but I did not prove
  the negative for computed (non-immediate) stateno writes; the search was for
  the four statenos as immediates plus a read of the two dispatch tables. A
  computed `ChangeState(x)` with `x` in 130–155 would not have been caught.
- The `0x46B895` handler for state 40 (`+0x21C = +0x10C`) was not identified; it
  is recorded here only because it is in the same table.

## 4. Second source: what the shipped 1.0 documentation says

Quoted from `local_mugen/extracted/mugen/docs/` (read-only; not copied into the
repo).

`trigger.html`, `GetHitVar`:

> **hitshaketime**: Returns time player is "frozen" during the hit. This number
> counts down by 1 for each game tick, and stops when it reaches zero. (int)
>
> **hittime**: Returns time before player regains control and returns to an idle
> state after being hit. This counts down by 1 per game tick, as long as
> hitshaketime (see above) is greater than 0. It stops counting down when the
> value reaches -1. (int)
>
> "GetHitVar(hittime) < 0" is equivalent to the HitOver trigger.
>
> **slidetime**: Returns time that player slides backwards (on the ground) after
> the hit. (int)
>
> **ctrltime**: Returns time before player regains control after guarding the
> hit. (int)

`trigger.html`, `HitOver` / `HitShakeOver`:

> **HitOver**: If the player is in a gethit state, returns 1 when the hittime has
> expired, and 0 otherwise.
>
> **HitShakeOver**: If the player is in a gethit state, returns 1 if the hit
> shake (the period when he is shaking in place) has ended, and 0 otherwise.

`sctrls.html`, the HitDef controller:

> **pausetime = p1_pausetime, p2_shaketime (int, int)**: This is the time that
> each player will pause on the hit. p1_pausetime is the time to freeze P1,
> measured in game-ticks. p2_pausetime is the time to make P2 shake before
> recoiling from the hit. Defaults to 0,0 if omitted.
>
> **guard.pausetime = p1_pausetime, p2_shaketime (int, int)**: Similar to the
> "pausetime" parameter, these are the times to pause each player if the hit was
> guarded. Defaults to the same values as the "pausetime" parameter if omitted.
>
> **damage = hit_damage, guard_damage (int, int)**: … The optional guard_damage
> parameter is the damage taken by P2 if the hit is guarded.

Three things this settles or corroborates:

1. The HitDef's second `pausetime` value is literally named `p2_shaketime`: **the
   shake clock is the HitDef's own second pausetime value**, which is why the
   active-block field the guard path reads (`+0x180`) and the one the clean-hit
   path reads (`+0x50`) are the same value from two different parameter families
   (`guard.pausetime` defaults to `pausetime`, per the second quote). It is not a
   pause applied to P2: the engine zeroes the blocker's own hitpause pair when it
   applies a block (`0x46E92E`) and a clean hit (`0x46E51E`), and pauses only P1
   (`+0xE74 ← hitdef+0x54` at `0x46D80F`, and `← hitdef+0x184` for a guard at
   `0x46D9FE`). "P2 shakes" — and §2.4 shows the shake is a drawn-position
   jitter.
2. `hitshaketime` counting "down by 1 for each game tick, and stops when it
   reaches zero" and `hittime` "stop[ping] when the value reaches -1" both match
   the binary exactly (`0x46B732` with an `if > 0` guard; `0x46C1B1` with an
   `if >= 0` guard against `-1`).
3. **One clause disagrees.** "This counts down by 1 per game tick, as long as
   hitshaketime (see above) is greater than 0" describes the opposite of
   `0x46C1A9/0x46C1AF`, which skip the hittime decrement when
   `+0x1090 != 0`. The binary is unambiguous and is also the only reading
   consistent with the same entry's next sentence, and with the fact that 150
   (the shake state) and 151 (the knock-back state) are separate states that the
   shipped file leaves on different triggers. Recorded as a doc defect, not as a
   contradiction of §1.4.

`history.html` also records the 1.0 change to P1 pausetime accounting
(entry "Version 1.0 RC1", 22 Sep 2009):

> CNS: Fixed: HitDef P1 pausetimes expired 1 tick sooner than specified. When
> loading 2002.04.14 and older characters, this fix will be compensated for by
> subtracting 1 from the P1 pausetime after expression evaluation.
>
> …
> To update a character from 2002.04.14 to 1.0, add 1 to the value of the P1
> parameter of all HitDef pausetimes.

Relevant only in that any engine implementing `mugenversion`-conditional
behaviour has a second, P1-side off-by-one to honour; it does not touch the four
victim clocks.

## 5. What this means for the engine

For a from-scratch engine whose characters ship no guard states of their own,
hold the blocker for **shake + hittime, counted sequentially**:

1. On a blocked hit, seed `shakeTicks_` from `guard.pausetime`'s second value
   (`guard.p2shaketime`) and `stunTicks_`-equivalent from `guard.hittime`, and set
   the file-visible `hittime`/`slidetime`/`ctrltime` from `guard.hittime` /
   `guard.slidetime` / `guard.ctrltime` (air: `airguard.ctrltime`). Leave the air
   block's `hittime`/`slidetime` **untouched**, as the reference does
   (`0x46E995` branch writes only `+0x109C`).
2. Tick `shakeTicks_` down 1 per frame while it is `> 0`; freeze the hittime
   counter while `shakeTicks_ != 0`; then tick it down 1 per frame until it
   passes zero, and treat "passed zero" as `hitover`. Total held time is the sum,
   not the max. Never tick during a hitpause, and never let the blocker itself be
   hitpaused while its shake is running.
3. `slidetime` and `ctrltime` are **not counters**. Expose them as the constants
   they are: `GetHitVar(slidetime)`/`GetHitVar(ctrltime)` must return the same
   number every frame until the next hit, because the shipped file compares them
   against `Time` (which restarts at 0 when the knock-back state is entered), not
   against a descending clock. An engine that decrements them will be
   `shake` ticks short on both the slide stop and the control return.
4. Enter states 150/152/154 and nothing else. If the character ships its own
   guard states, the engine must not end 150–155 for it — the file's
   `HitShakeOver`/`HitOver`/`GetHitVar(...)` triggers do that, and the engine's
   job is only to keep those four answers correct and to keep the built-in chain
   out of the way. This is what pass 18's `builtinExitsOnShake` already does for
   the shake end; the second half is that the built-in knock-back must not
   outlive or pre-empt an authored one.
5. Keep the block flag semantics: while a blocker is inside the guard family
   (120, 130, 131, 132, 140, 150–155), treat the next incoming hit as guarded
   even if the player is no longer holding back. The reference gets this from a
   per-frame re-assertion of `+0xE68` keyed on the *state number* (`0x46B8AD` for
   120 and 150–155, `0x46B8EC` for 130–132, `0x46B8FD` for 140), not from the
   input. This is what makes a guard chain hold for the full `guard.hittime`
   without the player holding back the whole time.
6. If the engine implements the built-in guard chain itself (rather than running
   a shipped `common1.cns`), then for a character with no guard states the
   observable behaviour to match is: hold for `guard.pausetime[1] + guard.hittime`
   ticks, hand over to the guard-hold, zero the outward velocity `guard.slidetime`
   ticks into the knock-back, and return control `guard.ctrltime` ticks into it.
   Those two are measured from *entry into the knock-back state*, not from the
   block — a distinction worth having a test for, since it is the one place where
   "the clock the file reads" and "the clock the engine ticks" disagree.

## 6. Evidence index

| claim | where |
|---|---|
| function bounds | `/tmp/mugen_peek/ghidra_funcs_46_49.txt` |
| guard stateno + four clock writes | disasm `0x46E880-0x46E9FD` |
| only call site of `0x46E7C0` | disasm `0x46DA1D`; `decompiled/angr_p9_hitresolve.c:496` |
| attacker pause writes | disasm `0x46D80F` (+0x54), `0x46D9FE` (+0x184) |
| clean-hit clock write | disasm `0x46E4D0`, `0x46E4F5` |
| shake decrement | disasm `0x46B728-0x46B733` |
| hittime decrement | disasm `0x46C1A1-0x46C1B1` |
| hitpause early-out / own-pause clear | disasm `0x46B6C5`, `0x46B489`, `0x46B45F-0x46B46D` |
| reset branch clearing `+0x1090` | disasm `0x46B7EC` |
| trigger opcode names | `decompiled/angr_00482b00_expression_eval.c:949-969` |
| trigger runtime (`hitshakeover`/`hitover`) | disasm `0x487592`, `0x4875A4` |
| GetHitVar getters | disasm `0x48785D`..`0x48787E` |
| state table | disasm `0x46B882-0x46B8B5`, tables at `0x46C404`/`0x46C418` |
| state 140 handler | disasm `0x46B8FD-0x46B986` |
| `+0xE68` block flag | writes `0x46B8AD/B8EF/B906/E906/E4D0/A9AB`; test `0x46D35F` |
| shake jitter | disasm `0x470970`, `0x470BB2` |
| shipped chain | `local_mugen/extracted/mugen/data/common1.cns:390-803` |
| documentation | `local_mugen/extracted/mugen/docs/{trigger,sctrls,history}.html` |

# Projectile controller RE — parameter types, defaults, storage, and consumers

Reference: `mugen.exe` (MUGEN 1.0, PE32 i386, MSVC 9.0, image base `0x400000`),
read from `/tmp/mugen_peek/mugen/mugen.exe` and its objdump text
`/tmp/mugen_peek/mugen_disasm.txt`. All addresses below are VAs unless marked
otherwise.

Two sources are used: the binary, and the MUGEN 1.0 distribution's own shipped
documentation, read read-only from `local_mugen/extracted/mugen/docs/`
(`sctrls.html`, `trigger.html`, `tutorial3.html`; never copied into the repo).
Where they disagree, the binary wins.

Scope: the `Projectile` controller's parameters, plus the two trigger opcodes
that inspect live projectiles (`NumProj`, `NumProjID`). HitDef inheritance into
the projectile's own hit record is only covered where it decides a projectile
parameter; the full HitDef block is documented separately by
`decompiled/INDEX.txt` (`0x47F380`).

## 0. Answers in one screen

| question | answer | confidence |
|---|---|---|
| What is `projscale`? | A **float pair** (`x_scale, y_scale`), not a boolean. Default **1,1**. The spawn path reads two floats and stores them as the projectile's draw scales at object **+0x50 (x)** and **+0x54 (y)**. | HIGH for type/default/storage |
| What is `proj.doscale`? | An **int** in the character-constants record at **+0x308**; read with the int reader at `0x433730` (parser xref `0x467A3A–0x467A48`) and copied into the hit/projectile active block at **+0x148** by `0x47F380` (read at `0x47FF30`, `ftol` via `0x499690`, store `0x148(%ebx)`). Every value in the local corpus is `0`. The **exact engine gate that combines it with `projscale` was not located**; only the storage and copy are proven. | HIGH for storage/default; LOW/UNRESOLVED for the gate |
| What is `projremovetime`? | Default **-1** (never removed by age). The per-projectile update at `0x472DB0` compares the projectile's age **+0x24** against **+0x3C** and only enters the removal state when the parameter is not `-1`. | HIGH |
| What is `projremove`? | Boolean, default **1**. Parsed/stored at definition +0x28 and consumed at object **+0x38**. The hit path at `0x46FCF3/0x46FDB2` tests `0x38(%eax)` before forcing the hit/removal anim state. | HIGH parse/default; MEDIUM hit-path consumption |
| What is `projmisstime`? | Default **0**. Per the docs it is the multi-hit re-hit delay: number of ticks after each hit before the projectile may hit again. Live object field **+0x30**; a copy of it reaches a hit/player block at **+0x2D0**, but the full consumer was not traced. | HIGH default/storage; MEDIUM consumer |
| What is `projpriority`? | Default **1**. Per the docs, equal priorities cancel; a higher priority cancels the lower and is decremented by 1. Live object field **+0x0C**. The projectile-vs-projectile collision implementation was **not located**. | HIGH default; LOW/UNRESOLVED collision |
| What is `projshadow`? | Three ints `R,G,B`, default **0,0,0** (no shadow). `R == -1` selects the stage shadow color. The definition record's shadow triple is zeroed; the live draw field was not pinned. | HIGH parse/default; consumer UNRESOLVED |
| What are the four anim parameters? | `projanim` default **0**; `projhitanim` default **-1** (no change); `projremanim` defaults to `projhitanim`; `projcancelanim` defaults to `projremanim`. Stored at object **+0x10** (hit), **+0x14** (removal), **+0x18** (cancel); `projanim` is the flying animation. | HIGH |
| What does `NumProj` count? | Entries in the projectile slot manager whose live object **state at +0x4 == 1** (flying). Code at `0x4733E0`, compare `0x473401`. | HIGH |
| What does `NumProjID` count? | Entries with **state at +0x4 == 1** *and* object **+0x0 == the requested ID** (first argument). Code at `0x473440`, compares `0x473461–0x47346F`. | HIGH |
| Does a negative `NumProjID` ID mean "count all"? | The docs say a negative ID is treated as 0. **No clamp was found in the trigger implementation**; the comparison is a plain equality against the argument. Docs-only claim. | LOW/UNRESOLVED engine side |

## 1. Where the controller lives

Function boundaries used throughout (from the objdump):

| function | range | size | role |
|---|---|---|---|
| parser | `0x480750` | `0x9FB` | reads the controller parameters into a definition record |
| spawn | `0x481150` | `0xBBE` | evaluates the definition record, maps fields to a live projectile object, `hitdef_fill` call at `0x48178C` |
| per-tick update | `0x472DB0` | `0x440` | ages the projectile, applies velocity/acceleration, handles removal |
| free / release | `0x472D70` | — | frees a slot |
| draw | `0x4734A0` | — | draws the projectile; reads the scale fields |
| `NumProj` | `0x4733E0` | — | trigger |
| `NumProjID` | `0x473440` | — | trigger |

The projectile manager is a sparse slot array. Its fields, as recovered while
tracing the update and free paths: **+0x4** slot stride, **+0x18** object array
base, **+0x1C** occupancy bitmap base, **+0x24** first live index, **+0x28**
live count. Live objects are referred to below by their own offsets.

The definition record used by the parser and spawn is **not** the live-object
layout; spawn evaluates the parsed definition and writes the live object. Some
parameter defaults live in the definition record, others are materialized only
at spawn.

## 2. Definition-record defaults

The parser initializes its record with an explicit block of stores across
`0x480750–0x480B7E`. The defaults that matter here:

| parameter | definition offset | default | evidence |
|---|---|---|---|
| `projremove` | +0x28 | **1** | initializer; spawn reads +0x28 |
| `projremovetime` | +0x34 | **-1** | initializer; spawn reads +0x34 |
| `projmisstime` | +0x40 | **0** | initializer |
| `projhits` | +0x7C | **1** | initializer |
| `projpriority` | +0x88 | **1** | initializer |
| `projhitanim` | +0xA0 | **-1** | initializer |
| `projremanim` | +0xAC | **-1**, then falls back to `projhitanim` | initializer + spawn fallback |
| `projcancelanim` | +0xB8 | **-1**, then falls back to `projremanim` | initializer + spawn fallback |
| `projscale` x | +0x154 | **1.0** | initializer |
| `projscale` y | +0x160 | **1.0** | initializer |
| `velocity`, `remvelocity`, `accel` | +0xF4…+0x148 | **0.0** | initializer |
| `velmul` | — | **1.0** | initializer |
| `projsprpriority` | +0xE8 | **3** | initializer |
| `projshadow` RGB | — | **0,0,0** | record triple zeroed; `-1` sentinel is docs + parser |
| bounds (`projedgebound`, `projstagebound`, `projheightbound`) | — | computed from engine constants (docs: 40/80/160 families) | parser computes, not literal defaults |

Parser xrefs for the parameter strings (from `push $0x…` sites):

```
projid          0x4A4838      projanim        0x4A4840
projhitanim     0x4A484C      projremanim     0x4A4858
projcancelanim  0x4A4864      projsprpriority 0x4A4874
projedgebound   0x4A4884      projstagebound  0x4A4894
projheightbound 0x4A48A4      remvelocity     0x4A48B4
projscale       0x4A48C0      projhits        0x4A48CC
projpriority    0x4A48DC      projremove      0x4A48EC
projremovetime  0x4A48F8      projmisstime    0x4A4908
projshadow      0x4A4918      velocity        0x49C79C
velmul          0x4A3110      accel           0x4A3EAC
```

`proj.doscale` is a character-constants parameter, not a Projectile
parameter: its string is at `0x4A27C0`, xrefs `0x467A3A–0x467A48`.

## 3. Spawn: definition record → live object

Verified mappings in `0x481150` (offsets are the live projectile object's):

| parameter | live field | spawn evidence |
|---|---|---|
| `projid` | +0x00 | stored before the anim/ID resolution block |
| `projhits` | +0x1C | `0x4813AB`-region: def+0x7C → obj+0x1C; negative is clamped to **1** at `0x48139B` |
| `projmisstime` | +0x30 | def+0x40 → obj+0x30; negative is replaced with **2** at `0x481349` (see §4.5) |
| `projpriority` | +0x0C | def+0x88 → obj+0x0C; no validation branch |
| `projhitanim` | +0x10 | def+0xA0 → obj+0x10; values `< -1` are replaced with **-1** at `0x481407` (the `-1` store lands in the same obj+0x10 slot after accounting for the six pushes) |
| `projremanim` | +0x14 | def+0xAC → obj+0x14; fallback to `projhitanim` when omitted |
| `projcancelanim` | +0x18 | def+0xB8 → obj+0x18; fallback to `projremanim` when omitted |
| `projscale` | +0x50 / +0x54 | two floats read at spawn; draw path proves the fields |
| `projremove` | +0x38 | definition +0x28; consumed by the hit path (§4.4) |
| `projremovetime` | +0x3C | definition +0x34; consumed by update (§4.3) |

Velocity-family reads in spawn are at `0x4814B3` onwards: `leal 0xF4(%ebp)` /
`0x100(%ebp)` / `0x10C(%ebp)` / `0x118(%ebp)` (velocity x/y, accel x/y),
`0x124(%ebp)` / `0x130(%ebp)` / `0x13C(%ebp)` / `0x148(%ebp)` (remvelocity and
the second accel pair), and `0x154(%ebp)` / `0x160(%ebp)` (`projscale` x/y).
Each raw value is read with `0x439330`, multiplied by the player's scale field
`+0xA4` (`flds 0xA4(%eax)`), and stored into the live object. `projscale` is
**not** multiplied by `+0xA4`; it is passed through unchanged (the `0x4816FF`
and `0x48170E` reads are stored without the `fmuls`).

The projectile's own hit record is filled by the standard HitDef filler:
`0x481779–0x48178C` pushes `def+0x184`, a stack scratch block, and the player,
then calls `0x47F380`. That is why `proj.doscale`'s active-block copy at +0x148
is part of this path.

## 4. Per-parameter evidence

### 4.1 `projscale` — float pair, default 1,1

Docs (`sctrls.html`):

> `projscale = x_scale, y_scale (float, float)` — Specifies the scale factor
> of the projectile. The final scale of the projectile is affected by both this
> parameter and the `proj.doscale` parameter in the `[Size]` group of p1's
> constants file. Defaults to 1,1 (normal size) if omitted.

Binary: the initializer writes `1.0` to definition +0x154 and +0x160; spawn
reads those two floats (`0x4816FF`, `0x48170E`) and stores them as a pair.
The draw function reads the pair directly and multiplies it by an additional
factor:

```
0x4734CF  flds 0x54(%esi)     ; scale y
0x4734D4  fmuls 0x4C(%esi)
0x4734E2  flds 0x50(%esi)     ; scale x
0x4734E5  fmuls 0x4C(%esi)
0x473509  flds 0x5C(%esi)
0x473511  flds 0x58(%esi)
```

So +0x50/+0x54 are the final draw-scale x/y pair; +0x4C is an additional
multiplier and +0x58/+0x5C are further draw parameters whose exact mapping is
listed as an unknown in §5. There is no boolean semantics anywhere: the parser
reads two floats, and the only stores are the pair.

Whether the final scale also scales the projectile's Clsn boxes is **not
proven**. Collision code was not located; the draw path only proves the sprite
scale fields.

### 4.2 `proj.doscale` — int at character-constants +0x308

Docs (`sctrls.html`, `[Size]` and `tutorial3.html`):

> `proj.doscale = 0` — You can leave the next 3 unchanged.

The docs describe it only through `projscale`: the final scale is affected by
both `projscale` and `proj.doscale`. `trigger.html` exposes it as
`size.proj.doscale` returning an **int**.

Binary:

- Parser/reader site `0x467A3A–0x467A48`:
  `leal 0x308(%ebx), %edx` then `call 0x433730` (the integer parameter reader).
  It is an int, stored in the character-constants record at +0x308.
- `0x47F380` (hitdef/active-block filler) reads the same field:
  `0x47FF30` `leal 0x308(...)` (or the equivalent load in that block),
  `call 0x499690` (`ftol`), and stores the result as
  `movl %eax, 0x148(%ebx)`. The active block therefore carries a **0/1 int
  converted to a 32-bit value** at +0x148.

Local corpus: every character constants file in `local_chars/` that declares
`proj.doscale` declares **0**. The spawn path reads the player's constants
record but the exact branch that decides between "use `projscale`" and "ignore
it" was not identified. What is proven is storage, type, default in the
corpus, and the copy into the active block; the gate is **UNRESOLVED**.

### 4.3 `projremovetime` — default -1, per-projectile age comparison

Docs (`sctrls.html`):

> `projremovetime = remove_time (int)` — Specifies the number of ticks after
> which the projectile should be removed from the screen. If -1, the projectile
> will not be removed. Defaults to -1.

Binary, update `0x472DB0`:

```
0x472FED  movl 0x3C(%esi), %eax        ; projremovetime
0x472FF0  cmpl %eax, 0x24(%esi)        ; age vs removetime
0x472FF3  jl   <keep>                  ; age < removetime -> keep
0x472FF8  jne  0x47302D                ; age != removetime...
```

The `jne` is the `-1` guard: when the stored limit is `-1`, the compare never
matches the age and the projectile is never removed by time. The removal
branch sets object state **2** at `0x47302D`, which selects the removal
animation (see §4.8). Age is advanced once per update:

```
0x4731B1  movl $1, %eax
0x4731B4  addl %eax, 0x20(%esi)        ; another counter
0x4731B7  addl %eax, 0x24(%esi)        ; age ticks
```

The comparison is per projectile and uses the object's own +0x24, not a global
clock.

### 4.4 `projremove` — boolean, default 1; hit path consumes +0x38

Docs (`sctrls.html`):

> `projremove = remove_flag (boolean)` — Set to a non-zero value to have the
> projectile be removed after it hits, or to 0 to disable this behavior.
> Defaults to 1.

Binary: definition +0x28 default **1**; spawn writes the live field **+0x38**.
Two hit-path fragments were found:

```
0x46FCF7  movl $1, 0x2C8(%ebx)         ; mark a hit/contact
0x46FDA9  decl 0x1C(%eax)              ; projhits--
0x46FDC9  cmpl $0, 0x38(%eax)          ; projremove == 0 ?
0x46FDCE  movl $4, 0x4(%eax)           ; state = 4 (hit/removal anim)
0x46FDD5  movl $0, 0x24(%eax)          ; reset age
```

and a second path:

```
0x46F5B2  decl 0x1C(%esi)
0x46F5C1  movl $2, 0x4(%esi)           ; state = 2 (removal anim)
0x46F5D1  movl 0xE8(%esi), %ecx
0x46F5D5  movl %ecx, 0x28(%esi)
```

The `cmpl $0, 0x38(%eax)` at `0x46FDC9` is the consumption of `projremove`:
a zero value bypasses the forced state write. The second path shows the
removal-state transition without the guard (a different removal cause), so the
exact branch shape around the first path is only partially reconstructed.
Confidence is therefore HIGH for parse/default/storage and MEDIUM for the
precise hit-path semantics.

### 4.5 `projmisstime` — default 0; multi-hit re-hit delay

Docs (`sctrls.html`):

> `projmisstime = miss_time (int)` — If the projectile is configured for
> multiple hits, miss_time specifies the number of ticks after each hit before
> the projectile can hit again. Defaults to 0.

Binary: definition +0x40, default **0**; spawn stores it at live **+0x30**.
Validation branch:

```
0x481316  cmpl %edi, %eax              ; raw projmisstime vs 0
0x481318  movl %eax, 0x50(%esp)        ; obj+0x30 <- raw value
0x48131C  jge  0x481359                ; >= 0 accepted
...
0x481349  movl $2, 0x68(%esp)          ; negative -> 2
```

A negative `projmisstime` is logged as a parameter error and replaced with
**2**, not 0 and not 1. This is a binary fact; the rationale is not documented.
Treat it as a reference quirk.

Consumer: a copy of the +0x30 value appears to reach a hit/player block at
**+0x2D0** in the hit bookkeeping region, but the comparison that decides
"can this projectile hit again yet" was not isolated. The docs' timing
semantics (ticks after a hit before re-hit) are accepted, but the engine
implementation is only partially located.

### 4.6 `projpriority` — default 1; collision implementation not located

Docs (`sctrls.html`):

> `projpriority = proj_priority (int)` — Specifies the projectile priority.
> If the projectile collides with another projectile of equal priority, they
> will cancel. If it collides with another of lower priority, it will cancel
> the lower-priority projectile, and the higher-priority one will have its
> priority decreased by 1. Defaults to 1.

Binary: definition +0x88 default **1**, spawn writes live **+0x0C**. No
projectile-vs-projectile collision routine was found that compares +0x0C. The
parameter's storage and default are proven; the equal-cancel / lower-decrement
behavior is docs-only in this pass. This is the largest behavioral unknown
among the numeric parameters.

### 4.7 `projshadow` — RGB triple, default 0,0,0, -1 sentinel

Docs (`sctrls.html`):

> `projshadow = shad_r, shad_g, shad_b (int, int, int)` — Specifies the R, G,
> and B components of the projectile's shadow. These components should be
> integers between 0 and 255, inclusive. If shad_r evaluates to -1, then the
> stage's shadow color will be used. The higher a component value, the less of
> that color is displayed in the shadow. So a perfectly black shadow is
> 255,255,255. Defaults to 0,0,0 (no shadow).

Binary: the parser string at `0x4A4918` is read with three int reads; the
definition record's shadow triple is initialized to zero. The `-1` sentinel is
documented and is consistent with the "use stage color" branch pattern used by
other shadow parameters, but the live draw consumer and the sentinel branch
were not pinned in this pass. HIGH for parse/default; consumer UNRESOLVED.

### 4.8 The anim family

Docs (`sctrls.html`):

> `projanim = anim_no (int)` — ... Defaults to 0.
> `projhitanim = anim_no (int)` — ... Defaults to -1 (no change in animation)
> if omitted.
> `projremanim = anim_no (int)` — ... If omitted, projhitanim is used instead.
> `projcancelanim = anim_no (int)` — ... If omitted, projremanim is used
> instead.

Binary:

- `projanim` default **0** (flying animation).
- `projhitanim` default **-1**; spawn also clamps values `< -1` to `-1`.
- `projremanim` and `projcancelanim` are copied from their fallback source
  when omitted (spawn fallback logic).
- Live object anim selection by state: `0x4730DB–0x4730EF` chooses
  **+0x14** for state 2 (removal), **+0x10** for state 4 (hit), **+0x18** for
  state 3 (cancel). An anim value of `-1` frees the slot at `0x4730B9`.
- State writes: `0x47302D` state 2, `0x46FDCE` state 4, and the cancel path
  uses state 3.

So the state machine is: 1 = flying, 2 = removal (`projremanim`), 3 = cancel
(`projcancelanim`), 4 = hit (`projhitanim`). The fallback chain is fully
consistent with the docs.

### 4.9 `NumProj`

`0x4733E0` walks the sparse array and counts entries whose state is **1**:

```
0x473401  cmpl $1, 0x4(%ecx)
0x473405  jne  <skip>
...
0x47340A  incl %eax
```

It does not count projectiles that are in the removal/hit/cancel states. This
matches the trigger's meaning ("number of projectiles currently active"), where
active means still flying.

### 4.10 `NumProjID`

`0x473440` applies the same state filter and additionally compares the
object's ID at +0x0 with the first argument:

```
0x473461  cmpl $1, 0x4(%ecx)
0x473465  jne  <skip>
0x473469  movl 0x10(%esp), %edx        ; first argument (ID)
0x47346D  cmpl %edx, (%ecx)            ; obj+0x0 == ID
0x47346F  jne  <skip>
0x473471  incl %eax
```

The ID is the projectile's own +0x0 field, written at spawn from `projid`.
The docs' statement that a negative ID is treated as 0 is **not** implemented
as a clamp in this routine; the comparison is a plain equality. If a negative
ID is to match `projid = 0`, that clamp must happen in the trigger compiler or
argument evaluation before this call, and that site was not found. Confidence
LOW/UNRESOLVED on the engine side; the docs are the only support for the
negative-ID behavior.

## 5. Explicit unknowns / UNRESOLVED

1. **`proj.doscale` gate site.** Storage at constants +0x308 and the copy to
   active +0x148 are proven. The branch that combines it with `projscale`
   (and whether the value semantics are "use scale" vs "ignore scale") is not
   located.
2. **Player +0x7C / +0x80 meaning.** Spawn reads these two floats when the
   player's `+0x98` field is set, multiplies `projscale` by them, and uses the
   result to scale the velocity/accel pairs. Their semantic names and the
   controller that writes them are unknown.
3. **Does `projscale` scale Clsn as well as the sprite?** The draw path proves
   +0x50/+0x54 are sprite draw scales. No collision code was found that reads
   them; unresolved.
4. **Projectile-vs-projectile priority collision.** No routine comparing
   object +0x0C was located. Equal-cancel and lower-decrement are docs-only.
5. **Consumers of `projremove`, `projmisstime`, `projhits`.** Partial hit-path
   fragments exist (see §4.4/§4.5), but the full "can hit again" and
   "remove after hit" decisions are not reconstructed end-to-end.
6. **Object +0x28 semantics.** It is written from player +0xE8 at `0x46F5D1`
   and not initialized by the spawn path in the chunk examined. Whether it is a
   pause/hitpause counter, a link index, or a copy of another player field is
   unknown.
7. **Exact +0x50/+0x54 draw-scale mapping.** They are the final scale x/y, but
   the multiplier at +0x4C and the fields +0x58/+0x5C read by the same draw
   function are not named.
8. **Negative-ID clamp for `NumProjID`.** Docs say negative → 0; no clamp in
   `0x473440`.
9. **`projshadow` live draw field and `-1` sentinel branch.** Parse/default are
   clear; the draw-side consumer is not pinned.
10. **Negative `projmisstime` → 2.** Observed at `0x481349`; why 2 is not
    documented.

## 6. Engine checklist, ordered by local-corpus frequency

Counts below are raw hits in `local_chars/`, shown as `hits/files`. They are a
priority guide for implementation and test coverage, not a claim about global
MUGEN usage.

| rank | parameter / trigger | hits/files | engine status |
|---|---|---|---|
| 1 | `NumProj` (trigger) | 199/48 | implemented semantics proven (§4.9) |
| 2 | `projanim` | 107/10 | default and storage proven |
| 3 | `projremovetime` | 93/9 | update comparison proven (§4.3) |
| 4 | `projpriority` | 86/8 | storage proven; collision unlocated |
| 5 | `projid` | 79/9 | storage proven; ID match in `NumProjID` proven |
| 6 | `projhitanim` | 70/7 | default/storage/clamp proven |
| 7 | `projshadow` | 63/6 | parse/default proven; draw consumer unresolved |
| 8 | `projhits` | 59/6 | default/clamp proven; hit decrement seen |
| 9 | `projremove` | 53/4 | default/storage proven; partial hit consumer |
| 10 | `projedgebound` | 52/8 | parsed; bound computation not detailed |
| 11 | `proj.doscale` | 50/49 | storage + active copy proven; gate unresolved |
| 12 | `projheightbound` | 50/7 | parsed; bound computation not detailed |
| 13 | `projremanim` | 41/7 | fallback chain proven |
| 14 | `projstagebound` | 38/5 | parsed; bound computation not detailed |
| 15 | `projsprpriority` | 31/3 | default 3 proven; draw ordering not traced |
| 16 | `projmisstime` | 30/5 | default/storage proven; full consumer unresolved |
| 17 | `NumProjID` (trigger) | 28/6 | state+ID filter proven (§4.10) |
| 18 | `projscale` | 24/2 | float pair/default/storage proven |
| 19 | `projcancelanim` | 23/1 | fallback chain proven |
| 20 | `ProjHit` / `ProjHitTime` (triggers) | — | not covered by this pass; see `trigger.html` |
| 21 | `NumProj` / `NumProjID` in state controllers | — | same opcodes as above; compiler dispatch not re-derived here |

Implementation order implied by the table: `NumProj` semantics and the anim
state machine first (they dominate), then removetime, then priority collision,
then the anim fallback variants, then the remaining bounds and shadow.

## 7. Corpus caveat

Every count in §6 and the "all local values are 0" claim for `proj.doscale`
were computed only over the local `local_chars/` corpus on this machine. That
corpus is a convenience sample, not a survey of MUGEN characters in general.
Do not cite these counts as "how often MUGEN characters use X"; they only say
which parameters this project's test corpus exercises, and therefore which
paths are worth implementing and testing first.

# INERT-CONTROLLERS-SURVEY

Survey of eight controllers that `StateMachine::runController`
(`app/src/main/cpp/engine/statemachine.cpp`) recognises but does nothing with:
`width`, `makedust`, `envadjust`, `explodbindtime`, `makeanim`, `makepalette`,
`zoom`, `removemappedexplods`.

All corpus numbers below are **measured** against `local_chars/` with the
section method (see "Method"). All doc quotes are **verbatim** from
`local_mugen/extracted/mugen/docs/sctrls.html`, which is explicitly *MUGEN 1.0*
(the file's own header says "Documentation for version 1.0 (2009)" / "Updated
13 September 2010"). Claims about MUGEN behaviour that I could not read in that
file are marked **inferred** or **not documented**.

---

## Summary — what is worth implementing, in order

`width` is by far the most-used of the eight (141 sections across 26 files and
14 packs) and is the only one whose effect is *gameplay*, not decoration: it
changes the push/collision width for a tick, which decides where a fighter stops
against the edge and against the other fighter. It should be first. `makedust`
is second by usage (42 sections, 18 files, 12 packs) but is purely visual and the
1.0 docs mark it deprecated in favour of `Explod`; it is cheap to emulate but
matters only to the picture. `explodbindtime` has **zero** corpus uses and is
**not documented** in the 1.0 file, but the engine already models explod bind
time end-to-end (`Explod::bindTime`, the carry loop, and `ModifyExplod`'s
`bindtime` setter), so wiring the controller is nearly free — it is the cheapest
of the remaining six. The other five — `envadjust`, `makeanim`, `makepalette`,
`zoom`, `removemappedexplods` — have **zero** corpus uses (measured; `makeanim`
matches are all the unrelated `GameMakeAnim`, and `zoom` appears only commented
out) and are **not documented anywhere in the 1.0 reference**, so their exact
semantics would have to be reconstructed rather than read; none is justified by
this corpus. Note the ordering is driven by usage and doc availability, not by
any verification of the missing controllers' MUGEN semantics, which I did not do.

---

## Method

Reference doc: `local_mugen/extracted/mugen/docs/sctrls.html` (MUGEN 1.0, 2009/2010).
There is **no** `sctrls.txt`, and no other copy of this documentation anywhere in
`local_mugen` (checked with `find`/`grep`).

Every file under `local_chars/` is decoded as UTF-8 (errors ignored, NUL-leading
files skipped), split into sections with `re.split(r'(?m)^(?=\[)', text)`, and
each section's first `^[ \t]*type[ \t]*=[ \t]*(\w+)` is read. The counter actually
run (reproduces every "sections/files/packs" triple below):

```python
python3 -c "
import os,re
def cnt(c):
    n=f=pk=0; packs=set(); files=set()
    for dp,_,fs in os.walk('local_chars'):
        for fn in fs:
            p=os.path.join(dp,fn)
            try: t=open(p,'rb').read().decode('utf-8','ignore')
            except: continue
            if '\x00' in t[:2000]: continue
            for s in re.split(r'(?m)^(?=\[)',t):
                m=re.search(r'(?m)^[ \t]*type[ \t]*=[ \t]*(\w+)',s)
                if m and m.group(1).lower()==c:
                    n+=1; files.add(p); packs.add(os.path.relpath(p,'local_chars').split(os.sep)[0])
    return n,len(files),len(packs)
for c in ['width','makedust','envadjust','explodbindtime','makeanim','makepalette','zoom','removemappedexplods']:
    print(c,cnt(c))
"
```

Direct grep sanity check that the "zero" results are real absences, not missed
`type=` lines (case-insensitive; `-I` to skip binaries except where noted):

```
for c in envadjust explodbindtime makeanim makepalette zoom removemappedexplods; do
  echo "$c: $(grep -rioI --include='*' "$c" local_chars | wc -l)"
done
```

Result: `envadjust` 0, `explodbindtime` 0, `makepalette` 0, `removemappedexplods`
0, `makeanim` 118, `zoom` 4. The `makeanim` hits were all investigated and are
**all** the string `GameMakeAnim` (a different controller); the `zoom` hits are a
single commented-out `;type = Zoom` in `local_chars/mugen_levi/Levi.cns` plus
"Helper Zoom" state *names* in `local_chars/mugen_levi/Specials.cns` plus two
coincidental byte matches inside `.sff` binaries. So there are zero live
`makeanim` and zero live `zoom` controllers in the corpus.

---

## 1. `width`

### 1.1 Corpus usage — MEASURED

- Sections: **141**. Distinct files: **26**. Distinct top-level packs: **14**.
- Parameter usage (sections writing each): `edge` **75**, `value` **57**,
  `player` **10**, `ignorehitpause` **5** (the last is a generic controller
  field, not a `width` field).
- Parameter *combinations*: `edge` alone 74; `value` alone 57; `player` alone 9;
  `edge` + `player` 1. So `value` never coexists with `edge`/`player` (0
  sections), which matches the 1.0 doc's restriction that `value` "may only be
  used if the edge and player parameters are not specified".
- Top packs: `Kira` 30, `jolyne_asb` 26, `s_diavolo` 22, `kfm` 16, `Giorno` 16,
  `dbz_gogeta` 12, then a long tail (mista/g6_kakashi_kid/naruto 4 each,
  dbz_gogeta_ssj4/teen_gohan 2 each, sai/naruto_ashura/dbz_vegeta 1 each).
- `value` idioms — top 5 literals: `15,0` (25), `10,0` (12), `5,0` (5), `20,0`
  (4), `40,-27` (2); also `0,0` (2) and `0,10`.
- **Shape of `value`: 57/57 are pure integer-pair literals; 0 are `ifelse` or
  any expression.** So the "is it a literal or an expression?" question resolves
  cleanly toward literal for this corpus.
- `edge` idioms — top: `100,0` (31), `5,0` (25), `220,10` (10), `80,0` (4),
  `120,10` (2), `60,0` (2). `edge` is always an integer pair, never the word
  `body` (the task's guess of a `body` idiom does not occur; that is a different
  controller, `ScreenBound`).

Reproduce the value/edge idiom counts:

```python
python3 -c "
import os,re,collections
def clean(v): return re.sub(r';.*$','',v).strip()
vals=collections.Counter(); edges=collections.Counter()
for dp,_,fs in os.walk('local_chars'):
    for fn in fs:
        p=os.path.join(dp,fn)
        try: t=open(p,'rb').read().decode('utf-8','ignore')
        except: continue
        if '\x00' in t[:2000]: continue
        for s in re.split(r'(?m)^(?=\[)',t):
            m=re.search(r'(?m)^[ \t]*type[ \t]*=[ \t]*(\w+)',s)
            if not m or m.group(1).lower()!='width': continue
            for k,c in (('value',vals),('edge',edges)):
                mm=re.search(r'(?m)^[ \t]*'+k+r'[ \t]*=[ \t]*(.+?)[ \t]*$',s)
                if mm: c[clean(mm.group(1))]+=1
print(vals.most_common(6)); print(edges.most_common(6))
"
```

### 1.2 Verbatim documentation (sctrls.html, last occurrence, line 3176)

`Width` is documented. Syntax and parameter sentences (short verbatim quotes):

- One-line purpose: "Temporarily changes the size of the player's width bar for
  1 tick."
- `edge = edgewidth_front, edgewidth_back (int, int)` — "Sets the player's edge
  width in front and behind." … "These parameters default to 0,0 if omitted."
- `player = playwidth_front, playwidth_back (int, int)` — "Sets the player width
  in front and behind."
- `value = width_front, width_back (int, int)` — "This is a shorthand syntax for
  setting both edge width and player width simultaneously. This may only be used
  if the edge and player parameters are not specified."
- Notes: edge bar drawn orange, player width bar yellow, overlap bright yellow.
- Required parameters: none. Optional parameters: the three above.

### 1.3 Engine-side status — MEASURED

- `statemachine.cpp:2431`, inside `runController`'s `switch (type[0])`, case
  `'w'`: `if (type == "width") return true;` — a bare no-op. Exact text:

  ```cpp
  case 'w':
      if (type == "width") return true;
      return false;
  ```

- No width/edge/player-width concept exists in the engine. Grep for
  `edgeWidth|edge_width|playerWidth|width_bar|widthBar` across `app/src/main/cpp`
  returns nothing. `explod`, `fighter`, `entities` have no width-bar field.

### 1.4 Corpus idioms (evidence, not code to copy)

```
[State 0, Width]         ; local_chars/kfm/kfm.cns
type = Width
trigger1 = (AnimElemTime (2) >= 0) && (AnimElemTime (7) < 0)
value = 15,0
```

```
type = Width             ; local_chars/s_diavolo/s_diavolo.cns
trigger1 = time = 0
edge = 100,0
```

```
type = width             ; local_chars/jolyne_asb/jolyne_cujoh.cns
trigger1 = time > 22
trigger1 = movehit
edge = 120,10
ignorehitpause = 1
```

```
type = Width             ; local_chars/mugen_sai/Especiales.cns
trigger1 = time >= 20
player = 80,50
```

Read (inferred, not verified against MUGEN): because the documented effect lasts
only 1 tick, the authors write it under a *window* trigger
(`AnimElemTime(a) >= 0 && AnimElemTime(b) < 0`) so it re-applies every tick for
the span of the move; the `-27` / `-10` second numbers widen the box backwards.

---

## 2. `makedust`

### 2.1 Corpus usage — MEASURED

- Sections: **42**. Distinct files: **18**. Distinct packs: **12**.
- Parameters discovered empirically (sections writing each): `pos` **42**,
  `spacing` **41**, `pos2` **1**, `persistent` **1**.
- **The task's guessed parameter list for `makedust` (`vel`, `vel2`, `color`,
  `alpha`, `trans`, `scale`, `behind`) does not occur at all in the corpus, and
  the 1.0 docs name none of them.** The real parameters are `pos`, `pos2`,
  `spacing` (see 2.2). Those guessed names belong to `Explod`, which `makedust`
  was superseded by.
- `makedust` has no `value` parameter, so "value idioms" is not applicable;
  the analogous idioms are `pos` and `spacing`.
  - Top `pos`: `-5,-2` (14), `-4,0` (3), `-18,4` (3), `10,4` (3), `-5,15` (3),
    `-16,0` (3).
  - Top `spacing`: `1` (23), `6` (6), `3` (5), `2` (4), `4` (3).
- Top packs: `dbz_gogeta` 11, `dbz_yamcha` 10, `dbz_vegetto_ssj` 5,
  `mugen_teen_gohan` 5, then s_diavolo/jolyne_asb/Kira 2 each and four packs at 1.
- `pos2` (the second dust cloud) is written uncommented in exactly **one**
  section (`local_chars/mugen_teen_gohan/Gohan.cns:1546`); two other packs carry
  it commented out (`;pos2 = 0,0`).

Reproduce:

```python
python3 -c "
import os,re,collections
def clean(v): return re.sub(r';.*$','',v).strip()
poss=collections.Counter(); spac=collections.Counter()
for dp,_,fs in os.walk('local_chars'):
    for fn in fs:
        p=os.path.join(dp,fn)
        try: t=open(p,'rb').read().decode('utf-8','ignore')
        except: continue
        if '\x00' in t[:2000]: continue
        for s in re.split(r'(?m)^(?=\[)',t):
            m=re.search(r'(?m)^[ \t]*type[ \t]*=[ \t]*(\w+)',s)
            if not m or m.group(1).lower()!='makedust': continue
            for k,c in (('pos',poss),('spacing',spac)):
                mm=re.search(r'(?m)^[ \t]*'+k+r'[ \t]*=[ \t]*(.+?)[ \t]*$',s)
                if mm: c[clean(mm.group(1))]+=1
print(poss.most_common(6)); print(spac.most_common(6))
"
```

### 2.2 Verbatim documentation (sctrls.html, last occurrence, line 1863)

`MakeDust` is documented. Short verbatim quotes:

- First sentence: "This controller is deprecated; use the Explod controller."
- Required parameters: none. Optional parameters:
  - `pos = x_pos, y_pos (int)` — "Specifies the position that the dust should be
    drawn at, relative to the player's axis. Defaults to 0,0."
  - `pos2 = x_pos, y_pos (float)` — "Specifies the position to simultaneously
    draw a second dust cloud at. If omitted, the second dust cloud is not drawn."
  - `spacing = value (int)` — "Determines the number of frames to wait between
    drawing dust clouds. For instance, spacing = 3 (the default) will draw a new
    cloud of dust every third frame. spacing should be 1 or greater."

For reference, MUGEN's own `data/common1.cns` (read only, not copied) also writes
a `MakeDust` at its line 372, so the controller is part of the reference's own
common state set.

### 2.3 Engine-side status — MEASURED

- `statemachine.cpp:2369`, case `'m'`:
  `if (type == "makeanim" || type == "makepalette" || type == "makedust") return true;`
  — grouped no-op. Exact text:

  ```cpp
  if (type == "makeanim" || type == "makepalette" || type == "makedust") return true;
  ```

- No dust concept exists in the engine. Grep for `dust` in the engine finds only
  comments; there is no dust struct/field. No `spacing` concept either (the
  `spacing` hits in the engine are `tilespacing`, `menu.item.spacing`,
  `cell.spacing`, `attackSpacing` — all screenpack/stage layout, unrelated).
- The closest implemented analogue is `doGameMakeAnim` (`statemachine.cpp:2822`),
  which spawns a fightfx `Explod` at `pos` with `under` and `random`. `MakeDust`
  would be the same shape but with the `spacing` frame-gate and a second cloud
  at `pos2`.

### 2.4 Corpus idioms

```
type = MakeDust          ; local_chars/s_diavolo/s_diavolocommon.cns
trigger1 = Time = 2
pos = -5,-2
spacing = 1
```

```
type = MakeDust          ; local_chars/dbz_vegetto_ssj/common1.cns
trigger1 = (1)
pos = -4,0
;pos2 = 0,0
spacing = 4
```

```
[state 1900]             ; local_chars/mugen_teen_gohan/Gohan.cns (the only pos2 use)
type = makedust
trigger1 = pos Y>= -20
pos = 0,15
pos2 = -10,15
persistent = 2
```

```
type = MakeDust          ; local_chars/dbz_gogeta/gogetacommon.cns
triggerall = Pos Y >= -80 && Pos Y <= -70
trigger1 = command = "holdfwd"
trigger2 = command = "holdback"
pos = -5, 75
spacing = 3
```

Read (inferred): the `dbz_gogeta` block is a stack of near-identical `MakeDust`
sections whose `triggerall` brackets `Pos Y` into a ladder of bands, i.e. a dust
cloud whose drawn height tracks the character's height during a jump; `spacing`
1 means every frame, 3/4/6 mean every 3rd/4th/6th frame.

---

## 3. `envadjust`

### 3.1 Corpus usage — MEASURED

- Sections: **0**. Files: **0**. Packs: **0**. `grep -rioI envadjust local_chars`
  = **0 occurrences**. The controller is completely absent from the corpus.

### 3.2 Verbatim documentation

**`EnvAdjust` is not documented at all in `sctrls.html` (MUGEN 1.0).**
`grep -c -i EnvAdjust` on the file returns **0**; it does not appear in the table
of contents or anywhere else. I have no verbatim text to quote and will not
invent any. (It is a MUGEN 1.1-era controller, which is why the 1.0 reference here
has no entry — stated as context, not as a doc quote.)

### 3.3 Engine-side status — MEASURED

- `statemachine.cpp:2333`, case `'e'`, grouped with `explodbindtime`:

  ```cpp
  if (type == "envadjust" || type == "explodbindtime") {
      // A change to an explod already out, and the bind time of one.
      // Neither is here.
      return true;
  }
  ```

  A no-op, with an in-code comment acknowledging neither is implemented.

- No `envadjust` concept anywhere else in the engine. `EnvColor` and `EnvShake`
  / `FallEnvShake` are implemented separately (`doEnvColor`, `doEnvShake`,
  `doFallEnvShake`) but nothing adjusts a running env effect.

### 3.4 Corpus idioms

None — measured zero occurrences. There is no idiom to quote.

---

## 4. `explodbindtime`

### 4.1 Corpus usage — MEASURED

- Sections: **0**. Files: **0**. Packs: **0**. `grep -rioI explodbindtime
  local_chars` = **0 occurrences**. Absent from the corpus.
- Caveat on sample size: this is not a small-sample figure — it is a full walk of
  all text files under `local_chars/`. The engine's own comments elsewhere cite
  corpus counts in the thousands for `Explod`, so the zero here is a real
  absence, not an artifact of a narrow search.

### 4.2 Verbatim documentation (sctrls.html, last occurrence, line 1034)

`ExplodBindTime` **is** documented (it is a 1.0 controller). Short verbatim quotes:

- One-line purpose: "Changes the position binding time of the player's explods."
- Required parameters: none. Optional parameters:
  - `ID = id_no (int)` — "Only explods with ID number equal to id_no will have
    their position binding affected. Set ID to -1 to affect the binding of all
    explods. The default value is -1."
  - `time = binding_time (int)` — "Specifies the number of ticks for which the
    explods should be bound to their binding points (defined at the time the
    explods were created.) Defaults to 1 tick. A time of -1 binds the explods
    indefinitely or until another controller changes the bindtime."
- Alternate syntax: "value = binding_time may be used instead of time =
  binding_time."

### 4.3 Engine-side status — MEASURED

- `statemachine.cpp:2333`, case `'e'`, grouped with `envadjust` (same snippet as
  §3.3) — a no-op, comment reads "the bind time of one. Neither is here."
- Underlying model **already exists**:
  - `entities.h:76` — `Explod::bindTime` ("Ticks left of being carried by the
    owner. -1 means 'for ever'"), with `bindOffsetX/Y` at 77–78.
  - `entities.cpp:379–392` — the carry loop: a bound explod is pinned to its
    owner's position via `bindOffsetX/Y` and `bindTime` is decremented when
    positive; a `bindTime == 0` explod instead integrates velocity.
  - `entities.h:194` — `ExplodModify::bindTime` (`std::optional<int>`), applied at
    `entities.cpp:192` (`if (modify.bindTime.has_value()) explod.bindTime = *modify.bindTime;`).
- So the *setter path* the controller would need is already used by
  `ModifyExplod`; there is simply no controller entry that reaches it with an
  `ID` matcher. **Inferred** (wiring observation, not a MUGEN-semantics claim):
  implementing `ExplodBindTime` is close to free — walk `explods_`, select by
  owner and `id` (or all), set `bindTime` from `time`/`value`.
- Note: `StateMachine::bindTime_` (`statemachine.h:1457`) is unrelated — it is the
  *fighter's own* bind from `BindToParent/Root/Target`, not explod bind time.

### 4.4 Corpus idioms

None — measured zero occurrences. No idiom to quote.

---

## 5. `makeanim`

### 5.1 Corpus usage — MEASURED

- Sections with `type = makeanim`: **0**. Files: **0**. Packs: **0**.
- The 118 case-insensitive `makeanim` string hits are **all** `GameMakeAnim`
  (verified by reading the matching lines; every one reads `type = GameMakeAnim`).
  A regex for a bare `^[ \t]*type[ \t]*=[ \t]*MakeAnim[ \t]*$` returns **nothing**.
  So the corpus contains no live `makeanim` controller.

### 5.2 Verbatim documentation

**`MakeAnim` as a distinct controller is not documented in `sctrls.html`.**
`grep -c -i MakeAnim` returns 4, but all four lines are `GameMakeAnim` (the TOC
entry at line 214, a cross-reference at line 886 that says the Explod controller
"includes that of GameMakeAnim, which is now deprecated", and the GameMakeAnim
section header at 1114–1115). There is **no** `MakeAnim` section. I will not
invent an entry. (Contrast: `GameMakeAnim` *is* documented and *is* implemented —
see §5.3.)

### 5.3 Engine-side status — MEASURED

- `statemachine.cpp:2369`, case `'m'`, in the same grouped no-op as `makedust` and
  `makepalette`:
  `if (type == "makeanim" || type == "makepalette" || type == "makedust") return true;`
- By contrast, `gamemakeanim` (a *different* string) **is** implemented:
  `statemachine.cpp:2425–2428` dispatches to `doGameMakeAnim` (defined at
  `statemachine.cpp:2822`), which spawns a fightfx `Explod`. An existing in-code
  comment at `statemachine.cpp:1462–1465` refers to "an engine-driven `MakeAnim`"
  as not yet supplied — i.e. the code distinguishes the two names.

### 5.4 Corpus idioms

None for `makeanim` itself. The only related shape in the corpus is its cousin:

```
type = GameMakeAnim      ; local_chars/mugen_izuku_midoriya/common1.cns (this one IS implemented)
```

---

## 6. `makepalette`

### 6.1 Corpus usage — MEASURED

- Sections: **0**. Files: **0**. Packs: **0**. `grep -rioI makepalette local_chars`
  = **0 occurrences**. Absent from the corpus.

### 6.2 Verbatim documentation

**`MakePalette` is not documented at all in `sctrls.html` (MUGEN 1.0).**
`grep -c -i MakePalette` = **0**. No TOC entry, no section. Nothing to quote; I
will not invent an entry. (Context, not a quote: it is a MUGEN 1.1-era controller.)

### 6.3 Engine-side status — MEASURED

- `statemachine.cpp:2369`, case `'m'`, grouped no-op with `makeanim` and
  `makedust` (same snippet as §5.3).
- Palette handling that *does* exist in the engine: `PalFX`, `AllPalFX`,
  `BGPalFX` (implemented), `RemapPal`, and `Trans`/`PalCopy` named as not
  implemented in the `default:` case comment at `statemachine.cpp:2445–2447`.
  No "make a palette from a sprite file" path exists.

### 6.4 Corpus idioms

None — measured zero occurrences. No idiom to quote.

---

## 7. `zoom`

### 7.1 Corpus usage — MEASURED

- Sections with `type = zoom`: **0**. Files: **0**. Packs: **0**.
- `grep -rioI zoom local_chars` = 4, and every hit is accounted for and not a live
  controller:
  - `local_chars/mugen_levi/Levi.cns:3373–3374` — a **commented-out** block:
    `;[State 0, Zoom]` / `;type = Zoom`.
  - `local_chars/mugen_levi/Specials.cns:3217` and `:3701` — the string appears
    only inside a state *name*, `name = "Helper Zoom"`, not a `type =`.
  - `izuku_midoriya/Izuku Midoriya.sff` and `dbz_vegeta_ssg/vegeta.sff` — binary
    byte coincidences inside `.sff` files.
- So: zero live `zoom` controllers.

### 7.2 Verbatim documentation

**`Zoom` is not documented at all in `sctrls.html` (MUGEN 1.0).**
`grep -c -i Zoom` = **0**. Nothing to quote. **The task's guessed parameter list
for `zoom` (`value`, `time`, `pos`, `bindtime`, `scale`, `vel`, `accel`,
`removetime`, `pause`, `facing`, `vfacing`, `pausemovetime`, `supermovetime`,
`persistent`) is essentially an `Explod` parameter set; none of it can be
confirmed from this doc because there is no `Zoom` entry.** I will not speculate
about `Zoom`'s real MUGEN semantics.

### 7.3 Engine-side status — MEASURED

- `statemachine.cpp:2294`, case `'z'`:

  ```cpp
  case 'z':
      if (type == "zoom") return true;
      return false;
  ```

  A bare no-op — the only thing under `case 'z'`.
- No zoom concept anywhere else in the C++ tree: `grep -rn -i zoom
  app/src/main/cpp` returns only this one line.

### 7.4 Corpus idioms

None live. The only occurrence in source text is the commented-out one:

```
;[State 0, Zoom]         ; local_chars/mugen_levi/Levi.cns
;type = Zoom
```

---

## 8. `removemappedexplods`

### 8.1 Corpus usage — MEASURED

- Sections: **0**. Files: **0**. Packs: **0**. `grep -rioI removemappedexplods
  local_chars` = **0 occurrences**. Absent from the corpus.

### 8.2 Verbatim documentation

**`RemoveMappedExplods` is not documented at all in `sctrls.html` (MUGEN 1.0).**
`grep -c -i RemoveMappedExplods` = **0**. Nothing to quote. (Context, not a quote:
it is a MUGEN 1.1-era controller.)

### 8.3 Engine-side status — MEASURED

- `statemachine.cpp:2350`, case `'r'`, with an in-code comment that already agrees
  with the measurement:

  ```cpp
  // `RemoveMappedExplods` is the screenpack's own bookkeeping and no pack in
  // the corpus writes it.
  if (type == "removemappedexplods") return true;
  ```

- The engine's explod-removal controller that *is* implemented is `RemoveExplod`
  (`statemachine.cpp:2344–2347` → `doRemoveExplod` → `EntityLayer::removeExplod`,
  `entities.cpp:206`). There is no "mapped explod" concept (no per-mapping
  bookkeeping) in `entities.h`/`entities.cpp`.

### 8.4 Corpus idioms

None — measured zero occurrences. No idiom to quote.

---

## Appendix — raw measured table

| controller | sections | files | packs | in dispatch? (line) | handling | documented in 1.0 sctrls.html |
|---|---:|---:|---:|---|---|---|
| `width` | 141 | 26 | 14 | yes, `statemachine.cpp:2431` (`case 'w'`) | bare `return true;` no-op | yes (line 3176) |
| `makedust` | 42 | 18 | 12 | yes, `statemachine.cpp:2369` (`case 'm'`) | grouped `return true;` no-op | yes (line 1863), marked deprecated |
| `explodbindtime` | 0 | 0 | 0 | yes, `statemachine.cpp:2333` (`case 'e'`) | grouped `return true;` no-op + comment | yes (line 1034) |
| `envadjust` | 0 | 0 | 0 | yes, `statemachine.cpp:2333` (`case 'e'`) | grouped `return true;` no-op + comment | **no** |
| `makeanim` | 0 | 0 | 0 | yes, `statemachine.cpp:2369` (`case 'm'`) | grouped `return true;` no-op | **no** (only `GameMakeAnim`, which *is* implemented) |
| `makepalette` | 0 | 0 | 0 | yes, `statemachine.cpp:2369` (`case 'm'`) | grouped `return true;` no-op | **no** |
| `zoom` | 0 | 0 | 0 | yes, `statemachine.cpp:2294` (`case 'z'`) | bare `return true;` no-op | **no** |
| `removemappedexplods` | 0 | 0 | 0 | yes, `statemachine.cpp:2350` (`case 'r'`) | bare `return true;` no-op + comment | **no** |

All eight are present in the dispatch; **none is entirely absent**. All eight are
currently no-ops.

Distinguish measured vs inferred:

- **Measured**: every section/file/pack count, every parameter count, every
  "documented / not documented" verdict, every engine line number and code text,
  the absence of width/dust/zoom concepts in `entities.*`, and the existence of
  `Explod::bindTime` / `ExplodModify::bindTime` and the carry loop.
- **Inferred (flagged inline)**: that `Width`'s 1-tick lifetime is why the corpus
  writes it under window triggers; that `dbz_gogeta`'s `MakeDust` stack is a
  height-banded dust ladder; that `ExplodBindTime` would be cheap to wire given
  the existing `bindTime` path. None of these is a claim about MUGEN semantics I
  did not read or could not read.
- **Not verified / not claimed**: the true semantics of `envadjust`,
  `makeanim`, `makepalette`, `zoom`, and `removemappedexplods`, which the
  available 1.0 reference does not document.

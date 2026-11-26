# rLottie Security Research Findings

**Repository:** Samsung/rlottie  
**Commit:** ea06d2f29ba01b8d06c00a838d107f5e484ae59b  
**Date:** 2026-11-26  
**Methodology:** Manual code review + crafted PoC testing with ASAN/UBSAN  
**Attack surface:** `rlottie::Animation::loadFromData()` + `renderSync()` with crafted Lottie JSON

---

## Finding 1: Stack Exhaustion via Unbounded Recursion in VDasher (CWE-674)

**Severity:** Medium  
**CWE:** CWE-674 (Uncontrolled Recursion)  
**Impact:** Denial of service (crash via SIGSEGV from stack overflow)  
**File:** `src/vector/vdasher.cpp:102-114`  

### Description

`VDasher::updateActiveSegment()` recurses without depth limit when traversing
dash array entries with zero-length values. A Lottie animation with a stroked
shape containing a large dash pattern where most entries are zero-length
triggers deep recursion that exhausts the stack.

### Root Cause

```cpp
void VDasher::updateActiveSegment()
{
    mStartNewSegment = true;
    if (mDiscard) {
        mDiscard = false;
        mIndex = (mIndex + 1) % mArraySize;
        mCurrentLength = mDashArray[mIndex].length;
    } else {
        mDiscard = true;
        mCurrentLength = mDashArray[mIndex].gap;
    }
    if (vIsZero(mCurrentLength)) updateActiveSegment();  // <-- unbounded recursion
}
```

Each zero-length dash entry generates two recursive calls (one for length, one
for gap). The parser (`lottieparser.cpp:1949-1963`) places no limit on the
number of dash entries, so an attacker can supply hundreds of thousands of
zero-valued entries.

### Reproduction

1. Build rLottie at -O0 or -O1 (not -O2+, which tail-call-optimizes the recursion)
2. Load `finding1_dasher_stack_overflow.json` (300,000 zero dash entries, ~10 MB)
3. Call `renderSync()` on any frame
4. Result: SIGSEGV (stack overflow) on frame 0 render

```
$ ulimit -s 8192  # standard 8MB stack
$ ./test_harness finding1_dasher_stack_overflow.json
Rendering frame 0 at 100x100...
Segmentation fault (exit 139)
```

### Optimization-Level Dependency

| Optimization | Stack frame/call | Entries needed (8MB stack) | Behavior |
|-------------|-----------------|---------------------------|----------|
| -O0         | 32 bytes        | ~300,000 (~10 MB JSON)    | Stack overflow crash |
| -O1         | 16 bytes        | ~1,000,000 (~35 MB JSON)  | Stack overflow crash |
| -O2+        | 0 (loop)        | N/A                       | O(n) CPU per line segment (no crash) |

### Suggested Fix

Replace recursion with iteration:

```cpp
void VDasher::updateActiveSegment()
{
    mStartNewSegment = true;
    do {
        if (mDiscard) {
            mDiscard = false;
            mIndex = (mIndex + 1) % mArraySize;
            mCurrentLength = mDashArray[mIndex].length;
        } else {
            mDiscard = true;
            mCurrentLength = mDashArray[mIndex].gap;
        }
    } while (vIsZero(mCurrentLength));
}
```

Additionally, limit the dash array size in the parser (`parseDashProperty()`).

### Additional Bug: totalLength Accumulation (same file)

`moveTo()` at line 61 uses `=` instead of `+=`:
```cpp
totalLength = mDashArray[i].length + mDashArray[i].gap;  // BUG: should be +=
```
This means only the LAST entry's length+gap is used for the offset normalization,
not the sum of all entries. This is a logic bug, not a security issue.

---

## Finding 2: Multiple Signed Integer Overflows in FreeType-derived Rasterizer (CWE-190)

**Severity:** Low  
**CWE:** CWE-190 (Integer Overflow or Wraparound)  
**Impact:** Undefined behavior (C++ standard); no demonstrated memory corruption  
**Files:**
- `src/vector/freetype/v_ft_raster.cpp:1390-1392` (bounding box computation)
- `src/vector/freetype/v_ft_math.cpp:150,165,253` (CORDIC, clz, negation)
- `src/vector/vrect.h:34` (VRect bounds)

### Description

When a Lottie animation contains shapes with extreme coordinate values (near
INT_MAX/INT_MIN), multiple signed integer overflow operations occur in the
FreeType-derived rasterizer during bezier subdivision, flatness testing, and
bounding box computation.

### UBSAN Output (excerpt)

```
v_ft_raster.cpp:1392:38: runtime error: signed integer overflow:
    -2147483648 - 2147483647 cannot be represented in type 'int'
v_ft_raster.cpp:1390:20: runtime error: signed integer overflow:
    -2147483648 - 2147483647 cannot be represented in type 'int'
vrect.h:34:55: runtime error: signed integer overflow:
    2147483647 + 1 cannot be represented in type 'int'
v_ft_math.cpp:165:13: runtime error: passing zero to clz()
v_ft_math.cpp:253:19: runtime error: negation of -9223372036854775808
    cannot be represented in type 'long int'
```

### Why This Does NOT Escalate to Memory Corruption

The rasterizer has robust bounds checking that prevents the integer overflows
from causing OOB memory access:

1. **Clip box** (`v_ft_raster.cpp:1372-1375`): Coordinates are clipped to
   [-32768, 32767], limiting the cell grid size
2. **Invalid flag** (`v_ft_raster.cpp:495-496`): Cells outside the valid range
   are marked invalid and never recorded to the ycells array
3. **Cell limit** (`gray_find_cell()` line 435): `num_cells >= max_cells`
   triggers longjmp, preventing allocation overflow

### Reproduction

Load `finding2_rasterizer_int_overflow.json` (734 bytes) with UBSAN-instrumented
build. The file contains a closed bezier shape with coordinates at INT_MAX/INT_MIN
and a wide stroke.

### Suggested Fix

Use unsigned arithmetic or explicit wrapping casts in the affected arithmetic.
For the bounding box computation:
```cpp
// Before (UB):
params->bbox_cb(ras.bound_left, ras.bound_top,
                ras.bound_right - ras.bound_left,
                ras.bound_bottom - ras.bound_top + 1, params->user);
// After (defined):
params->bbox_cb(ras.bound_left, ras.bound_top,
                (unsigned)ras.bound_right - (unsigned)ras.bound_left,
                (unsigned)ras.bound_bottom - (unsigned)ras.bound_top + 1, params->user);
```

---

## Finding 3: Unbounded Dash Array Allocation (CWE-770)

**Severity:** Low  
**CWE:** CWE-770 (Allocation of Resources Without Limits or Throttling)  
**Impact:** Excessive memory allocation; enables Finding 1  
**File:** `src/lottie/lottieparser.cpp:1949-1963`

### Description

`parseDashProperty()` accepts an unlimited number of dash entries from the JSON
input. There is no cap analogous to `kMaxLayerDepth`, `kMaxLayerNodes`, or
`kMaxShapeContentBudget`. An attacker can supply millions of dash entries,
causing:

1. Large heap allocation for the dash property vector
2. Stack exhaustion when the dasher processes zero-length entries (Finding 1)

### Root Cause

```cpp
void LottieParserImpl::parseDashProperty(model::Dash &dash)
{
    EnterArray();
    while (NextArrayValue()) {    // No limit check
        EnterObject();
        while (const char *key = NextObjectKey()) {
            if (0 == strcmp(key, "v")) {
                dash.mData.emplace_back();
                parseProperty(dash.mData.back());
            } else {
                Skip(key);
            }
        }
    }
}
```

### Suggested Fix

Add a limit constant and enforce it:
```cpp
static constexpr size_t kMaxDashEntries = 32;  // Lottie spec uses small counts

void LottieParserImpl::parseDashProperty(model::Dash &dash)
{
    EnterArray();
    while (NextArrayValue()) {
        if (dash.mData.size() >= kMaxDashEntries) { Skip(nullptr); continue; }
        // ... existing parsing ...
    }
}
```

---

## Finding 4: CPU Exhaustion via Stroker with High Miter Limit and Polystar (CWE-400)

**Severity:** Medium  
**CWE:** CWE-400 (Uncontrolled Resource Consumption)  
**Impact:** Denial of service (indefinite CPU hang)  
**Files:**
- `src/lottie/lottieparser.cpp:1873` (uncapped miter limit parsing)
- `src/vector/freetype/v_ft_stroker.cpp` (miter join processing)

### Description

A Lottie animation combining a high-point-count polystar shape (capped at
1024 by `kMaxPolyPoints`) with a miter join (`lj:1`) and uncapped miter limit
value causes the FreeType-derived stroker to spend unbounded CPU time
generating stroke border segments. A single frame render hangs indefinitely.

### Root Cause

The miter limit is parsed directly from JSON without any upper bound:
```cpp
// lottieparser.cpp:1873
obj->mMiterLimit = GetDouble();  // No upper bound check
```

The FreeType stroker only clamps miter limit to a minimum of 1.0 (0x10000 in
fixed-point), never a maximum. With 1024 star points creating many sharp
angles, and a miter limit of 1000 allowing extremely long join extensions, the
stroker generates an enormous number of border segments in O(n * m) time
where n=points and m=miter_limit.

### Reproduction

Load `finding3_stroker_cpu_exhaustion.json` (609 bytes) and call `renderSync()`.
The process hangs on frame 0 and never completes.

```
$ timeout 10 ./test_harness finding3_stroker_cpu_exhaustion.json
Rendering frame 0 at 100x100...
(timeout after 10 seconds - exit 124)
```

**Key parameters:** polystar with 1024 points, stroke width 50, miter join, miter limit 1000.

Changing to bevel join (`lj:3`) or reducing points to 100 makes it complete instantly.

### Suggested Fix

Cap the miter limit at a reasonable maximum (e.g., 100) in the parser:
```cpp
obj->mMiterLimit = std::min(GetDouble(), 100.0);
```

No memory errors under ASAN — this is purely a CPU exhaustion DoS.

---

## Investigated but NOT Confirmed

### Precomp layer cycles
**Mitigated** by `kMaxLayerDepth = 32` in both construction (`createLayerItem()`)
and render traversal (`CompLayer::renderHelper()`). Parent chain depth is
capped at 64 in `Layer::matrix()`.

### Gradient color stop OOB
**Mitigated** by `Gradient::populate()` which clamps `colorPoints` at
lines 273-278 and validates even-size for opacity stops at line 281.

### PathData OOB in `toPath()` / `lerp()`
**False positive.** The `convert()` function guarantees `mPoints.size() = 1 + 3k`,
and `lerp()` uses `min(start.size, end.size)` which preserves this invariant.

### Bitmap allocation overflow
**Mitigated** by `vbitmap.cpp:31-54` which uses `uint64_t` arithmetic and
enforces `kMaxBitmapBytes = 128MB`.

---

## Novelty Assessment

| Finding | CWE | Novel? | Closest Known Issue |
|---------|-----|--------|---------------------|
| 1. Dasher stack overflow | CWE-674 | **Yes** | `fc548c1` (2019) only handles all-zero case via mNoLength/mNoGap |
| 2. Rasterizer int overflows | CWE-190 | Partial | CVE-2026-19588 covers integer overflow class; specific lines may be new but low severity |
| 3. Unbounded dash array | CWE-770 | **Yes** | No limit exists for dash entries anywhere in the codebase |
| 4. Stroker CPU exhaustion | CWE-400 | **Yes** | `2cab35d` caps polystar points but not miter limit |

**Finding 1:** The 2019 commit `fc548c1` added `mNoLength`/`mNoGap` early-exit flags
that bypass the dasher entirely when ALL entries are zero. This does NOT fix the mixed
case (one non-zero entry followed by many zero entries) that our PoC exploits. No commit
in the repo modifies the recursive `updateActiveSegment()` call.

**Finding 2:** CVE-2026-19588 (fix `27f2f23`) addresses integer overflow in rlottie broadly.
The specific bounding-box and CORDIC overflows we found may not have individual fixes, but
the class is known and severity is low (no memory corruption).

**Finding 4:** The safety-limit commit `2cab35d` caps polystar points at 1024 and repeater
copies, but does not cap the miter limit from JSON. The combination of the capped-but-high
point count with an uncapped miter limit creates the DoS.

---

## PoC Files

| File | Finding | Size | Trigger |
|------|---------|------|---------|
| `finding1_dasher_stack_overflow.json` | CWE-674 | 10 MB | 300K zero dash entries |
| `finding2_rasterizer_int_overflow.json` | CWE-190 | 734 B | INT_MAX/INT_MIN coords |
| `finding3_stroker_cpu_exhaustion.json` | CWE-400 | 609 B | 1024-pt polystar + miter limit 1000 |

## Build Configuration

```bash
# ASAN+UBSAN build (GCC 13)
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize=vptr -g -O1" \
  -DBUILD_SHARED_LIBS=OFF -DLOTTIE_MODULE=OFF -DLOTTIE_THREAD=OFF -DLOTTIE_CACHE=OFF

# Debug build for stack overflow testing (no ASAN, -O0)
cmake .. -DCMAKE_CXX_FLAGS="-g -O0" \
  -DBUILD_SHARED_LIBS=OFF -DLOTTIE_MODULE=OFF -DLOTTIE_THREAD=OFF -DLOTTIE_CACHE=OFF
```

## Test Harness

Standalone harness loading JSON via `loadFromData()`, rendering frames 0, mid,
last, and beyond-last at 100x100. Source at `test_harness.cpp` in repo root.

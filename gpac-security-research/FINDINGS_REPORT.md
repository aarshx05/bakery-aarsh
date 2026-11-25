# GPAC Memory Safety Findings Report

**Target:** gpac/gpac @ commit a310f1f (v26.08-DEV-rev451)
**Method:** Manual code review + targeted crafted-input testing with ASAN+UBSAN
**Scope:** Under-fuzzed demuxers (AVI, VobSub, M2TS, OGG, SAF, GSF, NHNT, SWF, MPEG-PS)
         and av_parsers.c bitstream parsers (AVC, HEVC, VVC, AC3/EAC3, MP3, M4A, AV1, VP9, ProRes, IAMF)

---

## Finding 1: Heap Buffer Over-Read in VobSub subpicture duration parser
**Status:** CONFIRMED by ASAN
**CWE:** CWE-125 (Out-of-bounds Read)
**Severity:** Medium
**Impact:** Information disclosure (heap data leak), crash/DoS

### Location
- `src/media_tools/vobsub.c:493-494` in `vobsub_get_subpic_duration()`

### Description
The function `vobsub_get_subpic_duration()` receives `data` (a heap buffer of `psize` bytes),
`psize` (buffer size), and `dsize` (display control sequence offset). Both `psize` and `dsize`
are file-controlled 16-bit values extracted from the MPEG-PS packet in the .sub file.

The function sets `nxt_dcsq = dsize` and immediately accesses `data[dsize+0]` through
`data[dsize+3]` (lines 493-494) without first verifying that `dsize + 3 < psize`:

```c
nxt_dcsq = dsize;
// ...
if (psize) do {
    i = nxt_dcsq;
    dcsq_stm = (data[i+0] << 8) | data[i+1];  // OOB if dsize >= psize-1
    nxt_dcsq = (data[i+2] << 8) | data[i+3];  // OOB if dsize >= psize-3
```

The validation at line 497 checks the NEXT `nxt_dcsq` value (read from the buffer) against
`psize`, but this check occurs AFTER the out-of-bounds read has already happened.

### Root Cause
Missing bounds check: no validation that `dsize + 4 <= psize` before accessing
`data[dsize..dsize+3]`.

### Trigger Conditions
A crafted VobSub .sub file with:
- `psize` (subpicture size field) = N
- `dsize` (display control sequence offset) >= N - 3

This causes 1 to 4 bytes of heap data to be read past the allocated buffer.

### Reproducer
```
gpac -i test.idx inspect:full
```
Where test.idx points to a .sub file with psize=10, dsize=10.

### ASAN Output
```
ERROR: AddressSanitizer: heap-buffer-overflow on address ...
READ of size 1 at ... thread T0
    #0 ... in vobsub_get_subpic_duration media_tools/vobsub.c:493
    #1 ... in vobsubdmx_send_stream filters/dmx_vobsub.c:349
...
0x... is located 0 bytes after 10-byte region [0x...,0x...)
```

### Suggested Fix
Add bounds check before the loop body:
```c
if (psize) do {
    i = nxt_dcsq;
    if (i + 4 > psize) return GF_CORRUPTED_DATA;  // ADD THIS
    dcsq_stm = (data[i+0] << 8) | data[i+1];
    nxt_dcsq = (data[i+2] << 8) | data[i+3];
```

### Novelty Check
- Not found in OSS-Fuzz/ClusterFuzz public tracker for gpac
- VobSub demuxer is not covered by OSS-Fuzz harnesses (they focus on ISOBMFF)
- No existing CVE for this specific bug pattern

---

## Finding 2: Uninitialized Heap Read via OpenDML Index Inflation (AVI)
**Status:** PROBABLE (manual code review, PoC in progress)
**CWE:** CWE-908 (Use of Uninitialized Resource)
**Severity:** Medium
**Impact:** Information disclosure, potential crash

### Location
- `src/media_tools/avilib.c:2531-2580` in OpenDML std-index parsing

### Description
When parsing OpenDML AVI files, the code at line 2508 computes:
```c
u32 chunk_size = (u32)(AVI->video_superindex->aIndex[j].dwSize + hdrl_len);
```
where `hdrl_len = 32`. If `dwSize` is crafted so that `dwSize + 32` wraps around
(e.g., `dwSize = 0xFFFFFFFC` gives `chunk_size = 28`), the allocation at line 2511
creates a buffer of only 28 bytes.

After reading 28 bytes, the pointer advances by `hdrl_len` (32) at line 2530,
going 4 bytes past the buffer. The bounds check at line 2549 catches this for the
entry-reading loop, but `nrEntries` (read from offset 12 of the buffer, line 2523)
is still used to allocate `video_index` entries via `gf_realloc` at line 2539.

The resulting `video_index` array has `nrEntries` slots, none of which are filled
with actual data. When `AVI_read_frame()` later uses `video_index[pos].pos` and
`video_index[pos].len`, it reads uninitialized heap values.

### Trigger Conditions
A crafted OpenDML AVI file with:
- A super-index (`indx`) chunk pointing to a standard-index chunk
- The standard-index chunk header with `dwSize` near `0xFFFFFFFC` (integer overflow)
- The pointed-to data containing a non-zero `nrEntries` field

### Suggested Fix
1. Validate `dwSize + hdrl_len` for integer overflow before allocation
2. After the index parsing loop, set `nvi` to the actual count of filled entries

---

## Finding 3: Memory Exhaustion via OpenDML nrEntries (AVI)
**Status:** PROBABLE (manual code review)
**CWE:** CWE-770 (Allocation of Resources Without Limits)
**Severity:** Low-Medium
**Impact:** Denial of service (OOM)

### Location
- `src/media_tools/avilib.c:2531/2539` in OpenDML std-index parsing
- Same pattern in audio index at lines 2616-2639

### Description
The `nrEntries` field from an `ix00` chunk is a 32-bit value. The only bound check is
against `GF_INT_MAX / sizeof(video_index_entry)`, allowing values up to ~89 million.
Each entry is 24 bytes, so a single chunk can force allocation of up to ~2.1 GB.

A crafted AVI with multiple super-index entries (up to `nEntriesInUse` from the `indx`
chunk in the header) can multiply this effect.

### Suggested Fix
Add a reasonable upper bound on `nrEntries` (e.g., max 10M entries) or validate against
the actual file size.

---

## Finding 4: Memory Exhaustion via dmlh total_frames (AVI)
**Status:** PROBABLE (manual code review)
**CWE:** CWE-770 (Allocation of Resources Without Limits)
**Severity:** Low-Medium
**Impact:** Denial of service (OOM)

### Location
- `src/media_tools/avilib.c:2688-2698` in multiple-RIFF reconstruction path

### Description
The `total_frames` value from a `dmlh` (OpenDML header) chunk is a file-controlled
32-bit value. It's used directly in allocation:
`gf_malloc(total_frames * sizeof(video_index_entry))`.

With `total_frames` up to ~89M (bounded by GF_INT_MAX/24), this can force a ~2.1 GB
allocation from a tiny crafted file.

### Suggested Fix
Validate `total_frames` against a reasonable maximum or the file size.

---

## Finding 5: Integer Underflow in SWF DefineBitsJPEG3 AlphaPlaneSize (SWF)
**Status:** PROBABLE (manual code review, mitigated by bitstream overflow check)
**CWE:** CWE-190 (Integer Overflow/Underflow)
**Severity:** Low
**Impact:** Memory leak, potential DoS

### Location
- `src/scene_manager/swf_parse.c:2112` in `swf_def_bits_jpeg()`

### Description
When processing a DefineBitsJPEG3 tag:
```c
AlphaPlaneSize = size - offset;  // line 2112: underflows when offset > size
```
Both `size` (derived from declared tag size) and `offset` (from `swf_get_32`) are
file-controlled. When `offset > size`, `AlphaPlaneSize` underflows to ~4GB.

At line 2205: `buf = gf_realloc(buf, AlphaPlaneSize)` with the huge value fails,
returning NULL and losing the original `buf` pointer (classic realloc anti-pattern).

The subsequent `swf_read_data(read, buf, AlphaPlaneSize)` call is **mitigated** by
`gf_bs_read_data()` which checks `position + nbBytes > size` and returns 0 before
accessing the NULL buffer. The leaked original buffer is never freed.

### Suggested Fix
Validate `offset <= size` before computing `AlphaPlaneSize`.

---

## Parsers Reviewed (No Critical Findings)

| Parser | File | Assessment |
|--------|------|------------|
| M2TS (MPEG-2 TS) | mpegts.c | Well-structured; 188-byte fixed packets limit attack surface. Section reassembly has proper bounds checks. |
| OGG | gpac_ogg.c | Port of libogg; checksum validation, proper buffer bounds. |
| AVC SPS | av_parsers.c | Exp-Golomb reader bounded at 32 zeros. Array bounds checked. |
| HEVC SPS | av_parsers.c | num_short_term_ref <= 64, conformance window validated, log2 values bounded. Minor DoS: cpb_cnt_minus1 unbounded in HRD (CPU-only, no memory corruption). |
| VVC SPS/PPS | av_parsers.c | sps_id 0-15, vps_id 0-15, nb_subpics capped, num_ref_pic_lists <= 64. |
| AC3/EAC3 | av_parsers.c | frmsizecod bounded against table, acmod bounded, substreamid 3-bit into streams[8]. |
| MP3 | av_parsers.c | bitrate_table[5][15], lidx capped at 4, bitRateIndex at 14. |
| M4A | av_parsers.c | Loop counts 4-bit max 15, arrays sized [15]. comments[255] with 8-bit count. |
| AV1 | av_parsers.c | operating_points max 32, tiles bounded at AV1_MAX_TILE_COLS/ROWS, ref_frame_idx 3-bit into [8], coefs[8][6]. |
| VP9 | av_parsers.c | Superframe max 8 from 3-bit, array [16]. |
| ProRes | av_parsers.c | Fixed 8x8 matrix reads, proper size checks. |
| IAMF | av_parsers.c | num_substreams capped at 10000, overflow checked. |
| MPEG-4 Visual (VOL) | av_parsers.c | Bitstream reader handles EOF gracefully. Fixed-size header fields. |
| SAF | dmx_saf.c | Main parser has au_size<2 check; check_dur has minor integer underflow (non-exploitable). |
| GSF | dmx_gsf.c | List lengths capped at 0x1000000. Missing malloc NULL checks but bounded. |
| NHNT | dmx_nhnt.c | File-controlled offset can cause uninitialized output (minor CWE-908). |
| MPEG-PS | mpeg2_ps.c | PES header integer underflows only affect file I/O lengths (bounded by EOF). No memory corruption. |
| SRT/Subtitle | load_text.c | 2048-byte buffers throughout. gf_utf8 conversions properly bounded. |
| SWF (shapes/fonts/text) | swf_parse.c | Bitstream reader returns 0 on EOF. Shape/font/text parsing bounded by tag size tracking. |

---

## Methodology
1. GPAC built from source (commit a310f1f) with `./configure --enable-sanitizer` for ASAN+UBSAN
2. Manual source code review of each parser's binary data handling
3. Focus on: file-controlled sizes/offsets driving allocations/memcpy, loop bounds,
   index arithmetic, integer overflow/underflow
4. Crafted input files targeting identified patterns
5. ASAN+UBSAN instrumented build for crash confirmation
6. Novelty check: verified VobSub not covered by OSS-Fuzz harnesses
7. Deep automated review of av_parsers.c (18K+ lines) covering VVC, SEI, AC3, EAC3,
   MP3, M4A, AV1, VP9, ProRes, and IAMF parsers

## Files
- `vobsub-oob-read/craft_vobsub_minimal.py` - Minimal VobSub OOB read reproducer
- `vobsub-oob-read/craft_vobsub_oob.py` - Multiple VobSub OOB test cases

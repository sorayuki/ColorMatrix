# Patch notes

## Upstream five-wide indexing defect

The source defines five luma coefficient sets in this order:

| Index | Matrix |
|---:|---|
| 0 | Rec.709 |
| 1 | FCC |
| 2 | Rec.601 / SMPTE 170M |
| 3 | SMPTE 240M |
| 4 | Rec.2020 |

`calc_coefficients()` generates every source/destination pair in row-major
order, so `yuv_convert` has a row width of five. ColorMatrix 2.6 retained the
old four-wide expression in explicit mode selection and old row offsets in
`findMode()`.

Requested `Rec.601 -> Rec.709` therefore selected:

- buggy index: `2 * 4 + 0 = 8`;
- actual entry 8: `FCC -> SMPTE 240M`;
- correct index: `2 * 5 + 0 = 10`.

The reporter independently reproduced the original ColorMatrix output by
requesting `FCC -> SMPTE 240M` in VapourSynth.

## Source corrections

- Added `YUV_CONVERSION_COUNT` and `MATRIX_MODE_INDEX`.
- Replaced explicit and hint/D2V index arithmetic with the helper.
- Remapped legacy SIMD modes to the corrected 5×5 numbering.
- Replaced the stale four-wide debug-name chain with a 25-entry table.
- Added defensive conversion-index bounds checking.
- Added the classic-x64/API3 interface-v6 source-build path.

## Hybrid-compatible binary patches

The exact Hybrid DLLs were generated from a separate port whose source is not
in the upstream archive. `tools/patch_hybrid_binaries.py` therefore applies a
narrow, hash-locked correction to those binaries.

### x86

The normal-path branch at RVA `0x4115` is redirected to the trampoline at
`0x4161`. The trampoline enters a code cave that computes
`destination + source * 5`, stores the mode, and returns to `0x4175`.

### x64

The normal-path branch at RVA `0x30E7` is redirected to the first instruction
of the corrected block at `0x3145`. The block calculates and stores
`source * 5 + destination`, then continues at `0x3160`.

The D2V/hint row offsets are likewise changed from `8/12/4` to `10/15/5`.
The exact Hybrid binaries contain the C conversion path, so no binary SIMD
changes are needed.

The reporter has runtime-tested both R2 Hybrid-compatible DLLs successfully.

## R3 x64 source build

The new x64 source build is compiled with
`COLORMATRIX_CLASSIC_API3_X64` and includes
`colormatrix/avisynth26_api3_x64.h` instead of the old interface-v3 header.
Its plugin entry point:

```cpp
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(
    IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;
    env->AddFunction(...);
    return 0;
}
```

All ColorMatrix-used helper objects call the classic host through the linkage
table. This is a full source-level ABI adaptation, not an added export around
old baked wrappers.

## R3 runtime status

All four distributed DLLs have now been runtime-confirmed by the reporter in
Selur Hybrid. In particular, the genuine x64 source build
`bin/source-build/x64/ColorMatrix64.dll` with SHA-256
`2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF`
loaded and completed the Rec.601-to-Rec.709 workflow with no crash and no
incorrect color shift. This confirms that the classic-x64/API3 source
adaptation is operational in the reported Windows 10 Pro 22H2 x64 environment.

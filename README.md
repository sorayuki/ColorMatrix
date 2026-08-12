# ColorMatrix 2.6.1 — corrected five-wide indexing (R3, runtime-confirmed)

This repository is a corrected derivative of [`sorayuki/ColorMatrix`](https://github.com/sorayuki/ColorMatrix).

## Fixed ColorMatrix defect

ColorMatrix 2.6 added Rec.2020 as a fifth luma-coefficient set, expanding the
flattened conversion table from 4×4 to 5×5. Several call sites continued to
index the table as if each row had four entries:

```cpp
source * 4 + dest
```

The corrected code uses the actual coefficient count:

```cpp
#define MATRIX_MODE_INDEX(source, dest) \
    ((source) * YUV_COEFFS_LUMA_COUNT + (dest))
```

For `Rec.601 -> Rec.709`, the correct entry is `2 * 5 + 0 = 10`. The old
expression selected entry 8, which is `FCC -> SMPTE 240M` in the 5×5 table.
The reporter independently reproduced the original visual error by requesting
that exact FCC-to-SMPTE-240M operation in VapourSynth.

The same five-wide correction is applied to explicit modes, D2V/hint modes,
diagnostic names, bounds checks, and the legacy SIMD dispatcher.

## R3: genuine classic-x64/API3 source build

R3 adds a newly compiled 64-bit source build for Hybrid's classic AviSynth
runtime. This is not a machine-code patch of Hybrid's DLL.

The two historical source archives supplied for investigation were useful for
confirming old x64 class layouts, but both expose the older interface version 3
and neither defines `AVS_Linkage` or `AvisynthPluginInit3`. They therefore are
not the exact SDK required by Hybrid.

The missing build interface was reconstructed from three compatible sources:

1. AviSynth 2.6's documented interface-v6/API3 linkage design.
2. The preserved classic-v6 `AVS_Linkage` prefix and plugin wrappers in
   maintained compatibility headers.
3. Static ABI observations from Hybrid's original working x64
   `ColorMatrix64.dll`, including its export, `AVS_Linkage` offsets, class
   sizes, `IClip::GetVersion()`, and `IScriptEnvironment` vtable calls.

The resulting minimal header is:

```text
colormatrix/avisynth26_api3_x64.h
```

It supplies a real `AvisynthPluginInit3`, stores the host linkage table, and
routes plugin-side `PClip`, `PVideoFrame`, `AVSValue`, `VideoInfo`, and
`VideoFrame` operations through that table. This is the substantive difference
from the withdrawn R1 x64 source build, which merely stored the API3 pointer
while continuing to use the old baked C++ ABI.

The resulting x64 source-built DLL, SHA-256
`2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF`,
was subsequently runtime-confirmed by the reporter in Selur Hybrid
2026.03.21.1 on Windows 10 Pro 22H2 x64 with an AMD Ryzen 9950X. It loaded and
completed the Rec.601-to-Rec.709 workflow without a crash and without the
erroneous FCC-to-SMPTE-240M color shift.

See `ABI_RESEARCH.md`, `VALIDATION.md`, and `RUNTIME_VALIDATION.md` for the
evidence, test status, and validation scope.

## Binary status

The repository and binary package contain four DLLs. All four have been
runtime-confirmed in the reported Hybrid workflow:

| Path | SHA-256 | Status |
|---|---|---|
| `bin/hybrid-compatible/x86/colormatrix.dll` | `52ECC494CD930298E5778B7AA8D6F241CB25F0CA8A3FF13D5F945B535C737D1C` | Runtime-tested in Hybrid; no crash and no color shift |
| `bin/hybrid-compatible/x64/ColorMatrix64.dll` | `1E87CE9D680C050ACA88AB3C0137D220579AE05C23919EB6311310EEFE0FC1C1` | Runtime-tested in Hybrid; no crash and no color shift |
| `bin/source-build/x86/colormatrix.dll` | `C8109AF3A1EF32BC6FBD9062F60EF56B2136E4E259BE442952F7056FDD125395` | Runtime-tested in Hybrid; no crash and no color shift |
| `bin/source-build/x64/ColorMatrix64.dll` | `2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF` | Runtime-tested in Hybrid; no crash and no color shift |

The `hybrid-compatible` DLLs are narrow, hash-locked patches of Hybrid's exact
original binaries. The `source-build` DLLs are newly linked from this source
tree. Runtime confirmation covers the reported system and conversion workflow;
it is not a guarantee for every classic AviSynth host or every possible filter
configuration.

Back up Hybrid's original DLL before replacement and verify the SHA-256 value
of the selected replacement.

## Build

```bash
bash build/build_portable_clang_cl.sh
```

The script requires `clang-cl` and `lld-link`, and produces both x86 and x64
source builds. It intentionally uses ColorMatrix's C conversion path so that a
correct source mode cannot be routed through stale legacy SIMD mode numbering.

## Tests

```bash
python tests/test_mode_index.py
python tests/test_patched_pe.py \
  --x86 bin/hybrid-compatible/x86/colormatrix.dll \
  --x64 bin/hybrid-compatible/x64/ColorMatrix64.dll
python tests/test_source_api3_x64.py \
  --x86 bin/source-build/x86/colormatrix.dll \
  --x64 bin/source-build/x64/ColorMatrix64.dll
```

## License

The upstream source is distributed under GPLv2; see `gpl.txt`. The bundled
classic API declarations retain the AviSynth header's plugin-linking exception.

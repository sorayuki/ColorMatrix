# Validation performed for R3

## Runtime-confirmed binaries

The reporter tested all four corrected DLLs in Selur Hybrid on Windows 10 Pro
22H2 x64. The reported x64 system used an AMD Ryzen 9950X. Each DLL completed
the Rec.601-to-Rec.709 workflow without a crash and without the erroneous color
shift:

| Binary | SHA-256 | Reporter result |
|---|---|---|
| `bin/hybrid-compatible/x86/colormatrix.dll` | `52ECC494CD930298E5778B7AA8D6F241CB25F0CA8A3FF13D5F945B535C737D1C` | Correct; no crash and no color shift |
| `bin/hybrid-compatible/x64/ColorMatrix64.dll` | `1E87CE9D680C050ACA88AB3C0137D220579AE05C23919EB6311310EEFE0FC1C1` | Correct; no crash and no color shift |
| `bin/source-build/x86/colormatrix.dll` | `C8109AF3A1EF32BC6FBD9062F60EF56B2136E4E259BE442952F7056FDD125395` | Correct; no crash and no color shift |
| `bin/source-build/x64/ColorMatrix64.dll` | `2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF` | Correct; no crash and no color shift |

The x64 source-build confirmation was recorded on 2026-08-09. It closes the
remaining R3 runtime-validation item for the reconstructed classic-x64/API3
interface.

The reporter also reproduced the original defect by explicitly converting FCC
to SMPTE 240M in VapourSynth, independently confirming the table-index
analysis.

## Source-level tests

- `tests/test_mode_index.py` passes.
- Explicit `Rec.601 -> Rec.709` resolves to index 10.
- Entry 8 remains `FCC -> SMPTE 240M`.
- Hint/D2V row offsets and legacy SIMD groups match the five-wide numbering.

## Hybrid-compatible PE tests

`tests/test_patched_pe.py` checks:

- exact output SHA-256 values;
- PE machine types;
- corrected incoming branch targets;
- x86 code-cave calculation and return target;
- x64 corrected instruction block;
- corrected D2V/hint row offsets.

Both Hybrid-compatible DLLs have additionally passed the reporter's runtime
validation.

## Source-build static validation

`tests/test_source_api3_x64.py` checks:

- x86 source-build hash remains byte-identical to the reporter-validated build;
- x64 source-build hash and PE32+ machine type;
- the x64 DLL exports only `AvisynthPluginInit3`;
- Init3 stores the host's second argument and calls `AddFunction` at vtable
  offset `0x30`;
- `IClip::GetVersion()` returns 6;
- the source header contains the expected linkage-offset and object-size
  assertions;
- the source uses a real `AVS_Linkage`-backed Init3 path.

The x64 source build was linked twice with `/Brepro`; both outputs had the same
SHA-256 value:

```text
2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF
```

The reporter's successful Hybrid test confirms that this statically verified
API3 path also works at runtime in the reported environment.

## Validation scope

The runtime results establish correct operation for the reported Selur Hybrid
workflow and test system. They do not exhaustively certify every conversion
mode, input format, classic AviSynth build, or host application. The source and
PE tests remain useful guards against regression, but future changes should be
retested on Windows.

# Reporter runtime validation

## Recorded status

Runtime confirmation was completed for all four R3 DLLs. The final x64 source
build confirmation was recorded on 2026-08-09.

Reported environment:

- Selur Hybrid 2026.03.21.1
- Windows 10 Pro 22H2 x64
- AMD Ryzen 9950X
- ColorMatrix Rec.601-to-Rec.709 workflow used throughout the investigation

## Results

| Binary | SHA-256 | Result |
|---|---|---|
| `bin/hybrid-compatible/x86/colormatrix.dll` | `52ECC494CD930298E5778B7AA8D6F241CB25F0CA8A3FF13D5F945B535C737D1C` | Works correctly in Hybrid; no crash and no color shift |
| `bin/hybrid-compatible/x64/ColorMatrix64.dll` | `1E87CE9D680C050ACA88AB3C0137D220579AE05C23919EB6311310EEFE0FC1C1` | Works correctly in Hybrid; no crash and no color shift |
| `bin/source-build/x86/colormatrix.dll` | `C8109AF3A1EF32BC6FBD9062F60EF56B2136E4E259BE442952F7056FDD125395` | Works correctly in Hybrid; no crash and no color shift |
| `bin/source-build/x64/ColorMatrix64.dll` | `2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF` | Works correctly in Hybrid; no crash and no color shift |

The x64 source-build result is significant because it confirms the reconstructed
classic interface-v6/API3 compatibility layer at runtime, rather than only by
static PE inspection. The corrected source build applies the intended
Rec.601-to-Rec.709 conversion instead of the old accidental FCC-to-SMPTE-240M
operation.

## Scope

These are reporter-supplied runtime results for the environment and workflow
above. They validate the distributed hashes in that context; they are not an
exhaustive certification of every ColorMatrix mode, input format, AviSynth
variant, or host program. Any rebuilt or modified DLL should be rehashed and
retested.

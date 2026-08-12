# Binary selection

All four binaries in this directory have been runtime-confirmed by the reporter
in Selur Hybrid for the Rec.601-to-Rec.709 workflow. Each completed without a
crash and without the erroneous FCC-to-SMPTE-240M color shift.

## Hybrid-compatible

These are ABI-preserving, hash-locked patches of Hybrid's exact original DLLs.
They retain the original Hybrid binary interfaces while correcting the
five-wide matrix index.

## Source-build

These are newly compiled from the corrected repository:

- `x86/colormatrix.dll` — reporter-validated in Hybrid.
- `x64/ColorMatrix64.dll` — genuine classic interface-v6/API3 source build,
  reporter-validated in Hybrid.

See `../RUNTIME_VALIDATION.md` for hashes, environment, results, and scope.
Always verify SHA-256 and back up the original plugin before replacement.

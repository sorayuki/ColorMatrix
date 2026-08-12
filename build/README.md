# Reproducible portable source build

`build_portable_clang_cl.sh` cross-compiles ColorMatrix with `clang-cl` and
`lld-link`.

## Outputs

```text
bin/source-build/x86/colormatrix.dll
bin/source-build/x64/ColorMatrix64.dll
```

Both outputs are exact reporter-validated source builds. The x64 output is a
genuine classic interface-v6/API3 build using
`colormatrix/avisynth26_api3_x64.h`; it has been confirmed to load and operate
correctly in Selur Hybrid 2026.03.21.1 on Windows 10 Pro 22H2 x64.

## Requirements

- `clang-cl`
- `lld-link`
- a POSIX shell environment for the build script

The script builds its small import libraries locally and does not require a
Windows SDK installation in the container.

## Command

```bash
bash build/build_portable_clang_cl.sh
```

Expected hashes for this repository revision:

```text
C8109AF3A1EF32BC6FBD9062F60EF56B2136E4E259BE442952F7056FDD125395  x86/colormatrix.dll
2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF  x64/ColorMatrix64.dll
```

## Build design

Both targets use ColorMatrix's C conversion path. The x86 target retains the
source and flags that produced the validated DLL. The x64 target additionally
defines:

```text
COLORMATRIX_CLASSIC_API3_X64
AVISYNTH64
```

This selects the linkage-backed classic x64 header and exports only
`AvisynthPluginInit3`.

The build uses `/Brepro`. Rebuilding without source/toolchain changes should
produce the same output hashes in the same environment. Runtime confirmation
of the supplied outputs does not remove the need to retest after source,
compiler, linker, or compatibility-header changes.

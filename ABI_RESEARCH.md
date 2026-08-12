# Classic x64/API3 ABI research

## Goal

Build ColorMatrix from corrected source for the classic 64-bit AviSynth runtime
bundled with Selur Hybrid, rather than modifying Hybrid's existing binary.

## User-supplied historical archives

The following attached archives were inspected:

| Archive | SHA-256 | Relevant finding |
|---|---|---|
| `avisynth64_JoshyD.zip` | `CCA6A935D1964A8847517035CE677FF302A8F58277139A7EA15401E323654D71` | Historical x64 port; `AVISYNTH_INTERFACE_VERSION = 3`; direct/baked C++ plugin ABI; no `AVS_Linkage` or `AvisynthPluginInit3` |
| `avisynth-mt.zip` | `FC27DA257A2E599B17CA807FB61FB55ABC6D42B095EBCF41C43B6729C785F770` | AviSynth 2.5.8 MT tree; interface version 3; same pre-API3 model |

They are useful historical references, but neither is the exact interface used
by Hybrid's API3 x64 plugins.

## Classic AviSynth 2.6 evidence

AviSynth 2.6 introduced the linkage-facing API entry point, bumped the classic
interface to 6, and changed memory-size fields for 64-bit-safe use. Maintained
compatibility headers preserve the classic-v6 linkage prefix and explicitly
route plugin-side C++ wrappers through `AVS_Linkage`.

The R3 header includes only the API surface ColorMatrix needs. It does not claim
to be a complete replacement SDK.

## ABI observations from Hybrid's original x64 DLL

Original Hybrid binary:

```text
SHA-256 654454449F992122BFAE5671003491DB67212EE6B15FE0975F81B8B47223FEDF
```

Static inspection established:

- PE32+ x86-64 DLL.
- Sole plugin export: `AvisynthPluginInit3`.
- The Init3 second argument is stored as the global linkage pointer.
- `IScriptEnvironment::AddFunction` is invoked through vtable offset `0x30`.
- The linkage `Size` field is read as a 32-bit integer.
- Natural x64 alignment places the first linkage function pointer at `0x08`.
- `IClip::GetVersion()` returns 6.

Observed classic-prefix linkage offsets used by ColorMatrix:

| Entry | Offset |
|---|---:|
| `PClip_OPERATOR_ASSIGN1` | `0x1E8` |
| `PVideoFrame_DESTRUCTOR` | `0x220` |
| `AVSValue_DESTRUCTOR` | `0x278` |

Observed object sizes:

| Type | x64 size |
|---|---:|
| `VideoInfo` | 48 |
| `VideoFrameBuffer` | 24 |
| `VideoFrame` | 72 |
| `IClip` | 16 |
| `PClip` | 8 |
| `PVideoFrame` | 8 |
| `AVSValue` | 16 |

These values are asserted at compile time in
`colormatrix/avisynth26_api3_x64.h`.

## Why the old x64 source build crashed

The withdrawn x64 build exported a compatibility `AvisynthPluginInit3` and
saved its linkage pointer, but all C++ helper operations were still compiled
from the old interface-v3 header. Objects such as `PClip`, `PVideoFrame`, and
`AVSValue` consequently used the wrong baked ABI instead of host-supplied
linkage functions.

R3 changes the entire x64 plugin-facing layer:

- real interface-v6 `AvisynthPluginInit3`;
- global `AVS_linkage` initialized from the host;
- linkage-backed constructors, destructors, assignments, and accessors;
- x64 class layouts and linkage offsets guarded by `static_assert`;
- `IClip::GetVersion()` compiled as 6;
- the original `IScriptEnvironment` virtual interface order retained.

## Deliberate scope

ColorMatrix does not use `VideoFrameBuffer::GetDataSize()` or
`VideoFrame::GetOffset()` directly. Historical headers disagree on the exact
spelling/type of those two memory-size declarations across branches; R3 uses
the 64-bit-safe layout observed in Hybrid's binary and the 2.6 release history.
This does not affect ColorMatrix's executed call set.

## Runtime confirmation

The source-built x64 DLL produced by this interface layer has SHA-256
`2291DDA6A7FEE1D167F79D8846DB19BF4E974A22D795CB01D7BBF0B9764008EF`.
The reporter subsequently confirmed that it works correctly in Selur Hybrid
2026.03.21.1 on Windows 10 Pro 22H2 x64 with an AMD Ryzen 9950X: it loaded,
completed the Rec.601-to-Rec.709 workflow without crashing, and did not produce
the erroneous FCC-to-SMPTE-240M color shift.

That result validates the reconstructed interface-v6/API3 linkage path for the
reported Hybrid runtime. It does not imply exhaustive compatibility with every
classic AviSynth host or every unused declaration in this intentionally minimal
header.

## Reference locations

These references were used only to recover the classic interface contract;
the R3 DLL does **not** depend on AviSynth+ at runtime.

- AviSynth 2.6.0 release files and readme:
  `https://sourceforge.net/projects/avisynth2/files/AviSynth%202.6/AviSynth%202.6.0/`
- Maintained AviSynth header preserving the classic-v6 prefix:
  `https://github.com/AviSynth/AviSynthPlus/blob/master/avs_core/include/avisynth.h`
- Independent classic-interface compatibility copy used by VapourSynth:
  `https://github.com/vapoursynth/vapoursynth/blob/master/src/avisynth/avisynth.h`

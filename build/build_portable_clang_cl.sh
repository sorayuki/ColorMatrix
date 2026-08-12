#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$ROOT/build/generated"
OUT_DIR="$ROOT/bin/source-build"

for tool in clang-cl lld-link; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "error: $tool is required" >&2
    exit 1
  }
done

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"/{imports/x86,imports/x64,obj/x86,obj/x64} "$OUT_DIR"/{x86,x64}

cat > "$BUILD_DIR/imports/x86/kernel32.def" <<'EOF'
LIBRARY KERNEL32.dll
EXPORTS
  InterlockedIncrement
  InterlockedDecrement
  GetCurrentThreadId
  OutputDebugStringA
  CreateEventA
  CreateThread
  lstrcmpiA
  ResetEvent
  SetEvent
  WaitForSingleObject
  WaitForMultipleObjects
  CloseHandle
  GetCurrentProcess
  GetProcessAffinityMask
EOF

cat > "$BUILD_DIR/imports/x64/kernel32.def" <<'EOF'
LIBRARY KERNEL32.dll
EXPORTS
  InterlockedIncrement
  InterlockedDecrement
  GetCurrentThreadId
  OutputDebugStringA
  CreateEventA
  CreateThread
  lstrcmpiA
  ResetEvent
  SetEvent
  WaitForSingleObject
  WaitForMultipleObjects
  CloseHandle
  GetCurrentProcess
  GetProcessAffinityMask
EOF

cat > "$BUILD_DIR/imports/x86/msvcrt.def" <<'EOF'
LIBRARY MSVCRT.dll
EXPORTS
  sprintf
  malloc
  calloc
  fopen
  sscanf
  fgets
  strncmp
  memset
  memcpy
  fclose
  free
  abs
EOF
cp "$BUILD_DIR/imports/x86/msvcrt.def" "$BUILD_DIR/imports/x64/msvcrt.def"

lld-link /lib /machine:x86 /def:"$BUILD_DIR/imports/x86/kernel32.def" /out:"$BUILD_DIR/imports/x86/kernel32.lib"
lld-link /lib /machine:x86 /def:"$BUILD_DIR/imports/x86/msvcrt.def" /out:"$BUILD_DIR/imports/x86/msvcrt.lib"
lld-link /lib /machine:x64 /def:"$BUILD_DIR/imports/x64/kernel32.def" /out:"$BUILD_DIR/imports/x64/kernel32.lib"
lld-link /lib /machine:x64 /def:"$BUILD_DIR/imports/x64/msvcrt.def" /out:"$BUILD_DIR/imports/x64/msvcrt.lib"

COMMON=(
  /nologo /c /O2 /Oi- /GS- /GR- /EHs-c- /Zl /Zc:threadSafeInit- /Brepro
  /DWIN32 /DNDEBUG /D_WINDOWS /D_USRDLL /DCOLORMATRIX_EXPORTS
  /DCOLORMATRIX_C_ONLY /DCOLORMATRIX_PORTABLE_BUILD /DCOLORMATRIX_NO_CPP_EXCEPTIONS
  /I"$ROOT/build/compat" /I"$ROOT/colormatrix"
  /clang:-fno-builtin
  /clang:-Wno-microsoft-extra-qualification
  /clang:-Wno-ignored-attributes
)

build_arch() {
  local arch=$1 target=$2 machine=$3 entry=$4 output=$5
  local obj="$BUILD_DIR/obj/$arch"
  local imports="$BUILD_DIR/imports/$arch"
  local arch_defines=()
  if [[ "$arch" == "x64" ]]; then
    arch_defines+=(/DCOLORMATRIX_CLASSIC_API3_X64 /DAVISYNTH64)
  fi

  clang-cl --target="$target" "${COMMON[@]}" "${arch_defines[@]}" \
    /Fo"$obj/ColorMatrix.obj" "$ROOT/colormatrix/ColorMatrix.cpp"
  clang-cl --target="$target" "${COMMON[@]}" "${arch_defines[@]}" \
    /Fo"$obj/portable_runtime.obj" "$ROOT/colormatrix/portable_runtime.cpp"

  local aliases=()
  if [[ "$arch" == "x86" ]]; then
    # clang-cl decorates 32-bit stdcall dllimport symbols, while Windows PE
    # import names are undecorated. Resolve local decorated references to the
    # correct undecorated import-library symbols without changing the DLL's
    # actual import table names.
    aliases=(
      '/alternatename:__imp__InterlockedIncrement@4=__imp__InterlockedIncrement'
      '/alternatename:__imp__InterlockedDecrement@4=__imp__InterlockedDecrement'
      '/alternatename:__imp__GetCurrentThreadId@0=__imp__GetCurrentThreadId'
      '/alternatename:__imp__OutputDebugStringA@4=__imp__OutputDebugStringA'
      '/alternatename:__imp__CreateEventA@16=__imp__CreateEventA'
      '/alternatename:__imp__CreateThread@24=__imp__CreateThread'
      '/alternatename:__imp__lstrcmpiA@8=__imp__lstrcmpiA'
      '/alternatename:__imp__ResetEvent@4=__imp__ResetEvent'
      '/alternatename:__imp__SetEvent@4=__imp__SetEvent'
      '/alternatename:__imp__WaitForSingleObject@8=__imp__WaitForSingleObject'
      '/alternatename:__imp__WaitForMultipleObjects@16=__imp__WaitForMultipleObjects'
      '/alternatename:__imp__CloseHandle@4=__imp__CloseHandle'
      '/alternatename:__imp__GetCurrentProcess@0=__imp__GetCurrentProcess'
      '/alternatename:__imp__GetProcessAffinityMask@12=__imp__GetProcessAffinityMask'
    )
  fi

  lld-link /dll /nodefaultlib /Brepro /opt:ref /opt:icf \
    /machine:"$machine" /subsystem:windows /entry:"$entry" \
    /out:"$output" /implib:"$obj/ColorMatrix.lib" \
    "${aliases[@]}" \
    "$obj/ColorMatrix.obj" "$obj/portable_runtime.obj" \
    "$imports/kernel32.lib" "$imports/msvcrt.lib"
}

build_arch x86 i686-pc-windows-msvc x86 'DllMain@12' "$OUT_DIR/x86/colormatrix.dll"
sha256sum "$OUT_DIR/x86/colormatrix.dll"

build_arch x64 x86_64-pc-windows-msvc x64 DllMain "$OUT_DIR/x64/ColorMatrix64.dll"
sha256sum "$OUT_DIR/x64/ColorMatrix64.dll"

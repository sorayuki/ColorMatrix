#!/usr/bin/env python3
"""Static checks for the source-built x86 and classic API3 x64 DLLs.

Both expected hashes have separately passed reporter runtime validation in
Selur Hybrid; these checks guard the source/PE properties that produced them.
"""
from __future__ import annotations

import argparse
import hashlib
import re
import struct
from dataclasses import dataclass
from pathlib import Path

EXPECTED_X86 = "c8109af3a1ef32bc6fbd9062f60ef56b2136e4e259be442952f7056fdd125395"
EXPECTED_X64 = "2291dda6a7fee1d167f79d8846db19bf4e974a22d795cb01d7bbf0b9764008ef"


@dataclass(frozen=True)
class Section:
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int


class PE:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.data = path.read_bytes()
        assert self.data[:2] == b"MZ"
        self.pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        assert self.data[self.pe:self.pe + 4] == b"PE\0\0"
        self.machine, count = struct.unpack_from("<HH", self.data, self.pe + 4)
        optional_size = struct.unpack_from("<H", self.data, self.pe + 20)[0]
        self.optional = self.pe + 24
        self.magic = struct.unpack_from("<H", self.data, self.optional)[0]
        directory_base = self.optional + (112 if self.magic == 0x20B else 96)
        self.export_rva, self.export_size = struct.unpack_from("<II", self.data, directory_base)
        section_table = self.optional + optional_size
        self.sections: list[Section] = []
        for i in range(count):
            off = section_table + i * 40
            vsize, va, rsize, roff = struct.unpack_from("<IIII", self.data, off + 8)
            self.sections.append(Section(va, vsize, roff, rsize))

    def offset(self, rva: int) -> int:
        for s in self.sections:
            if s.virtual_address <= rva < s.virtual_address + max(s.virtual_size, s.raw_size):
                return s.raw_offset + rva - s.virtual_address
        raise AssertionError(f"unmapped RVA 0x{rva:X} in {self.path}")

    def at(self, rva: int, size: int) -> bytes:
        off = self.offset(rva)
        return self.data[off:off + size]

    def cstring(self, rva: int) -> str:
        off = self.offset(rva)
        end = self.data.index(0, off)
        return self.data[off:end].decode("ascii")

    def exports(self) -> dict[str, int]:
        off = self.offset(self.export_rva)
        (_flags, _time, _major, _minor, _name, ordinal_base,
         function_count, name_count, functions_rva, names_rva,
         ordinals_rva) = struct.unpack_from("<IIHHIIIIIII", self.data, off)
        functions_off = self.offset(functions_rva)
        names_off = self.offset(names_rva)
        ordinals_off = self.offset(ordinals_rva)
        result: dict[str, int] = {}
        for i in range(name_count):
            name_rva = struct.unpack_from("<I", self.data, names_off + 4 * i)[0]
            ordinal_index = struct.unpack_from("<H", self.data, ordinals_off + 2 * i)[0]
            assert ordinal_index < function_count
            function_rva = struct.unpack_from("<I", self.data, functions_off + 4 * ordinal_index)[0]
            result[self.cstring(name_rva)] = function_rva
        assert ordinal_base == 1
        return result


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_source_tree(root: Path) -> None:
    header = (root / "colormatrix" / "avisynth26_api3_x64.h").read_text(encoding="utf-8")
    source = (root / "colormatrix" / "ColorMatrix.cpp").read_text(encoding="utf-8")
    build = (root / "build" / "build_portable_clang_cl.sh").read_text(encoding="utf-8")

    required_header = [
        "AVISYNTH_INTERFACE_VERSION = 6",
        "struct AVS_Linkage",
        "int Size;",
        "offsetof(AVS_Linkage, PClip_OPERATOR_ASSIGN1) == 0x1E8",
        "offsetof(AVS_Linkage, PVideoFrame_DESTRUCTOR) == 0x220",
        "offsetof(AVS_Linkage, AVSValue_DESTRUCTOR) == 0x278",
        "sizeof(VideoInfo) == 48",
        "sizeof(VideoFrame) == 72",
        "sizeof(IClip) == 16",
        "sizeof(AVSValue) == 16",
    ]
    for text in required_header:
        assert text in header, text

    assert re.search(
        r"AvisynthPluginInit3\s*\(\s*IScriptEnvironment\* env,\s*"
        r"const AVS_Linkage\* const vectors\s*\)", source
    )
    assert "AVS_linkage = vectors;" in source
    assert "/DCOLORMATRIX_CLASSIC_API3_X64" in build
    assert "/DAVISYNTH64" in build


def check_x86(path: Path) -> None:
    pe = PE(path)
    assert sha256(path) == EXPECTED_X86
    assert pe.machine == 0x014C
    assert pe.magic == 0x10B


def check_x64(path: Path) -> None:
    pe = PE(path)
    assert sha256(path) == EXPECTED_X64
    assert pe.machine == 0x8664
    assert pe.magic == 0x20B

    exports = pe.exports()
    assert exports.keys() == {"AvisynthPluginInit3"}, exports
    init_rva = exports["AvisynthPluginInit3"]
    init = pe.at(init_rva, 0x40)

    # Prologue, store RDX (the linkage argument), and call AddFunction through
    # IScriptEnvironment's classic vtable slot at +0x30.
    assert init.startswith(bytes.fromhex("48 83 ec 28 48 89 15"))
    assert bytes.fromhex("ff 50 30") in init
    assert bytes.fromhex("31 c0 48 83 c4 28 c3") in init

    # The source-built IClip implementation advertises classic interface 6.
    get_version = bytes.fromhex("b8 06 00 00 00 c3")
    assert get_version in pe.data

    # Linkage-backed wrappers use the exact classic-prefix offsets observed in
    # Hybrid's original x64 ColorMatrix binary.
    for displacement in (0x1E8, 0x220, 0x278):
        needle = struct.pack("<I", displacement)
        assert needle in pe.data, f"missing linkage displacement 0x{displacement:X}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--x86", type=Path, required=True)
    parser.add_argument("--x64", type=Path, required=True)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    check_source_tree(args.root)
    check_x86(args.x86)
    check_x64(args.x64)
    print("R3 source-build API3 tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Static checks for the R2 Hybrid-compatible patched DLLs."""
from __future__ import annotations

import argparse
import hashlib
import struct
from dataclasses import dataclass
from pathlib import Path

EXPECTED = {
    "x86": "52ecc494cd930298e5778b7aa8d6f241cb25f0ca8a3ff13d5f945b535c737d1c",
    "x64": "1e87ce9d680c050aca88ab3c0137d220579ae05c23919eb6311310eefe0fc1c1",
}


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int


class PE:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.data = path.read_bytes()
        assert self.data[:2] == b"MZ"
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        assert self.data[pe:pe + 4] == b"PE\0\0"
        self.machine, count = struct.unpack_from("<HH", self.data, pe + 4)
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        table = pe + 24 + optional_size
        sections: list[Section] = []
        for i in range(count):
            off = table + 40 * i
            name = self.data[off:off + 8].rstrip(b"\0").decode("ascii", "replace")
            vsize, va, rsize, roff = struct.unpack_from("<IIII", self.data, off + 8)
            sections.append(Section(name, va, vsize, roff, rsize))
        self.sections = sections

    def section(self, name: str) -> Section:
        return next(s for s in self.sections if s.name == name)

    def offset(self, rva: int) -> int:
        for s in self.sections:
            if s.virtual_address <= rva < s.virtual_address + max(s.virtual_size, s.raw_size):
                return s.raw_offset + rva - s.virtual_address
        raise AssertionError(f"RVA 0x{rva:X} is unmapped in {self.path}")

    def at(self, rva: int, size: int) -> bytes:
        off = self.offset(rva)
        return self.data[off:off + size]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def short_jcc_target(pe: PE, rva: int, opcode: int = 0x75) -> int:
    insn = pe.at(rva, 2)
    assert insn[0] == opcode, insn.hex(" ")
    displacement = struct.unpack("<b", insn[1:])[0]
    return rva + 2 + displacement


def rel32_target(pe: PE, rva: int, opcode: int = 0xE9) -> int:
    insn = pe.at(rva, 5)
    assert insn[0] == opcode, insn.hex(" ")
    displacement = struct.unpack("<i", insn[1:])[0]
    return rva + 5 + displacement


def check_x86(path: Path) -> None:
    pe = PE(path)
    assert sha(path) == EXPECTED["x86"]
    assert pe.machine == 0x014C

    # R2 fix: normal source!=dest path must enter the trampoline, not bypass it.
    assert pe.at(0x4115, 2) == bytes.fromhex("75 4a")
    assert short_jcc_target(pe, 0x4115) == 0x4161
    assert rel32_target(pe, 0x4161) == 0x11200

    cave = bytes.fromhex(
        "39 c2 74 07 "          # cmp edx,eax / je equal
        "8d 04 90 "             # lea eax,[eax+edx*4]
        "01 d0 "                # add eax,edx => dest + source*5
        "eb 05 "
        "b8 fe ff ff ff "
        "89 81 e0 04 00 00 "
        "e9 5a 2f ff ff"
    )
    assert pe.at(0x11200, len(cave)) == cave
    assert rel32_target(pe, 0x11216) == 0x4175
    assert pe.section(".text").virtual_size >= 0x1021B

    for rva in (0x125F, 0x22C2):
        assert pe.at(rva, 4) == bytes.fromhex("83 45 ec 0a")
    for rva in (0x1D77, 0x23F7):
        assert pe.at(rva, 4) == bytes.fromhex("83 45 ec 0f")
    for rva in (0x1D95, 0x2415):
        assert pe.at(rva, 4) == bytes.fromhex("83 45 ec 05")


def check_x64(path: Path) -> None:
    pe = PE(path)
    assert sha(path) == EXPECTED["x64"]
    assert pe.machine == 0x8664

    # R2 fix: normal source!=dest path must enter at the first instruction.
    assert pe.at(0x30E7, 2) == bytes.fromhex("75 5c")
    assert short_jcc_target(pe, 0x30E7) == 0x3145

    block = bytes.fromhex(
        "39 d0 74 07 "
        "6b c0 05 "             # source *= 5
        "01 d0 "                # + dest
        "eb 05 "
        "b8 fe ff ff ff "
        "41 89 86 f8 04 00 00 "
        "90 90 90 90"
    )
    assert pe.at(0x3145, len(block)) == block
    assert pe.at(0x3155, 7) == bytes.fromhex("41 89 86 f8 04 00 00")

    assert pe.at(0x727F, 3) == bytes.fromhex("83 c0 0a")
    assert pe.at(0x72AF, 3) == bytes.fromhex("83 c0 0f")
    assert pe.at(0x72BE, 3) == bytes.fromhex("83 c0 05")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--x86", type=Path, required=True)
    ap.add_argument("--x64", type=Path, required=True)
    args = ap.parse_args()
    check_x86(args.x86)
    check_x64(args.x64)
    print("R2 patched PE tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

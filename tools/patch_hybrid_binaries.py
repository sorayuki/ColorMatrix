#!/usr/bin/env python3
"""Patch Hybrid's supplied ColorMatrix 2.6 DLLs for five-wide matrix indexing.

This script only accepts the exact x86/x64 DLLs analyzed for this release.  It
verifies SHA-256 hashes and every instruction sequence before changing bytes.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
from dataclasses import dataclass
from pathlib import Path

X86_SHA256 = "3d042d59df206cd45586348c72d2458dee6e5dd2fc5c8540916245da1a7c8ec2"
X64_SHA256 = "654454449f992122bfae5671003491db67212ee6b15fe0975f81b8b47223fedf"
X86_PATCHED_SHA256 = "52ecc494cd930298e5778b7aa8d6f241cb25f0ca8a3ff13d5f945b535c737d1c"
X64_PATCHED_SHA256 = "1e87ce9d680c050aca88ab3c0137d220579ae05c23919eb6311310eefe0fc1c1"


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int
    header_offset: int


class PEImage:
    def __init__(self, data: bytearray) -> None:
        self.data = data
        if data[:2] != b"MZ":
            raise ValueError("not an MZ executable")
        self.pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if data[self.pe_offset:self.pe_offset + 4] != b"PE\0\0":
            raise ValueError("not a PE image")
        coff = self.pe_offset + 4
        self.machine, section_count = struct.unpack_from("<HH", data, coff)
        optional_size = struct.unpack_from("<H", data, coff + 16)[0]
        section_table = self.pe_offset + 24 + optional_size
        sections: list[Section] = []
        for i in range(section_count):
            off = section_table + 40 * i
            name = bytes(data[off:off + 8]).rstrip(b"\0").decode("ascii", "replace")
            virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, off + 8)
            sections.append(Section(name, virtual_address, virtual_size, raw_offset, raw_size, off))
        self.sections = sections

    def section(self, name: str) -> Section:
        for section in self.sections:
            if section.name == name:
                return section
        raise ValueError(f"PE section {name!r} not found")

    def rva_to_offset(self, rva: int) -> int:
        for section in self.sections:
            span = max(section.virtual_size, section.raw_size)
            if section.virtual_address <= rva < section.virtual_address + span:
                return section.raw_offset + (rva - section.virtual_address)
        raise ValueError(f"RVA 0x{rva:X} does not map to a section")

    def expect_rva(self, rva: int, expected: bytes) -> int:
        off = self.rva_to_offset(rva)
        actual = bytes(self.data[off:off + len(expected)])
        if actual != expected:
            raise ValueError(
                f"unexpected bytes at RVA 0x{rva:X}:\n"
                f"  expected {expected.hex(' ')}\n"
                f"  actual   {actual.hex(' ')}"
            )
        return off

    def patch_rva(self, rva: int, expected: bytes, replacement: bytes) -> None:
        if len(expected) != len(replacement):
            raise ValueError("in-place patch length mismatch")
        off = self.expect_rva(rva, expected)
        self.data[off:off + len(replacement)] = replacement


def sha256(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()


def rel32(instruction_rva: int, target_rva: int) -> bytes:
    displacement = target_rva - (instruction_rva + 5)
    if not -(1 << 31) <= displacement < (1 << 31):
        raise ValueError("relative jump is out of range")
    return struct.pack("<i", displacement)


def patch_x64(data: bytearray) -> bytearray:
    if sha256(data) != X64_SHA256:
        raise ValueError("x64 input SHA-256 does not match the supported Hybrid ColorMatrix64.dll")
    pe = PEImage(data)
    if pe.machine != 0x8664:
        raise ValueError("x64 input is not an AMD64 PE image")

    # Redirect the earlier source!=dest branch into the rewritten constructor block.
    # It originally jumped directly to the old LEA at RVA 0x3156, which is now
    # inside the replacement sequence.
    pe.patch_rva(0x30E7, bytes.fromhex("75 6d"), bytes.fromhex("75 5c"))

    # Constructor: source == dest ? -2 : source * 5 + dest.
    original = bytes.fromhex(
        "3b c2 75 0d 41 c7 86 f8 04 00 00 fe ff ff ff "
        "eb 0a 8d 04 82 41 89 86 f8 04 00 00"
    )
    fixed = bytes.fromhex(
        "39 d0 74 07 "          # cmp eax,edx / je equal
        "6b c0 05 "             # imul eax,eax,5
        "01 d0 "                # add eax,edx
        "eb 05 "                # jmp store
        "b8 fe ff ff ff "       # equal: mov eax,-2
        "41 89 86 f8 04 00 00 " # store: mov [r14+0x4f8],eax
        "90 90 90 90"
    )
    pe.patch_rva(0x3145, original, fixed)

    # findMode(): old four-wide row offsets -> five-wide row offsets.
    pe.patch_rva(0x727F, bytes.fromhex("83 c0 08"), bytes.fromhex("83 c0 0a"))
    pe.patch_rva(0x72AF, bytes.fromhex("83 c0 0c"), bytes.fromhex("83 c0 0f"))
    pe.patch_rva(0x72BE, bytes.fromhex("83 c0 04"), bytes.fromhex("83 c0 05"))
    return data


def patch_x86(data: bytearray) -> bytearray:
    if sha256(data) != X86_SHA256:
        raise ValueError("x86 input SHA-256 does not match the supported Hybrid colormatrix.dll")
    pe = PEImage(data)
    if pe.machine != 0x014C:
        raise ValueError("x86 input is not an i386 PE image")

    # Redirect the earlier source!=dest branch to the trampoline at 0x4161.
    # It originally jumped to the old LEA at 0x416C, bypassing the trampoline.
    pe.patch_rva(0x4115, bytes.fromhex("75 55"), bytes.fromhex("75 4a"))

    constructor_rva = 0x4161
    return_rva = 0x4175
    cave_rva = 0x11200

    original = bytes.fromhex(
        "3b d0 75 07 b8 fe ff ff ff eb 03 8d 04 90 "
        "89 81 e0 04 00 00"
    )
    constructor_patch = b"\xE9" + rel32(constructor_rva, cave_rva) + b"\x90" * (len(original) - 5)
    pe.patch_rva(constructor_rva, original, constructor_patch)

    cave = (
        bytes.fromhex(
            "39 c2 74 07 "          # cmp edx,eax / je equal
            "8d 04 90 "             # lea eax,[eax+edx*4]
            "01 d0 "                # add eax,edx (source*5 + dest)
            "eb 05 "                # jmp store
            "b8 fe ff ff ff "       # equal: mov eax,-2
            "89 81 e0 04 00 00 "    # store: mov [ecx+0x4e0],eax
            "e9"
        )
        + rel32(cave_rva + 22, return_rva)
    )
    cave_off = pe.expect_rva(cave_rva, b"\0" * len(cave))
    pe.data[cave_off:cave_off + len(cave)] = cave

    # Extend .text's mapped virtual size to include the code cave. The raw
    # section already contains sufficient zero-filled space.
    text = pe.section(".text")
    required_virtual_size = cave_rva + len(cave) - text.virtual_address
    if required_virtual_size > text.raw_size:
        raise ValueError("x86 code cave exceeds .text raw section")
    if text.virtual_size != 0x101E6:
        raise ValueError(f"unexpected original .text VirtualSize 0x{text.virtual_size:X}")
    struct.pack_into("<I", pe.data, text.header_offset + 8, required_virtual_size)

    # findMode() is inlined twice in this build.
    for rva in (0x125F, 0x22C2):
        pe.patch_rva(rva, bytes.fromhex("83 45 ec 08"), bytes.fromhex("83 45 ec 0a"))
    for rva in (0x1D77, 0x23F7):
        pe.patch_rva(rva, bytes.fromhex("83 45 ec 0c"), bytes.fromhex("83 45 ec 0f"))
    for rva in (0x1D95, 0x2415):
        pe.patch_rva(rva, bytes.fromhex("83 45 ec 04"), bytes.fromhex("83 45 ec 05"))
    return data


def patch_file(kind: str, source: Path, destination: Path) -> None:
    data = bytearray(source.read_bytes())
    patched = patch_x86(data) if kind == "x86" else patch_x64(data)
    actual_hash = sha256(patched)
    expected_hash = X86_PATCHED_SHA256 if kind == "x86" else X64_PATCHED_SHA256
    if actual_hash != expected_hash:
        raise ValueError(
            f"internal error: {kind} patched SHA-256 mismatch: "
            f"expected {expected_hash}, got {actual_hash}"
        )
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(patched)
    print(f"{kind}: {source.name} -> {destination}")
    print(f"  SHA-256: {actual_hash}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--x86", type=Path, help="original Hybrid colormatrix.dll")
    parser.add_argument("--x64", type=Path, help="original Hybrid ColorMatrix64.dll")
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    if not args.x86 and not args.x64:
        parser.error("at least one of --x86 or --x64 is required")
    if args.x86:
        patch_file("x86", args.x86, args.out_dir / "x86" / "colormatrix.dll")
    if args.x64:
        patch_file("x64", args.x64, args.out_dir / "x64" / "ColorMatrix64.dll")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

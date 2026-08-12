#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "colormatrix" / "ColorMatrix.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "colormatrix" / "ColorMatrix.h").read_text(encoding="utf-8")


def mode(source: int, dest: int) -> int:
    return source * 5 + dest


def main() -> int:
    indices = [mode(source, dest) for source in range(5) for dest in range(5)]
    assert indices == list(range(25))
    assert mode(2, 0) == 10  # Rec.601 -> Rec.709
    assert mode(1, 3) == 8   # FCC -> SMPTE 240M

    assert "MATRIX_MODE_INDEX(source, dest)" in CPP
    assert "source*4+dest" not in CPP.replace(" ", "")
    assert "return 4+dest" not in CPP.replace(" ", "")
    assert "return 8+dest" not in CPP.replace(" ", "")
    assert "return 12+dest" not in CPP.replace(" ", "")
    assert "YUV_CONVERSION_COUNT" in HEADER

    # Verify the four legacy SIMD kernels were remapped to five-wide indices.
    expected_groups = [
        {1, 2, 3, 16, 17},
        {5, 8, 10, 13, 15},
        {7},
        {11},
    ]
    body = CPP[CPP.index("void (*find_YV12_SIMD"):CPP.index("int ColorMatrix::parseD2V")]
    numbers = [set(map(int, re.findall(r"modef == (\d+)", block)))
               for block in re.split(r"else if", body)[:4]]
    assert numbers == expected_groups, numbers

    print("source indexing tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

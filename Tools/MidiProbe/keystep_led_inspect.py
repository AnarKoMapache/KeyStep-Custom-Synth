#!/usr/bin/env python3
"""
Inspect Arturia KeyStep .led firmware container.

This does not modify the file.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import struct
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("led", type=Path)
    args = parser.parse_args()

    text = args.led.read_bytes().strip()
    compact = re.sub(rb"\s+", b"", text)

    print(f"Archivo: {args.led}")
    print(f"Tamaño texto: {len(text)} bytes")

    if not re.fullmatch(rb"[0-9a-fA-F]+", compact) or len(compact) % 2:
        print("No parece un .led en ASCII hex simple.")
        return

    raw = bytes.fromhex(compact.decode("ascii"))
    print(f"Tamaño decodificado: {len(raw)} bytes")
    print(f"SHA256 raw: {hashlib.sha256(raw).hexdigest()}")
    print(f"Primeros 32 bytes raw: {raw[:32].hex(' ')}")

    print("\nProbando posibles offsets de vector table:")
    for off in range(0, 40, 2):
        if off + 8 > len(raw):
            continue
        sp, pc = struct.unpack_from("<II", raw, off)
        ok_sp = 0x20000000 <= sp <= 0x20010000
        ok_pc = 0x08000000 <= pc <= 0x08080000 and pc & 1
        marker = " <-- probable app start" if ok_sp and ok_pc else ""
        print(f"  offset {off:02d}: SP=0x{sp:08X} PC=0x{pc:08X}{marker}")

    off = 18
    if off + 8 <= len(raw):
        sp, pc = struct.unpack_from("<II", raw, off)
        print("\nHipótesis actual:")
        print(f"  header Arturia: {off} bytes")
        print(f"  app raw size:   {len(raw) - off} bytes")
        print(f"  vector SP:      0x{sp:08X}")
        print(f"  reset handler:  0x{pc:08X}")
        print("  base app probable: 0x08004000")


if __name__ == "__main__":
    main()

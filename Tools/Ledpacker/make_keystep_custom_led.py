#!/usr/bin/env python3
"""
make_keystep_custom_led.py

Empaca un app.bin STM32F103 compilado para 0x08004000 dentro de un .led
compatible con Arturia KeyStep MK1, usando el .led oficial como plantilla.

Uso:
    python3 make_keystep_custom_led.py oficial.led app.bin salida_custom.led

Importante:
- app.bin debe estar linkeado a 0x08004000.
- app.bin debe caber dentro del tamaño de firmware que trae el .led oficial.
- Este script recalcula:
  1) checksum interno total al final de la imagen;
  2) checksum de cada frame;
  3) checksum final extendido del último bloque.
"""

from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path


FRAME_DATA_SIZE = 0x800
FRAME_HEADER_SIZE = 18
FRAME_FOOTER_SIZE = 12
FRAME_SIZE = FRAME_HEADER_SIZE + FRAME_DATA_SIZE + FRAME_FOOTER_SIZE


class Segment:
    def __init__(self, header: bytes, data: bytes, footer: bytes):
        self.header = bytearray(header)
        self.data = bytearray(data)
        self.footer = bytearray(footer)


def decode_led(path: Path) -> bytes:
    text = path.read_bytes().strip()
    compact = re.sub(rb"\s+", b"", text)

    if not re.fullmatch(rb"[0-9A-Fa-f]+", compact) or len(compact) % 2:
        raise SystemExit("El .led no parece ASCII hexadecimal simple.")

    return bytes.fromhex(compact.decode("ascii"))


def parse_template(raw: bytes) -> tuple[list[Segment], bytearray]:
    if len(raw) < FRAME_SIZE:
        raise SystemExit("Plantilla .led demasiado pequeña.")

    # KeyStep 1.0.28: N frames de 2078 bytes + tail final de 12 bytes.
    if (len(raw) - 12) % FRAME_SIZE == 0:
        frame_count = (len(raw) - 12) // FRAME_SIZE
    elif len(raw) % FRAME_SIZE == 0:
        frame_count = len(raw) // FRAME_SIZE
    else:
        raise SystemExit("Tamaño .led inesperado; no calza con frames 0x81E.")

    segments: list[Segment] = []
    pos = 0
    for _ in range(frame_count):
        header = raw[pos : pos + FRAME_HEADER_SIZE]
        pos += FRAME_HEADER_SIZE

        data = raw[pos : pos + FRAME_DATA_SIZE]
        pos += FRAME_DATA_SIZE

        footer = raw[pos : pos + FRAME_FOOTER_SIZE]
        pos += FRAME_FOOTER_SIZE

        segments.append(Segment(header, data, footer))

    tail = bytearray(raw[pos:])
    if len(tail) not in (0, 12):
        raise SystemExit(f"Tail inesperado: {len(tail)} bytes")

    return segments, tail


def calc_segment_checksum(header: bytes | bytearray,
                          data: bytes | bytearray,
                          footer_prefix: bytes | bytearray) -> int:
    """
    Checksum:
    suma de todos los bytes previos excepto los primeros dos bytes del header,
    invertida a 16 bits.

    El header se trata como struct >7sH7sH.
    El footer usa checksum little-endian.
    """
    header_tuple = struct.unpack(">7sH7sH", bytes(header))
    header_bytes = struct.pack(">7sH7sH", *header_tuple)

    total = sum(header_bytes[2:])
    total += sum(data)
    total += sum(footer_prefix)

    return (0x10000 - (total & 0xFFFF)) & 0xFFFF


def get_first_address(segment: Segment) -> int:
    header_tuple = struct.unpack(">7sH7sH", bytes(segment.header))
    page = header_tuple[1]
    return 0x08000000 + (page << 8)


def basic_vector_check(app: bytes) -> None:
    if len(app) < 8:
        raise SystemExit("app.bin demasiado pequeño.")

    sp, pc = struct.unpack_from("<II", app, 0)

    print(f"Vector SP: 0x{sp:08X}")
    print(f"Vector PC: 0x{pc:08X}")

    if not (0x20000000 <= sp <= 0x20010000):
        raise SystemExit("SP no parece estar en SRAM. Revisa el linker script.")

    if not (0x08004000 <= pc <= 0x08040000):
        raise SystemExit("PC no apunta a 0x08004000+. Revisa FLASH ORIGIN.")

    if (pc & 1) == 0:
        raise SystemExit("PC no tiene bit Thumb activo. Revisa el binario.")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("official_led", type=Path)
    parser.add_argument("app_bin", type=Path)
    parser.add_argument("out_led", type=Path)
    args = parser.parse_args()

    raw_template = decode_led(args.official_led)
    segments, tail = parse_template(raw_template)

    app = args.app_bin.read_bytes()
    basic_vector_check(app)

    capacity = len(segments) * FRAME_DATA_SIZE
    print(f"Capacidad plantilla: {capacity} bytes")
    print(f"Tamaño app.bin:      {len(app)} bytes")
    print(f"Inicio plantilla:    0x{get_first_address(segments[0]):08X}")

    if len(app) > capacity:
        raise SystemExit("app.bin no cabe en la plantilla .led oficial.")

    # Imagen completa de aplicación, rellenada con 0xFF.
    full = bytearray([0xFF]) * capacity
    full[:len(app)] = app

    # Checksum interno total en los últimos 2 bytes de la imagen.
    total_checksum = (0x10000 - (sum(full[:-2]) & 0xFFFF)) & 0xFFFF
    full[-2] = total_checksum & 0xFF
    full[-1] = (total_checksum >> 8) & 0xFF
    print(f"Checksum interno total: 0x{total_checksum:04X}")

    # Reemplazar datos de segmentos.
    for i, segment in enumerate(segments):
        start = i * FRAME_DATA_SIZE
        end = start + FRAME_DATA_SIZE
        segment.data[:] = full[start:end]

    # Recalcular checksums de frames.
    for i, segment in enumerate(segments):
        if i == len(segments) - 1 and len(tail) == 12:
            # Último frame: footer normal + tail.
            combined_footer = bytearray(segment.footer + tail)
            prefix = combined_footer[:-2]
            checksum = calc_segment_checksum(segment.header, segment.data, prefix)
            combined_footer[-2] = checksum & 0xFF
            combined_footer[-1] = (checksum >> 8) & 0xFF

            segment.footer[:] = combined_footer[:12]
            tail[:] = combined_footer[12:]
        else:
            prefix = segment.footer[:-2]
            checksum = calc_segment_checksum(segment.header, segment.data, prefix)
            segment.footer[-2] = checksum & 0xFF
            segment.footer[-1] = (checksum >> 8) & 0xFF

    out = bytearray()
    for segment in segments:
        out += segment.header
        out += segment.data
        out += segment.footer
    out += tail

    args.out_led.write_text(out.hex().upper(), encoding="ascii")
    print(f"OK: escrito {args.out_led}")


if __name__ == "__main__":
    main()

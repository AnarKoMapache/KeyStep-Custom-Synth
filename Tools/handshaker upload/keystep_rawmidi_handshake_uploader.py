#!/usr/bin/env python3
"""
keystep_rawmidi_handshake_uploader.py

Uploader experimental para Arturia KeyStep MK1 usando el inicio real
capturado desde MIDI Control Center.

Patrón confirmado en keystep_mcc_update_official3.pcapng:

1) MCC consulta identidad varias veces:
   F0 7E 7F 06 01 F7

2) MCC manda dos veces este handshake:
   F0 5A 57 6E 28 3C 4E 3F F7

3) El KeyStep responde cada handshake:
   F0 3F F7

4) Recién ahí empieza firmware .led como SysEx cortos:
   F0 <22 caracteres ASCII hex> F7

5) ACK:
   F0 1A F7  por frame normal
   F0 77 F7  final

Uso:
    python3 keystep_rawmidi_handshake_uploader.py firmware.led --port hw:2,0,0

Bootloader:
    Record + Stop + Play/Pause + conectar USB
"""

from __future__ import annotations

import argparse
import os
import re
import select
import subprocess
import time
from pathlib import Path


FRAME_RAW_SIZE = 0x81E
FRAME_HEX_SIZE = FRAME_RAW_SIZE * 2
TAIL_HEX_SIZE = 24
CHUNK_HEX_CHARS = 22

IDENTITY_REQ = b"\xF0\x7E\x7F\x06\x01\xF7"
MCC_START_HANDSHAKE = b"\xF0\x5A\x57\x6E\x28\x3C\x4E\x3F\xF7"

ACK_HANDSHAKE = b"\xF0\x3F\xF7"
ACK_FRAME = b"\xF0\x1A\xF7"
ACK_FINAL = b"\xF0\x77\xF7"


def compact_led(path: Path) -> bytes:
    data = path.read_bytes().strip()
    compact = re.sub(rb"\s+", b"", data).upper()
    if not re.fullmatch(rb"[0-9A-F]+", compact):
        raise SystemExit("El .led no parece texto hexadecimal ASCII.")
    if len(compact) % 2:
        raise SystemExit("El .led tiene cantidad impar de caracteres hex.")
    return compact


def split_led(compact: bytes) -> tuple[list[bytes], bytes]:
    body_len = len(compact) - TAIL_HEX_SIZE
    if body_len <= 0 or body_len % FRAME_HEX_SIZE != 0:
        raise SystemExit(
            f"Tamaño inesperado: total={len(compact)} body={body_len}; "
            "no calza con frames de 0x81E bytes + tail de 12 bytes."
        )

    frames = [
        compact[i * FRAME_HEX_SIZE:(i + 1) * FRAME_HEX_SIZE]
        for i in range(body_len // FRAME_HEX_SIZE)
    ]
    tail = compact[body_len:]
    return frames[:-1], frames[-1] + tail


def list_amidi_ports() -> list[tuple[str, str]]:
    try:
        r = subprocess.run(
            ["amidi", "-l"],
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except FileNotFoundError:
        return []

    ports = []
    for line in r.stdout.splitlines():
        m = re.match(r"^(?:IO|I|O)\s+(hw:(\d+),(\d+),(\d+))\s+(.+)$", line.strip())
        if m:
            ports.append((m.group(1), m.group(5)))
    return ports


def autodetect_hw() -> str | None:
    for hw, name in list_amidi_ports():
        if "keystep updater" in name.lower():
            return hw
    for hw, name in list_amidi_ports():
        if "keystep" in name.lower():
            return hw
    return None


def hw_to_devnode(hw: str) -> Path:
    m = re.fullmatch(r"hw:(\d+),(\d+),(\d+)", hw.strip())
    if not m:
        raise SystemExit(f"Puerto inválido: {hw}. Usa formato hw:2,0,0")
    card = int(m.group(1))
    device = int(m.group(2))
    return Path(f"/dev/snd/midiC{card}D{device}")


def drain(fd: int, seconds: float = 0.25) -> bytes:
    end = time.time() + seconds
    buf = bytearray()
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.02)
        if fd in r:
            try:
                part = os.read(fd, 4096)
            except BlockingIOError:
                part = b""
            if part:
                buf.extend(part)
        time.sleep(0.001)
    return bytes(buf)


def wait_for(fd: int, wanted: bytes, timeout: float) -> tuple[bool, bytes]:
    end = time.time() + timeout
    buf = bytearray()
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.02)
        if fd in r:
            try:
                part = os.read(fd, 4096)
            except BlockingIOError:
                part = b""
            if part:
                buf.extend(part)
                if wanted in buf:
                    return True, bytes(buf)
        time.sleep(0.001)
    return False, bytes(buf)


def send_and_wait(fd: int, msg: bytes, wanted: bytes, label: str, timeout: float) -> bool:
    os.write(fd, msg)
    ok, seen = wait_for(fd, wanted, timeout)
    if ok:
        print(f"{label}: ACK OK  recibido: {seen.hex(' ')}")
        return True
    if seen:
        print(f"{label}: SIN ACK esperado, recibido: {seen.hex(' ')}")
    else:
        print(f"{label}: SIN RESPUESTA")
    return False


def send_block(fd: int, block: bytes, label: str, want_ack: bytes,
               chunk_delay: float, ack_timeout: float) -> bool:
    sent = 0
    for i in range(0, len(block), CHUNK_HEX_CHARS):
        chunk = block[i:i + CHUNK_HEX_CHARS]
        msg = b"\xF0" + chunk + b"\xF7"
        os.write(fd, msg)
        sent += 1
        if chunk_delay:
            time.sleep(chunk_delay)

    ok, seen = wait_for(fd, want_ack, ack_timeout)
    if ok:
        print(f"{label}: enviado {sent} SysEx -> ACK OK  recibido: {seen.hex(' ')}")
        return True

    if seen:
        print(f"{label}: enviado {sent} SysEx -> SIN ACK esperado, recibido: {seen.hex(' ')}")
    else:
        print(f"{label}: enviado {sent} SysEx -> SIN RESPUESTA")
    return False


def dry_run(compact: bytes) -> None:
    normal, final = split_led(compact)
    total = 0
    print("Resumen .led:")
    print(f"  total hex chars: {len(compact)}")
    print(f"  frames normales: {len(normal)}")
    print(f"  bloque final: {len(final)} hex chars")
    print(f"  handshake MCC: {MCC_START_HANDSHAKE.hex(' ')}")
    for i, frame in enumerate(normal):
        n = (len(frame) + CHUNK_HEX_CHARS - 1) // CHUNK_HEX_CHARS
        total += n
        if i < 3 or i >= len(normal) - 2:
            print(f"  frame {i+1}: {len(frame)} chars -> {n} SysEx")
    n = (len(final) + CHUNK_HEX_CHARS - 1) // CHUNK_HEX_CHARS
    total += n
    print(f"  final: {len(final)} chars -> {n} SysEx")
    print(f"  total SysEx firmware: {total}")


def confirm(yes: bool, led: Path, hw: str, devnode: Path) -> None:
    if yes:
        return
    print()
    print("Vas a cargar firmware al KeyStep usando handshake estilo MCC.")
    print(f"Archivo: {led}")
    print(f"Puerto:  {hw}")
    print(f"Device:  {devnode}")
    print()
    print("Debe estar en bootloader: Record + Stop + Play/Pause + conectar USB.")
    ans = input("Escribe EXACTAMENTE: SI ENTIENDO EL RIESGO\n> ").strip()
    if ans != "SI ENTIENDO EL RIESGO":
        raise SystemExit("Cancelado.")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("led", type=Path, nargs="?")
    ap.add_argument("--port", help="ej: hw:2,0,0")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--yes", action="store_true")
    ap.add_argument("--chunk-delay", type=float, default=0.004)
    ap.add_argument("--frame-delay", type=float, default=0.05)
    ap.add_argument("--ack-timeout", type=float, default=8.0)
    ap.add_argument("--identity-count", type=int, default=0, help="opcional: manda identity request N veces antes del handshake")
    ap.add_argument("--only-handshake", action="store_true")
    ap.add_argument("--only-first-frame", action="store_true")
    args = ap.parse_args()

    if args.list:
        for hw, name in list_amidi_ports():
            print(f"{hw}  {name}")
        return

    if not args.led:
        raise SystemExit("Falta archivo .led")

    compact = compact_led(args.led)

    if args.dry_run:
        dry_run(compact)
        return

    hw = args.port or autodetect_hw()
    if not hw:
        print("Puertos detectados:")
        for p, n in list_amidi_ports():
            print(f"  {p}  {n}")
        raise SystemExit("No pude detectar KeyStep. Usa --port hw:X,Y,Z")

    devnode = hw_to_devnode(hw)
    if not devnode.exists():
        raise SystemExit(f"No existe {devnode}. Revisa amidi -l.")

    normal_frames, final_block = split_led(compact)
    confirm(args.yes, args.led, hw, devnode)

    print("Abriendo raw MIDI...")
    try:
        fd = os.open(str(devnode), os.O_RDWR | os.O_NONBLOCK)
    except PermissionError:
        raise SystemExit(
            f"Sin permiso para abrir {devnode}. Cierra otros programas MIDI o agrega tu usuario al grupo audio."
        )

    try:
        old = drain(fd, 0.5)
        if old:
            print(f"Datos previos drenados: {old.hex(' ')}")

        if args.identity_count > 0:
            print(f"Mandando identity request {args.identity_count} veces...")
            for _ in range(args.identity_count):
                os.write(fd, IDENTITY_REQ)
                time.sleep(0.05)
            leftover = drain(fd, 0.3)
            if leftover:
                print(f"Respuesta identity/drenaje: {leftover.hex(' ')}")

        print("Handshake MCC 1/2...")
        if not send_and_wait(fd, MCC_START_HANDSHAKE, ACK_HANDSHAKE, "Handshake 1", args.ack_timeout):
            raise SystemExit(2)

        time.sleep(0.25)

        print("Handshake MCC 2/2...")
        if not send_and_wait(fd, MCC_START_HANDSHAKE, ACK_HANDSHAKE, "Handshake 2", args.ack_timeout):
            raise SystemExit(3)

        if args.only_handshake:
            print("Modo --only-handshake: detenido después del handshake.")
            return

        time.sleep(0.1)

        for i, frame in enumerate(normal_frames):
            ok = send_block(
                fd,
                frame,
                f"Frame {i+1}/{len(normal_frames)+1}",
                ACK_FRAME,
                args.chunk_delay,
                args.ack_timeout,
            )
            if not ok:
                print("Detenido para no seguir enviando a ciegas.")
                raise SystemExit(4)
            if args.only_first_frame:
                print("Modo --only-first-frame: detenido después del primer frame.")
                return
            time.sleep(args.frame_delay)

        ok = send_block(
            fd,
            final_block,
            "Frame final + tail",
            ACK_FINAL,
            args.chunk_delay,
            args.ack_timeout,
        )
        if not ok:
            print("No llegó ACK final.")
            raise SystemExit(5)

        print()
        print("Carga terminada con ACK final.")
        print("Espera unos segundos, desconecta y reconecta normal.")

    finally:
        os.close(fd)


if __name__ == "__main__":
    main()

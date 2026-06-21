#!/usr/bin/env python3
"""
KeyStep MK1 bootloader USB-MIDI probe for Linux.

This script does NOT upload firmware.
It only:
- finds the "KeyStep Updater MIDI 1" ALSA raw MIDI port;
- sends a standard, non-destructive MIDI Universal Device Inquiry SysEx;
- listens briefly for any reply;
- prints raw bytes.

Requirements:
    sudo apt install alsa-utils
"""

from __future__ import annotations

import argparse
import re
import shlex
import subprocess
import sys
import time
from dataclasses import dataclass


IDENTITY_REQUEST = "F0 7E 7F 06 01 F7"


@dataclass
class MidiPort:
    direction: str
    device: str
    name: str


def run(command: list[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(
        command,
        check=check,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def list_raw_midi_ports() -> list[MidiPort]:
    try:
        result = run(["amidi", "-l"])
    except FileNotFoundError:
        print("ERROR: amidi no está instalado. Instala alsa-utils.", file=sys.stderr)
        raise SystemExit(1)

    ports: list[MidiPort] = []
    for line in result.stdout.splitlines():
        # Example:
        # IO  hw:2,0,0  KeyStep Updater MIDI 1
        m = re.match(r"^(IO|I|O)\s+(hw:[0-9]+,[0-9]+,[0-9]+)\s+(.+)$", line.strip())
        if m:
            ports.append(MidiPort(m.group(1), m.group(2), m.group(3).strip()))
    return ports


def autodetect_port(ports: list[MidiPort]) -> str | None:
    for p in ports:
        if "keystep updater" in p.name.lower():
            return p.device
    for p in ports:
        if "keystep" in p.name.lower():
            return p.device
    return None


def print_ports(ports: list[MidiPort]) -> None:
    if not ports:
        print("No se encontraron puertos raw MIDI.")
        return
    print("Puertos raw MIDI detectados:")
    for p in ports:
        print(f"  {p.direction:2s}  {p.device:10s}  {p.name}")


def send_sysex(port: str, hex_string: str) -> None:
    cmd = ["amidi", "-p", port, "-S", hex_string]
    print("$ " + shlex.join(cmd))
    result = run(cmd, check=False)
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def listen_while_sending(port: str, seconds: float, send_hex: str) -> bytes:
    dump_cmd = ["amidi", "-p", port, "-d"]
    print("$ " + shlex.join(dump_cmd))
    proc = subprocess.Popen(
        dump_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        time.sleep(0.3)
        send_sysex(port, send_hex)
        time.sleep(seconds)
    finally:
        proc.terminate()
        try:
            stdout, stderr = proc.communicate(timeout=1.5)
        except subprocess.TimeoutExpired:
            proc.kill()
            stdout, stderr = proc.communicate(timeout=1.5)

    if stderr:
        try:
            print(stderr.decode("utf-8", errors="replace"), end="", file=sys.stderr)
        except Exception:
            print(stderr, file=sys.stderr)

    return stdout or b""


def decode_amidi_dump(data: bytes) -> None:
    if not data:
        print("\nSin respuesta recibida.")
        return

    print("\nRespuesta cruda de amidi -d:")
    try:
        print(data.decode("utf-8", errors="replace"))
    except Exception:
        print(data)

    # amidi -d normally prints hex text, but also handle raw bytes just in case.
    text = data.decode("utf-8", errors="ignore")
    hex_tokens = re.findall(r"\b[0-9A-Fa-f]{2}\b", text)
    if hex_tokens:
        raw = bytes(int(x, 16) for x in hex_tokens)
        print("Bytes interpretados:")
        print(" ".join(f"{b:02X}" for b in raw))

        if raw.startswith(bytes.fromhex("F0 7E")) and bytes.fromhex("06 02") in raw:
            print("\nParece una respuesta estándar MIDI Identity Reply.")
        elif raw.startswith(b"\xF0"):
            print("\nParece una respuesta SysEx no estándar o propietaria.")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--list", action="store_true", help="listar puertos raw MIDI")
    parser.add_argument("--port", help="puerto raw MIDI, por ejemplo hw:2,0,0")
    parser.add_argument(
        "--identity",
        action="store_true",
        help="enviar Universal Device Inquiry y escuchar respuesta",
    )
    parser.add_argument(
        "--seconds",
        type=float,
        default=3.0,
        help="segundos de escucha después del envío",
    )

    args = parser.parse_args()

    ports = list_raw_midi_ports()

    if args.list:
        print_ports(ports)
        return

    port = args.port or autodetect_port(ports)

    if not port:
        print_ports(ports)
        print("\nNo pude autodetectar el KeyStep. Usa --port hw:X,Y,Z")
        raise SystemExit(1)

    print(f"Usando puerto: {port}")

    if args.identity:
        print(f"Enviando MIDI Universal Device Inquiry: {IDENTITY_REQUEST}")
        data = listen_while_sending(port, args.seconds, IDENTITY_REQUEST)
        decode_amidi_dump(data)
        return

    print("Nada que hacer. Usa --list o --identity.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Optional MIDI SysEx monitor.

This only helps if the Arturia bootloader exposes itself as a MIDI input and the OS
allows another application to read the same port while MIDI Control Center is updating.
On Windows that may fail because the port can be exclusive.

Install:
    pip install mido python-rtmidi

Run:
    python midi_sysex_monitor.py --list
    python midi_sysex_monitor.py --port "port name here" --out midi_log.txt
"""

from __future__ import annotations

import argparse
import time
from pathlib import Path

import mido


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--port")
    parser.add_argument("--out", type=Path, default=Path("midi_log.txt"))
    args = parser.parse_args()

    if args.list:
        print("Inputs:")
        for name in mido.get_input_names():
            print("  ", name)
        print("Outputs:")
        for name in mido.get_output_names():
            print("  ", name)
        return

    if not args.port:
        raise SystemExit("Use --list first, then pass --port.")

    with mido.open_input(args.port) as inp, args.out.open("w", encoding="utf-8") as f:
        print(f"Listening on {args.port}. Ctrl+C to stop.")
        for msg in inp:
            line = f"{time.time():.6f} {msg!r}\n"
            print(line, end="")
            f.write(line)
            f.flush()


if __name__ == "__main__":
    main()

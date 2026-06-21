#!/usr/bin/env python3
"""
Compare Arturia .led firmware file against extracted USB payload streams.

Usage:
    python compare_led_to_capture.py keystep_Firmware_Update__1_0_28.led out_payloads
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


def load_led(path: Path) -> tuple[bytes, bytes]:
    led_text = path.read_bytes().strip()

    # Arturia .led files for this KeyStep firmware appear to be ASCII hex.
    compact = re.sub(rb"\s+", b"", led_text)
    if re.fullmatch(rb"[0-9a-fA-F]+", compact) and len(compact) % 2 == 0:
        return led_text, bytes.fromhex(compact.decode("ascii"))

    return led_text, led_text


def find_all(haystack: bytes, needle: bytes, limit: int = 20) -> list[int]:
    out = []
    start = 0
    while len(out) < limit:
        idx = haystack.find(needle, start)
        if idx < 0:
            break
        out.append(idx)
        start = idx + 1
    return out


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("led", type=Path)
    parser.add_argument("payload_folder", type=Path)
    args = parser.parse_args()

    led_ascii, led_raw = load_led(args.led)

    print("LED file:")
    print(f"  path: {args.led}")
    print(f"  ascii/text bytes: {len(led_ascii)}")
    print(f"  decoded/raw bytes: {len(led_raw)}")
    print(f"  sha256 raw: {hashlib.sha256(led_raw).hexdigest()}")

    probes = [
        ("led_ascii_first_32", led_ascii[:32]),
        ("led_ascii_first_128", led_ascii[:128]),
        ("led_raw_first_16", led_raw[:16]),
        ("led_raw_first_32", led_raw[:32]),
        ("led_raw_first_128", led_raw[:128]),
    ]

    # Some bootloaders skip a proprietary header. Test probable offsets too.
    for off in [16, 18, 20, 256, 512, 1024, 4096, 0x4000]:
        if len(led_raw) > off + 32:
            probes.append((f"led_raw_offset_{off}_32", led_raw[off : off + 32]))

    bin_files = sorted(args.payload_folder.glob("*.bin"))
    if not bin_files:
        print("\nNo .bin payload streams found.")
        return

    report = {}

    for f in bin_files:
        blob = f.read_bytes()
        print(f"\nStream: {f.name} ({len(blob)} bytes)")
        stream_info = {}

        for name, probe in probes:
            if not probe:
                continue
            positions = find_all(blob, probe)
            if positions:
                print(f"  MATCH {name}: {positions[:5]}")
                stream_info[name] = positions

        # Find longish sequences of the raw firmware with sliding windows.
        best = []
        for off in range(0, max(1, len(led_raw) - 64), 256):
            chunk = led_raw[off : off + 64]
            pos = blob.find(chunk)
            if pos >= 0:
                best.append({"led_offset": off, "stream_offset": pos})
                if len(best) >= 20:
                    break

        if best:
            print("  Raw firmware chunks found:")
            for item in best[:10]:
                print(
                    f"    led+0x{item['led_offset']:x} -> "
                    f"stream+0x{item['stream_offset']:x}"
                )
            stream_info["raw_chunks"] = best

        if not stream_info:
            print("  No direct match with tested probes.")

        report[f.name] = stream_info

    (args.payload_folder / "comparison_report.json").write_text(
        json.dumps(report, indent=2),
        encoding="utf-8",
    )

    print(f"\nReport written to {args.payload_folder / 'comparison_report.json'}")


if __name__ == "__main__":
    main()

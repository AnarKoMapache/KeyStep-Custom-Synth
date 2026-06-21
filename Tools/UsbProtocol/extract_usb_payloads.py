#!/usr/bin/env python3
"""
Extract USB payload-like fields from a Wireshark/USBPcap capture.

Usage:
    python extract_usb_payloads.py keystep_update.pcapng --out out_payloads

Requires:
    tshark in PATH.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path


FIELDS = [
    "frame.number",
    "frame.time_relative",
    "usb.src",
    "usb.dst",
    "usb.endpoint_address",
    "usb.endpoint_number",
    "usb.transfer_type",
    "usb.capdata",
    "usb.data_fragment",
    "usbhid.data",
]


HEX_RE = re.compile(r"^[0-9a-fA-F: ]+$")


def run_tshark(pcap: Path) -> list[dict[str, str]]:
    cmd = [
        "tshark",
        "-r",
        str(pcap),
        "-T",
        "fields",
        "-E",
        "header=y",
        "-E",
        "separator=\t",
        "-E",
        "occurrence=f",
    ]
    for field in FIELDS:
        cmd += ["-e", field]

    result = subprocess.run(
        cmd,
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    lines = result.stdout.splitlines()
    if not lines:
        return []

    reader = csv.DictReader(lines, delimiter="\t")
    return list(reader)


def clean_hex(value: str) -> bytes:
    value = (value or "").strip()
    if not value:
        return b""
    if not HEX_RE.match(value):
        return b""
    value = value.replace(":", "").replace(" ", "")
    if len(value) % 2:
        return b""
    try:
        return bytes.fromhex(value)
    except ValueError:
        return b""


def endpoint_key(row: dict[str, str]) -> str:
    endpoint = (row.get("usb.endpoint_address") or "").strip()
    if endpoint:
        try:
            n = int(endpoint, 0)
            direction = "IN" if (n & 0x80) else "OUT"
            return f"ep_{endpoint}_{direction}"
        except ValueError:
            return f"ep_{endpoint}"

    number = (row.get("usb.endpoint_number") or "").strip()
    src = (row.get("usb.src") or "").strip()
    dst = (row.get("usb.dst") or "").strip()
    return f"epnum_{number}_src_{src}_dst_{dst}"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("pcap", type=Path)
    parser.add_argument("--out", type=Path, default=Path("out_payloads"))
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)

    rows = run_tshark(args.pcap)

    streams: dict[str, bytearray] = defaultdict(bytearray)
    records = []

    payload_fields = ["usb.capdata", "usb.data_fragment", "usbhid.data"]

    for row in rows:
        chosen_field = ""
        payload = b""

        for field in payload_fields:
            payload = clean_hex(row.get(field, ""))
            if payload:
                chosen_field = field
                break

        if not payload:
            continue

        key = endpoint_key(row)
        streams[key].extend(payload)

        records.append(
            {
                "frame": row.get("frame.number", ""),
                "time": row.get("frame.time_relative", ""),
                "endpoint": key,
                "field": chosen_field,
                "len": len(payload),
                "hex_preview": payload[:64].hex(),
            }
        )

    for key, blob in streams.items():
        safe = re.sub(r"[^a-zA-Z0-9_.-]+", "_", key)
        (args.out / f"{safe}.bin").write_bytes(bytes(blob))

    (args.out / "records.json").write_text(
        json.dumps(records, indent=2),
        encoding="utf-8",
    )

    summary = {
        "pcap": str(args.pcap),
        "records": len(records),
        "streams": {key: len(value) for key, value in streams.items()},
    }

    (args.out / "summary.json").write_text(
        json.dumps(summary, indent=2),
        encoding="utf-8",
    )

    print(json.dumps(summary, indent=2))
    print(f"\nOutput folder: {args.out}")


if __name__ == "__main__":
    main()

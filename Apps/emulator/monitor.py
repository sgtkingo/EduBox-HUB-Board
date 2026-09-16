#!/usr/bin/env python3
"""Read-only UART0 diagnostic monitor for EduBox HUB."""

from __future__ import annotations

import argparse
from contextlib import ExitStack
from datetime import datetime
import re
import sys
from typing import Optional, Sequence

from emulator import DEFAULT_BAUD_RATE, EmulatorError, available_port_info, choose_port


LEVELS = {"ERROR", "WARN", "INFO", "DEBUG", "RX", "TX", "PROTOCOL", "OTHER"}
ANSI_ESCAPE = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
TAG = re.compile(r"\[(ERROR|WARN|INFO|DEBUG|RX|TX|E|W|I|D|V)\]")
ESP_LEVEL = re.compile(r"^\s*([EWIDV])\s+\(")
SHORT_LEVELS = {"E": "ERROR", "W": "WARN", "I": "INFO", "D": "DEBUG", "V": "DEBUG"}


def classify_line(line: str) -> str:
    """Recognize application LOG tags, ESP-IDF and Arduino diagnostics."""
    clean = ANSI_ESCAPE.sub("", line)
    if clean.startswith("?"):
        return "PROTOCOL"
    tag = TAG.search(clean)
    if tag:
        return SHORT_LEVELS.get(tag[1], tag[1])
    esp = ESP_LEVEL.match(clean)
    if esp:
        return SHORT_LEVELS[esp[1]]
    return "OTHER"  # Keep boot messages/backtraces visible by default.


def parse_levels(value: str) -> set[str]:
    levels = {item.strip().upper() for item in value.split(",") if item.strip()}
    if not levels or levels - LEVELS:
        raise argparse.ArgumentTypeError("Použijte úrovně: " + ", ".join(sorted(LEVELS)))
    return levels


class LineBuffer:
    """Preserve partial serial reads and bound memory for unterminated input."""
    def __init__(self, limit: int = 16384) -> None:
        self.pending = bytearray()
        self.limit = limit

    def feed(self, data: bytes) -> list[str]:
        self.pending.extend(data)
        lines = []
        while True:
            separator = self.pending.find(b"\n")
            if separator < 0:
                break
            raw = bytes(self.pending[:separator])
            del self.pending[:separator + 1]
            if len(raw) > self.limit:
                raw = raw[:self.limit] + b" [monitor: truncated]"
            lines.append(raw.rstrip(b"\r\x00").decode("utf-8", errors="replace"))
        if len(self.pending) > self.limit:
            lines.append(bytes(self.pending[:self.limit]).decode("utf-8", errors="replace")
                         + " [monitor: truncated]")
            self.pending.clear()
        return lines


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Monitor diagnostiky EduBox na UART0 / USB-UART")
    parser.add_argument("--port", help="např. COM4; jediný port se vybere automaticky")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD_RATE, help="rychlost (výchozí 115200)")
    parser.add_argument("--ports", action="store_true", help="vypsat dostupné porty a skončit")
    parser.add_argument("--levels", type=parse_levels, help="filtr, např. ERROR,WARN nebo RX,TX")
    parser.add_argument("--contains", help="zobrazit pouze řádky obsahující tento text (case-sensitive)")
    parser.add_argument("--timestamps", action="store_true", help="přidat čas příjmu z počítače")
    parser.add_argument("--log", metavar="FILE", help="připojovat zobrazené řádky do UTF-8 souboru")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    parser = create_parser()
    args = parser.parse_args(argv)
    if args.baud <= 0:
        parser.error("--baud musí být kladné číslo")
    try:
        if args.ports:
            ports = available_port_info()
            for device, description in ports:
                print(f"{device:<12} {description}")
            if not ports:
                print("Nebyl nalezen žádný sériový port")
            return 0

        port = choose_port(args.port)
        try:
            import serial
        except ImportError as error:
            raise EmulatorError("Chybí pyserial: python -m pip install -r requirements.txt") from error

        with ExitStack() as stack:
            log_file = stack.enter_context(open(args.log, "a", encoding="utf-8")) if args.log else None
            # Set modem signals before opening; do not intentionally reset the ESP.
            connection = serial.Serial(
                port=None, baudrate=args.baud, bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
                timeout=0.2, xonxoff=False, rtscts=False, dsrdtr=False,
            )
            stack.callback(connection.close)
            connection.dtr = False
            connection.rts = False
            connection.port = port
            connection.open()
            print(f"Monitor {port}, {args.baud} 8N1. Ukončení: Ctrl+C.", file=sys.stderr)
            buffer = LineBuffer()
            while True:
                data = connection.read(min(max(connection.in_waiting, 1), 4096))
                for line in buffer.feed(data):
                    if not line:
                        continue
                    if args.levels is not None and classify_line(line) not in args.levels:
                        continue
                    if args.contains is not None and args.contains not in line:
                        continue
                    prefix = datetime.now().astimezone().isoformat(timespec="milliseconds") + " " if args.timestamps else ""
                    displayed = prefix + line
                    print(displayed, flush=True)
                    if log_file:
                        log_file.write(displayed + "\n")
                        log_file.flush()
    except KeyboardInterrupt:
        print("\nMonitor ukončen.", file=sys.stderr)
        return 0
    except (EmulatorError, OSError) as error:
        print(f"CHYBA: {error}. Zavřete ostatní programy používající COM port.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

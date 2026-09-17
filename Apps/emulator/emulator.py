#!/usr/bin/env python3
"""Serial client emulator for the EduBox VSCP 1.5 protocol."""

from __future__ import annotations

import argparse
import shlex
import sys
import time
from dataclasses import dataclass
from typing import Dict, Iterable, List, Mapping, Optional, Sequence, Tuple


API_VERSION = "1.5"
DEFAULT_BAUD_RATE = 115200
DEFAULT_TIMEOUT = 1.0
VALID_COMMANDS = {
    "INIT",
    "CONNECT",
    "DISCONNECT",
    "UPDATE",
    "CONFIG",
    "CONTROL",
}


class EmulatorError(RuntimeError):
    """A readable emulator, transport, or protocol error."""


@dataclass(frozen=True)
class ProtocolResponse:
    raw: str
    parameters: Dict[str, str]

    @property
    def ok(self) -> bool:
        return self.parameters.get("status") == "1"

    @property
    def error(self) -> str:
        return self.parameters.get("error", "")


def _validate_component(value: str, label: str, allow_comma: bool = True) -> None:
    forbidden = "&=\r\n\x00"
    if any(character in value for character in forbidden):
        raise EmulatorError(f"{label} obsahuje nepovolený znak (&, = nebo konec řádku)")
    if not allow_comma and "," in value:
        raise EmulatorError(f"{label} nesmí obsahovat čárku")
    if not value:
        raise EmulatorError(f"{label} nesmí být prázdný")


def build_request(command: str, parameters: Mapping[str, str]) -> str:
    command = command.upper()
    if command not in VALID_COMMANDS:
        raise EmulatorError(f"Neznámý příkaz: {command}")

    parts = [f"type={command}"]
    for key, value in parameters.items():
        if key == "type":
            continue
        key = str(key)
        value = str(value)
        _validate_component(key, "Název parametru", allow_comma=False)
        _validate_component(value, f"Hodnota parametru {key}")
        parts.append(f"{key}={value}")
    return "?" + "&".join(parts)


def parse_response(line: str) -> ProtocolResponse:
    line = line.strip()
    if not line.startswith("?"):
        raise EmulatorError("Odpověď nezačíná znakem ?")

    parameters: Dict[str, str] = {}
    for item in line[1:].split("&"):
        if "=" not in item:
            raise EmulatorError(f"Poškozený parametr odpovědi: {item!r}")
        key, value = item.split("=", 1)
        if not key:
            raise EmulatorError("Odpověď obsahuje prázdný název parametru")
        parameters[key.strip()] = value.strip()

    if "status" not in parameters:
        raise EmulatorError("V odpovědi chybí parametr status")
    return ProtocolResponse(raw=line, parameters=parameters)


def parse_assignments(items: Iterable[str]) -> Dict[str, str]:
    parameters: Dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise EmulatorError(f"Parametr musí mít tvar klíč=hodnota: {item!r}")
        key, value = item.split("=", 1)
        key = key.strip()
        value = value.strip()
        _validate_component(key, "Název parametru", allow_comma=False)
        _validate_component(value, f"Hodnota parametru {key}")
        parameters[key] = value
    return parameters


def available_port_info() -> List[Tuple[str, str]]:
    try:
        from serial.tools import list_ports
    except ImportError as error:
        raise EmulatorError(
            "Chybí balíček pyserial. Nainstalujte jej příkazem "
            "'python -m pip install -r requirements.txt'."
        ) from error
    return sorted(
        (port.device, port.description or "bez popisu")
        for port in list_ports.comports()
    )


def available_ports() -> List[str]:
    return [device for device, _ in available_port_info()]


def choose_port(requested_port: Optional[str]) -> str:
    if requested_port:
        return requested_port
    ports = available_ports()
    if len(ports) == 1:
        print(f"Automaticky vybrán port {ports[0]}")
        return ports[0]
    if not ports:
        raise EmulatorError("Nebyl nalezen žádný sériový port")
    raise EmulatorError(
        "Je připojeno více sériových portů. Použijte --port: " + ", ".join(ports)
    )


class SerialLineTransport:
    def __init__(
        self,
        port: str,
        baud_rate: int = DEFAULT_BAUD_RATE,
        timeout: float = DEFAULT_TIMEOUT,
        boot_wait: float = 1.0,
    ) -> None:
        try:
            import serial
        except ImportError as error:
            raise EmulatorError(
                "Chybí balíček pyserial. Nainstalujte jej příkazem "
                "'python -m pip install -r requirements.txt'."
            ) from error

        self._serial_exception = serial.SerialException
        try:
            self._serial = serial.Serial(
                port=port,
                baudrate=baud_rate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=min(timeout, 0.1),
                write_timeout=timeout,
            )
        except serial.SerialException as error:
            raise EmulatorError(f"Nelze otevřít port {port}: {error}") from error

        self.timeout = timeout
        if boot_wait > 0:
            time.sleep(boot_wait)
        self._serial.reset_input_buffer()

    def close(self) -> None:
        self._serial.close()

    def exchange(self, request: str) -> str:
        try:
            # Match the C++ client: discard responses left behind by a timed-out request.
            self._serial.reset_input_buffer()
            self._serial.write((request + "\n").encode("ascii"))
            self._serial.flush()
        except (UnicodeEncodeError, OSError, self._serial_exception) as error:
            raise EmulatorError(f"Požadavek nelze odeslat: {error}") from error

        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            try:
                raw_line = self._serial.readline()
            except (OSError, self._serial_exception) as error:
                raise EmulatorError(f"Chyba při čtení odpovědi: {error}") from error
            if not raw_line:
                continue
            line = raw_line.decode("ascii", errors="ignore").strip("\x00\r\n ")
            # Boot messages and diagnostics do not belong to the protocol.
            if line.startswith("?"):
                return line
            if line:
                print(f"LOG {line}")
        raise EmulatorError(f"Vypršel limit {self.timeout:.2f} s při čekání na odpověď")

    def __enter__(self) -> "SerialLineTransport":
        return self

    def __exit__(self, *_: object) -> None:
        self.close()


class VscpClient:
    def __init__(self, transport: SerialLineTransport) -> None:
        self.transport = transport

    def request(self, command: str, parameters: Mapping[str, str]) -> ProtocolResponse:
        request = build_request(command, parameters)
        print(f"TX  {request}")
        raw_response = self.transport.exchange(request)
        print(f"RX  {raw_response}")
        response = parse_response(raw_response)

        expected_id = parameters.get("id")
        response_id = response.parameters.get("id")
        if expected_id and response_id != expected_id:
            raise EmulatorError(
                f"UID v odpovědi nesouhlasí: očekáváno {expected_id}, přijato {response_id}"
            )
        print("    OK" if response.ok else f"    CHYBA: {response.error or 'status=0'}")
        return response

    def init(self, app: str = "python-emulator", database: str = "1.3") -> ProtocolResponse:
        return self.request("INIT", {"api": API_VERSION, "app": app, "db": database})

    def connect(self, uid: str, pins: str) -> ProtocolResponse:
        return self.request("CONNECT", {"id": uid, "pins": pins})

    def disconnect(self, uid: str) -> ProtocolResponse:
        return self.request("DISCONNECT", {"id": uid})

    def update(self, uid: str) -> ProtocolResponse:
        return self.request("UPDATE", {"id": uid})

    def config(self, uid: str, parameters: Mapping[str, str]) -> ProtocolResponse:
        return self.request("CONFIG", {"id": uid, **parameters})

    def control(self, uid: str, parameters: Mapping[str, str]) -> ProtocolResponse:
        return self.request("CONTROL", {"id": uid, **parameters})


def require_ok(response: ProtocolResponse, command: str) -> None:
    if not response.ok:
        raise EmulatorError(f"Scénář zastaven po {command}: {response.error or 'status=0'}")


def run_scenario(client: VscpClient, args: argparse.Namespace) -> None:
    """Exercise every requested command with safe default values."""
    config = parse_assignments(args.config)
    control = parse_assignments(args.control)

    require_ok(client.init(args.app, args.database), "INIT")
    require_ok(client.connect(args.sensor_id, args.sensor_pins), "CONNECT sensoru")
    try:
        require_ok(client.config(args.sensor_id, config), "CONFIG")
        for _ in range(args.updates):
            require_ok(client.update(args.sensor_id), "UPDATE")
            if args.interval > 0:
                time.sleep(args.interval)
    finally:
        require_ok(client.disconnect(args.sensor_id), "DISCONNECT sensoru")

    require_ok(client.connect(args.actuator_id, args.actuator_pins), "CONNECT aktuátoru")
    try:
        require_ok(client.control(args.actuator_id, control), "CONTROL")
    finally:
        require_ok(client.disconnect(args.actuator_id), "DISCONNECT aktuátoru")


SHELL_HELP = """Příkazy:
  init [app=python-emulator] [db=1.3]
  connect UID PINY                  např. connect S03 7
  disconnect UID
  update UID
  config UID klíč=hodnota [...]
  control UID klíč=hodnota [...]
  send TYP klíč=hodnota [...]       odešle obecný požadavek
  help
  quit
"""


def run_shell(client: VscpClient) -> None:
    print(SHELL_HELP)
    while True:
        try:
            line = input("vscp> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            return
        if not line:
            continue
        try:
            tokens = shlex.split(line)
            command = tokens[0].lower()
            arguments = tokens[1:]
            if command in {"quit", "exit"}:
                return
            if command == "help":
                print(SHELL_HELP)
            elif command == "init":
                values = parse_assignments(arguments)
                client.init(values.get("app", "python-emulator"), values.get("db", "1.3"))
            elif command == "connect" and len(arguments) == 2:
                client.connect(arguments[0], arguments[1])
            elif command == "disconnect" and len(arguments) == 1:
                client.disconnect(arguments[0])
            elif command == "update" and len(arguments) == 1:
                client.update(arguments[0])
            elif command == "config" and len(arguments) >= 2:
                client.config(arguments[0], parse_assignments(arguments[1:]))
            elif command == "control" and len(arguments) >= 2:
                client.control(arguments[0], parse_assignments(arguments[1:]))
            elif command == "send" and arguments:
                client.request(arguments[0], parse_assignments(arguments[1:]))
            else:
                print("Neplatné argumenty. Zadejte 'help'.")
        except EmulatorError as error:
            print(f"CHYBA: {error}", file=sys.stderr)


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Emulátor klienta komunikačního protokolu EduBox VSCP 1.5"
    )
    parser.add_argument("--port", help="sériový port, např. COM4; při jediném portu se vybere automaticky")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD_RATE, help="rychlost portu (výchozí: 115200)")
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT, help="limit odpovědi v sekundách")
    parser.add_argument("--boot-wait", type=float, default=1.0, help="čekání po otevření portu v sekundách")

    subparsers = parser.add_subparsers(dest="mode")
    subparsers.add_parser("ports", help="vypíše dostupné sériové porty")
    subparsers.add_parser("shell", help="spustí interaktivní terminál")

    send = subparsers.add_parser("send", help="odešle jeden příkaz")
    send.add_argument("command", choices=sorted(VALID_COMMANDS), type=str.upper)
    send.add_argument("parameters", nargs="*", metavar="KEY=VALUE")

    scenario = subparsers.add_parser("scenario", help="provede automatický test všech příkazů")
    scenario.add_argument("--app", default="python-emulator")
    scenario.add_argument("--database", default="1.3")
    scenario.add_argument("--sensor-id", default="S03", help="výchozí analogový Hallův senzor")
    scenario.add_argument("--sensor-pins", default="7")
    scenario.add_argument("--config", action="append", default=["res=12"], metavar="KEY=VALUE")
    scenario.add_argument("--updates", type=int, default=1)
    scenario.add_argument("--interval", type=float, default=0.2)
    scenario.add_argument("--actuator-id", default="A09", help="výchozí laser, CONTROL jej ponechá vypnutý")
    scenario.add_argument("--actuator-pins", default="7")
    scenario.add_argument("--control", action="append", default=["control=off"], metavar="KEY=VALUE")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    # Keep Czech diagnostics readable when Windows selected a legacy code page.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")
    args = create_parser().parse_args(argv)
    try:
        if args.mode == "ports":
            ports = available_port_info()
            if ports:
                for device, description in ports:
                    print(f"{device:<8} {description}")
            else:
                print("Nebyl nalezen žádný sériový port")
            return 0

        port = choose_port(args.port)
        with SerialLineTransport(port, args.baud, args.timeout, args.boot_wait) as transport:
            client = VscpClient(transport)
            if args.mode == "send":
                response = client.request(args.command, parse_assignments(args.parameters))
                return 0 if response.ok else 2
            if args.mode == "scenario":
                if args.updates < 1:
                    raise EmulatorError("--updates musí být alespoň 1")
                run_scenario(client, args)
                print("Scénář dokončen úspěšně.")
                return 0
            run_shell(client)
            return 0
    except EmulatorError as error:
        print(f"CHYBA: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

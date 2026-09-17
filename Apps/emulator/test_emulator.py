import unittest
import threading
from unittest.mock import patch

from emulator import EmulatorError, build_request, parse_assignments, parse_response
from emulator import API_VERSION, LIBRARY_VERSION, SerialLineTransport, shell_input


class CodecTests(unittest.TestCase):
    def test_build_request(self):
        self.assertEqual(
            build_request("connect", {"id": "S03", "pins": "7"}),
            "?type=CONNECT&id=S03&pins=7",
        )

    def test_parse_success_response(self):
        response = parse_response("?id=S03&status=1&val=2048")
        self.assertTrue(response.ok)
        self.assertEqual(response.parameters["id"], "S03")

    def test_parse_error_response(self):
        response = parse_response("?error=Device not found&id=S99&status=0")
        self.assertFalse(response.ok)
        self.assertEqual(response.error, "Device not found")

    def test_reject_assignment_without_equals(self):
        with self.assertRaises(EmulatorError):
            parse_assignments(["broken"])


class FakeSerial:
    def __init__(self, lines=()):
        self.lines = list(lines)
        self.sent = []
        self.closed = False

    def write(self, value):
        self.sent.append(value.decode("ascii").strip())

    def flush(self):
        pass

    def readline(self):
        return (self.lines.pop(0) + "\n").encode("ascii") if self.lines else b""

    def close(self):
        self.closed = True


class SessionTests(unittest.TestCase):
    def transport(self, lines=()):
        transport = object.__new__(SerialLineTransport)
        transport._serial = FakeSerial(lines)
        transport._serial_exception = OSError
        transport.timeout = 0.01
        transport.session_closed = False
        return transport

    def test_bidirectional_ping_and_response_routing(self):
        self.assertEqual((API_VERSION, LIBRARY_VERSION), ("1.6", "2.2.2"))
        wire = self.transport(("?side=server&seq=999&status=1", "?type=PING&side=server&seq=7",
                               "?side=server&seq=22&status=1"))
        self.assertEqual(wire.exchange("?type=PING&side=client&seq=22"), "?side=server&seq=22&status=1")
        self.assertEqual(wire._serial.sent, ["?type=PING&side=client&seq=22", "?side=client&seq=7&status=1"])
        wire._serial.lines.extend(("?side=server&seq=22&status=1", "?api=1.6&status=1"))
        self.assertEqual(wire.exchange("?type=INIT&api=1.6"), "?api=1.6&status=1")

    def test_bye_is_one_way_and_requires_new_init(self):
        wire = self.transport()
        self.assertEqual(wire.exchange("?type=BYE&side=client"), "")
        self.assertTrue(wire.session_closed)
        self.assertFalse(wire._serial.closed)
        with self.assertRaises(EmulatorError):
            wire.exchange("?type=UPDATE&id=S01")
        wire._serial.lines.append("?api=1.6&status=1")
        wire.exchange("?type=INIT&api=1.6")
        self.assertFalse(wire.session_closed)
        wire.close()
        self.assertTrue(wire._serial.closed)
        self.assertEqual(wire._serial.sent[-1], "?type=BYE&side=client")

    def test_remote_bye_interrupts_and_idle_ping_is_answered(self):
        wire = self.transport(("?type=PING&side=server&seq=42",))
        wire.poll()
        self.assertEqual(wire._serial.sent, ["?side=client&seq=42&status=1"])
        wire._serial.lines.append("?type=BYE&side=server")
        with self.assertRaisesRegex(EmulatorError, "Peer disconnected"):
            wire.exchange("?type=UPDATE&id=S01")
        self.assertTrue(wire.session_closed)

    def test_shell_services_transport_while_input_is_pending(self):
        polled = threading.Event()
        class Client:
            def poll(self): polled.set()
        def user_input(_):
            if not polled.wait(2): raise RuntimeError("Input blocked protocol polling")
            return "init"
        with patch("builtins.input", user_input):
            self.assertEqual(shell_input(Client()), "init")


if __name__ == "__main__":
    unittest.main()

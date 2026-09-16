import unittest
from contextlib import redirect_stderr, redirect_stdout
import io
import sys
from types import SimpleNamespace
from unittest.mock import patch

from monitor import LineBuffer, classify_line, create_parser, main


class MonitorTests(unittest.TestCase):
    def test_application_levels(self):
        for level in ("ERROR", "WARN", "INFO", "RX", "TX"):
            self.assertEqual(classify_line(f"LOG [123][{level}] UART2 test"), level)

    def test_sdk_formats(self):
        self.assertEqual(classify_line("\x1b[31mE (123) Wire: failed\x1b[0m"), "ERROR")
        self.assertEqual(classify_line("W (42) uart: overflow"), "WARN")
        self.assertEqual(classify_line("[ 123][W][main.cpp:1] warning"), "WARN")

    def test_boot_and_protocol(self):
        self.assertEqual(classify_line("Backtrace: 0x123"), "OTHER")
        self.assertEqual(classify_line("?id=S01&status=1"), "PROTOCOL")

    def test_fragmented_lines(self):
        buffer = LineBuffer()
        self.assertEqual(buffer.feed(b"LOG [1][R"), [])
        self.assertEqual(buffer.feed(b"X] test\r\nsecond\npartial"),
                         ["LOG [1][RX] test", "second"])
        self.assertEqual(buffer.feed(b"\n"), ["partial"])

    def test_fragmented_utf8(self):
        buffer = LineBuffer()
        data = "Žluťoučký\n".encode()
        self.assertEqual(buffer.feed(data[:1]), [])
        self.assertEqual(buffer.feed(data[1:]), ["Žluťoučký"])

    def test_invalid_bytes(self):
        self.assertIn("\ufffd", LineBuffer().feed(b"\xff\n")[0])

    def test_bounded_input(self):
        buffer = LineBuffer(limit=4)
        self.assertEqual(buffer.feed(b"123456"), ["1234 [monitor: truncated]"])
        self.assertEqual(buffer.feed(b"ok\n"), ["ok"])
        self.assertEqual(buffer.feed(b"123456\n"), ["1234 [monitor: truncated]"])

    def test_cli(self):
        args = create_parser().parse_args(["--port", "COM4", "--levels", "error,warn",
                                         "--timestamps", "--log", "monitor.log"])
        self.assertEqual(args.levels, {"ERROR", "WARN"})
        self.assertEqual(args.baud, 115200)

    def test_read_only_monitor_and_cleanup(self):
        class FakeSerial:
            in_waiting = 0
            closed = False
            reads = 0

            def open(self):
                self.opened = True

            def read(self, size):
                self.reads += 1
                if self.reads == 1:
                    return b"LOG [1][RX] test\nLOG [2][ERROR] bad\n"
                raise KeyboardInterrupt()

            def close(self):
                self.closed = True

        connection = FakeSerial()  # No write method: sending would fail this test.
        serial_module = SimpleNamespace(
            Serial=lambda **kwargs: connection, EIGHTBITS=8, PARITY_NONE="N", STOPBITS_ONE=1,
        )
        output = io.StringIO()
        with patch.dict(sys.modules, {"serial": serial_module}), redirect_stdout(output), redirect_stderr(io.StringIO()):
            self.assertEqual(main(["--port", "COM4", "--levels", "ERROR"]), 0)
        self.assertEqual(output.getvalue(), "LOG [2][ERROR] bad\n")
        self.assertTrue(connection.closed)
        self.assertFalse(connection.dtr)
        self.assertFalse(connection.rts)


if __name__ == "__main__":
    unittest.main()

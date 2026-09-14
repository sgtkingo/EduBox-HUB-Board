import unittest

from emulator import EmulatorError, build_request, parse_assignments, parse_response


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


if __name__ == "__main__":
    unittest.main()

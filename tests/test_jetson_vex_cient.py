"""Transport tests using a fake serial port; no robot or PySerial required."""

import contextlib
import io
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from jetson_vex_cient import V5Drive
from drive_example import main


class ClientTests(unittest.TestCase):
    def setUp(self):
        self.port = Mock()
        self.port.write.side_effect = len
        self.serial = types.SimpleNamespace(Serial=Mock(return_value=self.port))
        patcher = patch.dict(sys.modules, {"serial": self.serial})
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_drive_and_stop(self):
        with V5Drive("/dev/test") as brain:
            brain.drive(0.25, -0.5)
        self.assertEqual([call.args[0] for call in self.port.write.call_args_list],
                         [b"MOTOR_SPEED,0.250000,-0.500000\n", b"STOP_ALL\n"])
        self.port.close.assert_called_once()

    def test_invalid_speed_never_sends_motion(self):
        with V5Drive("/dev/test") as brain:
            for value in (float("nan"), float("inf"), -1.1, 1.1):
                with self.assertRaises(ValueError):
                    brain.drive(0, value)
            self.port.write.assert_not_called()

    def test_interrupt_attempts_stop(self):
        with self.assertRaises(KeyboardInterrupt):
            with V5Drive("/dev/test") as brain:
                brain.drive(0.25, 0.25)
                raise KeyboardInterrupt()
        self.assertEqual(self.port.write.call_args.args[0], b"STOP_ALL\n")
        self.port.close.assert_called_once()

    def test_disconnect_reports_stop_failure_and_closes(self):
        self.port.write.side_effect = OSError("disconnected")
        with contextlib.redirect_stderr(io.StringIO()) as errors:
            with self.assertRaises(OSError):
                with V5Drive("/dev/test"):
                    pass
        self.assertIn("Could not send STOP_ALL", errors.getvalue())
        self.port.close.assert_called_once()

    def test_partial_write_attempts_stop(self):
        self.port.write.side_effect = [1, len(b"STOP_ALL\n")]
        with self.assertRaises(OSError):
            with V5Drive("/dev/test") as brain:
                brain.drive(0.25, 0.25)
        self.assertEqual(self.port.write.call_args.args[0], b"STOP_ALL\n")
        self.port.close.assert_called_once()

    def test_example_drives_for_one_second_then_stops(self):
        with patch("drive_example.time.sleep") as sleep:
            main()
        sleep.assert_called_once_with(1)
        self.assertEqual([call.args[0] for call in self.port.write.call_args_list],
                         [b"MOTOR_SPEED,0.250000,0.250000\n", b"STOP_ALL\n"])
        self.port.close.assert_called_once()


if __name__ == "__main__":
    unittest.main()

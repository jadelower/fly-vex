"""USB serial client for the fly-vex drive-only firmware."""

import math
import sys


def speed_fraction(value):
    """Reject invalid inputs before sending a motor command."""
    value = float(value)
    if not math.isfinite(value) or not -1.0 <= value <= 1.0:
        raise ValueError("speed must be finite and between -1 and 1")
    return value


class V5Drive:
    """Use as a context manager; exit attempts STOP_ALL before closing USB.

    The firmware provides no acknowledgements or disconnect watchdog.
    A completed write does not confirm that the Brain applied the command.
    """

    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self._serial = None

    def __enter__(self):
        if self._serial is not None:
            raise RuntimeError("connection is already open")
        import serial

        self._serial = serial.Serial(
            self.port, self.baudrate, timeout=1, write_timeout=1
        )
        return self

    def _send(self, command):
        if self._serial is None:
            raise RuntimeError("use V5Drive inside a with block")
        data = (command + "\n").encode("ascii")
        if self._serial.write(data) != len(data):
            raise OSError("incomplete serial write")
        self._serial.flush()

    def drive(self, left, right):
        """Set left/right speed fractions; 1.0 currently means 200 RPM."""
        left, right = speed_fraction(left), speed_fraction(right)
        self._send("MOTOR_SPEED,{:.6f},{:.6f}".format(left, right))

    def stop(self):
        """Brake both drive motors."""
        self._send("STOP_ALL")

    def __exit__(self, exc_type, exc_value, traceback):
        try:
            self.stop()
        except OSError as error:
            print("Could not send STOP_ALL: {}".format(error), file=sys.stderr)
            if exc_type is None:
                raise
        finally:
            self._serial.close()
            self._serial = None
        return False

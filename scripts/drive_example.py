#!/usr/bin/env python3
"""Drive forward for one second, then stop."""

import time

from jetson_vex_cient import V5Drive


def main():
    # Change the port here if your Brain uses a different USB user serial device.
    # "/dev/ttyACM1" for Linux, "/dev/cu.usbmodem1103" for Mac, "COM3" for Windows.
    with V5Drive("/dev/cu.usbmodem1103") as brain:
        brain.drive(0.75, 0.75)
        time.sleep(1)
    # Leaving the with block stops both motors and closes the connection.


if __name__ == "__main__":
    main()

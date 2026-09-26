#!/usr/bin/env python3
"""Drive forward for one second, then stop."""

import time

from jetson_vex_cient import V5Drive


def main():
    # Change the port here if your Brain uses a different USB user serial device.
    with V5Drive("/dev/ttyACM1") as brain:
        brain.drive(0.25, 0.25)
        time.sleep(1)
    # Leaving the with block stops both motors and closes the connection.


if __name__ == "__main__":
    main()

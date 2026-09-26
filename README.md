# fly-vex

PROS / EZ-Template project with the serial motor receiver ported from
`vex-ai-2025/v5_controller_test`. The Jetson runs vision and ROS 2; the V5 Brain
executes newline-terminated commands received through its USB user port.

`include/robot_config.hpp` enables Jetson control by default. Set
`FLY_VEX_JETSON_CONTROL` to 0 to build the original EZ-Template example in
`src/main.cpp` and `src/autons.cpp`. Configure its sample wiring before use.
The two modes are mutually exclusive so joystick/PID commands do not overwrite
Jetson commands. Jetson mode does not require an IMU.

## Wiring and protocol

Both drive motors are currently configured as blue 6:1 cartridges (600 RPM).
The protocol's speed conversion still maps `1.0` to **200 RPM**, so `0.25`
commands 50 RPM and `0.5` commands 100 RPM. Match the motor configuration to
your physical cartridges before running.

| Motor | Smart port | Direction |
| --- | --- | --- |
| Left drive | 10 | Reversed (`-10` in code) |
| Right drive | 13 | Normal |

Commands are ASCII with a trailing newline; CRLF also works:

| Command | Behavior |
| --- | --- |
| `MOTOR_SPEED,0.5,0.5` | Both drive motors at 100 RPM |
| `STOP_ALL` | Brake all motors |

Speed fields are ordered **left, right**, clamped to [-1, 1]. Zero brakes that motor.
Legacy four-value commands such as `MOTOR_SPEED,0.5,0.5,0,0` are also accepted;
the last two fields are validated but ignored. `MOTOR_GOTO` is no longer supported.
Motors use hold braking. Malformed commands
and lines longer than 127 characters are discarded. The Brain displays the
last accepted motion command; no serial acknowledgement/telemetry is implemented.
Motion commands are ignored while competition control reports disabled.

There is no disconnect watchdog: speed commands persist until replaced or stopped.

## Build and upload from a Mac

Install the PROS VS Code extension and complete its CLI and ARM toolchain setup.
PROS recommends the extension for installation; see the
[official macOS instructions](https://pros.cs.purdue.edu/v5/getting-started/macos.html).
Open this project folder in VS Code and use the PROS terminal.

Confirm that `include/robot_config.hpp` contains:

```cpp
#define FLY_VEX_JETSON_CONTROL 1
```

This is a compile-time setting, not a switch in the Brain menu. Rebuild and
upload whenever you change it. With the terminal in the project root, build:

```sh
pros make
```

Power on the Brain with its battery connected, then connect it to the Mac with
a USB data cable. After the build succeeds, upload:

```sh
pros upload --slot 8
```

This replaces any program in slot **8**, matching the saved upload slot in
`project.pros`. Choose another slot if needed.
See the [PROS upload documentation](https://pros.cs.purdue.edu/v5/tutorials/walkthrough/uploading.html).
If `pros` or `arm-none-eabi-g++` is not found, complete the extension's toolchain
setup and reopen its terminal. Close other applications using the Brain's
serial connection before uploading.

## Run on the V5 Brain

1. Connect the left drive motor to smart port **10** and the right to **13**.
2. On the Brain, open the program in slot **8** and press **Run**.
3. Confirm the screen displays:

   ```text
   fly-vex: Jetson controller
   Ready to receive commands...
   ```

4. Connect the Brain's USB cable to the Jetson.

The program now waits for USB commands. Joystick driving and the original
EZ-Template autonomous examples are inactive in Jetson mode. If connected to
competition control, the robot must be enabled for motion commands to work.
Both the autonomous and driver-control callbacks wait for Jetson commands.

The left drive motor is reversed; the right uses normal direction. If a motor spins
the wrong way for forward travel, reverse that motor's port sign in
`src/jetson_controller.cpp` (for example, `13` to `-13`), then rebuild and upload.

## Send commands from the Jetson

The Python scripts run on the Jetson and do not require ROS:

| File | Purpose |
| --- | --- |
| [scripts/jetson_vex_cient.py](scripts/jetson_vex_cient.py) | Reusable `V5Drive` client: opens USB, sends drive/stop commands, and attempts to stop before closing. |
| [scripts/drive_example.py](scripts/drive_example.py) | Simple example: drive forward for one second, then stop. No command-line arguments. |
| [scripts/requirements.txt](scripts/requirements.txt) | PySerial dependency for the Jetson scripts. |
| [tests/test_jetson_vex_cient.py](tests/test_jetson_vex_cient.py) | Client and example tests using a simulated serial port. |

Copy or clone this project onto the Jetson. From its root directory, install
the Python dependency in a virtual environment:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r scripts/requirements.txt
python -m serial.tools.list_ports -v
```

Select the Brain's USB **user serial port**, often `/dev/ttyACM1`; enumeration
can vary, so verify your device. Do not have another serial terminal or ROS
node using the same connection. PySerial supports listing ports and writing
bytes directly; see its [documentation](https://pyserial.readthedocs.io/en/latest/shortintro.html).

With the drive wheels lifted, run:

```sh
python scripts/drive_example.py
```

The example takes no arguments: it commands both motors forward at **150 RPM
for one second** (`0.75` per side), then stops. It currently uses the Mac user
serial port `/dev/cu.usbmodem1103`. On the Jetson, change that string in
`scripts/drive_example.py` to your Brain's user port, often `/dev/ttyACM1`.
Port names can change when reconnecting the Brain; verify yours before running.

The Brain displays the accepted speed command. Leaving the client's `with`
block sends `STOP_ALL` and closes the connection, including when Ctrl+C or an
exception interrupts the example. A lost USB connection can prevent the stop
from arriving.

To reuse the client in another Python script placed inside `scripts/`:

```python
import time
from jetson_vex_cient import V5Drive

# Replace with your Brain's USB user serial port.
with V5Drive("/dev/ttyACM1") as brain:
    brain.drive(0.25, 0.25)
    time.sleep(1)
    brain.stop()  # Optional explicit stop; exiting the with block also stops.
```

The client uses 115200 baud and one-second serial read/write timeouts. It adds
the newline to each command and does not wait for a firmware acknowledgement.
`brain.drive(left, right)` accepts finite speed fractions from -1 to 1;
`brain.stop()` brakes both motors. Import `V5Drive` from `jetson_vex_cient`
when writing your own Jetson control code.

Other commands you can send are:

```text
MOTOR_SPEED,0.5,0.5
MOTOR_SPEED,-0.25,-0.25
MOTOR_SPEED,0,0
STOP_ALL
```

Append a newline (`\n`) to every command. The firmware does not send serial
acknowledgements, so do not wait for a reply before sending the next command.
**Send `STOP_ALL` before disconnecting: unplugging USB does not automatically
stop motion.**

If using the external `ros_vs_controller_node.py` bridge, select the user serial
device with its `port_name` ROS parameter. That bridge is not included here.
Its legacy four-value speed commands are accepted, with the last two values
ignored. The bridge normalizes RPS using 600 RPM, while this firmware's protocol
conversion uses 200 RPM; account for that mismatch when calibrating velocity.

If nothing moves, check that the program is running, the Brain shows the ready
message, the correct user serial port is selected, commands include a newline,
and competition control is not reporting disabled. `Invalid serial command` on
the Brain indicates that the received command did not pass validation.

## Validation

Host tests cover command parsing, RPM conversion, fragmented lines, CRLF, and
recovery after oversized input:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/jetson_protocol_test.cpp -o /tmp/fly-vex-protocol-test
/tmp/fly-vex-protocol-test
```

The six tests in [test_jetson_vex_cient.py](tests/test_jetson_vex_cient.py)
use a simulated serial port and require no PySerial or connected Brain.
They cover command formatting, invalid input, Ctrl+C cleanup,
partial writes, disconnect failures, and the one-second drive example:

```sh
python3 -m unittest discover -s tests -p 'test_jetson_vex_cient.py'
```

Build and host tests do not verify USB reception or motor motion on real hardware.

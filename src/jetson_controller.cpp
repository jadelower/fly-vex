#include "robot_config.hpp"

#if FLY_VEX_JETSON_CONTROL
#include "main.h"
#include "jetson_protocol.hpp"
#include "pros/apix.h"

#include <cstdio>
#include <mutex>

namespace {
pros::Motor left_motor(10, pros::MotorGears::blue, pros::MotorUnits::degrees);
pros::Motor right_motor(13, pros::MotorGears::blue, pros::MotorUnits::degrees);
pros::Motor* motors[] = {&left_motor, &right_motor};
pros::Mutex motor_mutex;

void stop_all() {
  for (auto motor : motors) motor->brake();
}

void handle_line(const std::string& line) {
  jetson::Command command{};
  if (!jetson::parse(line, command)) {
    pros::lcd::set_text(2, "Invalid serial command");
    return;
  }
  std::lock_guard<pros::Mutex> lock(motor_mutex);
  // A background receiver survives mode changes. Never apply motion while disabled.
  if (pros::competition::is_disabled() || command.type == jetson::Type::stop) {
    stop_all();
    return;
  }
  pros::lcd::set_text(2, line);
  if (command.type == jetson::Type::speed) {
    for (std::size_t i = 0; i < command.values.size(); ++i) {
      const int rpm = jetson::speed_rpm(command.values[i]);
      if (rpm == 0) motors[i]->brake();
      else motors[i]->move_velocity(rpm);
    }
  }
}

void receive_serial() {
  jetson::LineBuffer lines;
  while (true) {
    const int c = std::fgetc(stdin);
    if (c == EOF) {
      std::clearerr(stdin);
      pros::delay(10);
    } else {
      lines.push(static_cast<char>(c), handle_line);
    }
  }
}

void wait_for_commands() {
  while (true) pros::delay(20);
}
}  // namespace

void initialize() {
  pros::lcd::initialize();
  pros::lcd::set_text(0, "fly-vex: Jetson controller");
  // Raw USB user-port text, compatible with the existing PySerial bridge.
  pros::c::serctl(SERCTL_DISABLE_COBS, nullptr);
  for (auto motor : motors) motor->set_brake_mode(pros::MotorBrake::hold);
  stop_all();
  pros::lcd::set_text(1, "Ready to receive commands...");
  // Persistent task: don't delete a task blocked in stdin during mode changes.
  static pros::Task receiver(receive_serial);
}

void disabled() {
  std::lock_guard<pros::Mutex> lock(motor_mutex);
  stop_all();
}
void competition_initialize() {}
void autonomous() { wait_for_commands(); }
void opcontrol() { wait_for_commands(); }
#endif  // FLY_VEX_JETSON_CONTROL

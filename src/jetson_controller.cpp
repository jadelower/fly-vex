#include "robot_config.hpp"

#if FLY_VEX_JETSON_CONTROL

#include "main.h"
#include "jetson_protocol.hpp"
#include "pros/apix.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>

namespace {

constexpr std::uint32_t COMMAND_TIMEOUT_MS = 300;

pros::Motor left_motor(
    -10,
    pros::MotorGears::blue,
    pros::MotorUnits::degrees
);

pros::Motor right_motor(
    13,
    pros::MotorGears::blue,
    pros::MotorUnits::degrees
);

pros::Motor* motors[] = {
    &left_motor,
    &right_motor
};

pros::Mutex motor_mutex;

std::atomic<std::uint32_t> last_command_time{0};
std::atomic<bool> command_active{false};


void stop_all() {
    for (auto motor : motors) {
        motor->brake();
    }
}


void handle_line(const std::string& line) {
    jetson::Command command{};

    if (!jetson::parse(line, command)) {
        pros::lcd::set_text(2, "Invalid serial command");
        return;
    }

    std::lock_guard<pros::Mutex> lock(motor_mutex);

    if (
        pros::competition::is_disabled()
        || command.type == jetson::Type::stop
    ) {
        stop_all();
        command_active.store(false);
        pros::lcd::set_text(2, "Motors stopped");
        return;
    }

    if (command.type == jetson::Type::speed) {
        for (
            std::size_t i = 0;
            i < command.values.size();
            ++i
        ) {
            const int rpm =
                jetson::speed_rpm(command.values[i]);

            if (rpm == 0) {
                motors[i]->brake();
            } else {
                motors[i]->move_velocity(rpm);
            }
        }

        last_command_time.store(pros::millis());
        command_active.store(true);

        pros::lcd::set_text(2, line);
    }
}


void receive_serial() {
    jetson::LineBuffer lines;

    while (true) {
        const int character = std::fgetc(stdin);

        if (character == EOF) {
            std::clearerr(stdin);
            pros::delay(10);
        } else {
            lines.push(
                static_cast<char>(character),
                handle_line
            );
        }
    }
}


void watchdog() {
    while (true) {
        if (command_active.load()) {
            const std::uint32_t now = pros::millis();
            const std::uint32_t last =
                last_command_time.load();

            if (now - last > COMMAND_TIMEOUT_MS) {
                std::lock_guard<pros::Mutex> lock(
                    motor_mutex
                );

                // Check again after obtaining the lock.
                const std::uint32_t checked_now =
                    pros::millis();

                const std::uint32_t checked_last =
                    last_command_time.load();

                if (
                    command_active.load()
                    && checked_now - checked_last
                        > COMMAND_TIMEOUT_MS
                ) {
                    stop_all();
                    command_active.store(false);

                    pros::lcd::set_text(
                        2,
                        "WATCHDOG: motors stopped"
                    );
                }
            }
        }

        pros::delay(20);
    }
}


void wait_for_commands() {
    while (true) {
        pros::delay(20);
    }
}

}  // namespace


void initialize() {
    pros::lcd::initialize();
    pros::lcd::set_text(
        0,
        "fly-vex: Jetson controller"
    );

    pros::c::serctl(
        SERCTL_DISABLE_COBS,
        nullptr
    );

    for (auto motor : motors) {
        motor->set_brake_mode(
            pros::MotorBrake::hold
        );
    }

    stop_all();

    pros::lcd::set_text(
        1,
        "Ready to receive commands..."
    );

    static pros::Task receiver_task(
        receive_serial
    );

    static pros::Task watchdog_task(
        watchdog
    );
}


void disabled() {
    std::lock_guard<pros::Mutex> lock(
        motor_mutex
    );

    stop_all();
    command_active.store(false);
}


void competition_initialize() {}


void autonomous() {
    wait_for_commands();
}


void opcontrol() {
    wait_for_commands();
}

#endif  // FLY_VEX_JETSON_CONTROL
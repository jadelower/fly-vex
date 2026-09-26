#include "jetson_protocol.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
  jetson::Command c{};
  assert(jetson::parse("MOTOR_SPEED,0.5,-0.5", c));
  assert(c.type == jetson::Type::speed);
  assert(jetson::speed_rpm(c.values[0]) == 100);
  assert(jetson::speed_rpm(c.values[1]) == -100);
  assert(jetson::speed_rpm(4) == 200);
  assert(jetson::speed_rpm(-4) == -200);
  assert(jetson::parse("MOTOR_SPEED,0.5,-0.5,1,-1", c));
  assert(c.values.size() == 2 && c.values[0] == 0.5 && c.values[1] == -0.5);
  assert(jetson::parse("STOP_ALL", c) && c.type == jetson::Type::stop);
  for (const auto* invalid : {"", "UNKNOWN", "STOP_ALL,1", "MOTOR_SPEED,1,2,3",
      "MOTOR_SPEED,1", "MOTOR_SPEED,1,", "MOTOR_SPEED,nan,0", "MOTOR_SPEED,0,inf",
      "MOTOR_SPEED,0,0,nan,0", "MOTOR_SPEED,0,0,0,inf",
      "MOTOR_GOTO,1,270,50", "MOTOR_GOTO,2,-30,0",
      "MOTOR_SPEED,1,2,3,4,5", "MOTOR_SPEED,1,,3,4", "MOTOR_SPEED,nan,0,0,0",
      "MOTOR_SPEED,inf,0,0,0", "MOTOR_SPEED,1e999,0,0,0", "MOTOR_SPEED,1x,0,0,0",
      "MOTOR_GOTO,0,90,50", "MOTOR_GOTO,3,90,50", "MOTOR_GOTO,1.5,90,50",
      "MOTOR_GOTO,1,90,-50", "MOTOR_GOTO,1,90,50,"}) {
    assert(!jetson::parse(invalid, c));
  }
  jetson::LineBuffer buffer;
  std::vector<std::string> lines;
  auto collect = [&](const std::string& s) { lines.push_back(s); };
  for (char ch : std::string("MOTOR_SP")) buffer.push(ch, collect);
  assert(lines.empty());
  for (char ch : std::string("EED,0.5,-0.5\r\nSTOP_ALL\n")) buffer.push(ch, collect);
  assert(lines.size() == 2 && lines[0] == "MOTOR_SPEED,0.5,-0.5" && lines[1] == "STOP_ALL");
  for (char ch : std::string(128, 'x') + "STOP_ALL\nMOTOR_SPEED,0,0\n")
    buffer.push(ch, collect);
  assert(lines.size() == 3 && lines.back() == "MOTOR_SPEED,0,0");
  std::cout << "Jetson protocol tests passed\n";
}

#pragma once

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <string>

namespace jetson {
enum class Type { speed, stop };
struct Command {
  Type type;
  std::array<double, 2> values{};
};

// Validate the entire command before changing any motor.
inline bool parse(const std::string& line, Command& result) {
  std::array<std::string, 5> fields;
  std::size_t count = 0, start = 0;
  while (true) {
    if (count == fields.size()) return false;
    const auto end = line.find(',', start);
    fields[count++] = line.substr(start, end == std::string::npos ? end : end - start);
    if (end == std::string::npos) break;
    start = end + 1;
  }

  Command command{};
  if (fields[0] == "STOP_ALL" && count == 1) {
    command.type = Type::stop;
  } else if (fields[0] == "MOTOR_SPEED" && (count == 3 || count == 5)) {
    command.type = Type::speed;
  } else {
    return false;
  }
  for (std::size_t i = 1; i < count; ++i) {
    char* end = nullptr;
    errno = 0;
    const char* begin = fields[i].c_str();
    const double value = std::strtod(begin, &end);
    if (end == begin || end != begin + fields[i].size() || errno == ERANGE || !std::isfinite(value))
      return false;
    // Validate but ignore the two unused fields sent by the legacy Jetson bridge.
    if (i <= command.values.size()) command.values[i - 1] = value;
  }
  result = command;
  return true;
}

// Original VEX motors use ratio18_1: 100% velocity = 200 RPM.
inline int speed_rpm(double fraction) {
  return static_cast<int>(std::lround(std::clamp(fraction, -1.0, 1.0) * 200.0));
}

// Keep fragmented USB reads together. Discard overlong lines in their entirety.
class LineBuffer {
 public:
  template <typename Handler>
  void push(char c, Handler handle) {
    if (c == '\r' || c == '\n') {
      if (!overflow_ && !line_.empty()) handle(line_);
      line_.clear();
      overflow_ = false;
    } else if (!overflow_) {
      if (line_.size() == 127) {
        overflow_ = true;
        line_.clear();
      } else {
        line_ += c;
      }
    }
  }

 private:
  std::string line_;
  bool overflow_ = false;
};
}  // namespace jetson

#pragma once
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <thread>

class String : public std::string {
public:
  using std::string::string;
  String() = default;
  String(const std::string& value) : std::string(value) {}
  String(unsigned int value) : std::string(std::to_string(value)) {}
  char charAt(size_t index) const { return at(index); }
  int indexOf(char value, unsigned int offset = 0) const {
    auto index = find(value, offset); return index == npos ? -1 : static_cast<int>(index);
  }
  String substring(unsigned int start, unsigned int end) const { return substr(start, end - start); }
  String substring(unsigned int start) const { return substr(start); }
  void remove(unsigned int start, unsigned int count) { erase(start, count); }
  bool startsWith(const char* prefix) const { return find(prefix) == 0; }
  void trim() {
    auto first = find_first_not_of(" \r\n\t"), last = find_last_not_of(" \r\n\t");
    *this = first == npos ? "" : substr(first, last - first + 1);
  }
  void toUpperCase() { for (auto& ch : *this) ch = static_cast<char>(std::toupper(ch)); }
  void toLowerCase() { for (auto& ch : *this) ch = static_cast<char>(std::tolower(ch)); }
  void replace(const char* from, const char* to) {
    for (auto pos = find(from); pos != npos; pos = find(from)) std::string::replace(pos, std::string(from).size(), to);
  }
  long toInt() const { return std::strtol(c_str(), nullptr, 10); }
};
inline String operator+(const String& left, unsigned int right) { return String(std::string(left) + std::to_string(right)); }
inline long map(long value, long inMin, long inMax, long outMin, long outMax) {
  return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}
template<class T> T constrain(T value, T minimum, T maximum) { return std::max(minimum, std::min(value, maximum)); }
inline bool isDigit(char value) { return value >= '0' && value <= '9'; }
inline unsigned long millis() {
  return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1;
inline std::map<int, int> pinValues, pinModes;
inline int toneStarts = 0, servoStarts = 0, servoWrites = 0;
inline void pinMode(int pin, int mode) { pinModes[pin] = mode; }
inline void digitalWrite(int pin, int value) { pinValues[pin] = value; }
inline void analogWrite(int pin, int value) { pinValues[pin] = value; }
inline void ledcDetachPin(int) {}
inline void noTone(int pin) { pinValues[pin] = LOW; }
inline void tone(int pin, int, int) { ++toneStarts; pinValues[pin] = HIGH; }

/**
 * @file vscp_platform.hpp
 * @brief Minimal text, clock, and sleep abstraction for Arduino and desktop builds.
 */

#pragma once

#include "config.hpp"

#include <cstddef>
#include <cctype>

#ifdef ARDUINO_H_ENV
#include <Arduino.h>
#else
#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#endif

namespace vscp {

#ifdef ARDUINO_H_ENV
using String = ::String;
#else
using String = std::string;
#endif

namespace detail {

constexpr size_t STRING_NOT_FOUND = static_cast<size_t>(-1);

inline size_t stringLength(const String& value) {
  return value.length();
}

inline char stringCharacter(const String& value, size_t index) {
#ifdef ARDUINO_H_ENV
  return value.charAt(index);
#else
  return value[index];
#endif
}

inline size_t stringFind(const String& value, char character, size_t offset) {
#ifdef ARDUINO_H_ENV
  const int result = value.indexOf(character, static_cast<unsigned int>(offset));
  return result < 0 ? STRING_NOT_FOUND : static_cast<size_t>(result);
#else
  const size_t result = value.find(character, offset);
  return result == String::npos ? STRING_NOT_FOUND : result;
#endif
}

inline String stringSubstring(const String& value, size_t start, size_t end) {
#ifdef ARDUINO_H_ENV
  return value.substring(static_cast<unsigned int>(start), static_cast<unsigned int>(end));
#else
  return value.substr(start, end - start);
#endif
}

inline void trimString(String& value) {
#ifdef ARDUINO_H_ENV
  value.trim();
#else
  const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char character) {
    return std::isspace(character) != 0;
  });
  const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char character) {
    return std::isspace(character) != 0;
  }).base();
  value = first < last ? String(first, last) : String();
#endif
}

inline String stripMessage(const String& input, bool trim = true) {
  String output;
  output.reserve(stringLength(input));

  for (size_t index = 0; index < stringLength(input); ++index) {
    const char character = stringCharacter(input, index);
    if (character >= 32 && character <= 126) output += character;
  }

  if (trim) trimString(output);
  return output;
}

inline void uppercaseString(String& value) {
#ifdef ARDUINO_H_ENV
  value.toUpperCase();
#else
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    return static_cast<char>(std::toupper(character));
  });
#endif
}

inline const char* stringData(const String& value) {
  return value.c_str();
}

inline unsigned long monotonicMilliseconds() {
#ifdef ARDUINO_H_ENV
  return millis();
#else
  const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<unsigned long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
#endif
}

inline void sleepMilliseconds(unsigned long milliseconds) {
#ifdef ARDUINO_H_ENV
  delay(milliseconds);
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

}  // namespace detail
}  // namespace vscp

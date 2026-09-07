/**
 * @file vscp_types.hpp
 * @brief Shared protocol constants, commands, requests, and responses.
 */

#pragma once

#include <Arduino.h>
#include <map>

namespace vscp {

constexpr const char* API_VERSION = "1.4";
constexpr size_t MAX_MESSAGE_SIZE = 1024;
constexpr unsigned long DEFAULT_TIMEOUT_MS = 500;

using Parameters = std::map<String, String>;

enum class Command : uint8_t {
  Unknown,
  Init,
  Connect,
  Disconnect,
  Update,
  Config,
  Control,
  Reset
};

enum class Status : uint8_t {
  Error = 0,
  Ok = 1
};

struct Request {
  Command command = Command::Unknown;
  Parameters parameters;

  String value(const String& key) const;
  bool has(const String& key) const;
};

struct Response {
  Status status = Status::Error;
  String error;
  Parameters parameters;

  static Response ok();
  static Response fail(const String& errorMessage);
};

struct ResponseStatus {
  Status status = Status::Error;
  String error;
  Parameters parameters;
};

const char* commandName(Command command);
Command commandFromName(String name);

}  // namespace vscp

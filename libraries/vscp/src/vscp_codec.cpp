/**
 * @file vscp_codec.cpp
 * @brief Implementation of the shared VSCP message codec.
 */

#include "vscp_codec.hpp"

namespace vscp {

bool Codec::parseParameters(const String& message, Parameters& parameters, String& error) {
  parameters.clear();
  error = "";

  if (detail::stringLength(message) == 0 || detail::stringCharacter(message, 0) != '?') {
    error = "Message must start with ?";
    return false;
  }
  if (detail::stringLength(message) > MAX_MESSAGE_SIZE) {
    error = "Message too long";
    return false;
  }

  size_t cursor = 1;
  while (cursor < detail::stringLength(message)) {
    size_t separator = detail::stringFind(message, '&', cursor);
    if (separator == detail::STRING_NOT_FOUND) separator = detail::stringLength(message);

    const size_t equals = detail::stringFind(message, '=', cursor);
    if (equals == detail::STRING_NOT_FOUND || equals <= cursor || equals >= separator) {
      error = "Malformed parameter";
      return false;
    }

    String key = detail::stringSubstring(message, cursor, equals);
    String value = detail::stringSubstring(message, equals + 1, separator);
    detail::trimString(key);
    detail::trimString(value);
    if (detail::stringLength(key) == 0) {
      error = "Empty parameter name";
      return false;
    }
    parameters[key] = value;
    cursor = separator + 1;
  }

  return true;
}

bool Codec::parseRequest(const String& message, Request& request, String& error) {
  request = Request();
  if (!parseParameters(message, request.parameters, error)) return false;

  const auto type = request.parameters.find("type");
  if (type == request.parameters.end()) {
    error = "Missing type";
    return false;
  }
  request.command = commandFromName(type->second);
  return true;
}

bool Codec::parseResponse(const String& message, ResponseStatus& response, String& error) {
  response = ResponseStatus();
  if (!parseParameters(message, response.parameters, error)) return false;

  const auto status = response.parameters.find("status");
  if (status == response.parameters.end()) {
    error = "Missing status";
    return false;
  }

  response.status = status->second == "1" ? Status::Ok : Status::Error;
  const auto responseError = response.parameters.find("error");
  if (responseError != response.parameters.end()) response.error = responseError->second;
  return true;
}

String Codec::buildParameters(const Parameters& parameters) {
  String message = "?";
  bool first = true;
  for (const auto& parameter : parameters) {
    if (!first) message += '&';
    message += parameter.first;
    message += '=';
    message += parameter.second;
    first = false;
  }
  return message;
}

String Codec::buildRequest(Command command, const Parameters& parameters) {
  String message = "?type=";
  message += commandName(command);
  for (const auto& parameter : parameters) {
    if (parameter.first == "type") continue;
    message += '&';
    message += parameter.first;
    message += '=';
    message += parameter.second;
  }
  return message;
}

String Codec::buildResponse(const Response& response) {
  Parameters responseParameters = response.parameters;
  responseParameters["status"] = response.status == Status::Ok ? "1" : "0";
  if (response.status == Status::Error && detail::stringLength(response.error) > 0) {
    responseParameters["error"] = response.error;
  }
  return buildParameters(responseParameters);
}

}  // namespace vscp

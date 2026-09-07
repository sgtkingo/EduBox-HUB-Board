/**
 * @file vscp_codec.cpp
 * @brief Implementation of the shared VSCP message codec.
 */

#include "vscp_codec.hpp"

namespace vscp {

bool Codec::parseParameters(const String& message, Parameters& parameters, String& error) {
  parameters.clear();
  error = "";

  if (message.length() == 0 || message.charAt(0) != '?') {
    error = "Message must start with ?";
    return false;
  }
  if (message.length() > MAX_MESSAGE_SIZE) {
    error = "Message too long";
    return false;
  }

  int cursor = 1;
  while (cursor < message.length()) {
    int separator = message.indexOf('&', cursor);
    if (separator < 0) separator = message.length();

    const int equals = message.indexOf('=', cursor);
    if (equals <= cursor || equals >= separator) {
      error = "Malformed parameter";
      return false;
    }

    String key = message.substring(cursor, equals);
    String value = message.substring(equals + 1, separator);
    key.trim();
    value.trim();
    if (key.length() == 0) {
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
  Parameters requestParameters = parameters;
  requestParameters["type"] = commandName(command);
  return buildParameters(requestParameters);
}

String Codec::buildResponse(const Response& response) {
  Parameters responseParameters = response.parameters;
  responseParameters["status"] = response.status == Status::Ok ? "1" : "0";
  if (response.status == Status::Error && response.error.length() > 0) {
    responseParameters["error"] = response.error;
  }
  return buildParameters(responseParameters);
}

}  // namespace vscp

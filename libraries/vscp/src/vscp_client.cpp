/**
 * @file vscp_client.cpp
 * @brief Implementation of the transport-injected VSCP request client.
 */

#include "vscp_client.hpp"

namespace vscp {

Client::Client(Transport& transport, unsigned long timeoutMs)
    : transport_(transport), timeoutMs_(timeoutMs) {}

ResponseStatus Client::transact(Command command, Parameters parameters, bool requiresInit, const String& expectedId) {
  ResponseStatus result;
  if (requiresInit && !initialized_) {
    result.error = "Protocol not initialized";
    return result;
  }

  transport_.writeLine(Codec::buildRequest(command, parameters));
  const unsigned long startedAt = detail::monotonicMilliseconds();
  while (detail::monotonicMilliseconds() - startedAt < timeoutMs_) {
    String message;
    const ReadStatus readStatus = transport_.readLine(message);
    if (readStatus == ReadStatus::NoData) {
      detail::sleepMilliseconds(1);
      continue;
    }
    if (readStatus == ReadStatus::MessageTooLong) {
      result.error = "Response too long";
      return result;
    }

    String parseError;
    if (!Codec::parseResponse(message, result, parseError)) {
      result.error = parseError;
      return result;
    }
    if (detail::stringLength(expectedId) > 0) {
      const auto responseId = result.parameters.find("id");
      if (responseId == result.parameters.end() || responseId->second != expectedId) {
        result.status = Status::Error;
        result.error = "Response UID mismatch";
      }
    }
    return result;
  }

  result.error = "Response timeout";
  return result;
}

ResponseStatus Client::init(const String& application, const String& databaseVersion) {
  Parameters parameters;
  parameters["api"] = API_VERSION;
  if (detail::stringLength(application) > 0) parameters["app"] = application;
  if (detail::stringLength(databaseVersion) > 0) parameters["db"] = databaseVersion;
  ResponseStatus response = transact(Command::Init, parameters, false);
  initialized_ = response.status == Status::Ok;
  return response;
}

ResponseStatus Client::connect(const String& uid, const String& pins) {
  Parameters parameters{{"id", uid}, {"pins", pins}};
  return transact(Command::Connect, parameters, true, uid);
}

ResponseStatus Client::disconnect(const String& uid) {
  return transact(Command::Disconnect, Parameters{{"id", uid}}, true, uid);
}

ResponseStatus Client::update(const String& uid) {
  return transact(Command::Update, Parameters{{"id", uid}}, true, uid);
}

ResponseStatus Client::config(const String& uid, const Parameters& parameters) {
  Parameters requestParameters = parameters;
  requestParameters["id"] = uid;
  return transact(Command::Config, requestParameters, true, uid);
}

ResponseStatus Client::control(const String& uid, const Parameters& parameters) {
  Parameters requestParameters = parameters;
  requestParameters["id"] = uid;
  return transact(Command::Control, requestParameters, true, uid);
}

ResponseStatus Client::reset(const String& uid) {
  return transact(Command::Reset, Parameters{{"id", uid}}, true, uid);
}

}  // namespace vscp

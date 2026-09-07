/**
 * @file vscp_client.hpp
 * @brief Synchronous VSCP request client for HMI and controller applications.
 */

#pragma once

#include "io/vscp_transport.hpp"
#include "vscp_codec.hpp"

namespace vscp {

class Client {
public:
  explicit Client(Transport& transport, unsigned long timeoutMs = DEFAULT_TIMEOUT_MS);

  ResponseStatus init(const String& application = "", const String& databaseVersion = "");
  ResponseStatus connect(const String& uid, const String& pins);
  ResponseStatus disconnect(const String& uid);
  ResponseStatus update(const String& uid);
  ResponseStatus config(const String& uid, const Parameters& parameters);
  ResponseStatus control(const String& uid, const Parameters& parameters);
  ResponseStatus reset(const String& uid);

  bool isInitialized() const { return initialized_; }
  const char* apiVersion() const { return API_VERSION; }

private:
  ResponseStatus transact(Command command, Parameters parameters, bool requiresInit, const String& expectedId = "");

  Transport& transport_;
  unsigned long timeoutMs_;
  bool initialized_ = false;
};

}  // namespace vscp

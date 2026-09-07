/**
 * @file vscp.hpp
 * @brief Public umbrella header for the VSCP client/server library.
 */

#pragma once

#include "config.hpp"
#include "io/vscp_log_sink.hpp"
#include "io/vscp_transport.hpp"
#include "vscp_client.hpp"
#include "vscp_codec.hpp"
#include "vscp_server.hpp"
#include "vscp_types.hpp"

#ifdef ARDUINO_H_ENV
#include "io/vscp_stream_transport.hpp"
#endif

#ifdef STDIO_H_ENV
#include "io/vscp_iostream_transport.hpp"
#include "io/vscp_stdio_transport.hpp"
#endif

# VSCP client/server library

The library implements **Virtual Sensors Communication Protocol** API `1.4`.
It is not the event-based Very Simple Control Protocol.

## Components

- `vscp_codec.*`: shared URL-like message parser and serializer;
- `src/config.hpp`: compile-time protocol and platform defaults;
- `src/io/vscp_transport.hpp`: transport interface;
- `src/io/vscp_stream_transport.*`: non-blocking Arduino `Stream` adapter;
- `src/io/vscp_iostream_transport.*`: blocking `std::istream`/`std::ostream` adapter;
- `src/io/vscp_stdio_transport.*`: blocking C `FILE*` adapter;
- `vscp_client.*`: synchronous request client for an HMI/controller;
- `vscp_server.*`: non-blocking responder routed through command handlers.

Include all public components with `#include <vscp.hpp>`.

Configuration defaults live in `src/config.hpp` and can be overridden with
compiler definitions such as `-DMAX_PROTOCOL_REQUEST_SIZE=2048`.
This file configures only the protocol and supported build environments.
Physical transport settings such as UART number, baud rate, frame format, and
RX/TX pins belong to the consuming application. EduBox defines them in
`src/BoardConfig.hpp`.

## Environments

Arduino builds are detected through the `ARDUINO` macro and expose
`vscp::StreamTransport`. Desktop builds default to `STDIO_H_ENV` and expose
both `vscp::IostreamTransport` and `vscp::StdioTransport`. The desktop adapters
perform blocking line reads, which is appropriate for console applications;
an event loop should run them on a dedicated input thread.

```cpp
vscp::IostreamTransport cppTransport(std::cin, std::cout);
vscp::StdioTransport cTransport(stdin, stdout);
```

Define `VSCP_ENABLE_IOSTREAM=0` or `VSCP_ENABLE_STDIO=0` to omit either desktop
adapter. Defining both `ARDUINO_H_ENV` and `STDIO_H_ENV` is rejected.

## Transport diagnostics

`PROTOCOL_VERBOSE` controls diagnostics compiled into the common transport:

- `0`: logging disabled;
- `1`: framing and write errors;
- `2`: errors plus complete `[VSCP][RX]` and `[VSCP][TX]` frames.

A logger is explicitly injected so diagnostic output cannot silently corrupt the
protocol channel. Keep the log output and protocol output on different streams.

```cpp
// ESP32: VSCP runs on protocolUart, diagnostics use the USB console.
vscp::StreamLogSink debugLog(Serial);
vscp::StreamTransport transport(
    protocolUart, vscp::MAX_MESSAGE_SIZE, &debugLog);

// Desktop C++ streams.
vscp::IostreamLogSink desktopLog(std::clog);
vscp::IostreamTransport desktopTransport(
    std::cin, std::cout, vscp::MAX_MESSAGE_SIZE, &desktopLog);

// Desktop C streams.
vscp::StdioLogSink stdioLog(stderr);
vscp::StdioTransport stdioTransport(
    stdin, stdout, vscp::MAX_MESSAGE_SIZE, &stdioLog);
```

The sink can also be changed at runtime with `Transport::setLogSink()`.

Before dispatch, the common transport removes bytes outside printable ASCII
(`32..126`) and trims surrounding whitespace on both RX and TX. The Arduino
stream adapter also emits a separator newline and flushes each complete frame,
matching the framing behavior of the original UART messenger.

## Validation status

The synchronous client and Arduino stream transport have been exercised on
physical hardware. The handler-based server is covered by the desktop
client/server test in `tests/client_server_test.cpp` and is compiled as part of
the EduBox ESP32 firmware; its server role has not yet been validated on
physical hardware.

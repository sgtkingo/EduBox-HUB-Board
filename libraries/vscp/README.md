# VSCP client/server library

The library implements **Virtual Sensors Communication Protocol** API `1.4`.
It is not the event-based Very Simple Control Protocol.

## Components

- `vscp_codec.*`: shared URL-like message parser and serializer;
- `vscp_transport.hpp`: transport interface;
- `vscp_stream_transport.*`: non-blocking Arduino `Stream` adapter;
- `vscp_client.*`: synchronous request client for an HMI/controller;
- `vscp_server.*`: non-blocking responder routed through command handlers.

Include all public components with `#include <vscp.hpp>`.

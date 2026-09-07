# Upstream provenance

This library was refactored from the protocol contract and client implementation
in [`sgtkingo/VSCP`](https://github.com/sgtkingo/VSCP), commit
`0d4b02f03802966ba4497bc603d76b5d32922ead` (library `1.5.0`, protocol API
`1.4`).

The upstream implementation mixed the request client with a global UART
messenger. This local version separates the shared codec, transport, client, and
handler-based server so the HMI and EduBox HUB can use the same wire contract.

No upstream license file was present in the referenced commit. Distribution
terms must be clarified before this vendored derivative is distributed outside
the project.

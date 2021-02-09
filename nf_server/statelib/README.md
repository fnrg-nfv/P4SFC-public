# P4SFC State Library

<!-- transfer from shared library to static library to optimize the
performance -->

This directory contains the source codes of the state library(`libstate.a`)
in P4SFC. The state library is used by our customized elements to create
states. The state created can be obtained by other programs through gRPC.

## Build command

```bash
# if grpc and protobuf is not installed, run ./dep-install.sh
make
```

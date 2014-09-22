Librpc README
=============
# Overview
Librpc is a RPC (Remote Procedure Call) library written in C++, based on protobuf and libevent libraries.
Features:
* cross-platform, and can be used in c programs.
* dual rpc: client can call server rpc service, and server can call back client service with long connection.

# Usage
make sure libprotoc and libevent is installed.
Following below steps
* 1. add example.proto as hello.proto
* 2. add Makefile
* 3. add example_caller and example_callee code

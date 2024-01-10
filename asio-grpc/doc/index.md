# asio-grpc

## Overview

Feature overview, installation and performance benchmark can be found on [github](https://github.com/Tradias/asio-grpc).

* View of the entire API: [agrpc namespace](namespaceagrpc.html)
* Main workhorses of this library: `agrpc::GrpcContext`, `agrpc::GrpcExecutor`.
* Asynchronous gRPC clients: [cheat sheet](md_doc_client_rpc_cheat_sheet.html), `agrpc::ClientRPC`, 
* Asynchronous gRPC servers: [cheat sheet](md_doc_server_rpc_cheat_sheet.html), `agrpc::ServerRPC`, `agrpc::register_awaitable_rpc_handler`, 
`agrpc::register_yield_rpc_handler`, `agrpc::register_sender_rpc_handler`, `agrpc::register_callback_rpc_handler`
* GRPC Timer: `agrpc::Alarm`
* Combining GrpcContext and `asio::io_context`: `agrpc::run`, `agrpc::run_completion_queue`
* Faster, drop-in replacement for gRPC's [DefaultHealthCheckService](https://github.com/grpc/grpc/blob/v1.50.1/src/cpp/server/health/default_health_check_service.h): `agrpc::HealthCheckService`
* Writing Rust/Golang [select](https://go.dev/ref/spec#Select_statements)-style code: `agrpc::Waiter`
* Customizing asynchronous completion: [Completion token](md_doc_completion_token.html)
* Running `protoc` from CMake to generate gRPC source files: [CMake protobuf generate](md_doc_cmake_protobuf_generate.html)

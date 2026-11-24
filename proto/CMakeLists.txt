set(GTO_PROTO_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/gto/v1/common.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/gto/v1/jobs.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/gto/v1/server.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/gto/v1/solutions.proto
  ${CMAKE_CURRENT_SOURCE_DIR}/gto/v1/trees.proto
)

set(GTO_PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${GTO_PROTO_GEN_DIR})

add_library(gto_proto STATIC ${GTO_PROTO_FILES})
target_link_libraries(gto_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)
target_include_directories(gto_proto PUBLIC ${GTO_PROTO_GEN_DIR})

# Generated code is not ours to lint.
if(MSVC)
  target_compile_options(gto_proto PRIVATE /W0)
  # gRPC's own headers use a type gRPC has deprecated; MSVC reports it from
  # inside <xmemory> for every target that includes them.
  target_compile_options(gto_proto PUBLIC /wd4996)
else()
  target_compile_options(gto_proto PRIVATE -w)
endif()

protobuf_generate(
  TARGET gto_proto
  LANGUAGE cpp
  IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  PROTOC_OUT_DIR ${GTO_PROTO_GEN_DIR}
)

protobuf_generate(
  TARGET gto_proto
  LANGUAGE grpc
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
  IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  PROTOC_OUT_DIR ${GTO_PROTO_GEN_DIR}
)

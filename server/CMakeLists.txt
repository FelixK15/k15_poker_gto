add_executable(gto-server
  src/main.cpp
  src/server_service.cpp
  src/system_info.cpp
)
target_link_libraries(gto-server PRIVATE gto::solver gto_proto gRPC::grpc++ spdlog::spdlog)
target_compile_definitions(gto-server PRIVATE GTO_SERVER_VERSION="${PROJECT_VERSION}")

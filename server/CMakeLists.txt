# Everything except main() lives in a library so that the tests can link it.
add_library(gto_server_lib STATIC
  src/auth.cpp
  src/convert.cpp
  src/job_manager.cpp
  src/services.cpp
  src/system_info.cpp
  src/zstd_stream.cpp
)
target_include_directories(gto_server_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(gto_server_lib PUBLIC gto::solver gto_proto gRPC::grpc++ spdlog::spdlog
  zstd::libzstd)

add_executable(gto-server src/main.cpp)
target_link_libraries(gto-server PRIVATE gto_server_lib)
target_compile_definitions(gto-server PRIVATE GTO_SERVER_VERSION="${PROJECT_VERSION}")

if(GTO_BUILD_TESTS)
  add_executable(server_tests
    tests/job_manager_test.cpp
    tests/services_test.cpp
    tests/zstd_stream_test.cpp
  )
  target_link_libraries(server_tests PRIVATE gto_server_lib GTest::gtest GTest::gtest_main)
  gtest_discover_tests(server_tests)
endif()

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(gto-cli
  src/main.cpp
  src/spot_config.cpp
)
target_link_libraries(gto-cli PRIVATE gto::solver nlohmann_json::nlohmann_json)

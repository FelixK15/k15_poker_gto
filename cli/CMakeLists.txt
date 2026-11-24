add_executable(gto-cli src/main.cpp)
target_link_libraries(gto-cli PRIVATE gto::solver)

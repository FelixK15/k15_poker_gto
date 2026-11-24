configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
  @ONLY
)

add_library(solver STATIC
  src/card.cpp
  src/game_tree.cpp
  src/hand_evaluator.cpp
  src/range.cpp
  src/solver.cpp
  src/terminal.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)
add_library(gto::solver ALIAS solver)

target_include_directories(solver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

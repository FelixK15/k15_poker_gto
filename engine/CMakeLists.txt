configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
  @ONLY
)

add_library(solver STATIC
  src/card.cpp
  src/cpu.cpp
  src/game_tree.cpp
  src/hand_evaluator.cpp
  src/mc/abstraction.cpp
  src/mc/evaluate.cpp
  src/mc/game.cpp
  src/mc/preflop.cpp
  src/mc/report.cpp
  src/mc/solver.cpp
  src/mc/spot.cpp
  src/mc/storage.cpp
  src/mc/tree.cpp
  src/omaha.cpp
  src/range.cpp
  src/solver.cpp
  src/spot.cpp
  src/terminal.cpp
  src/thread_pool.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)
add_library(gto::solver ALIAS solver)

find_package(Threads REQUIRED)
target_link_libraries(solver PUBLIC Threads::Threads)
target_include_directories(solver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

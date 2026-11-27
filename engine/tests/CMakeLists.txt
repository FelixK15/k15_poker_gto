add_executable(solver_tests
  card_test.cpp
  game_tree_test.cpp
  hand_evaluator_test.cpp
  lock_test.cpp
  omaha_test.cpp
  plo_solver_test.cpp
  persistence_test.cpp
  range_test.cpp
  solver_test.cpp
  streets_test.cpp
  thread_pool_test.cpp
  terminal_test.cpp
  version_test.cpp
)
target_link_libraries(solver_tests PRIVATE gto::solver GTest::gtest GTest::gtest_main)
gtest_discover_tests(solver_tests)

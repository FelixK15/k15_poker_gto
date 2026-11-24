add_executable(solver_tests
  card_test.cpp
  version_test.cpp
)
target_link_libraries(solver_tests PRIVATE gto::solver GTest::gtest GTest::gtest_main)
gtest_discover_tests(solver_tests)

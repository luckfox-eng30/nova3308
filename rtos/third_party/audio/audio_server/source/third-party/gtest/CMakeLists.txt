cmake_minimum_required( VERSION 2.8.8 )

include_directories(include)

set(RK_TEST_GTEST_SRC
    src/gtest.cpp
    src/gtest-death-test.cpp
    src/gtest-filepath.cpp
    src/gtest-matchers.cpp
    src/gtest-port.cpp
    src/gtest-printers.cpp
    src/gtest-test-part.cpp
    src/gtest-typed-test.cpp
)

add_library(${LIB_STATIC_GTEST} STATIC ${RK_TEST_GTEST_SRC})
set_target_properties(${LIB_STATIC_GTEST} PROPERTIES FOLDER "rt_test_gtest")
target_link_libraries(${LIB_STATIC_GTEST})

SET (INTEGRATION_TARGET click_scope_integration_tests)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package(Qt5Core REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/scope)
include_directories (${CMAKE_BINARY_DIR}/scope)

FILE (GLOB TEST_SOURCES *.cpp)
FILE (GLOB TEST_HEADERS *.h)

add_executable (
  click_scope_integration_tests

  webclient_integration.cpp
)

qt5_use_modules(click_scope_integration_tests Core DBus Network Test)

target_link_libraries (
  click_scope_integration_tests

  clickscope

  gmock
  gmock_main

  ${CMAKE_THREAD_LIBS_INIT}
)

add_test(click_scope_integration_tests ${CMAKE_CURRENT_BINARY_DIR}/click_scope_integration_tests)

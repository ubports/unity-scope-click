SET (INTEGRATION_TARGET click_scope_integration_tests)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package(Qt5Core REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/scope)

FILE (GLOB TEST_SOURCES *.cpp)
FILE (GLOB TEST_HEADERS *.h)

add_executable (${INTEGRATION_TARGET}
  departmentsdb_integration.cpp
  webclient_integration.cpp
)

qt5_use_modules(${INTEGRATION_TARGET} Core DBus Network Test)

target_link_libraries (${INTEGRATION_TARGET}
  ${STORE_LIB_UNVERSIONED}

  gmock
  gmock_main

  ${CMAKE_THREAD_LIBS_INIT}
)

add_custom_target (test-integration-click-scope
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${INTEGRATION_TARGET}
  DEPENDS ${INTEGRATION_TARGET}
)

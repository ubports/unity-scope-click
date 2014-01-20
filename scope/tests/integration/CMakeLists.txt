SET (INTEGRATION_TARGET test-click-scope-integration)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package(Qt5Core REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/scope)
include_directories (${CMAKE_BINARY_DIR}/scope)

FILE (GLOB TEST_SOURCES *.cpp)
FILE (GLOB TEST_HEADERS *.h)

add_executable (${INTEGRATION_TARGET}
  ${TEST_SOURCES}
  ${TEST_HEADERS}
)

qt5_use_modules (${INTEGRATION_TARGET} Core DBus Network Test)

target_link_libraries ( ${INTEGRATION_TARGET}
  -Wl,-rpath,${CMAKE_BINARY_DIR}/src
  -L${CMAKE_BINARY_DIR}/src
  ${SCOPE_LIB_NAME}
  )

add_custom_target(click-scope-integration-tests
  COMMAND ${INTEGRATION_TARGET}
  DEPENDS ${INTEGRATION_TARGET}
)

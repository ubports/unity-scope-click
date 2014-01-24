SET (TESTS_TARGET test-click-scope)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package(Qt5Core REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/scope)
include_directories (${CMAKE_BINARY_DIR}/scope)

FILE (GLOB TEST_SOURCES *.cpp)
FILE (GLOB TEST_HEADERS *.h)

set_target_properties (${SCOPE_LIB_NAME}-tests PROPERTIES COMPILE_DEFINITIONS "USE_FAKE_NAM")

add_executable (${TESTS_TARGET}
  ${TEST_SOURCES}
  ${TEST_HEADERS}
)

qt5_use_modules (${TESTS_TARGET} Core DBus Network Test)

target_link_libraries ( ${TESTS_TARGET}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  )
target_link_libraries ( ${TESTS_TARGET}
  -Wl,-rpath,${CMAKE_BINARY_DIR}/src
  -L${CMAKE_BINARY_DIR}/src
  ${SCOPE_LIB_NAME}-tests
  )

add_custom_target(click-scope-tests
  COMMAND ${TESTS_TARGET}
  DEPENDS ${TESTS_TARGET}
)

add_custom_target(click-scope-tests-valgrind
  COMMAND valgrind --tool=memcheck "${CMAKE_CURRENT_BINARY_DIR}/${TESTS_TARGET}"
  DEPENDS ${TESTS_TARGET}
)

add_custom_target(click-scope-tests-valgrind-leaks
  COMMAND valgrind --tool=memcheck --track-origins=yes --num-callers=40 --leak-resolution=high --leak-check=full "${CMAKE_CURRENT_BINARY_DIR}/${TESTS_TARGET}"
  DEPENDS ${TESTS_TARGET}
)

add_definitions( -DUSE_FAKE_NAM )
add_subdirectory( integration )
add_subdirectory(download_manager_tool)
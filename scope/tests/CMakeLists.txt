set (CLICKSCOPE_TESTS_TARGET click-scope-tests)
find_package(Threads)

# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")

add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)

include_directories (
  ${CMAKE_SOURCE_DIR}/scope
  ${CMAKE_BINARY_DIR}/scope
  ${GTEST_INCLUDE_DIR}
  ${GMOCK_INCLUDE_DIR}
)

add_executable (${CLICKSCOPE_TESTS_TARGET}
  mock_network_access_manager.h
  test_download_manager.cpp
  test_index.cpp
  test_webclient.cpp
)

qt5_use_modules(${CLICKSCOPE_TESTS_TARGET} Core DBus Network Test)

target_link_libraries(${CLICKSCOPE_TESTS_TARGET}
  ${SCOPE_LIB_UNVERSIONED}

  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}

  gmock
  gmock_main

  ${CMAKE_THREAD_LIBS_INIT}
)

add_custom_target (test-click-scope
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)

add_subdirectory(integration)
add_subdirectory(download_manager_tool)

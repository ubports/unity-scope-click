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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_data.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/test_data.cpp)

add_executable (${CLICKSCOPE_TESTS_TARGET}
  mock_network_access_manager.h
  mock_webservice.h
  test_download_manager.cpp
  test_index.cpp
  test_interface.cpp
  test_reviews.cpp
  test_smartconnect.cpp
  test_webclient.cpp

  ${CMAKE_CURRENT_BINARY_DIR}/test_data.cpp
)

qt5_use_modules(${CLICKSCOPE_TESTS_TARGET} Core DBus Network Test)

target_link_libraries(${CLICKSCOPE_TESTS_TARGET}
  ${SCOPE_LIB_UNVERSIONED}

  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}

  gmock
  gmock_main

  ${CMAKE_THREAD_LIBS_INIT}
)

add_custom_target (test-click-scope
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)

add_custom_target (test-click-scope-disabled
  COMMAND GTEST_ALSO_RUN_DISABLED_TESTS=1 ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)

add_subdirectory(integration)
add_subdirectory(download_manager_tool)
add_subdirectory(click_interface_tool)
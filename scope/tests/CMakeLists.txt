set (CLICKSCOPE_TESTS_TARGET click-scope-tests)
find_package(Threads)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)

include_directories (
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${CMAKE_SOURCE_DIR}/scope
  ${JSON_CPP_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIR}
  ${GMOCK_INCLUDE_DIR}
)

add_executable (${CLICKSCOPE_TESTS_TARGET}
  test_query.cpp
)

qt5_use_modules(${CLICKSCOPE_TESTS_TARGET} Core DBus Network Test)

target_link_libraries(${CLICKSCOPE_TESTS_TARGET}
  ${STORE_LIB_UNVERSIONED}
  ${SCOPE_LIB_NAME}

  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_CLIENT_LDFLAGS}
  ${UBUNTU_DOWNLOAD_MANAGER_COMMON_LDFLAGS}
  ${JSON_CPP_LDFLAGS}

  gmock
  gmock_main

  ${CMAKE_THREAD_LIBS_INIT}
)

add_custom_target (test-click-scope
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)

add_custom_target(test-click-scope-valgrind
  COMMAND valgrind --tool=memcheck ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)

add_custom_target(test-click-scope-leaks
  COMMAND valgrind --tool=memcheck --track-origins=yes --num-callers=40 --leak-resolution=high --leak-check=full ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)
add_custom_target (test-click-scope-disabled
  COMMAND GTEST_ALSO_RUN_DISABLED_TESTS=1 ${CMAKE_CURRENT_BINARY_DIR}/${CLICKSCOPE_TESTS_TARGET}
  DEPENDS ${CLICKSCOPE_TESTS_TARGET}
)

add_subdirectory(integration)
add_subdirectory(download_manager_tool)
add_subdirectory(click_interface_tool)
add_subdirectory(fake_launcher)

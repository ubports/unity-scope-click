set (CLICKSCOPE_TESTS_TARGET click-scope-tests)
set (APPS_SCOPE_TESTS_TARGET apps-scope-tests)
find_package(Threads)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Sql REQUIRED)

include_directories (
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${CMAKE_SOURCE_DIR}/scope
  ${JSON_CPP_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIR}
  ${GMOCK_INCLUDE_DIR}
)

add_executable (${CLICKSCOPE_TESTS_TARGET}
  test_pay.cpp
  test_query.cpp
)

add_executable (${APPS_SCOPE_TESTS_TARGET}
  test_apps_query.cpp
)

qt5_use_modules(${CLICKSCOPE_TESTS_TARGET} Core DBus Network Test Sql)
qt5_use_modules(${APPS_SCOPE_TESTS_TARGET} Core DBus Network Test Sql)

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

target_link_libraries(${APPS_SCOPE_TESTS_TARGET}
  ${APPS_LIB_UNVERSIONED}
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

# ---

add_custom_target (test-apps-scope
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${APPS_SCOPE_TESTS_TARGET}
  DEPENDS ${APPS_SCOPE_TESTS_TARGET}
)

add_custom_target(test-apps-scope-valgrind
  COMMAND valgrind --tool=memcheck ${CMAKE_CURRENT_BINARY_DIR}/${APPS_SCOPE_TESTS_TARGET}
  DEPENDS ${APPS_SCOPE_TESTS_TARGET}
)

add_custom_target(test-apps-scope-leaks
  COMMAND valgrind --tool=memcheck --track-origins=yes --num-callers=40 --leak-resolution=high --leak-check=full ${CMAKE_CURRENT_BINARY_DIR}/${APPS_SCOPE_TESTS_TARGET}
  DEPENDS ${APPS_SCOPE_TESTS_TARGET}
)
add_custom_target (test-apps-scope-disabled
  COMMAND GTEST_ALSO_RUN_DISABLED_TESTS=1 ${CMAKE_CURRENT_BINARY_DIR}/${APPS_SCOPE_TESTS_TARGET}
  DEPENDS ${APPS_SCOPE_TESTS_TARGET}
)


add_subdirectory(integration)
add_subdirectory(download_manager_tool)
add_subdirectory(click_interface_tool)
add_subdirectory(fake_launcher)

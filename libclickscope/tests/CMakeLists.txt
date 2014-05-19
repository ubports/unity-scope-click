set (LIBCLICKSCOPE_TESTS_TARGET libclick-scope-tests)
find_package(Threads)

# Qt5 bits
SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)

include_directories (
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${JSON_CPP_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIR}
  ${GMOCK_INCLUDE_DIR}
)

add_executable (${LIBCLICKSCOPE_TESTS_TARGET}
  test_configuration.cpp
)

qt5_use_modules(${LIBCLICKSCOPE_TESTS_TARGET} Core)

target_link_libraries(${LIBCLICKSCOPE_TESTS_TARGET}
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

add_custom_target (test-libclickscope
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${LIBCLICKSCOPE_TESTS_TARGET}
  DEPENDS ${LIBCLICKSCOPE_TESTS_TARGET}
)

add_custom_target (test-libclickscope-valgrind
  COMMAND valgrind --tool=memcheck ${CMAKE_CURRENT_BINARY_DIR}/${LIBCLICKSCOPE_TESTS_TARGET}
  DEPENDS ${LIBCLICKSCOPE_TESTS_TARGET}
)

add_custom_target (test-libclickscope-leaks
  COMMAND valgrind --tool=memcheck --track-origins=yes --num-callers=40 --leak-resolution=high --leak-check=full ${CMAKE_CURRENT_BINARY_DIR}/${LIBCLICKSCOPE_TESTS_TARGET}
  DEPENDS ${LIBCLICKSCOPE_TESTS_TARGET}
)

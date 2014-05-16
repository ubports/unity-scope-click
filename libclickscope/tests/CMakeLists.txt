set (LIBCLICKSCOPE_TESTS_TARGET libclick-scope-tests)
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
  ${CMAKE_SOURCE_DIR}/libclickscope
  ${JSON_CPP_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIR}
  ${GMOCK_INCLUDE_DIR}
)

add_executable (${CLICKSCOPE_TESTS_TARGET}
  test_configuration.cpp
)

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

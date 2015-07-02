project(unity-scope-click)

cmake_minimum_required(VERSION 2.8.10)

set(SCOPE_CLICK_VERSION_MAJOR 0)
set(SCOPE_CLICK_VERSION_MINOR 0)
set(SCOPE_CLICK_VERSION_PATCH 1)

# Some default CFLAGS
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wextra -Werror -fPIC")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -g -Wextra -Wall -Werror -fPIC")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

set(STORE_LIB_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/unity-scopes/clickstore/)
set(STORE_DATA_DIR ${CMAKE_INSTALL_FULL_DATADIR}/unity/scopes/clickstore/)
set(APPS_LIB_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/unity-scopes/clickapps/)
set(APPS_DATA_DIR ${CMAKE_INSTALL_FULL_DATADIR}/unity/scopes/clickapps/)

find_program(INTLTOOL_MERGE intltool-merge)

include(UseGSettings)
find_package (PkgConfig REQUIRED)

pkg_check_modules(UNITY_SCOPES REQUIRED libunity-scopes>=0.6.7 libunity-api>=0.1.3)
add_definitions(${UNITY_SCOPES_CFLAGS} ${UNITY_SCOPES_CFLAGS_OTHER})

pkg_check_modules(UBUNTUONE REQUIRED ubuntuoneauth-2.0>=15.10)
add_definitions(${UBUNTUONE_CFLAGS} ${UBUNTUONE_CFLAGS_OTHER})

pkg_check_modules(UBUNTU_DOWNLOAD_MANAGER_CLIENT REQUIRED ubuntu-download-manager-client)
pkg_check_modules(UBUNTU_DOWNLOAD_MANAGER_COMMON REQUIRED ubuntu-download-manager-common)

pkg_check_modules(DBUSTEST REQUIRED dbustest-1)


SET (SCOPE_LIB_VERSION 0.2.0)
SET (SCOPE_LIB_SOVERSION 0)
SET (SCOPE_LIB_API_VERSION 2.0)
SET (STORE_LIB_UNVERSIONED com.canonical.scopes.clickstore)
SET (SCOPE_LIB_NAME clickscope)
SET (STORE_LIB_NAME ${STORE_LIB_UNVERSIONED}-${SCOPE_LIB_API_VERSION})
SET (APPS_LIB_UNVERSIONED scope)
SET (APPS_LIB_NAME ${APPS_LIB_UNVERSIONED}-${SCOPE_LIB_API_VERSION})

# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")

add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")

# Add our own subdirectories.
add_subdirectory(tests)
add_subdirectory(bin)
add_subdirectory(libclickscope)
add_subdirectory(scope)
add_subdirectory(data)
add_subdirectory(po)
add_subdirectory(tools)

include(EnableCoverageReport)
ENABLE_COVERAGE_REPORT(TARGETS ${SCOPE_LIB_NAME} ${STORE_LIB_UNVERSIONED} ${APPS_LIB_UNVERSIONED}
  TESTS click_scope_integration_tests libclick-scope-tests fake_launcher click-scope-tests apps-scope-tests download_manager_tool click_interface_tool init-departments test-integration-harness
  FILTER /usr/include ${CMAKE_BINARY_DIR}/*
)

# Custom targets for the tests
add_custom_target (test
  DEPENDS test-click-scope test-apps-scope test-libclickscope test-integration-harness
)

add_custom_target (test-disabled
  DEPENDS test-click-scope-disabled
)

# Add a custom target for integration tests, as they should not be run
# during normal make test.
add_custom_target (test-integration
  DEPENDS test-integration-click-scope
)

# Add a custom target for running the tests under valgrind.
add_custom_target (test-valgrind
  DEPENDS test-click-scope-valgrind test-apps-scope-valgrind test-libclickscope-valgrind
)

# Add a custom target for running the tests under valgrind with the
# full leak checks enabled.
add_custom_target (test-leaks
  DEPENDS test-click-scope-leaks test-apps-scope-leaks test-libclickscope-leaks
)

# Also let "make check" and partners work.
add_custom_target (check
  DEPENDS test
)

add_custom_target (check-integration
  DEPENDS test-integration
)

add_custom_target (check-valgrind
  DEPENDS test-valgrind
)

add_custom_target (check-leaks
  DEPENDS test-leaks
)

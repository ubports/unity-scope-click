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

set(SCOPE_LIB_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/unity-scopes/clickscope/)
set(SCOPE_DATA_DIR ${CMAKE_INSTALL_FULL_DATADIR}/unity/scopes/clickscope/)

include(FindPkgConfig)

pkg_check_modules(UNITY_SCOPES REQUIRED libunity-scopes>=0.4.0 libunity-api>=0.1.3)
add_definitions(${UNITY_SCOPES_CFLAGS} ${UNITY_SCOPES_CFLAGS_OTHER})

find_package (PkgConfig REQUIRED)
pkg_check_modules(UBUNTUONE REQUIRED ubuntuoneauth-2.0)
add_definitions(${UBUNTUONE_CFLAGS} ${UBUNTUONE_CFLAGS_OTHER})

pkg_check_modules(UBUNTU_DOWNLOAD_MANAGER_CLIENT REQUIRED ubuntu-download-manager-client)
pkg_check_modules(UBUNTU_DOWNLOAD_MANAGER_COMMON REQUIRED ubuntu-download-manager-common)

SET (SCOPE_LIB_VERSION 0.2.0)
SET (SCOPE_LIB_SOVERSION 0)
SET (SCOPE_LIB_API_VERSION 2.0)
SET (SCOPE_LIB_UNVERSIONED clickscope)
SET (SCOPE_LIB_NAME ${SCOPE_LIB_UNVERSIONED}-${SCOPE_LIB_API_VERSION})

add_subdirectory(autopilot)
add_subdirectory(libclickscope)
add_subdirectory(scope)
add_subdirectory(data)
add_subdirectory(po)

include(EnableCoverageReport) # Using gcov CMake modules from https://code.cor-lab.org/projects/rsc
#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage (or ninja if you use that backend)
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)
if(cmake_build_type_lower MATCHES coverage)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage" )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage" )
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage" )
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage" )
  ENABLE_COVERAGE_REPORT(TARGETS ${SCOPE_LIB_UNVERSIONED} FILTER /usr/include ${CMAKE_SOURCE_DIR}/tests/* ${CMAKE_BINARY_DIR}/*)
endif()

# Custom targets for the tests
add_custom_target (test
  DEPENDS test-click-scope
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
  DEPENDS test-click-scope-valgrind
)

# Add a custom target for running the tests under valgrind with the
# full leak checks enabled.
add_custom_target (test-leaks
  DEPENDS test-click-scope-leaks
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
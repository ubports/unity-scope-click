project(clickscope CXX)
cmake_minimum_required(VERSION 2.8.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

include(GNUInstallDirs)
set(SCOPE_DATA_DIR share/unity/scopes) # FIXME, not correct.

include(FindPkgConfig)

pkg_check_modules(UNITY_SCOPES REQUIRED libunity-scopes)

# Config.h variables to account for breaking changes in 0.1.8:
if (UNITY_SCOPES_VERSION VERSION_GREATER 0.1.7)
  SET (UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY 1)
  SET (UNITY_SCOPES_API_NEW_SHORTER_NAMESPACE 1)
else()
  SET (UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY 0)
  SET (UNITY_SCOPES_API_NEW_SHORTER_NAMESPACE 0)
endif ()


pkg_check_modules(LIBSIGNON REQUIRED libsignon-qt5 accounts-qt5)
add_definitions(${LIBSIGNON_CFLAGS} ${LIBSIGNON_CFLAGS_OTHER})

find_package (PkgConfig REQUIRED)
pkg_check_modules(UBUNTUONE REQUIRED ubuntuoneauth-2.0)
add_definitions(${UBUNTUONE_CFLAGS} ${UBUNTUONE_CFLAGS_OTHER})

SET (SCOPE_LIB_VERSION 0.2.0)
SET (SCOPE_LIB_SOVERSION 0)
SET (SCOPE_LIB_API_VERSION 2.0)
SET (SCOPE_LIB_UNVERSIONED clickscope)
SET (SCOPE_LIB_NAME ${SCOPE_LIB_UNVERSIONED}-${SCOPE_LIB_API_VERSION})

add_subdirectory(scope)
add_subdirectory(data)

add_custom_target(check
  DEPENDS click-scope-tests
)

add_custom_target(check-valgrind
  DEPENDS click-scope-tests-valgrind
)

add_custom_target(check-valgrind-leaks
  DEPENDS click-scope-tests-valgrind-leaks
)

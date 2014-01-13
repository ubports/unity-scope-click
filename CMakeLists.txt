project(clickscope CXX)
cmake_minimum_required(VERSION 2.8.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

include(GNUInstallDirs)
set(DATADIR share/unity/scopes) # FIXME, not correct.

include(FindPkgConfig)
pkg_check_modules(UNITY_SCOPES libunity-scopes REQUIRED)

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

add_subdirectory(src)
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

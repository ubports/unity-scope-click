project(unity-scope-click)

cmake_minimum_required(VERSION 2.8.10)

set(SCOPE_CLICK_VERSION_MAJOR 0)
set(SCOPE_CLICK_VERSION_MINOR 0)
set(SCOPE_CLICK_VERSION_PATCH 1)

# Some default CFLAGS
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wextra -Werror -fPIC")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -g -Wextra -Wall -Werror -fPIC")

include(GNUInstallDirs)
include(CTest)

set(SCOPE_DATA_DIR share/unity/scopes) # FIXME, not correct.

include(FindPkgConfig)

pkg_check_modules(UNITY_SCOPES REQUIRED libunity-scopes)
add_definitions(${UNITY_SCOPES_CFLAGS} ${UNITY_SCOPES_CFLAGS_OTHER})

# Config.h variables to account for breaking changes in 0.1.8:
if (UNITY_SCOPES_VERSION VERSION_GREATER 0.1.7)
  SET (UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY 1)
  SET (UNITY_SCOPES_API_NEW_SHORTER_NAMESPACE 1)
else()
  SET (UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY 0)
  SET (UNITY_SCOPES_API_NEW_SHORTER_NAMESPACE 0)
endif ()


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

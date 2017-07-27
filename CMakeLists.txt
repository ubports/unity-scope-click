project(unity-scope-click)

cmake_minimum_required(VERSION 2.8.10)

set(SCOPE_CLICK_VERSION_MAJOR 0)
set(SCOPE_CLICK_VERSION_MINOR 0)
set(SCOPE_CLICK_VERSION_PATCH 1)

# Some default CFLAGS
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wextra -Werror -fPIC")
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -g -Wextra -Wall -Werror -Werror=conversion-null -fPIC")

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

# Uncomment to enable network timings messages via stderr
# add_definitions(-DNETWORK_TIMINGS)

pkg_check_modules(UBUNTUONE REQUIRED ubuntuoneauth-2.0>=15.10)
add_definitions(${UBUNTUONE_CFLAGS} ${UBUNTUONE_CFLAGS_OTHER})

pkg_check_modules(UBUNTU_DOWNLOAD_MANAGER_CLIENT REQUIRED ubuntu-download-manager-client)
pkg_check_modules(UBUNTU_DOWNLOAD_MANAGER_COMMON REQUIRED ubuntu-download-manager-common)

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
add_subdirectory(bin)
add_subdirectory(libclickscope)
add_subdirectory(scope)
add_subdirectory(data)
add_subdirectory(po)
add_subdirectory(tools)

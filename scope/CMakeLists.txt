SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)

file (GLOB SCOPE_SOURCES *.cpp)

add_library(
    clickscope SHARED

    clickquery.cpp
    clickscope.cpp
    download-manager.cpp
    network-access-manager.cpp
    webclient.cpp)

configure_file(Config.h.in Config.h)
include_directories (${CMAKE_CURRENT_BINARY_DIR})

qt5_use_modules (clickscope Network)

target_link_libraries (
  clickscope

  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
)

add_subdirectory (tests)

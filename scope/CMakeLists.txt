SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)

file (GLOB SCOPE_SOURCES *.cpp)

add_library (${SCOPE_LIB_NAME} SHARED ${SCOPE_SOURCES})
add_library (${SCOPE_LIB_NAME}-tests SHARED ${SCOPE_SOURCES})

set_target_properties (${SCOPE_LIB_NAME}-tests PROPERTIES COMPILE_DEFINITIONS "USE_FAKE_NAM")

configure_file(Config.h.in Config.h)
include_directories (${CMAKE_CURRENT_BINARY_DIR})

qt5_use_modules (${SCOPE_LIB_NAME} Network)

target_link_libraries (${SCOPE_LIB_NAME}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
)

target_link_libraries (${SCOPE_LIB_NAME}-tests
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS}
)

add_subdirectory (tests)

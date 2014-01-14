SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)
find_package (Qt5Core REQUIRED)

file (GLOB SCOPE_SOURCES *.cpp)

add_library (${SCOPE_LIB_NAME} SHARED ${SCOPE_SOURCES} )

include_directories (include)
# to get generated Config.h:
include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)

qt5_use_modules (${SCOPE_LIB_NAME} Network)

target_link_libraries (${SCOPE_LIB_NAME}
  ${UNITY_SCOPES_LDFLAGS}
  ${UBUNTUONE_LDFLAGS})

set_property (TARGET ${SCOPE_LIB_NAME} PROPERTY COMPILE_FLAGS ${UNITY_SCOPES_CFLAGS})


add_subdirectory (tests)
add_subdirectory (include)
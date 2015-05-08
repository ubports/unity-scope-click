set(FAKE_LAUNCHER_TARGET fake_launcher)

include_directories (
  ${CMAKE_SOURCE_DIR}/scope/click
)

add_executable (${FAKE_LAUNCHER_TARGET}
  fake_launcher.cpp
  fake_launcher.h
)

target_link_libraries (${FAKE_LAUNCHER_TARGET}
  ${STORE_LIB_UNVERSIONED}
)

if(enable_coverage)
  target_link_libraries(${FAKE_LAUNCHER_TARGET} gcov)
endif()

find_package (PythonInterp)

set (AUTOPILOT_DIR unityclickscope)
set (COMMON_PYTHONPATH common)

execute_process (
  COMMAND python3 -c "from distutils import sysconfig; print(sysconfig.get_python_lib())"
  COMMAND sed -r -e "s|/usr/(local/)?||g"
  OUTPUT_VARIABLE PYTHON_PACKAGE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

install (
  DIRECTORY autopilot/${AUTOPILOT_DIR}
  DESTINATION ${PYTHON_PACKAGE_DIR}
)

add_custom_target(test-click-scope-autopilot-fake-servers
    COMMAND PYTHONPATH=$ENV{PYTHONPATH}:../${COMMON_PYTHONPATH} U1_SEARCH_BASE_URL=fake DOWNLOAD_BASE_URL=fake BUILD_DIR=${CMAKE_BINARY_DIR} autopilot3 run -v ${AUTOPILOT_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/autopilot
)

add_subdirectory(scope-harness)
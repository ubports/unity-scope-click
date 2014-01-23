set (Python_ADDITIONAL_VERSIONS 2.7)
find_package (PythonInterp)

set (AUTOPILOT_DIR unityclickscope)

execute_process (
  COMMAND ${PYTHON_EXECUTABLE} -c "from distutils import sysconfig; print sysconfig.get_python_lib()"
  COMMAND sed -r -e "s|/usr/(local/)?||g"
  OUTPUT_VARIABLE PYTHON_PACKAGE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (
  COMMAND echo "Installing autopilot to: ${PYTHON_PACKAGE_DIR}"
)

install (
  DIRECTORY ${AUTOPILOT_DIR}
  DESTINATION ${PYTHON_PACKAGE_DIR}
)

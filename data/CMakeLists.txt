configure_file(
  clickscope.ini.in
  clickscope.ini
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/clickscope.ini" clickscope-screenshot.jpg apps-scope.svg
  DESTINATION "${SCOPE_DATA_DIR}"
)

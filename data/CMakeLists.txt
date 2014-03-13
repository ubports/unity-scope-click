configure_file(
  clickscope.ini.in
  clickscope.ini
)
install(
  FILES clickscope-screenshot.jpg apps-scope.svg
  DESTINATION "${SCOPE_DATA_DIR}"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/clickscope.ini"
  DESTINATION "${SCOPE_LIB_DIR}"
)

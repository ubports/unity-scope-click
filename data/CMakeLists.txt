configure_file(
  clickscope.ini.in
  clickscope.ini
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/clickscope.ini" clickscope-screenshot.jpg
  DESTINATION "${SCOPE_DATA_DIR}"
)


add_custom_target(test-integration-harness
  COMMAND PYTHONPATH=$ENV{PYTHONPATH}:../${COMMON_PYTHONPATH} SCOPE_DIR=${CMAKE_BINARY_DIR}/scope xvfb-run -a python3 -m testtools.run discover
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

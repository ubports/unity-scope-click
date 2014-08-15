set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (INITDEPTS init-departments)

find_package (Qt5Core REQUIRED)

include_directories (
    ${CMAKE_SOURCE_DIR}/libclickscope
    ${JSON_CPP_INCLUDE_DIRS}
    )

add_executable (${INITDEPTS}
        init-departments.cpp
        )

qt5_use_modules (${INITDEPTS} Network Sql)

target_link_libraries(${INITDEPTS}
  ${SCOPE_LIB_NAME}
  -lpthread
)

install(TARGETS ${INITDEPTS} DESTINATION sbin)

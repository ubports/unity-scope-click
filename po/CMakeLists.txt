include(FindGettext)
find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
find_program(INTLTOOL_UPDATE intltool-update)

set(GETTEXT_PACKAGE ${PROJECT_NAME})
set(POT_FILE ${GETTEXT_PACKAGE}.pot)
file(STRINGS LINGUAS LINGUAS)

# Creates POTFILES
add_custom_target(POTFILES ALL
    COMMENT "Generating POTFILES"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/genpotfiles.sh ${CMAKE_SOURCE_DIR}
)

# Creates the .pot file containing the translations template
add_custom_target(${POT_FILE}
    COMMENT "Generating translation template"
    COMMAND XGETTEXT="${GETTEXT_XGETTEXT_EXECUTABLE}" srcdir="${CMAKE_CURRENT_SOURCE_DIR}" ${INTLTOOL_UPDATE} --gettext-package ${GETTEXT_PACKAGE} --pot
    COMMAND cp -f ${POT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/
    DEPENDS POTFILES
)

# Builds the binary translations catalog for each language
# it finds source translations (*.po) for
foreach(LANG ${LINGUAS})
    list(APPEND PO_FILES "${LANG}.po")
    gettext_process_po_files(${LANG} ALL PO_FILES "${LANG}.po")
    set(INSTALL_DIR ${CMAKE_INSTALL_LOCALEDIR}/${LANG}/LC_MESSAGES)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.gmo
            DESTINATION ${INSTALL_DIR}
            RENAME ${GETTEXT_PACKAGE}.mo
    )
endforeach(LANG)

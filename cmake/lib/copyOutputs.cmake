
function(copyOutputs TARGET_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${TARGET_FOLDER}/SKSE/Plugins")
    set(TRANSLATIONS_FOLDER "${TARGET_FOLDER}/Interface/Translations")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")
    message(STATUS "Translation output folder: ${TRANSLATIONS_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        #COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_LINKER_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_LINKER_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    # Keep translation-only builds deployable. A project POST_BUILD command does
    # not run when only a translation table changes.
    file(
        GLOB TRANSLATION_FILES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/Interface/Translations/*.txt"
    )
    string(MD5 TRANSLATION_TARGET_ID "${TARGET_FOLDER}")
    set(TRANSLATION_TARGET "${PROJECT_NAME}_translations_${TRANSLATION_TARGET_ID}")
    add_custom_target(
        "${TRANSLATION_TARGET}" ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${TRANSLATIONS_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Interface/Translations" "${TRANSLATIONS_FOLDER}"
        DEPENDS ${TRANSLATION_FILES}
        COMMENT "Copying SkyPrompt translation tables to ${TRANSLATIONS_FOLDER}"
        VERBATIM
    )
    add_dependencies("${PROJECT_NAME}" "${TRANSLATION_TARGET}")

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()

endfunction()
# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF 0f8e565f7ea355eacccd9cce8828925fff8f6cf3
    SHA512 e897a21c23b35e91e859c4b5a56c24b236fd47b6de7d8cf75e068dbd2ca929bbdb0f32f989b42d266119d084540f995647d12a1f11b388c446259bb5dd9199be
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
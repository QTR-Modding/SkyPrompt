# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF 8f3d0b22c8562dc236eeb8e7c61ea7dc2dd3d6a1
    SHA512 8c569d318880cfae5a3202dc875638757a54bfe23e00499ab8a688e596f3f888dc77e1337fb2e36c5ac85749ef5d4445b4e5935b7fa22a44ef935b3b53a0f286
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
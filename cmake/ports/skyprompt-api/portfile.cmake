# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF 1f6bd8bf8c5cc576afb6bc91800023f518d638ff
    SHA512 a3b5a2e682a654fb54283df4892b6404bcb1f7fa97181c6ca82223cac71fb8e27013be850ac2766c42ad5daba0d26ea7526337acad4b5a335b09eee4d5ba028e
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
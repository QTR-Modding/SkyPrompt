# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF 692d4dbb7341fbbdda873d3acc5f4b306f5f0b38
    SHA512 117dc1dbdace05d469ac04c85159066112cb779d49890966524b659b4f9eacbb2ee83dec0fbbf6bc46581794b377d06a1d9883c8a5abf413a9f356d7d86330ca
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
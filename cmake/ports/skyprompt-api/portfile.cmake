# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF 1c22934aa1148f8b2c492b2176325a7c253504b8
    SHA512 4d7945c070b71ae178bbcc6c12fb956dc6fb5c514e06875bbf31cfbe44b12a0752551eafd5b3f4b189fb8954664164b481ba9ca27c3e1cf0386ad4da12e38272
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
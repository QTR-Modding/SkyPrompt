# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF a7b650fe36331f7c6305842d69903bc3081ec122
    SHA512 33e2064706b342d4746239b09e8f07b72377363c767ccec264790bf14e2130cfb3402b48172b1645dcdf4655d36172df63e37ceba44d74e20a44de9aa00df51b
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 906aea070f1428f97a2c37102e0f0cd20a3a8f19
    SHA512 9683e648ac2dccb0e6f19eb4a64eb9676819d17ef94f7da49158f5175afb26a1c22f2e9b3e5e8ef3a050379ed9e0b4c4262890624c643d895c38490ad989039c
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

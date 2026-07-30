# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF af4efca430ab8d362f64251c88ba3cf4a249e90d
    SHA512 ded5ee4f81087b5f23bd8554cd44ee3e64896946fe01136dd483f8e9e6f73db3131cf4d8b983269ad2a3398c171434ac3103a704457d65f6bd48948354730116
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
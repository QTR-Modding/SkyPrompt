# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF 5c013bf6531d998b4a835b06f7b2ea06e72b5f34
    SHA512 745e6ae8868d017405d3d7624a4295e0adb737bdb20fd1f1aafb675f28abbf92376b4606253b15981b257497dc950c66b7752afa8a71763206a675d3cbc3f2b1
    HEAD_REF main
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
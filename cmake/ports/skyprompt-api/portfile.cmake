# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAPI
    REF b0df79668de46c4e741ed437c4df497dea3abbbd
    SHA512 d93a4b9b337c0c1dd499bb89b0f21114866221de8d7e4b93b095e577ee6ce4645ff0e4b1559da8910d12f35d169a532060bd9fbd40ae5527b67bfac412b15afb
    HEAD_REF theme
)

# Install codes
set(SkyPromptAPI_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAPI_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/NOTICE")
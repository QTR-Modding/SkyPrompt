# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 62d4f1607a31f9fa707d3ce380d0b35fd36b55f7
    SHA512 58b5c5456053a762376b00cdd2a87c5cfe70a1d29297f2fc22a1a1c7fb1414019f3c703c6eb6c401306adedd8ad5358929c7fb93e3d304afae682bb5c1914d15
    HEAD_REF update
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
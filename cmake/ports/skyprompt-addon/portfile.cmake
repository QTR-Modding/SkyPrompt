# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF c985e1aa6cd2820b089dac512a908644ca5758cc
    SHA512 bc6e8cad360fef94028e079063d1437a5314659c100118a3062c5629340191210b0abf8f3a5219e314e62181721d25d15bfe6bd0ba22620ba2b2fb8c473666ba
    HEAD_REF update
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
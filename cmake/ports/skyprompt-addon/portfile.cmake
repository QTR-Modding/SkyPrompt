# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 37d878f8039e417b51bf51ce1450eff1656d4f45
    SHA512 a8ac7106bcd5bb2cc6cc087a221945bc2723ce6bc6e47a066c42cdb80132eb4fe80e7d8272ac3c9bbed245f3a2f069db2fc63e2c978fd550c1b36fef5b9abb80
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

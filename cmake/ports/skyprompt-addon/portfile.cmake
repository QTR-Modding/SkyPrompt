# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF c6fbb7f67b6cf849d2478d661c63fcd49a8818d9
    SHA512 34131ab75563bea2c5b03b882e1c73c85d26d6cc45b51f4361c85ec42cc5dfb8ec84678ee1395d5c77028a2ab6c6bd53589a240521490c9cfb9ec2e12afa4843
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 2c451b159ffb03dc82164a319e41e0a951e5e8af
    SHA512 09898896fad6c5e7d5e11a75fd8d4c1762394ab4f010f7c3b1cdf30eca0a6ca57b018d749c939c14d6b4cf06d0a055b29e31018a9aaf33f401af3cd11d4e3690
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

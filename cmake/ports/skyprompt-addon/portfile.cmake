# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF v1.2.0
    SHA512 87152eb497b78c911383a2e4e429b73c8bd562ae493007adf456ef83250027f6a6b2697a50da0165f0e16147b137fceaafc8538afe28cca63adb0c33dd0f1134
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

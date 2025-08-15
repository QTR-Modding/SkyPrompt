# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF f1184b84287fdb96547b85f7789dd882284ee34f
    SHA512 81cb731d53afc5f7820b3ad86326fc8b3d785d5f5247eb5ad32157d75243774510d6b104d1280fa947bba7cceb489faea034fda68e79983d56ee6ba974e9e734
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF d524bdc62800e3ae278ce4000abbedde4826cdd5
    SHA512 606265e5383d10f90d4b3717a42effe10f40785ef5e6eb8e696fba61e75fca150ea30dc7d9c961b840f5616192a2ed3af924733e36b78a21bc0eca697c960035
    HEAD_REF update
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF cf1cc816cf5cd7e2b308e4ba9d3428477d7acb91
    SHA512 8b42e223589aa668ceaabe3bdf9deac7432cbb76d0e040d23fdd8823fab890e22435f0badf2cfc79fa432170aa53f5672c4bc2f3d50c2b2f26bc937a44bb0f24
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 4ddafb626d8d6e9723043f07820cc23650341f15
    SHA512 88eb2e015bba16fedcd642f7b262dedcccc038bb83e68c6e8311832733b173b31f88e1cef7ecaf129aae2d7b5d3d281fcfb1001ce483aa2157190687cff26802
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

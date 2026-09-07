# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF c9359ee0137a8e25db9aaa5c5c02fa41b9866ebb
    SHA512 339fb37ed5d874789298a9ea8386d99361b6c3873eb7c608d8f3c1bf4a1d4736986479f6899c8a69348a46fee29b8b4d29f6142f46e5d5acd3e534d610afefef
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

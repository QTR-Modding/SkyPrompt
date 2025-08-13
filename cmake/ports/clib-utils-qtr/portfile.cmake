# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/CLibUtilsQTR
    REF 1aee6a21c3fde99ac409fa2dbe9054c08abed92f
    SHA512 fbef04bc7f2b548e3edbccccc3cf2ff25ba5323944475b2eae45b2074b61eb86b99d910c36e82d40e93eec647dc51f3492d531fe6038a9611e601e584f946912
    HEAD_REF main
)

# Install codes
set(CLibUtilsQTR_SOURCE	${SOURCE_PATH}/include/CLibUtilsQTR)
file(INSTALL ${CLibUtilsQTR_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
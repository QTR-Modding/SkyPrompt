# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 475af86e96d175ffd75f88d07bf7605a9b154f65
    SHA512 d34053da99d3a84913c4d034731139d49d5c58b29a7e9831d7deb2bc23afad337d616abefc7fe998eeadee99db245cb2c02664799eacb3e54f1643ea1157f9bf
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

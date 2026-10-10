# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 3a1e4b86b9a06ffc57876bdaff154b8cde40c015
    SHA512 6e449a361209eca4ee12b0d9aef0bd75a57ef2f017272ad99d49144693ef7f903e126b04b031e6fbc730fbb187520b3cea7f0bcbd66ba8acbed9496a19d037d5
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

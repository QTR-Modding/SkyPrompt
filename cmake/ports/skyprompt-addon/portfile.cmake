# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF d1fc9ca81e7c55b29175f25b883cfb901bf95bcc
    SHA512 0e470c173bda6f9b76b7337e936bbf339cd6e97922aa6864e8f5825f564ebf81d268692ad892ec4179ca24d6a824b0d5c0a15d7f51dc9b2bd1e7b15b73a8e37d
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
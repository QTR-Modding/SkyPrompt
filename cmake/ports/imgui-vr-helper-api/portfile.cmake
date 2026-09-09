vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/imgui-vr-helper
    REF 90a94252e433cd46c6bd8c2fa2c47bdbd3497fc6
    SHA512 69275bea0b95344f4bfcb1d62bb80dd205b259dc15284bf2dfc6c07559726a797899bc968bde503e8597d7da7b620e4ccb4c6d7c79a59bac1a7a0a0a76ea1c62
    HEAD_REF main
)

# The handshake source compiles in the consumer, alongside its own dependencies.
file(INSTALL "${SOURCE_PATH}/api/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/ImGuiVRHelper"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.cpp" PATTERN "COPYING*")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/api/COPYING" "${SOURCE_PATH}/api/COPYING.LESSER")

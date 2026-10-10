# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO QTR-Modding/SkyPromptAddOn
    REF 4247c46ba68911a40bd1f64d9977f11b9a2ced58
    SHA512 b11bca9214268a82efb3f1916a2dd22c530288471d98f753140463b10fabd0ec7d0d8e2b2abe7aa4fa8840ee8145a15f44c0ccd96052af0e648a6c1415fab5af
    HEAD_REF main
)

# Install codes
set(SkyPromptAddOn_SOURCE	${SOURCE_PATH}/include/SkyPrompt)
file(INSTALL ${SkyPromptAddOn_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

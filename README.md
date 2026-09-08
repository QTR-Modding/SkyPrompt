#### BUILDING

SkyPrompt-owned code remains proprietary; see [LICENSE](LICENSE). Third-party
licenses are listed in [LICENSES.txt](LICENSES.txt).

Use the [SkyPrompt source](https://github.com/QTR-Modding/SkyPrompt) commit or tag
linked with your Nexus download, rather than the latest development branch.
Install Visual Studio's C++ tools (MSVC with C++23 support, Windows SDK, CMake and
Ninja), Git, and [vcpkg](https://github.com/microsoft/vcpkg). Run these commands
from a Visual Studio Developer PowerShell configured for x64.

Clone [CommonLibVR-MIT](https://github.com/QTR-Modding/CommonLibVR-MIT) and select
the revision used by this version of SkyPrompt:

```powershell
git clone --recursive https://github.com/QTR-Modding/CommonLibVR-MIT.git C:/src/CommonLibVR-MIT
git -C C:/src/CommonLibVR-MIT checkout 4190ec291f99c64b765c0647e25cf8a3a3d9a550
git -C C:/src/CommonLibVR-MIT submodule update --init --recursive
$env:COMMONLIB_SSE_FOLDER = 'C:/src/CommonLibVR-MIT'
$env:VCPKG_ROOT = 'C:/src/vcpkg'
```

Adjust those paths to your clones. In the SkyPrompt source directory, run:

```powershell
cmake --preset release
cmake --build build/release --parallel
```

CMake downloads the other dependencies using the revisions in `CMakeLists.txt`,
`vcpkg.json` and `cmake/ports`. The result is `build/release/SkyPrompt.dll`.
Leave mod-output environment variables unset to build without installing.

#### REBUILDING WITH A MODIFIED IMGUIVRHELPER CLIENT SDK

SkyPrompt compiles ImGuiVRHelper's LGPL-3.0-or-later `api/` files into its DLL.
The separate helper plugin's `src/` files are not compiled into SkyPrompt.
The exact client source revision is linked in [LICENSES.txt](LICENSES.txt).

Download that revision, edit the files under `api/`, then use your local copy
instead of CMake's download. The path below is the folder **containing** `api/`:

```powershell
cmake --preset release -DFETCHCONTENT_SOURCE_DIR_IMGUIVRHELPER=C:/src/imgui-vr-helper
cmake --build build/release --parallel
```

With Skyrim closed, back up the installed `SKSE/Plugins/SkyPrompt.dll` and replace
it with your rebuilt DLL. Keep ImGuiVRHelper installed separately for VR. To use
the original client SDK again, configure with
`-DFETCHCONTENT_SOURCE_DIR_IMGUIVRHELPER=` and rebuild.

#### PACKAGING A RELEASE

Keep `LICENSE`, `LICENSES.txt`, `README.md` and `licenses/ImGuiVRHelper/` in the
install archive. Each Nexus download must also link to its matching SkyPrompt
source commit or tag and these build instructions. Keep those sources and the
referenced dependency sources available for as long as required by their licenses;
do not point an older binary at a moving development branch.

#### FEATURES
Automatically imports:
- [CLibUtil](https://github.com/powerof3/CLibUtil) by powerof3
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) by Thiago099

#### THEME SPECIAL EFFECTS

See the [theme special effects guide](https://github.com/QTR-Modding/SkyPromptAPI/wiki/How-to-create-Themes%3F#special-visual-effects).

#### TRANSLATIONS

SkyPrompt loads Skyrim/SKSE translation tables from
`Data/Interface/Translations/SkyPrompt_<LANGUAGE>.txt`.

To add a language:

1. Copy `Interface/Translations/SkyPrompt_ENGLISH.txt`.
2. Rename `ENGLISH` to Skyrim's `sLanguage` value, such as `GERMAN`.
3. Translate only the text after each tab. Keep every `$SkyPrompt...` key unchanged.
4. Preserve `{}` placeholders and use `\n` for line breaks.
5. Save the file as UTF-16 little-endian with a BOM.

Do not put `##` in translated menu labels. Keep the four section names unique and do not
put `/` in them.

SkyPrompt automatically adds characters from the loaded table to its prompt/tutorial font
atlas. SKSE Menu Framework uses a separate font atlas for menu labels. For Chinese,
Japanese, Korean, Cyrillic, Thai, or Turkish text, enable the matching `Enable...` option
under `[Fonts]` in `Data/SKSE/Plugins/SKSEMenuFramework.ini` and select a `PrimaryFont`
that contains those glyphs.

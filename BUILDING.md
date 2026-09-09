# Building SkyPrompt

SkyPrompt-owned code remains proprietary; see [LICENSE](LICENSE). Third-party
licenses are listed in [LICENSES.txt](LICENSES.txt).

Use the [SkyPrompt source](https://github.com/QTR-Modding/SkyPrompt) commit or tag
linked with your Nexus download, rather than the latest development branch.
Install Visual Studio's C++ tools (MSVC with C++23 support, Windows SDK, CMake and
Ninja), Git, and [vcpkg](https://github.com/microsoft/vcpkg). Run these commands
from a Visual Studio Developer PowerShell configured for x64.

Set `VCPKG_ROOT` to your vcpkg folder:

```powershell
$env:VCPKG_ROOT = 'C:/src/vcpkg'
```

Clone [CommonLibVR-MIT](https://github.com/QTR-Modding/CommonLibVR-MIT) and select
the revision used by this version of SkyPrompt:

```powershell
git clone --recursive https://github.com/QTR-Modding/CommonLibVR-MIT.git C:/src/CommonLibVR-MIT
git -C C:/src/CommonLibVR-MIT checkout 4190ec291f99c64b765c0647e25cf8a3a3d9a550
git -C C:/src/CommonLibVR-MIT submodule update --init --recursive
$env:COMMONLIB_SSE_FOLDER = 'C:/src/CommonLibVR-MIT'
```

Adjust those paths to your clones. In the SkyPrompt source directory, run:

```powershell
cmake --preset release
cmake --build build/release --parallel
```

vcpkg downloads the other dependencies using the revisions in `vcpkg.json`
and `cmake/ports`. The result is `build/release/SkyPrompt.dll`.
Leave mod-output environment variables unset to build without installing.

## Rebuilding with a modified ImGuiVRHelper client SDK

SkyPrompt compiles ImGuiVRHelper's LGPL-3.0-or-later `api/` files into its DLL.
The separate helper plugin's `src/` files are not compiled into SkyPrompt.
The exact client source revision is linked in [LICENSES.txt](LICENSES.txt).

Download that revision, edit the files under `api/`, then use your local copy
instead of the packaged SDK. Point to the `api/` folder:

```powershell
cmake --preset release -DIMGUIVRHELPER_API_DIR=C:/src/imgui-vr-helper/api
cmake --build build/release --parallel
```

With Skyrim closed, back up the installed `SKSE/Plugins/SkyPrompt.dll` and replace
it with your rebuilt DLL. Keep ImGuiVRHelper installed separately for VR. To use
the original client SDK again, configure with
`-UIMGUIVRHELPER_API_DIR` and rebuild.

## Packaging a release

Keep `LICENSE`, `LICENSES.txt`, `README.md`, `BUILDING.md` and
`licenses/ImGuiVRHelper/` in the install archive. Each Nexus download must also
link to its matching SkyPrompt source commit or tag and these build instructions.
Keep those sources and the referenced dependency sources available for as long
as required by their licenses; do not point an older binary at a moving
development branch.

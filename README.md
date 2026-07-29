#### WINDOWS ENVIRONMENT VARIABLES TO SET

1. **`COMMONLIB_SSE_FOLDER`**: The path to your clone of Commonlib.
2. **`VCPKG_ROOT`**: The path to your clone of [vcpkg](https://github.com/microsoft/vcpkg).
3. (optional) **`SKYRIM_FOLDER`**: path of your Skyrim Special Edition folder.
4. (optional) **`SKYRIM_MODS_FOLDER`**: path of the folder where your mods are.

#### THINGS TO EDIT

1. In LICENSE:
- **`YEAR`**
- **`YOURNAME`**
2. CMakeLists.txt
- **`AUTHORNAME`**
- **`MDDNAME`**
- (optional) Your plugin version. Default: `0.1.0.0`
3. vcpkg.json
- **`name`**: Your plugin's name.
- **`version-string`**: Your plugin version. Default: `0.1.0.0`

#### FEATURES
Automatically imports:
- [CLibUtil](https://github.com/powerof3/CLibUtil) by powerof3
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) by Thiago099

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

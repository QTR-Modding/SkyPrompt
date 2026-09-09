#### BUILDING

See [BUILDING.md](BUILDING.md).

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

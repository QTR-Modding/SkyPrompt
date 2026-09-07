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

#### THEME SPECIAL EFFECTS

In the Theme menu, open Special Effects and choose Add Effect. Each effect has its
own settings; the X beside its name removes it. Text Background adds a text panel,
Progress Circle customizes the button's progress feedback, and List Indicators
customizes the arrows for hidden list rows. Activation Pop adds an expanding,
fading copy of the activated icon and text; the original stays in place.

To combine effects in a theme JSON file, put their settings in a `special_effects` array:

```json
"special_effects": [
    { "special_effect": 1 },
    { "special_effect": 2, "special_floats": [8.0, 4.0, 3.0] }
]
```

Each entry accepts `special_integers`, `special_floats`, `special_strings`, and
`special_bools`. ID 1 is Viny Arcs; ID 2 is Text Background. Existing single-effect
files still work. When present, the array takes precedence; an empty array removes
all configured effects. Export Theme writes the selected theme's effects and settings.

IDs 3-5 are reserved for the following SkyPrompt effects. Omitted parameters use
their defaults, so this entry alone enables Activation Pop:

```json
{ "special_effect": 5 }
```

Without ID 3 or 4, normal progress feedback and list arrows remain unchanged.
Adding either lets you customize or hide those parts. Without ID 5, there is no
activation pop. Viewing settings does not add parameters; editing a later
parameter fills earlier omitted positions with their defaults.

Array positions below are zero-based. Write floating-point JSON values with a
decimal point, such as `1.0`. Colors occupy `special_integers` as unsigned packed
ABGR values; JSON uses decimal integers, and the menu's color picker handles the
conversion. Color zero is fully transparent, not omitted.

##### ID 3: Progress Circle

`special_floats`:

| Index | Parameter | Default | Range |
| --- | --- | --- | --- |
| 0 | Radius multiplier | 1.0 | 0-4 |
| 1 | Thickness multiplier | 1.0 | 0-8 |
| 2 | Horizontal offset (screen pixels) | 0.0 | -500 to 500 |
| 3 | Vertical offset (screen pixels) | 0.0 | -500 to 500 |
| 4 | Clockwise rotation (degrees) | 0.0 | -360 to 360 |
| 5 | Hold marker size multiplier | 1.0 | 0-4 |

`special_integers`, in order: progress arc (white, alpha 180), completed
(RGBA 228,185,76,180), track (white, alpha 30), hold marker (white, alpha 180),
remove marker (RGBA 147,39,41,180), skip marker (RGBA 228,185,76,100).

`special_bools`, in order: show arc, show track, show hold marker, show
special-command marks, clockwise. All default to `true`. These options change
appearance, not how long an action must be held.

##### ID 4: List Indicators

`special_floats`:

| Index | Parameter | Default | Range |
| --- | --- | --- | --- |
| 0 | Arrow size multiplier | 1.0 | 0-4 |
| 1 | Horizontal offset (screen pixels) | 0.0 | -500 to 500 |
| 2 | Vertical offset (screen pixels) | 0.0 | -500 to 500 |
| 3 | Extra space between arrow and nearest row (screen pixels) | 0.0 | 0-500 |

`special_integers`: up arrow color, then down arrow color; both default to opaque
white. `special_bools`: show arrows, default `true`. Hiding arrows or setting size
to zero also removes their reserved space. This effect applies only to List.

##### ID 5: Activation Pop

`special_floats`:

| Index | Parameter | Default | Range |
| --- | --- | --- | --- |
| 0 | Duration (seconds) | 0.3 | 0.01-3 |
| 1 | End scale | 1.25 | 1-3 |
| 2 | Starting opacity | 0.6 | 0-1 |

There are no color or toggle parameters. The copy includes the icon, text, and
text shadow, not the progress circle or text background. It can finish after
the original prompt disappears.

To contribute a host effect, choose an unused stable ID without colliding with
SkyPromptAddOn; do not reuse or renumber existing IDs. Add its named indexes,
defaults, bounds, and definition in `src/ImGui/PromptEffects.h/.cpp`, register its
ID in the Theme menu's supported effects, and add rendering in the appropriate
prompt drawing path. Add English labels/help to both translation sources,
document the parameter order here, and verify omitted defaults, editing,
save/reload, and drawing in the relevant layouts. IDs 1-2 remain in SkyPromptAddOn;
host effects are implemented and released with SkyPrompt.

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

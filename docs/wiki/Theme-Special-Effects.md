Special effects change how prompts look. You can set them up in-game without editing JSON.

## Try it in-game

Activation Pop adds a growing, fading copy when you activate a prompt.

1. Open **SkyPrompt > Theme** in the Mod Control Panel and choose the theme marked **(active)**.
2. Open **Special Effects**, choose **Add Effect**, then **Activation Pop**.
3. Enable **Icon Only** if you want just the button icon to pop. Close the menu and activate a prompt to try it.

You can adjust the pop's duration, size, and opacity, or leave the defaults.
The X beside an effect removes it. Use **Save Theme** to keep changes to a named
theme; **Default** saves automatically. **Export Theme** saves a separate theme file.

## Edit a theme file

Here is a complete, minimal theme with Activation Pop:

```json
{
    "name": "My Theme",
    "special_effects": [
        { "special_effect": 5 }
    ]
}
```

`special_effects` is the list of effects to use. Each entry's `special_effect`
number identifies an effect; `5` means Activation Pop. Settings you leave out use
their defaults.

When editing an existing theme, keep its other fields and change only its
`special_effects` section. Theme files live in `Data/SKSE/Plugins/SkyPrompt/themes`.
After editing the file your mod uses, press **Reload Themes**. Creating a file
with a new name does not automatically make a mod use it.

## Combine effects

Add another entry to the list. This example combines a text background with an
icon-only activation pop:

```json
{
    "name": "My Theme",
    "special_effects": [
        { "special_effect": 2, "special_floats": [8.0, 4.0, 3.0] },
        { "special_effect": 5, "special_bools": [true] }
    ]
}
```

The background gets 8 pixels of horizontal padding, 4 pixels of vertical padding,
and a 3-pixel corner radius. The pop's `true` turns on **Icon Only**.

<details>
<summary>JSON settings reference</summary>

### Effect numbers

| Number | Effect |
| --- | --- |
| 1 | Viny Arcs |
| 2 | Text Background |
| 3 | Progress Circle |
| 4 | List Indicators |
| 5 | Activation Pop |

Normal progress feedback and List arrows do not need an effect entry. Add effect
3 or 4 to customize or hide them. Activation Pop appears only when you add effect 5.

### Reading the settings

`special_floats` holds decimal numbers, `special_bools` holds `true`/`false`
switches, and `special_integers` holds whole numbers such as colors.
`special_strings` is available for effects that need text values; effects 3-5 do not use it.

Values go in square brackets, in the order shown below. Position 0 means the
first value. For example, Activation Pop's `[0.3, 1.25, 0.6]` means a duration of
0.3 seconds, an end scale of 1.25, and an opacity of 0.6. To change a later value,
include the earlier values too. You can leave out trailing values to keep their defaults.

Write decimal values with a decimal point, such as `1.0` rather than `1`.
For colors, use the in-game color picker and **Export Theme** to get the numbers
for your JSON. For manual conversion, the format is unsigned packed ABGR, written
as a decimal integer. Color `0` is fully transparent. In the color defaults below,
alpha ranges from 0 (transparent) to 255 (opaque).

### Progress Circle (ID 3)

These settings change the circle's appearance, not how long an action must be held.

`special_floats`:

| Position | Setting | Default | Range |
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
special-command marks, clockwise. All default to `true`. Turning clockwise off
also mirrors the arc's starting position to the other side of the hold marker.

### List Indicators (ID 4)

These settings control the arrows that show when more List rows are hidden above or below.

`special_floats`:

| Position | Setting | Default | Range |
| --- | --- | --- | --- |
| 0 | Arrow size multiplier | 1.0 | 0-4 |
| 1 | Horizontal offset (screen pixels) | 0.0 | -500 to 500 |
| 2 | Vertical offset (screen pixels) | 0.0 | -500 to 500 |
| 3 | Extra space between arrow and nearest row (screen pixels) | 0.0 | 0-500 |

`special_integers`: up arrow color, then down arrow color; both default to opaque white.

`special_bools`: show arrows, default `true`. Hiding arrows or setting their size
to zero also removes their reserved space. This effect applies only to List.

### Activation Pop (ID 5)

The pop is a temporary copy of the activated icon and text. It grows and fades
without moving the original. **Icon Only** leaves out the text and grows the icon
from its center.

`special_floats`:

| Position | Setting | Default | Range |
| --- | --- | --- | --- |
| 0 | Duration (seconds) | 0.3 | 0.01-3 |
| 1 | End scale (1 keeps the original size) | 1.25 | 1-3 |
| 2 | Starting opacity (0 is invisible, 1 is opaque) | 0.6 | 0-1 |

`special_bools`: icon only, default `false`.

The full copy includes the text shadow, but not the progress circle or text
background. It can finish after the original prompt disappears. There are no color settings.

### Older theme files

The older single-effect format still works. If a file contains both that format
and `special_effects`, the list takes precedence. An empty list (`"special_effects": []`)
removes configured effects; normal progress circles and List arrows remain.

</details>

<details>
<summary>For developers adding a new effect to SkyPrompt</summary>

1. Choose an unused effect number and keep existing numbers unchanged. IDs 1-2 belong to SkyPromptAddOn; IDs 3-5 belong to SkyPrompt.
2. Add the parameter names, defaults, ranges, and effect definition in `src/ImGui/PromptEffects.h/.cpp`. Add the effect to the Theme menu's supported effects in `src/MCP.cpp`.
3. Add drawing code to the appropriate existing prompt drawing module. Add English labels and help text to both `src/Translations.cpp` and `Interface/Translations/SkyPrompt_ENGLISH.txt`.
4. Document the settings on this page. Check defaults, custom values, menu editing, save/reload, export, and drawing in the relevant layouts. Effects implemented in SkyPrompt ship with SkyPrompt, not SkyPromptAddOn.

</details>

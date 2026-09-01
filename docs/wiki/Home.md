# SkyPrompt

> An advanced ImGui‑powered prompt framework for Skyrim that lets mod authors (and dependent mods) show interactive, context‑aware on‑screen prompts.
> SkyPrompt itself does NOT add gameplay; it is a framework used by other mods.<br>
Nexus Link: https://www.nexusmods.com/skyrimspecialedition/mods/148703
---

## 👩‍💻 For Mod Authors

| Author Type | Start Here |
|-------------|-----------|
| Native SKSE DLL (C++ ) | https://github.com/QTR-Modding/SkyPromptAPI/wiki/SkyPrompt-API-Tutorial-(cpp) |
| Papyrus / Script | https://github.com/QTR-Modding/SkyPromptAPI/wiki/SkyPrompt-API-Tutorial-(Papyrus) |

These tutorials show how to register prompts, manage interaction states, customize visuals, and tap into advanced features.

---

## ✨ Core Concept

See nexus for examples.

SkyPrompt provides a unified “prompt bar” / interaction overlay:
- Shows buttons, actions, or instructions tied to keyboard, mouse, or multiple gamepad types.
- Lets prompts be stacked, cycled, attached to things, or persisted while held.
- Allows other mods to push, update, or clear prompts through a shared API.
- Includes a first‑time guided tutorial and a settings menu (appearance, input, themes).
- Mod authors using SkyPrompt's API have many options to control various aspects of the prompts they send (e.g., progress bar, text color, etc.).

---

## 🧩 Feature Highlights

- Overlay prompt system (1–4 slots depending on configuration)
- Attach prompts to world object references (3D)
- Single click, hold-to-interact, and hold-to-keep mechanics
- Stack multiple prompts in one list
- Cycle through prompts or delete them via multi-press “special commands”
- Hold-and-keep mode so a prompt persists while active
- Progress circle & dynamic text + color control
- Attach to inventory 3D objects
- Theming (fonts, icons, layout, spacing)
- Tutorial + in‑game settings + log panel

---

## 📬 Quick Start (User)

1. Install (SKSE plugin) like other DLL mods.
2. Run game → tutorial appears (first time).
3. Open mod settings (Menu Framework) to configure position, size, device keys, themes.
4. Install mods that integrate with SkyPrompt to see it in action.

---

## 🧪 Quick Start (Developer)

1. Read the appropriate API tutorial (C++ or Papyrus).
2. Add prompts via API.
3. Test edge cases: cycling, multi-press commands, hold interactions.
4. Provide a theme or rely on the defaults.

---

Have a feature request, issue, or integration idea?  
Open an issue or reach out on Discord. Enjoy building with SkyPrompt!
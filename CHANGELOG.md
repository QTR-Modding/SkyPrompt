# Changelog

## 2.3.8.1 - 2025-12-10 (PR #31)
### Added
- Added localization for prompt text (including embedded translation tokens) so SKSE translations are applied before prompts are processed.
- Introduced `SendPromptForControl`, allowing prompts to be sent based on keyboard, mouse, or gamepad bindings via the SkyPrompt API.

### Changed
- Replaced `clib_util::singleton::ISingleton` inheritance with `REX::Singleton` across singletons.
- Refined input key conversion and lookup to avoid double offsets and speed up key searches.
- Scoped the `OnMessage` handler in `plugin.cpp` and set `lorerim_mods_output` to `false` by default.
- Updated dependency ports and bumped the project version to `2.3.8.1`.

### Removed
- Deleted an unused mutex from the Papyrus bindings.

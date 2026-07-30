#include "Translations.h"

namespace {
    std::string Unescape(const std::string_view a_text) {
        std::string result;
        result.reserve(a_text.size());

        for (std::size_t i = 0; i < a_text.size(); ++i) {
            if (a_text[i] == '\\' && i + 1 < a_text.size()) {
                switch (a_text[i + 1]) {
                    case 'n':
                        result.push_back('\n');
                        ++i;
                        continue;
                    case 't':
                        result.push_back('\t');
                        ++i;
                        continue;
                    case '\\':
                        result.push_back('\\');
                        ++i;
                        continue;
                    default:
                        break;
                }
            }
            result.push_back(a_text[i]);
        }

        return result;
    }

    const StringMap<std::string> english = {
        {"$SkyPromptTutorialQuit", "Quit Tutorial"},
        {"$SkyPromptTutorialMash", "Quick! Mash Me!"},
        {"$SkyPromptTutorialPress", "Quick! Press Me!"},
        {"$SkyPromptTutorialDeleteAll", "Delete All: Triple Press and Hold the Button!"},
        {"$SkyPromptTutorialSkipNext", "Skip to Next: Triple Press the Button!"},
        {"$SkyPromptTutorialDelete", "Delete Me: Double Press the Button!"},
        {"$SkyPromptTutorialAccept", "Accept Prompt: Hold the Button!"},
        {"$SkyPromptTutorialMenuInfo",
         "SkyPrompt comes with an In-Game Menu for customization!\n"
         "To access the menu:\n"
         "{} - {} ({})\n"
         "{} - {} ({})"},
        {"$SkyPromptTutorialDeviceKeyboard", "Keyboard"},
        {"$SkyPromptTutorialDeviceGamepad", "Gamepad"},
        {"$SkyPromptTutorialToggleSinglePress", "Single Press"},
        {"$SkyPromptTutorialToggleHold", "Hold"},
        {"$SkyPromptTutorialToggleDoublePress", "Double Press"},
        {"$SkyPromptTutorialToggleOff", "Off"},
        {"$SkyPromptTutorialWelcome",
         "Thank you for installing SkyPrompt!\n"
         "I would like to show you a couple tips on how to handle the prompts!"},
        {"$SkyPromptTutorialTitle", "SkyPrompt Tutorial:"},
        {"$SkyPromptButtonOK", "Ok"},
        {"$SkyPromptButtonSure", "Sure!"},
        {"$SkyPromptButtonPass", "Pass..."},
        {"$SkyPromptButtonRestart", "Restart"},
        {"$SkyPromptButtonEnd", "End"},

        {"$SkyPromptMCPHelpMarker", "(?)"},
        {"$SkyPromptMCPPromptOrderTextFirst", "Text First (text then icon)"},
        {"$SkyPromptMCPPromptOrderIconFirst", "Icon First (icon then text)"},
        {"$SkyPromptMCPPromptAlignmentRadial", "Radial"},
        {"$SkyPromptMCPPromptAlignmentHorizontal", "Horizontal"},
        {"$SkyPromptMCPPromptAlignmentVertical", "Vertical"},
        {"$SkyPromptMCPPromptPivotTopLeft", "Top Left"},
        {"$SkyPromptMCPPromptPivotTopRight", "Top Right"},
        {"$SkyPromptMCPPromptPivotBottomLeft", "Bottom Left"},
        {"$SkyPromptMCPPromptPivotCenter", "Center"},
        {"$SkyPromptMCPPromptPivotBottomRight", "Bottom Right"},
        {"$SkyPromptMCPSettingsEnableMod", "Enable Mod"},
        {"$SkyPromptMCPSettingsStartTutorial", "Start Tutorial"},
        {"$SkyPromptMCPSettingsResetDefaults", "Reset to Defaults"},
        {"$SkyPromptMCPSettingsDrawDebug", "Draw Debug"},
        {"$SkyPromptMCPSettingsFadeSpeed", "Fade Speed"},
        {"$SkyPromptMCPSettingsXPercent", "X Percent"},
        {"$SkyPromptMCPSettingsYPercent", "Y Percent"},
        {"$SkyPromptMCPSettingsMarginX", "Margin X"},
        {"$SkyPromptMCPSettingsMarginY", "Margin Y"},
        {"$SkyPromptMCPSettingsPromptSize", "Prompt Size"},
        {"$SkyPromptMCPSettingsIcon2FontRatio", "Icon2Font Ratio"},
        {"$SkyPromptMCPSettingsPromptOrder", "Prompt Order"},
        {"$SkyPromptMCPSettingsPromptAlignment", "Prompt Alignment"},
        {"$SkyPromptMCPSettingsPromptPivot", "Prompt Pivot"},
        {"$SkyPromptMCPSettingsLineSpacing", "Line Spacing"},
        {"$SkyPromptMCPSettingsProgressSpeed", "Progress Speed"},
        {"$SkyPromptMCPSettingsLifetime", "Lifetime"},

        {"$SkyPromptMCPSectionSettings", "Settings"},
        {"$SkyPromptMCPSectionControls", "Controls"},
        {"$SkyPromptMCPSectionTheme", "Theme"},
        {"$SkyPromptMCPSectionLog", "Log"},
        {"$SkyPromptMCPLogTrace", "Trace"},
        {"$SkyPromptMCPLogInfo", "Info"},
        {"$SkyPromptMCPLogWarning", "Warning"},
        {"$SkyPromptMCPLogError", "Error"},
        {"$SkyPromptMCPLogGenerate", "Generate Log"},

        {"$SkyPromptMCPSettingsOnScreenPosition", "On-Screen Position"},
        {"$SkyPromptMCPPositionCustom", "Custom"},
        {"$SkyPromptMCPPositionBottom", "Bottom"},
        {"$SkyPromptMCPPositionBottomRight", "BottomRight"},
        {"$SkyPromptMCPPositionBottomRightSlight", "BottomRightSlight"},
        {"$SkyPromptMCPPositionBottomLeft", "BottomLeft"},
        {"$SkyPromptMCPPositionBottomLeftSlight", "BottomLeftSlight"},
        {"$SkyPromptMCPPositionTop", "Top"},
        {"$SkyPromptMCPPositionTopRight", "TopRight"},
        {"$SkyPromptMCPPositionTopRightSlight", "TopRightSlight"},
        {"$SkyPromptMCPPositionTopLeft", "TopLeft"},
        {"$SkyPromptMCPPositionTopLeftSlight", "TopLeftSlight"},
        {"$SkyPromptMCPPositionCenter", "Center"},
        {"$SkyPromptMCPPositionCenterRight", "CenterRight"},
        {"$SkyPromptMCPPositionCenterRightSlight", "CenterRightSlight"},
        {"$SkyPromptMCPPositionCenterLeft", "CenterLeft"},
        {"$SkyPromptMCPPositionCenterLeftSlight", "CenterLeftSlight"},
        {"$SkyPromptMCPPositionCenterTop", "CenterTop"},
        {"$SkyPromptMCPPositionCenterTopRight", "CenterTopRight"},
        {"$SkyPromptMCPPositionCenterTopRightSlight", "CenterTopRightSlight"},
        {"$SkyPromptMCPPositionCenterTopLeft", "CenterTopLeft"},
        {"$SkyPromptMCPPositionCenterTopLeftSlight", "CenterTopLeftSlight"},
        {"$SkyPromptMCPPositionCenterBottom", "CenterBottom"},
        {"$SkyPromptMCPPositionCenterBottomRight", "CenterBottomRight"},
        {"$SkyPromptMCPPositionCenterBottomRightSlight", "CenterBottomRightSlight"},
        {"$SkyPromptMCPPositionCenterBottomLeft", "CenterBottomLeft"},
        {"$SkyPromptMCPPositionCenterBottomLeftSlight", "CenterBottomLeftSlight"},

        {"$SkyPromptMCPThemeFont", "Font"},
        {"$SkyPromptMCPThemeFontShadow", "Font Shadow"},
        {"$SkyPromptMCPThemeFontHelp",
         "Adds Japanese, Korean, and full Chinese glyph sets. Increases font atlas size and loading time."},
        {"$SkyPromptMCPThemeName", "Theme Name"},
        {"$SkyPromptMCPThemeDescription", "Description"},
        {"$SkyPromptMCPThemeAuthor", "Author"},
        {"$SkyPromptMCPThemeVersion", "Version"},
        {"$SkyPromptMCPThemeSpecialSettings", "Special Effect Parameters (Advanced)"},
        {"$SkyPromptMCPThemeSpecialEffect", "Special Effect ID"},
        {"$SkyPromptMCPThemeSpecialIntegers", "Integer Parameters"},
        {"$SkyPromptMCPThemeSpecialStrings", "String Parameters"},
        {"$SkyPromptMCPThemeSpecialFloats", "Float Parameters"},
        {"$SkyPromptMCPThemeSpecialBools", "Boolean Parameters"},
        {"$SkyPromptMCPThemeExportOpen", "Export as Theme"},
        {"$SkyPromptMCPThemeExportTitle", "Export Theme"},
        {"$SkyPromptMCPThemeExportFilename", "Filename"},
        {"$SkyPromptMCPThemeExport", "Export"},
        {"$SkyPromptMCPThemeExportCancel", "Cancel"},
        {"$SkyPromptMCPThemeExportEmpty", "Enter a filename."},
        {"$SkyPromptMCPThemeExportInvalid", "The filename is invalid on Windows."},
        {"$SkyPromptMCPThemeExportExists", "A theme with this filename already exists."},
        {"$SkyPromptMCPThemeExportFailed", "Failed to export the theme. Check SkyPrompt.log."},
        {"$SkyPromptMCPThemeExportSuccess", "Exported theme to: {}"},
        {"$SkyPromptMCPDeviceKeyboardMouse", "Keyboard & Mouse"},
        {"$SkyPromptMCPDeviceGamepadXbox", "Gamepad (Xbox)"},
        {"$SkyPromptMCPDeviceGamepadPS4", "Gamepad (PS4)"},
        {"$SkyPromptMCPDeviceUnknown", "Unknown"},
        {"$SkyPromptMCPControlsCycleControls", "Cycle Controls"},
        {"$SkyPromptMCPControlsCycleLeft", "Cycle L"},
        {"$SkyPromptMCPControlsCycleRight", "Cycle R"},
        {"$SkyPromptMCPControlsMaxButtons", "Max Buttons"},
        {"$SkyPromptMCPControlsDeviceSelection", "Device Selection:"},
        {"$SkyPromptMCPControlsButton", "Button {}"},
        {"$SkyPromptMCPThemeReloadThemes", "Reload Themes"},
        {"$SkyPromptMCPSpecialCommands", "Special Commands"},
        {"$SkyPromptMCPSpecialCommandsHelp",
         "Double press: delete current prompt\n"
         "Triple press: cycle through prompts\n"
         "Triple press and hold: delete all prompts"},
        {"$SkyPromptMCPSpecialCommandsVisualize", "Visualize"},
        {"$SkyPromptMCPSpecialCommandsVisualizeHelp", "Visualization of the special commands"},
        {"$SkyPromptMCPSpecialCommandsResponsiveness", "Responsiveness"},
        {"$SkyPromptMCPSpecialCommandsResponsivenessHelp",
         "Higher values gives you less time to press the next key in return for faster response"},
    };

    StringMap<std::string> strings = english;
    std::string glyph_text;
    std::mutex glyph_text_mutex;
}

void Translations::Load() {
    strings = english;
    SKSE::Translation::ParseTranslation("SkyPrompt");

    for (auto& [key, value] : strings) {
        std::string translated;
        if (SKSE::Translation::Translate(key, translated)) {
            value = Unescape(translated);
        }
    }

    std::string loaded_glyph_text;
    const auto append_glyphs = [&loaded_glyph_text](const auto& table) {
        for (const auto& entry : table) {
            loaded_glyph_text.append(entry.second);
            loaded_glyph_text.push_back('\n');
        }
    };

    append_glyphs(strings);
    append_glyphs(english);

    {
        std::lock_guard lock(glyph_text_mutex);
        glyph_text = std::move(loaded_glyph_text);
    }
}

const std::string* Translations::TryGet(const std::string_view a_key) {
    const auto it = strings.find(a_key);
    return it != strings.end() ? std::addressof(it->second) : nullptr;
}

const std::string& Translations::Get(const std::string_view a_key) {
    if (const auto* value = TryGet(a_key)) {
        return *value;
    }

    logger::warn("Translation key not found: {}", a_key);
    static const std::string empty;
    return empty;
}

const std::string& Translations::GetEnglish(const std::string_view a_key) {
    if (const auto it = english.find(a_key); it != english.end()) {
        return it->second;
    }

    logger::warn("English translation key not found: {}", a_key);
    static const std::string empty;
    return empty;
}

std::string Translations::GlyphText() {
    std::lock_guard lock(glyph_text_mutex);
    return glyph_text;
}

std::string Translations::FormatArgs(const std::string_view a_key, const std::format_args a_args) {
    const auto& format = Get(a_key);
    try {
        return std::vformat(format, a_args);
    } catch (const std::format_error& e) {
        logger::error("Invalid translated format for '{}': {}", a_key, e.what());
    }

    if (const auto fallback = english.find(a_key); fallback != english.end()) {
        try {
            return std::vformat(fallback->second, a_args);
        } catch (const std::format_error& e) {
            logger::error("Invalid English fallback format for '{}': {}", a_key, e.what());
            return fallback->second;
        }
    }

    return format;
}

std::string Translations::WithID(const std::string_view a_visible_text, const std::string_view a_stable_id) {
    return std::format("{}###{}", a_visible_text, a_stable_id);
}

std::string Translations::ImGuiLabel(const std::string_view a_key, const std::string_view a_stable_id) {
    return WithID(Get(a_key), a_stable_id);
}

std::string Translations::MenuToggleMode(const std::string_view a_mode) {
    std::string normalized;
    normalized.reserve(a_mode.size());
    for (const unsigned char c : a_mode) {
        if (std::isalnum(c)) {
            normalized.push_back(static_cast<char>(std::toupper(c)));
        }
    }

    if (normalized == "SINGLEPRESS") {
        return Get("$SkyPromptTutorialToggleSinglePress");
    }
    if (normalized == "HOLD") {
        return Get("$SkyPromptTutorialToggleHold");
    }
    if (normalized == "DOUBLEPRESS") {
        return Get("$SkyPromptTutorialToggleDoublePress");
    }
    if (normalized == "OFF") {
        return Get("$SkyPromptTutorialToggleOff");
    }
    return std::string(a_mode);
}

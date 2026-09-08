#include "MCP.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "Utils.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Hooks.h"
#include "IconsFonts.h"
#include "Settings.h"
#include "Translations.h"
#include "Tutorial.h"
#include "SkyPrompt/AddOns.hpp"
#include "PromptEffects.h"

namespace {
    void HelpMarker(const std::string_view a_key) {
        const auto& marker = Translations::Get("$SkyPromptMCPHelpMarker");
        ImGuiMCP::TextDisabled("%s", marker.c_str());
        if (ImGuiMCP::BeginItemTooltip()) {
            ImGuiMCP::PushTextWrapPos(ImGuiMCP::GetFontSize() * 35.0f);
            const auto& text = Translations::Get(a_key);
            ImGuiMCP::TextUnformatted(text.c_str());
            ImGuiMCP::PopTextWrapPos();
            ImGuiMCP::EndTooltip();
        }
    }

    constexpr int menuSpacingStyleCount = 3;

    void PushMenuSpacing() {
        constexpr float horizontalGapEm = 0.5f;
        constexpr float verticalGapEm = 0.25f;
        constexpr float cellPaddingEm = 0.15f;
        const float fontSize = ImGuiMCP::GetFontSize();
        const ImGuiMCP::ImVec2 spacing{fontSize * horizontalGapEm, fontSize * verticalGapEm};
        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_ItemSpacing, spacing);
        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_ItemInnerSpacing, spacing);
        ImGuiMCP::PushStyleVar(ImGuiMCP::ImGuiStyleVar_CellPadding, {spacing.x, fontSize * cellPaddingEm});
    }

    bool BeginSettingsTable(const char* a_id) {
        if (!ImGuiMCP::BeginTable(a_id, 2, ImGuiMCP::ImGuiTableFlags_SizingStretchProp |
                                            ImGuiMCP::ImGuiTableFlags_NoSavedSettings)) return false;
        ImGuiMCP::TableSetupColumn("label", ImGuiMCP::ImGuiTableColumnFlags_WidthFixed);
        ImGuiMCP::TableSetupColumn("value", ImGuiMCP::ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    std::string SettingRow(const std::string_view a_label, const std::string_view a_id,
                           const std::string_view a_help_key = {}) {
        ImGuiMCP::TableNextRow();
        ImGuiMCP::TableSetColumnIndex(0);
        ImGuiMCP::AlignTextToFramePadding();
        ImGuiMCP::TextUnformatted(a_label.data(), a_label.data() + a_label.size());
        if (!a_help_key.empty()) {
            ImGuiMCP::SameLine();
            HelpMarker(a_help_key);
        }
        ImGuiMCP::TableSetColumnIndex(1);
        constexpr float controlWidthEm = 16.0f;
        ImGuiMCP::SetNextItemWidth(std::min(ImGuiMCP::GetContentRegionAvail().x,
                                          ImGuiMCP::GetFontSize() * controlWidthEm));
        return std::format("##{}", a_id);
    }

    bool BeginSettingsGroup(const std::string_view a_key, const std::string_view a_id) {
        ImGuiMCP::Spacing();
        const auto label = Translations::ImGuiLabel(a_key, a_id);
        return ImGuiMCP::CollapsingHeader(label.c_str()) &&
               BeginSettingsTable(std::format("{}.fields", a_id).c_str());
    }

    const std::string& PromptOrderLabel(const Theme::PromptOrder a_order) {
        switch (a_order) {
            case Theme::kTextFirst:
                return Translations::Get("$SkyPromptMCPPromptOrderTextFirst");
            case Theme::kIconFirst:
            default:
                return Translations::Get("$SkyPromptMCPPromptOrderIconFirst");
        }
    }

    const std::string& PromptAlignmentLabel(const Theme::PromptAlignment a_alignment) {
        switch (a_alignment) {
            case Theme::kRadial:
                return Translations::Get("$SkyPromptMCPPromptAlignmentRadial");
            case Theme::kHorizontal:
                return Translations::Get("$SkyPromptMCPPromptAlignmentHorizontal");
            case Theme::kDiamond:
                return Translations::Get("$SkyPromptMCPPromptAlignmentDiamond");
            case Theme::kList:
                return Translations::Get("$SkyPromptMCPPromptAlignmentList");
            case Theme::kVertical:
            default:
                return Translations::Get("$SkyPromptMCPPromptAlignmentVertical");
        }
    }

    const std::string& PromptPivotLabel(const Theme::PromptPivot a_pivot) {
        switch (a_pivot) {
            case Theme::kTopLeft:
                return Translations::Get("$SkyPromptMCPPromptPivotTopLeft");
            case Theme::kTopRight:
                return Translations::Get("$SkyPromptMCPPromptPivotTopRight");
            case Theme::kBottomLeft:
                return Translations::Get("$SkyPromptMCPPromptPivotBottomLeft");
            case Theme::kCenter:
                return Translations::Get("$SkyPromptMCPPromptPivotCenter");
            case Theme::kBottomRight:
            default:
                return Translations::Get("$SkyPromptMCPPromptPivotBottomRight");
        }
    }

    const std::string& DeviceLabel(const Input::DEVICE a_device) {
        switch (a_device) {
            case Input::DEVICE::kKeyboardMouse:
                return Translations::Get("$SkyPromptMCPDeviceKeyboardMouse");
            case Input::DEVICE::kGamepad:
                return Translations::Get("$SkyPromptMCPDeviceGamepad");
            case Input::DEVICE::kVR:
                return Translations::Get("$SkyPromptMCPDeviceVR");
            default:
                return Translations::Get("$SkyPromptMCPDeviceUnknown");
        }
    }

    const std::string& PositionLabel(const std::string_view a_name) {
        return Translations::Get(std::format("$SkyPromptMCPPosition{}", a_name));
    }

    bool LocalizedCheckbox(const std::string_view a_key, const std::string_view a_id, bool* a_value) {
        const auto label = Translations::ImGuiLabel(a_key, a_id);
        return ImGuiMCP::Checkbox(label.c_str(), a_value);
    }

    bool LocalizedCheckboxText(const std::string_view a_text, const std::string_view a_id, bool* a_value) {
        const auto label = Translations::WithID(a_text, a_id);
        return ImGuiMCP::Checkbox(label.c_str(), a_value);
    }

    bool LocalizedButton(const std::string_view a_key, const std::string_view a_id) {
        const auto label = Translations::ImGuiLabel(a_key, a_id);
        return ImGuiMCP::Button(label.c_str());
    }

    bool SettingCombo(const std::string_view a_key, const std::string_view a_id, const char* a_preview,
                      const std::string_view a_help_key = {}) {
        const auto label = SettingRow(Translations::Get(a_key), a_id, a_help_key);
        return ImGuiMCP::BeginCombo(label.c_str(), a_preview);
    }

    bool LocalizedSelectableText(const std::string_view a_text, const std::string_view a_id, const bool a_selected) {
        const auto label = Translations::WithID(a_text, a_id);
        return ImGuiMCP::Selectable(label.c_str(), a_selected);
    }

    bool SliderFloatCommitted(const char* a_label, float* a_value,
                              const float a_min, const float a_max) {
        ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max);
        return ImGuiMCP::IsItemDeactivatedAfterEdit();
    }

    bool SettingFloat(const std::string_view a_key, const std::string_view a_id, float* a_value,
                      const float a_min, const float a_max, const std::string_view a_help_key = {}) {
        const auto label = SettingRow(Translations::Get(a_key), a_id, a_help_key);
        return SliderFloatCommitted(label.c_str(), a_value, a_min, a_max);
    }

    void LocalizedText(const std::string_view a_key) {
        const auto& text = Translations::Get(a_key);
        ImGuiMCP::TextUnformatted(text.c_str());
    }

    struct ThemeEditor {
        std::string filename;
        std::string status;

        Theme::Theme& GetTheme() const {
            return filename.empty() ? Theme::default_theme : Theme::themes_loaded.at(filename);
        }

        void OnChanged() {
            status.clear();
            if (filename.empty()) MCP::Settings::to_json();
        }

        void RenderSelector();
        bool RenderLayout();
        bool RenderPosition();
        bool RenderAppearance();
        bool RenderAnimation();
        bool RenderSpecialEffects();
    };

    ThemeEditor theme_editor;

    void ThemeEditor::RenderSelector() {
        if (!BeginSettingsTable("theme.editor.fields")) return;
        const auto label = [](const std::string_view a_name, const Theme::Theme& a_theme) {
            return &a_theme == Theme::last_theme
                       ? Translations::Format("$SkyPromptMCPThemeActive", a_name)
                       : std::string(a_name);
        };
        const auto& default_name = Translations::Get("$SkyPromptMCPThemeDefault");
        const auto preview = label(filename.empty() ? default_name : filename, GetTheme());
        if (SettingCombo("$SkyPromptMCPSectionTheme", "theme.editor", preview.c_str())) {
            if (LocalizedSelectableText(label(default_name, Theme::default_theme),
                                        "theme.editor.default", filename.empty())) {
                filename.clear();
                status.clear();
            }
            if (filename.empty()) ImGuiMCP::SetItemDefaultFocus();
            for (const auto& [name, theme] : Theme::themes_loaded) {
                const bool selected = filename == name;
                if (LocalizedSelectableText(label(name, theme), std::format("theme.editor.file.{}", name), selected)) {
                    filename = name;
                    status.clear();
                }
                if (selected) ImGuiMCP::SetItemDefaultFocus();
            }
            ImGuiMCP::EndCombo();
        }
        ImGuiMCP::EndTable();
        if (!filename.empty()) {
            if (LocalizedButton("$SkyPromptMCPThemeSave", "theme.save")) {
                const auto path = std::filesystem::path(Theme::themes_folder) / (filename + ".json");
                status = GetTheme().Save(path)
                             ? Translations::Format("$SkyPromptMCPThemeSaveSuccess", filename)
                             : Translations::Get("$SkyPromptMCPThemeSaveFailed");
            }
        }
        if (!status.empty()) ImGuiMCP::TextUnformatted(status.c_str());
        ImGuiMCP::Separator();
    }

    void SyncOSPPresetSelection(const Theme::Theme& settings) {
        constexpr float epsilon = 0.0001f;

        if (std::abs(settings.marginX) <= epsilon && std::abs(settings.marginY) <= epsilon) {
            for (size_t i = 0; i < Presets::OSP::NOSPs; ++i) {
                const auto [x, y] = Presets::OSP::presets.for_level(i);
                if (std::abs(settings.xPercent - x) <= epsilon &&
                    std::abs(settings.yPercent - y) <= epsilon) {
                    MCP::Settings::current_OSP = i;
                    return;
                }
            }
        }

        MCP::Settings::current_OSP = Presets::OSP::NOSPs;
    }

    void ResetThemeAppearance(Theme::Theme& settings) {
        const Theme::Theme defaults;

#ifndef NDEBUG
        settings.n_max_buttons = defaults.n_max_buttons;
#endif
        settings.font_name = defaults.font_name;
        settings.font_shadow = defaults.font_shadow;
        settings.special_effects = defaults.special_effects;
        settings.fadeSpeed = defaults.fadeSpeed;
        settings.xPercent = defaults.xPercent;
        settings.yPercent = defaults.yPercent;
        settings.marginX = defaults.marginX;
        settings.marginY = defaults.marginY;
        settings.prompt_size = defaults.prompt_size;
        settings.icon2font_ratio = defaults.icon2font_ratio;
        settings.prompt_order = defaults.prompt_order;
        settings.prompt_alignment = defaults.prompt_alignment;
        settings.prompt_pivot = defaults.prompt_pivot;
        settings.linespacing = defaults.linespacing;
        settings.progress_speed = defaults.progress_speed;

        MCP::Settings::shouldReloadPromptSize.store(true);
    }

    bool ThemeEditor::RenderLayout() {
        if (!BeginSettingsGroup("$SkyPromptMCPThemeLayout", "theme.layout")) return false;
        auto& theme = GetTheme();
        bool changed = false;
#ifndef NDEBUG
        constexpr int minimumPromptCount = 1;
        const auto label = SettingRow(Translations::Get("$SkyPromptMCPControlsMaxButtons"), "theme.maxButtons",
                                      "$SkyPromptMCPControlsMaxButtonsHelp");
        if (ImGuiMCP::InputInt(label.c_str(), &theme.n_max_buttons)) {
            theme.n_max_buttons = std::max(theme.n_max_buttons, minimumPromptCount);
            changed = true;
        }
#endif
        const auto alignmentBefore = theme.prompt_alignment;
        if (SettingCombo("$SkyPromptMCPSettingsPromptAlignment", "settings.promptAlignment",
                         PromptAlignmentLabel(theme.prompt_alignment).c_str())) {
            for (const auto alignment : {Theme::kVertical, Theme::kHorizontal, Theme::kRadial, Theme::kDiamond, Theme::kList}) {
                const bool selected = theme.prompt_alignment == alignment;
                const auto id = std::format("settings.promptAlignment.{}", static_cast<int>(alignment));
                if (LocalizedSelectableText(PromptAlignmentLabel(alignment), id, selected)) {
                    theme.prompt_alignment = alignment;
                }
                if (selected) ImGuiMCP::SetItemDefaultFocus();
            }
            ImGuiMCP::EndCombo();
        }
        changed |= alignmentBefore != theme.prompt_alignment;

        const auto orderBefore = theme.prompt_order;
        if (SettingCombo("$SkyPromptMCPSettingsPromptOrder", "settings.promptOrder",
                         PromptOrderLabel(theme.prompt_order).c_str())) {
            for (const auto order : {Theme::kIconFirst, Theme::kTextFirst}) {
                const bool selected = theme.prompt_order == order;
                const auto id = order == Theme::kIconFirst ? "settings.promptOrder.iconFirst" : "settings.promptOrder.textFirst";
                if (LocalizedSelectableText(PromptOrderLabel(order), id, selected)) theme.prompt_order = order;
                if (selected) ImGuiMCP::SetItemDefaultFocus();
            }
            ImGuiMCP::EndCombo();
        }
        changed |= orderBefore != theme.prompt_order;
        changed |= SettingFloat("$SkyPromptMCPSettingsLineSpacing", "settings.lineSpacing", &theme.linespacing,
                                0.0f, 1.0f, "$SkyPromptMCPSettingsLineSpacingHelp");
        ImGuiMCP::EndTable();
        return changed;
    }

    bool ThemeEditor::RenderPosition() {
        if (!BeginSettingsGroup("$SkyPromptMCPThemePosition", "theme.position")) return false;
        auto& theme = GetTheme();
        SyncOSPPresetSelection(theme);
        bool changed = MCP::Settings::OSPPresetBox(theme);
        changed |= SettingFloat("$SkyPromptMCPSettingsXPercent", "settings.xPercent", &theme.xPercent,
                                0.0f, 1.0f, "$SkyPromptMCPSettingsXPercentHelp");
        changed |= SettingFloat("$SkyPromptMCPSettingsYPercent", "settings.yPercent", &theme.yPercent,
                                0.0f, 1.0f, "$SkyPromptMCPSettingsYPercentHelp");
        changed |= SettingFloat("$SkyPromptMCPSettingsMarginX", "settings.marginX", &theme.marginX,
                                -1000.0f, 1000.0f, "$SkyPromptMCPSettingsMarginXHelp");
        changed |= SettingFloat("$SkyPromptMCPSettingsMarginY", "settings.marginY", &theme.marginY,
                                -1000.0f, 1000.0f, "$SkyPromptMCPSettingsMarginYHelp");

        const auto pivotBefore = theme.prompt_pivot;
        if (SettingCombo("$SkyPromptMCPSettingsPromptPivot", "settings.promptPivot",
                         PromptPivotLabel(theme.prompt_pivot).c_str(), "$SkyPromptMCPSettingsPromptPivotHelp")) {
            for (const auto pivot : {Theme::kTopLeft, Theme::kTopRight, Theme::kBottomLeft, Theme::kBottomRight, Theme::kCenter}) {
                const bool selected = theme.prompt_pivot == pivot;
                const auto id = std::format("settings.promptPivot.{}", static_cast<int>(pivot));
                if (LocalizedSelectableText(PromptPivotLabel(pivot), id, selected)) theme.prompt_pivot = pivot;
                if (selected) ImGuiMCP::SetItemDefaultFocus();
            }
            ImGuiMCP::EndCombo();
        }
        changed |= pivotBefore != theme.prompt_pivot;
        ImGuiMCP::EndTable();
        return changed;
    }

    bool ThemeEditor::RenderAppearance() {
        if (!BeginSettingsGroup("$SkyPromptMCPThemeAppearance", "theme.appearance")) return false;
        auto& theme = GetTheme();
        bool changed = SettingFloat("$SkyPromptMCPSettingsPromptSize", "settings.promptSize", &theme.prompt_size,
                                    15.0f, 100.0f, "$SkyPromptMCPSettingsPromptSizeHelp");
        changed |= SettingFloat("$SkyPromptMCPSettingsIcon2FontRatio", "settings.icon2FontRatio", &theme.icon2font_ratio,
                                0.5f, 2.0f, "$SkyPromptMCPSettingsIcon2FontRatioHelp");
        if (changed) MCP::Settings::shouldReloadPromptSize.store(true);
        changed |= MCP::Settings::FontSettings(theme);
        ImGuiMCP::EndTable();
        return changed;
    }

    bool ThemeEditor::RenderAnimation() {
        if (!BeginSettingsGroup("$SkyPromptMCPThemeAnimation", "theme.animation")) return false;
        auto& theme = GetTheme();
        bool changed = SettingFloat("$SkyPromptMCPSettingsFadeSpeed", "settings.fadeSpeed", &theme.fadeSpeed,
                                    0.01f, 0.1f, "$SkyPromptMCPSettingsFadeSpeedHelp");
        changed |= SettingFloat("$SkyPromptMCPSettingsProgressSpeed", "settings.progressSpeed", &theme.progress_speed,
                                0.0f, 1.0f, "$SkyPromptMCPSettingsProgressSpeedHelp");
        ImGuiMCP::EndTable();
        return changed;
    }

    std::string SpecialEffectLabel(const uint32_t a_id) {
        if (const auto* definition = ImGui::PromptEffects::GetDefinition(a_id)) {
            return Translations::Get(definition->label);
        }
        using namespace SkyPrompt::AddOns::SpecialEffects;
        switch (a_id) {
            case kNone: return Translations::Get("$SkyPromptMCPThemeEffectNone");
            case kVinyArcs: return Translations::Get("$SkyPromptMCPThemeEffectVinyArcs");
            case kTextBackground: return Translations::Get("$SkyPromptMCPThemeEffectTextBackground");
            default: return Translations::Format("$SkyPromptMCPThemeEffectUnknown", a_id);
        }
    }

    bool RenderSpecialEffectSettings(Theme::SpecialEffect& a_effect, const std::string_view a_id) {
        const auto* definition = ImGui::PromptEffects::GetDefinition(a_effect.id);
        const bool background = a_effect.id == SkyPrompt::AddOns::SpecialEffects::kTextBackground;
        constexpr std::array backgroundFloatLabels = {
            "$SkyPromptMCPThemeBackgroundPaddingX", "$SkyPromptMCPThemeBackgroundPaddingY",
            "$SkyPromptMCPThemeBackgroundRadius"
        };
        constexpr std::array backgroundFloatHelp = {
            "$SkyPromptMCPThemeBackgroundPaddingHelp", "$SkyPromptMCPThemeBackgroundPaddingHelp",
            "$SkyPromptMCPThemeBackgroundRadiusHelp"
        };
        bool changed = false;
        constexpr float floatLimit = 500.0f;
        const auto floatCount = std::max(a_effect.floats.size(), definition ? definition->floats.size()
                                                                         : background ? backgroundFloatLabels.size() : 0);
        for (size_t i = 0; i < floatCount; ++i) {
            const auto* parameter = definition && i < definition->floats.size() ? &definition->floats[i] : nullptr;
            const bool named = background && i < backgroundFloatLabels.size();
            const auto label = SettingRow(parameter ? Translations::Get(parameter->label)
                                                   : named ? Translations::Get(backgroundFloatLabels[i])
                                                : Translations::Format("$SkyPromptMCPThemeSpecialFloat", i + 1),
                                          std::format("{}.float.{}", a_id, i),
                                          parameter ? parameter->help
                                                    : named ? backgroundFloatHelp[i] : "$SkyPromptMCPThemeSpecialFloatHelp");
            const float before = i < a_effect.floats.size() ? a_effect.floats[i]
                                                          : parameter ? parameter->default_value : 0.0f;
            float value = before;
            changed |= SliderFloatCommitted(label.c_str(), &value, parameter ? parameter->min : -floatLimit,
                                                                  parameter ? parameter->max : floatLimit);
            if (value != before) {
                while (i >= a_effect.floats.size()) {
                    const auto index = a_effect.floats.size();
                    a_effect.floats.push_back(definition && index < definition->floats.size()
                                                 ? definition->floats[index].default_value : 0.0f);
                }
                a_effect.floats[i] = value;
            }
        }
        constexpr uint32_t integerStep = 1;
        const auto integerCount = std::max(a_effect.integers.size(), definition ? definition->colors.size()
                                                                              : background ? size_t{1} : size_t{0});
        for (size_t i = 0; i < integerCount; ++i) {
            const auto* parameter = definition && i < definition->colors.size() ? &definition->colors[i] : nullptr;
            const bool named = background && i == 0;
            auto value = i < a_effect.integers.size() ? a_effect.integers[i]
                                                    : parameter ? parameter->default_value : IM_COL32(0, 0, 0, 128);
            const auto label = SettingRow(parameter ? Translations::Get(parameter->label)
                                                   : named ? Translations::Get("$SkyPromptMCPThemeBackgroundColor")
                                                : Translations::Format("$SkyPromptMCPThemeSpecialInteger", i + 1),
                                          std::format("{}.integer.{}", a_id, i),
                                          parameter ? parameter->help : named ? "$SkyPromptMCPThemeBackgroundColorHelp"
                                                : "$SkyPromptMCPThemeSpecialIntegerHelp");
            const auto& style = *ImGuiMCP::GetStyle();
            const float width = ImGuiMCP::CalcItemWidth();
            const float swatchWidth = ImGuiMCP::GetFrameHeight();
            constexpr float stepButtonCount = 2.0f;
            const float scalarMinWidth = swatchWidth + stepButtonCount * (swatchWidth + style.ItemInnerSpacing.x);
            const bool inlineColor = width >= scalarMinWidth + swatchWidth + style.ItemSpacing.x;
            ImGuiMCP::SetNextItemWidth(inlineColor ? width - swatchWidth - style.ItemSpacing.x : width);
            bool edited = ImGuiMCP::InputScalar(label.c_str(), ImGuiMCP::ImGuiDataType_U32, &value, &integerStep);
            if (inlineColor) ImGuiMCP::SameLine();
            auto color = ImGuiMCP::ColorConvertU32ToFloat4(value);
            const auto colorID = std::format("##{}.color.{}", a_id, i);
            if (ImGuiMCP::ColorEdit4(colorID.c_str(), &color.x,
                                    ImGuiMCP::ImGuiColorEditFlags_NoInputs | ImGuiMCP::ImGuiColorEditFlags_AlphaBar)) {
                value = ImGuiMCP::ColorConvertFloat4ToU32(color);
                edited = true;
            }
            if (edited) {
                while (i >= a_effect.integers.size()) {
                    const auto index = a_effect.integers.size();
                    a_effect.integers.push_back(definition && index < definition->colors.size()
                                                   ? definition->colors[index].default_value : 0);
                }
                a_effect.integers[i] = value;
                changed = true;
            }
        }
        if (definition) {
            for (size_t i = 0; i < definition->bools.size(); ++i) {
                const auto& parameter = definition->bools[i];
                const auto label = SettingRow(Translations::Get(parameter.label), std::format("{}.bool.{}", a_id, i),
                                              parameter.help);
                bool value = i < a_effect.bools.size() ? a_effect.bools[i] != 0 : parameter.default_value;
                if (ImGuiMCP::Checkbox(label.c_str(), &value)) {
                    while (i >= a_effect.bools.size()) {
                        a_effect.bools.push_back(definition->bools[a_effect.bools.size()].default_value);
                    }
                    a_effect.bools[i] = value;
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool ThemeEditor::RenderSpecialEffects() {
        if (!BeginSettingsGroup("$SkyPromptMCPThemeSpecialEffects", "theme.special")) return false;
        auto& effects = GetTheme().special_effects;
        using namespace SkyPrompt::AddOns::SpecialEffects;
        constexpr std::array<uint32_t, 5> supported = {
            kVinyArcs, kTextBackground, ImGui::PromptEffects::kProgressCircle,
            ImGui::PromptEffects::kListIndicators, ImGui::PromptEffects::kActivationPop
        };
        const auto present = [&](const auto id) {
            return std::ranges::find(effects, id, &Theme::SpecialEffect::id) != effects.end();
        };
        bool changed = false;
        ImGuiMCP::BeginDisabled(std::ranges::all_of(supported, present));
        if (SettingCombo("$SkyPromptMCPThemeAddEffect", "theme.effect.add",
                         Translations::Get("$SkyPromptMCPThemeChooseEffect").c_str())) {
            for (const auto id : supported) {
                if (present(id)) continue;
                if (LocalizedSelectableText(SpecialEffectLabel(id), std::format("theme.effect.add.{}", id), false)) {
                    auto& effect = effects.emplace_back();
                    effect.id = id;
                    if (id == kVinyArcs) {
                        effect.integers = {IM_COL32(255, 204, 0, 255), IM_COL32(200, 160, 0, 255),
                                           IM_COL32(200, 160, 0, 255)};
                        effect.floats = {0.0f, 0.0f};
                    }
                    changed = true;
                }
            }
            ImGuiMCP::EndCombo();
        }
        ImGuiMCP::EndDisabled();
        ImGuiMCP::EndTable();
        ImGuiMCP::Indent(ImGuiMCP::GetFontSize());
        std::optional<size_t> removed;
        for (size_t i = 0; i < effects.size(); ++i) {
            const auto id = std::format("theme.effect.{}", i);
            const auto label = Translations::WithID(SpecialEffectLabel(effects[i].id), id);
            bool visible = true;
            if (ImGuiMCP::CollapsingHeader(label.c_str(), &visible, ImGuiMCP::ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGuiMCP::Indent(ImGuiMCP::GetFontSize());
                if (BeginSettingsTable(std::format("{}.fields", id).c_str())) {
                    changed |= RenderSpecialEffectSettings(effects[i], id);
                    ImGuiMCP::EndTable();
                }
                ImGuiMCP::Unindent(ImGuiMCP::GetFontSize());
            }
            if (!visible) removed = i;
        }
        ImGuiMCP::Unindent(ImGuiMCP::GetFontSize());
        if (removed) {
            effects.erase(effects.begin() + *removed);
            changed = true;
        }
        return changed;
    }

    using SectionLabels = std::array<std::string_view, 4>;

    bool AreSectionLabelsSafe(const SectionLabels& a_labels) {
        for (std::size_t i = 0; i < a_labels.size(); ++i) {
            const auto label = a_labels[i];
            if (label.empty() || label.contains('/') || label.contains("##")) {
                return false;
            }

            for (std::size_t j = 0; j < i; ++j) {
                if (label == a_labels[j]) {
                    return false;
                }
            }
        }
        return true;
    }

    SectionLabels GetSectionLabels() {
        constexpr std::array keys = {
            "$SkyPromptMCPSectionSettings",
            "$SkyPromptMCPSectionControls",
            "$SkyPromptMCPSectionTheme",
            "$SkyPromptMCPSectionLog",
        };

        SectionLabels labels;
        std::ranges::transform(keys, labels.begin(),
                               [](const std::string_view key) { return std::string_view(Translations::Get(key)); });
        if (!AreSectionLabelsSafe(labels)) {
            logger::warn("Invalid or duplicate translated menu section label; using English section labels");
            std::ranges::transform(keys, labels.begin(), [](const std::string_view key) {
                return std::string_view(Translations::GetEnglish(key));
            });
        }
        return labels;
    }

    constexpr std::size_t max_export_name_length = 240;

    std::array<char, max_export_name_length + 1> export_name{};
    std::string export_status;
    std::string export_error;

    void SetExportName(const std::string_view a_name) {
        export_name.fill('\0');
        std::ranges::copy_n(a_name.begin(), std::min(a_name.size(), export_name.size() - 1), export_name.begin());
    }

    bool IsReservedWindowsName(const std::string_view a_name) {
        auto base = std::string(a_name.substr(0, a_name.find('.')));
        std::ranges::transform(base, base.begin(), [](const unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });

        constexpr std::array reserved = {"CON"sv, "PRN"sv, "AUX"sv, "NUL"sv};
        if (std::ranges::find(reserved, base) != reserved.end()) {
            return true;
        }
        return base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT")) && base.back() >= '1' &&
               base.back() <= '9';
    }

    const std::string* ExportNameError(const std::string_view a_name) {
        if (a_name.empty() ||
            std::ranges::all_of(a_name, [](const unsigned char character) { return character == ' '; })) {
            return std::addressof(Translations::Get("$SkyPromptMCPThemeExportEmpty"));
        }

        const auto path = std::filesystem::path(Theme::themes_folder) / std::format("{}.json", a_name);
        constexpr std::string_view invalid_characters = R"(<>:"/\|?*)";
        if (a_name.size() > max_export_name_length || path.native().size() >= MAX_PATH || a_name == "." ||
            a_name == ".." || a_name.back() == '.' || a_name.back() == ' ' ||
            std::ranges::any_of(a_name, [&](const unsigned char character) {
                return character < 32 || invalid_characters.contains(static_cast<char>(character));
            }) ||
            IsReservedWindowsName(a_name)) {
            return std::addressof(Translations::Get("$SkyPromptMCPThemeExportInvalid"));
        }
        return nullptr;
    }

    std::filesystem::path ExportTheme(const Theme::Theme& a_theme, const std::string_view a_name) {
        using namespace rapidjson;

        if (ExportNameError(a_name)) {
            return {};
        }

        const auto local_time = std::chrono::floor<std::chrono::seconds>(
            std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
        const auto plugin_version = SKSE::PluginDeclaration::GetSingleton()->GetVersion();
        const auto description =
            std::format("{}. SkyPrompt {}.{}.{}. {:%Y-%m-%d %H:%M:%S}.",
                        Translations::Get("$SkyPromptThemeExportDescription"), plugin_version.major(),
                        plugin_version.minor(), plugin_version.patch(), local_time);

        Document document;
        document.SetObject();
        auto& allocator = document.GetAllocator();
        const auto add_string = [&](const char* a_key, const std::string_view a_value) {
            Value key;
            key.SetString(a_key, allocator);
            Value value;
            value.SetString(a_value.data(), static_cast<SizeType>(a_value.size()), allocator);
            document.AddMember(key, value, allocator);
        };

        auto* player = RE::PlayerCharacter::GetSingleton();
        const auto* player_name = player ? player->GetDisplayFullName() : nullptr;
        add_string("name", a_name);
        add_string("description", description);
        add_string("author", player_name ? player_name : "");
        add_string("version", "1.0.0");
        a_theme.UpdateSettings(document);
        document.AddMember("hide_in_menu", a_theme.hide_in_menu, allocator);

        const auto folder = std::filesystem::path(Theme::themes_folder);
        std::error_code error;
        std::filesystem::create_directories(folder, error);
        if (error) {
            logger::error("Failed to create theme export folder: {}", error.message());
            return {};
        }

        const auto path = folder / std::format("{}.json", a_name);
        if (!Theme::WriteThemeFile(path, document)) return {};

        logger::info("Exported theme: {}", path.string());
        return path;
    }

    void RenderThemeExport(const Theme::Theme& a_theme) {
        const auto popup = Translations::ImGuiLabel("$SkyPromptMCPThemeExport", "theme.export.popup");
        if (LocalizedButton("$SkyPromptMCPThemeExport", "theme.export.open")) {
            const auto local_time = std::chrono::floor<std::chrono::seconds>(
                std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
            SetExportName(std::format("Theme_{:%Y-%m-%d_%H-%M-%S}", local_time));
            export_status.clear();
            export_error.clear();
            ImGuiMCP::OpenPopup(popup.c_str());
        }
        ImGuiMCP::SameLine();
        HelpMarker("$SkyPromptMCPThemeExportHelp");
        if (!export_status.empty()) {
            ImGuiMCP::TextColored({0.35f, 1.0f, 0.35f, 1.0f}, "%s", export_status.c_str());
        }

        if (!ImGuiMCP::BeginPopupModal(popup.c_str(), nullptr, ImGuiMCP::ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }

        ImGuiMCP::SetNextItemWidth(300.0f);
        if (ImGuiMCP::IsWindowAppearing()) {
            ImGuiMCP::SetKeyboardFocusHere();
        }
        if (ImGuiMCP::InputText("##theme.export.filename", export_name.data(), export_name.size(),
                                ImGuiMCP::ImGuiInputTextFlags_AutoSelectAll)) {
            export_error.clear();
        }
        ImGuiMCP::SameLine();
        ImGuiMCP::TextUnformatted(".json");

        const auto name = std::string_view(export_name.data());
        const auto* validation_error = ExportNameError(name);
        if (validation_error) {
            ImGuiMCP::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "%s", validation_error->c_str());
        }
        if (!export_error.empty()) {
            ImGuiMCP::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "%s", export_error.c_str());
        }

        ImGuiMCP::BeginDisabled(validation_error != nullptr);
        if (LocalizedButton("$SkyPromptMCPThemeExportConfirm", "theme.export.confirm")) {
            if (const auto path = ExportTheme(a_theme, name); !path.empty()) {
                export_status = Translations::Format("$SkyPromptMCPThemeExportSuccess", path.string());
                export_error.clear();
                ImGuiMCP::CloseCurrentPopup();
            } else {
                export_error = Translations::Get("$SkyPromptMCPThemeExportFailed");
            }
        }
        ImGuiMCP::EndDisabled();
        ImGuiMCP::SameLine();
        if (LocalizedButton("$SkyPromptMCPThemeExportCancel", "theme.export.cancel")) {
            ImGuiMCP::CloseCurrentPopup();
        }
        ImGuiMCP::EndPopup();
    }
}

void __stdcall MCP::RenderSettings() {
    PushMenuSpacing();
    bool enabled = Settings::initialized.load();
    if (LocalizedCheckbox("$SkyPromptMCPSettingsEnableMod", "settings.enableMod", &enabled)) {
        Settings::initialized.store(enabled);
    }
    ImGuiMCP::SameLine();
    if (LocalizedButton("$SkyPromptMCPSettingsStartTutorial", "settings.startTutorial")) {
        Tutorial::Manager::Start();
    }
#ifndef NDEBUG
    LocalizedCheckbox("$SkyPromptMCPSettingsDrawDebug", "settings.drawDebug", &Settings::draw_debug);
#endif
    ImGuiMCP::Spacing();
    if (BeginSettingsTable("settings.general")) {
        std::unique_lock lock(Theme::m_theme_);
        if (SettingFloat("$SkyPromptMCPSettingsLifetime", "settings.lifetime", &Settings::lifetime,
                         1.0f, 30.0f, "$SkyPromptMCPSettingsLifetimeHelp")) {
            Settings::shouldReloadLifetime.store(true);
            Settings::to_json();
        }
        ImGuiMCP::EndTable();
    }
    ImGuiMCP::PopStyleVar(menuSpacingStyleCount);
}

void __stdcall MCP::RenderLog() {
#ifndef NDEBUG
    LocalizedCheckbox("$SkyPromptMCPLogTrace", "log.trace", &LogSettings::log_trace);
#endif
    ImGuiMCP::SameLine();
    LocalizedCheckbox("$SkyPromptMCPLogInfo", "log.info", &LogSettings::log_info);
    ImGuiMCP::SameLine();
    LocalizedCheckbox("$SkyPromptMCPLogWarning", "log.warning", &LogSettings::log_warning);
    ImGuiMCP::SameLine();
    LocalizedCheckbox("$SkyPromptMCPLogError", "log.error", &LogSettings::log_error);

    // if "Generate Log" button is pressed, read the log file
    if (LocalizedButton("$SkyPromptMCPLogGenerate", "log.generate")) logLines = ReadLogFile();

    // Display each line in a new ImGui::Text() element
    for (const auto& line : logLines) {
        if (!LogSettings::log_trace && line.find("trace") != std::string::npos) continue;
        if (!LogSettings::log_info && line.find("info") != std::string::npos) continue;
        if (!LogSettings::log_warning && line.find("warning") != std::string::npos) continue;
        if (!LogSettings::log_error && line.find("error") != std::string::npos) continue;
        ImGuiMCP::TextUnformatted(line.c_str());
    }
}

void MCP::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        return;
    }

    is_installed = true;

    log_path = GetLogPath().string();

    const auto section_labels = GetSectionLabels();
    SKSEMenuFramework::SetSection(mod_name);
    SKSEMenuFramework::AddSectionItem(std::string(section_labels[0]), RenderSettings);
    SKSEMenuFramework::AddSectionItem(std::string(section_labels[1]), RenderControls);
    SKSEMenuFramework::AddSectionItem(std::string(section_labels[2]), RenderTheme);
    SKSEMenuFramework::AddSectionItem(std::string(section_labels[3]), RenderLog);
}

bool MCP::Settings::IsEnabled(const Input::DEVICE a_device) {
    const auto it = enabled_devices.find(a_device);
    return it != enabled_devices.end() && it->second;
}

bool MCP::Settings::OSPPresetBox(Theme::Theme& a_theme) {
    bool changed = false;

    const std::string_view current_preset_name =
        current_OSP < Presets::OSP::NOSPs ? Presets::OSP::OSPPool.to_name(current_OSP) : "Custom";
    const auto& current_position_label = PositionLabel(current_preset_name);
    if (SettingCombo("$SkyPromptMCPSettingsOnScreenPosition", "settings.onScreenPosition",
                            current_position_label.c_str())) {
        for (const auto& all_preset_names = Presets::OSP::OSPnames;
             const auto& preset_name : all_preset_names) {
            const bool isSelected = current_preset_name == preset_name;
            const auto id = std::format("settings.position.{}", preset_name);
            if (LocalizedSelectableText(PositionLabel(preset_name), id, isSelected)) {
                current_OSP = std::distance(all_preset_names.begin(),
                                            std::ranges::find(all_preset_names, preset_name));
                const auto [fst, snd] = Presets::OSP::presets.for_level(current_OSP);
                a_theme.xPercent = fst;
                a_theme.yPercent = snd;
                a_theme.marginX = 0.f;
                a_theme.marginY = 0.f;
                changed = true;
            }
            if (isSelected) ImGuiMCP::SetItemDefaultFocus();
        }
        ImGuiMCP::EndCombo();
    }

    return changed;
}

bool MCP::Settings::FontSettings(Theme::Theme& a_theme) {
    auto changed = false;
    const auto* iconFontManager = MANAGER(IconFont);
    const auto& fontInfos = iconFontManager->GetAvailableFonts();

    auto& font_name = a_theme.font_name;
    if (const auto selectedInfo = iconFontManager->GetFontInfoByName(font_name);
        selectedInfo && font_name != selectedInfo->GetName()) {
        font_name = std::string(selectedInfo->GetName());
        changed = true;
    }

    if (SettingCombo("$SkyPromptMCPThemeFont", "theme.font", font_name.c_str())) {
        for (const auto& fontInfo : fontInfos) {
            const bool isSelected = font_name == fontInfo.GetName();
            const auto font_label =
                Translations::WithID(fontInfo.GetName(), std::format("theme.font.{}", fontInfo.GetName()));
            if (ImGuiMCP::Selectable(font_label.c_str(), isSelected)) {
                if (!isSelected) {
                    a_theme.font_name = std::string(fontInfo.GetName());
                    changed = true;
                }
            }
            if (isSelected) ImGuiMCP::SetItemDefaultFocus();
        }
        ImGuiMCP::EndCombo();
    }

    if (SettingFloat("$SkyPromptMCPThemeFontShadow", "theme.fontShadow", &a_theme.font_shadow, 0.f,
                     1.f, "$SkyPromptMCPThemeFontShadowHelp")) {
        changed = true;
    }

    if (changed) {
        refreshStyle.store(true);
    }

    return changed;
}

void MCP::Settings::LoadDefaultPromptKeys() {
    using namespace SKSE::InputMap;

    default_keys = {{Input::DEVICE::kKeyboardMouse,
                     {Input::Manager::Convert(KEY::kNum1, RE::INPUT_DEVICE::kKeyboard),
                      Input::Manager::Convert(KEY::kNum2, RE::INPUT_DEVICE::kKeyboard),
                      Input::Manager::Convert(KEY::kNum3, RE::INPUT_DEVICE::kKeyboard),
                      Input::Manager::Convert(KEY::kNum4, RE::INPUT_DEVICE::kKeyboard)}},
                    {Input::DEVICE::kGamepad,
                     {kGamepadButtonOffset_B, kGamepadButtonOffset_X,
                      kGamepadButtonOffset_Y, kGamepadButtonOffset_A}}};
    default_keys[Input::DEVICE::kVR] = default_keys.at(Input::DEVICE::kGamepad);
    cycle_L = {
        {Input::DEVICE::kKeyboardMouse, Input::Manager::Convert(KEY::kLeft, RE::INPUT_DEVICE::kKeyboard)},
        {Input::DEVICE::kGamepad, kGamepadButtonOffset_DPAD_LEFT},
        {Input::DEVICE::kVR, kGamepadButtonOffset_DPAD_LEFT}};
    cycle_R = {
        {Input::DEVICE::kKeyboardMouse, Input::Manager::Convert(KEY::kRight, RE::INPUT_DEVICE::kKeyboard)},
        {Input::DEVICE::kGamepad, kGamepadButtonOffset_DPAD_RIGHT},
        {Input::DEVICE::kVR, kGamepadButtonOffset_DPAD_RIGHT}};
}

namespace {
    std::string ControlKeyName(const Input::DEVICE a_device, const uint32_t a_key) {
        if (a_device == Input::kVR) {
            using namespace SKSE::InputMap;
            switch (a_key) {
                case 0: return Translations::Get("$SkyPromptMCPControlsNone");
                case kGamepadButtonOffset_DPAD_UP: return Translations::Get("$SkyPromptMCPVRUp");
                case kGamepadButtonOffset_DPAD_DOWN: return Translations::Get("$SkyPromptMCPVRDown");
                case kGamepadButtonOffset_DPAD_LEFT: return Translations::Get("$SkyPromptMCPVRLeft");
                case kGamepadButtonOffset_DPAD_RIGHT: return Translations::Get("$SkyPromptMCPVRRight");
                case kGamepadButtonOffset_LEFT_THUMB: return Translations::Get("$SkyPromptMCPVRLeftStickClick");
                case kGamepadButtonOffset_RIGHT_THUMB: return Translations::Get("$SkyPromptMCPVRRightStickClick");
                case kGamepadButtonOffset_LEFT_SHOULDER: return Translations::Get("$SkyPromptMCPVRLeftGrip");
                case kGamepadButtonOffset_RIGHT_SHOULDER: return Translations::Get("$SkyPromptMCPVRRightGrip");
                case kGamepadButtonOffset_A: return "A";
                case kGamepadButtonOffset_B: return "B";
                case kGamepadButtonOffset_X: return "X";
                case kGamepadButtonOffset_Y: return "Y";
                case kGamepadButtonOffset_LT: return Translations::Get("$SkyPromptMCPVRLeftTrigger");
                case kGamepadButtonOffset_RT: return Translations::Get("$SkyPromptMCPVRRightTrigger");
                default: break;
            }
        }
        return SKSE::InputMap::GetKeyName(a_key);
    }

    void ControlBox(const char* label, const Input::DEVICE selected_device, uint32_t& selected_key,
                    std::vector<uint32_t> a_keys = {}) {
        if (ImGuiMCP::BeginCombo(label, ControlKeyName(selected_device, selected_key).c_str())) {
            if (a_keys.empty()) a_keys = Input::Manager::GetKeys(selected_device);
            for (const auto key_code : a_keys) {
                const auto key_name = ControlKeyName(selected_device, key_code);
                if (key_name.empty()) {
                    continue;
                }
                const bool isSelected = selected_key == key_code;
                const auto key_label = Translations::WithID(key_name, std::format("key.{}", key_code));
                if (ImGuiMCP::Selectable(key_label.c_str(), isSelected)) {
                    if (!isSelected) {
                        selected_key = key_code;
                    }
                }
                if (isSelected) ImGuiMCP::SetItemDefaultFocus();
            }
            ImGuiMCP::EndCombo();
        }
    }

    void DeviceBox(const std::string_view a_id) {
        size_t index = 0;
        while (!MCP::Settings::IsEnabled(MCP::current_device)) {
            auto it = MCP::Settings::default_keys.begin();
            std::advance(it, index);
            if (it == MCP::Settings::default_keys.end()) {
                MCP::current_device = Input::DEVICE::kUnknown;
                break;
            }
            MCP::current_device = it->first;
            ++index;
        }

        const auto combo_id = SettingRow(Translations::Get("$SkyPromptMCPControlsDeviceSelection"), a_id);
        const auto& preview = DeviceLabel(MCP::current_device);
        if (ImGuiMCP::BeginCombo(combo_id.c_str(), preview.c_str())) {
            for (const auto& device : MCP::Settings::default_keys | std::views::keys) {
                if (!MCP::Settings::IsEnabled(device)) {
                    continue;
                }
                const bool isSelected = MCP::current_device == device;
                const auto id = std::format("{}.{}", a_id, static_cast<int>(device));
                if (LocalizedSelectableText(DeviceLabel(device), id, isSelected)) {
                    if (!isSelected) {
                        MCP::current_device = device;
                    }
                }
                if (isSelected) ImGuiMCP::SetItemDefaultFocus();
            }
            ImGuiMCP::EndCombo();
        }
    }

    void RenderControl(uint32_t& a_key, const std::string_view a_label, const std::string_view a_id) {
        const auto label = SettingRow(a_label, a_id);
        ControlBox(label.c_str(), MCP::current_device, a_key);
    }

    bool RenderNavigationModifier() {
        const auto label = SettingRow(Translations::Get("$SkyPromptMCPControlsNavigationModifier"),
                                      "controls.vrNavigationModifier", "$SkyPromptMCPControlsNavigationModifierHelp");
        auto keys = Input::Manager::GetKeys(Input::kVR);
        std::erase_if(keys, [](const uint32_t a_key) {
            return a_key >= SKSE::InputMap::kGamepadButtonOffset_DPAD_UP &&
                   a_key <= SKSE::InputMap::kGamepadButtonOffset_DPAD_RIGHT;
        });
        keys.insert(keys.begin(), 0);
        auto& modifier = MCP::Settings::vr_navigation_modifier;
        const auto before = modifier;
        ControlBox(label.c_str(), Input::kVR, modifier, std::move(keys));
        return before != modifier;
    }
};

bool MCP::Settings::CycleControls() {
    bool settingsChanged = false;
    ImGuiMCP::Spacing();
    auto temp = cycle_controls.load();
    if (LocalizedCheckbox("$SkyPromptMCPControlsCycleControls", "controls.cycleControls", &temp)) {
        cycle_controls.store(temp);
        settingsChanged = true;
    }
    ImGuiMCP::SameLine();
    HelpMarker("$SkyPromptMCPControlsCycleControlsHelp");
    if (!cycle_controls) {
        return settingsChanged;
    }

    if (current_device != Input::DEVICE::kUnknown && BeginSettingsTable("controls.cycle")) {
        auto before = cycle_L.at(current_device);
        RenderControl(cycle_L.at(current_device), Translations::Get("$SkyPromptMCPControlsCycleLeft"), "controls.cycleLeft");
        if (before != cycle_L.at(current_device)) {
            settingsChanged = true;
        }

        before = cycle_R.at(current_device);
        RenderControl(cycle_R.at(current_device), Translations::Get("$SkyPromptMCPControlsCycleRight"), "controls.cycleRight");
        if (before != cycle_R.at(current_device)) {
            settingsChanged = true;
        }
        ImGuiMCP::EndTable();
    }

    return settingsChanged;
}

void MCP::Settings::ReloadThemes() {
    std::unique_lock lock(Theme::m_theme_);
    for (auto& [filename, a_theme] : Theme::themes_loaded) {
        a_theme.ReLoad(filename);
    }
}

void MCP::Settings::to_json() {
    using namespace rapidjson;

    Document doc;
    doc.SetObject();

    Document::AllocatorType& allocator = doc.GetAllocator();

    Value root(kObjectType);

    root.AddMember("fadeSpeed", Theme::default_theme.fadeSpeed, allocator);
    root.AddMember("xPercent", Theme::default_theme.xPercent, allocator);
    root.AddMember("yPercent", Theme::default_theme.yPercent, allocator);
    root.AddMember("marginX", Theme::default_theme.marginX, allocator);
    root.AddMember("marginY", Theme::default_theme.marginY, allocator);
    root.AddMember("prompt_size", Theme::default_theme.prompt_size, allocator);
    root.AddMember("icon2font_ratio", Theme::default_theme.icon2font_ratio, allocator);
    root.AddMember("linespacing", Theme::default_theme.linespacing, allocator);
    root.AddMember("progress_speed", Theme::default_theme.progress_speed, allocator);
    root.AddMember("prompt_order",
                   Value(Theme::toPromptOrderString(Theme::default_theme.prompt_order).data(), allocator), allocator);
    root.AddMember("prompt_alignment",
                   Value(Theme::toPromptAlignmentString(Theme::default_theme.prompt_alignment).data(), allocator),
                   allocator);
    root.AddMember("prompt_pivot",
                   Value(Theme::toPromptPivotString(Theme::default_theme.prompt_pivot).data(), allocator),
                   allocator);
    root.AddMember("lifetime", lifetime, allocator);

    // special commands
    Value special_commands(kObjectType);
    special_commands.AddMember("visualize", SpecialCommands::visualize, allocator);
    special_commands.AddMember("responsiveness", SpecialCommands::responsiveness, allocator);
    root.AddMember("special_commands", special_commands, allocator);

    // enabled devices
    Value enabled_devices_(kObjectType);
    for (const auto& [device, enabled] : enabled_devices) {
        const auto device_str = device_to_string(device);
        Value device_json(device_str.c_str(), allocator);
        enabled_devices_.AddMember(device_json, enabled, allocator);
    }
    root.AddMember("enabled_devices", enabled_devices_, allocator);

    // n_max_buttons
    root.AddMember("n_max_buttons", Theme::default_theme.n_max_buttons, allocator);

    // keys
    Value prompt_keys_json(kObjectType);
    for (const auto& [device, keys] : default_keys) {
        const auto device_str = device_to_string(device);
        // need array of keys for each device
        Value device_keys(kArrayType);
        for (const auto key : keys) {
            device_keys.PushBack(key, allocator);
        }
        Value device_json(device_str.c_str(), allocator);
        prompt_keys_json.AddMember(device_json, device_keys, allocator);
    }
    root.AddMember("keys", prompt_keys_json, allocator);
    root.AddMember("vr_navigation_modifier", vr_navigation_modifier, allocator);

    // cycle enabled (std::atomic cycle_controls)
    Value a_cycle_controls(kObjectType);
    a_cycle_controls.AddMember("cycle_controls", cycle_controls.load(), allocator);

    // cycle_L
    Value a_cycle_L(kObjectType);
    for (const auto& [device, key] : cycle_L) {
        const auto device_str = Input::device_to_string(device);
        Value device_json(device_str.c_str(), allocator);
        a_cycle_L.AddMember(device_json, key, allocator);
    }
    root.AddMember("cycle_L", a_cycle_L, allocator);
    // cycle_R
    Value a_cycle_R(kObjectType);
    for (const auto& [device, key] : cycle_R) {
        const auto device_str = Input::device_to_string(device);
        Value device_json(device_str.c_str(), allocator);
        a_cycle_R.AddMember(device_json, key, allocator);
    }
    root.AddMember("cycle_R", a_cycle_R, allocator);

    // theme
    Value theme(kObjectType);
    theme.AddMember("font_name", Value(Theme::default_theme.font_name.c_str(), allocator).Move(), allocator);
    theme.AddMember("font_shadow", Theme::default_theme.font_shadow, allocator);
    // theme:: file name for active icon, like font_name
    root.AddMember("Theme", theme, allocator);
    Theme::default_theme.UpdateSpecialEffects(root, allocator);

    // version

    Value version(kObjectType);
    auto plugin_version = SKSE::PluginDeclaration::GetSingleton()->GetVersion();
    version.AddMember("major", plugin_version.major(), allocator);
    version.AddMember("minor", plugin_version.minor(), allocator);
    version.AddMember("patch", plugin_version.patch(), allocator);
    version.AddMember("build", plugin_version.build(), allocator);

    root.AddMember("version", version, allocator);

    doc.AddMember("MCP", root, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    // save to mod folder
    if (!std::filesystem::exists(mod_folder)) {
        std::filesystem::create_directories(mod_folder);
    }

    std::ofstream file(mod_folder + "settings.json");
    file << buffer.GetString();
    file.close();
}

namespace {
    template <class T>
    void LoadDeviceSettings(const rapidjson::Value& a_settings, const char* a_name,
                            std::map<Input::DEVICE, T>& a_values) {
        const auto section = a_settings.FindMember(a_name);
        if (section == a_settings.MemberEnd() || !section->value.IsObject()) return;
        const auto& settings = section->value;
        for (auto& [device, value] : a_values) {
            auto member = settings.FindMember(Input::device_to_string(device).c_str());
            if (member == settings.MemberEnd()) {
                if (device == Input::kGamepad) {
                    const auto controlMap = RE::ControlMap::GetSingleton();
                    const bool orbis = controlMap && controlMap->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis;
                    member = settings.FindMember(orbis ? "Gamepad (PS4)" : "Gamepad (Xbox)");
                    if (member == settings.MemberEnd()) {
                        member = settings.FindMember(orbis ? "Gamepad (Xbox)" : "Gamepad (PS4)");
                    }
                } else if (device == Input::kVR && REL::Module::IsVR()) {
                    member = settings.FindMember("Gamepad (Xbox)");
                }
            }
            T loaded{};
            if (member != settings.MemberEnd() && Presets::Getters::JSON::Get(member->value, loaded)) {
                value = std::move(loaded);
            }
        }
    }
}

void MCP::Settings::from_json() {
    std::ifstream file(json_folder);
    std::string str((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());

    rapidjson::Document doc;
    doc.Parse(str.c_str());

    if (doc.HasParseError()) {
        logger::error("Failed to parse settings.json");
        return;
    }
    if (!doc.HasMember("MCP")) {
        logger::error("Failed to find MCP in settings.json");
        return;
    }
    auto& mcp = doc["MCP"];
    LoadDefaultPromptKeys();
    Theme::default_theme.LoadSpecialEffects(mcp);

    if (mcp.HasMember("fadeSpeed")) {
        Theme::default_theme.fadeSpeed = mcp["fadeSpeed"].GetFloat();
    }
    if (mcp.HasMember("xPercent")) {
        Theme::default_theme.xPercent = mcp["xPercent"].GetFloat();
    }
    if (mcp.HasMember("yPercent")) {
        Theme::default_theme.yPercent = mcp["yPercent"].GetFloat();
    }
    if (mcp.HasMember("marginX")) {
        Theme::default_theme.marginX = mcp["marginX"].GetFloat();
    }
    if (mcp.HasMember("marginY")) {
        Theme::default_theme.marginY = mcp["marginY"].GetFloat();
    }
    if (mcp.HasMember("prompt_size")) {
        Theme::default_theme.prompt_size = mcp["prompt_size"].GetFloat();
    }
    if (mcp.HasMember("icon2font_ratio")) {
        Theme::default_theme.icon2font_ratio = mcp["icon2font_ratio"].GetFloat();
    }
    if (mcp.HasMember("linespacing")) {
        Theme::default_theme.linespacing = mcp["linespacing"].GetFloat();
    }
    if (mcp.HasMember("progress_speed")) {
        Theme::default_theme.progress_speed = mcp["progress_speed"].GetFloat();
    }
    if (mcp.HasMember("prompt_order") && mcp["prompt_order"].IsString()) {
        Theme::default_theme.prompt_order = Theme::toPromptOrder(mcp["prompt_order"].GetString());
    }
    if (mcp.HasMember("prompt_alignment") && mcp["prompt_alignment"].IsString()) {
        Theme::default_theme.prompt_alignment = Theme::toPromptAlignment(mcp["prompt_alignment"].GetString());
    }
    if (mcp.HasMember("prompt_pivot") && mcp["prompt_pivot"].IsString()) {
        Theme::default_theme.prompt_pivot = Theme::toPromptPivot(mcp["prompt_pivot"].GetString());
    }
    if (mcp.HasMember("lifetime")) {
        lifetime = mcp["lifetime"].GetFloat();
    }

    LoadDeviceSettings(mcp, "enabled_devices", enabled_devices);

    // n_max_buttons
    if (mcp.HasMember("n_max_buttons")) {
        Theme::default_theme.n_max_buttons = mcp["n_max_buttons"].GetInt();
    }

    LoadDeviceSettings(mcp, "keys", default_keys);
    Presets::Getters::JSON::Get(mcp, "vr_navigation_modifier", vr_navigation_modifier);

    if (mcp.HasMember("cycle_controls")) {
        cycle_controls = mcp["cycle_controls"].GetBool();
    }

    LoadDeviceSettings(mcp, "cycle_L", cycle_L);
    LoadDeviceSettings(mcp, "cycle_R", cycle_R);

    // special commands
    if (mcp.HasMember("special_commands")) {
        auto& special_commands = mcp["special_commands"];
        if (special_commands.HasMember("visualize")) {
            SpecialCommands::visualize = special_commands["visualize"].GetBool();
        }
        if (special_commands.HasMember("responsiveness")) {
            SpecialCommands::responsiveness = special_commands["responsiveness"].GetFloat();
        }
    }

    if (mcp.HasMember("Theme")) {
        const rapidjson::Value& theme = mcp["Theme"];
        if (theme.HasMember("font_name")) Theme::default_theme.font_name = theme["font_name"].GetString();
        if (theme.HasMember("font_shadow")) Theme::default_theme.font_shadow = theme["font_shadow"].GetFloat();
    }

    refreshStyle.store(true);
}

void __stdcall MCP::RenderControls() {
    PushMenuSpacing();
    std::unique_lock lock(Theme::m_theme_);
    // Checkbox for each device
    bool settingsChanged = false;
    for (const auto& device : Settings::enabled_devices | std::views::keys) {
        const auto& label = DeviceLabel(device);
        if (device != Settings::enabled_devices.begin()->first) {
            ImGuiMCP::SameLine();
            const float width = ImGuiMCP::CalcTextSize(label.c_str()).x + ImGuiMCP::GetFrameHeight() +
                                ImGuiMCP::GetStyle()->ItemInnerSpacing.x;
            if (width > ImGuiMCP::GetContentRegionAvail().x) ImGuiMCP::NewLine();
        }
        const auto id = std::format("controls.enabledDevice.{}", static_cast<int>(device));
        if (LocalizedCheckboxText(label, id, &Settings::enabled_devices.at(device))) {
            settingsChanged = true;
        }
    }

    const auto prompt_keys_before = Settings::default_keys;

    ImGuiMCP::Spacing();
    if (BeginSettingsTable("controls.bindings")) {
        DeviceBox("controls.deviceSelection");

        if (current_device != Input::DEVICE::kUnknown) {
            auto& keys = Settings::default_keys.at(current_device);
            for (size_t i = 0; i < keys.size(); ++i) {
                RenderControl(keys[i], Translations::Format("$SkyPromptMCPControlsButton", i + 1),
                              std::format("controls.button.{}", i + 1));
            }
            if (current_device == Input::kVR && RenderNavigationModifier()) {
                settingsChanged = true;
            }
        }
        ImGuiMCP::EndTable();
    }

    if (Settings::CycleControls()) {
        settingsChanged = true;
    }

    if (settingsChanged || prompt_keys_before != Settings::default_keys) {
        Settings::to_json();
    }

    ImGuiMCP::Spacing();
    ImGuiMCP::Separator();
    ImGuiMCP::Spacing();
    Settings::SpecialCommands::Render();
    ImGuiMCP::PopStyleVar(menuSpacingStyleCount);
}

void __stdcall MCP::RenderTheme() {
    PushMenuSpacing();
    if (LocalizedButton("$SkyPromptMCPThemeReloadThemes", "theme.reloadThemes")) {
        Settings::ReloadThemes();
        refreshStyle.store(true);
    }

    std::unique_lock lock(Theme::m_theme_);
    ImGuiMCP::SameLine();
    RenderThemeExport(theme_editor.GetTheme());
    ImGuiMCP::Spacing();
    theme_editor.RenderSelector();
    auto& theme = theme_editor.GetTheme();
    bool changed = false;
    if (LocalizedButton("$SkyPromptMCPSettingsResetDefaults", "settings.resetDefaults")) {
        ResetThemeAppearance(theme);
        changed = true;
    }
    changed |= theme_editor.RenderLayout();
    changed |= theme_editor.RenderPosition();
    changed |= theme_editor.RenderAppearance();
    changed |= theme_editor.RenderAnimation();
    changed |= theme_editor.RenderSpecialEffects();
    if (changed) theme_editor.OnChanged();
    ImGuiMCP::PopStyleVar(menuSpacingStyleCount);
}

void MCP::Settings::SpecialCommands::Render() {
    // double press: delete current prompt
    // triple press: cycle through prompts
    // triple press and hold: delete all prompts
    // explain what special commands are

    LocalizedText("$SkyPromptMCPSpecialCommands");
    ImGuiMCP::SameLine();
    HelpMarker("$SkyPromptMCPSpecialCommandsHelp");

    if (LocalizedCheckbox("$SkyPromptMCPSpecialCommandsVisualize", "special.visualize", &visualize)) {
        to_json();
    }
    ImGuiMCP::SameLine();
    HelpMarker("$SkyPromptMCPSpecialCommandsVisualizeHelp");

    if (BeginSettingsTable("special.settings")) {
        if (SettingFloat("$SkyPromptMCPSpecialCommandsResponsiveness", "special.responsiveness", &responsiveness,
                         0.0f, 1.0f, "$SkyPromptMCPSpecialCommandsResponsivenessHelp")) {
            to_json();
            ImGui::Renderer::UpdateMaxIntervalBetweenPresses();
        }
        ImGuiMCP::EndTable();
    }
}

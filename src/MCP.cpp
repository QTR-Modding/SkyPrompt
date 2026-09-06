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
            case Input::DEVICE::kGamepadDirectX:
                return Translations::Get("$SkyPromptMCPDeviceGamepadXbox");
            case Input::DEVICE::kGamepadOrbis:
                return Translations::Get("$SkyPromptMCPDeviceGamepadPS4");
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

    bool LocalizedBeginCombo(const std::string_view a_key, const std::string_view a_id, const char* a_preview) {
        const auto label = Translations::ImGuiLabel(a_key, a_id);
        return ImGuiMCP::BeginCombo(label.c_str(), a_preview);
    }

    bool LocalizedSelectableText(const std::string_view a_text, const std::string_view a_id, const bool a_selected) {
        const auto label = Translations::WithID(a_text, a_id);
        return ImGuiMCP::Selectable(label.c_str(), a_selected);
    }

    bool SliderFloatCommitted(const std::string_view a_key, const std::string_view a_id, float* a_value,
                              const float a_min, const float a_max) {
        const auto label = Translations::ImGuiLabel(a_key, a_id);
        ImGuiMCP::SliderFloat(label.c_str(), a_value, a_min, a_max);
        return ImGuiMCP::IsItemDeactivatedAfterEdit();
    }

    bool SliderIntCommitted(const std::string_view a_key, const std::string_view a_id, int* a_value, const int a_min,
                            const int a_max) {
        const auto label = Translations::ImGuiLabel(a_key, a_id);
        ImGuiMCP::SliderInt(label.c_str(), a_value, a_min, a_max);
        return ImGuiMCP::IsItemDeactivatedAfterEdit();
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

        void Render();
    };

    ThemeEditor theme_editor;

    void ThemeEditor::Render() {
        const auto label = [](const std::string_view a_name, const Theme::Theme& a_theme) {
            return &a_theme == Theme::last_theme
                       ? Translations::Format("$SkyPromptMCPThemeActive", a_name)
                       : std::string(a_name);
        };
        const auto& default_name = Translations::Get("$SkyPromptMCPThemeDefault");
        const auto preview = label(filename.empty() ? default_name : filename, GetTheme());
        if (LocalizedBeginCombo("$SkyPromptMCPSectionTheme", "theme.editor", preview.c_str())) {
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
        if (!filename.empty()) {
            ImGuiMCP::SameLine();
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

    void ResetSettingsPageToDefaults(Theme::Theme& settings) {
        const Theme::Theme defaults;

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

        if (&settings == &Theme::default_theme) {
            MCP::Settings::lifetime = 5.0f;
            MCP::Settings::shouldReloadLifetime.store(true);
        }
        MCP::Settings::shouldReloadPromptSize.store(true);
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
        document.AddMember("special_effect", a_theme.special_effect, allocator);
        document.AddMember("hide_in_menu", a_theme.hide_in_menu, allocator);
        Value integers(kArrayType), floats(kArrayType), strings(kArrayType), bools(kArrayType);
        for (const auto value : a_theme.special_integers) integers.PushBack(value, allocator);
        for (const auto value : a_theme.special_floats) floats.PushBack(value, allocator);
        for (const auto& value : a_theme.special_strings) {
            strings.PushBack(Value(value.c_str(), allocator), allocator);
        }
        for (const auto value : a_theme.special_bools) bools.PushBack(value != 0, allocator);
        document.AddMember("special_integers", integers, allocator);
        document.AddMember("special_floats", floats, allocator);
        document.AddMember("special_strings", strings, allocator);
        document.AddMember("special_bools", bools, allocator);

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
    bool settingsChanged = false; // Flag to track changes

    // Checkbox for enable/disable mod
    bool enabled = Settings::initialized.load();
    if (LocalizedCheckbox("$SkyPromptMCPSettingsEnableMod", "settings.enableMod", &enabled)) {
        Settings::initialized.store(enabled);
    }
    ImGuiMCP::SameLine();
    if (LocalizedButton("$SkyPromptMCPSettingsStartTutorial", "settings.startTutorial")) {
        Tutorial::Manager::Start();
    }
    std::unique_lock lock(Theme::m_theme_);
    theme_editor.Render();
    auto& theme = theme_editor.GetTheme();
    if (LocalizedButton("$SkyPromptMCPSettingsResetDefaults", "settings.resetDefaults")) {
        ResetSettingsPageToDefaults(theme);
        settingsChanged = true;
    }
    #ifndef NDEBUG
    // Checkbox for debug mode
    ImGuiMCP::SameLine();
    LocalizedCheckbox("$SkyPromptMCPSettingsDrawDebug", "settings.drawDebug", &Settings::draw_debug);
    #endif

    SyncOSPPresetSelection(theme);
    if (Settings::OSPPresetBox(theme)) {
        settingsChanged = true;
    }

    // Slider for fade speed
    if (SliderFloatCommitted("$SkyPromptMCPSettingsFadeSpeed", "settings.fadeSpeed", &theme.fadeSpeed,
                             0.01f, 0.1f)) {
        settingsChanged = true;
    }

    // Slider for X Percent
    if (SliderFloatCommitted("$SkyPromptMCPSettingsXPercent", "settings.xPercent", &theme.xPercent, 0.0f,
                             1.0f)) {
        settingsChanged = true;
    }

    // Slider for Y Percent
    if (SliderFloatCommitted("$SkyPromptMCPSettingsYPercent", "settings.yPercent", &theme.yPercent, 0.0f,
                             1.0f)) {
        settingsChanged = true;
    }

    // Slider for Margin X
    if (SliderFloatCommitted("$SkyPromptMCPSettingsMarginX", "settings.marginX", &theme.marginX,
                             -1000.0f, 1000.0f)) {
        settingsChanged = true;
    }

    // Slider for Margin Y
    if (SliderFloatCommitted("$SkyPromptMCPSettingsMarginY", "settings.marginY", &theme.marginY,
                             -1000.0f, 1000.0f)) {
        settingsChanged = true;
    }

    // Slider for Prompt Size
    if (SliderFloatCommitted("$SkyPromptMCPSettingsPromptSize", "settings.promptSize",
                             &theme.prompt_size, 15.0f, 100.0f)) {
        Settings::shouldReloadPromptSize.store(true);
        settingsChanged = true;
    }

    // Slider for Icon2Font Ratio
    if (SliderFloatCommitted("$SkyPromptMCPSettingsIcon2FontRatio", "settings.icon2FontRatio",
                             &theme.icon2font_ratio, 0.5f, 2.0f)) {
        Settings::shouldReloadPromptSize.store(true);
        settingsChanged = true;
    }

    const auto prompt_order_before = theme.prompt_order;
    const auto& prompt_order_preview = PromptOrderLabel(theme.prompt_order);
    if (LocalizedBeginCombo("$SkyPromptMCPSettingsPromptOrder", "settings.promptOrder", prompt_order_preview.c_str())) {
        for (const auto prompt_order : {Theme::kIconFirst, Theme::kTextFirst}) {
            const bool selected = theme.prompt_order == prompt_order;
            const auto id =
                prompt_order == Theme::kIconFirst ? "settings.promptOrder.iconFirst" : "settings.promptOrder.textFirst";
            if (LocalizedSelectableText(PromptOrderLabel(prompt_order), id, selected)) {
                theme.prompt_order = prompt_order;
            }
            if (selected) {
                ImGuiMCP::SetItemDefaultFocus();
            }
        }
        ImGuiMCP::EndCombo();
    }
    if (prompt_order_before != theme.prompt_order) {
        settingsChanged = true;
    }

    const auto prompt_alignment_before = theme.prompt_alignment;
    const auto& prompt_alignment_preview = PromptAlignmentLabel(theme.prompt_alignment);
    if (LocalizedBeginCombo("$SkyPromptMCPSettingsPromptAlignment", "settings.promptAlignment",
                            prompt_alignment_preview.c_str())) {
        for (const auto prompt_alignment : {Theme::kVertical, Theme::kHorizontal, Theme::kRadial, Theme::kDiamond, Theme::kList}) {
            const bool selected = theme.prompt_alignment == prompt_alignment;
            const auto id = std::format("settings.promptAlignment.{}", static_cast<int>(prompt_alignment));
            if (LocalizedSelectableText(PromptAlignmentLabel(prompt_alignment), id, selected)) {
                theme.prompt_alignment = prompt_alignment;
            }
            if (selected) {
                ImGuiMCP::SetItemDefaultFocus();
            }
        }
        ImGuiMCP::EndCombo();
    }
    if (prompt_alignment_before != theme.prompt_alignment) {
        settingsChanged = true;
    }

    const auto prompt_pivot_before = theme.prompt_pivot;
    const auto& prompt_pivot_preview = PromptPivotLabel(theme.prompt_pivot);
    if (LocalizedBeginCombo("$SkyPromptMCPSettingsPromptPivot", "settings.promptPivot",
                            prompt_pivot_preview.c_str())) {
        for (const auto prompt_pivot : {
                 Theme::kTopLeft, Theme::kTopRight, Theme::kBottomLeft,
                 Theme::kBottomRight, Theme::kCenter
             }) {
            const bool selected = theme.prompt_pivot == prompt_pivot;
            const auto id = std::format("settings.promptPivot.{}", static_cast<int>(prompt_pivot));
            if (LocalizedSelectableText(PromptPivotLabel(prompt_pivot), id, selected)) {
                theme.prompt_pivot = prompt_pivot;
            }
            if (selected) {
                ImGuiMCP::SetItemDefaultFocus();
            }
        }
        ImGuiMCP::EndCombo();
    }
    if (prompt_pivot_before != theme.prompt_pivot) {
        settingsChanged = true;
    }

    // Slider for Line Spacing
    if (SliderFloatCommitted("$SkyPromptMCPSettingsLineSpacing", "settings.lineSpacing",
                             &theme.linespacing, 0.0f, 1.0f)) {
        settingsChanged = true;
    }

    // Slider for Progress Speed
    if (SliderFloatCommitted("$SkyPromptMCPSettingsProgressSpeed", "settings.progressSpeed",
                             &theme.progress_speed, 0.0f, 1.0f)) {
        settingsChanged = true;
    }

    ImGuiMCP::Separator();
    // Slider for Lifetime
    if (SliderFloatCommitted("$SkyPromptMCPSettingsLifetime", "settings.lifetime", &Settings::lifetime, 1.0f, 30.0f)) {
        Settings::shouldReloadLifetime.store(true);
        Settings::to_json();
    }

    if (settingsChanged) {
        theme_editor.OnChanged();
    }
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
    if (enabled_devices.contains(a_device)) {
        if (const auto gamepad_type = RE::ControlMap::GetSingleton()->GetGamePadType();
            gamepad_type == RE::PC_GAMEPAD_TYPE::kOrbis) {
            if (a_device == Input::DEVICE::kGamepadDirectX) {
                return false;
            }
        } else if (gamepad_type == RE::PC_GAMEPAD_TYPE::kDirectX) {
            if (a_device == Input::DEVICE::kGamepadOrbis) {
                return false;
            }
        } else if (gamepad_type == RE::PC_GAMEPAD_TYPE::kTotal) {
            if (a_device == Input::DEVICE::kGamepadDirectX || a_device == Input::DEVICE::kGamepadOrbis) {
                return false;
            }
        }
        return enabled_devices.at(a_device);
    }
    return false;
}

bool MCP::Settings::OSPPresetBox(Theme::Theme& a_theme) {
    bool changed = false;

    // Dropdown for OSP Preset
    ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.25f);
    const std::string_view current_preset_name =
        current_OSP < Presets::OSP::NOSPs ? Presets::OSP::OSPPool.to_name(current_OSP) : "Custom";
    const auto& current_position_label = PositionLabel(current_preset_name);
    if (LocalizedBeginCombo("$SkyPromptMCPSettingsOnScreenPosition", "settings.onScreenPosition",
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

    ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.25f);
    auto& font_name = a_theme.font_name;
    if (const auto selectedInfo = iconFontManager->GetFontInfoByName(font_name);
        selectedInfo && font_name != selectedInfo->GetName()) {
        font_name = std::string(selectedInfo->GetName());
        changed = true;
    }

    if (LocalizedBeginCombo("$SkyPromptMCPThemeFont", "theme.font", font_name.c_str())) {
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

    ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.25f);
    if (SliderFloatCommitted("$SkyPromptMCPThemeFontShadow", "theme.fontShadow", &a_theme.font_shadow, 0.f,
                             1.f)) {
        changed = true;
    }

    ImGuiMCP::SameLine();
    HelpMarker("$SkyPromptMCPThemeFontHelp");

    if (changed) {
        refreshStyle.store(true);
    }

    return changed;
}

void MCP::Settings::LoadDefaultPromptKeys() {
    default_keys = {{Input::DEVICE::kKeyboardMouse,
                     {Input::Manager::Convert(KEY::kNum1, RE::INPUT_DEVICE::kKeyboard),
                      Input::Manager::Convert(KEY::kNum2, RE::INPUT_DEVICE::kKeyboard),
                      Input::Manager::Convert(KEY::kNum3, RE::INPUT_DEVICE::kKeyboard),
                      Input::Manager::Convert(KEY::kNum4, RE::INPUT_DEVICE::kKeyboard)}},
                    {Input::DEVICE::kGamepadDirectX,
                     {Input::Manager::Convert(GAMEPAD_DIRECTX::kB, RE::INPUT_DEVICE::kGamepad),
                      Input::Manager::Convert(GAMEPAD_DIRECTX::kX, RE::INPUT_DEVICE::kGamepad),
                      Input::Manager::Convert(GAMEPAD_DIRECTX::kY, RE::INPUT_DEVICE::kGamepad),
                      Input::Manager::Convert(GAMEPAD_DIRECTX::kA, RE::INPUT_DEVICE::kGamepad)}},
                    {Input::DEVICE::kGamepadOrbis,
                     {Input::Manager::Convert(GAMEPAD_ORBIS::kPS3_B, RE::INPUT_DEVICE::kGamepad),
                      Input::Manager::Convert(GAMEPAD_ORBIS::kPS3_X, RE::INPUT_DEVICE::kGamepad),
                      Input::Manager::Convert(GAMEPAD_ORBIS::kPS3_Y, RE::INPUT_DEVICE::kGamepad),
                      Input::Manager::Convert(GAMEPAD_ORBIS::kPS3_A, RE::INPUT_DEVICE::kGamepad)}}};
    cycle_L = {
        {Input::DEVICE::kKeyboardMouse, Input::Manager::Convert(KEY::kLeft, RE::INPUT_DEVICE::kKeyboard)},
        {Input::DEVICE::kGamepadDirectX, Input::Manager::Convert(GAMEPAD_DIRECTX::kLeft, RE::INPUT_DEVICE::kGamepad)},
        {Input::DEVICE::kGamepadOrbis, Input::Manager::Convert(GAMEPAD_ORBIS::kLeft, RE::INPUT_DEVICE::kGamepad)}};
    cycle_R = {
        {Input::DEVICE::kKeyboardMouse, Input::Manager::Convert(KEY::kRight, RE::INPUT_DEVICE::kKeyboard)},
        {Input::DEVICE::kGamepadDirectX, Input::Manager::Convert(GAMEPAD_DIRECTX::kRight, RE::INPUT_DEVICE::kGamepad)},
        {Input::DEVICE::kGamepadOrbis, Input::Manager::Convert(GAMEPAD_ORBIS::kRight, RE::INPUT_DEVICE::kGamepad)}};
}

namespace {
    void ControlBox(const char* label, const Input::DEVICE selected_device, uint32_t& selected_key) {
        // dropdown with keys for selected device
        const auto converted_key = selected_key;
        if (ImGuiMCP::BeginCombo(label, SKSE::InputMap::GetKeyName(converted_key).c_str())) {
            for (const auto& key_code : Input::Manager::GetKeys(selected_device)) {
                const auto converted_keycode = key_code;
                const auto key_name = SKSE::InputMap::GetKeyName(converted_keycode);
                if (key_name.empty()) {
                    continue;
                }
                const bool isSelected = converted_key == converted_keycode;
                const auto key_label = Translations::WithID(key_name, std::format("key.{}", converted_keycode));
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

        const auto combo_id = std::format("##{}", a_id);
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

    void RenderControl(std::map<Input::DEVICE, uint32_t>& a_controls, const std::string_view a_label,
                       const std::string_view a_id, const std::string_view a_help_key = {}) {
        ImGuiMCP::TextUnformatted(a_label.data(), a_label.data() + a_label.size());
        ImGuiMCP::SameLine();
        ImGuiMCP::SetCursorPosX(200.f);
        ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.30f);
        const auto combo_id = std::format("##{}", a_id);
        ControlBox(combo_id.c_str(), MCP::current_device, a_controls.at(MCP::current_device));
        if (!a_help_key.empty()) {
            ImGuiMCP::SameLine();
            HelpMarker(a_help_key);
        }
    }
};

bool MCP::Settings::CycleControls() {
    bool settingsChanged = false;

    auto temp = cycle_controls.load();
    if (LocalizedCheckbox("$SkyPromptMCPControlsCycleControls", "controls.cycleControls", &temp)) {
        cycle_controls.store(temp);
        settingsChanged = true;
    }
    if (!cycle_controls) {
        return settingsChanged;
    }

    if (current_device != Input::DEVICE::kUnknown) {
        auto before = cycle_L;
        RenderControl(cycle_L, Translations::Get("$SkyPromptMCPControlsCycleLeft"), "controls.cycleLeft");
        if (before != cycle_L) {
            settingsChanged = true;
        }

        before = cycle_R;
        RenderControl(cycle_R, Translations::Get("$SkyPromptMCPControlsCycleRight"), "controls.cycleRight");
        if (before != cycle_R) {
            settingsChanged = true;
        }
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

    // enabled devices
    if (mcp.HasMember("enabled_devices")) {
        auto& enabled_devices_ = mcp["enabled_devices"];
        for (auto it = enabled_devices_.MemberBegin(); it != enabled_devices_.MemberEnd(); ++it) {
            const auto device = Input::from_string_to_device(it->name.GetString());
            if (device == Input::DEVICE::kUnknown) {
                logger::error("Unknown device in settings.json");
                continue;
            }
            const auto enabled = it->value.GetBool();
            if (enabled_devices.contains(device)) {
                enabled_devices.at(device) = enabled;
            }
        }
    }

    // n_max_buttons
    if (mcp.HasMember("n_max_buttons")) {
        Theme::default_theme.n_max_buttons = mcp["n_max_buttons"].GetInt();
    }

    // prompt keys
    if (mcp.HasMember("keys")) {
        auto& prompt_keys_json = mcp["keys"];
        for (auto it = prompt_keys_json.MemberBegin(); it != prompt_keys_json.MemberEnd(); ++it) {
            const auto device = Input::from_string_to_device(it->name.GetString());
            if (device == Input::DEVICE::kUnknown) {
                logger::error("Unknown device in settings.json");
                continue;
            }
            if (it->value.IsArray()) {
                std::vector<uint32_t> keys;
                for (auto& key : it->value.GetArray()) {
                    keys.push_back(key.GetUint());
                }
                if (default_keys.contains(device)) {
                    default_keys.at(device) = keys;
                } else {
                    default_keys[device] = keys;
                }
            }
        }
    } else {
        logger::error("Failed to find keys in settings.json");
    }

    if (mcp.HasMember("cycle_controls")) {
        cycle_controls = mcp["cycle_controls"].GetBool();
    }

    if (mcp.HasMember("cycle_L")) {
        auto& cycle_L_json = mcp["cycle_L"];
        for (auto it = cycle_L_json.MemberBegin(); it != cycle_L_json.MemberEnd(); ++it) {
            const auto device = Input::from_string_to_device(it->name.GetString());
            if (device == Input::DEVICE::kUnknown) {
                logger::error("Unknown device in settings.json");
                continue;
            }
            cycle_L[device] = it->value.GetUint();
        }
    }
    if (mcp.HasMember("cycle_R")) {
        auto& cycle_R_json = mcp["cycle_R"];
        for (auto it = cycle_R_json.MemberBegin(); it != cycle_R_json.MemberEnd(); ++it) {
            const auto device = Input::from_string_to_device(it->name.GetString());
            if (device == Input::DEVICE::kUnknown) {
                logger::error("Unknown device in settings.json");
                continue;
            }
            cycle_R[device] = it->value.GetUint();
        }
    }

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
    std::unique_lock lock(Theme::m_theme_);
    theme_editor.Render();
    auto& theme = theme_editor.GetTheme();

    bool max_buttons_changed;
    if (theme_editor.filename.empty()) {
        max_buttons_changed = SliderIntCommitted("$SkyPromptMCPControlsMaxButtons", "controls.maxButtons",
                                                  &theme.n_max_buttons, 1, 4);
    } else {
        const auto label = Translations::ImGuiLabel("$SkyPromptMCPControlsMaxButtons", "controls.maxButtons");
        max_buttons_changed = ImGuiMCP::InputInt(label.c_str(), &theme.n_max_buttons);
        theme.n_max_buttons = std::max(theme.n_max_buttons, 1);
    }
    if (max_buttons_changed) {
        theme_editor.OnChanged();
    }

    ImGuiMCP::Separator();
    // Checkbox for each device
    bool settingsChanged = false;
    for (const auto& device : Settings::enabled_devices | std::views::keys) {
        const auto id = std::format("controls.enabledDevice.{}", static_cast<int>(device));
        if (LocalizedCheckboxText(DeviceLabel(device), id, &Settings::enabled_devices.at(device))) {
            settingsChanged = true;
        }
        if (device != Settings::enabled_devices.rbegin()->first) {
            ImGuiMCP::SameLine();
        }
    }

    const auto prompt_keys_before = Settings::default_keys;

    LocalizedText("$SkyPromptMCPControlsDeviceSelection");
    ImGuiMCP::SameLine();
    ImGuiMCP::SetCursorPosX(200.f);
    ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.25f);
    DeviceBox("controls.deviceSelection");

    if (current_device != Input::DEVICE::kUnknown) {
        for (auto i = 0; i < Theme::default_theme.n_max_buttons; i++) {
            std::map<Input::DEVICE, uint32_t> curr_controls;
            for (const auto& [device, key] : Settings::default_keys) {
                curr_controls[device] = key.at(i);
            }
            RenderControl(curr_controls, Translations::Format("$SkyPromptMCPControlsButton", i + 1),
                          std::format("controls.button.{}", i + 1));
            for (auto& [device, key] : Settings::default_keys) {
                key.at(i) = curr_controls[device];
            }
        }
    }

    if (Settings::CycleControls()) {
        settingsChanged = true;
    }

    if (settingsChanged || prompt_keys_before != Settings::default_keys) {
        Settings::to_json();
    }

    ImGuiMCP::TextUnformatted("");
    Settings::SpecialCommands::Render();
}

void __stdcall MCP::RenderTheme() {
    if (LocalizedButton("$SkyPromptMCPThemeReloadThemes", "theme.reloadThemes")) {
        Settings::ReloadThemes();
        refreshStyle.store(true);
    }

    std::unique_lock lock(Theme::m_theme_);
    theme_editor.Render();
    auto& theme = theme_editor.GetTheme();
    if (Settings::FontSettings(theme)) {
        theme_editor.OnChanged();
    }
    RenderThemeExport(theme);
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

    if (SliderFloatCommitted("$SkyPromptMCPSpecialCommandsResponsiveness", "special.responsiveness", &responsiveness,
                             0.0f, 1.0f)) {
        to_json();
        ImGui::Renderer::UpdateMaxIntervalBetweenPresses();
    }
    ImGuiMCP::SameLine();
    HelpMarker("$SkyPromptMCPSpecialCommandsResponsivenessHelp");
}

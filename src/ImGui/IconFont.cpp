#include "IconsFonts.h"
#include "ClibUtilsQTR/StringHelpers.hpp"

namespace IconFont {
    IconTexture::IconTexture(const std::wstring_view a_iconName) :
        Texture(LR"(Data/Interface/ImGuiIcons/Icons/)", a_iconName) {
    }

    bool IconTexture::Load(const bool a_resizeToScreenRes) {
        const bool result = Texture::Load(a_resizeToScreenRes);

        if (result) {
            // store original size
            imageSize = size;
            // don't need this
            if (image) {
                image.reset();
            }
        }

        return result;
    }

    Manager::FontInfo::FontInfo(const std::string& a_name_without_ext, const std::string& a_ext) {
        nameWithoutExtension = a_name_without_ext;
        extension = a_ext;
        if (!extension.empty() && extension.front() != '.') {
            extension.insert(extension.begin(), '.');
        }
        nameWithExtension = nameWithoutExtension + extension;
    }

    Manager::FontInfo::FontInfo(const std::string& a_name_with_ext) {
        nameWithExtension = a_name_with_ext;
        const auto last_dot_pos = a_name_with_ext.find_last_of('.');
        if (last_dot_pos == std::string::npos || last_dot_pos == 0 || last_dot_pos == a_name_with_ext.size() - 1) {
            nameWithoutExtension = a_name_with_ext;
            extension.clear();
            return;
        }

        nameWithoutExtension = a_name_with_ext.substr(0, last_dot_pos);
        extension = a_name_with_ext.substr(last_dot_pos);
    }

    bool Manager::FontInfo::operator<(const FontInfo& a_rhs) const {
        if (nameWithoutExtension != a_rhs.nameWithoutExtension) {
            return nameWithoutExtension < a_rhs.nameWithoutExtension;
        }
        if (extension != a_rhs.extension) {
            return extension < a_rhs.extension;
        }
        return nameWithExtension < a_rhs.nameWithExtension;
    }

    bool Manager::FontInfo::operator==(const FontInfo& a_rhs) const {
        return nameWithExtension == a_rhs.nameWithExtension ||
               nameWithoutExtension == a_rhs.nameWithoutExtension &&
               extension == a_rhs.extension;
    }

    void Manager::LoadIcons() {
        unknownKey.Load();

        upKey.Load();
        downKey.Load();
        leftKey.Load();
        rightKey.Load();

        std::ranges::for_each(keyboard, [](auto& IconTexture) {
            IconTexture.second.Load();
        });
        std::ranges::for_each(gamePad, [](auto& IconTexture) {
            auto& [xbox, ps4] = IconTexture.second;
            xbox.Load();
            ps4.Load();
        });
        std::ranges::for_each(mouse, [](auto& IconTexture) {
            IconTexture.second.Load();
        });

        stepperLeft.Load();
        stepperRight.Load();
        checkbox.Load();
        checkboxFilled.Load();
    }

    bool Manager::LoadFonts(const std::set<std::string>& requiredFonts) {
        std::set<FontInfo> discoveredFonts;
        const std::filesystem::path fontPath = R"(Data\Interface\ImGuiIcons\Fonts\)";
        for (const auto& entry : std::filesystem::directory_iterator(fontPath)) {
            const auto extension = StringHelpers::toLowercase(entry.path().extension().string());
            if (extension == ".ttf" || extension == ".otf") {
                discoveredFonts.emplace(entry.path().filename().replace_extension("").string(), extension);
            }
        }

        availableFonts = std::move(discoveredFonts);
        if (availableFonts.empty()) {
            logger::error("No fonts found in {}", fontPath.string());
            return false;
        }

        ImFontConfig config;
        config.Flags = ImFontFlags_NoLoadError;
        const auto atlas = ImGui::GetIO().Fonts;
        for (const auto& filename : requiredFonts) {
            const auto name = GetFontInfoForRendering(filename).GetName();
            if (fonts.contains(name)) {
                continue;
            }
            const auto file = (fontPath / name).string();
            const auto font = atlas->AddFontFromFileTTF(file.c_str(), 0.0f, &config);
            if (!font) {
                logger::error("Failed to load font: {}", file);
                return false;
            }
            fonts.emplace(name, font);
        }
        return true;
    }

    ImFont* Manager::GetFont(const std::string_view filename) const {
        if (const auto it = fonts.find(filename); it != fonts.end()) {
            return it->second;
        }
        const auto it = fonts.find(GetFontInfoForRendering(filename).GetName());
        return it != fonts.end() ? it->second : fonts.begin()->second;
    }

    const Manager::FontInfo& Manager::GetFontInfoForRendering(const std::string_view filename) const {
        const auto font = GetFontInfoByName(filename);
        return font ? *font : *availableFonts.begin();
    }

    const std::set<Manager::FontInfo>& Manager::GetAvailableFonts() const {
        return availableFonts;
    }

    const Manager::FontInfo* Manager::GetFontInfoByName(const std::string_view a_fontName) const {
        const std::string key{a_fontName};

        const FontInfo exactKey{key};
        auto it = availableFonts.lower_bound(exactKey);
        if (it != availableFonts.end() && it->GetName() == key) {
            return std::addressof(*it);
        }

        const FontInfo baseKey{key, {}};
        it = availableFonts.lower_bound(baseKey);
        if (it != availableFonts.end() && it->GetNameWithoutExtension() == key) {
            return std::addressof(*it);
        }

        return nullptr;
    }

    const IconTexture* Manager::GetStepperLeft() const {
        return &stepperLeft;
    }

    const IconTexture* Manager::GetStepperRight() const {
        return &stepperRight;
    }

    const IconTexture* Manager::GetCheckbox() const {
        return &checkbox;
    }

    const IconTexture* Manager::GetCheckboxFilled() const {
        return &checkboxFilled;
    }

    const IconTexture* Manager::GetIcon(const std::uint32_t key) {
        switch (key) {
            case KEY::kUp:
            case SKSE::InputMap::kGamepadButtonOffset_DPAD_UP:
                return &upKey;
            case KEY::kDown:
            case SKSE::InputMap::kGamepadButtonOffset_DPAD_DOWN:
                return &downKey;
            case KEY::kLeft:
            case SKSE::InputMap::kGamepadButtonOffset_DPAD_LEFT:
                return &leftKey;
            case KEY::kRight:
            case SKSE::InputMap::kGamepadButtonOffset_DPAD_RIGHT:
                return &rightKey;
            default: {
                if (const auto inputDevice = MANAGER(Input)->GetInputDevice();
                    inputDevice == Input::DEVICE::kKeyboardMouse) {
                    if (key >= SKSE::InputMap::kMacro_MouseButtonOffset) {
                        if (const auto it = mouse.find(key); it != mouse.end()) {
                            return &it->second;
                        }
                    } else if (const auto it = keyboard.find(static_cast<KEY>(key)); it != keyboard.end()) {
                        return &it->second;
                    }
                } else {
                    if (const auto it = gamePad.find(key); it != gamePad.end()) {
                        return GetGamePadIcon(it->second);
                    }
                }
                return &unknownKey;
            }
        }
    }

    const IconTexture* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const {
        switch (buttonScheme) {
            case BUTTON_SCHEME::kAutoDetect:
                return MANAGER(Input)->GetInputDevice() == Input::kGamepad &&
                       RE::ControlMap::GetSingleton()->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis
                    ? &a_icons.ps4 : &a_icons.xbox;
            case BUTTON_SCHEME::kXbox:
                return &a_icons.xbox;
            case BUTTON_SCHEME::kPS4:
                return &a_icons.ps4;
            default:
                return &a_icons.xbox;
        }
    }

    bool Manager::IsImGuiIconsInstalled() {
        return std::filesystem::exists(R"(Data\Interface\ImGuiIcons\Fonts\Jost-Regular.ttf)");
    }
}

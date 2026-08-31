#include "IconsFonts.h"
#include "Renderer.h"
#include "Translations.h"
#include <imgui_impl_dx11.h>
#include "ClibUtilsQTR/StringHelpers.hpp"

namespace {
    ImFont* LoadFontIconSet(const float a_fontSize, const ImVector<ImWchar>& a_ranges,
                            const std::string& a_fontPath) {
        const auto& io = ImGui::GetIO();

        const auto iconManager = MANAGER(IconFont);
        const auto& availableFonts = iconManager->GetAvailableFonts();
        if (availableFonts.empty()) {
            logger::error("No available fonts in {}", a_fontPath);
            return nullptr;
        }

        const auto& font_name = Theme::last_theme->font_name;
        const auto selectedFontInfo = iconManager->GetFontInfoByName(font_name);
        const auto& fontInfo = selectedFontInfo ? *selectedFontInfo : *availableFonts.begin();
        const auto a_fontName = (std::filesystem::path(a_fontPath) / fontInfo.GetName()).string();
        const auto a_font = io.Fonts->AddFontFromFileTTF(a_fontName.c_str(), a_fontSize, nullptr, a_ranges.Data);
        if (!a_font) {
            logger::error("Failed to load font: {}", a_fontName);
            return nullptr;
        }

        return a_font;
    }
}

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

    bool Manager::ReloadFonts() {
        auto& io = ImGui::GetIO();
        std::set<FontInfo> discoveredFonts{};

        constexpr auto fontPath = R"(Data\Interface\ImGuiIcons\Fonts\)";

        for (const auto& entry : std::filesystem::directory_iterator(fontPath)) {
            const auto extension = StringHelpers::toLowercase(entry.path().extension().string());
            if (extension == ".ttf" || extension == ".otf") {
                discoveredFonts.emplace(entry.path().filename().replace_extension("").string(), extension);
            }
        }

        if (discoveredFonts.empty()) {
            logger::error("No fonts found in {}", fontPath);
            return false;
        }

        availableFonts = std::move(discoveredFonts);

        ImVector<ImWchar> ranges;

        ImFontGlyphRangesBuilder builder;
        builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
        const auto translated_glyphs = Translations::GlyphText();
        builder.AddText(translated_glyphs.c_str());
        builder.AddChar(0xf030); // CAMERA
        builder.AddChar(0xf017); // CLOCK
        builder.AddChar(0xf183); // PERSON
        builder.AddChar(0xf042); // CONTRAST
        builder.AddChar(0xf03e); // IMAGE

        builder.BuildRanges(&ranges);

        const auto resolutionScale = ImGui::Renderer::GetResolutionScale();

        float a_fontsize;
        {
            const auto& prompt_size = Theme::last_theme->prompt_size;
            a_fontsize = prompt_size * resolutionScale;
        }
        const auto a_smallfontsize = a_fontsize * 0.65f;

        constexpr int kMaxAtlasDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION; // 16384

        auto smallFontPtr = &smallFont;
        auto a_fontPath = fontPath;

        auto tryBuildFonts = [&io, &ranges, a_fontsize, a_smallfontsize, smallFontPtr,
                a_fontPath](float scale) -> bool {
            io.Fonts->Clear();

            io.FontDefault = LoadFontIconSet(a_fontsize * scale, ranges, a_fontPath);
            *smallFontPtr = LoadFontIconSet(a_smallfontsize * scale, ranges, a_fontPath);

            if (!io.FontDefault || !*smallFontPtr) {
                logger::error("Failed to load one or more fonts for scale {}", scale);
                return false;
            }

            if (!io.Fonts->Build()) {
                logger::error("Failed to rebuild ImGui font atlas at scale {}", scale);
                return false;
            }

            const auto texWidth = io.Fonts->TexWidth;
            const auto texHeight = io.Fonts->TexHeight;
            if (texWidth > kMaxAtlasDimension || texHeight > kMaxAtlasDimension) {
                logger::error("ImGui font atlas size {}x{} exceeds DirectX limit {} (scale {})", texWidth, texHeight,
                              kMaxAtlasDimension, scale);
                return false;
            }

            return true;
        };

        float scale = 1.0f;
        bool built = tryBuildFonts(scale);
        while (!built && scale > 0.1f) {
            scale *= 0.8f;
            logger::warn("Retrying font atlas build for font path {} at scale {}", a_fontPath, scale);
            built = tryBuildFonts(scale);
        }

        if (!built) {
            logger::critical("Failed to build ImGui font atlas within DirectX texture limits");
            return false;
        }

        ImGui_ImplDX11_InvalidateDeviceObjects();
        if (!ImGui_ImplDX11_CreateDeviceObjects()) {
            logger::error("Failed to recreate ImGui device objects after font reload");
            io.Fonts->Clear();
            smallFont = nullptr;
            return false;
        }
        return true;
    }

    ImFont* Manager::GetSmallFont() const {
        return smallFont;
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
                return MANAGER(Input)->GetInputDevice() == Input::DEVICE::kGamepadOrbis ? &a_icons.ps4 : &a_icons.xbox;
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

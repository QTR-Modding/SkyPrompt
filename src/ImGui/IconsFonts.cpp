#include "IconsFonts.h"
#include "Renderer.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include "SkyPrompt/AddOns.hpp"

namespace {
    ImFont* LoadFontIconSet(const float a_fontSize, const ImVector<ImWchar>& a_ranges,
                                   const std::string& a_fontPath) {
        const auto& io = ImGui::GetIO();

        const auto& font_name = Theme::last_theme->font_name;
        auto a_fontName =
            a_fontPath +
            (MCP::Settings::font_names.contains(font_name) ? font_name : *MCP::Settings::font_names.begin()) + ".ttf";
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
        std::set<std::string> availableFonts{};

        for (const auto& entry : std::filesystem::directory_iterator(fontPath)) {
            if (entry.path().extension() == ".ttf") {
                availableFonts.insert(entry.path().filename().replace_extension("").string());
            }
        }

        if (availableFonts.empty()) {
            logger::error("No fonts found in {}", fontPath);
            return false;
        }

        MCP::Settings::font_names = std::move(availableFonts);

        ImVector<ImWchar> ranges;

        ImFontGlyphRangesBuilder builder;
        builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
        builder.AddChar(0xf030); // CAMERA
        builder.AddChar(0xf017); // CLOCK
        builder.AddChar(0xf183); // PERSON
        builder.AddChar(0xf042); // CONTRAST
        builder.AddChar(0xf03e); // IMAGE

        builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
        builder.AddRanges(io.Fonts->GetGlyphRangesGreek());
        builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
        builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddRanges(io.Fonts->GetGlyphRangesThai());
        builder.AddRanges(io.Fonts->GetGlyphRangesVietnamese());

        if (MCP::Settings::extended_cjk) {
            builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
            builder.AddRanges(io.Fonts->GetGlyphRangesKorean());
            builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
        }

        builder.BuildRanges(&ranges);

        const auto resolutionScale = ImGui::Renderer::GetResolutionScale();

        float a_fontsize;
        {
            const auto& prompt_size = Theme::last_theme->prompt_size;
            a_fontsize = prompt_size * resolutionScale;
        }
        const auto a_largefontsize = a_fontsize * 1.2f;
        const auto a_smallfontsize = a_fontsize * 0.65f;

        constexpr int kMaxAtlasDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;  // 16384

        auto largeFontPtr = &largeFont;
        auto smallFontPtr = &smallFont;
        auto a_fontPath = fontPath;

        auto tryBuildFonts = [&io, &ranges, a_fontsize, a_largefontsize, a_smallfontsize, largeFontPtr, smallFontPtr, a_fontPath](float scale) -> bool {
            io.Fonts->Clear();
            io.Fonts->TexDesiredWidth = 4096;

            io.FontDefault = LoadFontIconSet(a_fontsize * scale, ranges, a_fontPath);
            *largeFontPtr = LoadFontIconSet(a_largefontsize * scale, ranges, a_fontPath);
            *smallFontPtr = LoadFontIconSet(a_smallfontsize * scale, ranges, a_fontPath);

            if (!io.FontDefault || !*largeFontPtr || !*smallFontPtr) {
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
                logger::error("ImGui font atlas size {}x{} exceeds DirectX limit {} (scale {})", texWidth, texHeight, kMaxAtlasDimension, scale);
                return false;
            }

            return true;
        };

        float scale = 1.0f;
        bool built = tryBuildFonts(scale);
        while (!built && scale > 0.3f) {
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
            return false;
        }
        return true;
    }

    ImFont* Manager::GetLargeFont() const {
        return largeFont;
    }

    ImFont* Manager::GetSmallFont() const {
        return smallFont;
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

    bool Manager::IsImGuiIconsInstalled() const {
        return std::filesystem::exists(fontName);
    }
}

namespace {
    // Utility: scale a packed ImU32 color's alpha by factor in [0,1]
    ImU32 MulAlpha(ImU32 c, float a) {
        a = ImClamp(a, 0.0f, 1.0f);
        const int A = (c >> IM_COL32_A_SHIFT) & 0xFF;
        const int newA = static_cast<int>(A * a + 0.5f);
        return (c & ~IM_COL32_A_MASK) | (static_cast<ImU32>(newA) << IM_COL32_A_SHIFT);
    }


    void DrawCircle(ImDrawList* drawList, const ImVec2 a_center, const float a_radius, const float progress,
                    const float thickness,
                    const std::optional<uint32_t> a_color = std::nullopt,
                    const std::optional<float> start_angle = std::nullopt) {
        const auto startColor = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 60);
        const auto endColor = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 180);

        constexpr int numSegments = 64;
        const float startAngle = start_angle.has_value() ? start_angle.value() - IM_PI / 2 : -IM_PI / 2;
        // Start at the top
        const float endAngle = startAngle + progress * 2.0f * IM_PI;

        ImVec2 prevPoint = a_center + ImVec2(cosf(startAngle) * a_radius, sinf(startAngle) * a_radius);

        if (startColor != endColor) {
            for (int i = 1; i <= numSegments; ++i) {
                const float t = i / static_cast<float>(numSegments);
                const float angle = startAngle + t * (endAngle - startAngle);
                ImVec2 newPoint = a_center + ImVec2(cosf(angle) * a_radius, sinf(angle) * a_radius);

                // Interpolate color
                const ImU32 color = IM_COL32(
                    (1 - t) * (startColor >> IM_COL32_R_SHIFT & 0xFF) + t * (endColor >> IM_COL32_R_SHIFT & 0xFF),
                    (1 - t) * (startColor >> IM_COL32_G_SHIFT & 0xFF) + t * (endColor >> IM_COL32_G_SHIFT & 0xFF),
                    (1 - t) * (startColor >> IM_COL32_B_SHIFT & 0xFF) + t * (endColor >> IM_COL32_B_SHIFT & 0xFF),
                    (1 - t) * (startColor >> IM_COL32_A_SHIFT & 0xFF) + t * (endColor >> IM_COL32_A_SHIFT & 0xFF)
                    );

                drawList->AddLine(prevPoint, newPoint, color, thickness);
                prevPoint = newPoint;
            }
        } else {
            drawList->PathArcTo(a_center, a_radius, startAngle, endAngle, numSegments);
            drawList->PathStroke(endColor, false, thickness);
        }
    }

    void DrawTriangle(ImDrawList* drawList, const ImVec2 iconCenter, const float outer_radius, const float inner_radius,
                      const std::optional<uint32_t> a_color = std::nullopt) {
        const float triangle_width = inner_radius * 0.5f;
        const float triangle_height = inner_radius * 0.25f;

        const auto p1 = ImVec2(iconCenter.x, iconCenter.y - outer_radius + triangle_height);
        // Tip (bottom, closer to center)
        const auto p2 = ImVec2(iconCenter.x - triangle_width * 0.5f, iconCenter.y - outer_radius - triangle_height);
        // Top left
        const auto p3 = ImVec2(iconCenter.x + triangle_width * 0.5f, iconCenter.y - outer_radius - triangle_height);
        // Top right
        const auto color = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 200);
        drawList->AddTriangleFilled(p1, p2, p3, color);
    }

    void DrawProgressMark(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float outer_radius,
                          const float a_thickness) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 30);
        DrawCircle(a_drawlist, iconCenter, outer_radius, 1.0, a_thickness, aColor);
    }

    void DrawHoldMark(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float outer_radius,
                      const float inner_radius) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 180);
        DrawTriangle(a_drawlist, iconCenter, outer_radius, inner_radius * 0.6f, aColor);
    }

    void DrawCross1(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
        constexpr auto a_red = IM_COL32(147, 39, 41, 180);
        const ImVec2 topRight = iconCenter + ImVec2(a_radius, -a_radius);
        const ImVec2 bottomLeft = iconCenter + ImVec2(-a_radius, a_radius);
        a_drawlist->AddLine(topRight, bottomLeft, a_red, a_thickness);
    }

    void DrawCross2(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
        constexpr auto a_red = IM_COL32(147, 39, 41, 180);
        const ImVec2 topLeft = iconCenter + ImVec2(-a_radius, -a_radius);
        const ImVec2 bottomRight = iconCenter + ImVec2(a_radius, a_radius);
        a_drawlist->AddLine(topLeft, bottomRight, a_red, a_thickness);
    }

    void DrawSkipPrompt(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius,
                        const float a_thickness) {
        constexpr auto a_yellow = IM_COL32(228, 185, 76, 100);
        DrawCircle(a_drawlist, iconCenter, a_radius, 1.f, a_thickness, a_yellow);
    }

    void DrawDeleteAll(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness,
                       const float progress) {
        constexpr auto a_red = IM_COL32(147, 39, 41, 180);
        DrawCircle(a_drawlist, iconCenter, a_radius, progress, a_thickness, a_red);
    }

    void DrawProgressCircle(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius,
                            const float a_thickness, const float progress, const float start_angle) {
        const auto aColor = progress + ImGui::Renderer::progress_circle_offset >= 1.f
                                ? IM_COL32(228, 185, 76, 180)
                                : IM_COL32(255, 255, 255, 180);
        DrawCircle(a_drawlist, iconCenter, a_radius, std::max(progress, 0.f), a_thickness, aColor,
                   std::max(start_angle, 0.f));
    }


    float GetIconSize() {
        const auto a_fontsize = ImGui::GetIO().FontDefault->FontSize;
        return a_fontsize * Theme::last_theme->icon2font_ratio;
    }

    ImVec2 GetIconSizeImVec() {
        const auto a_size = GetIconSize();
        return {a_size, a_size};
    }


    void AddTextWithShadow(ImDrawList* draw_list, ImFont* font, const float font_size, const ImVec2 position,
                           const ImU32 text_color, const char* text) {
        if (!draw_list || !font || !text || !*text) return;

        const auto shadow_color = IM_COL32(0, 0, 0, 255 * Theme::last_theme->font_shadow);
        draw_list->AddText(font, font_size, position + ImVec2(2.5f, 2.5f), shadow_color, text);
        draw_list->AddText(font, font_size, position, text_color, text);
    }


    ImVec2 ButtonIconWithCircularProgress(const char* a_text, const uint32_t a_text_color,
                                          const IconFont::IconTexture* a_texture, const float progress,
                                          const float button_state) {
        if (!a_texture || !a_texture->srView.Get()) {
            logger::error("Button icon texture not loaded.");
            return {};
        }

        // Calculate sizes
        const ImVec2 textSize = ImGui::CalcTextSize(a_text);

        const auto a_iconsize = GetIconSize();
        const float circleDiameter = a_iconsize * 1.25f;
        const float rowHeight = std::max(circleDiameter, textSize.y);

        // Record the "start" cursor Y.
        const float startY = ImGui::GetCursorPosY();

        // 1) Center the icon in this row
        const float iconOffset = (rowHeight - a_iconsize) * 0.5f;
        ImGui::SetCursorPosY(startY + iconOffset);

        // 2) Draw the icon (and get its final size)
        const auto iconSize = ButtonIcon(a_texture); // This already calls ImGui::Image(...)

        // 3) Draw the circle around the icon
        // Figure out where the icon was drawn so we can compute the circle center
        const ImVec2 iconRenderPos = ImGui::GetItemRectMin();
        const ImVec2 iconCenter{
            iconRenderPos.x + (iconSize.x * 0.5f),
            iconRenderPos.y + (iconSize.y * 0.5f)
        };
        const float radius = iconSize.y * 0.5f;
        const float thickness = radius / 6.f;
        const float circle_radius = circleDiameter / 2.f;

        const auto a_drawlist = ImGui::GetWindowDrawList();

        if (MCP::Settings::SpecialCommands::visualize) {
            if (button_state < 3.f) {
                if (button_state > 2.f) {
                    DrawCross2(a_drawlist, iconCenter, circle_radius * 0.6f, thickness);
                }
                if (button_state > 1.f) {
                    DrawCross1(a_drawlist, iconCenter, circle_radius * 0.6f, thickness);
                }
            } else if (progress > 0.f) {
                DrawDeleteAll(a_drawlist, iconCenter, circle_radius, thickness, progress);
            } else {
                DrawSkipPrompt(a_drawlist, iconCenter, circle_radius, thickness);
            }
        }

        if (const bool SinglePress_progress = progress < 0.f; SinglePress_progress || button_state > 0.f) {
            DrawProgressMark(a_drawlist, iconCenter, circle_radius, thickness);
            if (!SinglePress_progress) {
                DrawHoldMark(a_drawlist, iconCenter, circle_radius, radius);
            }
            if (button_state < 3.f) {
                const auto start_deg = SinglePress_progress
                                           ? 360.f * (1 + progress)
                                           : ImGui::Renderer::progress_circle_offset_deg;
                const auto a_progress = SinglePress_progress
                                            ? -progress
                                            : progress - ImGui::Renderer::progress_circle_offset;
                DrawProgressCircle(a_drawlist, iconCenter, circle_radius, thickness, a_progress,
                                   RE::deg_to_rad(start_deg));
            }
        }

        // 4) Move horizontally for the text
        ImGui::SameLine();

        // 5) Center the text in this row
        const float textOffset = (rowHeight - textSize.y) * 0.5f;
        ImGui::SetCursorPosY(startY + textOffset);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + circle_radius - radius + (rowHeight - textSize.y) * 0.5f);

        AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(),
                          ImGui::GetCursorScreenPos(), a_text_color ? a_text_color : IM_COL32(255, 255, 255, 255),
                          a_text);

        // Move ImGui cursor manually to avoid overlap
        ImGui::Dummy(textSize); // Moves cursor forward horizontally

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset * Theme::last_theme->linespacing * 5);

        return iconCenter;
    }

    void AddImageRotated(ImDrawList* dl, const ImTextureID tex,
                         const ImVec2 center, const ImVec2 size,
                         const float angle, const ImU32 col) {
        const auto h = ImVec2(size.x * 0.5f, size.y * 0.5f);
        const float c = cosf(angle), s = sinf(angle);

        auto rot = [&](const ImVec2 p) -> ImVec2 {
            return ImVec2(center.x + (p.x * c - p.y * s),
                          center.y + (p.x * s + p.y * c));
        };

        // Quad corners before rotation (relative to center)
        const ImVec2 p0 = rot(ImVec2(-h.x, -h.y));
        const ImVec2 p1 = rot(ImVec2(+h.x, -h.y));
        const ImVec2 p2 = rot(ImVec2(+h.x, +h.y));
        const ImVec2 p3 = rot(ImVec2(-h.x, +h.y));

        // Standard UVs
        constexpr ImVec2 uv0(0, 0), uv1(1, 0), uv2(1, 1), uv3(0, 1);
        dl->AddImageQuad(tex, p0, p1, p2, p3, uv0, uv1, uv2, uv3, col);
    }

    // helper: do NOT push/pop clip; caller controls clip once per frame
    void AddTextRotated(ImDrawList* dl, ImFont* font, const float font_size,
                        const ImVec2 pivot, const ImU32 col,
                        const char* text_begin, const char* text_end,
                        const float angle, const bool center_on_pivot = true) {
        if (!text_begin) return;
        if (!text_end) text_end = text_begin + strlen(text_begin);

        ImVec2 topleft = pivot;
        if (center_on_pivot) {
            const ImVec2 ts = ImGui::CalcTextSize(text_begin, text_end);
            topleft.x -= ts.x * 0.5f;
            topleft.y -= ts.y * 0.5f;
        }

        const int vtx_start = dl->VtxBuffer.Size;
        // use the current clip on the draw list
        const ImVec4 clip = dl->_ClipRectStack.back();
        font->RenderText(dl, font_size, topleft, col, clip, text_begin, text_end, 0.0f, false);
        const int vtx_end = dl->VtxBuffer.Size;

        const float c = cosf(angle), s = sinf(angle);
        for (int i = vtx_start; i < vtx_end; ++i) {
            const ImVec2 p = dl->VtxBuffer[i].pos;
            const ImVec2 d = {p.x - pivot.x, p.y - pivot.y};
            dl->VtxBuffer[i].pos.x = pivot.x + d.x * c - d.y * s;
            dl->VtxBuffer[i].pos.y = pivot.y + d.x * s + d.y * c;
        }
    }

    void DrawTriangleRotated(ImDrawList* dl, const ImVec2 center,
                             const float outer_radius, const float inner_radius,
                             const float angle, const ImU32 col) {
        const float tri_w = inner_radius * 0.5f;
        const float tri_h = inner_radius * 0.25f;

        // local (unrotated) vertices – tip is “up” (toward -Y)
        const auto p1 = ImVec2(0.0f, -outer_radius + tri_h); // tip (closer to center)
        const auto p2 = ImVec2(-tri_w * 0.5f, -outer_radius - tri_h);
        const auto p3 = ImVec2(+tri_w * 0.5f, -outer_radius - tri_h);

        const float c = cosf(angle), s = sinf(angle);
        auto rot = [&](const ImVec2 p) {
            return ImVec2(center.x + p.x * c - p.y * s,
                          center.y + p.x * s + p.y * c);
        };

        dl->AddTriangleFilled(rot(p1), rot(p2), rot(p3), col);
    }

    void RenderPromptsRadialRotated(const ImVec2 center,
                                    const std::vector<ImGui::RenderInfo>& batch,
                                    const float lineSpacingPx,
                                    const float baseRadius,
                                    const float startAngleRad) {
        if (batch.empty()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        ImFont* font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize();

        dl->PushClipRectFullScreen();

        const float iconSz = ImGui::GetIO().FontDefault->FontSize * Theme::last_theme->icon2font_ratio;
        const ImVec2 iconSzV = {iconSz, iconSz};

        const float circleDia = iconSz * 1.25f;
        const float outerR = circleDia * 0.5f;

        // Tangential footprint should be the ring's outer diameter, not iconSz.
        // Also mimic SameLine spacing a bit so neighboring rings don't kiss.
        const float arcSpacing = ImGui::GetStyle().ItemSpacing.x; // small extra gap (pixels)
        const float rowFootprint = circleDia;

        // Final angular step (radians per item)
        const float r = std::max(baseRadius, iconSz * 1.6f);
        const float step = std::max(0.001f, (rowFootprint + arcSpacing + lineSpacingPx) / r);

        const int n = static_cast<int>(batch.size());
        const float firstA = startAngleRad - 0.5f * (n - 1) * step; // center on the ray

        for (int i = 0; i < n; ++i) {
            const auto& ri = batch[i];
            const float a = firstA + i * step;

            // position on ring
            const ImVec2 iconCenter{center.x + r * cosf(a), center.y + r * sinf(a)};

            // orient along the radius; flip on left hemisphere to keep text upright
            float orient = a;
            if (cosf(a) < 0.0f) orient += IM_PI;

            // --- icon ---
            if (ri.texture && ri.texture->srView.Get()) {
                AddImageRotated(dl, (ImTextureID)ri.texture->srView.Get(), iconCenter, iconSzV, orient,
                                IM_COL32(255, 255, 255, static_cast<int>(255 * ri.alpha)));
            }

            {
                const float thick = outerR / 6.f;

                if (MCP::Settings::SpecialCommands::visualize) {
                    if (ri.button_state < 3.f) {
                        if (ri.button_state > 2.f) DrawCross2(dl, iconCenter, outerR * 0.6f, thick);
                        if (ri.button_state > 1.f) DrawCross1(dl, iconCenter, outerR * 0.6f, thick);
                    } else if (ri.progress > 0.f) {
                        DrawDeleteAll(dl, iconCenter, outerR, thick, ri.progress);
                    } else {
                        DrawSkipPrompt(dl, iconCenter, outerR, thick);
                    }
                }

                const bool singlePress = (ri.progress < 0.f);
                if (singlePress || ri.button_state > 0.f) {
                    DrawProgressMark(dl, iconCenter, outerR, thick);
                    if (!singlePress) {
                        const float tri_angle = a;
                        constexpr ImU32 tri_col = IM_COL32(255, 255, 255, 180);
                        DrawTriangleRotated(dl, iconCenter, outerR, iconSz * 0.5f, tri_angle, tri_col);
                    }
                    if (ri.button_state < 3.f) {
                        const float startDeg = singlePress
                                                   ? 360.f * (1.f + ri.progress)
                                                   : ImGui::Renderer::progress_circle_offset_deg;
                        const float prog = singlePress
                                               ? -ri.progress
                                               : ri.progress - ImGui::Renderer::progress_circle_offset;
                        DrawProgressCircle(dl, iconCenter, outerR, thick, std::max(prog, 0.f),
                                           RE::deg_to_rad(startDeg) + orient);
                    }
                }
            }

            const ImVec2 textSize = ImGui::CalcTextSize(ri.text.c_str());

            const float circle_radius = iconSz * 1.25f * 0.5f;

            const float spacing = ImGui::GetStyle().ItemSpacing.x;

            // distance from icon center to the *inner edge* of text in the flat layout:
            //   iconCenter + radius (icon right edge)
            // + spacing (SameLine gap)
            // + (circle_radius - radius) (ring clearance)
            // simplifies to: spacing + circle_radius
            const float near_edge_clearance = spacing + circle_radius;

            // place the *text center* so that its inner edge sits at that clearance
            const float center_dist = near_edge_clearance + textSize.x * 0.5f;

            // move along the outward normal; keep zero offset along tangent
            const ImVec2 normal = {cosf(a), sinf(a)};
            const ImVec2 textCenter = {
                iconCenter.x + normal.x * center_dist,
                iconCenter.y + normal.y * center_dist
            };

            // draw rotated text centered on this pivot
            const ImU32 color = MulAlpha(ri.text_color ? ri.text_color : IM_COL32(255, 255, 255, 255), ri.alpha);
            const ImU32 shadow = MulAlpha(
                IM_COL32(0, 0, 0, static_cast<int>(255 * Theme::last_theme->font_shadow)), ri.alpha);

            AddTextRotated(dl, font, fs, {textCenter.x + 2.5f, textCenter.y + 2.5f},
                           shadow, ri.text.c_str(), nullptr, orient, true);
            AddTextRotated(dl, font, fs, textCenter,
                           color, ri.text.c_str(), nullptr, orient, true);
        }

        dl->PopClipRect();
    }

    void RenderPromptsHorizontalCentered(const std::vector<ImGui::RenderInfo>& batch, float lineSpacingPx) {
        if (batch.empty()) return;

        const float iconSz = GetIconSize();
        const float circleDia = iconSz * 1.25f;
        ImFont* font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize();

        struct ItemDim {
            float width;
            float height;
            float textWidth;
            float textHeight;
            float textPad; // icon-to-text gap
        };
        std::vector<ItemDim> dims;
        dims.reserve(batch.size());

        // Measure each item's width (circle + vertical-style text padding + text)
        for (auto& ri : batch) {
            const ImVec2 textSize = ImGui::CalcTextSize(ri.text.c_str());
            const float circle_radius = circleDia * 0.5f;
            const float radius = iconSz * 0.5f;
            const float rowHeight = std::max(circleDia, textSize.y);

            // Same padding formula as ButtonIconWithCircularProgress
            const float textPad = circle_radius - radius + (rowHeight - textSize.y) * 0.5f;

            const float totalWidth = circleDia + textPad + textSize.x;
            dims.push_back({totalWidth, rowHeight, textSize.x, textSize.y, textPad});
        }

        // Total width of batch including spacing
        float totalWidth = 0.0f;
        for (size_t i = 0; i < dims.size(); ++i) {
            totalWidth += dims[i].width;
            if (i != dims.size() - 1)
                totalWidth += lineSpacingPx; // gap between items
        }

        // Start so that batch is centered at current cursor
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        startPos.x -= totalWidth * 0.5f;

        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());

        float xCursor = startPos.x;
        const float yCenter = startPos.y + dims[0].height * 0.5f; // vertical center line

        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& ri = batch[i];
            const auto& dim = dims[i];

            // Icon center position
            const ImVec2 iconCenter{
                xCursor + circleDia * 0.5f,
                yCenter
            };

            // --- Icon ---
            if (ri.texture && ri.texture->srView.Get()) {
                AddImageRotated(dl, (ImTextureID)ri.texture->srView.Get(), iconCenter, {iconSz, iconSz}, 0.0f,
                                IM_COL32(255, 255, 255, static_cast<int>(255 * ri.alpha)));
            }

            {
                const float outerR = circleDia * 0.5f;
                const float thick = outerR / 6.f;

                if (MCP::Settings::SpecialCommands::visualize) {
                    if (ri.button_state < 3.f) {
                        if (ri.button_state > 2.f) DrawCross2(dl, iconCenter, outerR * 0.6f, thick);
                        if (ri.button_state > 1.f) DrawCross1(dl, iconCenter, outerR * 0.6f, thick);
                    } else if (ri.progress > 0.f) {
                        DrawDeleteAll(dl, iconCenter, outerR, thick, ri.progress);
                    } else {
                        DrawSkipPrompt(dl, iconCenter, outerR, thick);
                    }
                }

                const bool singlePress = (ri.progress < 0.f);
                if (singlePress || ri.button_state > 0.f) {
                    DrawProgressMark(dl, iconCenter, outerR, thick);
                    if (!singlePress) {
                        DrawHoldMark(dl, iconCenter, outerR, iconSz * 0.5f);
                    }
                    if (ri.button_state < 3.f) {
                        const float startDeg = singlePress
                                                   ? 360.f * (1.f + ri.progress)
                                                   : ImGui::Renderer::progress_circle_offset_deg;
                        const float prog = singlePress
                                               ? -ri.progress
                                               : ri.progress - ImGui::Renderer::progress_circle_offset;
                        DrawProgressCircle(dl, iconCenter, outerR, thick, std::max(prog, 0.f),
                                           RE::deg_to_rad(startDeg));
                    }
                }
            }

            const ImVec2 textPos{
                xCursor + circleDia + dim.textPad,
                yCenter - dim.textHeight * 0.5f
            };

            const ImU32 color = ri.text_color ? ri.text_color : IM_COL32(255, 255, 255, 255);
            AddTextWithShadow(dl, font, fs, textPos, color, ri.text.c_str());

            // Advance cursor for next item
            xCursor += dim.width + lineSpacingPx;
        }
    }

    SkyPrompt::AddOns::SpecialEffects::SpecialsView
    GetSpecialsView(const Theme::Theme& t) {
        SkyPrompt::AddOns::SpecialEffects::SpecialsView v;
        v.effectID = t.special_effect;
        v.integers = std::span{t.special_integers};
        v.strings = std::span{t.special_strings};
        v.floats = std::span{t.special_floats};
        v.bools = std::span{t.special_bools};
        return v;
    }
}


ImVec2 ImGui::ButtonIcon(const IconFont::IconTexture* a_texture) {
    const auto a_size = GetIconSizeImVec();
    Image(reinterpret_cast<ImTextureID>(a_texture->srView.Get()), a_size);
    return a_size;
}

void ImGui::DrawCycleIndicators(SkyPromptAPI::ClientID curr_index, SkyPromptAPI::ClientID queue_size) {
    auto* iconMgr = MANAGER(IconFont);
    const auto* inputMgr = MANAGER(Input);
    const auto curr_device = inputMgr->GetInputDevice();
    const uint32_t keyL = MCP::Settings::cycle_L.at(curr_device);

    const uint32_t keyR = MCP::Settings::cycle_R.at(curr_device);

    const float iconSz = GetIconSize() * 0.6f;

    const float spacing = GetFontSize() * 0.25f;
    Dummy(ImVec2(0.0f, spacing));

    if (const auto* icoL = iconMgr->GetIcon(keyL))
        Image((ImTextureID)icoL->srView.Get(), {iconSz, iconSz});

    SameLine();

    if (const auto* icoR = iconMgr->GetIcon(keyR))
        Image((ImTextureID)icoR->srView.Get(), {iconSz, iconSz});

    SameLine();

    auto* smallFont = MANAGER(IconFont)->GetSmallFont();
    if (smallFont) {
        PushFont(smallFont);
        const std::string text = std::format("({}/{})", curr_index, queue_size);
        const ImVec2 textSize = CalcTextSize(text.c_str());

        const float textOffset = (iconSz - textSize.y) * 0.5f;
        SetCursorPosY(GetCursorPosY() + textOffset);

        TextUnformatted(text.c_str());
        PopFont();
    }
}

void ImGui::RenderSkyPrompt() {
    if (renderBatch.empty()) {
        return;
    }

    const auto& curr_theme = Theme::last_theme;
    const auto prompt_alignment = curr_theme->prompt_alignment;
    const auto special_effect = curr_theme->special_effect;

    std::ranges::sort(renderBatch,
                      [](const RenderInfo& a, const RenderInfo& b) {
                          return a.row < b.row;
                      });

    switch (prompt_alignment) {
        case Theme::PromptAlignment::kVertical:
            for (const auto& a_renderInfo : renderBatch) {
                PushStyleVar(ImGuiStyleVar_Alpha, a_renderInfo.alpha);
                ButtonIconWithCircularProgress(a_renderInfo.text.c_str(), a_renderInfo.text_color,
                                               a_renderInfo.texture, a_renderInfo.progress,
                                               a_renderInfo.button_state);
                PopStyleVar();
            }
            break;
        case Theme::PromptAlignment::kHorizontal:
            RenderPromptsHorizontalCentered(renderBatch, GetFontSize() * curr_theme->linespacing);
            break;
        case Theme::PromptAlignment::kRadial: {
            const float lineSpacingPx = GetFontSize() * curr_theme->linespacing;
            const float iconSize = GetIO().FontDefault->FontSize * curr_theme->icon2font_ratio;
            const float baseRadius = iconSize * 3;
            const auto a_center = GetWindowPos();

            RenderPromptsRadialRotated(a_center,
                                       renderBatch,
                                       lineSpacingPx, baseRadius, 0.0f); // ray to the right
            break;
        }
    }

    if (special_effect > 0) {
        const auto a_center = GetWindowPos();
        const auto a_size = GetIO().FontDefault->FontSize * curr_theme->icon2font_ratio;
        SkyPrompt::AddOns::RenderSpecialEffect(GetSpecialsView(*curr_theme), a_center, a_size,
                                               Renderer::GetResolutionScale());
    }
}
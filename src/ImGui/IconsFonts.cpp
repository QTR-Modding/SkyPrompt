#include "IconsFonts.h"
#include "Input.h"
#include "Renderer.h"
#include "imgui_internal.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <numbers>
#include "MCP.h"


namespace IconFont
{
	IconTexture::IconTexture(const std::wstring_view a_iconName) :
		ImGui::Texture(LR"(Data/Interface/ImGuiIcons/Icons/)", a_iconName)
	{}

	bool IconTexture::Load(const bool a_resizeToScreenRes)
	{
		const bool result = ImGui::Texture::Load(a_resizeToScreenRes);

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

	void Manager::LoadIcons()
	{
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

	void Manager::ReloadFonts()
	{
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		MCP::Settings::font_names.clear();

        for (const auto& entry : std::filesystem::directory_iterator(fontPath)) {
            if (entry.path().extension() == ".ttf") {
                MCP::Settings::font_names.insert(entry.path().filename().replace_extension("").string());
            }
        }

		if (MCP::Settings::font_names.empty()) {
			logger::error("No fonts found in {}", fontPath);
			return;
		}

		ImVector<ImWchar> ranges;

		ImFontGlyphRangesBuilder builder;
		builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
		builder.AddChar(0xf030);  // CAMERA
		builder.AddChar(0xf017);  // CLOCK
		builder.AddChar(0xf183);  // PERSON
		builder.AddChar(0xf042);  // CONTRAST
		builder.AddChar(0xf03e);  // IMAGE
		builder.BuildRanges(&ranges);

		const auto resolutionScale = ImGui::Renderer::GetResolutionScale();

		float a_fontsize;
		{
		    const auto& prompt_size = Theme::last_theme->prompt_size;
		    a_fontsize = prompt_size * resolutionScale;
		}
		//const auto a_iconsize = a_fontsize * 1.f;
		const auto a_largefontsize = a_fontsize * 1.2f;
		//const auto a_largeiconsize = a_largefontsize * 1.f;
		const auto a_smallfontsize = a_fontsize * 0.65f;

		io.FontDefault = LoadFontIconSet(a_fontsize, ranges);
		largeFont = LoadFontIconSet(a_largefontsize, ranges);
		smallFont = LoadFontIconSet(a_smallfontsize, ranges);

		io.Fonts->Build();

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	ImFont* Manager::LoadFontIconSet(const float a_fontSize, const ImVector<ImWchar>& a_ranges) const
	{
		const auto& io = ImGui::GetIO();

		const auto& font_name = Theme::last_theme->font_name;
		auto a_fontName = fontPath + (MCP::Settings::font_names.contains(font_name) ? font_name : *MCP::Settings::font_names.begin()) + ".ttf";
		const auto a_font = io.Fonts->AddFontFromFileTTF(a_fontName.c_str(), a_fontSize, nullptr, a_ranges.Data);
		if (!a_font) {
			logger::error("Failed to load font: {}", a_fontName);
			return nullptr;
		}

		return a_font;
	}

	ImFont* Manager::GetLargeFont() const
	{
		return largeFont;
	}

	ImFont* Manager::GetSmallFont() const {
		return smallFont;
    }

    const IconTexture* Manager::GetStepperLeft() const
	{
		return &stepperLeft;
	}
	const IconTexture* Manager::GetStepperRight() const
	{
		return &stepperRight;
	}

	const IconTexture* Manager::GetCheckbox() const
	{
		return &checkbox;
	}
	const IconTexture* Manager::GetCheckboxFilled() const
	{
		return &checkboxFilled;
	}

	const IconTexture* Manager::GetIcon(const std::uint32_t key)
	{
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
		default:
			{
				if (const auto inputDevice = MANAGER(Input)->GetInputDevice(); inputDevice == Input::DEVICE::kKeyboardMouse) {
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

	const IconTexture* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const
	{
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

    void DrawCircle(ImDrawList* drawList, const ImVec2 a_center, const float a_radius, const float progress, const float thickness,
        const std::optional<uint32_t> a_color = std::nullopt, const std::optional<float> start_angle = std::nullopt)
    {
        const auto startColor = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 60);
        const auto endColor = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 180);

        constexpr int numSegments = 64;
        const float startAngle = start_angle.has_value() ? start_angle.value() - IM_PI / 2 : -IM_PI / 2; // Start at the top
        const float endAngle = startAngle + progress * 2.0f * IM_PI;

        ImVec2 prevPoint = a_center + ImVec2(cosf(startAngle) * a_radius, sinf(startAngle) * a_radius);

		if (startColor != endColor) {
            for (int i = 1; i <= numSegments; ++i)
            {
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
		}
		else {
            drawList->PathArcTo(a_center, a_radius, startAngle, endAngle, numSegments);
            drawList->PathStroke(endColor, false, thickness);
		}
    }

	void DrawTriangle(ImDrawList* drawList, const ImVec2 iconCenter, const float outer_radius, const float inner_radius,const std::optional<uint32_t> a_color = std::nullopt) {
        const float triangle_width = inner_radius * 0.5f;
        const float triangle_height = inner_radius * 0.25f;

        const auto p1 = ImVec2(iconCenter.x, iconCenter.y - outer_radius + triangle_height); // Tip (bottom, closer to center)
        const auto p2 = ImVec2(iconCenter.x - triangle_width * 0.5f, iconCenter.y - outer_radius - triangle_height); // Top left
        const auto p3 = ImVec2(iconCenter.x + triangle_width * 0.5f, iconCenter.y - outer_radius - triangle_height); // Top right
		const auto color = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 200);
        drawList->AddTriangleFilled(p1, p2, p3, color);
    }

	void DrawProgressMark(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float outer_radius, const float a_thickness) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 30);
        DrawCircle(a_drawlist, iconCenter, outer_radius, 1.0, a_thickness, aColor);
    }

	void DrawHoldMark(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float outer_radius, const float inner_radius) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 180);
		DrawTriangle(a_drawlist, iconCenter, outer_radius, inner_radius*0.6f,aColor);
    }

	void DrawCross1(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
		constexpr auto a_red= IM_COL32(147, 39, 41, 180);
		const ImVec2 topRight = iconCenter + ImVec2(a_radius, -a_radius);
        const ImVec2 bottomLeft = iconCenter + ImVec2(-a_radius, a_radius);
	    a_drawlist->AddLine(topRight, bottomLeft, a_red, a_thickness);
    }

	void DrawCross2(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
		constexpr auto a_red= IM_COL32(147, 39, 41, 180);
	    const ImVec2 topLeft = iconCenter + ImVec2(-a_radius, -a_radius);
	    const ImVec2 bottomRight = iconCenter + ImVec2(a_radius, a_radius);
        a_drawlist->AddLine(topLeft, bottomRight, a_red, a_thickness);
    }

	void DrawSkipPrompt(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
        constexpr auto a_yellow = IM_COL32(228, 185, 76, 100);
		DrawCircle(a_drawlist, iconCenter, a_radius, 1.f, a_thickness, a_yellow);
    }

	void DrawDeleteAll(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness, const float progress) {
	    constexpr auto a_red= IM_COL32(147, 39, 41, 180);
		DrawCircle(a_drawlist, iconCenter, a_radius, progress, a_thickness, a_red);
    }

	void DrawProgressCircle(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness, const float progress, const float start_angle) {
		const auto aColor = progress + ImGui::Renderer::progress_circle_offset >= 1.f ? IM_COL32(228, 185, 76, 180) : IM_COL32(255, 255, 255, 180);
        DrawCircle(a_drawlist, iconCenter, a_radius, std::max(progress, 0.f), a_thickness, aColor, std::max(start_angle,0.f));
    }


	float GetIconSize() {
		const auto a_fontsize = ImGui::GetIO().FontDefault->FontSize;
        return a_fontsize * Theme::last_theme->icon2font_ratio;
    }

	ImVec2 GetIconSizeImVec() {
		const auto a_size = GetIconSize();
		return {a_size, a_size};
    }

	
    void AddTextWithShadow(ImDrawList* draw_list, ImFont* font, const float font_size, const ImVec2 position, const ImU32 text_color, const char* text)
    {
        if (!draw_list || !font || !text || !*text) return;

        const auto shadow_color = IM_COL32(0, 0, 0, 255 * Theme::last_theme->font_shadow);
        draw_list->AddText(font, font_size, position + ImVec2(2.5f, 2.5f), shadow_color, text);
        draw_list->AddText(font, font_size, position, text_color, text);
    }


	ImVec2 ButtonIconWithCircularProgress(const char* a_text, const uint32_t a_text_color,
                                             const IconFont::IconTexture* a_texture, const float progress,
                                             const float button_state)
    {
        if (!a_texture || !a_texture->srView.Get()) {
            logger::error("Button icon texture not loaded.");
		    return {};
        }

        // Calculate sizes
        const ImVec2 textSize = ImGui::CalcTextSize(a_text);

	    const auto a_iconsize = GetIconSize();
        const float circleDiameter = a_iconsize  * 1.25f;
        const float rowHeight = std::max(circleDiameter, textSize.y);

        // Record the "start" cursor Y.
        const float startY = ImGui::GetCursorPosY();

        // 1) Center the icon in this row
        const float iconOffset = (rowHeight - a_iconsize) * 0.5f;
        ImGui::SetCursorPosY(startY + iconOffset);

        // 2) Draw the icon (and get its final size)
        const auto iconSize = ButtonIcon(a_texture);  // This already calls ImGui::Image(...)

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
		    }
		    else if (progress > 0.f) {
			    DrawDeleteAll(a_drawlist, iconCenter, circle_radius, thickness, progress);
		    }
		    else {
		        DrawSkipPrompt(a_drawlist, iconCenter, circle_radius, thickness);
		    }
	    }

	    if (const bool SinglePress_progress = progress < 0.f; SinglePress_progress || button_state > 0.f) {
	        DrawProgressMark(a_drawlist, iconCenter, circle_radius,thickness);
		    if (!SinglePress_progress) {
		        DrawHoldMark(a_drawlist, iconCenter, circle_radius,radius);
		    }
	        if (button_state < 3.f) {
			    const auto start_deg = SinglePress_progress ? 360.f * (1 + progress) : ImGui::Renderer::progress_circle_offset_deg;
			    const auto a_progress = SinglePress_progress ? -progress : progress - ImGui::Renderer::progress_circle_offset;
		        DrawProgressCircle(a_drawlist, iconCenter, circle_radius, thickness, a_progress, RE::deg_to_rad(start_deg));
	        }
	    }


        // 4) Move horizontally for the text
        ImGui::SameLine();

        // 5) Center the text in this row
        const float textOffset = (rowHeight - textSize.y) * 0.5f;
        ImGui::SetCursorPosY(startY + textOffset);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + circle_radius - radius);

        AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(), 
                          ImGui::GetCursorScreenPos(), a_text_color ? a_text_color : IM_COL32(255, 255, 255, 255), a_text);

        // Move ImGui cursor manually to avoid overlap
        ImGui::Dummy(textSize);  // Moves cursor forward horizontally

	    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset * Theme::last_theme->linespacing*5);

	    return iconCenter;
    }

    void AddImageRotated(ImDrawList* dl, const ImTextureID tex,
                                const ImVec2 center, const ImVec2 size,
                                const float angle, const ImU32 col = IM_COL32_WHITE)
    {
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
        const ImVec2 uv0(0,0), uv1(1,0), uv2(1,1), uv3(0,1);
        dl->AddImageQuad(tex, p0, p1, p2, p3, uv0, uv1, uv2, uv3, col);
    }

    // helper: do NOT push/pop clip; caller controls clip once per frame
    void AddTextRotated(ImDrawList* dl, ImFont* font, const float font_size,
                        const ImVec2 pivot, const ImU32 col,
                        const char* text_begin, const char* text_end,
                        const float angle, const bool center_on_pivot = true)
    {
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
            const ImVec2 d = { p.x - pivot.x, p.y - pivot.y };
            dl->VtxBuffer[i].pos.x = pivot.x + d.x * c - d.y * s;
            dl->VtxBuffer[i].pos.y = pivot.y + d.x * s + d.y * c;
        }
    }

    void DrawTriangleRotated(ImDrawList* dl, const ImVec2 center,
                                       const float outer_radius, const float inner_radius,
                                       const float angle, const ImU32 col)
    {
        const float tri_w = inner_radius * 0.5f;
        const float tri_h = inner_radius * 0.25f;

        // local (unrotated) vertices – tip is “up” (toward -Y)
        const auto p1 = ImVec2(0.0f, -outer_radius + tri_h);   // tip (closer to center)
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
                                       const float startAngleRad)
    {
        if (batch.empty()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        ImFont*     font = ImGui::GetFont();
        const float fs   = ImGui::GetFontSize();

        dl->PushClipRectFullScreen();

        const float iconSz   = ImGui::GetIO().FontDefault->FontSize * Theme::last_theme->icon2font_ratio;
        const ImVec2 iconSzV = { iconSz, iconSz };

        const float circleDia = iconSz * 1.25f;
        const float outerR    = circleDia * 0.5f;            // ring outer radius (same as you use for rings)

        // Tangential footprint should be the ring's outer diameter, not iconSz.
        // Also mimic SameLine spacing a bit so neighboring rings don't kiss.
        const float arcSpacing = ImGui::GetStyle().ItemSpacing.x; // small extra gap (pixels)
        const float rowFootprint = circleDia;

        // Final angular step (radians per item)
        const float r     = std::max(baseRadius, iconSz * 1.6f);
        const float step = std::max(0.001f, (rowFootprint + arcSpacing + lineSpacingPx) / r);

        const int   n      = static_cast<int>(batch.size());
        const float firstA = startAngleRad - 0.5f * (n - 1) * step;          // center on the ray


        for (int i = 0; i < n; ++i) {
            const auto& ri = batch[i];
            const float a  = firstA + i * step;

            // position on ring
            const ImVec2 iconCenter{ center.x + r * cosf(a), center.y + r * sinf(a) };

            // orient along the radius; flip on left hemisphere to keep text upright
            float orient = a;
            if (cosf(a) < 0.0f) orient += IM_PI;

            // --- icon ---
            if (ri.texture && ri.texture->srView.Get()) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ri.alpha);
                AddImageRotated(dl, (ImTextureID)ri.texture->srView.Get(), iconCenter, iconSzV, orient);
                ImGui::PopStyleVar();
            }

            // --- rings/progress (unchanged) ---
            {
                const float thick     = outerR / 6.f;

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
                        constexpr ImU32 tri_col   = IM_COL32(255, 255, 255, 180);
                        DrawTriangleRotated(dl, iconCenter, outerR, iconSz * 0.5f, tri_angle, tri_col);
                    }
                    if (ri.button_state < 3.f) {
                        const float startDeg = singlePress ? 360.f * (1.f + ri.progress)
                                                           : ImGui::Renderer::progress_circle_offset_deg;
                        const float prog     = singlePress ? -ri.progress
                                                           : ri.progress - ImGui::Renderer::progress_circle_offset;
                        DrawProgressCircle(dl, iconCenter, outerR, thick, std::max(prog, 0.f),
                                           RE::deg_to_rad(startDeg));
                    }
                }
            }

            // --- text placement: exact flat-equivalent clearance ---
            const ImVec2 textSize = ImGui::CalcTextSize(ri.text.c_str());

            const float circle_radius = iconSz * 1.25f * 0.5f;

            // this is the piece you were missing vs. flat SameLine()
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
            const ImVec2 normal = { cosf(a), sinf(a) };
            const ImVec2 textCenter = {
                iconCenter.x + normal.x * center_dist,
                iconCenter.y + normal.y * center_dist
            };

            // draw rotated text centered on this pivot
            const ImU32 color  = ri.text_color ? ri.text_color : IM_COL32(255,255,255,255);
            const ImU32 shadow = IM_COL32(0, 0, 0, static_cast<int>(255 * Theme::last_theme->font_shadow * ri.alpha));

            AddTextRotated(dl, font, fs, {textCenter.x + 2.5f, textCenter.y + 2.5f}, shadow,
                           ri.text.c_str(), nullptr, orient, /*center_on_pivot=*/true);

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ri.alpha);
            AddTextRotated(dl, font, fs, textCenter, color,
                           ri.text.c_str(), nullptr, orient, /*center_on_pivot=*/true);
            ImGui::PopStyleVar();

        }

        dl->PopClipRect();
    }


}


ImVec2 ImGui::ButtonIcon(const IconFont::IconTexture* a_texture)
{
	const auto a_size = GetIconSizeImVec();
	ImGui::Image(reinterpret_cast<ImTextureID>(a_texture->srView.Get()), a_size);
	return a_size;
}

void ImGui::DrawCycleIndicators(SkyPromptAPI::ClientID curr_index, SkyPromptAPI::ClientID queue_size)
{
	auto* iconMgr  = MANAGER(IconFont);
    const auto* inputMgr = MANAGER(Input);
	const auto curr_device = inputMgr->GetInputDevice();
    const uint32_t keyL = MCP::Settings::cycle_L.at(curr_device);

    const uint32_t keyR = MCP::Settings::cycle_R.at(curr_device);

    const float iconSz = GetIconSize() * 0.6f;

	const float spacing = ImGui::GetFontSize() * 0.25f;
    ImGui::Dummy(ImVec2(0.0f, spacing));

    if (const auto* icoL = iconMgr->GetIcon(keyL))
        ImGui::Image((ImTextureID)icoL->srView.Get(), {iconSz, iconSz});

    ImGui::SameLine();

    if (const auto* icoR = iconMgr->GetIcon(keyR))
        ImGui::Image((ImTextureID)icoR->srView.Get(), {iconSz, iconSz});

    ImGui::SameLine();

	auto* smallFont = MANAGER(IconFont)->GetSmallFont();
	if (smallFont) {
        ImGui::PushFont(smallFont);
        const std::string text = std::format("({}/{})", curr_index, queue_size);
        const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

        const float textOffset = (iconSz - textSize.y) * 0.5f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset);

        ImGui::TextUnformatted(text.c_str());
		ImGui::PopFont();
    }
}

void ImGui::RenderSkyPrompt()
{
	const auto& curr_theme = Theme::last_theme;
	const auto prompt_alignment = curr_theme->prompt_alignment;
	const auto special_effects = curr_theme->special_effects;

	switch (prompt_alignment) {
	    case Theme::PromptAlignment::kVertical:
			for (const auto& a_renderInfo : renderBatch) {
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, a_renderInfo.alpha);
				ButtonIconWithCircularProgress(a_renderInfo.text.c_str(), a_renderInfo.text_color,
														 a_renderInfo.texture, a_renderInfo.progress,
														 a_renderInfo.button_state);
				ImGui::PopStyleVar();
			}
			break;
		case Theme::PromptAlignment::kHorizontal:
			break;
		case Theme::PromptAlignment::kRadial: {
            const float lineSpacingPx = ImGui::GetFontSize() * curr_theme->linespacing;
            const float iconSize      = ImGui::GetIO().FontDefault->FontSize * curr_theme->icon2font_ratio;
            const float baseRadius    = iconSize * 4;

            RenderPromptsRadialRotated(ImGui::renderBatchCenter-ImVec2(baseRadius,0),
                                              ImGui::renderBatch,
                                              lineSpacingPx, baseRadius,0.0f); // ray to the right
            break;
        }
	}

	switch (special_effects) {
	}
}



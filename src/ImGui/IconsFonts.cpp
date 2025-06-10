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
		const auto a_fontsize = MCP::Settings::prompt_size * resolutionScale;
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
		auto a_fontName = fontPath + (MCP::Settings::font_names.contains(MCP::Settings::font_name) ? MCP::Settings::font_name : *MCP::Settings::font_names.begin()) + ".ttf";
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
}


ImVec2 ImGui::ButtonIcon(const IconFont::IconTexture* a_texture)
{
	using namespace MCP::Settings;
	const ImVec2 a_size = {prompt_size*icon2font_ratio, prompt_size*icon2font_ratio};
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

    const float iconSz = MCP::Settings::prompt_size *
                         MCP::Settings::icon2font_ratio * 0.6f;

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

void ImGui::AddTextWithShadow(ImDrawList* draw_list, ImFont* font, const float font_size, const ImVec2 position, const ImU32 text_color, const char* text, const ImU32 shadow_color, const ImVec2 shadow_offset)
{
    if (!draw_list || !font || !text || !*text) return;
	draw_list->AddText(font, font_size, position + shadow_offset, shadow_color, text);
    draw_list->AddText(font, font_size, position, text_color, text);
}

ImVec2 ImGui::ButtonIconWithCircularProgress(const char* a_text, uint32_t a_text_color, const IconFont::IconTexture* a_texture, const float progress, const float button_state)
{
    if (!a_texture || !a_texture->srView.Get()) {
        logger::error("Button icon texture not loaded.");
		return {};
    }

    // Calculate sizes
    const ImVec2 textSize = ImGui::CalcTextSize(a_text);

    const float circleDiameter = MCP::Settings::prompt_size * MCP::Settings::icon2font_ratio * 1.25f;
    const float rowHeight = std::max(circleDiameter, textSize.y);

    // Record the "start" cursor Y.
    const float startY = ImGui::GetCursorPosY();

    // 1) Center the icon in this row
    const float iconOffset = (rowHeight - (MCP::Settings::prompt_size * MCP::Settings::icon2font_ratio)) * 0.5f;
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
	const float circle_radius = circleDiameter / 2.f;

	const auto a_drawlist = GetWindowDrawList();

	constexpr auto a_yellow = IM_COL32(228, 185, 76, 100);
	constexpr auto a_red= IM_COL32(147, 39, 41, 180);
	const auto outer_color = button_state > 3.f && progress > 0.f ? a_red : a_yellow;

    if (button_state > 2.f) {
		const auto a_radius = circle_radius * 0.6f;
		const ImVec2 topLeft = iconCenter + ImVec2(-a_radius, -a_radius);
		const ImVec2 bottomRight = iconCenter + ImVec2(a_radius, a_radius);
		if (MCP::Settings::SpecialCommands::visualize && button_state < 3.f) {
            a_drawlist->AddLine(topLeft, bottomRight, a_red, radius / 6.f);
		}
	}
	if (button_state > 1.f) {
		const auto a_radius = circle_radius * 0.6f;
		const ImVec2 topRight = iconCenter + ImVec2(a_radius, -a_radius);
        const ImVec2 bottomLeft = iconCenter + ImVec2(-a_radius, a_radius);
		if (MCP::Settings::SpecialCommands::visualize && button_state < 3.f) {
	        a_drawlist->AddLine(topRight, bottomLeft, a_red, radius / 6.f);
		}
	}
	if (button_state > 3.f) {
        if (MCP::Settings::SpecialCommands::visualize) {
			DrawCircle(a_drawlist, iconCenter, circle_radius, progress > 0.f ? progress : 1.f, radius / 6.f,outer_color);
        }
	}
	else if (button_state > 0.f) {
        const auto aColor = progress >= 1.f ? IM_COL32(228, 185, 76, 180) : IM_COL32(255, 255, 255, 180);
        DrawCircle(a_drawlist, iconCenter, circle_radius, std::max(progress-1.f/12.f,0.f), radius / 6.f,aColor,RE::deg_to_rad(15));
	}

	if (button_state > 0.f) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 30);
        constexpr auto aColor2 = IM_COL32(255, 255, 255, 180);
        DrawCircle(a_drawlist, iconCenter, circle_radius, 1.0, radius / 6.f, aColor);
		DrawTriangle(a_drawlist, iconCenter, circle_radius, radius*0.6f,aColor2);
	}

    // 4) Move horizontally for the text
    ImGui::SameLine();

    // 5) Center the text in this row
    const float textOffset = (rowHeight - textSize.y) * 0.5f;
    ImGui::SetCursorPosY(startY + textOffset);
    ImGui::SetCursorPosX(GetCursorPosX() + circle_radius - radius);

    const ImVec2 textScreenPos = ImGui::GetCursorScreenPos();

	const auto text_color = a_text_color ? a_text_color : IM_COL32(255, 255, 255, 255);
    AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(), 
        textScreenPos, text_color, a_text);

    // Move ImGui cursor manually to avoid overlap
    ImGui::Dummy(textSize);  // Moves cursor forward horizontally


	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset * MCP::Settings::linespacing*5);

	return iconCenter;
}



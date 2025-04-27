#include "Styles.h"
#include "IconsFonts.h"
#include "MCP.h"
#include "Renderer.h"

namespace ImGui
{
	void Styles::ConvertVec4StylesToU32()
	{
		frameBG_WidgetU32 = ColorConvertFloat4ToU32(user.frameBG_Widget);
		frameBG_WidgetActiveU32 = ColorConvertFloat4ToU32(user.frameBG_WidgetActive);
		gridLinesU32 = ColorConvertFloat4ToU32(user.gridLines);
		sliderBorderU32 = ColorConvertFloat4ToU32(user.sliderBorder);
		sliderBorderActiveU32 = ColorConvertFloat4ToU32(user.sliderBorderActive);
		iconDisabledU32 = ColorConvertFloat4ToU32(user.iconDisabled);
	}

	ImU32 Styles::GetColorU32(const USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kIconDisabled:
			return iconDisabledU32;
		case USER_STYLE::kGridLines:
			return gridLinesU32;
		case USER_STYLE::kSliderBorder:
			return sliderBorderU32;
		case USER_STYLE::kSliderBorderActive:
			return sliderBorderActiveU32;
		case USER_STYLE::kFrameBG_Widget:
			return frameBG_WidgetU32;
		case USER_STYLE::kFrameBG_WidgetActive:
			return frameBG_WidgetActiveU32;
		default:
			return ImU32();
		}
	}

	ImVec4 Styles::GetColorVec4(const USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kIconDisabled:
			return user.iconDisabled;
		case USER_STYLE::kComboBoxTextBox:
			return user.comboBoxTextBox;
		case USER_STYLE::kComboBoxText:
			return user.comboBoxText;
		default:
			return ImVec4();
		}
	}

	float Styles::GetVar(const USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kButtons:
			return user.buttonScale;
		case USER_STYLE::kCheckbox:
			return user.checkboxScale;
		case USER_STYLE::kStepper:
			return user.stepperScale;
		case USER_STYLE::kSeparatorThickness:
			return user.separatorThickness;
		case USER_STYLE::kGridLines:
			return user.gridThickness;
		default:
			return 1.0f;
		}
	}

	void Styles::OnStyleRefresh() const {
		if (!MCP::refreshStyle.exchange(false)) {
			return;
		}
		logger::info("Refreshing ImGui style");
		//LoadStyles();

		ImGuiStyle style;
		auto&      colors = style.Colors;

		style.WindowBorderSize = user.borderSize;
		style.TabBarBorderSize = user.borderSize;
		style.TabRounding = 0.0f;

		colors[ImGuiCol_WindowBg] = user.background;
		colors[ImGuiCol_ChildBg] = user.background;

		colors[ImGuiCol_Border] = user.border;
		colors[ImGuiCol_Separator] = user.separator;

		colors[ImGuiCol_Text] = user.text;
		colors[ImGuiCol_TextDisabled] = user.textDisabled;

		colors[ImGuiCol_FrameBg] = user.frameBG;
		colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_FrameBg];
		colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBg];

		colors[ImGuiCol_SliderGrab] = user.sliderGrab;
		colors[ImGuiCol_SliderGrabActive] = user.sliderGrabActive;

		colors[ImGuiCol_Button] = user.button;
		colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_Button];
		colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_Button];

		colors[ImGuiCol_Header] = user.header;
		colors[ImGuiCol_HeaderHovered] = colors[ImGuiCol_Header];
		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_Header];

		colors[ImGuiCol_Tab] = user.tab;
		colors[ImGuiCol_TabHovered] = user.tabHovered;
		colors[ImGuiCol_TabActive] = colors[ImGuiCol_TabHovered];
		colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		colors[ImGuiCol_NavCursor] = ImVec4();

		style.ScaleAllSizes(Renderer::GetResolutionScale());
		ImGui::GetStyle() = style;

		MANAGER(IconFont)->ReloadFonts();
	}

	void Styles::RefreshStyle()
	{
        MCP::refreshStyle.store(true);
	}

	ImU32 GetUserStyleColorU32(const USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetColorU32(a_style);
	}

	ImVec4 GetUserStyleColorVec4(const USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetColorVec4(a_style);
	}

	float GetUserStyleVar(const USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetVar(a_style);
	}
}

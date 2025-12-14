#include "Styles.h"
#include "IconsFonts.h"
#include "MCP.h"
#include "Renderer.h"

namespace ImGui {
    void Styles::OnStyleRefresh() const {
        if (!MCP::refreshStyle.exchange(false)) {
            return;
        }

        ImGuiStyle style;
        auto& colors = style.Colors;

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
        GetStyle() = style;

        static bool reloadAttempted = false;
        if (!MANAGER(IconFont)->ReloadFonts()) {
            if (!reloadAttempted) {
                reloadAttempted = true;
                Theme::ReLoadDefaultTheme();
            }
            // else: already attempted reload, do not retry to avoid infinite loop
        } else {
            reloadAttempted = false; // reset on success
        }
    }

    void Styles::RefreshStyle() {
        MCP::refreshStyle.store(true);
    }
}
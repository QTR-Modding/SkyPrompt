#include "VR.h"
#include "Styles.h"
#include "ImGuiVRHelperClientSDK.h"

namespace ImGui::VR {
    namespace {
        struct HUD {
            ImGuiVRHelperPluginAPI::Client helper;
            float aspectRatio = 0.0f;
        } hud;

        void UpdateAspectRatio() {
            if (hud.aspectRatio > 0.0f) return;
            const auto openVR = RE::BSOpenVR::GetSingleton();
            if (!openVR || !openVR->vrSystem) return;

            float halfWidth = 0.0f;
            float above = 0.0f;
            float below = 0.0f;
            for (const auto eye : {vr::Eye_Left, vr::Eye_Right}) {
                float left, right, top, bottom;
                openVR->vrSystem->GetProjectionRaw(eye, &left, &right, &top, &bottom);
                halfWidth = std::max({halfWidth, -left, right});
                above = std::max(above, -top);
                below = std::max(below, bottom);
            }
            // The helper's head-centred HUD spans the widest half-FOV of either eye.
            if (above + below > 0.0f) {
                hud.aspectRatio = 2.0f * halfWidth / (above + below);
            }
        }

        void ScaleDrawData(ImDrawData& data) {
            const auto scale = data.FramebufferScale;
            if (scale == ImVec2(1.0f, 1.0f)) return;

            // Our DX11 backend predates framebuffer scaling support.
            for (const auto commands : data.CmdLists) {
                for (auto& vertex : commands->VtxBuffer) {
                    vertex.pos = vertex.pos * scale;
                }
            }
            data.ScaleClipRects(scale);
            data.DisplayPos = data.DisplayPos * scale;
            data.DisplaySize = data.DisplaySize * scale;
            data.FramebufferScale = ImVec2(1.0f, 1.0f);
        }
    }

    void Connect() {
        if (!REL::Module::IsVR()) return;

        const auto plugin = SKSE::PluginDeclaration::GetSingleton();
        if (hud.helper.Connect(plugin->GetName().data(), plugin->GetVersion().string(".").c_str(),
                               ImGuiVRHelperPluginAPI::kClientFlag_HUDMode)) {
            logger::info("ImGui VR Helper connected for prompt rendering.");
        } else {
            logger::warn("ImGui VR Helper is unavailable; prompts will only appear on the desktop.");
        }
    }

    void PrepareFrame(const ImVec2& previousDisplaySize) {
        if (!hud.helper.ApplyPanelDisplaySize()) return;
        UpdateAspectRatio();
        auto& io = GetIO();
        if (hud.aspectRatio > 0.0f) {
            const auto panelWidth = io.DisplaySize.x;
            io.DisplaySize.x = io.DisplaySize.y * hud.aspectRatio;
            io.DisplayFramebufferScale.x = panelWidth / io.DisplaySize.x;
        }
        if (previousDisplaySize != io.DisplaySize) {
            Styles::RefreshStyle();
        }
    }

    void Render(ID3D11DeviceContext* context, const bool draw) {
        if (draw) {
            if (hud.helper.IsConnected()) ScaleDrawData(*GetDrawData());
            hud.helper.RenderFrame(context);
            return;
        }
        if (!hud.helper.IsConnected() || !context) return;

        ImGuiVRHelperPluginAPI::PanelHandle panel{};
        if (hud.helper.Helper()->GetPanel(hud.helper.Id(), &panel) && panel.rtv) {
            constexpr float transparent[] = {0.0f, 0.0f, 0.0f, 0.0f};
            context->ClearRenderTargetView(panel.rtv, transparent);
        }
    }
}

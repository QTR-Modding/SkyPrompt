#include "VR.h"
#include "Styles.h"
#include "ImGuiVRHelperClientSDK.h"

namespace ImGui::VR {
    namespace {
        ImGuiVRHelperPluginAPI::Client helper;
    }

    void Connect() {
        if (!REL::Module::IsVR()) return;

        const auto plugin = SKSE::PluginDeclaration::GetSingleton();
        if (helper.Connect(plugin->GetName().data(), plugin->GetVersion().string(".").c_str(),
                           ImGuiVRHelperPluginAPI::kClientFlag_HUDMode)) {
            logger::info("ImGui VR Helper connected for prompt rendering.");
        } else {
            logger::warn("ImGui VR Helper is unavailable; prompts will only appear on the desktop.");
        }
    }

    void PrepareFrame(const ImVec2& previousDisplaySize) {
        if (helper.ApplyPanelDisplaySize() && previousDisplaySize != GetIO().DisplaySize) {
            Styles::RefreshStyle();
        }
    }

    void Render(ID3D11DeviceContext* context, const bool draw) {
        if (draw) {
            helper.RenderFrame(context);
            return;
        }
        if (!helper.IsConnected() || !context) return;

        ImGuiVRHelperPluginAPI::PanelHandle panel{};
        if (helper.Helper()->GetPanel(helper.Id(), &panel) && panel.rtv) {
            constexpr float transparent[] = {0.0f, 0.0f, 0.0f, 0.0f};
            context->ClearRenderTargetView(panel.rtv, transparent);
        }
    }
}

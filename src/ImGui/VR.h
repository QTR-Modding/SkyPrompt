#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"

namespace ImGui::VR {
    struct WorldAnchor {
        RE::NiPoint3 position;
        ImVec2 origin;
    };

    void Connect();
    void PrepareFrame(const ImVec2& previousDisplaySize);
    void Render(ID3D11DeviceContext* context, bool draw);
    std::optional<WorldAnchor> GetWorldAnchor(const RE::NiPoint3& position);
    const std::optional<WorldAnchor>& GetCurrentWorldAnchor();
    void BeginWorldPrompt(const WorldAnchor& anchor, ImDrawList* window = nullptr);
    void EndWorldPrompt();
}

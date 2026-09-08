#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"

namespace ImGui::VR {
    void Connect();
    void PrepareFrame(const ImVec2& previousDisplaySize);
    void Render(ID3D11DeviceContext* context, bool draw);
}

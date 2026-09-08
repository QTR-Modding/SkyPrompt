#pragma once
#include "IconsFonts.h"
#include "VR.h"

namespace ImGui {
    class ActivationPop {
        struct Visual {
            Interaction interaction;
            std::string text;
            ComPtr<ID3D11ShaderResourceView> texture;
            ImVec2 iconCenter;
            ImVec2 textCenter;
            ImVec2 textSize;
            float iconSize;
            float fontSize;
            float angle;
            float alpha;
            float shadow;
            float duration;
            float endScale;
            float opacity;
            ImU32 color;
            int frame;
            std::optional<VR::WorldAnchor> worldAnchor;
        };

        struct Copy {
            Visual visual;
            std::chrono::steady_clock::time_point start;
        };

        std::mutex mutex;
        std::atomic<bool> enabled = false;
        std::vector<Visual> visible;
        std::vector<Copy> copies;

        static void DrawCopy(const Visual& visual, float progress);

    public:
        void Capture(const RenderInfo& info, ImVec2 iconCenter, ImVec2 textCenter,
                     ImVec2 textSize, float angle = 0.0f, bool showIcon = true);
        void Trigger(const Interaction& interaction);
        void Draw();
        void Clear();
    };
}

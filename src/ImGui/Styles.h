#pragma once
#include <REX/REX/Singleton.h>
#include "imgui.h"

namespace ImGui {
    class Styles : public REX::Singleton<Styles> {
    public:
        void OnStyleRefresh() const;
        static void RefreshStyle();

    private:
        struct Style {
            float bgAlpha{0.68f};
            float disabledAlpha{0.30f};

            ImVec4 background{0.0f, 0.0f, 0.0f, bgAlpha};

            ImVec4 border{0.569f, 0.545f, 0.506f, bgAlpha};
            float borderSize{3.5f};

            ImVec4 text{1.0f, 1.0f, 1.0f, 1.0f};
            ImVec4 textDisabled{1.0f, 1.0f, 1.0f, disabledAlpha};

            ImVec4 button{0.0f, 0.0f, 0.0f, bgAlpha}; // arrow button

            ImVec4 frameBG{0.2f, 0.2f, 0.2f, bgAlpha};

            ImVec4 sliderGrab{1.0f, 1.0f, 1.0f, 0.245f};
            ImVec4 sliderGrabActive{1.0f, 1.0f, 1.0f, 0.531f};

            ImVec4 header{1.0f, 1.0f, 1.0f, 0.1f}; // select highlight
            ImVec4 tab{0.0f, 0.0f, 0.0f, 0.0f};
            ImVec4 tabHovered{0.2f, 0.2f, 0.2f, 1.0f};

            ImVec4 separator{0.569f, 0.545f, 0.506f, bgAlpha};
        };

        Style user;
    };
}

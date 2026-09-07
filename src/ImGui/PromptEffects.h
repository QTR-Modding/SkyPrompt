#pragma once
#include "Theme.h"

namespace ImGui::PromptEffects {
    enum EffectID : uint32_t {
        kProgressCircle = 3,
        kListIndicators = 4,
        kActivationPop = 5
    };

    namespace Progress {
        enum FloatIndex : size_t { kRadius, kThickness, kOffsetX, kOffsetY, kRotation, kHoldMarkerSize };
        enum ColorIndex : size_t { kArc, kCompleted, kTrack, kHold, kRemoval, kSkip };
        enum BoolIndex : size_t { kShowArc, kShowTrack, kShowHold, kShowSpecialCommands, kClockwise };
    }
    namespace ListIndicators {
        enum FloatIndex : size_t { kSize, kOffsetX, kOffsetY, kSpacing };
        enum ColorIndex : size_t { kUp, kDown };
        enum BoolIndex : size_t { kShow };
    }
    namespace Pop {
        enum FloatIndex : size_t { kDuration, kEndScale, kOpacity };
    }

    struct FloatParameter {
        const char* label;
        const char* help;
        float default_value;
        float min;
        float max;
    };
    struct ColorParameter {
        const char* label;
        const char* help;
        uint32_t default_value;
    };
    struct BoolParameter {
        const char* label;
        const char* help;
        bool default_value;
    };
    struct Definition {
        uint32_t id;
        const char* label;
        std::span<const FloatParameter> floats;
        std::span<const ColorParameter> colors;
        std::span<const BoolParameter> bools;
    };

    const Definition* GetDefinition(uint32_t a_id);
    const Theme::SpecialEffect* Find(EffectID a_id);
    float Float(const Theme::SpecialEffect* a_effect, EffectID a_id, size_t a_index);
    uint32_t Color(const Theme::SpecialEffect* a_effect, EffectID a_id, size_t a_index);
    bool Bool(const Theme::SpecialEffect* a_effect, EffectID a_id, size_t a_index);
}

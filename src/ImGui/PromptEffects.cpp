#include "PromptEffects.h"
#include "imgui.h"
#include "SkyPrompt/AddOns.hpp"

namespace ImGui::PromptEffects {
    namespace {
        constexpr FloatParameter progressFloats[] = {
            {"$SkyPromptMCPEffectRadius", "$SkyPromptMCPEffectRadiusHelp", 1.0f, 0.0f, 4.0f},
            {"$SkyPromptMCPEffectThickness", "$SkyPromptMCPEffectThicknessHelp", 1.0f, 0.0f, 8.0f},
            {"$SkyPromptMCPEffectOffsetX", "$SkyPromptMCPEffectOffsetXHelp", 0.0f, -500.0f, 500.0f},
            {"$SkyPromptMCPEffectOffsetY", "$SkyPromptMCPEffectOffsetYHelp", 0.0f, -500.0f, 500.0f},
            {"$SkyPromptMCPEffectRotation", "$SkyPromptMCPEffectRotationHelp", 0.0f, -360.0f, 360.0f},
            {"$SkyPromptMCPEffectHoldSize", "$SkyPromptMCPEffectHoldSizeHelp", 1.0f, 0.0f, 4.0f}
        };
        constexpr ColorParameter progressColors[] = {
            {"$SkyPromptMCPEffectArcColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(255, 255, 255, 180)},
            {"$SkyPromptMCPEffectCompletedColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(228, 185, 76, 180)},
            {"$SkyPromptMCPEffectTrackColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(255, 255, 255, 30)},
            {"$SkyPromptMCPEffectHoldColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(255, 255, 255, 180)},
            {"$SkyPromptMCPEffectRemovalColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(147, 39, 41, 180)},
            {"$SkyPromptMCPEffectSkipColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(228, 185, 76, 100)}
        };
        constexpr BoolParameter progressBools[] = {
            {"$SkyPromptMCPEffectShowArc", "$SkyPromptMCPEffectShowArcHelp", true},
            {"$SkyPromptMCPEffectShowTrack", "$SkyPromptMCPEffectShowTrackHelp", true},
            {"$SkyPromptMCPEffectShowHold", "$SkyPromptMCPEffectShowHoldHelp", true},
            {"$SkyPromptMCPEffectShowCommands", "$SkyPromptMCPEffectShowCommandsHelp", true},
            {"$SkyPromptMCPEffectClockwise", "$SkyPromptMCPEffectClockwiseHelp", true}
        };
        constexpr FloatParameter listFloats[] = {
            {"$SkyPromptMCPEffectIndicatorSize", "$SkyPromptMCPEffectIndicatorSizeHelp", 1.0f, 0.0f, 4.0f},
            {"$SkyPromptMCPEffectOffsetX", "$SkyPromptMCPEffectOffsetXHelp", 0.0f, -500.0f, 500.0f},
            {"$SkyPromptMCPEffectOffsetY", "$SkyPromptMCPEffectOffsetYHelp", 0.0f, -500.0f, 500.0f},
            {"$SkyPromptMCPEffectIndicatorSpacing", "$SkyPromptMCPEffectIndicatorSpacingHelp", 0.0f, 0.0f, 500.0f}
        };
        constexpr ColorParameter listColors[] = {
            {"$SkyPromptMCPEffectUpColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(255, 255, 255, 255)},
            {"$SkyPromptMCPEffectDownColor", "$SkyPromptMCPEffectColorHelp", IM_COL32(255, 255, 255, 255)}
        };
        constexpr BoolParameter listBools[] = {
            {"$SkyPromptMCPEffectShowIndicators", "$SkyPromptMCPEffectShowIndicatorsHelp", true}
        };
        constexpr FloatParameter popFloats[] = {
            {"$SkyPromptMCPEffectPopDuration", "$SkyPromptMCPEffectPopDurationHelp", 0.3f, 0.01f, 3.0f},
            {"$SkyPromptMCPEffectPopScale", "$SkyPromptMCPEffectPopScaleHelp", 1.25f, 1.0f, 3.0f},
            {"$SkyPromptMCPEffectPopOpacity", "$SkyPromptMCPEffectPopOpacityHelp", 0.6f, 0.0f, 1.0f}
        };
        constexpr BoolParameter popBools[] = {
            {"$SkyPromptMCPEffectPopIconOnly", "$SkyPromptMCPEffectPopIconOnlyHelp", false}
        };
        constexpr Definition progress{kProgressCircle, "$SkyPromptMCPThemeEffectProgressCircle", progressFloats, progressColors, progressBools};
        constexpr Definition list{kListIndicators, "$SkyPromptMCPThemeEffectListIndicators", listFloats, listColors, listBools};
        constexpr Definition pop{kActivationPop, "$SkyPromptMCPThemeEffectActivationPop", popFloats, {}, popBools};
        using SkyPrompt::AddOns::SpecialEffects::Separator;
        using SkyPrompt::AddOns::SpecialEffects::Frame;
        constexpr FloatParameter separatorFloats[] = {
            {"$SkyPromptMCPEffectLineThickness", "$SkyPromptMCPEffectLineThicknessHelp", Separator::defaultThickness, 0.0f, Separator::maxThickness},
            {"$SkyPromptMCPEffectSeparatorSpacing", "$SkyPromptMCPEffectSeparatorSpacingHelp", Separator::defaultSpacing, 0.0f, Separator::maxSpacing},
            {"$SkyPromptMCPEffectSeparatorExtension", "$SkyPromptMCPEffectSeparatorExtensionHelp", Separator::defaultExtension, Separator::minExtension, Separator::maxExtension},
            {"$SkyPromptMCPThemeBackgroundRadius", "$SkyPromptMCPEffectSeparatorRadiusHelp", Separator::defaultRounding, 0.0f, Separator::maxRounding},
            {"$SkyPromptMCPEffectGradientSpeed", "$SkyPromptMCPEffectGradientSpeedHelp", SkyPrompt::AddOns::SpecialEffects::defaultGradientSpeed, -SkyPrompt::AddOns::SpecialEffects::maxGradientSpeed, SkyPrompt::AddOns::SpecialEffects::maxGradientSpeed}
        };
        constexpr ColorParameter separatorColors[] = {
            {"$SkyPromptMCPEffectLineColor", "$SkyPromptMCPEffectGradientHelp", Separator::defaultColor},
            {"$SkyPromptMCPEffectColor2", "$SkyPromptMCPEffectGradientHelp", Separator::defaultColor},
            {"$SkyPromptMCPEffectColor3", "$SkyPromptMCPEffectGradientHelp", Separator::defaultColor}
        };
        constexpr FloatParameter frameFloats[] = {
            {"$SkyPromptMCPThemeBackgroundPaddingX", "$SkyPromptMCPEffectFramePaddingHelp", Frame::defaultPaddingX, Frame::minPadding, Frame::maxPadding},
            {"$SkyPromptMCPThemeBackgroundPaddingY", "$SkyPromptMCPEffectFramePaddingHelp", Frame::defaultPaddingY, Frame::minPadding, Frame::maxPadding},
            {"$SkyPromptMCPThemeBackgroundRadius", "$SkyPromptMCPThemeBackgroundRadiusHelp", Frame::defaultRounding, 0.0f, Frame::maxRounding},
            {"$SkyPromptMCPEffectLineThickness", "$SkyPromptMCPEffectLineThicknessHelp", Frame::defaultThickness, 0.0f, Frame::maxThickness},
            {"$SkyPromptMCPEffectGradientSpeed", "$SkyPromptMCPEffectGradientSpeedHelp", SkyPrompt::AddOns::SpecialEffects::defaultGradientSpeed, -SkyPrompt::AddOns::SpecialEffects::maxGradientSpeed, SkyPrompt::AddOns::SpecialEffects::maxGradientSpeed}
        };
        constexpr ColorParameter frameColors[] = {
            {"$SkyPromptMCPEffectBorderColor", "$SkyPromptMCPEffectGradientHelp", Frame::defaultColor},
            {"$SkyPromptMCPEffectColor2", "$SkyPromptMCPEffectGradientHelp", Frame::defaultColor},
            {"$SkyPromptMCPEffectColor3", "$SkyPromptMCPEffectGradientHelp", Frame::defaultColor}
        };
        constexpr Definition separator{SkyPrompt::AddOns::SpecialEffects::kSeparator, "$SkyPromptMCPThemeEffectSeparator", separatorFloats, separatorColors, {}};
        constexpr Definition frame{SkyPrompt::AddOns::SpecialEffects::kFrame, "$SkyPromptMCPThemeEffectFrame", frameFloats, frameColors, {}};
    }

    const Definition* GetDefinition(const uint32_t a_id) {
        switch (a_id) {
            case kProgressCircle: return &progress;
            case kListIndicators: return &list;
            case kActivationPop: return &pop;
            case SkyPrompt::AddOns::SpecialEffects::kSeparator: return &separator;
            case SkyPrompt::AddOns::SpecialEffects::kFrame: return &frame;
            default: return nullptr;
        }
    }

    const Theme::SpecialEffect* Find(const uint32_t a_id) {
        const auto& effects = Theme::last_theme->special_effects;
        const auto effect = std::ranges::find(effects, a_id, &Theme::SpecialEffect::id);
        return effect != effects.end() ? &*effect : nullptr;
    }

    float Float(const Theme::SpecialEffect* a_effect, const EffectID a_id, const size_t a_index) {
        const auto& parameter = GetDefinition(a_id)->floats[a_index];
        const float value = a_effect && a_index < a_effect->floats.size()
                                ? a_effect->floats[a_index] : parameter.default_value;
        return std::isfinite(value) ? std::clamp(value, parameter.min, parameter.max) : parameter.default_value;
    }

    uint32_t Color(const Theme::SpecialEffect* a_effect, const EffectID a_id, const size_t a_index) {
        return a_effect && a_index < a_effect->integers.size()
                   ? a_effect->integers[a_index] : GetDefinition(a_id)->colors[a_index].default_value;
    }

    bool Bool(const Theme::SpecialEffect* a_effect, const EffectID a_id, const size_t a_index) {
        return a_effect && a_index < a_effect->bools.size()
                   ? a_effect->bools[a_index] != 0 : GetDefinition(a_id)->bools[a_index].default_value;
    }
}

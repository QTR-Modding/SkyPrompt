#include "ActivationPop.h"
#include "PromptEffects.h"
#include "PromptLayouts.h"
#include "imgui_internal.h"

namespace ImGui {
    void ActivationPop::Capture(const RenderInfo& info, const ImVec2 iconCenter, const ImVec2 textCenter,
                                 const ImVec2 textSize, const float angle, const bool showIcon) {
        using namespace PromptEffects;
        const auto* effect = Find(kActivationPop);
        if (!effect || info.alpha <= 0.0f) return;
        const float opacity = Float(effect, kActivationPop, Pop::kOpacity);
        if (opacity <= 0.0f) return;

        std::lock_guard lock(mutex);
        auto found = std::ranges::find(visible, info.interaction, &Visual::interaction);
        if (found == visible.end()) {
            visible.emplace_back();
            found = std::prev(visible.end());
        }
        auto& visual = *found;
        visual.interaction = info.interaction;
        if (Bool(effect, kActivationPop, Pop::kIconOnly)) visual.text.clear();
        else visual.text = info.text;
        auto* texture = showIcon && info.texture ? info.texture->srView.Get() : nullptr;
        if (visual.texture.Get() != texture) visual.texture = texture;
        visual.iconCenter = iconCenter;
        visual.textCenter = textCenter;
        visual.textSize = textSize;
        visual.iconSize = PromptLayouts::GetIconSize();
        visual.fontSize = GetFontSize();
        visual.angle = angle;
        visual.alpha = info.alpha;
        visual.shadow = Theme::last_theme->font_shadow;
        visual.duration = Float(effect, kActivationPop, Pop::kDuration);
        visual.endScale = Float(effect, kActivationPop, Pop::kEndScale);
        visual.opacity = opacity;
        visual.color = info.text_color ? info.text_color : IM_COL32_WHITE;
        visual.frame = GetFrameCount();
        visual.worldAnchor = VR::GetCurrentWorldAnchor();
        enabled.store(true);
    }

    void ActivationPop::Trigger(const Interaction& interaction) {
        if (!enabled.load()) return;
        std::lock_guard lock(mutex);
        if (const auto found = std::ranges::find(visible, interaction, &Visual::interaction);
            found != visible.end()) {
            copies.push_back({*found, std::chrono::steady_clock::now()});
        }
    }

    void ActivationPop::DrawCopy(const Visual& visual, const float progress) {
        auto* drawList = GetForegroundDrawList();
        auto* font = GetFont();
        const float alpha = visual.alpha * visual.opacity * (1.0f - progress);
        const auto color = [alpha](const ImU32 value) {
            auto rgba = ColorConvertU32ToFloat4(value);
            rgba.w *= alpha;
            return ColorConvertFloat4ToU32(rgba);
        };
        const float c = cosf(visual.angle), s = sinf(visual.angle);
        const auto firstVertex = drawList->VtxBuffer.Size;
        const ImVec2 iconHalfSize{visual.iconSize * 0.5f, visual.iconSize * 0.5f};
        if (visual.texture) {
            drawList->AddImage(reinterpret_cast<ImTextureID>(visual.texture.Get()),
                               visual.iconCenter - iconHalfSize, visual.iconCenter + iconHalfSize,
                               {0.0f, 0.0f}, {1.0f, 1.0f}, color(IM_COL32_WHITE));
            ShadeVertsTransformPos(drawList, firstVertex, drawList->VtxBuffer.Size,
                                   visual.iconCenter, c, s, visual.iconCenter);
        }

        ImVec2 center = visual.iconCenter;
        if (!visual.text.empty()) {
            const auto firstTextVertex = drawList->VtxBuffer.Size;
            const auto position = visual.textCenter - visual.textSize * 0.5f;
            constexpr ImVec2 shadowOffset{2.5f, 2.5f};
            drawList->AddText(font, visual.fontSize, position + shadowOffset,
                              color(IM_COL32(0, 0, 0, 255 * visual.shadow)), visual.text.c_str());
            drawList->AddText(font, visual.fontSize, position, color(visual.color), visual.text.c_str());
            ShadeVertsTransformPos(drawList, firstTextVertex, drawList->VtxBuffer.Size,
                                   visual.textCenter, c, s, visual.textCenter);

            center = visual.textCenter;
            if (visual.texture) {
                const auto offset = visual.iconCenter - visual.textCenter;
                const ImVec2 localIcon{offset.x * c + offset.y * s, -offset.x * s + offset.y * c};
                const auto minimum = ImMin(visual.textSize * -0.5f, localIcon - iconHalfSize);
                const auto maximum = ImMax(visual.textSize * 0.5f, localIcon + iconHalfSize);
                const auto localCenter = (minimum + maximum) * 0.5f;
                center += ImVec2(localCenter.x * c - localCenter.y * s, localCenter.x * s + localCenter.y * c);
            }
        }
        const float scale = 1.0f + (visual.endScale - 1.0f) * progress;
        ShadeVertsTransformPos(drawList, firstVertex, drawList->VtxBuffer.Size, center, scale, 0.0f, center);
    }

    void ActivationPop::Draw() {
        if (!enabled.load()) return;
        std::lock_guard lock(mutex);
        const auto now = std::chrono::steady_clock::now();
        std::erase_if(visible, [](const Visual& visual) { return visual.frame != GetFrameCount(); });
        std::erase_if(copies, [now](const Copy& copy) {
            return std::chrono::duration<float>(now - copy.start).count() >= copy.visual.duration;
        });
        for (const auto& copy : copies) {
            if (copy.visual.worldAnchor) VR::BeginWorldPrompt(*copy.visual.worldAnchor);
            DrawCopy(copy.visual, std::chrono::duration<float>(now - copy.start).count() / copy.visual.duration);
            if (copy.visual.worldAnchor) VR::EndWorldPrompt();
        }
        if (visible.empty() && copies.empty()) enabled.store(false);
    }

    void ActivationPop::Clear() {
        if (!enabled.load()) return;
        std::lock_guard lock(mutex);
        visible.clear();
        copies.clear();
        enabled.store(false);
    }
}

#pragma once
#include "IconsFonts.h"

namespace ImGui::PromptLayouts {
    struct PromptBounds {
        ImVec2 min;
        ImVec2 size;
    };

    struct VerticalPromptRow {
        ImVec2 textSize;
        float centerY;
        float textOffset;
    };

    struct VerticalPromptLayout {
        std::vector<VerticalPromptRow> rows;
        PromptBounds bounds;
        float iconX;
    };

    struct PromptItemDimensions {
        float width;
        float height;
        float textWidth;
        float textHeight;
        float textPad;
    };

    struct HorizontalPromptLayout {
        std::vector<PromptItemDimensions> items;
        ImVec2 size;
    };

    enum DiamondArm : std::size_t {
        kDiamondBottom,
        kDiamondRight,
        kDiamondLeft,
        kDiamondTop,
        kDiamondArmCount
    };

    struct DiamondPromptLayout {
        HorizontalPromptLayout prompts;
        std::vector<ImVec2> positions;
        PromptBounds bounds;
    };

    float GetIconSize(const Theme::Theme& theme);
    float GetSeparatorGap(const Theme::Theme& theme);
    VerticalPromptLayout MeasureVerticalPrompts(const Theme::Theme& theme, const std::vector<RenderInfo>& batch);
    HorizontalPromptLayout MeasureHorizontalPrompts(const Theme::Theme& theme, const std::vector<RenderInfo>& batch,
                                                    float lineSpacingPx);
    DiamondArm GetDiamondArm(std::size_t index);
    bool IsDiamondTextFirst(const Theme::Theme& theme, DiamondArm arm);
    float GetPromptIconCenterX(const Theme::Theme& theme, const PromptItemDimensions& dimensions, bool textFirst);
    DiamondPromptLayout MeasureDiamondPrompts(const Theme::Theme& theme, const std::vector<RenderInfo>& batch,
                                              float lineSpacingPx);

    struct List {
        enum class Navigation {
            kUnhandled,
            kNone,  // Navigation input without a step during release or repeat delay.
            kPrevious,
            kNext
        };

        static constexpr float repeatDelay = 0.35f;
        static constexpr float repeatRate = 0.12f;

        size_t selection = 0;
        size_t firstVisible = 0;

        static float GetIndicatorPadding(const Theme::Theme& theme);
        static Navigation GetNavigation(const RE::ButtonEvent& button, uint32_t buttonKey, uint32_t activateKey);
        void MoveSelection(Navigation navigation, size_t promptCount);
        void Reset();
        void OnRowRemoved(size_t index);
        void OnRowRestored(size_t from, size_t to);
        void ClampSelection(size_t promptCount);
        void UpdateViewport(size_t promptCount, size_t visibleCount);
        bool PrepareRow(RenderInfo& row, size_t index, size_t promptCount, size_t visibleCount) const;
    };
}

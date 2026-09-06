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

    float GetIconSize();
    VerticalPromptLayout MeasureVerticalPrompts(const std::vector<RenderInfo>& batch);
    HorizontalPromptLayout MeasureHorizontalPrompts(const std::vector<RenderInfo>& batch,
                                                    float lineSpacingPx);
    DiamondArm GetDiamondArm(std::size_t index);
    bool IsDiamondTextFirst(DiamondArm arm);
    float GetPromptIconCenterX(const PromptItemDimensions& dimensions, bool textFirst);
    DiamondPromptLayout MeasureDiamondPrompts(const std::vector<RenderInfo>& batch,
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

        static Navigation GetNavigation(const RE::ButtonEvent& button, uint32_t activateKey);
        void MoveSelection(Navigation navigation, size_t promptCount);
        void Reset();
        void ClampSelection(size_t promptCount);
        void UpdateViewport(size_t promptCount, size_t visibleCount);
        bool PrepareRow(RenderInfo& row, size_t index, size_t promptCount, size_t visibleCount) const;
    };
}

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
}

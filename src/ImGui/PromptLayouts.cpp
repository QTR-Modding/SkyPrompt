#include "PromptLayouts.h"
#include "Theme.h"
#include "imgui_internal.h"

namespace ImGui::PromptLayouts {
    float GetIconSize() {
        const auto a_fontsize = ImGui::GetIO().FontDefault->FontSize;
        return a_fontsize * Theme::last_theme->icon2font_ratio;
    }

    VerticalPromptLayout MeasureVerticalPrompts(const std::vector<ImGui::RenderInfo>& batch) {
        VerticalPromptLayout layout{};
        if (batch.empty()) {
            return layout;
        }

        const float iconSize = GetIconSize();
        const float circleDiameter = iconSize * 1.25f;
        const float circleOverhang = (circleDiameter - iconSize) * 0.5f;
        const float extraSpacing = Theme::last_theme->linespacing * 5.0f;
        const bool textFirst = Theme::last_theme->prompt_order == Theme::kTextFirst;

        layout.rows.reserve(batch.size());
        float maxPrefixWidth = 0.0f;
        float maxTextExtent = 0.0f;
        const float scrollPadding = Theme::last_theme->prompt_alignment == Theme::kList ? ImGui::GetFontSize() : 0.0f;
        float rowStart = scrollPadding;
        float maxBottom = 0.0f;
        for (const auto& renderInfo : batch) {
            const ImVec2 textSize = ImGui::CalcTextSize(renderInfo.text.c_str());
            const float rowHeight = std::max(circleDiameter, textSize.y);
            const float iconOffset = (rowHeight - iconSize) * 0.5f;
            const float textOffset = (rowHeight - textSize.y) * 0.5f;
            const float textPad = circleOverhang + textOffset;

            layout.rows.push_back({
                .textSize = textSize,
                .centerY = rowStart + rowHeight * 0.5f,
                .textOffset = textOffset
            });
            maxPrefixWidth = std::max(maxPrefixWidth, textSize.x + textPad);
            maxTextExtent = std::max(maxTextExtent, textSize.x + textOffset);
            maxBottom = std::max(maxBottom, rowStart + rowHeight);

            if (textFirst) {
                rowStart += textOffset + textSize.y + textOffset * extraSpacing;
            } else {
                rowStart += std::max(iconOffset + iconSize, textOffset + textSize.y) +
                    ImGui::GetStyle().ItemSpacing.y +
                    textOffset * extraSpacing;
            }
        }

        layout.iconX = textFirst ? ImGui::GetStyle().ItemSpacing.x + maxPrefixWidth : 0.0f;
        layout.bounds = {
            .min = {textFirst ? 0.0f : -circleOverhang, 0.0f},
            .size = {circleDiameter + ImGui::GetStyle().ItemSpacing.x + maxTextExtent, maxBottom + scrollPadding}
        };
        return layout;
    }

    HorizontalPromptLayout MeasureHorizontalPrompts(const std::vector<ImGui::RenderInfo>& batch,
                                                    const float lineSpacingPx) {
        HorizontalPromptLayout layout;
        layout.items.reserve(batch.size());

        const float iconSize = GetIconSize();
        const float circleDiameter = iconSize * 1.25f;
        for (const auto& renderInfo : batch) {
            const ImVec2 textSize = ImGui::CalcTextSize(renderInfo.text.c_str());
            const float rowHeight = std::max(circleDiameter, textSize.y);
            const float textPad =
                (circleDiameter - iconSize) * 0.5f + (rowHeight - textSize.y) * 0.5f;

            layout.items.push_back({
                .width = circleDiameter + textPad + textSize.x,
                .height = rowHeight,
                .textWidth = textSize.x,
                .textHeight = textSize.y,
                .textPad = textPad
            });
            layout.size.x += layout.items.back().width;
            layout.size.y = std::max(layout.size.y, rowHeight);
        }

        if (layout.items.size() > 1) {
            layout.size.x += lineSpacingPx * static_cast<float>(layout.items.size() - 1);
        }
        return layout;
    }

    DiamondArm GetDiamondArm(const std::size_t index) {
        return static_cast<DiamondArm>(index % kDiamondArmCount);
    }

    bool IsDiamondTextFirst(const DiamondArm arm) {
        const bool textFirst = arm == kDiamondLeft || arm == kDiamondTop;
        return Theme::last_theme->prompt_order == Theme::kIconFirst ? textFirst : !textFirst;
    }

    float GetPromptIconCenterX(const PromptItemDimensions& dimensions, const bool textFirst) {
        const float circleDiameter = GetIconSize() * 1.25f;
        return textFirst
                   ? dimensions.textWidth + dimensions.textPad + circleDiameter * 0.5f
                   : circleDiameter * 0.5f;
    }

    DiamondPromptLayout MeasureDiamondPrompts(const std::vector<ImGui::RenderInfo>& batch,
                                              const float lineSpacingPx) {
        DiamondPromptLayout layout;
        layout.prompts = MeasureHorizontalPrompts(batch, 0.0f);
        layout.positions.resize(batch.size());
        if (batch.empty()) {
            return layout;
        }

        float leftInward = 0.0f;
        float rightInward = 0.0f;
        for (std::size_t i = 0; i < batch.size(); ++i) {
            const auto arm = GetDiamondArm(i);
            if (arm != kDiamondLeft && arm != kDiamondRight) {
                continue;
            }
            const auto& dimensions = layout.prompts.items[i];
            const float iconCenterX = GetPromptIconCenterX(dimensions, IsDiamondTextFirst(arm));
            if (arm == kDiamondLeft) {
                leftInward = std::max(leftInward, dimensions.width - iconCenterX);
            } else {
                rightInward = std::max(rightInward, iconCenterX);
            }
        }

        float radius = layout.prompts.size.y + lineSpacingPx;
        if (leftInward > 0.0f && rightInward > 0.0f) {
            radius = std::max(radius, (leftInward + rightInward + lineSpacingPx) * 0.5f);
        }
        std::array<float, kDiamondArmCount> nextPosition{};
        std::array<std::size_t, kDiamondArmCount> armSizes{};
        float sideBottom = std::numeric_limits<float>::lowest();
        float bottomHeight = 0.0f;

        for (std::size_t i = 0; i < batch.size(); ++i) {
            const auto arm = GetDiamondArm(i);
            const auto& dimensions = layout.prompts.items[i];
            const float iconCenterX = GetPromptIconCenterX(dimensions, IsDiamondTextFirst(arm));
            auto& position = layout.positions[i];

            if (arm == kDiamondBottom || arm == kDiamondTop) {
                const float centerY = arm == kDiamondBottom ? radius : -radius;
                const float left = armSizes[arm] == 0 ? -iconCenterX : nextPosition[arm];
                position = {left, centerY - dimensions.height * 0.5f};
                nextPosition[arm] = left + dimensions.width + lineSpacingPx;
                if (arm == kDiamondBottom) {
                    bottomHeight = std::max(bottomHeight, dimensions.height);
                }
            } else {
                const float centerX = arm == kDiamondRight ? radius : -radius;
                const float top = armSizes[arm] == 0
                                      ? -dimensions.height * 0.5f
                                      : nextPosition[arm];
                position = {centerX - iconCenterX, top};
                nextPosition[arm] = top + dimensions.height + lineSpacingPx;
                sideBottom = std::max(sideBottom, top + dimensions.height);
            }
            ++armSizes[arm];
        }

        if (armSizes[kDiamondBottom] > 0 &&
            armSizes[kDiamondRight] + armSizes[kDiamondLeft] > 0) {
            const float bottomCenter = std::max(
                radius, sideBottom + lineSpacingPx + bottomHeight * 0.5f);
            const float offset = bottomCenter - radius;
            for (std::size_t i = kDiamondBottom; i < layout.positions.size(); i += kDiamondArmCount) {
                layout.positions[i].y += offset;
            }
        }

        const float highest = std::numeric_limits<float>::max();
        const float lowest = std::numeric_limits<float>::lowest();
        ImVec2 boundsMin{highest, highest};
        ImVec2 boundsMax{lowest, lowest};

        for (std::size_t i = 0; i < batch.size(); ++i) {
            const auto& dimensions = layout.prompts.items[i];
            const auto& position = layout.positions[i];
            boundsMin.x = std::min(boundsMin.x, position.x);
            boundsMin.y = std::min(boundsMin.y, position.y);
            boundsMax.x = std::max(boundsMax.x, position.x + dimensions.width);
            boundsMax.y = std::max(boundsMax.y, position.y + dimensions.height);
        }

        for (auto& position : layout.positions) {
            position -= boundsMin;
        }
        layout.bounds = {.min = {0.0f, 0.0f}, .size = boundsMax - boundsMin};
        return layout;
    }

    static ImVec2 GetPromptPivotFactor(const Theme::PromptPivot pivot) {
        switch (pivot) {
            case Theme::kTopLeft:
                return {0.0f, 0.0f};
            case Theme::kTopRight:
                return {1.0f, 0.0f};
            case Theme::kBottomLeft:
                return {0.0f, 1.0f};
            case Theme::kCenter:
                return {0.5f, 0.5f};
            case Theme::kBottomRight:
            default:
                return {1.0f, 1.0f};
        }
    }

    static ImVec2 GetContentOrigin(const ImVec2& anchor, const PromptBounds& bounds) {
        const auto pivot = GetPromptPivotFactor(Theme::last_theme->prompt_pivot);
        return {
            anchor.x - bounds.size.x * pivot.x - bounds.min.x,
            anchor.y - bounds.size.y * pivot.y - bounds.min.y
        };
    }
}


ImVec2 ImGui::GetSkyPromptContentOrigin(const ImVec2& anchor) {
    using namespace ImGui::PromptLayouts;
    if (renderBatch.empty()) {
        return anchor;
    }

    const auto promptAlignment = Theme::last_theme->prompt_alignment;
    const float lineSpacingPx = GetFontSize() * Theme::last_theme->linespacing;
    if (promptAlignment != Theme::kList) {
        std::ranges::sort(renderBatch, {}, &RenderInfo::row);
    }

    if (promptAlignment == Theme::kDiamond) {
        const auto layout = MeasureDiamondPrompts(renderBatch, lineSpacingPx);
        return GetContentOrigin(anchor, layout.bounds);
    }

    if (promptAlignment == Theme::kRadial) {
        return anchor;
    }

    if (promptAlignment == Theme::kHorizontal) {
        const auto layout = MeasureHorizontalPrompts(renderBatch, lineSpacingPx);
        return GetContentOrigin(anchor, {.min = {0.0f, 0.0f}, .size = layout.size});
    }
    return GetContentOrigin(anchor, MeasureVerticalPrompts(renderBatch).bounds);
}

namespace ImGui::PromptLayouts {
    void List::MoveSelection(const Navigation navigation, const size_t promptCount) {
        if (selection >= promptCount) return;
        if (navigation == Navigation::kPrevious && selection > 0) {
            --selection;
        } else if (navigation == Navigation::kNext && selection + 1 < promptCount) {
            ++selection;
        }
    }

    List::Navigation List::GetNavigation(const RE::ButtonEvent& button, const uint32_t activateKey) {
        using namespace SKSE::InputMap;
        const auto key = Input::Manager::Convert(button.GetIDCode(), button.GetDevice());
        if (key == activateKey) return Navigation::kUnhandled;
        const bool up = key == Input::Manager::Convert(MOUSE::kWheelUp, RE::INPUT_DEVICE::kMouse) ||
                        key == kGamepadButtonOffset_DPAD_UP;
        const bool down = key == Input::Manager::Convert(MOUSE::kWheelDown, RE::INPUT_DEVICE::kMouse) ||
                          key == kGamepadButtonOffset_DPAD_DOWN;
        if (up || down) {
            const float held = button.HeldDuration();
            if (button.IsDown() || (button.IsPressed() &&
                ImGui::CalcTypematicRepeatAmount(held - ImGui::GetIO().DeltaTime, held,
                    repeatDelay, repeatRate) > 0)) {
                return up ? Navigation::kPrevious : Navigation::kNext;
            }
            return Navigation::kNone;
        }
        return Navigation::kUnhandled;
    }

    void List::UpdateViewport(const size_t promptCount, const size_t visibleCount) {
        ClampSelection(promptCount);
        firstVisible = std::clamp(firstVisible,
            selection >= visibleCount ? selection - visibleCount + 1 : 0, selection);
    }

    void List::Reset() {
        selection = firstVisible = 0;
    }

    void List::ClampSelection(const size_t promptCount) {
        selection = promptCount == 0 ? 0 : std::min(selection, promptCount - 1);
        firstVisible = std::min(firstVisible, selection);
    }

    bool List::PrepareRow(RenderInfo& row, const size_t index, const size_t promptCount,
                          const size_t visibleCount) const {
        if (index < firstVisible || index - firstVisible >= visibleCount) {
            return false;
        }
        row.selected = index == selection;
        row.moreAbove = index == firstVisible && firstVisible > 0;
        row.moreBelow = index - firstVisible == visibleCount - 1 && index + 1 < promptCount;
        return true;
    }
}

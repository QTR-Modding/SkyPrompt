#include "PromptLayouts.h"
#include "PromptEffects.h"
#include "Renderer.h"
#include "imgui_internal.h"
#include "SkyPrompt/AddOns.hpp"

namespace {
    using namespace ImGui::PromptLayouts;

    SkyPrompt::AddOns::SpecialEffects::SpecialsView
    GetSpecialsView(const Theme::SpecialEffect& a_effect) {
        return {a_effect.id, a_effect.integers, a_effect.strings, a_effect.floats, a_effect.bools};
    }

    void RenderTextEffects(ImDrawList* a_drawList, const ImVec2 a_min, const ImVec2 a_max,
                           const float a_angle = 0.0f, const float a_alpha = 1.0f) {
        for (const auto& effect : Theme::last_theme->special_effects) {
            SkyPrompt::AddOns::RenderSpecialEffect(GetSpecialsView(effect), a_drawList,
                                                   a_min, a_max, a_angle, a_alpha);
        }
    }

    // Utility: scale a packed ImU32 color's alpha by factor in [0,1]
    ImU32 MulAlpha(const ImU32 c, float a) {
        a = ImClamp(a, 0.0f, 1.0f);
        const int A = (c >> IM_COL32_A_SHIFT) & 0xFF;
        const int newA = static_cast<int>(A * a + 0.5f);
        return (c & ~IM_COL32_A_MASK) | (static_cast<ImU32>(newA) << IM_COL32_A_SHIFT);
    }


    void DrawCircle(ImDrawList* drawList, const ImVec2 center, const float radius, const float progress,
                    const float thickness, const ImU32 color, const float start = 0.0f,
                    const bool clockwise = true) {
        if (thickness <= 0.0f) return;
        constexpr int segments = 64;
        const float startAngle = start - IM_PI * 0.5f;
        const float endAngle = startAngle + (clockwise ? progress : -progress) * 2.0f * IM_PI;
        drawList->PathArcTo(center, radius, startAngle, endAngle, segments);
        drawList->PathStroke(color, false, thickness);
    }

    void DrawHoldMark(ImDrawList* drawList, const ImVec2 center, const float radius,
                      const float size, const float angle, const ImU32 color) {
        if (size <= 0.0f) return;
        const float width = size * 0.5f;
        const float height = size * 0.25f;
        const float c = cosf(angle), s = sinf(angle);
        const auto rotate = [&](const ImVec2 p) {
            return center + ImVec2(p.x * c - p.y * s, p.x * s + p.y * c);
        };
        drawList->AddTriangleFilled(rotate({0.0f, -radius + height}),
                                   rotate({-width * 0.5f, -radius - height}),
                                   rotate({width * 0.5f, -radius - height}), color);
    }

    void DrawSpecialCommandMarks(ImDrawList* drawList, const float progress, const float buttonState,
                                 const ImVec2 center, const float radius, const float thickness,
                                 const Theme::SpecialEffect* effect) {
        using namespace ImGui::PromptEffects;
        if (!MCP::Settings::SpecialCommands::visualize || thickness <= 0.0f ||
            !Bool(effect, kProgressCircle, Progress::kShowSpecialCommands)) return;
        const auto red = ImGui::PromptEffects::Color(effect, kProgressCircle, Progress::kRemoval);
        if (buttonState < 3.0f) {
            const float crossRadius = radius * 0.6f;
            if (buttonState > 2.0f) {
                drawList->AddLine(center + ImVec2(-crossRadius, -crossRadius),
                                  center + ImVec2(crossRadius, crossRadius), red, thickness);
            }
            if (buttonState > 1.0f) {
                drawList->AddLine(center + ImVec2(crossRadius, -crossRadius),
                                  center + ImVec2(-crossRadius, crossRadius), red, thickness);
            }
        } else if (progress > 0.0f) {
            DrawCircle(drawList, center, radius, progress, thickness, red, 0.0f,
                       Bool(effect, kProgressCircle, Progress::kClockwise));
        } else {
            DrawCircle(drawList, center, radius, 1.0f, thickness,
                       ImGui::PromptEffects::Color(effect, kProgressCircle, Progress::kSkip));
        }
    }

    void DrawPromptStateOverlay(ImDrawList* drawList, const float progress, const float buttonState,
                                const ImVec2 center, float radius, float thickness,
                                const float holdSize, const float angle = 0.0f) {
        using namespace ImGui::PromptEffects;
        const auto* effect = Find(kProgressCircle);
        radius *= Float(effect, kProgressCircle, Progress::kRadius);
        thickness *= Float(effect, kProgressCircle, Progress::kThickness);
        if (radius <= 0.0f) return;
        const auto firstVertex = drawList->VtxBuffer.Size;
        DrawSpecialCommandMarks(drawList, progress, buttonState, center, radius, thickness, effect);

        const bool singlePress = progress < 0.0f;
        if (singlePress || buttonState > 0.0f) {
            if (Bool(effect, kProgressCircle, Progress::kShowTrack)) {
                DrawCircle(drawList, center, radius, 1.0f, thickness,
                           ImGui::PromptEffects::Color(effect, kProgressCircle, Progress::kTrack));
            }
            if (!singlePress && Bool(effect, kProgressCircle, Progress::kShowHold)) {
                DrawHoldMark(drawList, center, radius,
                             holdSize * Float(effect, kProgressCircle, Progress::kHoldMarkerSize), angle,
                             ImGui::PromptEffects::Color(effect, kProgressCircle, Progress::kHold));
            }
            if (buttonState < 3.0f && Bool(effect, kProgressCircle, Progress::kShowArc)) {
                const float start = singlePress ? 360.0f * (1.0f + progress)
                                               : ImGui::Renderer::progress_circle_offset_deg;
                const float arc = std::max(singlePress ? -progress
                                          : progress - ImGui::Renderer::progress_circle_offset, 0.0f);
                const auto color = ImGui::PromptEffects::Color(effect, kProgressCircle,
                    arc + ImGui::Renderer::progress_circle_offset >= 1.0f ? Progress::kCompleted : Progress::kArc);
                const bool clockwise = Bool(effect, kProgressCircle, Progress::kClockwise);
                const float startAngle = RE::deg_to_rad(start);
                DrawCircle(drawList, center, radius, arc, thickness, color,
                           angle + (clockwise ? startAngle : -startAngle), clockwise);
            }
        }

        const float rotation = RE::deg_to_rad(Float(effect, kProgressCircle, Progress::kRotation));
        const ImVec2 offset{Float(effect, kProgressCircle, Progress::kOffsetX),
                            Float(effect, kProgressCircle, Progress::kOffsetY)};
        if (rotation == 0.0f && offset.x == 0.0f && offset.y == 0.0f) return;
        const float c = cosf(rotation), s = sinf(rotation);
        for (auto i = firstVertex; i < drawList->VtxBuffer.Size; ++i) {
            auto& vertex = drawList->VtxBuffer[i];
            const auto p = vertex.pos - center;
            vertex.pos = center + offset + ImVec2(p.x * c - p.y * s, p.x * s + p.y * c);
        }
    }

    ImVec2 GetIconSizeImVec() {
        const auto a_size = GetIconSize();
        return {a_size, a_size};
    }


    void AddTextWithShadow(ImDrawList* draw_list, ImFont* font, const float font_size, const ImVec2 position,
                           const ImVec2 text_size, const ImU32 text_color, const char* text) {
        if (!draw_list || !font || !text || !*text) return;

        RenderTextEffects(draw_list, position, position + text_size);
        const auto shadow_color = IM_COL32(0, 0, 0, 255 * Theme::last_theme->font_shadow);
        draw_list->AddText(font, font_size, position + ImVec2(2.5f, 2.5f), shadow_color, text);
        draw_list->AddText(font, font_size, position, text_color, text);
    }


    ImVec2 DrawPromptIconWithCircularProgress(const IconFont::IconTexture* a_texture,
                                              const float a_startY,
                                              const float a_iconOffset,
                                              ImDrawList* a_drawlist,
                                              const float a_circle_radius,
                                              const float a_progress,
                                              const float a_button_state) {
        ImGui::SetCursorPosY(a_startY + a_iconOffset);
        const auto iconSize = ButtonIcon(a_texture);

        const ImVec2 iconRenderPos = ImGui::GetItemRectMin();
        const ImVec2 iconCenter{
            iconRenderPos.x + (iconSize.x * 0.5f),
            iconRenderPos.y + (iconSize.y * 0.5f)
        };
        const float iconRadius = iconSize.y * 0.5f;
        const float thickness = iconRadius / 6.f;

        DrawPromptStateOverlay(a_drawlist, a_progress, a_button_state, iconCenter,
                               a_circle_radius, thickness, iconRadius * 0.6f);

        return iconCenter;
    }

    void DrawVerticalPrompt(const ImGui::RenderInfo& info, const float a_textFirstIconX) {
        if (!info.texture || !info.texture->srView.Get()) {
            logger::error("Button icon texture not loaded.");
            return;
        }

        // Calculate sizes
        const ImVec2 textSize = ImGui::CalcTextSize(info.text.c_str());

        const auto a_iconsize = GetIconSize();
        const float circleDiameter = a_iconsize * 1.25f;
        const float rowHeight = std::max(circleDiameter, textSize.y);

        // Record the "start" cursor Y.
        const float startY = ImGui::GetCursorPosY();

        const float iconOffset = (rowHeight - a_iconsize) * 0.5f;
        const float textOffset = (rowHeight - textSize.y) * 0.5f;
        const float radius = a_iconsize * 0.5f;
        const float circle_radius = circleDiameter * 0.5f;
        const float textPad = circle_radius - radius + textOffset;

        const auto textColor = info.text_color ? info.text_color : IM_COL32(255, 255, 255, 255);
        const auto a_drawlist = ImGui::GetWindowDrawList();
        ImVec2 iconCenter;
        ImVec2 textPosition;
        if (Theme::last_theme->prompt_order == Theme::kTextFirst) {
            ImGui::SetCursorPosX(a_textFirstIconX - ImGui::GetStyle().ItemSpacing.x - textPad - textSize.x);
            ImGui::SetCursorPosY(startY + textOffset);
            textPosition = ImGui::GetCursorScreenPos();
            AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(),
                              textPosition, textSize, textColor, info.text.c_str());
            ImGui::Dummy(textSize);

            ImGui::SameLine();
            ImGui::SetCursorPosX(a_textFirstIconX);
            iconCenter = DrawPromptIconWithCircularProgress(
                info.texture, startY, iconOffset, a_drawlist, circle_radius, info.progress, info.button_state);

            // Keep the same vertical advance baseline used by the icon-first path.
            ImGui::SetCursorPosY(startY + textOffset + textSize.y);
        } else {
            iconCenter = DrawPromptIconWithCircularProgress(
                info.texture, startY, iconOffset, a_drawlist, circle_radius, info.progress, info.button_state);

            ImGui::SameLine();
            ImGui::SetCursorPosY(startY + textOffset);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPad);

            textPosition = ImGui::GetCursorScreenPos();
            AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(),
                              textPosition, textSize, textColor, info.text.c_str());
            ImGui::Dummy(textSize); // Moves cursor forward horizontally
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset * Theme::last_theme->linespacing * 5);

        MANAGER(ImGui::Renderer)->activationPop.Capture(info, iconCenter, textPosition + textSize * 0.5f, textSize);
    }

    void AddImageRotated(ImDrawList* dl, const ImTextureID tex,
                         const ImVec2 center, const ImVec2 size,
                         const float angle, const ImU32 col) {
        const auto h = ImVec2(size.x * 0.5f, size.y * 0.5f);
        const float c = cosf(angle), s = sinf(angle);

        auto rot = [&](const ImVec2 p) -> ImVec2 {
            return {center.x + (p.x * c - p.y * s),
                    center.y + (p.x * s + p.y * c)};
        };

        // Quad corners before rotation (relative to center)
        const ImVec2 p1 = rot(ImVec2(-h.x, -h.y));
        const ImVec2 p2 = rot(ImVec2(+h.x, -h.y));
        const ImVec2 p3 = rot(ImVec2(+h.x, +h.y));
        const ImVec2 p4 = rot(ImVec2(-h.x, +h.y));

        // Standard UVs
        constexpr ImVec2 uv1(0, 0), uv2(1, 0), uv3(1, 1), uv4(0, 1);
        dl->AddImageQuad(tex, p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);
    }

    // helper: do NOT push/pop clip; caller controls clip once per frame
    void AddTextRotated(ImDrawList* dl, ImFont* font, const float font_size,
                        const ImVec2 pivot, const ImU32 col,
                        const char* text_begin, const char* text_end,
                        const float angle, const bool center_on_pivot = true) {
        if (!text_begin) return;
        if (!text_end) text_end = text_begin + strlen(text_begin);

        ImVec2 topleft = pivot;
        if (center_on_pivot) {
            const ImVec2 ts = ImGui::CalcTextSize(text_begin, text_end);
            topleft.x -= ts.x * 0.5f;
            topleft.y -= ts.y * 0.5f;
        }

        const int vtx_start = dl->VtxBuffer.Size;
        // use the current clip on the draw list
        const ImVec4 clip = dl->_ClipRectStack.back();
        font->RenderText(dl, font_size, topleft, col, clip, text_begin, text_end, 0.0f, false);
        const int vtx_end = dl->VtxBuffer.Size;

        const float c = cosf(angle), s = sinf(angle);
        for (int i = vtx_start; i < vtx_end; ++i) {
            const ImVec2 p = dl->VtxBuffer[i].pos;
            const ImVec2 d = {p.x - pivot.x, p.y - pivot.y};
            dl->VtxBuffer[i].pos.x = pivot.x + d.x * c - d.y * s;
            dl->VtxBuffer[i].pos.y = pivot.y + d.x * s + d.y * c;
        }
    }


    void RenderPromptsRadialRotated(const ImVec2 anchor,
                                    const std::vector<ImGui::RenderInfo>& batch,
                                    const float lineSpacingPx,
                                    const float bendRadius,
                                    const float midpointAngleRad) {
        if (batch.empty()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        ImFont* font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize();

        dl->PushClipRectFullScreen();

        const float iconSz = ImGui::GetIO().FontDefault->FontSize * Theme::last_theme->icon2font_ratio;
        const ImVec2 iconSzV = {iconSz, iconSz};

        const float circleDia = iconSz * 1.25f;
        const float outerR = circleDia * 0.5f;
        const float baseSpacing = ImGui::GetStyle().ItemSpacing.x;
        const float circleOverhang = (circleDia - iconSz) * 0.5f;
        const bool textFirst = Theme::last_theme->prompt_order == Theme::kTextFirst;
        const auto layout = MeasureVerticalPrompts(batch);

        const float radius = std::max(bendRadius, 1.0f);
        const float centerlineX = layout.bounds.min.x + layout.bounds.size.x * 0.5f;
        const float midpointY = layout.bounds.min.y + layout.bounds.size.y * 0.5f;
        const ImVec2 midpointX = {cosf(midpointAngleRad), sinf(midpointAngleRad)};
        const float extraArcSpacingPx =
            lineSpacingPx - (textFirst ? 0.0f : ImGui::GetStyle().ItemSpacing.y);

        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& ri = batch[i];
            const auto& row = layout.rows[i];
            const float centeredIndex =
                static_cast<float>(i) - static_cast<float>(batch.size() - 1) * 0.5f;
            const float theta =
                (row.centerY - midpointY + centeredIndex * extraArcSpacingPx) / radius;
            const float angle = midpointAngleRad + theta;
            const ImVec2 rowX = {cosf(angle), sinf(angle)};
            const ImVec2 rowY = {-rowX.y, rowX.x};
            const ImVec2 arcPoint = {
                anchor.x + radius * (rowX.x - midpointX.x),
                anchor.y + radius * (rowX.y - midpointX.y)
            };
            const float textPad = circleOverhang + row.textOffset;
            const float rowLeftX = layout.bounds.min.x;
            const float iconCenterX = textFirst
                                          ? rowLeftX + row.textSize.x + baseSpacing +
                                              textPad + iconSz * 0.5f
                                          : layout.iconX + iconSz * 0.5f;
            const float textCenterX = textFirst
                                          ? rowLeftX + row.textSize.x * 0.5f
                                          : iconSz + baseSpacing + textPad + row.textSize.x * 0.5f;
            const ImVec2 iconCenter{
                arcPoint.x + (iconCenterX - centerlineX) * rowX.x,
                arcPoint.y + (iconCenterX - centerlineX) * rowX.y
            };

            // --- icon ---
            if (ri.texture && ri.texture->srView.Get()) {
                AddImageRotated(dl, (ImTextureID)ri.texture->srView.Get(), iconCenter, iconSzV, angle,
                                IM_COL32(255, 255, 255, static_cast<int>(255 * ri.alpha)));
            }

            const auto firstOverlayVertex = dl->VtxBuffer.Size;
            {
                const float thick = outerR / 6.f;
                DrawPromptStateOverlay(dl, ri.progress, ri.button_state, iconCenter, outerR, thick,
                                       iconSz * 0.5f, angle);
            }
            for (auto vertex = firstOverlayVertex; vertex < dl->VtxBuffer.Size; ++vertex) {
                dl->VtxBuffer[vertex].col = MulAlpha(dl->VtxBuffer[vertex].col, ri.alpha);
            }

            const ImVec2 textCenter = {
                arcPoint.x + (textCenterX - centerlineX) * rowX.x,
                arcPoint.y + (textCenterX - centerlineX) * rowX.y
            };

            // draw rotated text centered on this pivot
            const ImU32 color = MulAlpha(ri.text_color ? ri.text_color : IM_COL32(255, 255, 255, 255), ri.alpha);
            const ImU32 shadow = MulAlpha(
                IM_COL32(0, 0, 0, static_cast<int>(255 * Theme::last_theme->font_shadow)), ri.alpha);
            const ImVec2 shadowOffset{
                2.5f * (rowX.x + rowY.x),
                2.5f * (rowX.y + rowY.y)
            };

            RenderTextEffects(dl, textCenter - row.textSize * 0.5f, textCenter + row.textSize * 0.5f, angle, ri.alpha);
            AddTextRotated(dl, font, fs, textCenter + shadowOffset,
                           shadow, ri.text.c_str(), nullptr, angle, true);
            AddTextRotated(dl, font, fs, textCenter,
                           color, ri.text.c_str(), nullptr, angle, true);
            MANAGER(ImGui::Renderer)->activationPop.Capture(ri, iconCenter, textCenter, row.textSize, angle);
        }

        dl->PopClipRect();
    }

    void DrawHorizontalPrompt(ImDrawList* drawList, ImFont* font, const float fontSize,
                              const ImGui::RenderInfo& renderInfo,
                              const PromptItemDimensions& dimensions,
                              const ImVec2& position, const bool textFirst) {
        const float iconSize = GetIconSize();
        const float circleDiameter = iconSize * 1.25f;
        const float yCenter = position.y + dimensions.height * 0.5f;
        const float iconCenterX = position.x + GetPromptIconCenterX(dimensions, textFirst);
        const ImVec2 iconCenter{iconCenterX, yCenter};

        if (renderInfo.texture && renderInfo.texture->srView.Get()) {
            AddImageRotated(drawList, (ImTextureID)renderInfo.texture->srView.Get(), iconCenter,
                            {iconSize, iconSize}, 0.0f,
                            IM_COL32(255, 255, 255, static_cast<int>(255 * renderInfo.alpha)));
        }

        const auto firstVertex = drawList->VtxBuffer.Size;
        {
            const float outerRadius = circleDiameter * 0.5f;
            const float thickness = outerRadius / 6.0f;
            DrawPromptStateOverlay(drawList, renderInfo.progress, renderInfo.button_state,
                                   iconCenter, outerRadius, thickness, iconSize * 0.5f * 0.6f);
        }

        const ImVec2 textPosition{
            textFirst ? position.x : position.x + circleDiameter + dimensions.textPad,
            yCenter - dimensions.textHeight * 0.5f
        };
        const ImU32 color = renderInfo.text_color
                                ? renderInfo.text_color
                                : IM_COL32(255, 255, 255, 255);
        AddTextWithShadow(drawList, font, fontSize, textPosition,
                          {dimensions.textWidth, dimensions.textHeight}, color, renderInfo.text.c_str());
        for (auto i = firstVertex; i < drawList->VtxBuffer.Size; ++i) {
            drawList->VtxBuffer[i].col = MulAlpha(drawList->VtxBuffer[i].col, renderInfo.alpha);
        }
        MANAGER(ImGui::Renderer)->activationPop.Capture(renderInfo, iconCenter,
            textPosition + ImVec2(dimensions.textWidth, dimensions.textHeight) * 0.5f,
            {dimensions.textWidth, dimensions.textHeight});
    }

    void RenderPromptsVertical(const std::vector<ImGui::RenderInfo>& batch) {
        float textFirstIconX = 0.0f;
        if (Theme::last_theme->prompt_order == Theme::kTextFirst) {
            const auto layout = MeasureVerticalPrompts(batch);
            textFirstIconX = ImGui::GetCursorPosX() + layout.iconX;
        }
        auto* drawList = ImGui::GetWindowDrawList();
        for (const auto& renderInfo : batch) {
            const auto firstVertex = drawList->VtxBuffer.Size;
            DrawVerticalPrompt(renderInfo, textFirstIconX);
            for (auto vertex = firstVertex; vertex < drawList->VtxBuffer.Size; ++vertex) {
                drawList->VtxBuffer[vertex].col = MulAlpha(drawList->VtxBuffer[vertex].col, renderInfo.alpha);
            }
        }
    }

    void DrawListIndicators(ImDrawList* drawList, const std::vector<ImGui::RenderInfo>& batch,
                            const VerticalPromptLayout& layout, const ImVec2 start) {
        using namespace ImGui::PromptEffects;
        const auto* effect = Find(kListIndicators);
        const float scale = Float(effect, kListIndicators, ListIndicators::kSize);
        if (!Bool(effect, kListIndicators, ListIndicators::kShow) || scale <= 0.0f) return;
        const ImVec2 offset{Float(effect, kListIndicators, ListIndicators::kOffsetX),
                            Float(effect, kListIndicators, ListIndicators::kOffsetY)};
        if (batch.front().moreAbove) {
            ImGui::RenderArrow(drawList, start + offset + ImVec2(layout.iconX, 0.0f),
                MulAlpha(ImGui::PromptEffects::Color(effect, kListIndicators, ListIndicators::kUp), batch.front().alpha),
                ImGuiDir_Up, scale);
        }
        if (batch.back().moreBelow) {
            ImGui::RenderArrow(drawList,
                start + offset + ImVec2(layout.iconX, layout.bounds.size.y - ImGui::GetFontSize() * scale),
                MulAlpha(ImGui::PromptEffects::Color(effect, kListIndicators, ListIndicators::kDown), batch.back().alpha),
                ImGuiDir_Down, scale);
        }
    }

    void RenderPromptsList(const std::vector<ImGui::RenderInfo>& batch) {
        const auto layout = MeasureVerticalPrompts(batch);
        const auto start = ImGui::GetCursorScreenPos();
        auto* drawList = ImGui::GetWindowDrawList();
        const float iconSize = GetIconSize();
        const float radius = iconSize * 1.25f * 0.5f;
        const bool textFirst = Theme::last_theme->prompt_order == Theme::kTextFirst;
        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& info = batch[i];
            const auto& row = layout.rows[i];
            const auto firstVertex = drawList->VtxBuffer.Size;
            const ImVec2 center = start + ImVec2(layout.iconX + iconSize * 0.5f, row.centerY);
            if (info.selected) {
                drawList->AddImage((ImTextureID)info.texture->srView.Get(),
                    center - ImVec2(iconSize, iconSize) * 0.5f, center + ImVec2(iconSize, iconSize) * 0.5f);
                DrawPromptStateOverlay(drawList, info.progress, info.button_state, center,
                                       radius, radius / 6.0f, iconSize * 0.5f * 0.6f);
            }
            const float textPad = radius - iconSize * 0.5f + row.textOffset;
            const float textX = textFirst
                ? layout.iconX - ImGui::GetStyle().ItemSpacing.x - textPad - row.textSize.x
                : iconSize + ImGui::GetStyle().ItemSpacing.x + textPad;
            AddTextWithShadow(drawList, ImGui::GetFont(), ImGui::GetFontSize(),
                start + ImVec2(textX, row.centerY - row.textSize.y * 0.5f),
                row.textSize, info.text_color ? info.text_color : IM_COL32_WHITE, info.text.c_str());
            for (auto vertex = firstVertex; vertex < drawList->VtxBuffer.Size; ++vertex) {
                drawList->VtxBuffer[vertex].col = MulAlpha(drawList->VtxBuffer[vertex].col, info.alpha);
            }
            MANAGER(ImGui::Renderer)->activationPop.Capture(info, center,
                start + ImVec2(textX + row.textSize.x * 0.5f, row.centerY), row.textSize, 0.0f, info.selected);
        }
        DrawListIndicators(drawList, batch, layout, start);
        ImGui::Dummy(layout.bounds.size);
    }

    void RenderPromptsHorizontal(const std::vector<ImGui::RenderInfo>& batch, const float lineSpacingPx) {
        if (batch.empty()) return;

        ImFont* font = ImGui::GetFont();
        const float fs = ImGui::GetFontSize();

        const auto layout = MeasureHorizontalPrompts(batch, lineSpacingPx);
        const ImVec2 startPos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());

        float xCursor = startPos.x;
        const float yCenter = startPos.y + layout.size.y * 0.5f;
        const bool textFirst = Theme::last_theme->prompt_order == Theme::kTextFirst;

        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& ri = batch[i];
            const auto& dim = layout.items[i];
            DrawHorizontalPrompt(dl, font, fs, ri, dim,
                                 {xCursor, yCenter - dim.height * 0.5f}, textFirst);
            xCursor += dim.width + lineSpacingPx;
        }

        ImGui::Dummy(layout.size);
    }

    void RenderPromptsDiamond(const std::vector<ImGui::RenderInfo>& batch, const float lineSpacingPx) {
        const auto layout = MeasureDiamondPrompts(batch, lineSpacingPx);
        const ImVec2 startPosition = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        ImFont* font = ImGui::GetFont();
        const float fontSize = ImGui::GetFontSize();

        for (std::size_t i = 0; i < batch.size(); ++i) {
            DrawHorizontalPrompt(drawList, font, fontSize, batch[i], layout.prompts.items[i],
                                 startPosition + layout.positions[i],
                                 IsDiamondTextFirst(GetDiamondArm(i)));
        }

        ImGui::Dummy(layout.bounds.size);
    }

}


ImVec2 ImGui::ButtonIcon(const IconFont::IconTexture* a_texture) {
    const auto a_size = GetIconSizeImVec();
    Image(reinterpret_cast<ImTextureID>(a_texture->srView.Get()), a_size);
    return a_size;
}

void ImGui::DrawCycleIndicators(SkyPromptAPI::ClientID curr_index, SkyPromptAPI::ClientID queue_size) {
    auto* iconMgr = MANAGER(IconFont);
    const auto* inputMgr = MANAGER(Input);
    const auto curr_device = inputMgr->GetInputDevice();
    const uint32_t keyL = MCP::Settings::cycle_L.at(curr_device);

    const uint32_t keyR = MCP::Settings::cycle_R.at(curr_device);

    const float iconSz = ImGui::PromptLayouts::GetIconSize() * 0.6f;

    const float spacing = GetFontSize() * 0.25f;
    Dummy(ImVec2(0.0f, spacing));

    if (const auto* icoL = iconMgr->GetIcon(keyL))
        Image((ImTextureID)icoL->srView.Get(), {iconSz, iconSz});

    SameLine();

    if (const auto* icoR = iconMgr->GetIcon(keyR))
        Image((ImTextureID)icoR->srView.Get(), {iconSz, iconSz});

    SameLine();

    if (auto* smallFont = MANAGER(IconFont)->GetSmallFont()) {
        PushFont(smallFont);
        const std::string text = std::format("({}/{})", curr_index, queue_size);
        const ImVec2 textSize = CalcTextSize(text.c_str());

        const float textOffset = (iconSz - textSize.y) * 0.5f;
        SetCursorPosY(GetCursorPosY() + textOffset);

        TextUnformatted(text.c_str());
        PopFont();
    }
}


void ImGui::RenderSkyPrompt(const ImVec2& anchor) {
    if (renderBatch.empty()) {
        return;
    }

    const auto& curr_theme = Theme::last_theme;
    const auto prompt_alignment = curr_theme->prompt_alignment;

    // Decorations may extend beyond the auto-sized layout window.
    auto* drawList = GetWindowDrawList();
    drawList->PushClipRectFullScreen();
    switch (prompt_alignment) {
        case Theme::PromptAlignment::kVertical:
            RenderPromptsVertical(renderBatch);
            break;
        case Theme::PromptAlignment::kList:
            RenderPromptsList(renderBatch);
            break;
        case Theme::PromptAlignment::kHorizontal:
            RenderPromptsHorizontal(renderBatch, GetFontSize() * curr_theme->linespacing);
            break;
        case Theme::PromptAlignment::kDiamond:
            RenderPromptsDiamond(renderBatch, GetFontSize() * curr_theme->linespacing);
            break;
        case Theme::PromptAlignment::kRadial: {
            const float lineSpacingPx = GetFontSize() * curr_theme->linespacing;
            const float iconSize = GetIO().FontDefault->FontSize * curr_theme->icon2font_ratio;
            const float bendRadius = iconSize * 6;

            RenderPromptsRadialRotated(anchor,
                                       renderBatch,
                                       lineSpacingPx, bendRadius, 0.0f);
            break;
        }
    }

    drawList->PopClipRect();

    for (const auto& effect : curr_theme->special_effects) {
        const auto a_size = GetIO().FontDefault->FontSize * curr_theme->icon2font_ratio;
        SkyPrompt::AddOns::RenderSpecialEffect(GetSpecialsView(effect), anchor, a_size,
                                               Renderer::GetResolutionScale());
    }
}

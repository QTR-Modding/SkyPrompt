#include "PromptLayouts.h"
#include "Renderer.h"
#include "imgui_internal.h"
#include "SkyPrompt/AddOns.hpp"

namespace {
    using namespace ImGui::PromptLayouts;

    // Utility: scale a packed ImU32 color's alpha by factor in [0,1]
    ImU32 MulAlpha(const ImU32 c, float a) {
        a = ImClamp(a, 0.0f, 1.0f);
        const int A = (c >> IM_COL32_A_SHIFT) & 0xFF;
        const int newA = static_cast<int>(A * a + 0.5f);
        return (c & ~IM_COL32_A_MASK) | (static_cast<ImU32>(newA) << IM_COL32_A_SHIFT);
    }


    void DrawCircle(ImDrawList* drawList, const ImVec2 a_center, const float a_radius, const float progress,
                    const float thickness,
                    const std::optional<uint32_t> a_color = std::nullopt,
                    const std::optional<float> start_angle = std::nullopt) {
        const auto startColor = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 60);
        const auto endColor = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 180);

        constexpr int numSegments = 64;
        const float startAngle = start_angle.has_value() ? start_angle.value() - IM_PI / 2 : -IM_PI / 2;
        // Start at the top
        const float endAngle = startAngle + progress * 2.0f * IM_PI;

        ImVec2 prevPoint = a_center + ImVec2(cosf(startAngle) * a_radius, sinf(startAngle) * a_radius);

        if (startColor != endColor) {
            for (int i = 1; i <= numSegments; ++i) {
                const float t = i / static_cast<float>(numSegments);
                const float angle = startAngle + t * (endAngle - startAngle);
                ImVec2 newPoint = a_center + ImVec2(cosf(angle) * a_radius, sinf(angle) * a_radius);

                // Interpolate color
                const ImU32 color = IM_COL32(
                    (1 - t) * (startColor >> IM_COL32_R_SHIFT & 0xFF) + t * (endColor >> IM_COL32_R_SHIFT & 0xFF),
                    (1 - t) * (startColor >> IM_COL32_G_SHIFT & 0xFF) + t * (endColor >> IM_COL32_G_SHIFT & 0xFF),
                    (1 - t) * (startColor >> IM_COL32_B_SHIFT & 0xFF) + t * (endColor >> IM_COL32_B_SHIFT & 0xFF),
                    (1 - t) * (startColor >> IM_COL32_A_SHIFT & 0xFF) + t * (endColor >> IM_COL32_A_SHIFT & 0xFF)
                    );

                drawList->AddLine(prevPoint, newPoint, color, thickness);
                prevPoint = newPoint;
            }
        } else {
            drawList->PathArcTo(a_center, a_radius, startAngle, endAngle, numSegments);
            drawList->PathStroke(endColor, false, thickness);
        }
    }

    void DrawTriangle(ImDrawList* drawList, const ImVec2 iconCenter, const float outer_radius, const float inner_radius,
                      const std::optional<uint32_t> a_color = std::nullopt) {
        const float triangle_width = inner_radius * 0.5f;
        const float triangle_height = inner_radius * 0.25f;

        const auto p1 = ImVec2(iconCenter.x, iconCenter.y - outer_radius + triangle_height);
        // Tip (bottom, closer to center)
        const auto p2 = ImVec2(iconCenter.x - triangle_width * 0.5f, iconCenter.y - outer_radius - triangle_height);
        // Top left
        const auto p3 = ImVec2(iconCenter.x + triangle_width * 0.5f, iconCenter.y - outer_radius - triangle_height);
        // Top right
        const auto color = a_color.has_value() ? a_color.value() : IM_COL32(255, 255, 255, 200);
        drawList->AddTriangleFilled(p1, p2, p3, color);
    }

    void DrawProgressMark(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float outer_radius,
                          const float a_thickness) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 30);
        DrawCircle(a_drawlist, iconCenter, outer_radius, 1.0, a_thickness, aColor);
    }

    void DrawHoldMark(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float outer_radius,
                      const float inner_radius) {
        constexpr auto aColor = IM_COL32(255, 255, 255, 180);
        DrawTriangle(a_drawlist, iconCenter, outer_radius, inner_radius * 0.6f, aColor);
    }

    void DrawCross1(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
        constexpr auto a_red = IM_COL32(147, 39, 41, 180);
        const ImVec2 topRight = iconCenter + ImVec2(a_radius, -a_radius);
        const ImVec2 bottomLeft = iconCenter + ImVec2(-a_radius, a_radius);
        a_drawlist->AddLine(topRight, bottomLeft, a_red, a_thickness);
    }

    void DrawCross2(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness) {
        constexpr auto a_red = IM_COL32(147, 39, 41, 180);
        const ImVec2 topLeft = iconCenter + ImVec2(-a_radius, -a_radius);
        const ImVec2 bottomRight = iconCenter + ImVec2(a_radius, a_radius);
        a_drawlist->AddLine(topLeft, bottomRight, a_red, a_thickness);
    }

    void DrawSkipPrompt(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius,
                        const float a_thickness) {
        constexpr auto a_yellow = IM_COL32(228, 185, 76, 100);
        DrawCircle(a_drawlist, iconCenter, a_radius, 1.f, a_thickness, a_yellow);
    }

    void DrawDeleteAll(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius, const float a_thickness,
                       const float progress) {
        constexpr auto a_red = IM_COL32(147, 39, 41, 180);
        DrawCircle(a_drawlist, iconCenter, a_radius, progress, a_thickness, a_red);
    }

    void DrawProgressCircle(ImDrawList* a_drawlist, const ImVec2 iconCenter, const float a_radius,
                            const float a_thickness, const float progress, const float start_angle) {
        const auto aColor = progress + ImGui::Renderer::progress_circle_offset >= 1.f
                                ? IM_COL32(228, 185, 76, 180)
                                : IM_COL32(255, 255, 255, 180);
        DrawCircle(a_drawlist, iconCenter, a_radius, std::max(progress, 0.f), a_thickness, aColor,
                   std::max(start_angle, 0.f));
    }

    template <class DrawHoldMarkFn>
    void DrawPromptStateOverlay(ImDrawList* dl, const ImGui::RenderInfo& ri, const ImVec2 iconCenter,
                                const float outerR, const float thickness,
                                const DrawHoldMarkFn& drawHoldMark,
                                const float angleOffsetRad = 0.0f) {
        if (MCP::Settings::SpecialCommands::visualize) {
            if (ri.button_state < 3.f) {
                if (ri.button_state > 2.f) DrawCross2(dl, iconCenter, outerR * 0.6f, thickness);
                if (ri.button_state > 1.f) DrawCross1(dl, iconCenter, outerR * 0.6f, thickness);
            } else if (ri.progress > 0.f) {
                DrawDeleteAll(dl, iconCenter, outerR, thickness, ri.progress);
            } else {
                DrawSkipPrompt(dl, iconCenter, outerR, thickness);
            }
        }

        const bool singlePress = (ri.progress < 0.f);
        if (singlePress || ri.button_state > 0.f) {
            DrawProgressMark(dl, iconCenter, outerR, thickness);
            if (!singlePress) {
                drawHoldMark();
            }
            if (ri.button_state < 3.f) {
                const float startDeg = singlePress
                                           ? 360.f * (1.f + ri.progress)
                                           : ImGui::Renderer::progress_circle_offset_deg;
                const float prog = singlePress
                                       ? -ri.progress
                                       : ri.progress - ImGui::Renderer::progress_circle_offset;
                DrawProgressCircle(dl, iconCenter, outerR, thickness, std::max(prog, 0.f),
                                   RE::deg_to_rad(startDeg) + angleOffsetRad);
            }
        }
    }

    ImVec2 GetIconSizeImVec() {
        const auto a_size = GetIconSize();
        return {a_size, a_size};
    }


    void AddTextWithShadow(ImDrawList* draw_list, ImFont* font, const float font_size, const ImVec2 position,
                           const ImU32 text_color, const char* text) {
        if (!draw_list || !font || !text || !*text) return;

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

        if (MCP::Settings::SpecialCommands::visualize) {
            if (a_button_state < 3.f) {
                if (a_button_state > 2.f) {
                    DrawCross2(a_drawlist, iconCenter, a_circle_radius * 0.6f, thickness);
                }
                if (a_button_state > 1.f) {
                    DrawCross1(a_drawlist, iconCenter, a_circle_radius * 0.6f, thickness);
                }
            } else if (a_progress > 0.f) {
                DrawDeleteAll(a_drawlist, iconCenter, a_circle_radius, thickness, a_progress);
            } else {
                DrawSkipPrompt(a_drawlist, iconCenter, a_circle_radius, thickness);
            }
        }

        if (const bool singlePressProgress = a_progress < 0.f; singlePressProgress || a_button_state > 0.f) {
            DrawProgressMark(a_drawlist, iconCenter, a_circle_radius, thickness);
            if (!singlePressProgress) {
                DrawHoldMark(a_drawlist, iconCenter, a_circle_radius, iconRadius);
            }
            if (a_button_state < 3.f) {
                const auto start_deg = singlePressProgress
                                           ? 360.f * (1 + a_progress)
                                           : ImGui::Renderer::progress_circle_offset_deg;
                const auto a_progress_value = singlePressProgress
                                                  ? -a_progress
                                                  : a_progress - ImGui::Renderer::progress_circle_offset;
                DrawProgressCircle(a_drawlist, iconCenter, a_circle_radius, thickness, a_progress_value,
                                   RE::deg_to_rad(start_deg));
            }
        }

        return iconCenter;
    }

    ImVec2 ButtonIconWithCircularProgress(const char* a_text, const uint32_t a_text_color,
                                          const IconFont::IconTexture* a_texture, const float progress,
                                          const float button_state, const float a_textFirstIconX) {
        if (!a_texture || !a_texture->srView.Get()) {
            logger::error("Button icon texture not loaded.");
            return {};
        }

        // Calculate sizes
        const ImVec2 textSize = ImGui::CalcTextSize(a_text);

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

        const auto textColor = a_text_color ? a_text_color : IM_COL32(255, 255, 255, 255);
        const auto a_drawlist = ImGui::GetWindowDrawList();
        ImVec2 iconCenter;
        if (Theme::last_theme->prompt_order == Theme::kTextFirst) {
            ImGui::SetCursorPosX(a_textFirstIconX - ImGui::GetStyle().ItemSpacing.x - textPad - textSize.x);
            ImGui::SetCursorPosY(startY + textOffset);
            AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(),
                              ImGui::GetCursorScreenPos(), textColor, a_text);
            ImGui::Dummy(textSize);

            ImGui::SameLine();
            ImGui::SetCursorPosX(a_textFirstIconX);
            iconCenter = DrawPromptIconWithCircularProgress(
                a_texture, startY, iconOffset, a_drawlist, circle_radius, progress, button_state);

            // Keep the same vertical advance baseline used by the icon-first path.
            ImGui::SetCursorPosY(startY + textOffset + textSize.y);
        } else {
            iconCenter = DrawPromptIconWithCircularProgress(
                a_texture, startY, iconOffset, a_drawlist, circle_radius, progress, button_state);

            ImGui::SameLine();
            ImGui::SetCursorPosY(startY + textOffset);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPad);

            AddTextWithShadow(a_drawlist, ImGui::GetFont(), ImGui::GetFontSize(),
                              ImGui::GetCursorScreenPos(), textColor, a_text);
            ImGui::Dummy(textSize); // Moves cursor forward horizontally
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset * Theme::last_theme->linespacing * 5);

        return iconCenter;
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

    void DrawTriangleRotated(ImDrawList* dl, const ImVec2 center,
                             const float outer_radius, const float inner_radius,
                             const float angle, const ImU32 col) {
        const float tri_w = inner_radius * 0.5f;
        const float tri_h = inner_radius * 0.25f;

        // local (unrotated) vertices – tip is “up” (toward -Y)
        const auto p1 = ImVec2(0.0f, -outer_radius + tri_h); // tip (closer to center)
        const auto p2 = ImVec2(-tri_w * 0.5f, -outer_radius - tri_h);
        const auto p3 = ImVec2(+tri_w * 0.5f, -outer_radius - tri_h);

        const float c = cosf(angle), s = sinf(angle);
        auto rot = [&](const ImVec2 p) {
            return ImVec2(center.x + p.x * c - p.y * s,
                          center.y + p.x * s + p.y * c);
        };

        dl->AddTriangleFilled(rot(p1), rot(p2), rot(p3), col);
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
                DrawPromptStateOverlay(dl, ri, iconCenter, outerR, thick,
                                       [&]() {
                                           constexpr ImU32 tri_col = IM_COL32(255, 255, 255, 180);
                                           DrawTriangleRotated(dl, iconCenter, outerR, iconSz * 0.5f, angle, tri_col);
                                       },
                                       angle);
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

            AddTextRotated(dl, font, fs, textCenter + shadowOffset,
                           shadow, ri.text.c_str(), nullptr, angle, true);
            AddTextRotated(dl, font, fs, textCenter,
                           color, ri.text.c_str(), nullptr, angle, true);
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
            DrawPromptStateOverlay(drawList, renderInfo, iconCenter, outerRadius, thickness,
                                   [&]() {
                                       DrawHoldMark(drawList, iconCenter, outerRadius, iconSize * 0.5f);
                                   });
        }

        const ImVec2 textPosition{
            textFirst ? position.x : position.x + circleDiameter + dimensions.textPad,
            yCenter - dimensions.textHeight * 0.5f
        };
        const ImU32 color = renderInfo.text_color
                                ? renderInfo.text_color
                                : IM_COL32(255, 255, 255, 255);
        AddTextWithShadow(drawList, font, fontSize, textPosition, color, renderInfo.text.c_str());
        for (auto i = firstVertex; i < drawList->VtxBuffer.Size; ++i) {
            drawList->VtxBuffer[i].col = MulAlpha(drawList->VtxBuffer[i].col, renderInfo.alpha);
        }
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
            ButtonIconWithCircularProgress(renderInfo.text.c_str(), renderInfo.text_color,
                                           renderInfo.texture, renderInfo.progress,
                                           renderInfo.button_state, textFirstIconX);
            for (auto vertex = firstVertex; vertex < drawList->VtxBuffer.Size; ++vertex) {
                drawList->VtxBuffer[vertex].col = MulAlpha(drawList->VtxBuffer[vertex].col, renderInfo.alpha);
            }
        }
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

    SkyPrompt::AddOns::SpecialEffects::SpecialsView
    GetSpecialsView(const Theme::Theme& t) {
        SkyPrompt::AddOns::SpecialEffects::SpecialsView v;
        v.effectID = t.special_effect;
        v.integers = std::span{t.special_integers};
        v.strings = std::span{t.special_strings};
        v.floats = std::span{t.special_floats};
        v.bools = std::span{t.special_bools};
        return v;
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
    const auto special_effect = curr_theme->special_effect;

    switch (prompt_alignment) {
        case Theme::PromptAlignment::kVertical:
            RenderPromptsVertical(renderBatch);
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

    if (special_effect > 0) {
        const auto a_size = GetIO().FontDefault->FontSize * curr_theme->icon2font_ratio;
        SkyPrompt::AddOns::RenderSpecialEffect(GetSpecialsView(*curr_theme), anchor, a_size,
                                               Renderer::GetResolutionScale());
    }
}

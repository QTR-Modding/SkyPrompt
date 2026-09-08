#include "VR.h"
#include "Styles.h"
#include "ImGuiVRHelperClientSDK.h"
#include "imgui_internal.h"

namespace ImGui::VR {
    namespace {
        struct HUD {
            ImGuiVRHelperPluginAPI::Client helper;
            float aspectRatio = 0.0f;
            float verticalFOV = 0.0f;
        } hud;

        struct World {
            struct Group {
                WorldAnchor anchor;
                ImVec2 halfSize;
                ImVec2 atlasMin;
                float scale = 1.0f;
            };

            struct Range {
                ImDrawList* list;
                unsigned int firstIndex;
                unsigned int endIndex;
                int firstVertex;
                int endVertex;
                size_t group;
            };

            struct Commands {
                ImDrawList* list;
                ImVector<ImDrawCmd> hud;
                ImVector<ImDrawCmd> world;
            };

            ImGuiVRHelperPluginAPI::Client helper;
            ImGuiVRHelperPluginAPI::PanelHandle panel{};
            std::optional<WorldAnchor> active;
            std::array<Range, 3> starts;
            std::vector<Group> groups;
            std::vector<Range> ranges;
            std::vector<ImGuiVRHelperPluginAPI::WorldQuad> quads;

            void Reset() {
                panel = {};
                active.reset();
                groups.clear();
                ranges.clear();
                quads.clear();
            }

            void Begin(const WorldAnchor& anchor, ImDrawList* window) {
                active = anchor;
                const std::array lists{GetBackgroundDrawList(), window, GetForegroundDrawList()};
                for (size_t i = 0; i < lists.size(); ++i) {
                    starts[i] = {.list = lists[i], .group = groups.size()};
                    if (lists[i] && lists[i] != window) {
                        starts[i].firstIndex = static_cast<unsigned int>(lists[i]->IdxBuffer.Size);
                        starts[i].firstVertex = lists[i]->VtxBuffer.Size;
                    }
                }
            }

            void End() {
                if (!active) return;
                Group group{.anchor = *active};
                const auto firstRange = ranges.size();
                for (auto range : starts) {
                    if (!range.list) continue;
                    range.endIndex = static_cast<unsigned int>(range.list->IdxBuffer.Size);
                    range.endVertex = range.list->VtxBuffer.Size;
                    if (range.firstIndex == range.endIndex) continue;
                    for (int i = range.firstVertex; i < range.endVertex; ++i) {
                        const auto offset = range.list->VtxBuffer[i].pos - group.anchor.origin;
                        group.halfSize = ImMax(group.halfSize, {std::abs(offset.x), std::abs(offset.y)});
                    }
                    ranges.push_back(range);
                }
                active.reset();
                if (ranges.size() == firstRange) return;
                // Symmetric bounds keep the world anchor at the texture's centre,
                // including asymmetric pivots, margins and effect padding.
                constexpr float texturePadding = 2.0f;
                group.halfSize += ImVec2(texturePadding, texturePadding);
                groups.push_back(group);
            }

            void Pack(const ImVec2 panelSize, const float verticalFOV, const float coverage) {
                const auto columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(
                    static_cast<float>(groups.size()) * panelSize.x / panelSize.y))));
                const auto rows = (groups.size() + columns - 1) / columns;
                const ImVec2 cell{panelSize.x / columns, panelSize.y / static_cast<float>(rows)};
                const auto* nodes = RE::PlayerCharacter::GetSingleton()->GetVRNodeData();
                // Skyrim's VR world units, matching the helper's public world-position convention.
                constexpr float metersPerGameUnit = 0.01428f;
                const float metersPerUnit = metersPerGameUnit / nodes->RoomNode->world.scale;
                for (size_t i = 0; i < groups.size(); ++i) {
                    auto& group = groups[i];
                    const auto size = group.halfSize * 2.0f;
                    group.scale = std::min(cell.x / size.x, cell.y / size.y);
                    group.atlasMin = {cell.x * static_cast<float>(i % columns),
                                      cell.y * static_cast<float>(i / columns)};
                    const auto maximum = group.atlasMin + size * group.scale;
                    const float distance = group.anchor.position.GetDistance(nodes->HmdNode->world.translate);
                    const float metersPerPixel = distance * metersPerUnit * verticalFOV * coverage / GetIO().DisplaySize.y;
                    quads.push_back({group.atlasMin.x / panelSize.x, group.atlasMin.y / panelSize.y,
                                     maximum.x / panelSize.x, maximum.y / panelSize.y,
                                     {group.anchor.position.x, group.anchor.position.y, group.anchor.position.z},
                                     size.y * metersPerPixel});
                }
            }

            ImVec2 ToAtlas(const ImVec2 point, const Group& group) const {
                return (point - group.anchor.origin + group.halfSize) * group.scale + group.atlasMin;
            }

            std::vector<Commands> SplitCommands(const ImDrawData& data) const {
                std::vector<Commands> result;
                for (auto* list : data.CmdLists) {
                    auto& output = result.emplace_back();
                    output.list = list;
                    for (const auto& command : list->CmdBuffer) {
                        if (command.UserCallback) {
                            output.hud.push_back(command);
                            output.world.push_back(command);
                            continue;
                        }
                        const auto end = command.IdxOffset + command.ElemCount;
                        for (auto first = command.IdxOffset; first < end;) {
                            const Range* owner = nullptr;
                            auto next = end;
                            for (const auto& range : ranges) {
                                if (range.list != list || range.endIndex <= first) continue;
                                if (range.firstIndex <= first) {
                                    owner = &range;
                                    next = std::min(next, range.endIndex);
                                    break;
                                }
                                next = std::min(next, range.firstIndex);
                            }
                            auto part = command;
                            part.IdxOffset = first;
                            part.ElemCount = next - first;
                            if (owner) {
                                const auto& group = groups[owner->group];
                                const auto minimum = ImMax(group.atlasMin,
                                    ToAtlas({command.ClipRect.x, command.ClipRect.y}, group));
                                const auto maximum = ImMin(group.atlasMin + group.halfSize * (2.0f * group.scale),
                                    ToAtlas({command.ClipRect.z, command.ClipRect.w}, group));
                                part.ClipRect = {minimum.x, minimum.y, maximum.x, maximum.y};
                                output.world.push_back(part);
                            } else {
                                output.hud.push_back(part);
                            }
                            first = next;
                        }
                    }
                }
                return result;
            }

            void MoveVertices(const ImVec2 framebufferScale) const {
                for (const auto& range : ranges) {
                    const auto& group = groups[range.group];
                    for (int i = range.firstVertex; i < range.endVertex; ++i) {
                        auto& position = range.list->VtxBuffer[i].pos;
                        position = ToAtlas(position / framebufferScale, group);
                    }
                }
            }
        } world;

        void UpdateAspectRatio() {
            if (hud.aspectRatio > 0.0f) return;
            const auto openVR = RE::BSOpenVR::GetSingleton();
            if (!openVR || !openVR->vrSystem) return;

            float halfWidth = 0.0f;
            float above = 0.0f;
            float below = 0.0f;
            for (const auto eye : {vr::Eye_Left, vr::Eye_Right}) {
                float left, right, top, bottom;
                openVR->vrSystem->GetProjectionRaw(eye, &left, &right, &top, &bottom);
                halfWidth = std::max({halfWidth, -left, right});
                above = std::max(above, -top);
                below = std::max(below, bottom);
            }
            // The helper's head-centred HUD spans the widest half-FOV of either eye.
            if (above + below > 0.0f) {
                hud.aspectRatio = 2.0f * halfWidth / (above + below);
                hud.verticalFOV = above + below;
            }
        }

        bool HasWorldTracking() {
            const auto* player = RE::PlayerCharacter::GetSingleton();
            const auto* nodes = player ? player->GetVRNodeData() : nullptr;
            return nodes && nodes->HmdNode && nodes->RoomNode && nodes->RoomNode->world.scale > 0.0f;
        }

        void ScaleDrawData(ImDrawData& data) {
            const auto scale = data.FramebufferScale;
            if (scale == ImVec2(1.0f, 1.0f)) return;

            // Our DX11 backend predates framebuffer scaling support.
            for (const auto commands : data.CmdLists) {
                for (auto& vertex : commands->VtxBuffer) {
                    vertex.pos = vertex.pos * scale;
                }
            }
            data.ScaleClipRects(scale);
            data.DisplayPos = data.DisplayPos * scale;
            data.DisplaySize = data.DisplaySize * scale;
            data.FramebufferScale = ImVec2(1.0f, 1.0f);
        }

        void RenderPanels(ID3D11DeviceContext* context) {
            auto& data = *GetDrawData();
            if (world.groups.empty() || !HasWorldTracking()) {
                world.helper.SubmitWorldQuads(nullptr, 0);
                ScaleDrawData(data);
                hud.helper.RenderFrame(context);
                return;
            }

            const ImVec2 panelSize{static_cast<float>(world.panel.width), static_cast<float>(world.panel.height)};
            world.Pack(panelSize, hud.verticalFOV, hud.helper.GetHudCoverage());
            auto commands = world.SplitCommands(data);
            for (auto& item : commands) item.list->CmdBuffer.swap(item.hud);
            const auto scale = data.FramebufferScale;
            ScaleDrawData(data);
            hud.helper.RenderFrame(context);

            // Reuse the existing mesh; only attached ranges go into the world atlas.
            world.MoveVertices(scale);
            for (auto& item : commands) item.list->CmdBuffer.swap(item.world);
            const auto displaySize = data.DisplaySize;
            data.DisplaySize = panelSize;
            world.helper.RenderToPanel(context);
            world.helper.SubmitWorldQuads(world.quads.data(), world.quads.size());
            data.DisplaySize = displaySize;
            for (auto& item : commands) item.list->CmdBuffer.swap(item.world);
        }

    }

    void Connect() {
        if (!REL::Module::IsVR()) return;

        const auto plugin = SKSE::PluginDeclaration::GetSingleton();
        if (hud.helper.Connect(plugin->GetName().data(), plugin->GetVersion().string(".").c_str(),
                               ImGuiVRHelperPluginAPI::kClientFlag_HUDMode)) {
            logger::info("ImGui VR Helper connected for prompt rendering.");
            if (hud.helper.HasWorldQuads()) {
                const auto name = std::string(plugin->GetName()) + " World";
                world.helper.Connect(name.c_str(), plugin->GetVersion().string(".").c_str(),
                                     ImGuiVRHelperPluginAPI::kClientFlag_WorldQuad);
            }
        } else {
            logger::warn("ImGui VR Helper is unavailable; prompts will only appear on the desktop.");
        }
    }

    void PrepareFrame(const ImVec2& previousDisplaySize) {
        world.Reset();
        if (!hud.helper.ApplyPanelDisplaySize()) return;
        if (world.helper.IsConnected()) {
            world.helper.Helper()->GetPanel(world.helper.Id(), &world.panel);
        }
        UpdateAspectRatio();
        auto& io = GetIO();
        if (hud.aspectRatio > 0.0f) {
            const auto panelWidth = io.DisplaySize.x;
            io.DisplaySize.x = io.DisplaySize.y * hud.aspectRatio;
            io.DisplayFramebufferScale.x = panelWidth / io.DisplaySize.x;
        }
        if (previousDisplaySize != io.DisplaySize) {
            Styles::RefreshStyle();
        }
    }

    void Render(ID3D11DeviceContext* context, const bool draw) {
        if (draw) {
            if (hud.helper.IsConnected()) RenderPanels(context);
            else hud.helper.RenderFrame(context);
            return;
        }
        world.helper.SubmitWorldQuads(nullptr, 0);
        world.Reset();
        if (!hud.helper.IsConnected() || !context) return;

        ImGuiVRHelperPluginAPI::PanelHandle panel{};
        if (hud.helper.Helper()->GetPanel(hud.helper.Id(), &panel) && panel.rtv) {
            constexpr float transparent[] = {0.0f, 0.0f, 0.0f, 0.0f};
            context->ClearRenderTargetView(panel.rtv, transparent);
        }
    }

    std::optional<WorldAnchor> GetWorldAnchor(const RE::NiPoint3& position) {
        if (!world.panel.rtv || world.panel.width == 0 || world.panel.height == 0 ||
            !HasWorldTracking() || hud.verticalFOV <= 0.0f) return std::nullopt;
        return WorldAnchor{position, GetIO().DisplaySize * 0.5f};
    }

    const std::optional<WorldAnchor>& GetCurrentWorldAnchor() {
        return world.active;
    }

    void BeginWorldPrompt(const WorldAnchor& anchor, ImDrawList* window) {
        world.Begin(anchor, window);
    }

    void EndWorldPrompt() {
        world.End();
    }
}

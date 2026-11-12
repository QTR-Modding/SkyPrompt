#include "Renderer.h"
#include "CLibUtilsQTR/BoundingBox.hpp"
#include "Hooks.h"
#include "CLibUtilsQTR/Tasker.hpp"
#include "IconsFonts.h"
#include "Styles.h"
#include "Utils.h"
#include "Service.h"
#include "Tutorial.h"
#include "API/HandshakeRegistry.h"


using namespace ImGui::Renderer;

float ImGui::Renderer::GetResolutionScale() {
    const auto height = ImGui::GetIO().DisplaySize.y;
    return DisplayTweaks::borderlessUpscale ? DisplayTweaks::resolutionScale : height / 1080.0f;
}

void ImGui::Renderer::RenderPrompts() {
    const auto manager = MANAGER(ImGui::Renderer);
    manager->SendEvents();
    manager->CleanUpQueue();

    if (MCP::Settings::shouldReloadLifetime.exchange(false)) {
        manager->activationPop.Clear();
        manager->ResetQueue();
        return;
    }

    if (MCP::Settings::shouldReloadPromptSize.exchange(false)) {
        manager->activationPop.Clear();
        Styles::GetSingleton()->RefreshStyle();
        return;
    }

    if (manager->IsPaused()) {
        manager->Start();
    }
    if (manager->HasTask()) {
        manager->Render();
    }
    if (Manager::IsGameFrozen() && !Tutorial::showing_tutorial.load()) {
        manager->activationPop.Clear();
    } else {
        manager->activationPop.Draw();
    }
}


namespace {
    float ButtonStateToFloat(const ButtonState& a_button_state) {
        auto a_press_count = a_button_state.pressCount;
        if (a_button_state.isPressing && a_button_state.pressCount < 3) {
            a_press_count--;
        }
        return static_cast<float>(a_press_count) + 0.1f;
    }

    std::pair<int, float> splitFloat(const float x) {
        float int_part_f;
        float frac_part = std::modf(x, &int_part_f);
        int int_part = static_cast<int>(std::abs(int_part_f)); // always positive
        return {int_part, frac_part}; // frac_part keeps original sign
    }

    float GetSecondsSinceLastFrame() {
        return RE::GetSecondsSinceLastFrame() / (RE::BSTimer::QGlobalTimeMultiplier() + EPSILON);
    }
}

float InteractionButton::GetProgressOverride(const bool increment) const {
    if (PromptTypeFlags::GetHasProgress(type)) {
        return 0.f;
    }
    if (!increment) {
        return mutables.progress; // Return current progress if not incrementing
    }

    const auto [mult, prog] = splitFloat(mutables.progress);

    if (mult == 0) return prog;

    if (const auto new_frac = prog - mult * 0.1f * GetSecondsSinceLastFrame();
        new_frac < -1.f || std::signbit(new_frac) != std::signbit(prog)) {
        mutables.progress = static_cast<float>(mult);
    } else {
        mutables.progress = (std::signbit(new_frac) ? -1 : 1) * mult + new_frac;
    }
    return std::abs(prog);
}

void ButtonQueue::Clear() {
    current_button = nullptr;
    buttons.clear();
    Reset();
}

void ButtonQueue::Reset() {
    lifetime = MCP::Settings::lifetime;
    alpha = 0.0f; // Reset alpha to start fade-in
    elapsed = 0.0f;
}

void ButtonQueue::WakeUp() {
    // wake up all buttons
    lifetime = MCP::Settings::lifetime;
    alpha = 1.0f;
    elapsed = 0.0f;
}

void ButtonQueue::Show(const Theme::Theme& theme, float progress, const InteractionButton* button2show, const ButtonState& a_button_state) {
    if (button2show) {
        Reset();
        current_button = button2show;
        return;
    }
    if (!current_button) {
        current_button = Next();
        return;
    }

    const auto seconds = GetSecondsSinceLastFrame();
    if (expired()/* && current_button->alpha>0.f*/) {
        alpha = std::max(alpha - theme.fadeSpeed * seconds * 120.f, 0.0f);
    } else {
        alpha = std::min(alpha + theme.fadeSpeed * seconds * 120.f, 1.0f);
    }
    if (Tutorial::showing_tutorial.load() || !Manager::IsGameFrozen()) {
        elapsed += seconds;
    }

    std::string extra_text;
    if (const auto total = buttons.size(); total > 1) {
        if (const auto it = buttons.find(*current_button); it != buttons.end()) {
            const auto index = std::distance(buttons.begin(), it) + 1;
            extra_text = fmt::format(" ({}/{})", index, total);
        } else {
            logger::error("Current button not found in the queue");
            current_button = Next();
            if (!current_button) return;
        }
    }

    const auto button_type = current_button->type;
    const bool has_progress = PromptTypeFlags::GetHasProgress(button_type);
    if (const auto progress_override = current_button->GetProgressOverride(true); progress_override > EPSILON) {
        progress = has_progress ? progress_override : -progress_override;
        WakeUp();
    }

    if (current_button->HasMissingAttachedRef()) {
        return;
    }

    auto button_state = has_progress ? ButtonStateToFloat(a_button_state) : -1.f;

    const auto buttonKey = current_button->GetKey(theme);
    const auto icon_manager = MANAGER(IconFont);
    if (icon_manager->unavailable_keys.contains(buttonKey)) return;
    const std::string base_text = current_button->mutables.text; // Cache the base text
    std::string a_text;
    a_text.reserve(base_text.size() + 1 + extra_text.size()); // Reserve memory to avoid reallocations
    a_text.append(base_text);
    if (!extra_text.empty()) {
        // If extra_text is not empty
        a_text.append(" ").append(extra_text);
    }

    const IconFont::IconTexture* buttonIcon = icon_manager->GetIcon(buttonKey);
    if (!buttonIcon) {
        logger::error("Button icon not found for key {}", buttonKey);
        icon_manager->unavailable_keys.insert(buttonKey);
        return;
    }

    if (buttonIcon->srView.Get()) {
        ImGui::renderBatch.emplace_back(a_text.c_str(), current_button->mutables.text_color, buttonIcon, progress,
                                        button_state, alpha, current_button->interaction);
    } else {
        logger::error("Button icon texture not loaded for key {}", buttonKey);
        icon_manager->unavailable_keys.insert(buttonKey);
    }
}


const InteractionButton* ButtonQueue::AddButton(const InteractionButton& a_button) {
    if (const auto [fst, snd] = buttons.insert(a_button); snd) {
        return &*fst;
    }
    return nullptr;
}

bool ButtonQueue::RemoveButton(const Interaction& a_interaction) {
    const auto it = std::ranges::find_if(buttons,
                                       [&](const auto& button) { return button.interaction == a_interaction; });
    if (it == buttons.end()) {
        return false;
    }
    if (current_button == &*it) {
        current_button = buttons.size() > 1 ? Next() : nullptr;
    }
    buttons.erase(it);
    return true;
}

const InteractionButton* ButtonQueue::Next() const {
    if (current_button) {
        if (auto it = buttons.find(*current_button); it != buttons.end()) {
            ++it;
            if (it != buttons.end()) {
                return &*it;
            }
            return &*buttons.begin();
        }
    } else if (!buttons.empty()) {
        return &*buttons.begin();
    }
    return nullptr;
}

Manager::Group* Manager::GetGroup(const SkyPromptAPI::ClientID pageID, const SkyPromptAPI::ClientID groupID) {
    const auto page = pages.find(pageID);
    if (page == pages.end()) return nullptr;
    const auto group = page->second.find(groupID);
    return group != page->second.end() ? &group->second : nullptr;
}

bool Manager::Group::ContainsClient(const SkyPromptAPI::ClientID clientID) const {
    return clientID != 0 && std::ranges::any_of(managers, [clientID](const auto& manager) {
        return manager && manager->GetClientID() == clientID;
    });
}

bool Manager::Group::CanJoin(const SkyPromptAPI::ClientID clientID) const {
    bool found_client = false;
    for (const auto& manager : managers) {
        if (const auto rowClientID = manager->GetClientID(); rowClientID != 0) {
            found_client = true;
            if (!Handshake::compatibility.AreCompatible(clientID, rowClientID)) {
                return false;
            }
        }
    }
    return found_client;
}

void Manager::Group::CollectClients(std::set<SkyPromptAPI::ClientID>& clients) const {
    for (const auto& manager : managers) {
        if (const auto clientID = manager->GetClientID(); clientID != 0) clients.insert(clientID);
    }
}

void Manager::Group::CollectRowKeys(std::map<const SubManager*, uint32_t>& keys) const {
    for (const auto& manager : managers) {
        const bool receivesInput = theme->prompt_alignment != Theme::kList ||
                                   (list.selection < managers.size() && manager == managers[list.selection]);
        keys.emplace(manager.get(), receivesInput ? manager->GetPromptKey(*theme) : 0);
    }
}

void Manager::Group::MoveClientRowsTo(const SkyPromptAPI::ClientID clientID, Group& target) {
    const auto selected = list.selection < managers.size() ? managers[list.selection].get() : nullptr;
    for (size_t index = 0; index < managers.size();) {
        if (managers[index]->GetClientID() != clientID) {
            ++index;
            continue;
        }
        if (managers[index].get() == selected) target.list.selection = target.managers.size();
        target.managers.push_back(std::move(managers[index]));
        managers.erase(managers.begin() + index);
        list.OnRowRemoved(index);
    }
}

void Manager::Group::UpdateRowKeys(const std::map<const SubManager*, uint32_t>& previousKeys) {
    list.ClampSelection(managers.size());
    int index = 0;
    for (const auto& manager : managers) {
        const bool receivesInput = theme->prompt_alignment != Theme::kList || manager == managers[list.selection];
        manager->SetDefaultKeyIndex(*theme, index++, previousKeys.at(manager.get()), receivesInput);
    }
}

bool Manager::CanJoinPage(const SkyPromptAPI::ClientID pageID, const SkyPromptAPI::ClientID clientID) const {
    bool found_client = false;
    for (const auto& group : pages.at(pageID) | std::views::values) {
        if (!group.HasQueue()) continue;
        found_client = true;
        if (!group.CanJoin(clientID)) return false;
    }
    return found_client;
}

SkyPromptAPI::ClientID Manager::RegroupPage(const SkyPromptAPI::ClientID pageID,
                                           const SkyPromptAPI::ClientID addedClient) {
    auto& page = pages.at(pageID);
    std::set<SkyPromptAPI::ClientID> clients;
    std::map<const SubManager*, uint32_t> previousKeys;
    for (const auto& group : page | std::views::values) {
        group.CollectClients(clients);
        group.CollectRowKeys(previousKeys);
    }
    if (addedClient != 0) clients.insert(addedClient);
    if (clients.empty()) return 0;

    std::map<Theme::Theme*, SkyPromptAPI::ClientID> providers;
    auto fallback = &Theme::default_theme;
    for (const auto clientID : clients) {
        if (const auto request = requestedThemes.find(clientID); request != requestedThemes.end()) {
            providers.try_emplace(request->second, clientID);
            fallback = request->second;
        }
    }

    bool themesChanged = false;
    SkyPromptAPI::ClientID addedGroup = 0;
    for (const auto clientID : clients) {
        const auto request = requestedThemes.find(clientID);
        const auto theme = request != requestedThemes.end() ? request->second : fallback;
        const auto groupID = providers.size() <= 1 ? pageID : providers.at(theme);
        const auto [targetIt, inserted] = page.try_emplace(groupID);
        auto& target = targetIt->second;
        themesChanged |= inserted || target.theme != theme;
        target.theme = theme;
        if (clientID == addedClient) addedGroup = groupID;
        for (auto& [sourceID, source] : page) {
            if (sourceID != groupID) source.MoveClientRowsTo(clientID, target);
        }
    }
    for (auto& group : page | std::views::values) group.UpdateRowKeys(previousKeys);
    const auto removed = std::erase_if(page, [&](const auto& entry) {
        if (!entry.second.IsEmpty()) return false;
        if (providers.size() <= 1) return entry.first != pageID;
        return std::ranges::none_of(providers, [&](const auto& provider) { return provider.second == entry.first; });
    });
    if (pageID == current_pageID && (themesChanged || removed != 0)) MCP::refreshStyle.store(true);
    return addedGroup;
}

std::pair<SkyPromptAPI::ClientID, SkyPromptAPI::ClientID> Manager::FindGroupIDs(const SkyPromptAPI::ClientID clientID) const {
    const auto find_group = [clientID](const auto& pageGroups) {
        return std::ranges::find_if(pageGroups, [clientID](const auto& entry) {
            return entry.second.ContainsClient(clientID);
        });
    };
    if (const auto page = pages.find(current_pageID); page != pages.end()) {
        if (const auto group = find_group(page->second); group != page->second.end()) {
            return {current_pageID, group->first};
        }
    }

    for (const auto& [pageID, pageGroups] : pages) {
        if (pageID == current_pageID) continue;
        if (const auto group = find_group(pageGroups); group != pageGroups.end()) {
            return {pageID, group->first};
        }
    }

    return {};
}

bool Manager::InitializeClient(const SkyPromptAPI::ClientID a_clientID) {
    {
        std::shared_lock lock(mutex_);
        if (a_clientID == 0 || current_pageID == a_clientID || pages.contains(a_clientID)) {
            return false;
        }
    }

    {
        std::unique_lock lock(mutex_);
        pages.try_emplace(a_clientID).first->second.try_emplace(a_clientID);
    }

    return true;
}

bool Manager::IsGameFrozen() {
    if (const auto main = RE::Main::GetSingleton()) {
        if (main->GetRuntimeData().freezeTime) return true;
        if (!main->GetRuntimeData().gameActive) return true;
    } else return true;
    if (RE::UI::GetSingleton()->GameIsPaused()) return true;
    return false;
}

void SubManager::SendEvent(const Interaction& a_interaction, const SkyPromptAPI::PromptEventType event_type,
                           const std::pair<float, float> delta, const float progress_override) {
    const SkyPromptAPI::EventID a_event = InteractionID::Local(a_interaction.event);
    const SkyPromptAPI::ActionID a_action = InteractionID::Local(a_interaction.action);
    std::shared_lock lock(sink_mutex_);
    if (const auto it = sinks.find(a_interaction); it != sinks.end()) {
        for (const auto& a_sink : it->second) {
            if (!a_sink) continue;
            for (const auto prompts = a_sink->GetPrompts(); const auto& prompt : prompts) {
                if (prompt.eventID == a_event && prompt.actionID == a_action) {
                    SkyPromptAPI::Prompt a_prompt = prompt;
                    if (std::abs(progress_override) > 0.f) {
                        a_prompt.progress = progress_override;
                    }
                    Manager::GetSingleton()->AddEventToSend(InteractionID::Client(a_interaction.event), a_sink,
                                                           a_prompt, event_type, delta);
                }
            }
        }
    }
}

namespace {
    ImVec2 WorldToScreenLoc(const RE::NiPoint3 position) {
        static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address(); // 2F4C910, 2FE75F0
        static auto g_viewPort = (RE::NiRect<float>*)RELOCATION_ID(519618, 406160).address(); // 2F4DED0, 2FE8B98

        ImVec2 screenLocOut;
        const RE::NiPoint3 niWorldLoc(position.x, position.y, position.z);

        float zVal;

        RE::NiCamera::WorldPtToScreenPt3((float(*)[4])g_worldToCamMatrix, *g_viewPort, niWorldLoc, screenLocOut.x,
                                         screenLocOut.y, zVal, 1e-5f);
        const ImVec2 rect = ImGui::GetIO().DisplaySize;

        screenLocOut.x = rect.x * screenLocOut.x;
        screenLocOut.y = 1.0f - screenLocOut.y;
        screenLocOut.y = rect.y * screenLocOut.y;

        return screenLocOut;
    }

    ImVec2 WorldToScreenLoc(const RE::NiPoint3 position, const RE::NiPointer<RE::NiCamera>& a_cam) {
        float z;
        ImVec2 screenLocOut;
        RE::NiCamera::WorldPtToScreenPt3(a_cam->GetRuntimeData().worldToCam, a_cam->GetRuntimeData2().port,
                                         position, screenLocOut.x, screenLocOut.y, z, 1e-5f);
        const ImVec2 rect = ImGui::GetIO().DisplaySize;
        screenLocOut.x = rect.x * screenLocOut.x;
        screenLocOut.y = 1.0f - screenLocOut.y;
        screenLocOut.y = rect.y * screenLocOut.y;
        return screenLocOut;
    }

    void OffsetPosition(const RE::NiPoint3& a_pos, const RE::NiPoint3& a_cam_pos, RE::NiPoint3& a_out,
                        const float a_offset_x, const float a_offset_y) {
        const auto diff = a_pos - a_cam_pos;
        constexpr RE::NiPoint3 z_vec(0.f, 0.f, 1.f);
        const auto right_vec = diff.UnitCross(z_vec);
        a_out = a_pos + right_vec * a_offset_x + z_vec * a_offset_y;
    }

    constexpr float CLAMP_MAX_OVERSHOOT = -100;

    void FastClampToScreen(ImVec2& point) {
        const ImVec2 rect = ImGui::GetIO().DisplaySize;
        if (point.x < 0.0) {
            const float overshootX = abs(point.x);
            if (overshootX > CLAMP_MAX_OVERSHOOT) point.x += overshootX - CLAMP_MAX_OVERSHOOT;
        } else if (point.x > rect.x) {
            const float overshootX = point.x - rect.x;
            if (overshootX > CLAMP_MAX_OVERSHOOT) point.x -= overshootX - CLAMP_MAX_OVERSHOOT;
        }

        if (point.y < 0.0) {
            const float overshootY = abs(point.y);
            if (overshootY > CLAMP_MAX_OVERSHOOT) point.y += overshootY - CLAMP_MAX_OVERSHOOT;
        } else if (point.y > rect.y) {
            const float overshootY = point.y - rect.y;
            if (overshootY > CLAMP_MAX_OVERSHOOT) point.y -= overshootY - CLAMP_MAX_OVERSHOOT;
        }
    }
}

SubManager::AttachedPosition SubManager::GetAttachedObjectPos(const Theme::Theme& theme) const {
    if (const auto ref = GetAttachedObject()) {
        constexpr float padding = 10.f;
        RE::NiPoint3 pos;
        ImVec2 pos2d;

        if (const auto temp_ref = RE::Inventory3DManager::GetSingleton()->tempRef;
            temp_ref && ref->GetFormID() == temp_ref->GetFormID()) {
            if (const auto inv3dmngr = RE::Inventory3DManager::GetSingleton(); !inv3dmngr->GetRuntimeData().loadedModels
                .empty()) {
                if (const auto& model = inv3dmngr->GetRuntimeData().loadedModels.back().spModel) {
                    OffsetPosition(model->world.translate, RE::UI3DSceneManager::GetSingleton()->cachedCameraPos, pos,
                                   model->worldBound.radius - theme.attachedOffsetX, theme.attachedOffsetY);
                    pos2d = WorldToScreenLoc(pos, RE::UI3DSceneManager::GetSingleton()->camera) + ImVec2{
                                (theme.prompt_size + padding) * DisplayTweaks::resolutionScale, 0};
                }
            }
            FastClampToScreen(pos2d);
            return {pos2d, std::nullopt};
        }
        const auto* vrNodes = REL::Module::IsVR() ? RE::PlayerCharacter::GetSingleton()->GetVRNodeData() : nullptr;
        const auto cameraPos = vrNodes && vrNodes->HmdNode ? vrNodes->HmdNode->world.translate
            : RE::PlayerCamera::GetSingleton()->GetRuntimeData2().pos;
        if (const auto a_head = [&]() -> RE::NiAVObject* {
            if (const auto actor = ref->As<RE::Actor>()) {
                if (const auto middle = actor->GetMiddleHighProcess()) {
                    return middle->headNode;
                }
            }
            return nullptr;
        }()) {
            constexpr float npc_head_size = 15.f;
            const float objectScale = ref->GetScale();
            const auto npc_head_pos = a_head->world.translate;
            const auto diff = npc_head_pos - cameraPos;
            constexpr RE::NiPoint3 z_vec(0.f, 0.f, 1.f);
            const auto right_vec = diff.UnitCross(z_vec);
            pos = npc_head_pos + right_vec * (npc_head_size * objectScale);
            pos2d = ImVec2{
                        (theme.prompt_size + padding) * DisplayTweaks::resolutionScale, 0};
        } else {
            DirectX::BoundingOrientedBox bounding_box;
            BoundingBox::GetOBB(ref, bounding_box, true);

            const auto center = bounding_box.Center;

            pos = RE::NiPoint3{center.x, center.y, center.z + bounding_box.Extents.z + 10.f};
            pos2d = ImVec2{0.f, -(theme.prompt_size + padding) * DisplayTweaks::resolutionScale};
        }

        OffsetPosition(pos, cameraPos, pos, -theme.attachedOffsetX, theme.attachedOffsetY);

        if (auto world = VR::GetWorldAnchor(pos)) {
            return {world->origin + pos2d, world};
        }
        pos2d += WorldToScreenLoc(pos);
        FastClampToScreen(pos2d);

        return {pos2d, std::nullopt};
    }
    return {};
}

RE::TESObjectREFR* SubManager::GetAttachedObject() const {
    std::shared_lock lock(q_mutex_);
    if (const auto curr_button = interactQueue.current_button) {
        return curr_button->attached_object.get().get();
    }
    return nullptr;
}

bool SubManager::Update(const Interaction& a_interaction, const ButtonMutables& a_mutables) const {
    std::unique_lock lock(q_mutex_);
    for (const auto& a_button : interactQueue.buttons) {
        if (a_button.interaction == a_interaction) {
            a_button.mutables = a_mutables;
            return true;
        }
    }
    return false;
}

void SubManager::RestoreMutables(const Interaction& a_interaction) {
    auto& owners = sinks.at(a_interaction);
    for (size_t index = owners.size(); index > 0;) {
        const auto owner = owners[--index];
        for (const auto prompts = owner->GetPrompts(); const auto& prompt : prompts) {
            if (prompt.eventID != InteractionID::Local(a_interaction.event) ||
                prompt.actionID != InteractionID::Local(a_interaction.action) || prompt.text.empty()) {
                continue;
            }
            auto text = std::string(prompt.text);
            TranslateEmbedded(text);
            Update(a_interaction, {text, prompt.text_color, prompt.progress});
            std::rotate(owners.begin() + index, owners.begin() + index + 1, owners.end());
            return;
        }
    }
}

void SubManager::Show(const Theme::Theme& theme, const InteractionButton* button2show) {
    interactQueue.Show(theme, progress_circle, button2show, buttonState);
}

void SubManager::ButtonStateActions(const Theme::Theme& theme) {
    buttonState.pressCount = std::min(6, buttonState.pressCount);

    SkyPromptAPI::PromptType a_type;
    float progress_override;
    Interaction a_interaction;

    {
        std::shared_lock lock(q_mutex_);
        if (const auto button = interactQueue.current_button) {
            a_type = button->type;
            progress_override = button->GetProgressOverride(false);
            a_interaction = button->interaction;
        } else {
            return;
        }
    }

    if (PromptTypeFlags::GetHasProgress(a_type)) {
        if (const auto now = std::chrono::steady_clock::now(); !buttonState.isPressing) {
            if (now - buttonState.lastPressTime > maxIntervalBetweenPresses) {
                if (buttonState.pressCount == 2) {
                    RemoveFromQ(a_interaction);
                    SendEvent(a_interaction, SkyPromptAPI::PromptEventType::kDeclined);
                } else if (buttonState.pressCount == 3) {
                    NextPrompt(theme);
                }
                buttonState.pressCount = 0;
            } else if (buttonState.pressCount == 4 || buttonState.pressCount >= 6) {
                NextPrompt(theme);
                buttonState.pressCount = 5;
            }
        }
    } else {
        const auto [mult, frac] = splitFloat(progress_override);
        if (mult > 0.f && std::abs(frac) < EPSILON) {
            SendEvent(a_interaction, SkyPromptAPI::PromptEventType::kDeclined, {0.f, 0.f}, progress_override);
        }
    }
}

SubManager::~SubManager() {
    ClearQueue();
}

void SubManager::Add2Q(const Theme::Theme& theme, const InteractionButton& iButton, const bool show) {
    std::unique_lock lock(q_mutex_);
    if (const auto button = interactQueue.AddButton(iButton); button && show) {
        std::shared_lock lock2(progress_mutex_);
        if (!Manager::GetSingleton()->IsPaused() && progress_circle == 0.f) {
            Show(theme, button);
        }
    }
}

bool SubManager::RemoveFromQ(const Interaction& a_interaction) {
    std::unique_lock lock(q_mutex_);
    const auto current = interactQueue.current_button;
    const bool removingCurrent = current && current->interaction == a_interaction;
    if (!interactQueue.RemoveButton(a_interaction)) {
        return false;
    }
    if (removingCurrent) {
        ResetButtonState();
    }
    return true;
}

bool SubManager::RemoveFromQ(const SkyPromptAPI::ClientID a_clientID,
                            const SkyPromptAPI::PromptSink* a_prompt_sink,
                            const std::optional<Interaction>& a_interaction) {
    bool removed = false;
    std::unique_lock lock(sink_mutex_);
    for (auto it = sinks.begin(); it != sinks.end();) {
        const bool restore = !it->second.empty() && it->second.back() == a_prompt_sink;
        if (InteractionID::Client(it->first.event) != a_clientID ||
            (a_interaction && it->first != *a_interaction) || !std::erase(it->second, a_prompt_sink)) {
            ++it;
            continue;
        }
        removed = true;
        if (it->second.empty()) {
            RemoveFromQ(it->first);
            it = sinks.erase(it);
        } else {
            if (restore) RestoreMutables(it->first);
            ++it;
        }
    }
    return removed;
}

void SubManager::SetDefaultKeyIndex(const Theme::Theme& theme, const int index,
                                   const std::optional<uint32_t> previousKey, const bool receivesInput) {
    std::unique_lock lock(q_mutex_);
    const auto current = interactQueue.current_button;
    const auto oldKey = previousKey ? *previousKey : (current ? current->GetKey(theme) : 0);
    for (const auto& button : interactQueue.buttons) {
        button.default_key_index = index;
    }
    const auto newKey = current && receivesInput ? current->GetKey(theme) : 0;
    if (!receivesInput || newKey != oldKey) {
        ResetButtonState();
    }
}

void SubManager::ResetButtonState() {
    std::unique_lock lock(progress_mutex_);
    progress_circle = 0.0f;
    buttonState.Reset();
    blockProgress.store(false);
}

void SubManager::ResetQueue() {
    {
        std::unique_lock lock(q_mutex_);
        interactQueue.Reset();
    }
    ResetButtonState();
}

void SubManager::ShowQueue(const Theme::Theme& theme) {
    if (std::shared_lock lock(q_mutex_); !interactQueue.IsEmpty()) {
        const auto curr_ = interactQueue.current_button;
        if (!curr_ || interactQueue.IsHidden()) {
            {
                std::unique_lock lock2(progress_mutex_);
                progress_circle = 0.0f;
            }
        }
        if (interactQueue.expired()) {
            for (auto& button : interactQueue.buttons) {
                SendEvent(button.interaction, SkyPromptAPI::PromptEventType::kTimingOut);
            }
        }
        Show(theme, nullptr);
    }
}

void SubManager::WakeUpQueue() {
    std::unique_lock lock(q_mutex_);
    interactQueue.WakeUp();
    wakeup_queued_.store(false);
}

SubManager* Manager::Add2Q(
    const SkyPromptAPI::ClientID pageID, const SkyPromptAPI::ClientID groupID,
    const Interaction& a_interaction, const ButtonMutables& a_mutables,
    const SkyPromptAPI::PromptType a_type, const RefID a_refid, const std::map<Input::DEVICE, uint32_t>& a_bttn_map,
    const bool show) {
    std::unique_lock lock(mutex_);
    const auto group = GetGroup(pageID, groupID);
    if (!group) {
        return nullptr;
    }
    return group->Add2Q(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, show);
}

SubManager* Manager::Group::Add2Q(
    const Interaction& a_interaction, const ButtonMutables& a_mutables,
    const SkyPromptAPI::PromptType a_type, const RefID a_refid, const std::map<Input::DEVICE, uint32_t>& a_bttn_map,
    const bool show) {
    auto& manager_list = managers;

    for (const auto& a_manager : manager_list) {
        if (a_manager->Update(a_interaction, a_mutables)) {
            return a_manager.get();
        }
    }

    int index = 0;
    for (const auto& a_manager : manager_list) {
        if (const auto event = a_manager->GetQueuedEvent(); event == 0) {
            const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
            a_manager->Add2Q(*theme, iButton, show);
            return a_manager.get();
        } else if (event == a_interaction.event) {
            a_manager->WakeUpQueue();
            const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
            a_manager->Add2Q(*theme, iButton, show);
            return a_manager.get();
        }
        ++index;
    }

    {
        std::shared_lock theme_lock(Theme::m_theme_);
        if (theme->prompt_alignment != Theme::kList && manager_list.size() >= theme->n_max_buttons) {
            return nullptr;
        }
    }
    // If no manager has the event, make a new row.
    manager_list.emplace_back(std::make_unique<SubManager>());

    const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
    manager_list.back()->Add2Q(*theme, iButton, show);
    return manager_list.back().get();
}

bool Manager::RequestTheme(const SkyPromptAPI::ClientID a_clientID, const std::string_view theme_name) {
    std::unique_lock lock(mutex_);
    std::unique_lock theme_lock(Theme::m_theme_);
    if (!pages.contains(a_clientID)) return false;
    auto& theme = requestedThemes[a_clientID];
    theme = &Theme::default_theme;
    bool success = true;
    if (!theme_name.empty()) {
        const auto it = Theme::themes_loaded.find(std::string(theme_name));
        if (it == Theme::themes_loaded.end()) {
            logger::error("Theme not found: {}", theme_name);
            success = false;
        } else if (!it->second.ReLoad(theme_name)) {
            success = false;
        } else {
            theme = &it->second;
        }
    }
    const auto pageID = FindGroupIDs(a_clientID).first;
    if (pageID != 0) {
        RegroupPage(pageID);
        if (pageID == current_pageID) MCP::refreshStyle.store(true);
    }
    return success;
}

std::vector<const Theme::Theme*> Manager::GetCurrentThemes() const {
    std::shared_lock lock(mutex_);
    std::vector<const Theme::Theme*> themes;
    if (const auto page = pages.find(current_pageID); page != pages.end()) {
        for (const auto& group : page->second | std::views::values) {
            if (std::ranges::find(themes, group.theme) == themes.end()) themes.push_back(group.theme);
        }
    }
    if (themes.empty()) themes.push_back(&Theme::default_theme);
    return themes;
}

void Manager::PrepareFonts() {
    const float resolutionScale = GetResolutionScale();
    std::set<std::pair<std::string, float>> requiredFonts;
    {
        std::shared_lock lock(mutex_);
        std::shared_lock theme_lock(Theme::m_theme_);
        if (const auto page = pages.find(current_pageID); page != pages.end()) {
            for (const auto& group : page->second | std::views::values) {
                requiredFonts.emplace(group.theme->font_name, group.theme->prompt_size * resolutionScale);
            }
        } else {
            requiredFonts.emplace(Theme::default_theme.font_name, Theme::default_theme.prompt_size * resolutionScale);
        }
    }

    const auto fontManager = MANAGER(IconFont);
    activationPop.Clear();
    if (fontManager->ReloadFonts(requiredFonts)) return;

    {
        std::unique_lock lock(mutex_);
        std::unique_lock theme_lock(Theme::m_theme_);
        Theme::default_theme = Theme::Theme();
        if (const auto page = pages.find(current_pageID); page != pages.end()) {
            for (auto& [clientID, theme] : requestedThemes) {
                if (std::ranges::any_of(page->second | std::views::values, [clientID](const auto& group) {
                    return group.ContainsClient(clientID);
                })) theme = &Theme::default_theme;
            }
            RegroupPage(current_pageID);
        }
        requiredFonts.clear();
        requiredFonts.emplace(Theme::default_theme.font_name, Theme::default_theme.prompt_size * resolutionScale);
    }
    if (!fontManager->ReloadFonts(requiredFonts)) return;
}

bool Manager::SwitchToPage(const SkyPromptAPI::ClientID pageID) {
    if (std::shared_lock lock(mutex_); pageID == current_pageID) {
        return false;
    }

    if (!MCP::Settings::cycle_controls.load()) {
        Clear(SkyPromptAPI::kRemovedByMod);
    } else {
        CleanUpQueue();
        ResetQueue();
    }

    std::unique_lock lock(mutex_);

    const auto previous = pages.find(current_pageID);
    current_pageID = pageID;
    auto& page = pages.at(pageID);
    for (auto& group : page | std::views::values) {
        group.OnShown();
    }

    const bool themesChanged = previous != pages.end()
        ? !std::ranges::equal(previous->second | std::views::values, page | std::views::values,
                              {}, &Group::theme, &Group::theme)
        : std::ranges::any_of(page | std::views::values, [](const auto& group) {
            return group.theme != &Theme::default_theme;
        });
    if (themesChanged) {
        MCP::refreshStyle.store(true);
    }

    return true;
}

bool Manager::CyclePage(const bool a_left) {
    std::shared_lock lock(mutex_);
    if (pages.size() <= 1) {
        return false;
    }
    const bool any_has_queue = std::ranges::any_of(pages, [this](const auto& entry) {
        return entry.first != current_pageID &&
               std::ranges::any_of(entry.second | std::views::values, &Group::HasQueue);
    });

    if (!any_has_queue) {
        return false;
    }

    auto it = pages.find(current_pageID);
    while (it->first == current_pageID ||
           std::ranges::all_of(it->second | std::views::values, &Group::IsEmpty)) {
        if (a_left) {
            if (it == pages.begin()) {
                it = std::prev(pages.end());
            } else {
                --it;
            }
        } else {
            ++it;
            if (it == pages.end()) {
                it = pages.begin();
            }
        }
    }

    lock.unlock();
    return SwitchToPage(it->first);
}

bool Manager::Add2Q(const SkyPromptAPI::PromptSink* a_prompt_sink, const SkyPromptAPI::ClientID a_clientID) {
    bool refresh = false;
    SkyPromptAPI::ClientID pageID, groupID;
    {
        std::shared_lock lock(mutex_);
        if (!pages.contains(a_clientID)) return false;
        std::tie(pageID, groupID) = FindGroupIDs(a_clientID);
        if (const auto group = GetGroup(pageID, groupID)) {
            refresh = group->WakeUpPrompt(a_prompt_sink);
        }
    }

    const bool newClient = pageID == 0;
    if (newClient) {
        std::unique_lock lock(mutex_);
        pageID = a_clientID;
        if (current_pageID != 0 && current_pageID != a_clientID && CanJoinPage(current_pageID, a_clientID)) {
            pageID = current_pageID;
        }
        std::shared_lock theme_lock(Theme::m_theme_);
        groupID = RegroupPage(pageID, a_clientID);
    }

    const auto prompts = a_prompt_sink->GetPrompts();
    bool success = true;
    for (const auto& [text, a_event, a_action, a_type, a_refid, button_key, text_color, progress] : prompts) {
        auto a_txt = std::string(text);
        if (a_txt.empty()) {
            logger::warn("Empty prompt text");
            success = false;
            break;
        }

        TranslateEmbedded(a_txt);

        std::map<Input::DEVICE, uint32_t> temp_button_keys;
        for (const auto& [a_device, key] : button_key) {
            Input::DEVICE device = Input::from_RE_device(a_device);
            if (device == Input::DEVICE::kUnknown) {
                continue;
            }
            if (const auto a_new_key = MANAGER(Input)->Convert(key, a_device)) {
                temp_button_keys[device] = a_new_key;
            }
        }
        const auto interaction = MakeInteraction(a_clientID, a_event, a_action);
        if (const auto submanager =
            Add2Q(pageID, groupID, interaction, {.text = a_txt, .text_color = text_color, .progress = progress},
                  a_type, a_refid,
                  temp_button_keys, !refresh)) {
            submanager->AddSink(interaction, a_prompt_sink);
        } else {
            logger::warn("Failed to add interaction to the queue");
            success = false;
            if (!refresh) break;
        }
    }

    if (newClient && (!success || prompts.empty())) {
        std::unique_lock lock(mutex_);
        std::shared_lock theme_lock(Theme::m_theme_);
        RegroupPage(pageID);
    }
    if (refresh) {
        RestorePromptOrder(pageID, groupID, a_clientID, prompts);
    } else if (success) {
        SwitchToPage(pageID);
    }

    return success && !refresh;
}

void Manager::RestorePromptOrder(const SkyPromptAPI::ClientID pageID, const SkyPromptAPI::ClientID groupID,
                                 const SkyPromptAPI::ClientID clientID,
                                 const std::span<const SkyPromptAPI::Prompt> prompts) {
    std::unique_lock lock(mutex_);
    const auto group = GetGroup(pageID, groupID);
    if (!group) return;
    group->RestorePromptOrder(clientID, prompts, pageID == current_pageID);
}

void Manager::Group::RestorePromptOrder(const SkyPromptAPI::ClientID clientID,
                                      const std::span<const SkyPromptAPI::Prompt> prompts, const bool active) {
    if (managers.size() < 2) return;
    auto& rows = managers;

    bool moved = false;
    auto before = rows.end();
    for (size_t index = prompts.size(); index > 0;) {
        const auto& prompt = prompts[--index];
        // A row follows the first appearance of its event, not each stacked action.
        if (std::ranges::any_of(prompts.first(index), [&](const auto& earlier) {
            return earlier.eventID == prompt.eventID;
        })) continue;

        const auto event = InteractionID::Pack(clientID, prompt.eventID);
        const auto row = std::ranges::find_if(rows, [event](const auto& candidate) {
            return candidate->GetQueuedEvent() == event;
        });
        if (row == rows.end()) continue;

        if (row > before) {
            if (active) list.OnRowRestored(row - rows.begin(), before - rows.begin());
            std::rotate(before, row, std::next(row));
            moved = true;
        } else {
            before = row;
        }
    }
    if (moved) {
        int index = 0;
        for (const auto& row : rows) row->SetDefaultKeyIndex(*theme, index++);
        if (active) list.ClampSelection(rows.size());
    }
}

bool Manager::Group::WakeUpPrompt(const SkyPromptAPI::PromptSink* a_prompt_sink) const {
    bool result = false;
    for (const auto& a_manager : managers) {
        if (a_manager->IsInQueue(a_prompt_sink)) {
            result = true;
            a_manager->WakeUpQueue();
        }
    }
    return result;
}

bool Manager::Group::RemoveFromQ(const SkyPromptAPI::ClientID clientID, const SkyPromptAPI::PromptSink* sink,
                               const std::optional<Interaction>& interaction, const bool active) {
    bool removed = false;
    for (const auto& manager : managers) {
        removed |= manager->RemoveFromQ(clientID, sink, interaction);
    }
    if (!active) {
        RemoveEmptyRows(false);
    }
    return removed;
}

bool Manager::RemoveFromQ(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink,
                         const std::optional<Interaction>& a_interaction) {
    bool removed = false;
    {
        std::unique_lock lock(mutex_);
        const auto [pageID, groupID] = FindGroupIDs(a_clientID);
        if (const auto group = GetGroup(pageID, groupID)) {
            removed = group->RemoveFromQ(a_clientID, a_prompt_sink, a_interaction, pageID == current_pageID);
            if (removed && pageID != current_pageID) {
                std::shared_lock theme_lock(Theme::m_theme_);
                RegroupPage(pageID);
            }
        }
    }
    {
        std::unique_lock lock(events_to_send_mutex);
        const auto it = events_to_send_.find({a_clientID, a_prompt_sink});
        if (it != events_to_send_.end()) {
            if (a_interaction) {
                std::erase_if(it->second, [&](const auto& event) {
                    return MakeInteraction(a_clientID, event.prompt.eventID, event.prompt.actionID) == *a_interaction;
                });
            }
            if (!a_interaction || it->second.empty()) {
                events_to_send_.erase(it);
            }
        }
    }

    if (removed) {
        CleanUpQueue();
    }
    return removed;
}

bool Manager::HasTask() const {
    std::shared_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    return page != pages.end() && std::ranges::any_of(page->second | std::views::values, [](const auto& group) {
        return group.HasQueue();
    });
}

bool Manager::Group::HasQueue() const {
    for (const auto& manager : managers) {
        if (manager && manager->HasQueue()) {
            return true;
        }
    }
    return false;
}

void SubManager::CleanUpQueue(const Theme::Theme& theme) {
    // if everything has expired AND has alpha=0, clear the queue
    if (std::unique_lock lock(q_mutex_);
        interactQueue.expired() && interactQueue.alpha <= 0.f
    ) {
        lock.unlock();
        ClearQueue(SkyPromptAPI::kTimeout);
    }
    ButtonStateActions(theme);
}

void SubManager::ClearQueue() {
    {
        std::unique_lock lock(q_mutex_);
        interactQueue.Clear();
    }
    {
        std::unique_lock lock(progress_mutex_);
        progress_circle = 0.0f;
    }

    blockProgress.store(false);
}

void SubManager::ClearQueue(const SkyPromptAPI::PromptEventType a_event_type) {
    {
        std::shared_lock lock(q_mutex_);
        for (const auto& a_button : interactQueue.buttons) {
            SendEvent(a_button.interaction, a_event_type);
        }
    }
    ClearQueue();
}

bool SubManager::HasQueue() const {
    std::shared_lock lock(q_mutex_);
    return !interactQueue.IsEmpty();
}

void SubManager::Start() {
    blockProgress.store(false);
}

void SubManager::Stop() {
    ResetQueue();
    blockProgress.store(false);
}

bool SubManager::UpdateProgressCircle(const Theme::Theme& theme, const bool isPressing) {
    if (!wakeup_queued_.load()) {
        wakeup_queued_.store(true);
        clib_utilsQTR::Tasker::GetSingleton()->PushTask([] {
                                                            Manager::GetSingleton()->WakeUpQueue();
                                                        },
                                                        100
            );
    }

    if (!isPressing) {
        std::unique_lock lock(progress_mutex_);
        buttonState.acceptedThisPress = false;
        if (progress_circle > theme.progress_speed) {
            buttonState.pressCount = 0;
        }
        progress_circle = 0.0f;
        blockProgress.store(false);
        return false;
    }
    if (blockProgress.load()) {
        return false;
    }
    {
        std::unique_lock lock(progress_mutex_);
        progress_circle += GetSecondsSinceLastFrame() * theme.progress_speed * 4.f;
    }

    SkyPromptAPI::PromptType a_type = SkyPromptAPI::kSinglePress;
    Interaction interaction;
    {
        std::shared_lock lock(q_mutex_);
        if (interactQueue.current_button) {
            const auto interaction_button = interactQueue.current_button;
            a_type = interaction_button->type;
            interaction = interaction_button->interaction;
        }
    }

    const bool has_progress = PromptTypeFlags::GetHasProgress(a_type);
    const bool is_holdandkeeptype = PromptTypeFlags::GetIsHoldAndKeepType(a_type);

    if (std::shared_lock lock(progress_mutex_); progress_circle > (has_progress ? progress_circle_max : 0.f)) {
        progress_circle = is_holdandkeeptype ? progress_circle_max : 0.0f;
        lock.unlock();

        if (buttonState.pressCount == 3 && has_progress) {
            buttonState.pressCount = 0;
            ClearQueue(SkyPromptAPI::kDeclined);
        } else {
            if (!buttonState.acceptedThisPress) {
                Manager::GetSingleton()->activationPop.Trigger(interaction);
                buttonState.acceptedThisPress = true;
            }
            if (a_type == SkyPromptAPI::kHold) {
                RemoveFromQ(interaction);
            }
            const auto curr_button = GetCurrentButton();
            SendEvent(interaction, SkyPromptAPI::PromptEventType::kAccepted, {0.f, 0.f},
                      curr_button ? curr_button->GetProgressOverride(false) : 0.f);
            Start();
            if (!is_holdandkeeptype) {
                blockProgress.store(true);
            }
        }

        return true;
    }

    return false;
}

uint32_t SubManager::GetPromptKey(const Theme::Theme& theme) const {
    if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
        return interactQueue.current_button->GetKey(theme);
    }
    return 0;
}

SkyPromptAPI::PromptType SubManager::GetPromptType() const {
    if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
        return interactQueue.current_button->type;
    }
    return SkyPromptAPI::kSinglePress;
}

void SubManager::NextPrompt(const Theme::Theme& theme) {
    {
        std::unique_lock lock(q_mutex_);
        if (const auto next = interactQueue.Next()) {
            Show(theme, next);
        }
    }
    if (Tutorial::Tutorial2::showing_tutorial.load()) {
        const SCENES::Event a_id = InteractionID::Pack(Tutorial::client_id, 0);
        if (const auto a_interaction = GetCurrentInteraction(); a_id == a_interaction.event) {
            Tutorial::Tutorial2::to_be_deleted.erase(InteractionID::Local(a_interaction.action));
            if (Tutorial::Tutorial2::to_be_deleted.empty()) {
                SKSE::GetTaskInterface()->AddTask([]() {
                        SkyPromptAPI::RemovePrompt(Tutorial::Tutorial2::Sink::GetSingleton(), Tutorial::client_id);
                        Tutorial::Tutorial2::showing_tutorial.store(false);
                        if (!SkyPromptAPI::SendPrompt(Tutorial::Tutorial3::Sink::GetSingleton(), Tutorial::client_id)) {
                            logger::error("Failed to Send Tutorial3 prompts.");
                        }
                    }
                    );
            }
        }
    }
}

bool SubManager::HasPrompt() const {
    std::shared_lock lock(q_mutex_);
    return interactQueue.current_button;
}

bool SubManager::IsHidden() const {
    std::shared_lock lock(q_mutex_);
    const auto button = interactQueue.current_button;
    return !button || interactQueue.IsHidden() || button->HasMissingAttachedRef();
}

SCENES::Event SubManager::GetQueuedEvent() const {
    std::shared_lock lock(q_mutex_);
    return interactQueue.buttons.empty() ? 0 : interactQueue.buttons.begin()->interaction.event;
}

SkyPromptAPI::ClientID SubManager::GetClientID() const {
    return InteractionID::Client(GetQueuedEvent());
}

Interaction SubManager::GetCurrentInteraction() const {
    std::shared_lock lock(q_mutex_);
    if (interactQueue.current_button) {
        return interactQueue.current_button->interaction;
    }
    return {};
}

const InteractionButton* SubManager::GetCurrentButton() const {
    std::shared_lock lock(q_mutex_);
    if (interactQueue.current_button) {
        return interactQueue.current_button;
    }
    return nullptr;
}

void SubManager::AddSink(const Interaction& a_interaction, const SkyPromptAPI::PromptSink* a_sink) {
    std::unique_lock lock(sink_mutex_);
    auto& owners = sinks[a_interaction];
    // The latest submitter supplies the displayed mutable values.
    if (!owners.empty() && owners.back() == a_sink) return;
    std::erase(owners, a_sink);
    owners.push_back(a_sink);
}

bool SubManager::IsInQueue(const SkyPromptAPI::PromptSink* a_sink) const {
    std::shared_lock lock(sink_mutex_);
    for (const auto& sinks_ : sinks | std::views::values) {
        if (auto it = std::ranges::find(sinks_, a_sink); it != sinks_.end()) {
            return true;
        }
    }
    return false;
}

bool InteractionButton::HasMissingAttachedRef() const {
    return attached_object && !attached_object.get();
}

uint32_t InteractionButton::GetKey(const Theme::Theme& theme) const {
    if (theme.prompt_alignment == Theme::kList) {
        return GetControlKey(RE::UserEvents::GetSingleton()->activate);
    }
    const auto a_device = MANAGER(Input)->GetInputDevice();
    if (const auto it = keys.find(a_device); it != keys.end()) {
        return it->second; // client-provided key for this device
    }

    // fallback to default key for this device
    const auto& default_keys = MCP::Settings::default_keys;
    const auto defaultsIt = default_keys.find(a_device);
    if (defaultsIt == default_keys.end() || defaultsIt->second.empty()) {
        logger::error("No fallback keys for device {}", static_cast<int>(a_device));
        return 0;
    }

    const auto& defaults = defaultsIt->second;
    const size_t safeIndex = std::min(
        static_cast<size_t>(std::max(default_key_index, 0)),
        defaults.size() - 1
        );

    return defaults[safeIndex];
}

InteractionButton::InteractionButton(const Interaction& a_interaction, const Mutables& a_mutables,
                                     const SkyPromptAPI::PromptType a_type, const RefID a_refid,
                                     std::map<Input::DEVICE, uint32_t> a_keys, const int a_default_key_index) {
    interaction = a_interaction;
    type = a_type;
    mutables = a_mutables;
    if (const auto a_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refid)) {
        attached_object = a_ref->GetHandle();
    } else if (const auto temp = RE::Inventory3DManager::GetSingleton()->tempRef) {
        if (temp->GetFormID() == a_refid) {
            attached_object = temp->GetHandle();
        }
    }
    keys = std::move(a_keys);
    default_key_index = a_default_key_index;
}

void Manager::Start() {
    isPaused.store(false);
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    for (auto& group : page->second | std::views::values) {
        group.Start();
    }
}

void Manager::Group::Start() {
    for (const auto& a_manager : managers) {
        a_manager->Start();
    }
}

void Manager::Stop() {
    isPaused.store(true);
    activationPop.Clear();
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    for (auto& group : page->second | std::views::values) {
        group.Stop();
    }
}

void Manager::Group::Stop() {
    for (const auto& a_manager : managers) {
        a_manager->Stop();
    }
}

void Manager::CleanUpQueue() {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    bool removed = false;
    for (auto& group : page->second | std::views::values) {
        removed |= group.CleanUpQueue();
    }
    if (removed) {
        {
            std::shared_lock theme_lock(Theme::m_theme_);
            RegroupPage(current_pageID);
        }
        if (std::ranges::all_of(page->second | std::views::values, &Group::IsEmpty)) {
            lock.unlock();
            CyclePage(false);
        }
    }
}

bool Manager::Group::CleanUpQueue() {
    for (const auto& manager : managers) {
        manager->CleanUpQueue(*theme);
    }
    return RemoveEmptyRows(true);
}

bool Manager::Group::RemoveEmptyRows(const bool active) {
    auto& rows = managers;
    bool removed = false;
    for (size_t index = rows.size(); index > 0;) {
        --index;
        if (!rows[index]->HasQueue()) {
            rows.erase(rows.begin() + index);
            if (active) {
                list.OnRowRemoved(index);
            }
            removed = true;
        }
    }
    if (removed) {
        int index = 0;
        for (const auto& row : rows) {
            row->SetDefaultKeyIndex(*theme, index++);
        }
        if (active) {
            list.ClampSelection(rows.size());
        }
    }
    return removed;
}

void Manager::Group::ShowPromptRow(const size_t index, const bool isList, const size_t visibleCount) {
    // Advance every row's lifetime, but draw only the List viewport.
    const auto before = renderBatch.size();
    managers[index]->ShowQueue(*theme);
    if (isList && renderBatch.size() != before &&
        !list.PrepareRow(renderBatch.back(), index, managers.size(), visibleCount)) {
        renderBatch.resize(before);
    }
}

void Manager::Group::Render(const SkyPromptAPI::ClientID pageID, const SkyPromptAPI::ClientID groupID,
                           const SkyPromptAPI::ClientID pageIndex, const SkyPromptAPI::ClientID pageCount) {
    const auto& groupTheme = *theme;
    const auto& groupFonts = MANAGER(IconFont)->GetFonts(groupTheme.font_name,
                                                      groupTheme.prompt_size * GetResolutionScale());
    PushFont(groupFonts.normalFont);
    const auto [width, height] = ImGui::GetIO().DisplaySize;

    const auto resScale = GetResolutionScale();
    const ImVec2 windowPos(
        width * groupTheme.xPercent - groupTheme.marginX * resScale,
        height * groupTheme.yPercent - groupTheme.marginY * resScale
        );

    const bool isList = groupTheme.prompt_alignment == Theme::kList;
    const auto visibleCount = static_cast<size_t>(std::max(groupTheme.n_max_buttons, 1));
    if (isList) {
        list.UpdateViewport(managers.size(), visibleCount);
    }
    std::map<RefID, std::vector<size_t>> rowsByObject;
    renderBatch.clear();

    for (const auto index : std::views::iota(size_t{0}, managers.size())) {
        if (const auto a_ref = managers[index]->GetAttachedObject()) {
            rowsByObject[a_ref->GetFormID()].push_back(index);
            continue;
        }
        ShowPromptRow(index, isList, visibleCount);
    }

    const float windowPadding = 0.4f * groupTheme.prompt_size;
    fmt::memory_buffer windowName;
    fmt::format_to(std::back_inserter(windowName), "SkyPrompt##{}_{}", pageID, groupID);
    windowName.push_back('\0');
    BeginImGuiWindow(windowName.data(),
                     GetSkyPromptContentOrigin(groupTheme, windowPos), windowPadding);
    RenderSkyPrompt(groupTheme, windowPos);

    if (pageCount > 1) {
        DrawCycleIndicators(groupTheme, groupFonts.smallFont, pageIndex, pageCount);
    }

    EndImGuiWindow();

    int i = 0;
    for (const auto& rowIndices : rowsByObject | std::views::values) {
        const auto position = managers[rowIndices[0]]->GetAttachedObjectPos(groupTheme);
        auto window_pos = position.screen;
        window_pos.x -= groupTheme.marginX * resScale;
        window_pos.y -= groupTheme.marginY * resScale;
        renderBatch.clear();
        for (const auto index : rowIndices) {
            ShowPromptRow(index, isList, visibleCount);
        }
        windowName.clear();
        fmt::format_to(std::back_inserter(windowName), "SkyPromptHover{}##{}_{}", i++, pageID, groupID);
        windowName.push_back('\0');
        BeginImGuiWindow(windowName.data(),
                         GetSkyPromptContentOrigin(groupTheme, window_pos), windowPadding);
        if (position.world) VR::BeginWorldPrompt(*position.world, GetWindowDrawList());
        RenderSkyPrompt(groupTheme, window_pos);
        if (position.world) VR::EndWorldPrompt();
        EndImGuiWindow();
    }
    PopFont();
}

void Manager::Render() {
    if (IsPaused()) {
        return;
    }

    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;

    SkyPromptAPI::ClientID otherPages = 0;
    SkyPromptAPI::ClientID index = 0;
    if (MCP::Settings::cycle_controls.load()) {
        for (const auto& [pageID, pageGroups] : pages) {
            if (pageID != current_pageID) {
                if (std::ranges::any_of(pageGroups | std::views::values, [](const auto& group) {
                    return group.HasQueue();
                })) {
                    ++otherPages;
                }
            } else {
                index = otherPages;
            }
        }
    }
    for (auto& [groupID, group] : page->second) {
        if (group.IsEmpty()) continue;
        group.Render(current_pageID, groupID, index + 1, otherPages + 1);
    }
}

void Manager::Clear(const SkyPromptAPI::PromptEventType a_event_type) {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    for (auto& group : page->second | std::views::values) {
        group.Clear(a_event_type);
    }
}

void Manager::Group::Clear(const SkyPromptAPI::PromptEventType eventType) {
    for (const auto& a_manager : managers) {
        a_manager->ClearQueue(eventType);
    }
    managers.clear();
    list.Reset();
}

void Manager::Group::OnShown() {
    list.Reset();
}

Interaction Manager::MakeInteraction(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::EventID a_event,
                                     const SkyPromptAPI::ActionID a_action) {
    return {InteractionID::Pack(a_clientID, a_event), InteractionID::Pack(a_clientID, a_action)};
}

void Manager::ResetQueue() const {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    for (const auto& group : page->second | std::views::values) {
        group.ResetQueue();
    }
}

void Manager::Group::ResetQueue() const {
    for (auto& a_manager : managers) {
        a_manager->ResetQueue();
    }
}

void Manager::WakeUpQueue() const {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    for (const auto& group : page->second | std::views::values) {
        group.WakeUpQueue();
    }
}

void Manager::Group::WakeUpQueue() const {
    for (auto& a_manager : managers) {
        a_manager->WakeUpQueue();
    }
}

bool Manager::IsHidden() const {
    std::shared_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    return page == pages.end() ||
           std::ranges::all_of(page->second | std::views::values, &Group::IsHidden);
}

bool Manager::Group::IsHidden() const {
    for (const auto& a_manager : managers) {
        if (!a_manager->IsHidden()) {
            return false;
        }
    }
    return true;
}

std::optional<bool> Manager::Group::ProcessInput(RE::InputEvent* event, const uint32_t buttonKey) {
    if (const auto handled = ProcessListInput(event, buttonKey)) return *handled;
    const auto button_event = event->AsButtonEvent();
    const auto mouse_event = event->AsMouseMoveEvent();
    const auto thumbstick_event = event->AsThumbstickEvent();
    auto key = buttonKey;
    if (mouse_event) {
        key = SkyPromptAPI::kMouseMove;
    } else if (thumbstick_event) {
        key = thumbstick_event->IsLeft() ? SkyPromptAPI::kThumbstickMoveL : SkyPromptAPI::kThumbstickMoveR;
    } else if (!button_event) {
        return std::nullopt;
    }

    auto inputRows = std::span(managers);
    if (theme->prompt_alignment == Theme::kList) {
        if (list.selection >= inputRows.size()) return std::nullopt;
        inputRows = inputRows.subspan(list.selection, 1);
    }

    bool block = false;
    for (const auto& submanager : inputRows) {
        if (submanager->IsHidden()) continue;
        const auto prompt_key = submanager->GetPromptKey(*theme);
        if (prompt_key == 0 || prompt_key != key) continue;
        if (PromptTypeFlags::GetBlocksInput(submanager->GetPromptType())) {
            block = true;
        }
        if (button_event) {
            const auto now = std::chrono::steady_clock::now();
            submanager->buttonState.isPressing = button_event->IsPressed();
            if (button_event->IsDown()) {
                submanager->buttonState.acceptedThisPress = false;
                submanager->buttonState.pressCount++;
                submanager->buttonState.lastPressTime = now;
                submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kDown);
            } else if (button_event->IsUp()) {
                submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kUp);
            }
            if (submanager->buttonState.pressCount > 0 || !submanager->buttonState.isPressing) {
                submanager->UpdateProgressCircle(*theme, submanager->buttonState.isPressing);
            }
        } else if (mouse_event) {
            submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kMove,
                                  {static_cast<float>(mouse_event->mouseInputX),
                                   static_cast<float>(mouse_event->mouseInputY)});
            submanager->UpdateProgressCircle(*theme, mouse_event->mouseInputX != 0 || mouse_event->mouseInputY != 0);
        } else if (thumbstick_event) {
            submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kMove,
                                  {thumbstick_event->xValue, thumbstick_event->yValue});
            submanager->UpdateProgressCircle(*theme, thumbstick_event->xValue != 0.f || thumbstick_event->yValue != 0.f);
        }
    }

    if (block) return true;
    return std::nullopt;
}

std::optional<bool> Manager::Group::ProcessInput(const Input::VRNavigation::Direction direction) {
    switch (direction) {
        case Input::VRNavigation::Direction::kUp:
            return ProcessListNavigation(PromptLayouts::List::Navigation::kPrevious);
        case Input::VRNavigation::Direction::kDown:
            return ProcessListNavigation(PromptLayouts::List::Navigation::kNext);
        default:
            return std::nullopt;
    }
}

std::optional<bool> Manager::Group::ProcessListInput(RE::InputEvent* event, const uint32_t buttonKey) {
    if (theme->prompt_alignment != Theme::kList) return std::nullopt;
    const auto button = event->AsButtonEvent();
    if (!button) return std::nullopt;
    const auto navigation = PromptLayouts::List::GetNavigation(
        *button, buttonKey, GetControlKey(RE::UserEvents::GetSingleton()->activate));
    return ProcessListNavigation(navigation);
}

std::optional<bool> Manager::Group::ProcessListNavigation(const PromptLayouts::List::Navigation navigation) {
    if (navigation == PromptLayouts::List::Navigation::kUnhandled) return std::nullopt;

    if (theme->prompt_alignment != Theme::kList) return std::nullopt;
    if (list.selection >= managers.size()) return std::nullopt;
    const auto previous = list.selection;
    const auto selected = managers[previous].get();
    const bool hidden = selected->IsHidden();
    // Finish the current press before changing its target.
    if (navigation != PromptLayouts::List::Navigation::kNone && (hidden || !selected->buttonState.isPressing)) {
        do {
            const auto current = list.selection;
            list.MoveSelection(navigation, managers.size());
            if (list.selection == current) {
                list.selection = previous;
                break;
            }
        } while (managers[list.selection]->IsHidden());

        if (!hidden || list.selection != previous) {
            selected->buttonState.Reset();
            WakeUpQueue();
        }
    }
    if (hidden && list.selection == previous) {
        return std::nullopt;
    }
    const auto inputRow = hidden ? managers[list.selection].get() : selected;
    return PromptTypeFlags::GetBlocksInput(inputRow->GetPromptType());
}

void Manager::Group::CollectPromptButtons(std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>>& buttons) const {
    if (theme->prompt_alignment == Theme::kList) {
        if (const auto selected = list.selection < managers.size() ? managers[list.selection].get() : nullptr;
            selected && !selected->IsHidden()) {
            if (const auto key = selected->GetPromptKey(*theme); key != 0) {
                buttons.emplace_back(selected->GetPromptType(), key);
            }
        }
        return;
    }
    for (const auto& a_manager : managers) {
        if (a_manager->IsHidden()) {
            continue;
        }
        if (const auto key = a_manager->GetPromptKey(*theme); key != 0) {
            auto type = a_manager->GetPromptType();
            buttons.emplace_back(type, key);
        }
    }
}

std::optional<bool> Manager::ProcessInput(RE::InputEvent* event, const uint32_t buttonKey) {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return std::nullopt;
    std::optional<bool> block;
    for (auto& group : page->second | std::views::values) {
        if (const auto handled = group.ProcessInput(event, buttonKey)) {
            block = block.value_or(false) || *handled;
        }
    }
    return block;
}

std::optional<bool> Manager::ProcessInput(const Input::VRNavigation::Direction direction) {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return std::nullopt;
    std::optional<bool> block;
    for (auto& group : page->second | std::views::values) {
        if (const auto handled = group.ProcessInput(direction)) {
            block = block.value_or(false) || *handled;
        }
    }
    return block;
}

std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>> Manager::GetPromptButtons() const {
    std::shared_lock lock(mutex_);
    std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>> buttons;
    if (const auto page = pages.find(current_pageID); page != pages.end()) {
        for (const auto& group : page->second | std::views::values) {
            group.CollectPromptButtons(buttons);
        }
    }
    return buttons;
}

void Manager::ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func) {
    std::unique_lock lock(mutex_);
    const auto page = pages.find(current_pageID);
    if (page == pages.end()) return;
    for (auto& group : page->second | std::views::values) {
        group.ForEachManager(a_func);
    }
}

void Manager::Group::ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func) {
    for (auto& a_manager : managers) {
        a_func(a_manager);
    }
}

void Manager::AddEventToSend(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_sink,
                             const SkyPromptAPI::Prompt& a_prompt,
                             const SkyPromptAPI::PromptEventType event_type, const std::
                             pair<float, float> a_delta) {
    std::unique_lock lock(events_to_send_mutex);
    events_to_send_[{a_clientID, a_sink}].push_back({.prompt = a_prompt, .type = event_type, .delta = a_delta});
}

void Manager::SendEvents() {
    std::unique_lock lock(events_to_send_mutex);
    const auto sinks_to_notify = events_to_send_ | std::views::keys | std::ranges::to<std::vector>();
    for (const auto& key : sinks_to_notify) {
        auto it = events_to_send_.find(key);
        if (!key.second || it == events_to_send_.end()) {
            continue;
        }
        // Callbacks may remove pending prompts or enqueue more events. Keep this batch bounded.
        for (auto remaining = it->second.size(); remaining > 0; --remaining) {
            it = events_to_send_.find(key);
            if (it == events_to_send_.end() || it->second.empty()) {
                break;
            }
            const auto event = it->second.front();
            it->second.pop_front();
            lock.unlock();
            key.second->ProcessEvent(event);
            lock.lock();
        }
    }
    std::erase_if(events_to_send_, [](const auto& entry) {
        return !entry.first.second || entry.second.empty();
    });
}

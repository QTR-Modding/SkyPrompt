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
        manager->ShowQueue();
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

void ButtonQueue::Show(float progress, const InteractionButton* button2show, const ButtonState& a_button_state) {
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
        alpha = std::max(alpha - Theme::last_theme->fadeSpeed * seconds * 120.f, 0.0f);
    } else {
        alpha = std::min(alpha + Theme::last_theme->fadeSpeed * seconds * 120.f, 1.0f);
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

    auto button_state = has_progress ? ButtonStateToFloat(a_button_state) : -1.f;

    const auto buttonKey = current_button->GetKey();
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
    // check if the button already exists
    if (const auto it = buttons.find(a_button); it != buttons.end()) {
        return nullptr;
    }
    // add the button
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

const std::vector<std::unique_ptr<SubManager>>* Manager::GetManagerList(const SkyPromptAPI::ClientID a_clientID) const {
    std::shared_lock lock(mutex_);
    if (last_clientID == a_clientID) {
        return &managers;
    }
    if (client_managers.contains(a_clientID)) {
        return &client_managers.at(a_clientID);
    }
    return nullptr;
}

std::vector<std::unique_ptr<SubManager>>* Manager::GetManagerList(const SkyPromptAPI::ClientID a_clientID) {
    std::shared_lock lock(mutex_);
    if (last_clientID == a_clientID) {
        return &managers;
    }
    if (client_managers.contains(a_clientID)) {
        return &client_managers.at(a_clientID);
    }
    return nullptr;
}

SkyPromptAPI::ClientID Manager::FindCompatibleClientID(const SkyPromptAPI::ClientID a_clientID) const {
    const auto contains_client = [a_clientID](const auto& manager_list) {
        return std::ranges::any_of(manager_list, [a_clientID](const auto& manager) {
            return manager &&
                   std::ranges::any_of(manager->GetInteractions(), [a_clientID](const Interaction& interaction) {
                       return InteractionID::Client(interaction.event) == a_clientID;
                   });
        });
    };

    std::shared_lock lock(mutex_);

    if (contains_client(managers)) {
        return last_clientID;
    }

    for (const auto& [clientID, manager_list] : client_managers) {
        if (contains_client(manager_list)) {
            return clientID;
        }
    }

    return 0;
}

bool Manager::InitializeClient(const SkyPromptAPI::ClientID a_clientID) {
    {
        std::shared_lock lock(mutex_);
        if (a_clientID == 0 || last_clientID == a_clientID || client_managers.contains(a_clientID)) {
            return false;
        }
    }

    {
        std::unique_lock lock(mutex_);
        client_managers[a_clientID] = std::vector<std::unique_ptr<SubManager>>{};
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

    void OffsetRight(const RE::NiPoint3& a_pos, const RE::NiPoint3& a_cam_pos, RE::NiPoint3& a_out,
                     const float a_offset) {
        const auto diff = a_pos - a_cam_pos;
        constexpr RE::NiPoint3 z_vec(0.f, 0.f, 1.f);
        const auto right_vec = diff.UnitCross(z_vec);
        a_out = a_pos + right_vec * a_offset;
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

ImVec2 SubManager::GetAttachedObjectPos() const {
    if (const auto ref = GetAttachedObject()) {
        constexpr float padding = 10.f;
        RE::NiPoint3 pos;
        ImVec2 pos2d;

        if (const auto temp_ref = RE::Inventory3DManager::GetSingleton()->tempRef;
            temp_ref && ref->GetFormID() == temp_ref->GetFormID()) {
            if (const auto inv3dmngr = RE::Inventory3DManager::GetSingleton(); !inv3dmngr->GetRuntimeData().loadedModels
                .empty()) {
                if (const auto& model = inv3dmngr->GetRuntimeData().loadedModels.back().spModel) {
                    OffsetRight(model->world.translate, RE::UI3DSceneManager::GetSingleton()->cachedCameraPos, pos,
                                model->worldBound.radius);
                    pos2d = WorldToScreenLoc(pos, RE::UI3DSceneManager::GetSingleton()->camera) + ImVec2{
                                (Theme::last_theme->prompt_size + padding) * DisplayTweaks::resolutionScale, 0};
                }
            }
        } else if (const auto a_head = [&]() -> RE::NiAVObject* {
            if (const auto actor = ref->As<RE::Actor>()) {
                if (const auto middle = actor->GetMiddleHighProcess()) {
                    return middle->headNode;
                }
            }
            return nullptr;
        }()) {
            constexpr float npc_head_size = 15.f;
            const float objectScale = ref->GetScale();
            const auto cameraPos = RE::PlayerCamera::GetSingleton()->GetRuntimeData2().pos;
            const auto npc_head_pos = a_head->world.translate;
            const auto diff = npc_head_pos - cameraPos;
            constexpr RE::NiPoint3 z_vec(0.f, 0.f, 1.f);
            const auto right_vec = diff.UnitCross(z_vec);
            pos = npc_head_pos + right_vec * (npc_head_size * objectScale);
            pos2d = WorldToScreenLoc(pos) + ImVec2{
                        (Theme::last_theme->prompt_size + padding) * DisplayTweaks::resolutionScale, 0};
        } else {
            DirectX::BoundingOrientedBox bounding_box;
            BoundingBox::GetOBB(ref, bounding_box, true);

            const auto center = bounding_box.Center;

            pos = RE::NiPoint3{center.x, center.y, center.z + bounding_box.Extents.z + 10.f};
            pos2d = WorldToScreenLoc(pos);

            pos2d += ImVec2{0.f, -(Theme::last_theme->prompt_size + padding) * DisplayTweaks::resolutionScale};
        }

        FastClampToScreen(pos2d);

        return pos2d;
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

void SubManager::Update(const Interaction& a_interaction, const ButtonMutables& a_mutables) const {
    std::unique_lock lock(q_mutex_);
    for (const auto& a_button : interactQueue.buttons) {
        if (a_button.interaction == a_interaction) {
            a_button.mutables = a_mutables;
            return;
        }
    }
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

void SubManager::Show(const InteractionButton* button2show) {
    interactQueue.Show(progress_circle, button2show, buttonState);
}

void SubManager::ButtonStateActions() {
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
                    NextPrompt();
                }
                buttonState.pressCount = 0;
            } else if (buttonState.pressCount == 4 || buttonState.pressCount >= 6) {
                NextPrompt();
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

void SubManager::Add2Q(const InteractionButton& iButton, const bool show) {
    std::unique_lock lock(q_mutex_);
    if (const auto button = interactQueue.AddButton(iButton); button && show) {
        std::shared_lock lock2(progress_mutex_);
        if (!Manager::GetSingleton()->IsPaused() && progress_circle == 0.f) {
            Show(button);
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

void SubManager::SetDefaultKeyIndex(const int index) {
    std::unique_lock lock(q_mutex_);
    const auto current = interactQueue.current_button;
    const auto oldKey = current ? current->GetKey() : 0;
    for (const auto& button : interactQueue.buttons) {
        button.default_key_index = index;
    }
    if (current && current->GetKey() != oldKey) {
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

void SubManager::ShowQueue() {
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
        Show(nullptr);
    }
}

void SubManager::WakeUpQueue() {
    std::unique_lock lock(q_mutex_);
    interactQueue.WakeUp();
    wakeup_queued_.store(false);
}

SubManager* Manager::Add2Q(
    const SkyPromptAPI::ClientID a_clientID, const Interaction& a_interaction, const ButtonMutables& a_mutables,
    const SkyPromptAPI::PromptType a_type, const RefID a_refid, const std::map<Input::DEVICE, uint32_t>& a_bttn_map,
    const bool show) {
    const auto manager_list = GetManagerList(a_clientID);
    if (!manager_list) {
        return nullptr;
    }

    std::unique_lock lock(mutex_);

    for (const auto& a_manager : *manager_list) {
        if (std::ranges::any_of(a_manager->GetInteractions(),
                                [&](const auto& i) { return i == a_interaction; })) {
            a_manager->Update(a_interaction, a_mutables);
            return a_manager.get();
        }
    }

    int index = 0;
    for (const auto& a_manager : *manager_list) {
        if (const auto& interactions = a_manager->GetInteractions(); interactions.empty()) {
            const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
            a_manager->Add2Q(iButton, show);
            return a_manager.get();
        } else if (interactions.front().event == a_interaction.event) {
            a_manager->WakeUpQueue();
            const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
            a_manager->Add2Q(iButton, show);
            return a_manager.get();
        }
        ++index;
    }

    {
        std::shared_lock theme_lock(Theme::m_theme_);
        const auto it = Theme::themes.find(a_clientID);
        const auto& theme = it != Theme::themes.end() ? *it->second : Theme::default_theme;
        if (theme.prompt_alignment != Theme::kList && manager_list->size() >= theme.n_max_buttons) {
            return nullptr;
        }
    }
    // If no manager has the event, make a new row.
    manager_list->emplace_back(std::make_unique<SubManager>());

    const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
    manager_list->back()->Add2Q(iButton, show);
    return manager_list->back().get();
}

bool Manager::SwitchToClientManager(const SkyPromptAPI::ClientID client_id) {
    if (std::shared_lock lock(mutex_); client_id == last_clientID) {
        return false;
    }

    if (!MCP::Settings::cycle_controls.load()) {
        Clear(SkyPromptAPI::kRemovedByMod);
    } else {
        CleanUpQueue();
        ResetQueue();
    }

    std::unique_lock lock(mutex_);

    if (last_clientID != 0) {
        client_managers.at(last_clientID) = std::move(managers);
    }

    managers = std::move(client_managers.at(client_id));
    last_clientID = client_id;
    list.Reset();

    std::shared_lock theme_lock(Theme::m_theme_);
    const auto last_theme = Theme::last_theme;
    Theme::last_theme = Theme::themes.contains(last_clientID) ? Theme::themes.at(last_clientID) : &Theme::default_theme;

    if (last_theme != Theme::last_theme) {
        MCP::refreshStyle.store(true);
    }

    return true;
}

bool Manager::CycleClient(const bool a_left) {
    std::shared_lock lock(mutex_);
    if (client_managers.size() <= 1) {
        return false;
    }
    const bool any_has_queue = std::ranges::any_of(client_managers | std::views::values,
                                                   [](const auto& managers) {
                                                       return !managers.empty() && std::ranges::any_of(
                                                                  managers, [](const auto& m) {
                                                                      return m && m->HasQueue();
                                                                  });
                                                   });

    if (!any_has_queue) {
        return false;
    }

    auto it = client_managers.find(last_clientID);
    while (it->first == last_clientID || it->second.empty()) {
        if (a_left) {
            if (it == client_managers.begin()) {
                it = std::prev(client_managers.end());
            } else {
                --it;
            }
        } else {
            ++it;
            if (it == client_managers.end()) {
                it = client_managers.begin();
            }
        }
    }

    lock.unlock();
    return SwitchToClientManager(it->first);
}

bool Manager::Add2Q(const SkyPromptAPI::PromptSink* a_prompt_sink, const SkyPromptAPI::ClientID a_clientID) {
    const bool refresh = IsInQueue(a_clientID, a_prompt_sink, true);
    auto compatibleID = FindCompatibleClientID(a_clientID);

    if (compatibleID == 0) {
        compatibleID = a_clientID;

        if (std::shared_lock lock(mutex_); last_clientID != 0 && last_clientID != a_clientID) {
            bool found_client = false;
            bool all_compatible = true;

            for (const auto& manager : managers) {
                for (const auto& interaction : manager->GetInteractions()) {
                    found_client = true;

                    if (!Handshake::compatibility.AreCompatible(a_clientID, InteractionID::Client(interaction.event))) {
                        all_compatible = false;
                        break;
                    }
                }

                if (!all_compatible) {
                    break;
                }
            }

            if (found_client && all_compatible) {
                compatibleID = last_clientID;
            }
        }
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
            Add2Q(compatibleID, interaction, {.text = a_txt, .text_color = text_color, .progress = progress},
                  a_type, a_refid,
                  temp_button_keys, !refresh)) {
            submanager->AddSink(interaction, a_prompt_sink);
        } else {
            logger::warn("Failed to add interaction to the queue");
            success = false;
            if (!refresh) break;
        }
    }

    if (refresh) {
        RestorePromptOrder(compatibleID, a_clientID, prompts);
    } else if (success) {
        SwitchToClientManager(compatibleID);
    }

    return success && !refresh;
}

void Manager::RestorePromptOrder(const SkyPromptAPI::ClientID pageID, const SkyPromptAPI::ClientID clientID,
                                 const std::span<const SkyPromptAPI::Prompt> prompts) {
    const auto rows = GetManagerList(pageID);
    std::unique_lock lock(mutex_);
    if (!rows || rows->size() < 2) return;

    bool moved = false;
    auto before = rows->end();
    for (size_t index = prompts.size(); index > 0;) {
        const auto& prompt = prompts[--index];
        // A row follows the first appearance of its event, not each stacked action.
        if (std::ranges::any_of(prompts.first(index), [&](const auto& earlier) {
            return earlier.eventID == prompt.eventID;
        })) continue;

        const auto event = InteractionID::Pack(clientID, prompt.eventID);
        const auto row = std::ranges::find_if(*rows, [event](const auto& candidate) {
            const auto interactions = candidate->GetInteractions();
            return !interactions.empty() && interactions.front().event == event;
        });
        if (row == rows->end()) continue;

        if (row > before) {
            if (rows == &managers) list.OnRowRestored(row - rows->begin(), before - rows->begin());
            std::rotate(before, row, std::next(row));
            moved = true;
        } else {
            before = row;
        }
    }
    if (moved) {
        int index = 0;
        for (const auto& row : *rows) row->SetDefaultKeyIndex(index++);
        if (rows == &managers) list.ClampSelection(rows->size());
    }
}

bool Manager::IsInQueue(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink,
                        const bool wake_up) {
    bool result = false;

    const auto compatibleID = FindCompatibleClientID(a_clientID);
    if (compatibleID == 0) {
        return false;
    }

    const auto a_list = GetManagerList(compatibleID);

    if (!a_list) {
        return false;
    }

    std::shared_lock lock(mutex_);
    for (const auto& a_manager : *a_list) {
        if (a_manager->IsInQueue(a_prompt_sink)) {
            result = true;
            if (wake_up) {
                a_manager->WakeUpQueue();
            } else {
                return result;
            }
        }
    }
    return result;
}

bool Manager::RemoveFromQ(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink,
                         const std::optional<Interaction>& a_interaction) {
    const auto compatibleID = FindCompatibleClientID(a_clientID);
    bool removed = false;
    if (const auto manager_list = compatibleID != 0 ? GetManagerList(compatibleID) : nullptr) {
        std::unique_lock lock(mutex_);
        for (const auto& a_manager : *manager_list) {
            removed |= a_manager->RemoveFromQ(a_clientID, a_prompt_sink, a_interaction);
        }
        if (manager_list != &managers) {
            RemoveEmptyRows(*manager_list);
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
    for (const auto& a_manager : managers) {
        if (a_manager->HasQueue()) {
            return true;
        }
    }
    return false;
}

void SubManager::CleanUpQueue() {
    // if everything has expired AND has alpha=0, clear the queue
    if (std::unique_lock lock(q_mutex_);
        interactQueue.expired() && interactQueue.alpha <= 0.f
    ) {
        lock.unlock();
        ClearQueue(SkyPromptAPI::kTimeout);
    }
    ButtonStateActions();
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

bool SubManager::UpdateProgressCircle(const bool isPressing) {
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
        if (progress_circle > Theme::last_theme->progress_speed) {
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
        progress_circle += GetSecondsSinceLastFrame() * Theme::last_theme->progress_speed * 4.f;
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

uint32_t SubManager::GetPromptKey() const {
    if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
        return interactQueue.current_button->GetKey();
    }
    return 0;
}

SkyPromptAPI::PromptType SubManager::GetPromptType() const {
    if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
        return interactQueue.current_button->type;
    }
    return SkyPromptAPI::kSinglePress;
}

void SubManager::NextPrompt() {
    {
        std::unique_lock lock(q_mutex_);
        if (const auto next = interactQueue.Next()) {
            Show(next);
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
    if (!HasPrompt()) {
        return true;
    }
    std::shared_lock lock(q_mutex_);
    return interactQueue.IsHidden();
}

std::vector<Interaction> SubManager::GetInteractions() const {
    std::shared_lock lock(q_mutex_);
    std::vector<Interaction> interactions;
    for (const auto& a_button : interactQueue.buttons) {
        interactions.push_back(a_button.interaction);
    }
    return interactions;
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

uint32_t InteractionButton::GetKey() const {
    if (Theme::last_theme->prompt_alignment == Theme::kList) {
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
    for (const auto& a_manager : managers) {
        a_manager->Start();
    }
}

void Manager::Stop() {
    isPaused.store(true);
    activationPop.Clear();
    std::unique_lock lock(mutex_);
    for (const auto& a_manager : managers) {
        a_manager->Stop();
    }
}

void Manager::CleanUpQueue() {
    std::unique_lock lock(mutex_);
    for (const auto& manager : managers) {
        manager->CleanUpQueue();
    }
    if (RemoveEmptyRows(managers) && managers.empty()) {
        lock.unlock();
        CycleClient(false);
    }
}

bool Manager::RemoveEmptyRows(std::vector<std::unique_ptr<SubManager>>& rows) {
    bool removed = false;
    for (size_t index = rows.size(); index > 0;) {
        --index;
        if (!rows[index]->HasQueue()) {
            rows.erase(rows.begin() + index);
            if (&rows == &managers) {
                list.OnRowRemoved(index);
            }
            removed = true;
        }
    }
    if (removed) {
        int index = 0;
        for (const auto& row : rows) {
            row->SetDefaultKeyIndex(index++);
        }
        if (&rows == &managers) {
            list.ClampSelection(rows.size());
        }
    }
    return removed;
}

void Manager::ShowPromptRow(const size_t index, const bool isList, const size_t visibleCount) {
    // Advance every row's lifetime, but draw only the List viewport.
    const auto before = renderBatch.size();
    managers[index]->ShowQueue();
    if (isList && renderBatch.size() != before &&
        !list.PrepareRow(renderBatch.back(), index, managers.size(), visibleCount)) {
        renderBatch.resize(before);
    }
}

void Manager::ShowQueue() {
    if (IsPaused()) {
        return;
    }

    // Get the screen size
    const auto [width, height] = ImGui::GetIO().DisplaySize;

    // Calculate position
    const auto resScale = GetResolutionScale();
    const ImVec2 windowPos(
        width * Theme::last_theme->xPercent - Theme::last_theme->marginX * resScale,
        height * Theme::last_theme->yPercent - Theme::last_theme->marginY * resScale
        );

    const bool isList = Theme::last_theme->prompt_alignment == Theme::kList;
    const auto visibleCount = static_cast<size_t>(std::max(Theme::last_theme->n_max_buttons, 1));
    if (isList) {
        std::unique_lock lock(mutex_);
        list.UpdateViewport(managers.size(), visibleCount);
    }
    std::map<RefID, std::vector<size_t>> rowsByObject;
    renderBatch.clear();

    for (std::shared_lock lock(mutex_); const auto index : std::views::iota(size_t{0}, managers.size())) {
        if (const auto a_ref = managers[index]->GetAttachedObject()) {
            rowsByObject[a_ref->GetFormID()].push_back(index);
            continue;
        }
        ShowPromptRow(index, isList, visibleCount);
    }

    BeginImGuiWindow("SkyPrompt", GetSkyPromptContentOrigin(windowPos));
    RenderSkyPrompt(windowPos);

    if (MCP::Settings::cycle_controls.load()) {
        SkyPromptAPI::ClientID n_has_prompts = 0;
        SkyPromptAPI::ClientID index = 0;
        for (std::shared_lock lock(mutex_);
             const auto& [a_clientID,a_managers] : client_managers) {
            for (auto& a_manager : a_managers) {
                if (a_manager && a_manager->HasQueue()) {
                    n_has_prompts++;
                    break;
                }
            }
            if (a_clientID == last_clientID) {
                index = n_has_prompts;
            }
        }

        if (n_has_prompts > 0) {
            DrawCycleIndicators(index + 1, n_has_prompts + 1);
        }
    }

    EndImGuiWindow();

    int i = 0;
    for (std::shared_lock lock(mutex_);
         const auto& rowIndices : rowsByObject | std::views::values) {
        auto window_pos = managers[rowIndices[0]]->GetAttachedObjectPos();
        window_pos.x -= Theme::last_theme->marginX * resScale;
        window_pos.y -= Theme::last_theme->marginY * resScale;
        renderBatch.clear();
        for (const auto index : rowIndices) {
            ShowPromptRow(index, isList, visibleCount);
        }
        BeginImGuiWindow(std::format("SkyPromptHover{}", i++).c_str(),
                         GetSkyPromptContentOrigin(window_pos));
        RenderSkyPrompt(window_pos);
        EndImGuiWindow();
    }
}

void Manager::Clear(const SkyPromptAPI::PromptEventType a_event_type) {
    std::unique_lock lock(mutex_);
    for (const auto& a_manager : managers) {
        a_manager->ClearQueue(a_event_type);
    }
    managers.clear();
    list.Reset();
}

Interaction Manager::MakeInteraction(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::EventID a_event,
                                     const SkyPromptAPI::ActionID a_action) {
    return {InteractionID::Pack(a_clientID, a_event), InteractionID::Pack(a_clientID, a_action)};
}

void Manager::ResetQueue() const {
    std::unique_lock lock(mutex_);
    for (auto& a_manager : managers) {
        a_manager->ResetQueue();
    }
}

void Manager::WakeUpQueue() const {
    std::unique_lock lock(mutex_);
    for (auto& a_manager : managers) {
        a_manager->WakeUpQueue();
    }
}

bool Manager::IsHidden() const {
    std::shared_lock lock(mutex_);
    for (const auto& a_manager : managers) {
        if (!a_manager->IsHidden()) {
            return false;
        }
    }
    return true;
}

std::optional<bool> Manager::ProcessListInput(RE::InputEvent* event) {
    if (Theme::last_theme->prompt_alignment != Theme::kList) return std::nullopt;
    const auto button = event->AsButtonEvent();
    if (!button) return std::nullopt;
    const auto navigation = list.GetNavigation(*button, GetControlKey(RE::UserEvents::GetSingleton()->activate));
    if (navigation == PromptLayouts::List::Navigation::kUnhandled) return std::nullopt;

    std::unique_lock lock(mutex_);
    if (list.selection >= managers.size()) return std::nullopt;
    const auto selected = managers[list.selection].get();
    if (selected->IsHidden()) return std::nullopt;
    const bool block = PromptTypeFlags::GetBlocksInput(selected->GetPromptType());
    // Finish the current press before changing its target.
    if (navigation != PromptLayouts::List::Navigation::kNone && !selected->buttonState.isPressing) {
        selected->buttonState.Reset();
        list.MoveSelection(navigation, managers.size());
        for (const auto& manager : managers) {
            manager->WakeUpQueue();
        }
    }
    return block;
}

SubManager* Manager::GetSubManagerByKey(const uint32_t a_prompt_key) const {
    std::shared_lock lock(mutex_);
    if (Theme::last_theme->prompt_alignment == Theme::kList) {
        const auto selected = list.selection < managers.size() ? managers[list.selection].get() : nullptr;
        return selected && selected->GetPromptKey() == a_prompt_key ? selected : nullptr;
    }
    for (auto& a_manager : managers) {
        if (const auto key = a_manager->GetPromptKey(); key == a_prompt_key) {
            return a_manager.get();
        }
    }
    return nullptr;
}

std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>> Manager::GetPromptButtons() const {
    std::shared_lock lock(mutex_);
    std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>> buttons;
    if (Theme::last_theme->prompt_alignment == Theme::kList) {
        if (const auto selected = list.selection < managers.size() ? managers[list.selection].get() : nullptr;
            selected && !selected->IsHidden()) {
            if (const auto key = selected->GetPromptKey(); key != 0) {
                buttons.emplace_back(selected->GetPromptType(), key);
            }
        }
        return buttons;
    }
    for (const auto& a_manager : managers) {
        if (a_manager->IsHidden()) {
            continue;
        }
        if (const auto key = a_manager->GetPromptKey(); key != 0) {
            auto type = a_manager->GetPromptType();
            buttons.emplace_back(type, key);
        }
    }
    return buttons;
}

void Manager::ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func) {
    std::unique_lock lock(mutex_);
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

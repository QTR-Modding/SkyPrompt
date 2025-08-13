#include "Renderer.h"
#include "Hooks.h"
#include "CLibUtilsQTR/Tasker.hpp"
#include "IconsFonts.h"
#include "Styles.h"
#include "Utils.h"
#include "Geometry.h"
#include "Tutorial.h"

using namespace ImGui::Renderer;

float ImGui::Renderer::GetResolutionScale()
{
    static auto height = RE::BSGraphics::Renderer::GetScreenSize().height;
	return DisplayTweaks::borderlessUpscale ? DisplayTweaks::resolutionScale : static_cast<float>(height)/ 1080.0f;
}

void ImGui::Renderer::RenderPrompts() {
    
	const auto manager = MANAGER(ImGui::Renderer);
	manager->SendEvents();
	manager->CleanUpQueue();

	if (MCP::Settings::shouldReloadLifetime.exchange(false)) {
		manager->ResetQueue();
		return;
	}
	if (MCP::Settings::shouldReloadPromptSize.exchange(false)) {
		ImGui::Styles::GetSingleton()->RefreshStyle();
		return;
	}
	if (manager->IsPaused()) {
		manager->Start();
	}
    if (!manager->HasTask()) {
		return;
	}

	manager->ShowQueue();
}


std::optional<std::pair<float,float>> InteractionButton::Show(const float alpha, const std::string& extra_text, const float progress, const float button_state) const
{
    const auto buttonKey = GetKey();
	const auto icon_manager = MANAGER(IconFont);
	if (icon_manager->unavailable_keys.contains(buttonKey)) return std::nullopt;
	const std::string base_text = mutables.text; // Cache the base text
	std::string a_text;
    a_text.reserve(base_text.size() + 1 + extra_text.size()); // Reserve memory to avoid reallocations
	a_text.append(base_text);
    if (!extra_text.empty()) { // If extra_text is not empty
        a_text.append(" ").append(extra_text);
    }

    const IconFont::IconTexture* buttonIcon = icon_manager->GetIcon(buttonKey);
    if (!buttonIcon) {
        logger::error("Button icon not found for key {}", buttonKey);
		icon_manager->unavailable_keys.insert(buttonKey);
		return std::nullopt;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
	std::optional<std::pair<float,float>> a_position;
    if (buttonIcon->srView.Get()) {
        const auto position = ImGui::ButtonIconWithCircularProgress(a_text.c_str(), mutables.text_color, buttonIcon, progress, button_state);
		a_position = { position.x,position.y };
    } else {
        logger::error("Button icon texture not loaded for key {}", buttonKey);
		icon_manager->unavailable_keys.insert(buttonKey);
    }

    ImGui::PopStyleVar(); // Restore alpha

	return a_position;
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
		int int_part = static_cast<int>(std::abs(int_part_f));  // always positive
		return {int_part, frac_part};  // frac_part keeps original sign
	}

	float GetSecondsSinceLastFrame() {
		return RE::GetSecondsSinceLastFrame() / (RE::BSTimer::QGlobalTimeMultiplier() + EPSILON);
    }
}

float InteractionButton::GetProgressOverride(const bool increment) const {

	if (type != SkyPromptAPI::kSinglePress) {
		return 0.f;
	}
	if (!increment) {
		return mutables.progress; // Return current progress if not incrementing
	}

	const auto [mult, prog] = splitFloat(mutables.progress);

    if (mult == 0) return prog;

    if (const auto new_frac = prog - mult* 0.1f * GetSecondsSinceLastFrame(); 
		new_frac < -1.f || std::signbit(new_frac) != std::signbit(prog)) {
		mutables.progress = static_cast<float>(mult);
	}
	else {
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
		alpha = std::max(alpha - MCP::Settings::fadeSpeed*seconds*120.f, 0.0f);
	}
	else {
        alpha = std::min(alpha + MCP::Settings::fadeSpeed*seconds*120.f, 1.0f);
	}
    if (Tutorial::showing_tutorial.load() || !Manager::IsGameFrozen()) {
	    elapsed += seconds;
    }

    std::string extra_text;
    if (const auto total = buttons.size(); total>1) {
	    if (const auto it = buttons.find(*current_button); it != buttons.end()) {
		    const auto index = std::distance(buttons.begin(), it) + 1;
		    extra_text = fmt::format(" ({}/{})", index, total);
	    }
	    else {
		    logger::error("Current button not found in the queue");
		    current_button = Next();
		    if (!current_button) return;
	    }
    }

    const auto button_type = current_button->type;
    if (const auto progress_override = current_button->GetProgressOverride(true); progress_override>EPSILON) {
	    progress = button_type == SkyPromptAPI::kSinglePress ? -progress_override : progress_override;
		WakeUp();
    }
    if (const auto pos = current_button->Show(alpha, extra_text, progress,
                                              button_type == SkyPromptAPI::kSinglePress
                                                  ? -1.f
                                                  : ButtonStateToFloat(a_button_state)); 
	    pos.has_value()) {
	    position = pos.value();
    }
}


const InteractionButton* ButtonQueue::AddButton(const InteractionButton& a_button)
{
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

bool ButtonQueue::RemoveButton(const Interaction& a_interaction)
{
    if (current_button && current_button->interaction == a_interaction) {
	    buttons.erase(*current_button);
	}
	// otherwise we need to find it in the Q.
	else {
		const auto it = std::ranges::find_if(buttons,
                                       [&](const auto& btn) { return btn.interaction == a_interaction; });
		if (it == buttons.end()) {
            return false;
        }
		buttons.erase(it);
	}
	current_button = nullptr;
    Reset();
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
    }
	else if (!buttons.empty()) {
		return &*buttons.begin();
	}
    return nullptr;
}

void ImGui::Renderer::Manager::ReArrange() {
	std::map<SCENES::Event,std::vector<InteractionButton>> interactions;
	std::map<Interaction,std::vector<const SkyPromptAPI::PromptSink*>> sinks;
	SkyPromptAPI::ClientID a_clientID;

	{
		std::shared_lock lock(mutex_);
		a_clientID = last_clientID;
		for (const auto& a_manager : managers) {

		    for (const auto& interaction_button : a_manager->GetButtons()) {
				auto& interaction = interaction_button.interaction;
				for (auto i = Input::DEVICE::kKeyboardMouse; i < Input::DEVICE::kTotal;
                     i = static_cast<Input::DEVICE>(static_cast<int>(i) + 1)) {
				}
                interactions[interaction.event].push_back(interaction_button);
            }

			for (const auto& [interaction, a_sinks] : a_manager->GetSinks()) {
				if (const auto it = sinks.find(interaction); it != sinks.end()) {
					it->second.insert(it->second.end(), a_sinks.begin(), a_sinks.end());
				}
				else {
					sinks[interaction] = a_sinks;
				}
			}
		}
	}

	{
	    std::unique_lock lock(mutex_);
        managers.clear();
	}

	// distribute the interactions to the managers
	for (const auto& interaction_buttons : interactions | std::views::values) {
		for (const auto& interaction_button : interaction_buttons) {
			const auto a_ref = interaction_button.attached_object.get().get();
			const auto a_refid = a_ref ? a_ref->GetFormID() : 0;
            if (!Add2Q(a_clientID, interaction_button.interaction, interaction_button.mutables, interaction_button.type, a_refid, interaction_button.keys, true)) {
               logger::error("Failed to add interaction to the queue");  
            }
		}
	}

	// distribute the sinks to the managers
	{
		std::unique_lock lock(mutex_);
		for (const auto& a_manager : managers) {
			for (const auto& [interaction, a_sinks] : sinks) {
				if (a_manager->IsInQueue(interaction)) {
					for (const auto& a_sink : a_sinks) {
						a_manager->AddSink(interaction, a_sink);
					}
				}
			}
		}
	}
}

bool ImGui::Renderer::Manager::IsInQueue(const Interaction& a_interaction) const
{
	std::shared_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		if (a_manager->HasQueue()) {
			for (const auto& interaction : a_manager->GetInteractions()) {
				if (interaction == a_interaction) {
					return true;
				}
			}
		}
	}
	return false;
}

const std::vector<std::unique_ptr<SubManager>>* ImGui::Renderer::Manager::GetManagerList(const SkyPromptAPI::ClientID a_clientID) const
{
	std::shared_lock lock(mutex_);
	if (last_clientID == a_clientID) {
		return &managers;
	}
	if (client_managers.contains(a_clientID)) {
		return &client_managers.at(a_clientID);
	}
	return nullptr;
}

std::vector<std::unique_ptr<SubManager>>* ImGui::Renderer::Manager::GetManagerList(const SkyPromptAPI::ClientID a_clientID)
{
	std::shared_lock lock(mutex_);
	if (last_clientID == a_clientID) {
		return &managers;
	}
	if (client_managers.contains(a_clientID)) {
		return &client_managers.at(a_clientID);
	}
	return nullptr;
}

bool ImGui::Renderer::Manager::InitializeClient(const SkyPromptAPI::ClientID a_clientID)
{
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

bool ImGui::Renderer::Manager::IsGameFrozen()
{
	if (const auto main = RE::Main::GetSingleton()) {
        if (main->freezeTime) return true;
        if (!main->gameActive) return true;
    }
	else return true;
	if (RE::UI::GetSingleton()->GameIsPaused()) return true;
	return false;
}

void ImGui::Renderer::SubManager::SendEvent(const Interaction& a_interaction, const SkyPromptAPI::PromptEventType event_type, const std::pair<float,float> delta, const float progress_override)
{
    constexpr uint32_t a_max = std::numeric_limits<SkyPromptAPI::ClientID>::max();
    const SkyPromptAPI::EventID a_event = a_interaction.event % a_max;
    const SkyPromptAPI::ActionID a_action = a_interaction.action % a_max;
	std::shared_lock lock(sink_mutex_);
	if (const auto it = sinks.find(a_interaction); it != sinks.end()) {
		for (const auto& a_sink : it->second) {
			if (!a_sink) continue;
            for (const auto prompts = a_sink->GetPrompts(); const auto& prompt : prompts) {
				if (prompt.eventID == a_event && prompt.actionID == a_action) {
					SkyPromptAPI::Prompt a_prompt = prompt;
					if (std::abs(progress_override)>0.f) {
						a_prompt.progress = progress_override;
					}
					Manager::GetSingleton()->AddEventToSend(a_sink, a_prompt, event_type, delta);
				}
			}
		}
	}
}

namespace {
    ImVec2 WorldToScreenLoc(const RE::NiPoint3 position) {
        static uintptr_t g_worldToCamMatrix = RELOCATION_ID(519579, 406126).address();         // 2F4C910, 2FE75F0
        static auto g_viewPort = (RE::NiRect<float>*)RELOCATION_ID(519618, 406160).address();  // 2F4DED0, 2FE8B98

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

    void OffsetRight(const RE::NiPoint3& a_pos, const RE::NiPoint3& a_cam_pos, RE::NiPoint3& a_out, const float a_offset) {
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

ImVec2 ImGui::Renderer::SubManager::GetAttachedObjectPos() const
{
    if (const auto ref = GetAttachedObject()) {
        constexpr float padding = 10.f;
        RE::NiPoint3 pos;
		ImVec2 pos2d;

		if (const auto temp_ref = RE::Inventory3DManager::GetSingleton()->tempRef; temp_ref && ref->GetFormID() == temp_ref->GetFormID()) {
            if (const auto inv3dmngr = RE::Inventory3DManager::GetSingleton(); !inv3dmngr ->GetRuntimeData().loadedModels.empty()) {
				if (const auto& model = inv3dmngr->GetRuntimeData().loadedModels.back().spModel) {
					OffsetRight(model->world.translate,RE::UI3DSceneManager::GetSingleton()->cachedCameraPos,pos,model->worldBound.radius);
			        pos2d = WorldToScreenLoc(pos,RE::UI3DSceneManager::GetSingleton()->camera) + ImVec2{(Theme::last_theme.prompt_size+padding) * DisplayTweaks::resolutionScale,0};
				}
			}
		}
		else if (const auto a_head = ref->GetNodeByName(RE::FixedStrings::GetSingleton()->npcHead)) {
            constexpr float npc_head_size = 15.f;
			const auto cameraPos = RE::PlayerCamera::GetSingleton()->pos;
            const auto npc_head_pos = a_head->world.translate;
			const auto diff = npc_head_pos - cameraPos;
			constexpr RE::NiPoint3 z_vec(0.f, 0.f, 1.f);
			const auto right_vec = diff.UnitCross(z_vec);
			pos = npc_head_pos + right_vec * npc_head_size;
			pos2d = WorldToScreenLoc(pos) + ImVec2{(Theme::last_theme.prompt_size+padding) * DisplayTweaks::resolutionScale,0};
		}
		else {
            const auto geo = Geometry(ref);

            const auto bound = geo.GetBoundingBox(ref->GetAngle(), ref->GetScale());
            const RE::NiPoint3 center = (bound.first + bound.second) / 2;

            pos = WorldObjects::GetPosition(ref) + RE::NiPoint3{center.x, center.y, bound.second.z + 16};
            pos2d = WorldToScreenLoc(pos);
		}


        FastClampToScreen(pos2d);

		return pos2d;
	}
	return {};
}

RE::TESObjectREFR* ImGui::Renderer::SubManager::GetAttachedObject() const {
	std::shared_lock lock(q_mutex_);
	if (const auto curr_button = interactQueue.current_button) {
		return curr_button->attached_object.get().get();
	}
	return nullptr;
}

void ImGui::Renderer::SubManager::Update(const SkyPromptAPI::ClientID a_client_id, const SkyPromptAPI::PromptSink* a_prompt_sink) const {
    using Update = ButtonMutables;
    std::map<Interaction, Update> updates;
    for (const auto prompts = a_prompt_sink->GetPrompts(); const auto& prompt : prompts) {
        if (prompt.text.empty()) {
	        logger::warn("Empty prompt text for interaction {}", prompt.eventID);
	        continue;
        }
        const auto a_interaction = Manager::MakeInteraction(a_client_id, prompt.eventID, prompt.actionID);
        updates[a_interaction] = Update(std::string(prompt.text), prompt.text_color, prompt.progress);
    }
    if (updates.empty()) {
        return;
    }
    std::unique_lock lock(q_mutex_);
    for (const auto& a_button : interactQueue.buttons) {
        if (updates.contains(a_button.interaction)) {
	        a_button.mutables = updates.at(a_button.interaction);
        }
    }
}

void ImGui::Renderer::SubManager::Show(const InteractionButton* button2show)
{
	interactQueue.Show(progress_circle,button2show,buttonState);
}

void ImGui::Renderer::SubManager::ButtonStateActions()
{
	buttonState.pressCount = std::min(6,buttonState.pressCount);

	SkyPromptAPI::PromptType a_type;
	float progress_override;
	Interaction a_interaction;

    {
	    std::shared_lock lock(q_mutex_);
	    if (const auto button = interactQueue.current_button) {
			a_type = button->type;
			progress_override = button->GetProgressOverride(false);
			a_interaction = button->interaction;
	    }
		else {
		    return;
		}
	}

	if (a_type != SkyPromptAPI::kSinglePress) {
	    if (const auto now = std::chrono::steady_clock::now(); !buttonState.isPressing) {
		    if (now - buttonState.lastPressTime > maxIntervalBetweenPresses) {
				if (buttonState.pressCount == 2) {
					RemoveCurrentPrompt();
					SendEvent(a_interaction, SkyPromptAPI::PromptEventType::kDeclined);
				}
				else if (buttonState.pressCount == 3) {
					NextPrompt();
				}
			    buttonState.pressCount = 0;
			}
	        else if (buttonState.pressCount == 4 || buttonState.pressCount >= 6) {
	            NextPrompt();
                buttonState.pressCount = 5;
	        }
		}
	}
	else {
		const auto [mult, frac] = splitFloat(progress_override);
		if (mult > 0.f && std::abs(frac)<EPSILON) {
			SendEvent(a_interaction, SkyPromptAPI::PromptEventType::kDeclined,{0.f,0.f},progress_override);
		}
	}
}

ImGui::Renderer::SubManager::~SubManager()
{
	ClearQueue();
}

void ImGui::Renderer::SubManager::Add2Q(const InteractionButton& iButton, const bool show)
{
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
	return interactQueue.RemoveButton(a_interaction);
}

void ImGui::Renderer::SubManager::RemoveFromQ(const SkyPromptAPI::PromptSink* a_prompt_sink)
{
	std::unique_lock lock(sink_mutex_);
	for (auto it = sinks.begin(); it != sinks.end();) {
		auto a_interaction = it->first;
		if (std::erase(it->second, a_prompt_sink)) {
		    RemoveFromQ(a_interaction);
		}

	    if (it->second.empty()) {
			it = sinks.erase(it);
		}
		else {
			++it;
		}
	}
}

void ImGui::Renderer::SubManager::RemoveCurrentPrompt()
{
	if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
		lock.unlock();
	    {
		    std::unique_lock lock2(q_mutex_);
		    interactQueue.Reset();
		    const auto* next_button = interactQueue.size() > 1 ? interactQueue.Next() : nullptr;
		    interactQueue.RemoveButton(interactQueue.current_button->interaction);
		    interactQueue.current_button = next_button;
	    }
	    {
		    std::unique_lock lock2(progress_mutex_);
		    progress_circle = 0.0f;
	    }
	}
}

void SubManager::ResetQueue() {
    {
	    std::unique_lock lock(q_mutex_);
	    interactQueue.Reset();
    }
    {
	    std::unique_lock lock(progress_mutex_);
	    progress_circle = 0.0f;
		buttonState.Reset();
    }
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

void ImGui::Renderer::SubManager::WakeUpQueue() {
    std::unique_lock lock(q_mutex_);
    interactQueue.WakeUp();
    wakeup_queued_.store(false);
}

SubManager* ImGui::Renderer::Manager::Add2Q(
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
            return a_manager.get();
        }
    }

	int index = 0;
	for (const auto& a_manager : *manager_list) {
        if (const auto& interactions = a_manager->GetInteractions(); interactions.empty()) {
			const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
			a_manager->Add2Q(iButton, show);
			return a_manager.get();
		}
        else if (interactions.front().event == a_interaction.event) {
			a_manager->WakeUpQueue();
            const auto iButton = InteractionButton(a_interaction, a_mutables, a_type, a_refid, a_bttn_map, index);
            a_manager->Add2Q(iButton, show);
			return a_manager.get();
        }
		++index;
	}

    if (manager_list->size() < MCP::Settings::n_max_buttons) {
	    // if no manager has the event, make a new manager
	    manager_list->emplace_back(std::make_unique<SubManager>());
	}
	else {
		return nullptr;
	}

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
	}
	else {
	    CleanUpQueue();
        ResetQueue();
	}


    std::unique_lock lock(mutex_);

	if (last_clientID != 0) {
        client_managers.at(last_clientID) = std::move(managers);
	}

	managers = std::move(client_managers.at(client_id));
	last_clientID = client_id;

	std::shared_lock theme_lock(Theme::m_theme_);
	Theme::last_theme = Theme::themes.contains(last_clientID) ? *Theme::themes.at(last_clientID) : Theme::default_theme;

	return true;
}

bool ImGui::Renderer::Manager::CycleClient(const bool a_left)
{
    std::shared_lock lock(mutex_);
    if (client_managers.size() <= 1) {
        return false;
    }
	const bool any_has_queue = std::ranges::any_of(client_managers | std::views::values,
	    [](const auto& managers) {
		    return !managers.empty() && std::ranges::any_of(managers, [](const auto& m) {
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

bool ImGui::Renderer::Manager::Add2Q(const SkyPromptAPI::PromptSink* a_prompt_sink, const SkyPromptAPI::ClientID a_clientID)
{

    for (const auto prompts = a_prompt_sink->GetPrompts();
		const auto& [text, a_event, a_action, a_type, a_refid, button_key, text_color, progress] : prompts) {
	    if (text.empty()) {
		    logger::warn("Empty prompt text");
		    return false;
	    }

		std::map<Input::DEVICE, uint32_t> temp_button_keys;
		for (const auto& [a_device, key] : button_key) {
			Input::DEVICE device = Input::from_RE_device(a_device);
			if (device == Input::DEVICE::kUnknown) {
				continue;
			}
			if (const auto a_new_key = MANAGER(Input)->Convert(key,a_device)) {
			    temp_button_keys[device] = a_new_key;
			}
		}
        const auto interaction = MakeInteraction(a_clientID,a_event,a_action);
        const auto a_txt = std::string(text);
		if (const auto submanager = Add2Q(a_clientID, interaction,{a_txt,text_color,progress}, a_type, a_refid,temp_button_keys, true)) {
            if (!GetManagerList(a_clientID)) {
				logger::error("Failed to get manager list");
				return false;
			}
			submanager->AddSink(interaction, a_prompt_sink);
		}
		else {
			logger::warn("Failed to add interaction to the queue");
			return false;
		}

	}

	SwitchToClientManager(a_clientID);

	return true;
}

bool ImGui::Renderer::Manager::IsInQueue(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink, const bool wake_up)
{
	bool result = false;

    const auto a_list = GetManagerList(a_clientID);

	if (!a_list) {
		return false;
	}

	std::shared_lock lock(mutex_);
	for (const auto& a_manager : *a_list) {
		if (a_manager->IsInQueue(a_prompt_sink)) {
			result = true;
			if (wake_up) {
				a_manager->Update(a_clientID,a_prompt_sink);
				a_manager->WakeUpQueue();
			}
			else {
				return result;
			}
		}
	}
	return result;
}

void ImGui::Renderer::Manager::RemoveFromQ(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink) {

	const auto manager_list = GetManagerList(a_clientID);

    if (!manager_list) {
		return;
	}

	{
        std::unique_lock lock(mutex_);
	    for (const auto& a_manager : *manager_list) {
		    a_manager->RemoveFromQ(a_prompt_sink);
	    }
	}
	{
        std::unique_lock lock(events_to_send_mutex);
        events_to_send_.erase(a_prompt_sink);
    }

	CleanUpQueue();

}

bool Manager::HasTask() const {
	for (const auto& a_manager : managers) {
		if (a_manager->HasQueue()) {
			return true;
		}
	}
	return false;
}

void ImGui::Renderer::SubManager::CleanUpQueue()
{
	// if everything has expired AND has alpha=0, clear the queue
	if (std::unique_lock lock(q_mutex_);
		interactQueue.expired() && interactQueue.alpha <= 0.f
		) {
		lock.unlock();
		ClearQueue(SkyPromptAPI::kTimeout);
	}
	ButtonStateActions();
}

void ImGui::Renderer::SubManager::ClearQueue()
{
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

void ImGui::Renderer::SubManager::ClearQueue(const SkyPromptAPI::PromptEventType a_event_type)
{
    {
        std::shared_lock lock(q_mutex_);
        for (const auto& a_button : interactQueue.buttons) {
	        SendEvent(a_button.interaction, a_event_type);
        }
    }
	ClearQueue();
}

bool ImGui::Renderer::SubManager::HasQueue() const
{
	std::shared_lock lock(q_mutex_);
	return !interactQueue.IsEmpty();
}

void ImGui::Renderer::SubManager::Start()
{
	blockProgress.store(false);
}

void ImGui::Renderer::SubManager::Stop()
{
	ResetQueue();
	blockProgress.store(false);
}

bool ImGui::Renderer::SubManager::UpdateProgressCircle(const bool isPressing)
{
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
		if (progress_circle > Theme::last_theme.progress_speed) {
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
	    progress_circle += GetSecondsSinceLastFrame() * Theme::last_theme.progress_speed*4.f;
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

    if (std::shared_lock lock(progress_mutex_); progress_circle > (a_type == SkyPromptAPI::kSinglePress ? 0.f : progress_circle_max)) {
        progress_circle = a_type == SkyPromptAPI::kHoldAndKeep ? progress_circle_max : 0.0f;
		lock.unlock();

        if (buttonState.pressCount == 3 && a_type != SkyPromptAPI::kSinglePress) {
            buttonState.pressCount = 0;
            ClearQueue(SkyPromptAPI::kDeclined);
        }
		else {
	        if (a_type == SkyPromptAPI::kHold) {
	            Stop();
	            RemoveCurrentPrompt();
	        }
			const auto curr_button = GetCurrentButton();
			SendEvent(interaction, SkyPromptAPI::PromptEventType::kAccepted, {0.f,0.f}, curr_button ? curr_button->GetProgressOverride(false) : 0.f);
	        Start();
			if (a_type != SkyPromptAPI::kHoldAndKeep) {
	            blockProgress.store(true);
			}
		}

        return true;
    }

	return false;
}

uint32_t ImGui::Renderer::SubManager::GetPromptKey() const {
	if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
		return interactQueue.current_button->GetKey();
	}
	return 0;
}

void ImGui::Renderer::SubManager::NextPrompt() {
    {
	    std::unique_lock lock(q_mutex_);
	    if (const auto next = interactQueue.Next()) {
		    Show(next);
	    }
    }
	if (Tutorial::Tutorial2::showing_tutorial.load()) {
		const SCENES::Event a_id = Tutorial::client_id*std::numeric_limits<SkyPromptAPI::ClientID>::max();
		if (const auto a_interaction = GetCurrentInteraction(); a_id == a_interaction.event) {
			Tutorial::Tutorial2::to_be_deleted.erase(static_cast<SkyPromptAPI::ActionID>(a_interaction.action-static_cast<ACTIONS::Action>(a_id)));
			if (Tutorial::Tutorial2::to_be_deleted.empty()) {
				SKSE::GetTaskInterface()->AddTask([]() {
					SkyPromptAPI::RemovePrompt(Tutorial::Tutorial2::Sink::GetSingleton(),Tutorial::client_id);
					Tutorial::Tutorial2::showing_tutorial.store(false);
					if (!SkyPromptAPI::SendPrompt(Tutorial::Tutorial3::Sink::GetSingleton(),Tutorial::client_id)) {
						logger::error("Failed to Send Tutorial3 prompts.");
					}
				}
				);
			}
		}
	}
}

bool ImGui::Renderer::SubManager::HasPrompt() const
{
	std::shared_lock lock(q_mutex_);
	return interactQueue.current_button;
}

bool ImGui::Renderer::SubManager::IsHidden() const
{
	if (!HasPrompt()) {
		return true;
	}
	std::shared_lock lock(q_mutex_);
	return interactQueue.IsHidden();
}

std::vector<Interaction> ImGui::Renderer::SubManager::GetInteractions() const {
	std::shared_lock lock(q_mutex_);
	std::vector<Interaction> interactions;
	for (const auto& a_button : interactQueue.buttons) {
		interactions.push_back(a_button.interaction);
	}
	return interactions;
}

Interaction ImGui::Renderer::SubManager::GetCurrentInteraction() const
{
	std::shared_lock lock(q_mutex_);
	if (interactQueue.current_button) {
		return interactQueue.current_button->interaction;
	}
	return {};
}

std::vector<InteractionButton> ImGui::Renderer::SubManager::GetButtons() const
{
	std::shared_lock lock(q_mutex_);
	std::vector<InteractionButton> buttons;
	for (const auto& a_button : interactQueue.buttons) {
		buttons.push_back(a_button);
	}
	return buttons;
}

const InteractionButton* ImGui::Renderer::SubManager::GetCurrentButton() const {
	std::shared_lock lock(q_mutex_);
	if (interactQueue.current_button) {
		return interactQueue.current_button;
	}
	return nullptr;
}

void ImGui::Renderer::SubManager::AddSink(const Interaction& a_interaction, const SkyPromptAPI::PromptSink* a_sink)
{
	{
		std::shared_lock lock(sink_mutex_);
		// if the sink is already in the queue, return
		if (const auto it = sinks.find(a_interaction); it != sinks.end()) {
			if (std::ranges::find(it->second, a_sink) != it->second.end()) {
				return;
			}
		}
	}
	std::unique_lock lock(sink_mutex_);
	sinks[a_interaction].push_back(a_sink);
}

bool ImGui::Renderer::SubManager::IsInQueue(const SkyPromptAPI::PromptSink* a_sink) const
{
	std::shared_lock lock(sink_mutex_);
	for (const auto& sinks_ : sinks | std::views::values) {
		if (auto it = std::ranges::find(sinks_, a_sink); it != sinks_.end()) {
			return true;
		}
	}
	return false;
}

bool ImGui::Renderer::SubManager::IsInQueue(const Interaction& a_interaction) const
{
	std::shared_lock lock(q_mutex_);
	for (const auto& a_button : interactQueue.buttons) {
		if (a_button.interaction == a_interaction) {
			return true;
		}
	}
	return false;
}

uint32_t InteractionButton::GetKey() const
{
	const auto manager = MANAGER(Input)->GetSingleton();
	const auto a_device = manager->GetInputDevice();
	return keys.contains(a_device) ? keys.at(a_device) : MCP::Settings::prompt_keys.at(a_device).at(default_key_index);
}

InteractionButton::InteractionButton(const Interaction& a_interaction, const Mutables& a_mutables,
    const SkyPromptAPI::PromptType a_type, const RefID a_refid, std::map<Input::DEVICE, uint32_t> a_keys, const int a_default_key_index) {
	interaction = a_interaction;
	type = a_type;
	mutables = a_mutables;
	if (const auto a_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refid)) {
		attached_object = a_ref->GetHandle();
	}
	else if (const auto temp = RE::Inventory3DManager::GetSingleton()->tempRef) {
	    if (temp->GetFormID() == a_refid) {
			attached_object = temp->GetHandle();
	    }
	}
	keys = std::move(a_keys);
	default_key_index = a_default_key_index;
}

void ImGui::Renderer::Manager::Start()
{
	isPaused.store(false);
	std::unique_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		a_manager->Start();
	}
}

void ImGui::Renderer::Manager::Stop()
{
	isPaused.store(true);
	std::unique_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		a_manager->Stop();
	}
}

void ImGui::Renderer::Manager::CleanUpQueue()
{
	std::vector<size_t> to_remove;

	{
		std::shared_lock lock(mutex_);
		for (size_t i = 0; i < managers.size(); ++i) {
			managers[i]->CleanUpQueue();
			if (!managers[i]->HasQueue()) {
				to_remove.push_back(i);
			}
		}
	}
	if (!to_remove.empty()) {
        {
		    std::unique_lock lock(mutex_);
		    std::sort(to_remove.rbegin(), to_remove.rend());
		    for (const size_t idx : to_remove) {
			    managers.erase(managers.begin() + idx);
		    }
        }
		if (std::shared_lock lock(mutex_);managers.empty()) {
			lock.unlock();
			CycleClient(false);
			return;
		}
		ReArrange();
	}
}

void ImGui::Renderer::Manager::ShowQueue() {
	if (IsPaused()) {
		return;
	}
	
    // Get the screen size
    const auto [width, height] = RE::BSGraphics::Renderer::GetScreenSize();

    // Calculate position
    const ImVec2 bottomRightPos(
        width * Theme::last_theme.xPercent - Theme::last_theme.marginX,
        height * Theme::last_theme.yPercent - Theme::last_theme.marginY
    );

    // Set the window position
    ImGui::SetNextWindowPos(bottomRightPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f)); // Pivot at the bottom-right

	BeginImGuiWindow("SkyPrompt");

	std::map<RefID,std::vector<SubManager*>> object_managers;

	for (std::shared_lock lock(mutex_);
		auto& a_manager : managers) {
		if (const auto a_ref = a_manager->GetAttachedObject()) {
			object_managers[a_ref->GetFormID()].push_back(a_manager.get());
		    continue;
		}
		a_manager->ShowQueue();
	}

	if (MCP::Settings::cycle_controls.load()) {

	    SkyPromptAPI::ClientID n_has_prompts = 0;
	    SkyPromptAPI::ClientID index = 0;
	    for (std::shared_lock lock(mutex_);
			const auto& [a_clientID,a_managers]: client_managers) {
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
		    DrawCycleIndicators(index+1, n_has_prompts+1);
        }
	}

	EndImGuiWindow();

	int i = 0;
	for (std::shared_lock lock(mutex_);
		const auto& managers_ : object_managers | std::views::values) {
		auto window_pos = managers_[0]->GetAttachedObjectPos();
		SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		BeginImGuiWindow(std::format("SkyPromptHover{}",i++).c_str());
		for (const auto a_manager : managers_) {
			a_manager->ShowQueue();
		}
	    EndImGuiWindow();
	}
}

void ImGui::Renderer::Manager::Clear(const SkyPromptAPI::PromptEventType a_event_type)
{
	std::unique_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		a_manager->ClearQueue(a_event_type);
	}
    managers.clear();
}

Interaction ImGui::Renderer::Manager::MakeInteraction(const SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::EventID a_event, const SkyPromptAPI::ActionID a_action)
{
	const uint32_t start_index = a_clientID * std::numeric_limits<SkyPromptAPI::ClientID>::max();
	const uint32_t event_id = start_index + a_event;
	const uint32_t action_id = start_index + a_action;
	auto interaction = Interaction(event_id, action_id);
	return interaction;
}

void ImGui::Renderer::Manager::ResetQueue() const {
	std::unique_lock lock(mutex_);
	for (auto& a_manager : managers) {
		a_manager->ResetQueue();
	}
}

void ImGui::Renderer::Manager::WakeUpQueue() const
{
	std::unique_lock lock(mutex_);
	for (auto& a_manager : managers) {
		a_manager->WakeUpQueue();
	}
}

bool ImGui::Renderer::Manager::IsHidden() const
{
	std::shared_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		if (!a_manager->IsHidden()) {
			return false;
		}
	}
	return true;
}

SubManager* ImGui::Renderer::Manager::GetSubManagerByKey(const uint32_t a_prompt_key) const {
	std::shared_lock lock(mutex_);
	for (auto& a_manager : managers) {
		if (const auto key = a_manager->GetPromptKey(); key == a_prompt_key) {
			return a_manager.get();
		}
	}
	return nullptr;

}

std::vector<uint32_t> ImGui::Renderer::Manager::GetPromptKeys() const
{
	std::vector<uint32_t> keys;
	for (std::shared_lock lock(mutex_); const auto& a_manager : managers) {
		if (a_manager->IsHidden()) {
			continue;
		}
		if (const auto key = a_manager->GetPromptKey(); key != 0) {
			keys.push_back(key);
		}
	}
	return keys;
}

void ImGui::Renderer::Manager::ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func) {
	std::unique_lock lock(mutex_);
	for (auto& a_manager : managers) {
		a_func(a_manager);
	}
}

void ImGui::Renderer::Manager::AddEventToSend(const SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::Prompt& a_prompt, const SkyPromptAPI::PromptEventType event_type, const std::
                                              pair<float, float> a_delta)
{
	std::unique_lock lock(events_to_send_mutex);
	if (const auto it = events_to_send_.find(a_sink); it != events_to_send_.end()) {
		it->second.push_back({ a_prompt, event_type, a_delta });
	}
	else {
		events_to_send_[a_sink] = { { a_prompt, event_type, a_delta } };
	}
}

void Manager::SendEvents() {

    std::vector<const SkyPromptAPI::PromptSink*> sinks_to_notify;

	std::shared_lock lock(events_to_send_mutex);

    for (const auto& sink : events_to_send_ | std::views::keys) {
		if (sink) {
            sinks_to_notify.push_back(sink);
		}
    }

    for (const auto sink : sinks_to_notify) {
        std::vector<SkyPromptAPI::PromptEvent> events;
        if (auto it = events_to_send_.find(sink); it != events_to_send_.end()) {
            events = it->second;
        }
        for (const auto& event : events) {
			if (!sink || !events_to_send_.contains(sink)) {
				break;
			}
			lock.unlock();
            sink->ProcessEvent(event);
			lock.lock();
        }
	}

	lock.unlock();

	std::unique_lock lock2(events_to_send_mutex);
	events_to_send_.clear();
}
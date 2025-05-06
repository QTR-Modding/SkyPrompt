#include "Renderer.h"
#include "Hooks.h"
#include "Tasker.h"
#include "IconsFonts.h"
#include "MCP.h"
#include "Settings.h"
#include "Styles.h"
#include "Utils.h"
#include "Geometry.h"

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
    const auto buttonKey = button_key();
	const auto icon_manager = MANAGER(IconFont);
	if (icon_manager->unavailable_keys.contains(buttonKey)) return std::nullopt;
	const std::string base_text = text; // Cache the base text
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

	// progress circle

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
	std::optional<std::pair<float,float>> a_position;
    if (buttonIcon->srView.Get()) {
        const auto position = ImGui::ButtonIconWithCircularProgress(a_text.c_str(), buttonIcon,progress,button_state);
		a_position = { position.x,position.y };
    } else {
        logger::error("Button icon texture not loaded for key {}", buttonKey);
		icon_manager->unavailable_keys.insert(buttonKey);
    }

    ImGui::PopStyleVar(); // Restore alpha

	return a_position;
}

void ButtonQueue::Clear() {
	current_button = nullptr;
	buttons.clear();
	Reset();
}

void ButtonQueue::Reset() const {
	lifetime = MCP::Settings::lifetime;
	alpha = 0.0f; // Reset alpha to start fade-in
	elapsed = 0.0f;
}

void ButtonQueue::WakeUp() const {
	// wake up all buttons
	lifetime = MCP::Settings::lifetime;
	alpha = 1.0f;
	elapsed = 0.0f;
}

namespace {
    float ButtonStateToFloat(const ButtonState& a_button_state) {
		auto a_press_count = a_button_state.pressCount;
		if (a_button_state.isPressing && a_button_state.pressCount < 3) {
            a_press_count--;
		}
	    return static_cast<float>(a_press_count) + 0.1f;
    }
}

void ButtonQueue::Show(const float progress, const InteractionButton* button2show, const ButtonState& a_button_state) {
	if (button2show) {
		Reset();
		current_button = button2show;
	}
	else if (current_button) {
		if (!Manager::IsGameFrozen()) {
		    if (expired()/* && current_button->alpha>0.f*/) {
				alpha = std::max(alpha - MCP::Settings::fadeSpeed, 0.0f);
	        }
			else {
                alpha = std::min(alpha + MCP::Settings::fadeSpeed, 1.0f);
			}
			elapsed += RE::GetSecondsSinceLastFrame();
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
        if (const auto pos = current_button->Show(alpha,extra_text,progress,current_button->type == SkyPromptAPI::kSinglePress ? -1.f : ButtonStateToFloat(a_button_state)); 
			pos.has_value()) {
			position = pos.value();
		}
	}
	else {
        current_button = Next();
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
	bool erased = false;
    if (current_button && current_button->interaction == a_interaction) {
	    buttons.erase(*current_button);
		erased = true;
	}
	// otherwise we need to find it in the Q.
	else {
		for (auto it = buttons.begin(); it != buttons.end(); ++it) {
			if (it->interaction == a_interaction) {
				buttons.erase(it);
				erased = true;
				break;
			}
		}
	}
	if (erased) {
	    current_button = nullptr;
        Reset();
	}
	return erased;
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
	std::map<Interaction,std::vector<SkyPromptAPI::PromptSink*>> sinks;
    std::map<Input::DEVICE, std::vector<uint32_t>> keys;
    std::map<SCENES::Event, RefID> objects;

	{
		std::shared_lock lock(mutex_);
		for (const auto& a_manager : managers) {

		    for (const auto& interaction_button : a_manager->GetButtons()) {
				auto& interaction = interaction_button.interaction;
				for (auto i = Input::DEVICE::kKeyboardMouse; i < Input::DEVICE::kTotal;
                     i = static_cast<Input::DEVICE>(static_cast<int>(i) + 1)) {
                    keys[i].push_back(ButtonSettings::GetInteractionKey(interaction, i));
				}
				
                interactions[interaction.event].push_back(interaction_button);
            }

            objects[a_manager->GetCurrentInteraction().event] = a_manager->GetAttachedObjectID();

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

	Clear();

	// distribute the interactions to the managers
	for (const auto& interaction_buttons : interactions | std::views::values) {
		for (const auto& interaction_button : interaction_buttons) {
			const auto& interaction = interaction_button.interaction;
            if (const auto submanager = Add2Q(interaction, interaction_button.type, true, keys).get()) {
                if (!submanager->GetAttachedObjectID() && objects.contains(interaction.event)) {
                    submanager->Attach2Object(objects.at(interaction.event));
				}
			} 
			else {
				logger::error("Failed to add interaction to the queue");
			}

		}
	}

	// distribute the sinks to the managers
	{
		std::shared_lock lock(mutex_);
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

void ImGui::Renderer::SubManager::SendEvent(const Interaction& a_interaction, const SkyPromptAPI::PromptEventType event_type, const std::pair<float,float> delta)
{
	std::shared_lock lock(sink_mutex_);
	if (const auto it = sinks.find(a_interaction); it != sinks.end()) {
		for (const auto& a_sink : it->second) {
			if (!a_sink) continue;
			for (const auto& prompt : a_sink->GetPrompts()) {
				if (prompt.text == a_interaction.name()) {
					Manager::GetSingleton()->AddEventToSend(a_sink, prompt, event_type, delta);
				}
			}
		}
	}
}

bool ImGui::Renderer::SubManager::Attach2Object(const RefID a_refid)
{
	if (const auto a_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refid)) {
		attached_object = a_ref->GetHandle();
		return true;
	}
	return false;
}

static ImVec2 WorldToScreenLoc(const RE::NiPoint3 position) {
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

ImVec2 ImGui::Renderer::SubManager::GetAttachedObjectPos() const
{
	if (const auto ref = attached_object.get().get()) {
        auto geo = Geometry(ref);

        const auto bound = geo.GetBoundingBox(ref->GetAngle(), ref->GetScale());

        const RE::NiPoint3 center = (bound.first + bound.second) / 2;

        const RE::NiPoint3 pos = ref->GetPosition() + RE::NiPoint3{center.x, center.y, bound.second.z + 16};

        const ImVec2 pos2d = WorldToScreenLoc(pos);

		return pos2d;
	}
	return {};
}

void ImGui::Renderer::SubManager::RemoveFromSinks(SkyPromptAPI::PromptSink* a_prompt_sink)
{
	std::unique_lock lock(sink_mutex_);
	for (auto& sinks_ : sinks | std::views::values) {
		if (const auto it = std::ranges::find(sinks_, a_prompt_sink); it != sinks_.end()) {
			sinks_.erase(it);
		}
	}
}

void ImGui::Renderer::SubManager::ButtonStateActions()
{
	buttonState.pressCount = std::min(3,buttonState.pressCount);

	if (const auto now = std::chrono::steady_clock::now(); !buttonState.isPressing) {
		if (now - buttonState.lastPressTime > maxIntervalBetweenPresses) {
			if (buttonState.pressCount == 2) {
			    const auto a_interaction = GetCurrentInteraction();
                RemoveCurrentPrompt();
			    SendEvent(a_interaction, SkyPromptAPI::PromptEventType::kDeclined);
			}
			else if (buttonState.pressCount == 3) {
				NextPrompt();
			}
			buttonState.pressCount = 0;
		}
	}
}

ImGui::Renderer::SubManager::~SubManager()
{
	ClearQueue();
}

void ImGui::Renderer::SubManager::Add2Q(const Interaction& a_interaction, const SkyPromptAPI::PromptType a_type, const bool show)
{
	const InteractionButton iButton(a_interaction, a_type);
	std::unique_lock lock(q_mutex_);
    if (const auto button = interactQueue.AddButton(iButton); button && show) {
		std::shared_lock lock2(progress_mutex_);
		if (!Manager::GetSingleton()->IsPaused() && progress_circle == 0.f) {
            interactQueue.Show(progress_circle,button,buttonState);
		}
	}
}

bool SubManager::RemoveFromQ(const Interaction& a_interaction) {

	std::unique_lock lock(q_mutex_);
	return interactQueue.RemoveButton(a_interaction);
}

void ImGui::Renderer::SubManager::RemoveFromQ(SkyPromptAPI::PromptSink* a_prompt_sink)
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
	    auto a_interaction = interactQueue.current_button->interaction;
		lock.unlock();
		std::unique_lock lock2(q_mutex_);
		interactQueue.Reset();
		const auto* next_button = interactQueue.size() > 1 ? interactQueue.Next() : nullptr;
		interactQueue.RemoveButton(interactQueue.current_button->interaction);
		interactQueue.current_button = next_button;
		progress_circle = 0.0f;
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
		interactQueue.Show(progress_circle,nullptr,buttonState);
	}
}

void ImGui::Renderer::SubManager::WakeUpQueue() const {
    std::unique_lock lock(q_mutex_);
    interactQueue.WakeUp();
}

std::unique_ptr<SubManager>& ImGui::Renderer::Manager::Add2Q(
    const Interaction& a_interaction, const SkyPromptAPI::PromptType a_type,
    const bool show, const std::map<Input::DEVICE, std::vector<uint32_t>>& buttonKeys) {
    // WakeUpQueue();
    size_t index = 0;
    std::unique_lock lock(mutex_);
	for (auto& a_manager : managers) {
        if (a_manager->HasQueue() && a_manager->GetInteractions().front().event == a_interaction.event) {
            for (const auto& [a_device, keys] : buttonKeys.empty() ? MCP::Settings::prompt_keys : buttonKeys) {
                ButtonSettings::SetInteractionKey(a_interaction, a_device, keys.at(index));
            }
            // logger::info("Index: {}, event {}, action {}", index, static_cast<int>(a_interaction.event),
            // static_cast<int>(a_interaction.action.action));
			a_manager->WakeUpQueue();
            a_manager->Add2Q(a_interaction, a_type, show);
			return a_manager;
        }
		++index;
	}
	if (managers.size() < MCP::Settings::n_max_buttons) {
	    // if no manager has the event, make a new manager
	    managers.emplace_back(std::make_unique<SubManager>());

	}
    //logger::info("Index: {}, event {}, action {}", index, static_cast<int>(a_interaction.event), static_cast<int>(a_interaction.action.action));
    for (const auto& [a_device, keys] : buttonKeys.empty() ? MCP::Settings::prompt_keys : buttonKeys) {
        ButtonSettings::SetInteractionKey(a_interaction, a_device, keys.at(index));
	}
    managers.back()->Add2Q(a_interaction, a_type, show);
	return managers.back();
}

bool ImGui::Renderer::Manager::Add2Q(SkyPromptAPI::PromptSink* a_prompt_sink, const SkyPromptAPI::ClientID a_clientID)
{
    for (const auto& prompts = a_prompt_sink->GetPrompts();
		const auto& [text, button_key, a_event, a_action, a_type, a_refid] : prompts) {
	    if (text.empty()) {
		    logger::warn("Empty prompt text");
		    return false;
	    }

        const uint32_t start_index = a_clientID * std::numeric_limits<SkyPromptAPI::ClientID>::max();
		const uint32_t event_id = start_index + a_event;
		const uint32_t action_id = start_index + a_action;
		auto interaction = Interaction(event_id, action_id);
		interaction.text = text;

		if (const auto& submanager = Add2Q(interaction, a_type)) {
			if (const auto a_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_refid)) {
				submanager->Attach2Object(a_ref->GetFormID());
			}
			submanager->AddSink(interaction, a_prompt_sink);
		}
		else {
			logger::warn("Failed to add interaction to the queue");
			return false;
		}

	    if (!button_key.empty()) {
			for (const auto [a_device, key] : button_key) {
			    Input::DEVICE device;
			    switch (a_device) {
			    case RE::INPUT_DEVICE::kKeyboard:
				    device = Input::DEVICE::kKeyboardMouse;
				    break;
			    case RE::INPUT_DEVICE::kMouse:
				    device = Input::DEVICE::kKeyboardMouse;
				    break;
			    case RE::INPUT_DEVICE::kGamepad:
				    device = RE::ControlMap::GetSingleton()->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis ? Input::DEVICE::kGamepadOrbis : Input::DEVICE::kGamepadDirectX;
				    break;
			    default:
				    continue;
			    }
			    ButtonSettings::SetInteractionKey(interaction, device, Input::Manager::GetSingleton()->Convert(key,a_device));
		    }
	    }
	}

	return true;
}

bool ImGui::Renderer::Manager::IsInQueue(SkyPromptAPI::PromptSink* a_prompt_sink, const bool wake_up) const
{
	bool result = false;
	std::shared_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		if (a_manager->IsInQueue(a_prompt_sink)) {
			result = true;
			if (wake_up) {
				a_manager->WakeUpQueue();
			}
			else {
				return result;
			}
		}
	}
	return result;
}

void ImGui::Renderer::Manager::RemoveFromQ(SkyPromptAPI::PromptSink* a_prompt_sink) const {
	std::unique_lock lock(mutex_);
	for (const auto& a_manager : managers) {
		a_manager->RemoveFromQ(a_prompt_sink);
	}
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

void ImGui::Renderer::SubManager::ClearQueue(const SkyPromptAPI::PromptEventType a_type)
{
    {
        std::shared_lock lock(q_mutex_);
        for (const auto& a_button : interactQueue.buttons) {
			SendEvent(a_button.interaction, a_type);
        }
    }

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
	//{
	    //std::unique_lock lock(q_mutex_);
	    /*if (interactQueue.current_button) {
            interactQueue.current_button->Reset();
	    }*/
	//}
	blockProgress.store(false);
}

bool ImGui::Renderer::SubManager::UpdateProgressCircle(const bool isPressing)
{

	if (!isPressing) {
	    std::unique_lock lock(progress_mutex_);
		if (progress_circle > 0.5f*MCP::Settings::progress_speed) {
			buttonState.pressCount = 0;
		}
        progress_circle = 0.0f;
		blockProgress.store(false);
		return false;
	}

	if (blockProgress.load()) {
	    return false;
	}

	Tasker::GetSingleton()->PushTask([] {
	    Manager::GetSingleton()->WakeUpQueue();
	},
		100
	);

	{
	    std::unique_lock lock(progress_mutex_);
	    progress_circle += RE::GetSecondsSinceLastFrame() * MCP::Settings::progress_speed*4.f / (RE::BSTimer::GetSingleton()->QGlobalTimeMultiplier()+EPSILON);
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
        progress_circle = 0.0f;
		lock.unlock();

        if (buttonState.pressCount == 3) {
		    buttonState.pressCount = 0;
		    ClearQueue();
	    }
		else {
	        if (a_type == SkyPromptAPI::kHold) {
	            Stop();
	            RemoveCurrentPrompt();
	        }
	        SendEvent(interaction, SkyPromptAPI::PromptEventType::kAccepted);
	        Start();
	        blockProgress.store(true);
		}

		return true;
    }

	return false;
}

uint32_t ImGui::Renderer::SubManager::GetPromptKey() const {
	if (std::shared_lock lock(q_mutex_); interactQueue.current_button) {
		return interactQueue.current_button->button_key();
	}
	return 0;
}

void ImGui::Renderer::SubManager::NextPrompt() {
	std::unique_lock lock(q_mutex_);
	if (const auto next = interactQueue.Next()) {
		interactQueue.Show(progress_circle,next,buttonState);
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

void ImGui::Renderer::SubManager::AddSink(const Interaction& a_interaction, SkyPromptAPI::PromptSink* a_sink)
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

bool ImGui::Renderer::SubManager::IsInQueue(SkyPromptAPI::PromptSink* a_sink) const
{
	std::shared_lock lock(sink_mutex_);
	for (const auto& sinks_ : sinks | std::views::values) {
		if (std::ranges::find(sinks_, a_sink) != sinks_.end()) {
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

uint32_t InteractionButton::button_key() const
{
	using namespace ButtonSettings;
	const auto manager = MANAGER(Input)->GetSingleton();
	const auto a_device = manager->GetInputDevice();
	std::shared_lock lock(button_key_lock);
	return GetInteractionKey(interaction, a_device);
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
	bool deleted = false;
    {
	    std::shared_lock lock(mutex_);
	    // delete empty managers
	    for (auto it = managers.begin(); it != managers.end();) {
		    it->get()->CleanUpQueue();
		    if (!it->get()->HasQueue()) {
			    lock.unlock();
			    {
			        std::unique_lock lock2(mutex_);
			        it = managers.erase(it);
			    }
			    lock.lock();
			    deleted = true;
		    }
		    else {
			    ++it;
		    }
	    }
    }
	if (deleted) {
		ReArrange();
	}
}

void ImGui::Renderer::Manager::ShowQueue() const {
	if (IsPaused()) {
		return;
	}
	
    // Get the screen size
    const auto [width, height] = RE::BSGraphics::Renderer::GetScreenSize();

    // Calculate position
    const ImVec2 bottomRightPos(
        width * MCP::Settings::xPercent - MCP::Settings::marginX,
        height * MCP::Settings::yPercent - MCP::Settings::marginY
    );

    // Set the window position
    ImGui::SetNextWindowPos(bottomRightPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f)); // Pivot at the bottom-right

	BeginImGuiWindow("SkyPrompt");

	std::map<RefID,std::vector<SubManager*>> object_managers;
	std::shared_lock lock(mutex_);
	for (auto& a_manager : managers) {
		if (a_manager->IsAttached2Object()) {
			object_managers[a_manager->GetAttachedObjectID()].push_back(a_manager.get());
		    continue;
		}
		a_manager->ShowQueue();
	}

	EndImGuiWindow();

	int i = 0;
	for (const auto& managers_ : object_managers | std::views::values) {
		auto window_pos = managers_[0]->GetAttachedObjectPos();
		SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		BeginImGuiWindow(std::format("SkyPromptHover{}",i++).c_str());
		for (const auto a_manager : managers_) {
			a_manager->ShowQueue();
		}
	    EndImGuiWindow();
	}
}

void ImGui::Renderer::Manager::Clear(const bool API_call)
{
    std::unique_lock lock(mutex_);
    for (const auto& a_manager : managers) {
		if (API_call) {
            a_manager->ClearQueue(SkyPromptAPI::kRemovedByMod);
		}
		else {
			a_manager->ClearQueue();
		}
    }
    managers.clear();
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

void ImGui::Renderer::Manager::AddEventToSend(SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::Prompt& a_prompt, const SkyPromptAPI::PromptEventType event_type, const std::
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

void ImGui::Renderer::Manager::SendEvents()
{
	std::vector<std::pair<SkyPromptAPI::PromptSink*, std::vector<SkyPromptAPI::PromptEvent>>> events_copy;
	{
		std::unique_lock lock(events_to_send_mutex);
		events_copy.reserve(events_to_send_.size());
		for (const auto& [a_sink, events] : events_to_send_) {
			events_copy.push_back({ a_sink, events });
		}
	    events_to_send_.clear();
	}
	for (const auto& [a_sink, events] : events_copy) {
		for (const auto& prompt_event : events) {
			if (!a_sink) continue;
			a_sink->ProcessEvent(prompt_event);
		}
	}
}

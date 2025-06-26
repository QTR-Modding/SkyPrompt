#include "Sinks.h"

void PapyrusAPI::PapyrusSink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) return;

    const auto a_type = event.type;
    if (a_type != SkyPromptAPI::PromptEventType::kMove) {
        if (std::shared_lock lock(prompt_mutex_); last_type == a_type) {
            return;
        }
    }
    {
	    std::unique_lock lock(prompt_mutex_);
	    last_type = a_type;
    }

    skyPromptEvents.QueueEvent(
        static_cast<int>(clientID),
        static_cast<int>(event.type),
        static_cast<int>(event.prompt.eventID),
        static_cast<int>(event.prompt.actionID),
        static_cast<float>(event.delta.first),
        static_cast<float>(event.delta.second),
        static_cast<float>(event.prompt.progress)
    );
}

std::span<const SkyPromptAPI::Prompt> PapyrusAPI::PapyrusSink::GetPrompts() const {
    std::shared_lock lock(prompt_mutex_);
    return { &prompt, 1 };
}

SkyPromptAPI::EventID PapyrusAPI::PapyrusSink::GetEventID() const {
    std::shared_lock lock(prompt_mutex_);
    return prompt.eventID;
}

SkyPromptAPI::ActionID PapyrusAPI::PapyrusSink::GetActionID() const {
    std::shared_lock lock(prompt_mutex_);
    return prompt.actionID;
}

void PapyrusAPI::PapyrusSink::SetKeys(const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>>& buttonKeys)
{

    std::unique_lock lock(prompt_mutex_);
    prompt.button_key = {};
    bindings = buttonKeys;
    prompt.button_key = bindings;
}

void PapyrusAPI::PapyrusSink::SetText(const std::string& a_text)
{
    if (!a_text.empty()) {
        std::unique_lock lock(prompt_mutex_);
        prompt.text = "";
        text = a_text;
        prompt.text = text;
    }
}

void PapyrusAPI::PapyrusSink::SetType(const SkyPromptAPI::PromptType a_type)
{
    std::unique_lock lock(prompt_mutex_);
    prompt.type = a_type;
}

void PapyrusAPI::PapyrusSink::SetRefID(const RE::FormID a_refid)
{
    std::unique_lock lock(prompt_mutex_);
    prompt.refid = a_refid;
}

void PapyrusAPI::PapyrusSink::SetProgress(const float progress)
{
    std::unique_lock lock(prompt_mutex_);
	prompt.progress = progress;
}

void PapyrusAPI::PapyrusSink::SetEventID(const SkyPromptAPI::EventID a_eventID)
{
    std::unique_lock lock(prompt_mutex_);
    prompt.eventID = a_eventID;
}

void PapyrusAPI::PapyrusSink::SetActionID(const SkyPromptAPI::ActionID a_actionID)
{
    std::unique_lock lock(prompt_mutex_);
    prompt.actionID = a_actionID;
}

bool PapyrusAPI::AddPrompt(SkyPromptAPI::ClientID clientID, const std::string& text,
                           // NOLINT(misc-use-internal-linkage)
                           const SkyPromptAPI::EventID eventID, const SkyPromptAPI::ActionID actionID,
                           const SkyPromptAPI::PromptType type, const RE::TESForm* refForm,
                           const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>>& buttonKeys, const float progress) {
    {
        std::shared_lock lock(mutex_);
        if (papyrusSinks.contains(clientID)) {
		    auto& sinks = papyrusSinks.at(clientID);
            if (const auto it = sinks.find({ eventID, actionID }); it != sinks.end()) {
			    lock.unlock();
			    std::unique_lock lock2(mutex_);
			    it->second->SetKeys(buttonKeys);
			    it->second->SetText(text);
			    it->second->SetType(type);
			    it->second->SetRefID(refForm ? refForm->GetFormID() : 0);
				it->second->SetProgress(progress);
			    return true;
		    }
        }
    }

    std::unique_lock lock(mutex_);
    papyrusSinks[clientID][{eventID,actionID}] = std::make_unique<PapyrusSink>(clientID);
    const auto& sinks = papyrusSinks.at(clientID);
    auto& sink = sinks.at({ eventID, actionID });
    sink->SetKeys(buttonKeys);
    sink->SetText(text);
    sink->SetType(type);
    sink->SetRefID(refForm ? refForm->GetFormID() : 0);
	sink->SetProgress(progress);
    sink->SetEventID(eventID);
    sink->SetActionID(actionID);
    return true;
}
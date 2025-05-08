#include "Sinks.h"

void PapyrusAPI::PapyrusSink::ProcessEvent(SkyPromptAPI::PromptEvent event) {
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

    const auto args =
    RE::MakeFunctionArguments(
        static_cast<int>(clientID),
        static_cast<int>(event.type),
        static_cast<int>(event.prompt.eventID),
        static_cast<int>(event.prompt.actionID),
        static_cast<float>(event.delta.first),
        static_cast<float>(event.delta.second)
    );

    vm->SendEventAll("SkyPromptEvent", args);
}

PapyrusAPI::PapyrusSink* PapyrusAPI::AddPrompt(SkyPromptAPI::ClientID clientID, const std::string& text,
    const SkyPromptAPI::EventID eventID, const SkyPromptAPI::ActionID actionID, const SkyPromptAPI::PromptType type,
    const RE::TESForm* refForm, std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>>* buttonKeys) {

	if (const auto a_sink = GetPrompt(clientID, eventID, actionID)) {
		a_sink->prompt.text = text;
		a_sink->prompt.type = type;
		a_sink->prompt.refid = refForm ? refForm->GetFormID() : 0;
		if (buttonKeys) {
			a_sink->prompt.button_key = *buttonKeys;
		}
		return a_sink;
	}

	auto sink = std::make_unique<PapyrusSink>(clientID);
	sink->prompt.text = text;
	sink->prompt.eventID = eventID;
	sink->prompt.actionID = actionID;
	sink->prompt.type = type;
	sink->prompt.refid = refForm ? refForm->GetFormID() : 0;
	auto* sink_ptr = sink.get();
	std::unique_lock lock(mutex_);
	papyrusSinks[clientID][{eventID,actionID}] = std::move(sink);
	return sink_ptr;
}

PapyrusAPI::PapyrusSink* PapyrusAPI::GetPrompt(SkyPromptAPI::ClientID const clientID, SkyPromptAPI::EventID eventID, SkyPromptAPI::ActionID actionID)
{
	std::shared_lock lock(mutex_);
	if (papyrusSinks.empty()) {
		return nullptr;
	}
	if (!papyrusSinks.contains(clientID)) {
		return nullptr;
    }

	auto& sinks = papyrusSinks.at(clientID);
	const auto it = sinks.find({ eventID, actionID });
    return it != sinks.end() ? it->second.get() : nullptr;

}

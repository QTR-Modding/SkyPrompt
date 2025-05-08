#include "Service.h"
#include "Renderer.h"

bool ProcessSendPrompt(SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {

	if (!a_sink) {
		return false;
	}
	if (a_clientID == 0) {
		return false;
	}

	const auto manager = MANAGER(ImGui::Renderer);
	if (manager->IsInQueue(a_clientID, a_sink, true)) {
		return false;
	}

    for (const auto new_prompts = a_sink->GetPrompts(); auto& prompt : new_prompts) {
		if (prompt.text.empty()) {
			logger::warn("Empty prompt text");
			return false;
		}
	}
	return manager->Add2Q(a_sink, a_clientID);
}

void ProcessRemovePrompt(SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {

	if (!a_sink) {
		return;
	}

    if (a_clientID == 0) {
		return;
	}
	const auto manager = MANAGER(ImGui::Renderer);
	if (!manager->IsInQueue(a_clientID, a_sink)) {
		return;
	}

	manager->RemoveFromQ(a_clientID, a_sink);
}

SkyPromptAPI::ClientID ProcessRequestClientID()
{
	std::lock_guard lock(Interactions::mutex_);
	if (Interactions::last_clientID == std::numeric_limits<SkyPromptAPI::ClientID>::max()) {
		return 0;
	}
	if (MANAGER(ImGui::Renderer)->InitializeClient(Interactions::last_clientID+1)) {
	    return ++Interactions::last_clientID;
	}
	return 0;
}

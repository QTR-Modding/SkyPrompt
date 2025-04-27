#include "Service.h"
#include "Renderer.h"

bool ProcessSendPrompt(SkyPromptAPI::PromptSink* a_sink, const bool a_force, const SkyPromptAPI::ClientID a_clientID) {

	if (!a_sink) {
		return false;
	}
	if (a_clientID == 0) {
		return false;
	}

	const auto manager = MANAGER(ImGui::Renderer);
	if (manager->IsInQueue(a_sink)) {
		return false;
	}
	const auto n_prompts = a_sink->GetPrompts().size();

    if (const auto n_current_prompts = manager->GetPromptKeys().size();
		n_current_prompts + n_prompts > MCP::Settings::n_max_buttons) {
		if (a_force) manager->Clear();
		else return false;
	}

	return manager->Add2Q(a_sink, a_clientID);
}

DLLEXPORT bool ProcessSendHint(SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID)
{
	if (!a_sink) {
		return false;
	}
	if (a_clientID == 0) {
		return false;
	}

	const auto manager = MANAGER(ImGui::Renderer);
	if (manager->IsInQueue(a_sink)) {
		return false;
	}
	manager->Clear();

	return manager->Add2Q(a_sink, a_clientID, true);
}

void ProcessRemovePrompt(SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {

	if (!a_sink) {
		return;
	}

    if (a_clientID == 0) {
		return;
	}
	const auto manager = MANAGER(ImGui::Renderer);
	if (!manager->IsInQueue(a_sink)) {
		return;
	}

	manager->RemoveFromQ(a_sink);
}

SkyPromptAPI::ClientID ProcessRequestClientID()
{
	std::lock_guard lock(Interactions::mutex_);
	if (Interactions::last_clientID == std::numeric_limits<SkyPromptAPI::ClientID>::max()) {
		return 0;
	}
	return ++Interactions::last_clientID;
}

#include "Service.h"
#include "Renderer.h"

bool ProcessSendPrompt(SkyPromptAPI::PromptSink* a_sink, const bool a_force) {

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

	return manager->Add2Q(a_sink);
}

void ProcessRemovePrompt(SkyPromptAPI::PromptSink* a_sink) {
	const auto manager = MANAGER(ImGui::Renderer);
	if (!manager->IsInQueue(a_sink)) {
		return;
	}
	manager->RemoveFromQ(a_sink);
}

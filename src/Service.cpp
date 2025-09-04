#include "Service.h"
#include "Renderer.h"
#include "Theme.h"

bool ProcessSendPrompt(const SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {

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

void ProcessRemovePrompt(const SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {

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

SkyPromptAPI::ClientID ProcessRequestClientID(int a_major, int a_minor)
{

	constexpr int major = SkyPromptAPI::MAJOR;
	constexpr int minor = SkyPromptAPI::MINOR;

	if (a_major != major) {
	    logger::error("Mismatch in MAJOR version of SkyPrompt ({}.{}) and Client ({}.{})!",
			major,minor,a_major,a_minor);
		return 0;
	}

	if (a_minor != minor) {
	    logger::error("Mismatch in MINOR version of SkyPrompt ({}.{}) and Client ({}.{})!",
			major,minor,a_major,a_minor);
		return 0;
	}

	std::lock_guard lock(Service::mutex_);
	if (Service::last_clientID == std::numeric_limits<SkyPromptAPI::ClientID>::max()) {
		return 0;
	}
	if (MANAGER(ImGui::Renderer)->InitializeClient(Service::last_clientID+1)) {
	    return ++Service::last_clientID;
	}
	return 0;
}

bool ProcessRequestTheme(SkyPromptAPI::ClientID a_clientID, std::string_view theme_name)
{
	if (a_clientID == 0) {
		logger::error("Invalid ClientID: {}", a_clientID);
		return false;
	}
	if (theme_name.empty()) {
		logger::error("Theme name cannot be empty");
		return false;
	}
	if (auto theme_str = std::string(theme_name); Theme::themes_loaded.contains(theme_str)){
	    std::unique_lock lock(Theme::m_theme_);
		auto& a_theme = Theme::themes_loaded.at(theme_str);
	    a_theme.ReLoad(theme_str);
		Theme::themes[a_clientID] = &a_theme;
		return true;
	} else {
		logger::error("Theme not found: {}", theme_str);
		return false;
	}
}

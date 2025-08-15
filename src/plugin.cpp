#include "Hooks.h"
#include "IconsFonts.h"
#include "logger.h"
#include "MCP.h"
#include "Settings.h"
#include "Styles.h"
#include "Theme.h"
#include "Utils.h"
#include "PapyrusAPI/Bindings.h"
#include "Tutorial.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
	    SpeedProfiler profiler("Plugin load (Part 2)");
        MCP::Register();
        if (!SKSE::GetPapyrusInterface()->Register(PapyrusAPI::Register)) {
            logger::error("Failed to register Papyrus API");
        }
        if (OtherSettings::first_install) {
            Tutorial::Manager::Start();
        }
    }
	if (message->type == SKSE::MessagingInterface::kInputLoaded) {
		if (MCP::Settings::prompt_keys.empty()) {
			MCP::Settings::LoadDefaultPromptKeys();
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
	SpeedProfiler profiler("Plugin load (Part 1)");
    SetupLog();
    SKSE::Init(skse);

    logger::info("Plugin loaded");

    if (!MANAGER(IconFont)->IsImGuiIconsInstalled()) {
		logger::error("Failed to load ImGui icons.");
		return false;
    }
	logger::info("ImGui icons loaded.");

    Theme::LoadThemes();

    if (const auto messaging = SKSE::GetMessagingInterface()) {
        messaging->RegisterListener(OnMessage);
    }

    Settings::GetSingleton()->LoadSettings();
	ImGui::Styles::GetSingleton()->RefreshStyle();
	ImGui::Renderer::Install();

    return true;
}
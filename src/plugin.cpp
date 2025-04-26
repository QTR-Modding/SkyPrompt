#include "Hooks.h"
#include "IconsFonts.h"
#include "logger.h"
#include "MCP.h"
#include "Settings.h"
#include "Styles.h"
#include "Utils.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
	    SpeedProfiler profiler("Plugin load (Part 2)");
        MCP::Register();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
    }
    if (message->type == SKSE::MessagingInterface::kPostLoad) {

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

    if (const auto messaging = SKSE::GetMessagingInterface()) {
        messaging->RegisterListener(OnMessage);
    }

    Settings::GetSingleton()->LoadSettings();
	ImGui::Styles::GetSingleton()->RefreshStyle();
	ImGui::Renderer::Install();

    return true;
}
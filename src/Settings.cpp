#include "Settings.h"
#include "Hooks.h"
#include "Renderer.h"

void Settings::SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)>& a_func, const bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); !a_generate && rc < SI_OK) {
		return;
	}

	a_func(ini);

	(void)ini.SaveFile(a_path);
}

void Settings::SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, const std::function<void(CSimpleIniA&)>& a_func)
{
	SerializeINI(a_defaultPath, a_func);
	SerializeINI(a_userPath, a_func);
}

void Settings::LoadSettings() const
{
	SerializeINI(defaultDisplayTweaksPath, userDisplayTweaksPath, [](auto& ini) {
		ImGui::Renderer::LoadSettings(ini);  // display tweaks scaling
	});
	try {
	    MCP::Settings::from_json();
        ImGui::Renderer::UpdateMaxIntervalBetweenPresses();
	}
	catch (const std::exception& e) {
		logger::error("Failed to load settings: {}", e.what());
	}
}

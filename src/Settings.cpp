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

std::array<std::pair<float, float>, Presets::OSP::NOSPs> Presets::OSP::getOSPs()
{
	std::array<std::pair<float, float>, NOSPs> result;
	size_t index = 0;
	for (auto a_float:Presets::OSP::OSPY) {
	    for (auto b_float:Presets::OSP::OSPX) {
			result[index]= {b_float, a_float};
			++index;
		}
	}
	return result;
}

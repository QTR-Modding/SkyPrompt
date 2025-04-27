#pragma once
#include <shared_mutex>
#include "Input.h"
#include "Interaction.h"
#include "MCP.h"


class Settings : public clib_util::singleton::ISingleton<Settings>
{
public:
	void LoadSettings() const;

private:
	static void SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)>& a_func, bool a_generate = false);
	static void SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, const std::function<void(CSimpleIniA&)>& a_func);

	const wchar_t* defaultDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks.ini" };
	const wchar_t* userDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks_Custom.ini" };
};

namespace ButtonSettings {

	inline std::shared_mutex button_key_lock;

	using namespace MCP::Settings;
    inline std::map<Interaction, std::map<Input::DEVICE,uint32_t>> interactionKeys;

	void SetInteractionKey(const Interaction& a_interaction, Input::DEVICE a_device, uint32_t converted_key);
	uint32_t GetInteractionKey(const Interaction& a_interaction, Input::DEVICE a_device);
}

namespace OtherSettings {
    [[maybe_unused]] inline float search_scaling = 0.5f;
}
#pragma once
#include "CLibUtilsQTR/PresetSettings.hpp"


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

namespace OtherSettings {
    [[maybe_unused]] inline float search_scaling = 0.5f;
}

namespace Presets {
	namespace OSP {
        constexpr std::array<std::string_view, 25> OSPnames = { // on-screen position names
		    "Bottom",
		    "BottomRight",
		    "BottomRightSlight",
		    "BottomLeft",
		    "BottomLeftSlight",
		    "Top",
		    "TopRight",
		    "TopRightSlight",
            "TopLeft",
		    "TopLeftSlight",
		    "Center",
		    "CenterRight",
		    "CenterRightSlight",
		    "CenterLeft",
		    "CenterLeftSlight",
			"CenterTop",
			"CenterTopRight",
			"CenterTopRightSlight",
			"CenterTopLeft",
			"CenterTopLeftSlight",
			"CenterBottom",
			"CenterBottomRight",
			"CenterBottomRightSlight",
			"CenterBottomLeft",
			"CenterBottomLeftSlight",
        };

	    using namespace clib_utilsQTR;
	    constexpr size_t NOSPs = OSPnames.size(); // number of on-screen positions
	    constexpr PresetPool OSPPool{OSPnames};

		// for promptsize 54.6
        constexpr std::array OSPX = { // X offsets for on-screen positions
			0.584f, // center
			0.913f, // right
			0.751f, // right slight
			0.250f, // left
			0.415f, // left slight

		};
		constexpr std::array OSPY = { // Y offsets for on-screen positions
			0.981f, // bottom
			0.210f, // top
			0.650f, // center
			0.460f, // center top
			0.815f, // center bottom
		};

		std::array<std::pair<float, float>, NOSPs> getOSPs();

        inline auto presets = PresetSetting<std::pair<float,float>,NOSPs,OSPPool>(getOSPs());
	}
}
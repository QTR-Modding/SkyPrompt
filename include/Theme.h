#pragma once
#include <shared_mutex>

#include "Settings.h"
#include "SkyPrompt/API.hpp"
#include "boost/pfr/core.hpp"
#include "CLibUtilsQTR/PresetHelpers/Config.hpp"
#include "rapidjson/document.h"

namespace Theme {

	using namespace Presets;

	struct ThemeBlock {
        Field<std::string,rapidjson::Value> theme_name = { "name",""};
		Field<std::string, rapidjson::Value> theme_description = { "description","" };
		Field<std::string, rapidjson::Value> theme_author = { "author","" };
		Field<std::string, rapidjson::Value> theme_version = { "version","" };

		Field<float, rapidjson::Value> marginX = { "marginX", 20.0f };
		Field<float, rapidjson::Value> marginY = { "marginY", 20.0f };
		Field<float, rapidjson::Value> xPercent = { "xPercent", Presets::OSP::OSPX[1] };
		Field<float, rapidjson::Value> yPercent = { "yPercent", Presets::OSP::OSPY[4] };
		Field<float, rapidjson::Value> prompt_size = { "prompt_size", 45.65f };
		Field<float, rapidjson::Value> icon2font_ratio = { "icon2font_ratio", 1.f };
		Field<float, rapidjson::Value> linespacing = { "linespacing", 0.267f };
		Field<float, rapidjson::Value> progress_speed = { "progress_speed", .552f };

		Field<std::string, rapidjson::Value> font_name = { "font_name", "Jost-Regular" };
		Field<float, rapidjson::Value> font_shadow = { "font_shadow", 0.2f };
		Field<std::string, rapidjson::Value> prompt_alignment = { "prompt_alignment", "vertical" }; // e.g. radial

        void load(rapidjson::Value& a_block) {
            boost::pfr::for_each_field(*this, [&](auto& field) {
                field.load(a_block);
            });
        }
    };

	enum PromptAlignment : uint8_t {
		kRadial,
		kHorizontal,
		kVertical
	};

    PromptAlignment toPromptAlignment(const std::string& alignment);

    struct Theme {

	    std::string theme_name = "Default Theme";
		std::string theme_description = "Default theme for SkyPrompt";
		std::string theme_author = "Quantumyilmaz";
		std::string theme_version = "1.0.0";

		float marginX = 20.0f;
		float marginY = 20.0f;
		float xPercent = Presets::OSP::OSPX[1];
		float yPercent = Presets::OSP::OSPY[4];
		float prompt_size = 45.65f;
		float icon2font_ratio = 1.f;
		float linespacing = 0.267f;
		float progress_speed = .552f;

		std::string font_name = "Jost-Regular";
		float font_shadow = 0.2f;

		PromptAlignment prompt_alignment = kVertical;
		int special_effects = 0; // TODO: e.g. Viny's yellow arcs

		Theme() = default;
		Theme(const ThemeBlock& block);
	};

	inline auto default_theme = Theme();

    void LoadThemes();

	inline std::unordered_map<std::string, Theme> themes_loaded;

	inline std::shared_mutex m_theme_;
	inline std::unordered_map<SkyPromptAPI::ClientID, Theme*> themes;
	inline auto last_theme = Theme();
};
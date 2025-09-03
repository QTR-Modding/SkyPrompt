#pragma once
#include <shared_mutex>
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

		Field<float, rapidjson::Value> marginX = { "marginX", 0.0f };
		Field<float, rapidjson::Value> marginY = { "marginY", 0.0f };
		Field<float, rapidjson::Value> xPercent = { "xPercent", 0.85f };
		Field<float, rapidjson::Value> yPercent = { "yPercent", 0.85f };
		Field<float, rapidjson::Value> prompt_size = { "prompt_size", 45.65f };
		Field<float, rapidjson::Value> icon2font_ratio = { "icon2font_ratio", 1.f };
		Field<float, rapidjson::Value> linespacing = { "linespacing", 0.267f };
		Field<float, rapidjson::Value> progress_speed = { "progress_speed", .552f };
		Field<float, rapidjson::Value> fadeSpeed = { "fadeSpeed", .02f };

		Field<std::string, rapidjson::Value> font_name = { "font_name", "Jost-Regular" };
		Field<float, rapidjson::Value> font_shadow = { "font_shadow", 0.2f };
		Field<std::string, rapidjson::Value> prompt_alignment = { "prompt_alignment", "vertical" }; // e.g. radial

		Field<uint32_t, rapidjson::Value> special_effect = { "special_effect", 0 };
		Field<std::vector<uint32_t>, rapidjson::Value> special_integers = { "special_integers", {} };
		Field<std::vector<std::string>, rapidjson::Value> special_strings = { "special_strings", {} };
		Field<std::vector<float>, rapidjson::Value> special_floats = { "special_floats", {} };
		Field<std::vector<bool>, rapidjson::Value> special_bools = { "special_bools", {} };

		Field<bool, rapidjson::Value> hide_in_menu = { "hide_in_menu", false};

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

		float marginX = 0.f;
		float marginY = 0.f;
		float xPercent = 0.85f;
		float yPercent = 0.85f;
		float prompt_size = 45.65f;
		float icon2font_ratio = 1.f;
		float linespacing = 0.267f;
		float progress_speed = .552f;
		float fadeSpeed = .02f;

		std::string font_name = "Jost-Regular";
		float font_shadow = 0.2f;

		PromptAlignment prompt_alignment = kVertical;
		uint32_t special_effect = 0;

		std::vector<uint32_t> special_integers;
		std::vector<std::string> special_strings;
		std::vector<float> special_floats;
		std::vector<uint8_t> special_bools;

		bool hide_in_menu = false;

		Theme() = default;
        explicit Theme(const ThemeBlock& block);

		void ReLoad();
	};

	inline auto default_theme = Theme();

    void LoadThemes();

	inline std::unordered_map<std::string, Theme> themes_loaded;

	inline std::shared_mutex m_theme_;
	inline std::unordered_map<SkyPromptAPI::ClientID, Theme*> themes;
	inline Theme* last_theme = &default_theme;
};
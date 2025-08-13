#pragma once
#include <shared_mutex>
#include "Settings.h"
#include "SkyPrompt/API.hpp"

namespace Theme {

	enum PromptAlignment : uint8_t {
		kRadial,
		kHorizontal,
		kVertical
	};

    inline PromptAlignment toPromptAlignment(const std::string& alignment) {
		if (alignment == "Radial") return kRadial;
		if (alignment == "Horizontal") return kHorizontal;
		if (alignment == "Vertical") return kVertical;
		return kVertical; // default
	}

	struct Theme {

		uint32_t theme_id = 0; // Default theme ID

	    std::string theme_name = "Default Theme";
		std::string theme_description = "Default theme for SkyPrompt";
		std::string theme_author = "Quantumyilmaz";
		std::string theme_version = "1.0.0";

		size_t osp = 21; // CenterBottomRight
		float marginX = 20.0f;
		float marginY = 20.0f;
		float xPercent = Presets::OSP::OSPX[1];;
		float yPercent = Presets::OSP::OSPY[4];
		float prompt_size = 45.65f;
		float icon2font_ratio = 1.f;
		float linespacing = 0.267f;
		float progress_speed = .552f;

		std::string font_name = "Jost-Regular";
		float font_shadow = 0.2f;

		PromptAlignment prompt_alignment = kVertical; // e.g. radial	
		int special_effects; // TODO: e.g. Viny's yellow arcs
	};

	inline auto default_theme = Theme();

    inline void LoadThemes(){};

	inline std::shared_mutex m_theme_;
	inline std::unordered_map<std::string, Theme> themes_loaded;
	inline std::unordered_map<SkyPromptAPI::ClientID, Theme> themes;
};
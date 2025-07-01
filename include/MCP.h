#pragma once
#include "Input.h"
#include "Settings.h"
#include "rapidjson/document.h"

namespace MCP {

	inline std::atomic_bool refreshStyle{ false };
	inline bool is_installed = false;

    inline std::string log_path;
    inline std::vector<std::string> logLines;

    void __stdcall RenderSettings();
	void __stdcall RenderControls();
	void __stdcall RenderTheme();
    void __stdcall RenderLog();
    void Register();

    namespace Settings {
        inline size_t current_OSP = 21; // CenterBottomRight
        constexpr float marginX = 20.0f;
        constexpr float marginY = 20.0f;
        inline float fadeSpeed{ 0.02f };
	    inline float xPercent = Presets::OSP::OSPX[1];
        inline float yPercent = Presets::OSP::OSPY[4];
        inline float prompt_size = 45.65f;
		inline float icon2font_ratio = 1.f;
		inline float linespacing = 0.267f;
	    inline float progress_speed = .552f;
	    inline float lifetime = 5.f;
		inline bool draw_debug = false;

        inline int n_max_buttons = 4;
		inline std::map<Input::DEVICE,std::vector<uint32_t>> prompt_keys;
        inline std::map<Input::DEVICE, uint32_t> cycle_L;
        inline std::map<Input::DEVICE, uint32_t> cycle_R;

        inline std::atomic shouldReloadPromptSize=true;
        inline std::atomic shouldReloadLifetime=true;
	    inline std::atomic initialized{ true };
		inline std::atomic cycle_controls = true; // multipage prompt support


		inline std::map<Input::DEVICE, bool> enabled_devices = {
			{Input::DEVICE::kKeyboardMouse, true },
			{Input::DEVICE::kGamepadDirectX, true },
			{Input::DEVICE::kGamepadOrbis, true }	
		};

		bool IsEnabled(Input::DEVICE a_device);

		namespace SpecialCommands {
			inline bool visualize = true;
			inline float responsiveness = 0.67f;
			void Render();
		};

		// Settings::Theme
	    inline std::string font_name = "Jost-Regular";
	    inline std::set<std::string> font_names;
	    inline float font_shadow = 0.2f;

		void OSPPresetBox();
		bool FontSettings();
		void LoadDefaultPromptKeys();
		bool CycleControls();

        void to_json();
        void from_json();

    };

	inline auto current_device = Input::DEVICE::kKeyboardMouse;
};


namespace LogSettings {
#ifndef NDEBUG
    inline bool log_trace = true;
#else
	inline bool log_trace = false;
#endif
    inline bool log_info = true;
    inline bool log_warning = true;
    inline bool log_error = true;
};
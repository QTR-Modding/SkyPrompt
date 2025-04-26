#pragma once
#include "Input.h"
#include "rapidjson/document.h"

namespace MCP {

    inline std::string log_path;
    inline std::vector<std::string> logLines;

    void __stdcall RenderSettings();
	void __stdcall RenderControls();
    void __stdcall RenderLog();
    void Register();

    namespace Settings {
        constexpr float marginX = 20.0f;
        constexpr float marginY = 20.0f;
        inline float fadeSpeed{ 0.007f };
	    inline float xPercent = 0.8f;
        inline float yPercent = 0.97f;
        inline float prompt_size = 40;
		inline float icon2font_ratio = 1.f;
	    inline float progress_speed = 1.f;
	    inline float lifetime = 20.f;
#ifndef NDEBUG
		inline bool draw_debug = true;
#endif

        inline int n_max_buttons = 4;
		inline std::map<Input::DEVICE,std::vector<uint32_t>> prompt_keys = {
			{ Input::DEVICE::kKeyboard, {KEY::kNum1,KEY::kNum2,KEY::kNum3,KEY::kNum4} },
			{ Input::DEVICE::kMouse, {MOUSE::kMiddleButton,MOUSE::kButton3,MOUSE::kButton4,MOUSE::kButton5} },
			{ Input::DEVICE::kGamepadDirectX, {GAMEPAD_DIRECTX::kA,GAMEPAD_DIRECTX::kB,GAMEPAD_DIRECTX::kX,GAMEPAD_DIRECTX::kY} },
			{ Input::DEVICE::kGamepadOrbis, {GAMEPAD_ORBIS::kPS3_A, GAMEPAD_ORBIS::kPS3_B, GAMEPAD_ORBIS::kPS3_X, GAMEPAD_ORBIS::kPS3_Y} },
        };

		inline std::atomic multi_threaded = true;

        inline std::atomic shouldReloadPromptSize=true;
        inline std::atomic shouldReloadLifetime=true;
	    inline std::atomic initialized{ false };


		inline std::map<Input::DEVICE, bool> enabled_devices = {
			{Input::DEVICE::kKeyboard, true },
			{Input::DEVICE::kMouse, false },
			{Input::DEVICE::kGamepadDirectX, true },
			{Input::DEVICE::kGamepadOrbis, true }	
		};

		bool IsEnabled(Input::DEVICE a_device);

		namespace SpecialCommands {
			inline bool visualize = true;
			inline float responsiveness = 0.9f;
			void Render();
		};

        void to_json();
        void from_json();

    };

	inline auto current_device = Input::DEVICE::kKeyboard;
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
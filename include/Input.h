#pragma once

namespace Input
{
	enum class DEVICE
	{
		kUnknown = 0,
		kKeyboardMouse,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	std::string device_to_string(DEVICE a_device);
	DEVICE from_string_to_device(const std::string& a_device);

	class Manager final :
		public clib_util::singleton::ISingleton<Manager>
	{
	public:

		[[nodiscard]] DEVICE GetInputDevice() const;
		void UpdateInputDevice(RE::InputEvent* event);
		[[nodiscard]] static uint32_t Convert(uint32_t button_key, RE::INPUT_DEVICE a_device);
        static std::vector<uint32_t> GetKeys(DEVICE a_device);

	private:

		// members
		bool screenshotQueued{ false };
		bool menuHidden{ false };

		float keyHeldDuration{ 0.5 };

		std::uint32_t screenshotKeyboard{ 0 };
		std::uint32_t screenshotMouse{ 0 };
		std::uint32_t screenshotGamepad{ 0 };

		DEVICE inputDevice{ DEVICE::kKeyboardMouse };
	};
}

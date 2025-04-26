#pragma once

namespace Input
{
	enum class DEVICE
	{
		kUnknown = 0,
		kKeyboard,
		kMouse,
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
		[[nodiscard]] uint32_t Convert(uint32_t button_key) const;
		[[nodiscard]] static uint32_t Convert(uint32_t button_key, DEVICE a_device);
        static std::vector<uint32_t> GetKeys(DEVICE a_device);

	private:
		void SendKeyEvent(std::uint32_t a_key, float a_value, bool a_keyPressed) const;

		// members
		bool screenshotQueued{ false };
		bool menuHidden{ false };

		float keyHeldDuration{ 0.5 };

		std::uint32_t screenshotKeyboard{ 0 };
		std::uint32_t screenshotMouse{ 0 };
		std::uint32_t screenshotGamepad{ 0 };

		DEVICE inputDevice{ DEVICE::kKeyboard };
	};
}

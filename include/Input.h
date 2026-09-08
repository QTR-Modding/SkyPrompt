#pragma once
#include "REX/REX/Singleton.h"

namespace Input {
    enum DEVICE {
        kUnknown = 0,
        kKeyboardMouse,
        kGamepad,
        kVR,
        kTotal
    };

    std::string device_to_string(DEVICE a_device);
    DEVICE from_RE_device(RE::INPUT_DEVICE a_device);

    struct VRNavigation {
        enum class Direction { kNone, kUp, kDown, kLeft, kRight };

        uint32_t modifier = 0;
        bool blocksInput = false;
        Direction direction = Direction::kNone;
        std::chrono::steady_clock::time_point nextRepeat;

        Direction Step(float x, float y);
    };

    class Manager final :
        public REX::Singleton<Manager> {
    public:
        [[nodiscard]] DEVICE GetInputDevice() const;
        void UpdateInputDevice(RE::InputEvent* event);
        [[nodiscard]] static uint32_t Convert(uint32_t button_key, RE::INPUT_DEVICE a_device);
        static std::vector<uint32_t> GetKeys(DEVICE a_device);

        VRNavigation vrNavigation;

    private:
        // members
        bool screenshotQueued{false};
        bool menuHidden{false};

        float keyHeldDuration{0.5};

        std::uint32_t screenshotKeyboard{0};
        std::uint32_t screenshotMouse{0};
        std::uint32_t screenshotGamepad{0};

        DEVICE inputDevice{REL::Module::IsVR() ? kVR : kKeyboardMouse};
    };
}

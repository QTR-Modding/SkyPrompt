#include "Input.h"
#include "Renderer.h"
#include "Service.h"
#include "imgui_internal.h"
#include <imgui.h>

namespace {
    ImGuiKey ToImGuiKey(const KEY a_key) {
        switch (a_key) {
            case KEY::kTab:
                return ImGuiKey_Tab;
            case KEY::kLeft:
                return ImGuiKey_LeftArrow;
            case KEY::kRight:
                return ImGuiKey_RightArrow;
            case KEY::kUp:
                return ImGuiKey_UpArrow;
            case KEY::kDown:
                return ImGuiKey_DownArrow;
            case KEY::kPageUp:
                return ImGuiKey_PageUp;
            case KEY::kPageDown:
                return ImGuiKey_PageDown;
            case KEY::kHome:
                return ImGuiKey_Home;
            case KEY::kEnd:
                return ImGuiKey_End;
            case KEY::kInsert:
                return ImGuiKey_Insert;
            case KEY::kDelete:
                return ImGuiKey_Delete;
            case KEY::kBackspace:
                return ImGuiKey_Backspace;
            case KEY::kSpacebar:
                return ImGuiKey_Space;
            case KEY::kEnter:
                return ImGuiKey_Enter;
            case KEY::kEscape:
                return ImGuiKey_Escape;
            case KEY::kApostrophe:
                return ImGuiKey_Apostrophe;
            case KEY::kComma:
                return ImGuiKey_Comma;
            case KEY::kMinus:
                return ImGuiKey_Minus;
            case KEY::kPeriod:
                return ImGuiKey_Period;
            case KEY::kSlash:
                return ImGuiKey_Slash;
            case KEY::kSemicolon:
                return ImGuiKey_Semicolon;
            case KEY::kEquals:
                return ImGuiKey_Equal;
            case KEY::kBracketLeft:
                return ImGuiKey_LeftBracket;
            case KEY::kBackslash:
                return ImGuiKey_Backslash;
            case KEY::kBracketRight:
                return ImGuiKey_RightBracket;
            case KEY::kTilde:
                return ImGuiKey_GraveAccent;
            case KEY::kCapsLock:
                return ImGuiKey_CapsLock;
            case KEY::kScrollLock:
                return ImGuiKey_ScrollLock;
            case KEY::kNumLock:
                return ImGuiKey_NumLock;
            case KEY::kPrintScreen:
                return ImGuiKey_PrintScreen;
            case KEY::kPause:
                return ImGuiKey_Pause;
            case KEY::kKP_0:
                return ImGuiKey_Keypad0;
            case KEY::kKP_1:
                return ImGuiKey_Keypad1;
            case KEY::kKP_2:
                return ImGuiKey_Keypad2;
            case KEY::kKP_3:
                return ImGuiKey_Keypad3;
            case KEY::kKP_4:
                return ImGuiKey_Keypad4;
            case KEY::kKP_5:
                return ImGuiKey_Keypad5;
            case KEY::kKP_6:
                return ImGuiKey_Keypad6;
            case KEY::kKP_7:
                return ImGuiKey_Keypad7;
            case KEY::kKP_8:
                return ImGuiKey_Keypad8;
            case KEY::kKP_9:
                return ImGuiKey_Keypad9;
            case KEY::kKP_Decimal:
                return ImGuiKey_KeypadDecimal;
            case KEY::kKP_Divide:
                return ImGuiKey_KeypadDivide;
            case KEY::kKP_Multiply:
                return ImGuiKey_KeypadMultiply;
            case KEY::kKP_Subtract:
                return ImGuiKey_KeypadSubtract;
            case KEY::kKP_Plus:
                return ImGuiKey_KeypadAdd;
            case KEY::kKP_Enter:
                return ImGuiKey_KeypadEnter;
            case KEY::kLeftShift:
                return ImGuiKey_LeftShift;
            case KEY::kLeftControl:
                return ImGuiKey_LeftCtrl;
            case KEY::kLeftAlt:
                return ImGuiKey_LeftAlt;
            case KEY::kLeftWin:
                return ImGuiKey_LeftSuper;
            case KEY::kRightShift:
                return ImGuiKey_RightShift;
            case KEY::kRightControl:
                return ImGuiKey_RightCtrl;
            case KEY::kRightAlt:
                return ImGuiKey_RightAlt;
            case KEY::kRightWin:
                return ImGuiKey_RightSuper;
            /*case KEY::kAPPS:
                return ImGuiKey_Menu; - doesn't fire*/
            case KEY::kNum0:
                return ImGuiKey_0;
            case KEY::kNum1:
                return ImGuiKey_1;
            case KEY::kNum2:
                return ImGuiKey_2;
            case KEY::kNum3:
                return ImGuiKey_3;
            case KEY::kNum4:
                return ImGuiKey_4;
            case KEY::kNum5:
                return ImGuiKey_5;
            case KEY::kNum6:
                return ImGuiKey_6;
            case KEY::kNum7:
                return ImGuiKey_7;
            case KEY::kNum8:
                return ImGuiKey_8;
            case KEY::kNum9:
                return ImGuiKey_9;
            case KEY::kA:
                return ImGuiKey_A;
            case KEY::kB:
                return ImGuiKey_B;
            case KEY::kC:
                return ImGuiKey_C;
            case KEY::kD:
                return ImGuiKey_D;
            case KEY::kE:
                return ImGuiKey_E;
            case KEY::kF:
                return ImGuiKey_F;
            case KEY::kG:
                return ImGuiKey_G;
            case KEY::kH:
                return ImGuiKey_H;
            case KEY::kI:
                return ImGuiKey_I;
            case KEY::kJ:
                return ImGuiKey_J;
            case KEY::kK:
                return ImGuiKey_K;
            case KEY::kL:
                return ImGuiKey_L;
            case KEY::kM:
                return ImGuiKey_M;
            case KEY::kN:
                return ImGuiKey_N;
            case KEY::kO:
                return ImGuiKey_O;
            case KEY::kP:
                return ImGuiKey_P;
            case KEY::kQ:
                return ImGuiKey_Q;
            case KEY::kR:
                return ImGuiKey_R;
            case KEY::kS:
                return ImGuiKey_S;
            case KEY::kT:
                return ImGuiKey_T;
            case KEY::kU:
                return ImGuiKey_U;
            case KEY::kV:
                return ImGuiKey_V;
            case KEY::kW:
                return ImGuiKey_W;
            case KEY::kX:
                return ImGuiKey_X;
            case KEY::kY:
                return ImGuiKey_Y;
            case KEY::kZ:
                return ImGuiKey_Z;
            case KEY::kF1:
                return ImGuiKey_F1;
            case KEY::kF2:
                return ImGuiKey_F2;
            case KEY::kF3:
                return ImGuiKey_F3;
            case KEY::kF4:
                return ImGuiKey_F4;
            case KEY::kF5:
                return ImGuiKey_F5;
            case KEY::kF6:
                return ImGuiKey_F6;
            case KEY::kF7:
                return ImGuiKey_F7;
            case KEY::kF8:
                return ImGuiKey_F8;
            case KEY::kF9:
                return ImGuiKey_F9;
            case KEY::kF10:
                return ImGuiKey_F10;
            case KEY::kF11:
                return ImGuiKey_F11;
            case KEY::kF12:
                return ImGuiKey_F12;
            default:
                return ImGuiKey_None;
        }
    }

    ImGuiKey ToImGuiKey(const GAMEPAD_DIRECTX a_key) {
        switch (a_key) {
            case GAMEPAD_DIRECTX::kUp:
                return ImGuiKey_GamepadDpadUp;
            case GAMEPAD_DIRECTX::kDown:
                return ImGuiKey_GamepadDpadDown;
            case GAMEPAD_DIRECTX::kLeft:
                return ImGuiKey_GamepadDpadLeft;
            case GAMEPAD_DIRECTX::kRight:
                return ImGuiKey_GamepadDpadRight;
            case GAMEPAD_DIRECTX::kStart:
                return ImGuiKey_GamepadStart;
            case GAMEPAD_DIRECTX::kBack:
                return ImGuiKey_GamepadBack;
            case GAMEPAD_DIRECTX::kLeftThumb:
                return ImGuiKey_GamepadL3;
            case GAMEPAD_DIRECTX::kRightThumb:
                return ImGuiKey_GamepadR3;
            case GAMEPAD_DIRECTX::kLeftShoulder:
                return ImGuiKey_GamepadL1;
            case GAMEPAD_DIRECTX::kRightShoulder:
                return ImGuiKey_GamepadR1;
            case GAMEPAD_DIRECTX::kA:
                return ImGuiKey_GamepadFaceDown;
            case GAMEPAD_DIRECTX::kB:
                return ImGuiKey_GamepadFaceRight;
            case GAMEPAD_DIRECTX::kX:
                return ImGuiKey_GamepadFaceLeft;
            case GAMEPAD_DIRECTX::kY:
                return ImGuiKey_GamepadFaceUp;
            default:
                return ImGuiKey_None;
        }
    }

    // faking this with keyboard inputs, since ImGUI doesn't support DirectInput
    ImGuiKey ToImGuiKey(const GAMEPAD_ORBIS a_key) {
        switch (a_key) {
            // Move / Tweak / Resize Window (in Windowing mode)
            case GAMEPAD_ORBIS::kUp:
                return ImGuiKey_UpArrow;
            // Move / Tweak / Resize Window (in Windowing mode)
            case GAMEPAD_ORBIS::kDown:
                return ImGuiKey_DownArrow;
            // Move / Tweak / Resize Window (in Windowing mode)
            case GAMEPAD_ORBIS::kLeft:
                return ImGuiKey_LeftArrow;
            // Move / Tweak / Resize Window (in Windowing mode)
            case GAMEPAD_ORBIS::kRight:
                return ImGuiKey_RightArrow;

            case GAMEPAD_ORBIS::kPS3_Start:
                return ImGuiKey_GamepadStart;
            case GAMEPAD_ORBIS::kPS3_Back:
                return ImGuiKey_GamepadBack;
            case GAMEPAD_ORBIS::kPS3_L3:
                return ImGuiKey_GamepadL3;
            case GAMEPAD_ORBIS::kPS3_R3:
                return ImGuiKey_GamepadR3;

            // Tweak Slower / Focus Previous (in Windowing mode)
            case GAMEPAD_ORBIS::kPS3_LB:
                return ImGuiKey_NavKeyboardTweakSlow;
            // Tweak Faster / Focus Next (in Windowing mode)
            case GAMEPAD_ORBIS::kPS3_RB:
                return ImGuiKey_NavKeyboardTweakFast;
            // Activate / Open / Toggle / Tweak
            case GAMEPAD_ORBIS::kPS3_A:
                return ImGuiKey_Enter;
            // Cancel / Close / Exit
            case GAMEPAD_ORBIS::kPS3_B:
                return ImGuiKey_Escape;

            case GAMEPAD_ORBIS::kPS3_X:
                return ImGuiKey_GamepadFaceLeft;
            case GAMEPAD_ORBIS::kPS3_Y:
                return ImGuiKey_GamepadFaceUp;
            default:
                return ImGuiKey_None;
        }
    }
};

namespace Input {
    DEVICE Manager::GetInputDevice() const {
        return inputDevice;
    }

    void Manager::UpdateInputDevice(RE::InputEvent* event) {
        if (!event) return;
        if (const auto buttonEvent = event->AsButtonEvent()) {
            if (buttonEvent->IsUp()) {
                return;
            }

            switch (const auto device = event->GetDevice()) {
                case RE::INPUT_DEVICE::kKeyboard:
                    if (MCP::Settings::IsEnabled(kKeyboardMouse)) {
                        inputDevice = kKeyboardMouse;
                    }
                    break;
                case RE::INPUT_DEVICE::kMouse: {
                    if (MCP::Settings::IsEnabled(kKeyboardMouse)) {
                        inputDevice = kKeyboardMouse;
                    }
                }
                break;
                case RE::INPUT_DEVICE::kGamepad: {
                    if (RE::ControlMap::GetSingleton()->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis) {
                        if (MCP::Settings::IsEnabled(kGamepadOrbis)) {
                            inputDevice = kGamepadOrbis;
                        }
                    } else {
                        if (MCP::Settings::IsEnabled(kGamepadDirectX)) {
                            inputDevice = kGamepadDirectX;
                        }
                    }
                }
                break;
                default:
                    break;
            }
        } else if (event->AsThumbstickEvent()) {
            if (RE::ControlMap::GetSingleton()->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis) {
                if (MCP::Settings::IsEnabled(kGamepadOrbis)) {
                    inputDevice = kGamepadOrbis;
                }
            } else {
                if (MCP::Settings::IsEnabled(kGamepadDirectX)) {
                    inputDevice = kGamepadDirectX;
                }
            }
        } else if (event->AsMouseMoveEvent()) {
            if (MCP::Settings::IsEnabled(kKeyboardMouse)) {
                inputDevice = kKeyboardMouse;
            }
        }
    }

    uint32_t Manager::Convert(const uint32_t button_key, const RE::INPUT_DEVICE a_device) {
        using namespace SKSE::InputMap;
        if (button_key == SkyPromptAPI::kMouseMove ||
            button_key == SkyPromptAPI::kThumbstickMoveL ||
            button_key == SkyPromptAPI::kThumbstickMoveR ||
            button_key == SkyPromptAPI::kSkyrim
        ) {
            return button_key;
        }
        if (a_device == RE::INPUT_DEVICE::kKeyboard) {
            return button_key;
        }
        if (a_device == RE::INPUT_DEVICE::kMouse) {
            if (kMacro_MouseButtonOffset <= button_key && button_key < kMacro_GamepadOffset) {
                return button_key;
            }
            return button_key + kMacro_MouseButtonOffset;
        }
        if (a_device == RE::INPUT_DEVICE::kGamepad) {
            if (kMacro_GamepadOffset <= button_key && button_key < kMaxMacros) {
                return button_key;
            }
            return GamepadMaskToKeycode(button_key);
        }
        return 0;
    }

    uint32_t Manager::GetActivateKey() const {
        const auto controlMap = RE::ControlMap::GetSingleton();
        const auto& activate = RE::UserEvents::GetSingleton()->activate;
        auto device = inputDevice == kKeyboardMouse ? RE::INPUT_DEVICE::kKeyboard : RE::INPUT_DEVICE::kGamepad;
        auto key = controlMap->GetMappedKey(activate, device);
        if (key == RE::ControlMap::kInvalid && device == RE::INPUT_DEVICE::kKeyboard) {
            device = RE::INPUT_DEVICE::kMouse;
            key = controlMap->GetMappedKey(activate, device);
        }
        return key == RE::ControlMap::kInvalid ? 0 : Convert(key, device);
    }

    std::optional<bool> Manager::ProcessListInput(RE::InputEvent* event) {
        if (Theme::last_theme->prompt_alignment != Theme::kList) {
            listStickDirection = 0;
            return std::nullopt;
        }
        const auto renderer = MANAGER(ImGui::Renderer);
        const auto selected = renderer->GetSelectedListPrompt();
        if (!selected || selected->IsHidden()) {
            listStickDirection = 0;
            return std::nullopt;
        }
        const bool block = PromptTypeFlags::GetBlocksInput(selected->GetPromptType());
        constexpr float repeatDelay = 0.35f;
        constexpr float repeatRate = 0.12f;
        if (const auto button = event->AsButtonEvent()) {
            using namespace SKSE::InputMap;
            const auto key = Convert(button->GetIDCode(), button->GetDevice());
            if (key == GetActivateKey()) return std::nullopt;
            const bool up = key == Convert(MOUSE::kWheelUp, RE::INPUT_DEVICE::kMouse) ||
                            key == kGamepadButtonOffset_DPAD_UP;
            const bool down = key == Convert(MOUSE::kWheelDown, RE::INPUT_DEVICE::kMouse) ||
                              key == kGamepadButtonOffset_DPAD_DOWN;
            if (up || down) {
                const float held = button->HeldDuration();
                if (button->IsDown() || (button->IsPressed() &&
                    ImGui::CalcTypematicRepeatAmount(held - ImGui::GetIO().DeltaTime, held, repeatDelay, repeatRate) > 0)) {
                    renderer->MoveListSelection(up);
                }
                return block;
            }
        } else if (const auto stick = event->AsThumbstickEvent(); stick && stick->IsRight()) {
            constexpr float deadzone = 0.5f;
            const int direction = stick->yValue > deadzone ? -1 : stick->yValue < -deadzone ? 1 : 0;
            const auto now = std::chrono::steady_clock::now();
            if (direction != 0 && (direction != listStickDirection || now >= nextListRepeat)) {
                renderer->MoveListSelection(direction < 0);
                nextListRepeat = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<float>(direction != listStickDirection ? repeatDelay : repeatRate));
            }
            const bool navigating = direction != 0 || listStickDirection != 0;
            listStickDirection = direction;
            return block && navigating;
        }
        return std::nullopt;
    }

    std::vector<uint32_t> Manager::GetKeys(const DEVICE a_device) {
        using namespace SKSE::InputMap;

        std::vector<uint32_t> keys;
        switch (a_device) {
            case kKeyboardMouse:
                for (uint32_t key = 1; key <= static_cast<uint32_t>(KEY::kRightWin); ++key) {
                    const auto keyboard_key = static_cast<KEY>(key);
                    if (keyboard_key == KEY::kLeftWin || keyboard_key == KEY::kRightWin ||
                        ToImGuiKey(keyboard_key) == ImGuiKey_None) {
                        continue;
                    }
                    keys.push_back(key);
                }
                for (uint32_t key = kMacro_MouseButtonOffset; key < kMacro_GamepadOffset; ++key) {
                    keys.push_back(key);
                }
                break;
            case kGamepadDirectX:
            case kGamepadOrbis:
                for (uint32_t key = kMacro_GamepadOffset; key < kMaxMacros; ++key) {
                    keys.push_back(key);
                }
                break;
            default:
                break;
        }
        return keys;
    }

    std::string device_to_string(const DEVICE a_device) {
        switch (a_device) {
            case kKeyboardMouse:
                return "Keyboard & Mouse";
            case kGamepadDirectX:
                return "Gamepad (Xbox)";
            case kGamepadOrbis:
                return "Gamepad (PS4)";
            default:
                return "Unknown";
        }
    }

    DEVICE from_string_to_device(const std::string& a_device) {
        if (a_device == "Keyboard & Mouse") {
            return kKeyboardMouse;
        }
        if (a_device == "Gamepad (Xbox)") {
            return kGamepadDirectX;
        }
        if (a_device == "Gamepad (PS4)") {
            return kGamepadOrbis;
        }
        return kUnknown;
    }

    DEVICE from_RE_device(const RE::INPUT_DEVICE a_device) {
        switch (a_device) {
            case RE::INPUT_DEVICE::kKeyboard:
                return kKeyboardMouse;
            case RE::INPUT_DEVICE::kMouse:
                return kKeyboardMouse;
            case RE::INPUT_DEVICE::kGamepad: {
                if (RE::ControlMap::GetSingleton()->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis) {
                    return kGamepadOrbis;
                }
                return kGamepadDirectX;
            }
            default:
                return kUnknown;
        }
    }
}
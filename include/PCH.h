#pragma once
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace logger = SKSE::log;
using namespace std::literals;


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800
#define IMGUI_DEFINE_MATH_OPERATORS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#define MANAGER(T) T::Manager::GetSingleton()

#undef GetObject

constexpr float EPSILON = 1e-10f;

#include <wrl/client.h>

#include <DirectXTex.h>

#include <ankerl/unordered_dense.h>

using EventResult = RE::BSEventNotifyControl;

using KEY = RE::BSWin32KeyboardDevice::Key;
using GAMEPAD_DIRECTX = RE::BSWin32GamepadDevice::Key;
using GAMEPAD_ORBIS = RE::BSPCOrbisGamepadDevice::Key;
using MOUSE = RE::BSWin32MouseDevice::Key;

using FormID = RE::FormID;
using RefID = RE::FormID;

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

template <class K, class D>
using Map = ankerl::unordered_dense::map<K, D>;

struct string_hash {
    using is_transparent = void; // enable heterogeneous overloads
    using is_avalanching = void; // mark class as high quality avalanching hash

    [[nodiscard]] std::uint64_t operator()(const std::string_view str) const noexcept {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }
};

template <class D>
using StringMap = ankerl::unordered_dense::map<std::string, D, string_hash, std::equal_to<>>;
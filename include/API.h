#pragma once
#include <windows.h>

namespace SkyPromptAPI {

    #define DECLARE_API_FUNC_EX(                               \
        localName,   /* The name you call in your code */      \
        hostName,    /* The name actually exported by DLL */   \
        returnType,                                            \
        defaultValue,                                           \
        signature,   /* Parameter list in parentheses */        \
        callArgs     /* Just the parameter names in parentheses */ \
    )                                                          \
    using _##localName = returnType (*) signature;             \
    [[nodiscard]] inline returnType localName signature {      \
        static auto dllHandle = GetModuleHandle(L"SkyPrompt"); \
        if (!dllHandle) {                                   \
            return defaultValue;                               \
        }                                                      \
        static auto func =                                     \
            reinterpret_cast<_##localName>(GetProcAddress(dllHandle, hostName)); \
        if (func) {                                            \
            return func callArgs;                              \
        }                                                      \
        return defaultValue;                                   \
    }

	using ClientID = uint16_t;
	using EventID = uint16_t;
	using ActionID = uint16_t;
	using ButtonID = uint32_t; // RE::BSWin32KeyboardDevice::Key, RE::BSWin32MouseDevice::Key, RE::BSWin32GamepadDevice::Key, RE::BSPCOrbisGamepadDevice::Key

    constexpr ButtonID kMouseMove = 283;
    constexpr ButtonID kThumbstickMove = 284;
    constexpr ButtonID kSkyrim = 285;

    enum PromptType : std::uint8_t {
        kSinglePress,
        kHold,
        kHoldAndKeep,
    };

	struct Prompt {
		std::string_view text;
        EventID eventID;
		ActionID actionID;
		PromptType type;
		RE::FormID refid;
        std::span<const std::pair<RE::INPUT_DEVICE, ButtonID>> button_key;
        uint32_t text_color;
        explicit Prompt(const std::string_view a_text="", const EventID a_eventID=0, const ActionID a_actionID=0, const PromptType a_type=PromptType::kSinglePress, const RE::FormID a_refid=0, const std::span<const std::pair<RE::INPUT_DEVICE, ButtonID>> a_button_key={}, const uint32_t a_text_color=0xFFFFFFFF) :
			text(a_text), eventID(a_eventID), actionID(a_actionID), type(a_type), refid(a_refid), button_key(a_button_key), text_color(a_text_color) {}
	};

	enum PromptEventType : std::uint8_t {
		kAccepted,
		kDeclined,
        kRemovedByMod,
		kTimingOut,
		kTimeout,
		kDown,
        kUp,
		kMove
	};

    struct PromptEvent {
		Prompt prompt;
		PromptEventType type;
        std::pair<float,float> delta;
	};

    class PromptSink {
    public:
		virtual void ProcessEvent(PromptEvent event) const = 0;
		virtual std::span<const Prompt> GetPrompts() const = 0;
    protected:
        virtual ~PromptSink() = default;
    };

    DECLARE_API_FUNC_EX(
        RequestClientID,                          /* localName */
        "ProcessRequestClientID",                     /* hostName */
        ClientID,                                       /* returnType */
        0,                                      /* defaultValue */
        (), /* signature */
        ()         /* callArgs */
    );

    DECLARE_API_FUNC_EX(
        SendPrompt,                          /* localName */
        "ProcessSendPrompt",                     /* hostName */
        bool,                                       /* returnType */
        false,                                      /* defaultValue */
        (const PromptSink* a_sink, ClientID a_clientID), /* signature */
        (a_sink, a_clientID)         /* callArgs */
    );

    DECLARE_API_FUNC_EX(
        RemovePrompt,                          /* localName */
        "ProcessRemovePrompt",                     /* hostName */
        void,                                       /* returnType */
        ,                                      /* defaultValue */
        (const PromptSink* a_sink, ClientID a_clientID), /* signature */
        (a_sink, a_clientID)         /* callArgs */
    );

};

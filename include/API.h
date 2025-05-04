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

	struct Prompt {
		std::string_view text;
        std::span<std::pair<RE::INPUT_DEVICE, ButtonID>> button_key;
        EventID a_eventID;
		ActionID a_actionID;
	};

	enum PromptEventType {
		kAccepted,
		kDeclined,
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
		virtual void ProcessEvent(PromptEvent event) = 0;
		virtual std::span<const Prompt> GetPrompts() = 0;
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

    // 1) The macro name:       SendPrompt
    // 2) The return type:      bool
    // 3) The default value:    false
    // 4) The parameter list:   (PromptSink* a_sink, bool a_force)
    // 5) The call arguments:   (a_sink, a_force)

    DECLARE_API_FUNC_EX(
        SendPrompt,                          /* localName */
        "ProcessSendPrompt",                     /* hostName */
        bool,                                       /* returnType */
        false,                                      /* defaultValue */
        (PromptSink* a_sink, bool a_force, ClientID a_clientID, RE::FormID a_refid), /* signature */
        (a_sink, a_force, a_clientID, a_refid)         /* callArgs */
    );

    DECLARE_API_FUNC_EX(
        SendHint,                          /* localName */
        "ProcessSendHint",                     /* hostName */
        bool,                                       /* returnType */
        false,                                      /* defaultValue */
        (PromptSink* a_sink, ClientID a_clientID, RE::FormID a_refid), /* signature */
        (a_sink, a_clientID, a_refid)         /* callArgs */
    );

    // 1) The macro name:       RemovePrompt
    // 2) The return type:      void
    // 3) The default value:    
    // 4) The parameter list:   (PromptSink* a_sink)
    // 5) The call arguments:   (a_sink)

    DECLARE_API_FUNC_EX(
        RemovePrompt,                          /* localName */
        "ProcessRemovePrompt",                     /* hostName */
        void,                                       /* returnType */
        ,                                      /* defaultValue */
        (PromptSink* a_sink, ClientID a_clientID), /* signature */
        (a_sink, a_clientID)         /* callArgs */
    );

};

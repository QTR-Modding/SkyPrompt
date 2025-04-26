#pragma once

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

	using ClientID = uint8_t;
	using EventID = uint16_t;
	using ActionID = uint16_t;

	struct Prompt {
		std::string_view text;
        std::span<std::pair<RE::INPUT_DEVICE, uint32_t>> button_key;
        EventID a_eventID;
		ActionID a_actionID;
	};

    struct PromptEvent {
		Prompt prompt;
		int type; // 0 = accepted, 1 = declined, 2 = timeout
	};

    class PromptSink {
    public:
		virtual void ProcessEvent(PromptEvent event) = 0;
		virtual std::span<const Prompt> GetPrompts() = 0;
    protected:
        virtual ~PromptSink() = default;
    };


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
        (PromptSink* a_sink, bool a_force, uint16_t a_clientID), /* signature */
        (a_sink, a_force, a_clientID)         /* callArgs */
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

    DECLARE_API_FUNC_EX(
        RequestClientID,                          /* localName */
        "ProcessRequestClientID",                     /* hostName */
        ClientID,                                       /* returnType */
        0,                                      /* defaultValue */
        (), /* signature */
        ()         /* callArgs */
    );
};

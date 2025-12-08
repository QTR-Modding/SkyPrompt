#pragma once
#include "SkyPrompt/API.hpp"

#define DLLEXPORT __declspec(dllexport)


static_assert(sizeof(SkyPromptAPI::ClientID) == sizeof(uint16_t), "ClientID size mismatch");
static_assert(sizeof(SkyPromptAPI::EventID) == sizeof(uint16_t), "EventID size mismatch");
static_assert(sizeof(SkyPromptAPI::ActionID) == sizeof(uint16_t), "ActionID size mismatch");

extern "C" DLLEXPORT bool ProcessSendPrompt(const SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT void
ProcessRemovePrompt(const SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT SkyPromptAPI::ClientID ProcessRequestClientID(int a_major = 1, int a_minor = 0);
extern "C" DLLEXPORT bool ProcessRequestTheme(SkyPromptAPI::ClientID a_clientID, std::string_view theme_name);

namespace Service {
    inline std::mutex mutex_;
    inline SkyPromptAPI::ClientID last_clientID = 0;
};


struct PromptTypeFlags {
    enum Flag : std::uint8_t {
        kProgress = 1 << 0,
        kKeep = 1 << 1,
        kBlock = 1 << 2,
    };

    static constexpr Flag GetFlags(SkyPromptAPI::PromptType a_type) noexcept {
        auto flags = static_cast<Flag>(0);
        switch (a_type) {
            case SkyPromptAPI::PromptType::kHold:
            case SkyPromptAPI::PromptType::kHoldAndKeep:
            case SkyPromptAPI::PromptType::kHintHold:
            case SkyPromptAPI::PromptType::kHintHoldAndKeep:
                flags = static_cast<Flag>(flags | kProgress);
                break;
            default:
                break;
        }
        switch (a_type) {
            case SkyPromptAPI::PromptType::kHoldAndKeep:
            case SkyPromptAPI::PromptType::kHintHoldAndKeep:
                flags = static_cast<Flag>(flags | kKeep);
                break;
            default:
                break;
        }
        switch (a_type) {
            case SkyPromptAPI::PromptType::kSinglePress:
            case SkyPromptAPI::PromptType::kHold:
            case SkyPromptAPI::PromptType::kHoldAndKeep:
                flags = static_cast<Flag>(flags | kBlock);
                break;
            default:
                break;
        }
        return flags;
    }

    static constexpr bool GetBlocksInput(const SkyPromptAPI::PromptType a_type) noexcept {
        return static_cast<bool>(GetFlags(a_type) & kBlock);
    }

    static constexpr bool GetHasProgress(const SkyPromptAPI::PromptType a_type) noexcept {
        return static_cast<bool>(GetFlags(a_type) & kProgress);
    }

    static constexpr bool GetIsHoldAndKeepType(const SkyPromptAPI::PromptType a_type) noexcept {
        return static_cast<bool>(GetFlags(a_type) & kKeep);
    }
};
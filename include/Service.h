#pragma once
#include "SkyPrompt/API.hpp"

#define DLLEXPORT __declspec(dllexport)


static_assert(sizeof(SkyPromptAPI::ClientID) == sizeof(uint16_t), "ClientID size mismatch");
static_assert(sizeof(SkyPromptAPI::EventID) == sizeof(uint16_t), "EventID size mismatch");
static_assert(sizeof(SkyPromptAPI::ActionID) == sizeof(uint16_t), "ActionID size mismatch");

extern "C" DLLEXPORT bool ProcessSendPrompt(const SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT void ProcessRemovePrompt(const SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT SkyPromptAPI::ClientID ProcessRequestClientID(int a_major=1, int a_minor=0);
extern "C" DLLEXPORT bool ProcessRequestTheme(SkyPromptAPI::ClientID a_clientID, std::string_view theme_name);

namespace Service {
    inline std::mutex mutex_;
    inline SkyPromptAPI::ClientID last_clientID = 0;
};

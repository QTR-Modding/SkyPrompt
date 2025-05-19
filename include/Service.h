#pragma once
#include "API.h"

#define DLLEXPORT __declspec(dllexport)


static_assert(sizeof(SkyPromptAPI::ClientID) == sizeof(uint16_t), "ClientID size mismatch");
static_assert(sizeof(SkyPromptAPI::EventID) == sizeof(uint16_t), "EventID size mismatch");
static_assert(sizeof(SkyPromptAPI::ActionID) == sizeof(uint16_t), "ActionID size mismatch");

extern "C" DLLEXPORT bool ProcessSendPrompt(const SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT void ProcessRemovePrompt(const SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT SkyPromptAPI::ClientID ProcessRequestClientID();

namespace Interactions {
    inline std::mutex mutex_;
    inline SkyPromptAPI::ClientID last_clientID = 0;
};

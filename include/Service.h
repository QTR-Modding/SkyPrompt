#pragma once
#include "API.h"

#define DLLEXPORT __declspec(dllexport)

extern "C" DLLEXPORT bool ProcessSendPrompt(SkyPromptAPI::PromptSink* a_sink, bool a_force, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT void ProcessRemovePrompt(SkyPromptAPI::PromptSink* a_sink, SkyPromptAPI::ClientID a_clientID);
extern "C" DLLEXPORT SkyPromptAPI::ClientID ProcessRequestClientID();

namespace Interactions {
    inline std::mutex mutex_;
    inline SkyPromptAPI::ClientID last_clientID = 0;
};

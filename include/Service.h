#pragma once

#include "API.h"

#define DLLEXPORT __declspec(dllexport)

extern "C" DLLEXPORT bool ProcessSendPrompt(SkyPromptAPI::PromptSink* a_sink, bool a_force);
extern "C" DLLEXPORT void ProcessRemovePrompt(SkyPromptAPI::PromptSink* a_sink);

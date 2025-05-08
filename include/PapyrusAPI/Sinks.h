#pragma once
#include <shared_mutex>
#include "API.h"

namespace PapyrusAPI {
	using PromptKey = std::pair<SkyPromptAPI::EventID,SkyPromptAPI::ActionID>;
	class PapyrusSink final : public SkyPromptAPI::PromptSink {
	public:
        explicit PapyrusSink(const SkyPromptAPI::ClientID a_clientID) : clientID(a_clientID), prompt(), last_type() {
        }

        SkyPromptAPI::ClientID clientID{};
        SkyPromptAPI::Prompt prompt;
        void ProcessEvent(SkyPromptAPI::PromptEvent event) override;
        std::span<const SkyPromptAPI::Prompt> GetPrompts() override { return {{prompt}}; }
		[[nodiscard]] SkyPromptAPI::EventID GetEventID() const { return prompt.eventID; }
		[[nodiscard]] SkyPromptAPI::ActionID GetActionID() const { return prompt.actionID; }
	protected:
        std::shared_mutex prompt_mutex_;
        SkyPromptAPI::PromptEventType last_type;
    };

    inline std::shared_mutex mutex_;
    inline std::unordered_map<SkyPromptAPI::ClientID, std::map<PromptKey,std::unique_ptr<PapyrusSink>>> papyrusSinks;

    PapyrusSink* AddPrompt(SkyPromptAPI::ClientID clientID, const std::string& text, SkyPromptAPI::EventID eventID,
                           SkyPromptAPI::ActionID actionID, SkyPromptAPI::PromptType type, const RE::TESForm* refForm,
                        std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>>* buttonKeys=nullptr);
    PapyrusSink* GetPrompt(SkyPromptAPI::ClientID clientID,SkyPromptAPI::EventID eventID,
                           SkyPromptAPI::ActionID actionID);
};
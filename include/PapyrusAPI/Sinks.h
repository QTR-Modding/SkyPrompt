#pragma once
#include <shared_mutex>
#include "SkyPrompt/API.hpp"

namespace PapyrusAPI {
	inline SKSE::RegistrationSet<int, int, int, int, float, float, float> skyPromptEvents("OnSkyPromptEvent"sv);

	using PromptKey = std::pair<SkyPromptAPI::EventID,SkyPromptAPI::ActionID>;
	class PapyrusSink final : public SkyPromptAPI::PromptSink {
	public:
		explicit PapyrusSink(const SkyPromptAPI::ClientID a_clientID) : last_type(), clientID(a_clientID){};

		~PapyrusSink() override = default;

        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        [[nodiscard]] SkyPromptAPI::EventID GetEventID() const;
        [[nodiscard]] SkyPromptAPI::ActionID GetActionID() const;
        void SetKeys(const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>>& buttonKeys);
		void SetText(const std::string& a_text);
		void SetType(SkyPromptAPI::PromptType a_type);
		void SetRefID(RE::FormID a_refid);
		void SetProgress(float progress);
		void SetEventID(SkyPromptAPI::EventID a_eventID);
		void SetActionID(SkyPromptAPI::ActionID a_actionID);
	private:
        mutable SkyPromptAPI::PromptEventType last_type;
        mutable std::shared_mutex prompt_mutex_;
        std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> bindings;
		std::string text;
	    SkyPromptAPI::Prompt prompt;
        SkyPromptAPI::ClientID clientID{};
	};

    inline std::shared_mutex mutex_;
    inline std::unordered_map<SkyPromptAPI::ClientID, std::map<PromptKey,std::unique_ptr<PapyrusSink>>> papyrusSinks;

    bool AddPrompt(SkyPromptAPI::ClientID clientID, const std::string& text, SkyPromptAPI::EventID eventID,
                   SkyPromptAPI::ActionID actionID, SkyPromptAPI::PromptType type, const RE::TESForm* refForm,
                   const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>>& buttonKeys, float progress);
}
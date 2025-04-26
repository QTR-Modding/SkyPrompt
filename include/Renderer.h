#pragma once
#include <atomic>
#include <shared_mutex>
#include "MCP.h"
#include "Settings.h"
#include "API.h"


namespace ImGui::Renderer {
    struct ButtonState;
}

struct InteractionButton
{
	Interaction interaction;
	std::string text;

	[[nodiscard]] uint32_t button_key() const;

	// constructor
    explicit InteractionButton(const Interaction& a_interaction) : interaction(a_interaction) {text=interaction.name();}
	bool operator==(const InteractionButton& a_rhs) const { return text == a_rhs.text; }
	bool operator<(const InteractionButton& a_rhs) const { return text < a_rhs.text; }
};

struct Button2Show {
	InteractionButton iButton;
	mutable float alpha;
	mutable float lifetime=MCP::Settings::lifetime;
	mutable float elapsed = 0.0f;

	void Update(float a_timeStep) const;
	void Reset() const;
	void WakeUp() const;
	std::optional<std::pair<float,float>> Show(bool hiding=false, const std::string& extra_text="", float progress=0.f, float button_state = -1.f) const;
	void Hide() const;
	bool IsHidden() const;

	[[nodiscard]] bool expired() const { return elapsed >= lifetime; }
    explicit Button2Show(const Interaction& a_interaction, const float a_alpha = 0.0f) : iButton(a_interaction), alpha(a_alpha) {}

	bool operator<(const Button2Show& a_rhs) const
	{
		return iButton < a_rhs.iButton;
	}

	bool operator==(const Button2Show& a_rhs) const
	{
		return iButton == a_rhs.iButton;
	}
};

struct ButtonQueue {

	// default constructor
	ButtonQueue() = default;

	std::set<Button2Show> buttons;
    const Button2Show* current_button=nullptr;
	void Reset() const;
	void WakeUp() const;
	void Show(float progress,const Button2Show* button2show, const ImGui::Renderer::ButtonState& a_button_state);
	const Button2Show* AddButton(const Button2Show& a_button);
    bool RemoveButton(const Interaction& a_interaction);
	[[nodiscard]] bool IsEmpty() const { return buttons.empty(); }
	[[nodiscard]] const Button2Show* Next() const;
	[[nodiscard]] size_t size() const { return buttons.size(); }

	mutable std::pair<float,float> position = { 0.0f, 0.0f };
};

namespace ImGui::Renderer
{

	namespace DisplayTweaks
	{
		inline float resolutionScale{ 1.0f };
		inline bool  borderlessUpscale{ false };
	}

    inline void LoadSettings(const CSimpleIniA& a_ini)
	{
		DisplayTweaks::resolutionScale = static_cast<float>(a_ini.GetDoubleValue("Render", "ResolutionScale", DisplayTweaks::resolutionScale));
		DisplayTweaks::borderlessUpscale = static_cast<float>(a_ini.GetBoolValue("Render", "BorderlessUpscale", DisplayTweaks::borderlessUpscale));
	}

	float GetResolutionScale();
	static void RenderPrompt();
	void RenderUI(); // starts here

	struct ButtonState {
		bool isPressing = false;
        int pressCount = 0;
        std::chrono::steady_clock::time_point lastPressTime;
    };

	class SubManager
	{

		mutable std::shared_mutex q_mutex_;
		mutable std::shared_mutex progress_mutex_;
		mutable std::shared_mutex sink_mutex_;
	    ButtonQueue interactQueue;
		float progress_circle = 0.0f;
		float progress_circle_max = 1.0f;

		std::atomic<bool> blockProgress = false;

		std::map<Interaction,std::vector<SkyPromptAPI::PromptSink*>> sinks;

		void RemoveFromSinks(SkyPromptAPI::PromptSink* a_prompt_sink);
		void ButtonStateActions();

	public:

        SubManager() = default;
		~SubManager();

        ButtonState buttonState;

        void Add2Q(const Interaction& a_interaction, bool show=true);
        bool RemoveFromQ(const Interaction& a_interaction);
        void RemoveFromQ(SkyPromptAPI::PromptSink* a_prompt_sink);
		void RemoveCurrentPrompt();
        void ResetQueue();
        void ShowQueue();
        void WakeUpQueue() const;
		void CleanUpQueue();
		void ClearQueue();
		bool HasQueue() const;
		void Start();
		void Stop();
		bool UpdateProgressCircle(bool isPressing);
		float GetProgressCircle() const;
		uint32_t GetPromptKey() const;
		void OnProgressComplete();
		void NextPrompt();
		bool HasPrompt() const;
		bool IsHidden() const;

		std::vector<Interaction> GetInteractions() const;
		Interaction GetCurrentInteraction() const;
		void AddSink(const Interaction& a_interaction, SkyPromptAPI::PromptSink* a_sink);
		std::map<Interaction, std::vector<SkyPromptAPI::PromptSink*>> GetSinks() const { return sinks; }
		bool IsInQueue(SkyPromptAPI::PromptSink* a_sink) const;
		bool IsInQueue(const Interaction& a_interaction) const;
		void SendEvent(const Interaction& a_interaction, int event_type);
	};

	class Manager : public clib_util::singleton::ISingleton<Manager>
	{

		void ReArrange();
		bool IsInQueue(const Interaction& a_interaction) const;

	public:

		static bool IsGameFrozen();
		std::atomic<bool> isPaused = false;
		std::vector<std::unique_ptr<SubManager>> managers;

		mutable std::shared_mutex mutex_;

        std::unique_ptr<SubManager>& Add2Q(const Interaction& a_interaction, bool show = true);
        bool Add2Q(SkyPromptAPI::PromptSink* a_prompt_sink);
		bool IsInQueue(SkyPromptAPI::PromptSink* a_prompt_sink) const;
		void RemoveFromQ(const Interaction& a_interaction);
		void RemoveFromQ(SkyPromptAPI::PromptSink* a_prompt_sink) const;
		[[nodiscard]] bool HasTask() const;
		void Start();
		void Stop();
		void CleanUpQueue();
        void ShowQueue() const;
        void Clear();
        void ResetQueue() const;
        void WakeUpQueue() const;
		bool IsPaused() const { return isPaused.load(); }
		bool IsHidden() const;
		SubManager* GetSubManagerByKey(uint32_t a_prompt_key) const;
		std::vector<uint32_t> GetPromptKeys() const;

		void ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func);
	};
}
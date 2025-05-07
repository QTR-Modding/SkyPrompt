#pragma once
#include <shared_mutex>
#include "imgui.h"
#include "API.h"
#include "MCP.h"
#include "Settings.h"


namespace ImGui::Renderer {
    struct ButtonState;
}

struct InteractionButton
{
    Interaction interaction;
    std::string text;
    SkyPromptAPI::PromptType type = SkyPromptAPI::PromptType::kSinglePress;
    RE::ObjectRefHandle attached_object;

    [[nodiscard]] uint32_t button_key() const;

    explicit InteractionButton(const Interaction& a_interaction, SkyPromptAPI::PromptType a_type, RefID a_refid);
    bool operator==(const InteractionButton& a_rhs) const { return text == a_rhs.text; }
    bool operator<(const InteractionButton& a_rhs) const { return interaction < a_rhs.interaction; }
    [[nodiscard]] std::optional<std::pair<float,float>> Show(float alpha = false, const std::string& extra_text="", float progress=0.f, float button_state = -1.f) const;
};

struct ButtonQueue {

    ButtonQueue() = default;

    float alpha;
    float lifetime=MCP::Settings::lifetime;
    float elapsed = 0.0f;
    [[nodiscard]] bool expired() const { return elapsed >= lifetime; }
    bool IsHidden() const { return alpha <= 0.f; }

    std::set<InteractionButton> buttons;
    const InteractionButton* current_button=nullptr;
    void Clear();
    void Reset();
    void WakeUp();
    void Show(float progress,const InteractionButton* button2show, const ImGui::Renderer::ButtonState& a_button_state);
    const InteractionButton* AddButton(const InteractionButton& a_button);
    bool RemoveButton(const Interaction& a_interaction);
    [[nodiscard]] bool IsEmpty() const { return buttons.empty(); }
    [[nodiscard]] const InteractionButton* Next() const;
    [[nodiscard]] size_t size() const { return buttons.size(); }

    std::pair<float,float> position = { 0.0f, 0.0f };
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
    void RenderPrompts(); // starts here

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

        void Add2Q(const Interaction& a_interaction, SkyPromptAPI::PromptType a_type, RefID a_refid, bool show=true);
        bool RemoveFromQ(const Interaction& a_interaction);
        void RemoveFromQ(SkyPromptAPI::PromptSink* a_prompt_sink);
        void RemoveCurrentPrompt();
        void ResetQueue();
        void ShowQueue();
        void WakeUpQueue();
        void CleanUpQueue();
        void ClearQueue(SkyPromptAPI::PromptEventType a_type=SkyPromptAPI::kDeclined);
        bool HasQueue() const;
        void Start();
        void Stop();
        bool UpdateProgressCircle(bool isPressing);
        uint32_t GetPromptKey() const;
        void NextPrompt();
        bool HasPrompt() const;
        bool IsHidden() const;

        std::vector<Interaction> GetInteractions() const;
        Interaction GetCurrentInteraction() const;
        std::vector<InteractionButton> GetButtons() const;
        const InteractionButton* GetCurrentButton() const;
        void AddSink(const Interaction& a_interaction, SkyPromptAPI::PromptSink* a_sink);
        std::map<Interaction, std::vector<SkyPromptAPI::PromptSink*>> GetSinks() const { return sinks; }
        bool IsInQueue(SkyPromptAPI::PromptSink* a_sink) const;
        bool IsInQueue(const Interaction& a_interaction) const;
        void SendEvent(const Interaction& a_interaction, SkyPromptAPI::PromptEventType event_type, std::pair<float,float> delta = {0.f,0.f});

        ImVec2 GetAttachedObjectPos() const;
        RE::TESObjectREFR* GetAttachedObject() const;
	};

    class Manager : public clib_util::singleton::ISingleton<Manager>
    {

        void ReArrange();
        bool IsInQueue(const Interaction& a_interaction) const;

        std::shared_mutex events_to_send_mutex;
        std::map<SkyPromptAPI::PromptSink*, std::vector<SkyPromptAPI::PromptEvent>> events_to_send_;

    public:

        static bool IsGameFrozen();
        std::atomic<bool> isPaused = false;
        std::vector<std::unique_ptr<SubManager>> managers;

        mutable std::shared_mutex mutex_;

        std::unique_ptr<SubManager>& Add2Q(const Interaction& a_interaction, SkyPromptAPI::PromptType a_type,
                                           RefID a_refid, bool show = true, const std::map<Input::DEVICE, std::vector<uint32_t>>& buttonKeys = {});
        bool Add2Q(SkyPromptAPI::PromptSink* a_prompt_sink, SkyPromptAPI::ClientID a_clientID);
        bool IsInQueue(SkyPromptAPI::PromptSink* a_prompt_sink, bool wake_up=false) const;
        void RemoveFromQ(SkyPromptAPI::PromptSink* a_prompt_sink) const;
        [[nodiscard]] bool HasTask() const;
        void Start();
        void Stop();
        void CleanUpQueue();
        void ShowQueue() const;
        void Clear(bool API_call=false);
        void ResetQueue() const;
        void WakeUpQueue() const;
        bool IsPaused() const { return isPaused.load(); }
        bool IsHidden() const;
        SubManager* GetSubManagerByKey(uint32_t a_prompt_key) const;
        std::vector<uint32_t> GetPromptKeys() const;

        void ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func);
        void AddEventToSend(SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::Prompt& a_prompt, SkyPromptAPI::PromptEventType event_type, std::
                            pair<float, float> a_delta);
        void SendEvents();
	};
}
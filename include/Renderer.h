#pragma once
#include <shared_mutex>
#include "imgui.h"
#include "SkyPrompt/API.hpp"
#include "Interaction.h"
#include "MCP.h"
#include "Theme.h"
#include "PromptLayouts.h"
#include "ActivationPop.h"
#include "ClibUtil/simpleINI.hpp"


namespace ImGui::Renderer {
    struct ButtonState;

    struct ButtonMutables {
        std::string text;
        uint32_t text_color;
        float progress;
    };

    constexpr float progress_circle_offset = 1.f / 12.f;
    constexpr float progress_circle_offset_deg = 360.f * progress_circle_offset * 0.5f;
}

struct InteractionButton {
    using Mutables = ImGui::Renderer::ButtonMutables;
    Interaction interaction;
    mutable Mutables mutables;
    SkyPromptAPI::PromptType type = SkyPromptAPI::PromptType::kSinglePress;
    RE::ObjectRefHandle attached_object;
    std::map<Input::DEVICE, uint32_t> keys;
    mutable int default_key_index = 0;

    [[nodiscard]] uint32_t GetKey(const Theme::Theme& theme) const;

    explicit InteractionButton(const Interaction& a_interaction, const Mutables& a_mutables,
                               SkyPromptAPI::PromptType a_type, RefID a_refid, std::map<Input::DEVICE, uint32_t> a_keys,
                               int a_default_key_index);
    bool operator==(const InteractionButton& a_rhs) const { return interaction == a_rhs.interaction; }
    bool operator<(const InteractionButton& a_rhs) const { return interaction < a_rhs.interaction; }
    float GetProgressOverride(bool increment) const;
};

struct ButtonQueue {
    ButtonQueue() = default;

    float alpha;
    float lifetime = MCP::Settings::lifetime;
    float elapsed = 0.0f;
    [[nodiscard]] bool expired() const { return elapsed >= lifetime; }
    [[nodiscard]] bool IsHidden() const { return alpha <= 0.f; }

    std::set<InteractionButton> buttons;
    const InteractionButton* current_button = nullptr;
    void Clear();
    void Reset();
    void WakeUp();
    void Show(const Theme::Theme& theme, float progress, const InteractionButton* button2show,
              const ImGui::Renderer::ButtonState& a_button_state);
    const InteractionButton* AddButton(const InteractionButton& a_button);
    bool RemoveButton(const Interaction& a_interaction);
    [[nodiscard]] bool IsEmpty() const { return buttons.empty(); }
    [[nodiscard]] const InteractionButton* Next() const;
    [[nodiscard]] size_t size() const { return buttons.size(); }
};

namespace ImGui::Renderer {
    namespace DisplayTweaks {
        inline float resolutionScale{1.0f};
        inline bool borderlessUpscale{false};
    }

    inline void LoadSettings(const CSimpleIniA& a_ini) {
        DisplayTweaks::resolutionScale = static_cast<float>(a_ini.GetDoubleValue(
            "Render", "ResolutionScale", DisplayTweaks::resolutionScale));
        DisplayTweaks::borderlessUpscale = static_cast<float>(a_ini.GetBoolValue(
            "Render", "BorderlessUpscale", DisplayTweaks::borderlessUpscale));
    }

    float GetResolutionScale();
    void RenderPrompts(); // starts here

    struct ButtonState {
        bool isPressing = false;
        bool acceptedThisPress = false;
        int pressCount = 0;
        std::chrono::steady_clock::time_point lastPressTime;

        void Reset() {
            isPressing = false;
            acceptedThisPress = false;
            pressCount = 0;
        }
    };

    class SubManager {
        mutable std::shared_mutex q_mutex_;
        mutable std::shared_mutex progress_mutex_;
        mutable std::shared_mutex sink_mutex_;
        ButtonQueue interactQueue;
        float progress_circle = 0.0f;
        float progress_circle_max = 1.f;

        std::atomic<bool> blockProgress = false;

        std::map<Interaction, std::vector<const SkyPromptAPI::PromptSink*>> sinks;

        mutable std::atomic<bool> wakeup_queued_{false};

        void ButtonStateActions(const Theme::Theme& theme);
        void ResetButtonState();
        void RestoreMutables(const Interaction& a_interaction);
        void Show(const Theme::Theme& theme, const InteractionButton* button2show);

    public:
        SubManager() = default;
        ~SubManager();

        ButtonState buttonState;

        void Add2Q(const Theme::Theme& theme, const InteractionButton& iButton, bool show = true);
        bool RemoveFromQ(const Interaction& a_interaction);
        bool RemoveFromQ(SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink,
                         const std::optional<Interaction>& a_interaction = std::nullopt);
        void SetDefaultKeyIndex(const Theme::Theme& theme, int index,
                                std::optional<uint32_t> previousKey = std::nullopt, bool receivesInput = true);
        void ResetQueue();
        void ShowQueue(const Theme::Theme& theme);
        void WakeUpQueue();
        void CleanUpQueue(const Theme::Theme& theme);
        void ClearQueue();
        void ClearQueue(SkyPromptAPI::PromptEventType a_event_type);
        bool HasQueue() const;
        void Start();
        void Stop();
        bool UpdateProgressCircle(const Theme::Theme& theme, bool isPressing);
        uint32_t GetPromptKey(const Theme::Theme& theme) const;
        SkyPromptAPI::PromptType GetPromptType() const;
        void NextPrompt(const Theme::Theme& theme);
        bool HasPrompt() const;
        bool IsHidden() const;

        std::vector<Interaction> GetInteractions() const;
        SkyPromptAPI::ClientID GetClientID() const;
        Interaction GetCurrentInteraction() const;
        const InteractionButton* GetCurrentButton() const;
        void AddSink(const Interaction& a_interaction, const SkyPromptAPI::PromptSink* a_sink);
        bool IsInQueue(const SkyPromptAPI::PromptSink* a_sink) const;
        void SendEvent(const Interaction& a_interaction, SkyPromptAPI::PromptEventType event_type,
                       std::pair<float, float> delta = {0.f, 0.f}, float progress_override = 0.f);

        struct AttachedPosition {
            ImVec2 screen;
            std::optional<VR::WorldAnchor> world;
        };
        AttachedPosition GetAttachedObjectPos(const Theme::Theme& theme) const;
        RE::TESObjectREFR* GetAttachedObject() const;

        void Update(const Interaction& a_interaction, const ButtonMutables& a_mutables) const;
    };

    class Manager : public REX::Singleton<Manager> {
        void RestorePromptOrder(SkyPromptAPI::ClientID pageID, SkyPromptAPI::ClientID groupID,
                                SkyPromptAPI::ClientID clientID,
                                std::span<const SkyPromptAPI::Prompt> prompts);

        std::shared_mutex events_to_send_mutex;
        std::map<std::pair<SkyPromptAPI::ClientID, const SkyPromptAPI::PromptSink*>,
                 std::deque<SkyPromptAPI::PromptEvent>> events_to_send_;
        SubManager* Add2Q(SkyPromptAPI::ClientID pageID, SkyPromptAPI::ClientID groupID, const Interaction& a_interaction,
                          const ButtonMutables& a_mutables,
                          SkyPromptAPI::PromptType a_type, RefID a_refid,
                          const std::map<Input::DEVICE, uint32_t>& a_bttn_map, bool show = true);

        bool SwitchToPage(SkyPromptAPI::ClientID pageID);

        SkyPromptAPI::ClientID current_pageID = 0;

        mutable std::shared_mutex mutex_;
        std::atomic<bool> isPaused = false;


        struct Group {
            std::vector<std::unique_ptr<SubManager>> managers;
            PromptLayouts::List list;
            Theme::Theme* theme = &Theme::default_theme;

            void Render(SkyPromptAPI::ClientID pageID, SkyPromptAPI::ClientID groupID,
                        SkyPromptAPI::ClientID pageIndex, SkyPromptAPI::ClientID pageCount);
            void ShowPromptRow(size_t index, bool isList, size_t visibleCount);
            std::optional<bool> ProcessInput(RE::InputEvent* event, uint32_t buttonKey);
            std::optional<bool> ProcessInput(Input::VRNavigation::Direction direction);
            std::optional<bool> ProcessListInput(RE::InputEvent* event, uint32_t buttonKey);
            std::optional<bool> ProcessListNavigation(PromptLayouts::List::Navigation navigation);
            SubManager* GetSubManagerByKey(uint32_t a_prompt_key) const;
            std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>> GetPromptButtons() const;
            void CollectPromptButtons(std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>>& buttons) const;
            bool WakeUpPrompt(const SkyPromptAPI::PromptSink* a_prompt_sink) const;
            bool RemoveEmptyRows(bool active);
            bool ContainsClient(SkyPromptAPI::ClientID clientID) const;
            void CollectClients(std::set<SkyPromptAPI::ClientID>& clients) const;
            void CollectRowKeys(std::map<const SubManager*, uint32_t>& keys) const;
            void MoveClientRowsTo(SkyPromptAPI::ClientID clientID, Group& target);
            void UpdateRowKeys(const std::map<const SubManager*, uint32_t>& previousKeys);
            bool CanJoin(SkyPromptAPI::ClientID clientID) const;
            SubManager* Add2Q(const Interaction& a_interaction, const ButtonMutables& a_mutables,
                              SkyPromptAPI::PromptType a_type, RefID a_refid,
                              const std::map<Input::DEVICE, uint32_t>& a_bttn_map, bool show);
            bool RemoveFromQ(SkyPromptAPI::ClientID clientID, const SkyPromptAPI::PromptSink* sink,
                             const std::optional<Interaction>& interaction, bool active);
            void RestorePromptOrder(SkyPromptAPI::ClientID clientID, std::span<const SkyPromptAPI::Prompt> prompts,
                                    bool active);
            void Start();
            void Stop();
            bool CleanUpQueue();
            void Clear(SkyPromptAPI::PromptEventType eventType);
            void ResetQueue() const;
            void WakeUpQueue() const;
            bool HasQueue() const;
            bool IsHidden() const;
            bool IsEmpty() const { return managers.empty(); }
            void OnShown();
            void ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func);
        };

        std::map<SkyPromptAPI::ClientID, std::map<SkyPromptAPI::ClientID, Group>> pages;
        std::map<SkyPromptAPI::ClientID, Theme::Theme*> requestedThemes;

        // Caller holds mutex_.
        Group* GetGroup(SkyPromptAPI::ClientID pageID, SkyPromptAPI::ClientID groupID);

        // Caller holds mutex_.
        std::pair<SkyPromptAPI::ClientID, SkyPromptAPI::ClientID> FindGroupIDs(SkyPromptAPI::ClientID clientID) const;
        bool CanJoinPage(SkyPromptAPI::ClientID pageID, SkyPromptAPI::ClientID clientID) const;
        // Caller holds mutex_ exclusively and m_theme_ shared or exclusively.
        SkyPromptAPI::ClientID RegroupPage(SkyPromptAPI::ClientID pageID, SkyPromptAPI::ClientID addedClient = 0);

        void Clear(SkyPromptAPI::PromptEventType a_event_type);

    public:
        ActivationPop activationPop;

        static bool IsGameFrozen();
        static Interaction MakeInteraction(SkyPromptAPI::ClientID a_clientID, SkyPromptAPI::EventID a_event,
                                           SkyPromptAPI::ActionID a_action);

        bool Add2Q(const SkyPromptAPI::PromptSink* a_prompt_sink, SkyPromptAPI::ClientID a_clientID);
        bool RemoveFromQ(SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_prompt_sink,
                         const std::optional<Interaction>& a_interaction = std::nullopt);
        [[nodiscard]] bool HasTask() const;
        void Start();
        void Stop();
        void CleanUpQueue();
        void Render();
        void ResetQueue() const;
        void WakeUpQueue() const;
        bool IsPaused() const { return isPaused.load(); }
        bool IsHidden() const;
        std::optional<bool> ProcessInput(RE::InputEvent* event, uint32_t buttonKey);
        std::optional<bool> ProcessInput(Input::VRNavigation::Direction direction);
        std::vector<std::pair<SkyPromptAPI::PromptType, uint32_t>> GetPromptButtons() const;

        void ForEachManager(const std::function<void(std::unique_ptr<SubManager>&)>& a_func);
        void AddEventToSend(SkyPromptAPI::ClientID a_clientID, const SkyPromptAPI::PromptSink* a_sink,
                            const SkyPromptAPI::Prompt& a_prompt,
                            SkyPromptAPI::PromptEventType event_type,
                            std::pair<float, float> a_delta);
        void SendEvents();

        bool InitializeClient(SkyPromptAPI::ClientID a_clientID);
        bool RequestTheme(SkyPromptAPI::ClientID a_clientID, std::string_view theme_name);
        std::vector<const Theme::Theme*> GetCurrentThemes() const;
        void PrepareFonts();
        bool CyclePage(bool a_left);
    };
}

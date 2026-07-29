#pragma once
#include "Utils.h"
#include "SkyPrompt/API.hpp"
#include "MCP.h"
#include "Translations.h"

namespace Tutorial {
    constexpr std::string_view quit_me = "$SkyPromptTutorialQuit";
    inline std::atomic_bool showing_tutorial = false;
    void ReadMenuFrameworkStrings();
    inline std::string MF_KB_key;
    inline std::string MF_KB_mode;
    inline std::string MF_GP_key;
    inline std::string MF_GP_mode;
    inline float old_xpos;
    inline float old_ypos;

    inline SkyPromptAPI::ClientID client_id = 0;

    namespace Tutorial5 {
        constexpr std::string_view str1 = "$SkyPromptTutorialMash";

        const SkyPromptAPI::Prompt prompt1(str1, 0, 0, SkyPromptAPI::PromptType::kSinglePress);
        const SkyPromptAPI::Prompt prompt4(quit_me, 1, 0, SkyPromptAPI::PromptType::kSinglePress);

        class Sink final : public SkyPromptAPI::PromptSink, public REX::Singleton<Sink> {
            mutable std::array<SkyPromptAPI::Prompt, 2> m_prompts = {prompt1, prompt4};
            float mult = 8.f;

        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return m_prompts; }
            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;

            void Start() const;
        };
    }

    namespace Tutorial4 {
        constexpr std::string_view str1 = "$SkyPromptTutorialPress";

        const SkyPromptAPI::Prompt prompt1(str1, 0, 0, SkyPromptAPI::PromptType::kSinglePress);
        const SkyPromptAPI::Prompt prompt4(quit_me, 1, 0, SkyPromptAPI::PromptType::kSinglePress);

        class Sink final : public SkyPromptAPI::PromptSink, public REX::Singleton<Sink> {
            std::array<SkyPromptAPI::Prompt, 2> m_prompts = {prompt1, prompt4};

        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return m_prompts; }
            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;

            void Start();
        };
    }

    namespace Tutorial3 {
        inline std::chrono::steady_clock::time_point last_delete_t;

        constexpr std::string_view str1 = "$SkyPromptTutorialDeleteAll";
        constexpr std::string_view str2 = "$SkyPromptTutorialDeleteAll ";

        const SkyPromptAPI::Prompt prompt1(str1, 0, 0, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt2(str2, 0, 1, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt4(quit_me, 1, 0, SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

        class Sink final : public SkyPromptAPI::PromptSink, public REX::Singleton<Sink> {
            std::array<const SkyPromptAPI::Prompt, 3> m_prompts = {prompt1, prompt2, prompt4};

        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return m_prompts; }
            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };
    }

    namespace Tutorial2 {
        inline std::atomic_bool showing_tutorial = false;

        constexpr std::string_view str1 = "$SkyPromptTutorialSkipNext";
        constexpr std::string_view str2 = "$SkyPromptTutorialSkipNext ";

        const SkyPromptAPI::Prompt prompt1(str1, 0, 0, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt2(str2, 0, 1, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt4(quit_me, 1, 0, SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

        class Sink final : public SkyPromptAPI::PromptSink, public REX::Singleton<Sink> {
            std::array<const SkyPromptAPI::Prompt, 3> m_prompts = {prompt1, prompt2, prompt4};

        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return m_prompts; }
            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };
    }

    namespace Tutorial1 {
        inline std::chrono::steady_clock::time_point last_delete_t;

        constexpr std::string_view str1 = "$SkyPromptTutorialDelete";
        constexpr std::string_view str2 = "$SkyPromptTutorialDelete ";

        const SkyPromptAPI::Prompt prompt1(str1, 0, 0, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt2(str2, 0, 1, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt4(quit_me, 1, 0, SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

        class Sink final : public SkyPromptAPI::PromptSink, public REX::Singleton<Sink> {
            std::array<const SkyPromptAPI::Prompt, 3> m_prompts = {prompt1, prompt2, prompt4};

        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return m_prompts; }
            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };
    };

    namespace Tutorial0 {
        inline std::chrono::steady_clock::time_point last_delete_t;

        constexpr std::string_view str1 = "$SkyPromptTutorialAccept";
        constexpr std::string_view str2 = "$SkyPromptTutorialAccept ";

        const SkyPromptAPI::Prompt prompt1(str1, 0, 0, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt2(str2, 0, 1, SkyPromptAPI::PromptType::kHold);
        const SkyPromptAPI::Prompt prompt4(quit_me, 1, 0, SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

        class Sink final : public SkyPromptAPI::PromptSink, public REX::Singleton<Sink> {
            std::array<const SkyPromptAPI::Prompt, 3> m_prompts = {prompt1, prompt2, prompt4};

        public:
            std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return m_prompts; }
            void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };
    }

    class Manager : public REX::Singleton<Manager> {
    public:
        static void Callback(const unsigned int a_int) {
            if (!a_int) {
                if (client_id == 0) {
                    client_id = SkyPromptAPI::RequestClientID();
                }
                if (client_id > 0) {
                    Tutorial0::to_be_deleted = {0, 1};
                    Tutorial1::to_be_deleted = {0, 1};
                    Tutorial2::to_be_deleted = {0, 1};
                    Tutorial3::to_be_deleted = {0, 1};
                    if (!SkyPromptAPI::RequestTheme(client_id, "skyprompt_tutorial")) {
                        logger::error("Failed to request skyprompt_tutorial theme.");
                    }
                    ShowTutorial();
                    showing_tutorial.store(true);
                }
            } else if (MCP::is_installed) {
                ReadMenuFrameworkStrings();
                const auto message = Translations::Format(
                    "$SkyPromptTutorialMenuInfo", Translations::Get("$SkyPromptTutorialDeviceKeyboard"), MF_KB_key,
                    Translations::MenuToggleMode(MF_KB_mode), Translations::Get("$SkyPromptTutorialDeviceGamepad"),
                    MF_GP_key, Translations::MenuToggleMode(MF_GP_mode));
                SKSE::GetTaskInterface()->AddTask(
                    [message]() { ShowMessageBox(message, {"$SkyPromptButtonOK"}, [](unsigned int) {}); });
            }
        }

        static void Start() {
            showing_tutorial.store(false);
            ShowMessageBox("$SkyPromptTutorialWelcome", {"$SkyPromptButtonSure", "$SkyPromptButtonPass"}, Callback);
        }

        static void End(const SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {
            RemovePrompt(a_sink, a_clientID);
            showing_tutorial.store(false);
            ShowMessageBox("$SkyPromptTutorialTitle", {"$SkyPromptButtonRestart", "$SkyPromptButtonEnd"}, Callback);
        }

        static void ShowTutorial() {
            MCP::Settings::initialized = true;
            if (!SkyPromptAPI::SendPrompt(Tutorial0::Sink::GetSingleton(), client_id)) {
                logger::error("Failed to Send ShowTutorial prompts.");
            }
        }
    };
};
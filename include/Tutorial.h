#pragma once
#include "Utils.h"
#include "SkyPrompt/API.hpp"
#include "MCP.h"

namespace Tutorial {

    constexpr std::string_view quit_me = "Quit Tutorial";
	inline std::atomic_bool showing_tutorial = false;
	void ReadMenuFrameworkStrings();
	void SwitchToTutorialPos();
	void SwitchBackFromTutorialPos();
    inline std::string MF_KB_key;
	inline std::string MF_KB_mode;
	inline std::string MF_GP_key;
	inline std::string MF_GP_mode;
	inline float old_xpos;
	inline float old_ypos;

	namespace Tutorial3 {

	    inline SkyPromptAPI::ClientID client_id=0;
		inline std::chrono::steady_clock::time_point last_delete_t;

        constexpr std::string_view str1 = "Delete All: Triple Press and Hold the Button!";
        constexpr std::string_view str2 = "Delete All: Triple Press and Hold the Button! ";

		const SkyPromptAPI::Prompt prompt1(str1,0,0,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt2(str2,0,1,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt4(quit_me,1,0,SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

		class Sink final : public SkyPromptAPI::PromptSink,public clib_util::singleton::ISingleton<Sink> {
			std::array<const SkyPromptAPI::Prompt,3> m_prompts = {prompt1,prompt2,prompt4};
		public:
			std::span<const SkyPromptAPI::Prompt> GetPrompts() const override {return m_prompts;}
			void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };

	}

	namespace Tutorial2 {

	    inline SkyPromptAPI::ClientID client_id=0;

        constexpr std::string_view str1 = "Skip to Next: Triple Press the Button!";
        constexpr std::string_view str2 = "Skip to Next: Triple Press the Button! ";

		const SkyPromptAPI::Prompt prompt1(str1,0,0,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt2(str2,0,1,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt4(quit_me,1,0,SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

		class Sink final : public SkyPromptAPI::PromptSink,public clib_util::singleton::ISingleton<Sink> {
			std::array<const SkyPromptAPI::Prompt,3> m_prompts = {prompt1,prompt2,prompt4};


		public:
			std::span<const SkyPromptAPI::Prompt> GetPrompts() const override {return m_prompts;}
			void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };

	}

	namespace Tutorial1 {

	    inline SkyPromptAPI::ClientID client_id=0;
        inline std::chrono::steady_clock::time_point last_delete_t;
		
        constexpr std::string_view str1 = "Delete Me: Double Press the Button!";
        constexpr std::string_view str2 = "Delete Me: Double Press the Button! ";
        constexpr std::string_view quit_me = "Quit Tutorial";

		const SkyPromptAPI::Prompt prompt1(str1,0,0,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt2(str2,0,1,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt4(quit_me,1,0,SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

		class Sink final : public SkyPromptAPI::PromptSink,public clib_util::singleton::ISingleton<Sink> {
			std::array<const SkyPromptAPI::Prompt,3> m_prompts = {prompt1,prompt2,prompt4};


		public:
			std::span<const SkyPromptAPI::Prompt> GetPrompts() const override {return m_prompts;}
			void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };

	};

	namespace Tutorial0 {

	    inline SkyPromptAPI::ClientID client_id=0;
		inline std::chrono::steady_clock::time_point last_delete_t;

        constexpr std::string_view str1 = "Accept Prompt: Hold the Button!";
        constexpr std::string_view str2 = "Accept Prompt: Hold the Button! ";

		const SkyPromptAPI::Prompt prompt1(str1,0,0,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt2(str2,0,1,SkyPromptAPI::PromptType::kHold);
		const SkyPromptAPI::Prompt prompt4(quit_me,1,0,SkyPromptAPI::PromptType::kSinglePress);

        inline std::set<SkyPromptAPI::ActionID> to_be_deleted;

		class Sink final : public SkyPromptAPI::PromptSink,public clib_util::singleton::ISingleton<Sink> {
			std::array<const SkyPromptAPI::Prompt,3> m_prompts = {prompt1,prompt2,prompt4};
		public:
			std::span<const SkyPromptAPI::Prompt> GetPrompts() const override {return m_prompts;}
			void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };

	}

	namespace ModPageImage {

	    inline SkyPromptAPI::ClientID client_id=0;
		inline std::chrono::steady_clock::time_point last_delete_t;

        constexpr std::string_view str1 = "SkyPrompt";

		const SkyPromptAPI::Prompt prompt1(str1,0,0,SkyPromptAPI::PromptType::kHold);

		class Sink final : public SkyPromptAPI::PromptSink,public clib_util::singleton::ISingleton<Sink> {
			std::array<const SkyPromptAPI::Prompt,1> m_prompts = {prompt1};
		public:
			std::span<const SkyPromptAPI::Prompt> GetPrompts() const override {return m_prompts;}
			void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        };

	}

    class Manager : public clib_util::singleton::ISingleton<Manager>
    {
    public:
        static void Callback(const unsigned int a_int) {
		    if (!a_int) {
			    if (Tutorial0::client_id == 0) {
				    Tutorial0::client_id = SkyPromptAPI::RequestClientID();
			    }
			    if (Tutorial0::client_id > 0) {
			        Tutorial::Tutorial0::to_be_deleted = {0,1};
			        Tutorial::Tutorial1::to_be_deleted = {0,1};
			        Tutorial::Tutorial2::to_be_deleted = {0,1};
			        Tutorial::Tutorial3::to_be_deleted = {0,1};		        
			        ShowTutorial();
				    showing_tutorial.store(true);
			    }
			}
            else if (MCP::is_installed) {
				ReadMenuFrameworkStrings();
				std::string message = std::format(
                    "SkyPrompt comes with an In-Game Menu for customization! \n"
                    "To access the menu: \n"
                    "Keyboard - {} ({}) \n"
                    "Gamepad - {} ({}) \n",
                    MF_KB_key, MF_KB_mode,
                    MF_GP_key, MF_GP_mode
                );
				SKSE::GetTaskInterface()->AddTask([message]() {
				    ShowMessageBox(
			        message, 
			        { "Ok" }, [](unsigned int){});
				}
				);
            }
	    }

        static void Start(){
			showing_tutorial.store(false);
		    ShowMessageBox(
			    "Thank you for installing SkyPrompt! \n"
			    "I would like to show you a couple tips on how to handle the prompts!", 
			    { "Sure!","Pass..." }, Callback);
	    }

        static void End(const SkyPromptAPI::PromptSink* a_sink, const SkyPromptAPI::ClientID a_clientID) {
			SkyPromptAPI::RemovePrompt(a_sink,a_clientID);
			showing_tutorial.store(false);
			SwitchBackFromTutorialPos();
            ShowMessageBox(
			    "SkyPrompt Tutorial:", 
			    { "Restart","End" }, Callback);
            
        }

	    static void ShowTutorial() {
			MCP::Settings::initialized = true;
			SwitchToTutorialPos();
			if (!SkyPromptAPI::SendPrompt(Tutorial0::Sink::GetSingleton(),Tutorial0::client_id)) {
				logger::error("Failed to Send ShowTutorial prompts.");
			}
	    }
    };
};
#pragma once
#include "Tutorial.h"
#include "ClibUtil/simpleINI.hpp"

void Tutorial::Tutorial3::Sink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    if (event.prompt.eventID && event.type == SkyPromptAPI::kAccepted) {
        Manager::End(this,client_id);
        return;
    }
    switch (event.type) {
        case SkyPromptAPI::PromptEventType::kAccepted:
            SkyPromptAPI::RemovePrompt(this,client_id);
        case SkyPromptAPI::PromptEventType::kTimeout:
        case SkyPromptAPI::PromptEventType::kRemovedByMod:
        case SkyPromptAPI::PromptEventType::kTimingOut:
            // ReSharper disable once CppNoDiscardExpression
            if (SkyPromptAPI::SendPrompt(this,client_id)) {
                to_be_deleted = { 0,1 };
            }
            return;
        case SkyPromptAPI::PromptEventType::kDeclined:
            to_be_deleted.erase(event.prompt.actionID);
            if (to_be_deleted.size() < 3) {
                const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_delete_t).count();
                if (delta > 200) {
                    SkyPromptAPI::RemovePrompt(this,client_id);
                    if (SkyPromptAPI::SendPrompt(this,client_id)) {
                        to_be_deleted = {0,1};
                    }
                }
            }
            last_delete_t = std::chrono::steady_clock::now();
            to_be_deleted.erase(event.prompt.actionID);
        default:
            break;
    }
    if (to_be_deleted.empty()) {
        SkyPromptAPI::RemovePrompt(this,client_id);
		Tutorial4::Sink::GetSingleton()->Start();
    }
}

void Tutorial::Tutorial4::Sink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    if (event.prompt.eventID && event.type == SkyPromptAPI::kAccepted) {
        Manager::End(this,client_id);
        return;
    }
    switch (event.type) {
        case SkyPromptAPI::PromptEventType::kAccepted:
            SkyPromptAPI::RemovePrompt(this,client_id);
            Tutorial5::Sink::GetSingleton()->Start();
            break;
        case SkyPromptAPI::PromptEventType::kDeclined:
        case SkyPromptAPI::PromptEventType::kTimeout:
        case SkyPromptAPI::PromptEventType::kRemovedByMod:
        case SkyPromptAPI::PromptEventType::kTimingOut:
            GetSingleton()->Start();
        default:
            break;
    }
}

void Tutorial::Tutorial4::Sink::Start()
{
    m_prompts[0].progress = 10.99f;
    if (!SkyPromptAPI::SendPrompt(GetSingleton(),client_id)) {
    }
}

void Tutorial::Tutorial2::Sink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    if (event.prompt.eventID && event.type == SkyPromptAPI::kAccepted) {
        Manager::End(this,client_id);
        return;
    }
    switch (event.type) {
        case SkyPromptAPI::PromptEventType::kAccepted:
        case SkyPromptAPI::PromptEventType::kDeclined:
            SkyPromptAPI::RemovePrompt(this,client_id);
        case SkyPromptAPI::PromptEventType::kTimeout:
        case SkyPromptAPI::PromptEventType::kRemovedByMod:
        case SkyPromptAPI::PromptEventType::kTimingOut:
            // ReSharper disable once CppNoDiscardExpression
            if (SkyPromptAPI::SendPrompt(this,client_id)) {
                to_be_deleted = { 0,1 };
            }
            return;
        default:
            break;
    }
    if (to_be_deleted.empty()) {
        SkyPromptAPI::RemovePrompt(this,client_id);
        if (!SkyPromptAPI::SendPrompt(Tutorial3::Sink::GetSingleton(),client_id)) {
            logger::error("Failed to Send Tutorial3 prompts.");
        }
    }
}

void Tutorial::Tutorial1::Sink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    if (event.prompt.eventID && event.type == SkyPromptAPI::kAccepted) {
        Manager::End(this,client_id);
        return;
    }
    switch (event.type) {
        case SkyPromptAPI::PromptEventType::kAccepted:
            SkyPromptAPI::RemovePrompt(this,client_id);
        case SkyPromptAPI::PromptEventType::kTimeout:
        case SkyPromptAPI::PromptEventType::kRemovedByMod:
        case SkyPromptAPI::PromptEventType::kTimingOut:
            // ReSharper disable once CppNoDiscardExpression
            if (SkyPromptAPI::SendPrompt(this,client_id)) {
                to_be_deleted = { 0,1 };
            }
            return;
        case SkyPromptAPI::PromptEventType::kDeclined:
            if (to_be_deleted.size() < 3) {
                const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_delete_t).count();
                if (delta < 100) {
                    SkyPromptAPI::RemovePrompt(this,client_id);
                    if (SkyPromptAPI::SendPrompt(this,client_id)) {
                        to_be_deleted = {0,1};
                    }
                }
            }
            last_delete_t = std::chrono::steady_clock::now();
            to_be_deleted.erase(event.prompt.actionID);
        default:
            break;
    }
    if (to_be_deleted.empty()) {
        SkyPromptAPI::RemovePrompt(this,client_id);
        if (!SkyPromptAPI::SendPrompt(Tutorial2::Sink::GetSingleton(),client_id)) {
            logger::error("Failed to Send Tutorial2 prompts.");
        }
    }
}

void Tutorial::ReadMenuFrameworkStrings()
{
	CSimpleIniA ini;
	ini.SetUnicode(true);
    if (ini.LoadFile("Data/SKSE/Plugins/SKSEMenuFramework.ini") < 0) {
        logger::error("Failed to load SKSEMenuFramework.ini");
        return;
	}

    std::string temp = "F1";
    MF_KB_key = clib_util::ini::get_value(ini,temp,"General","ToggleKey");
    std::ranges::transform(MF_KB_key, MF_KB_key.begin(),[](const unsigned char c) { return std::toupper(c); });
	std::string temp2 = "SinglePress";
	MF_KB_mode = clib_util::ini::get_value(ini, temp2, "General", "ToggleMode");
	std::string temp3 = "lb";
	MF_GP_key = clib_util::ini::get_value(ini, temp3, "General", "ToggleKeyGamePad");
    std::ranges::transform(MF_GP_key, MF_GP_key.begin(),[](const unsigned char c) { return std::toupper(c); });
	std::string temp4 = "DoublePress";
	MF_GP_mode = clib_util::ini::get_value(ini, temp4, "General", "ToggleModeGamePad");
}

void Tutorial::SwitchToTutorialPos()
{
    old_xpos = MCP::Settings::xPercent;
	old_ypos = MCP::Settings::yPercent;

    const auto [fst, snd] = Presets::OSP::presets.for_level(11);
	MCP::Settings::xPercent = fst;
    MCP::Settings::yPercent = snd;
}

void Tutorial::SwitchBackFromTutorialPos()
{
    MCP::Settings::xPercent = old_xpos;
    MCP::Settings::yPercent = old_ypos;
}

void Tutorial::Tutorial5::Sink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    if (event.prompt.eventID && event.type == SkyPromptAPI::kAccepted) {
        Manager::End(this,client_id);
        return;
    }
    switch (event.type) {
        case SkyPromptAPI::PromptEventType::kAccepted:
			m_prompts[0].progress = event.prompt.progress;
            m_prompts[0].progress += 0.25f;
            if (m_prompts[0].progress > mult+1.f) {
                Manager::End(this,client_id);
            }
            else if (SkyPromptAPI::SendPrompt(GetSingleton(),client_id)) {
            }
            break;
        case SkyPromptAPI::PromptEventType::kTimeout:
        case SkyPromptAPI::PromptEventType::kRemovedByMod:
        case SkyPromptAPI::PromptEventType::kTimingOut:
            if (!SkyPromptAPI::SendPrompt(GetSingleton(),client_id)) {
            }
        default:
            break;
    }
}

void Tutorial::Tutorial5::Sink::Start() const {
    m_prompts[0].progress = mult+.99f;
    if (!SkyPromptAPI::SendPrompt(GetSingleton(),client_id)) {
		logger::error("Failed to send Tutorial4 QTE prompt.");
    }
}

void Tutorial::Tutorial0::Sink::ProcessEvent(const SkyPromptAPI::PromptEvent event) const {
    if (event.prompt.eventID && event.type == SkyPromptAPI::kAccepted) {
        Manager::End(this,client_id);
        return;
    }
    switch (event.type) {
        case SkyPromptAPI::PromptEventType::kDeclined:
            SkyPromptAPI::RemovePrompt(this,client_id);
        case SkyPromptAPI::PromptEventType::kTimeout:
        case SkyPromptAPI::PromptEventType::kRemovedByMod:
        case SkyPromptAPI::PromptEventType::kTimingOut:
            // ReSharper disable once CppNoDiscardExpression
            if (SkyPromptAPI::SendPrompt(this,client_id)) {
                to_be_deleted = { 0,1 };
            }
            return;
        case SkyPromptAPI::PromptEventType::kAccepted:
            to_be_deleted.erase(event.prompt.actionID);
        default:
            break;
    }
    if (to_be_deleted.empty()) {
        SkyPromptAPI::RemovePrompt(this,client_id);
        if (!SkyPromptAPI::SendPrompt(Tutorial1::Sink::GetSingleton(),client_id)) {
            logger::error("Failed to Send Tutorial1 prompts.");
        }
    }
}

#include "Bindings.h"
#include "Sinks.h"
#include "ClibUtil/editorID.hpp"

namespace {
    std::unordered_map<RE::FormID, std::pair<SkyPromptAPI::ClientID, bool>> registeredClients;

    bool SendPrompt(RE::StaticFunctionTag*, SkyPromptAPI::ClientID clientID, std::string text,
                    SkyPromptAPI::EventID eventID,
                    SkyPromptAPI::ActionID actionID, SkyPromptAPI::PromptType type, RE::TESForm* refForm,
                    RE::BSTArray<uint32_t> devices, RE::BSTArray<uint32_t> keys, float progress) {
        if (devices.size() != keys.size()) return false;
        std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> bindings;
        for (RE::BSTArray<uint32_t>::size_type i = 0; i < devices.size(); ++i) {
            auto a_device = static_cast<RE::INPUT_DEVICE>(devices[i]);
            auto a_key = keys[i];
            bindings.emplace_back(a_device, a_key);
        }

        if (PapyrusAPI::AddPrompt(clientID, text, eventID, actionID, type, refForm, bindings, progress)) {
            std::shared_lock lock(PapyrusAPI::mutex_);
            if (const auto it = PapyrusAPI::papyrusSinks.find(clientID); it != PapyrusAPI::papyrusSinks.end()) {
                auto& sinks = it->second;
                if (const auto a_sink = sinks.find({eventID, actionID}); a_sink != sinks.end()) {
                    return SkyPromptAPI::SendPrompt(a_sink->second.get(), clientID);
                }
            }
        }
        return false;
    }

    bool SendPromptForControl(RE::StaticFunctionTag*, SkyPromptAPI::ClientID clientID, std::string text,
                              SkyPromptAPI::EventID eventID, SkyPromptAPI::ActionID actionID,
                              SkyPromptAPI::PromptType type, RE::TESForm* refForm,
                              std::string a_controlName, int a_contextID, float progress) {
        constexpr std::array devices = {RE::INPUT_DEVICE::kKeyboard, RE::INPUT_DEVICE::kMouse,
                                        RE::INPUT_DEVICE::kGamepad};

        std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> bindings;
        for (size_t i = 0; i < devices.size(); ++i) {
            auto a_device = devices[i];
            auto a_key = RE::ControlMap::GetSingleton()->GetMappedKey(a_controlName, a_device,
                                                                      static_cast<RE::ControlMap::InputContextID>(
                                                                          a_contextID));
            if (a_key == RE::ControlMap::kInvalid) {
                continue;
            }
            bindings.emplace_back(a_device, a_key);
        }

        if (PapyrusAPI::AddPrompt(clientID, text, eventID, actionID, type, refForm, bindings, progress)) {
            std::shared_lock lock(PapyrusAPI::mutex_);
            if (const auto it = PapyrusAPI::papyrusSinks.find(clientID); it != PapyrusAPI::papyrusSinks.end()) {
                auto& sinks = it->second;
                if (const auto a_sink = sinks.find({eventID, actionID}); a_sink != sinks.end()) {
                    return SkyPromptAPI::SendPrompt(a_sink->second.get(), clientID);
                }
            }
        }
        return false;
    }

    void RemovePrompt(RE::StaticFunctionTag*, SkyPromptAPI::ClientID clientID, SkyPromptAPI::EventID eventID,
                      SkyPromptAPI::ActionID actionID) {
        std::shared_lock lock(PapyrusAPI::mutex_);
        if (const auto it = PapyrusAPI::papyrusSinks.find(clientID); it != PapyrusAPI::papyrusSinks.end()) {
            auto& sinks = it->second;
            if (const auto a_sink = sinks.find({eventID, actionID}); a_sink != sinks.end()) {
                SkyPromptAPI::RemovePrompt(a_sink->second.get(), clientID);
                lock.unlock();
                std::unique_lock lock2(PapyrusAPI::mutex_);
                sinks.erase(a_sink);
            }
        }
    }

    SkyPromptAPI::ClientID RegisterForSkyPromptEvent(RE::StaticFunctionTag*, RE::TESForm* a_form, int a_major,
                                                     int a_minor) {
        if (!a_form || a_form->IsDynamicForm()) {
            return 0;
        }

        if (a_major != SkyPromptAPI::MAJOR) {
            logger::error("API version mismatch. SkyPrompt: {}.{}, Papyrus Quest {}: {}.{}",
                          SkyPromptAPI::MAJOR, SkyPromptAPI::MINOR, clib_util::editorID::get_editorID(a_form), a_major,
                          a_minor);
            return 0;
        }

        const auto a_formID = a_form->GetFormID();
        if (std::unique_lock lock(PapyrusAPI::mutex_); registeredClients.contains(a_formID)) {
            if (auto& [a_ID, is_registered] = registeredClients.at(a_formID);
                is_registered || PapyrusAPI::skyPromptEvents.Register(a_form)) {
                is_registered = true;
                return a_ID;
            }
            return 0;
        }

        if (PapyrusAPI::skyPromptEvents.Register(a_form)) {
            const auto a_clientID = SkyPromptAPI::RequestClientID();
            if (a_clientID == 0) {
                PapyrusAPI::skyPromptEvents.Unregister(a_formID);
                return 0;
            }
            {
                std::unique_lock lock(PapyrusAPI::mutex_);
                registeredClients[a_formID] = {a_clientID, true};
            }
            return a_clientID;
        }

        return 0;
    }

    bool UnregisterFromSkyPromptEvent(RE::StaticFunctionTag*, RE::TESForm* a_form) {
        if (!a_form) {
            return false;
        }
        const auto a_formID = a_form->GetFormID();

        {
            std::shared_lock lock(PapyrusAPI::mutex_);
            if (!registeredClients.contains(a_formID)) {
                return false;
            }
        }

        std::unique_lock lock(PapyrusAPI::mutex_);
        if (auto& [a_ID, is_registered] = registeredClients.at(a_formID); is_registered) {
            if (PapyrusAPI::skyPromptEvents.Unregister(a_formID)) {
                is_registered = false;
                return true;
            }
        }
        return false;
    }

    bool RequestTheme(RE::StaticFunctionTag*, SkyPromptAPI::ClientID clientID, std::string theme_name) {
        return SkyPromptAPI::RequestTheme(clientID, theme_name);
    }
}

bool PapyrusAPI::Register(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("RegisterForSkyPromptEvent", "SkyPrompt", RegisterForSkyPromptEvent);
    vm->RegisterFunction("UnregisterFromSkyPromptEvent", "SkyPrompt", UnregisterFromSkyPromptEvent);
    vm->RegisterFunction("SendPrompt", "SkyPrompt", SendPrompt);
    vm->RegisterFunction("SendPromptForControl", "SkyPrompt", SendPromptForControl);
    vm->RegisterFunction("RemovePrompt", "SkyPrompt", RemovePrompt);
    vm->RegisterFunction("RequestTheme", "SkyPrompt", RequestTheme);

    return true;
}
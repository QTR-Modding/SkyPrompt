#include "Bindings.h"
#include "Sinks.h"

namespace {

    std::unordered_set<SkyPromptAPI::ClientID> registeredClients;

    int RegisterForSkyPrompt(RE::StaticFunctionTag*) {
        const std::uint16_t clientID = SkyPromptAPI::RequestClientID();
        if (clientID == 0) {
            return 0;
        }
        if (registeredClients.contains(clientID)) {
            return clientID;
        }
        registeredClients.insert(clientID);
        return clientID;
    }

    bool SendPrompt(RE::StaticFunctionTag*, const SkyPromptAPI::ClientID clientID, std::string text, const SkyPromptAPI::EventID eventID,
                    const SkyPromptAPI::ActionID actionID, const SkyPromptAPI::PromptType type, RE::TESForm* refForm,
        RE::BSTArray<uint32_t> devices, RE::BSTArray<uint32_t> keys) {

        if (devices.size() != keys.size()) return false;
        std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> bindings;
        for (RE::BSTArray<uint32_t>::size_type i = 0; i < devices.size(); ++i) {
			auto a_device = static_cast<RE::INPUT_DEVICE>(devices[i]);
			auto a_key = static_cast<SkyPromptAPI::ButtonID>(keys[i]);
            bindings.emplace_back(a_device, a_key);
        }

		if (PapyrusAPI::AddPrompt(clientID, text, eventID, actionID, type, refForm, bindings)) {
			std::shared_lock lock(PapyrusAPI::mutex_);
			if (const auto it = PapyrusAPI::papyrusSinks.find(clientID); it != PapyrusAPI::papyrusSinks.end()) {
				auto& sinks = it->second;
				if (const auto a_sink = sinks.find({ eventID, actionID }); a_sink != sinks.end()) {
			        return SkyPromptAPI::SendPrompt(a_sink->second.get(), clientID);
				}
			}
		}
		return false;
    }

    void RemovePrompt(RE::StaticFunctionTag*, const SkyPromptAPI::ClientID clientID, const SkyPromptAPI::EventID eventID, const SkyPromptAPI::ActionID actionID) {
		std::shared_lock lock(PapyrusAPI::mutex_);
		if (const auto it = PapyrusAPI::papyrusSinks.find(clientID); it != PapyrusAPI::papyrusSinks.end()) {
			auto& sinks = it->second;
			if (const auto a_sink = sinks.find({ eventID, actionID }); a_sink != sinks.end()) {
				SkyPromptAPI::RemovePrompt(a_sink->second.get(), clientID);
				lock.unlock();
				std::unique_lock lock2(PapyrusAPI::mutex_);
				sinks.erase(a_sink);
			}
		}
    }

    void RegisterForSkyPromptEvent(RE::StaticFunctionTag*, RE::TESForm* a_form) {
        if (!a_form) {
            return;
        }
        PapyrusAPI::skyPromptEvents.Register(a_form);
    }

    void UnregisterFromSkyPromptEvent(RE::StaticFunctionTag*, RE::TESForm* a_form) {
        if (!a_form) {
            return;
        }
        PapyrusAPI::skyPromptEvents.Unregister(a_form);
    }
}

bool PapyrusAPI::Register(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("RegisterForSkyPrompt", "SkyPrompt", RegisterForSkyPrompt);
    vm->RegisterFunction("SendPrompt", "SkyPrompt", SendPrompt);
    vm->RegisterFunction("RemovePrompt", "SkyPrompt", RemovePrompt);
    vm->RegisterFunction("RegisterForSkyPromptEvent", "SkyPrompt", RegisterForSkyPromptEvent);
    vm->RegisterFunction("UnregisterFromSkyPromptEvent", "SkyPrompt", UnregisterFromSkyPromptEvent);

    return true;
}

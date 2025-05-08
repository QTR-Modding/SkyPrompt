#include "Bindings.h"
#include "Sinks.h"

namespace {

    std::unordered_set<SkyPromptAPI::ClientID> registeredClients;

    SkyPromptAPI::ClientID RequestClientID(RE::StaticFunctionTag*) {
        return SkyPromptAPI::RequestClientID();
    }

    void RegisterForSkyPrompt(RE::StaticFunctionTag*, const std::uint16_t clientID) {
        registeredClients.insert(clientID);
    }

    void UnregisterFromSkyPrompt(RE::StaticFunctionTag*, const std::uint16_t clientID) {
        registeredClients.erase(clientID);
    }

    bool SendPrompt(RE::StaticFunctionTag*, const SkyPromptAPI::ClientID clientID, std::string text, const SkyPromptAPI::EventID eventID,
                    const SkyPromptAPI::ActionID actionID, const SkyPromptAPI::PromptType type, RE::TESForm* refForm,
        RE::BSTArray<RE::INPUT_DEVICE> devices, RE::BSTArray<SkyPromptAPI::ButtonID> keys) {

        if (devices.size() != keys.size()) return false;
        std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> bindings;
        for (RE::BSTArray<uint32_t>::size_type i = 0; i < devices.size(); ++i) {
            bindings.emplace_back(devices[i], keys[i]);
        }

		if (const auto sink = PapyrusAPI::AddPrompt(clientID, text, eventID, actionID, type, refForm, bindings.empty() ? nullptr : &bindings)) {
            sink->prompt.button_key = bindings;
			return SkyPromptAPI::SendPrompt(sink, clientID);
		}
		return false;
    }

    void RemovePrompt(RE::StaticFunctionTag*, const SkyPromptAPI::ClientID clientID, const SkyPromptAPI::EventID eventID, const SkyPromptAPI::ActionID actionID) {
		if (const auto a_sink = PapyrusAPI::GetPrompt(clientID, eventID, actionID)) {
            SkyPromptAPI::RemovePrompt(a_sink,clientID);
		}
    }

}

bool PapyrusAPI::Register(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("RequestClientID", "SkyPrompt", RequestClientID);
    vm->RegisterFunction("RegisterForSkyPrompt", "SkyPrompt", RegisterForSkyPrompt);
    vm->RegisterFunction("UnregisterFromSkyPrompt", "SkyPrompt", UnregisterFromSkyPrompt);
    vm->RegisterFunction("SendPrompt", "SkyPrompt", SendPrompt);
	vm->RegisterFunction("RemovePrompt", "SkyPrompt", RemovePrompt);
    return true;
}

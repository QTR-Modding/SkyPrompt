#include "MCP.h"
#include "Utils.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Hooks.h"
#include "Settings.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"

static void HelpMarker(const char* desc) {
    MCP_API::TextDisabled("(?)");
    if (MCP_API::BeginItemTooltip()) {
        MCP_API::PushTextWrapPos(MCP_API::GetFontSize() * 35.0f);
        MCP_API::TextUnformatted(desc);
        MCP_API::PopTextWrapPos();
        MCP_API::EndTooltip();
    }
}

void __stdcall MCP::RenderSettings()
{
    bool settingsChanged = false; // Flag to track changes

    // Checkbox for enable/disable mod
    bool enabled = Settings::initialized.load();
    if (MCP_API::Checkbox("Enable Mod", &enabled)) {
        Settings::initialized.store(enabled);
    }
#ifndef NDEBUG
	// Checkbox for debug mode
	MCP_API::SameLine();
	MCP_API::Checkbox("Draw Debug", &Settings::draw_debug);
#endif

    // Slider for fade speed
    if (!MCP_API::SliderFloat("Fade Speed", &Settings::fadeSpeed, 0.0f, 0.1f)) {
        if (MCP_API::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for X Percent
    if (!MCP_API::SliderFloat("X Percent", &Settings::xPercent, 0.0f, 1.0f)) {
        if (MCP_API::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for Y Percent
    if (!MCP_API::SliderFloat("Y Percent", &Settings::yPercent, 0.0f, 1.0f)) {
        if (MCP_API::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for Prompt Size
    if (!MCP_API::SliderFloat("Prompt Size", &Settings::prompt_size, 15.0f, 100.0f)) {
        if (MCP_API::IsItemDeactivatedAfterEdit()) {
            Settings::shouldReloadPromptSize.store(true);
            settingsChanged = true;
        }
    }

	// Slider for Icon2Font Ratio
	if (!MCP_API::SliderFloat("Icon2Font Ratio", &Settings::icon2font_ratio, 0.5f, 2.0f)) {
		if (MCP_API::IsItemDeactivatedAfterEdit()) {
		    Settings::shouldReloadPromptSize.store(true);
	        settingsChanged = true;
		}
	}

	// Slider for Line Spacing
	if (!MCP_API::SliderFloat("Line Spacing", &Settings::linespacing, 0.0f, 1.0f)) {
		if (MCP_API::IsItemDeactivatedAfterEdit()) {
			settingsChanged = true;
		}
	}

    // Slider for Progress Speed
    if (!MCP_API::SliderFloat("Progress Speed", &Settings::progress_speed, 0.0f, 1.0f)) {
        if (MCP_API::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for Lifetime
    if (!MCP_API::SliderFloat("Lifetime", &Settings::lifetime, 1.0f, 30.0f)) {
        if (MCP_API::IsItemDeactivatedAfterEdit()) {
            Settings::shouldReloadLifetime.store(true);
            settingsChanged = true;
        }
        
    }

    if (settingsChanged) {
		Settings::to_json();
    }
}



void __stdcall MCP::RenderLog()
{
#ifndef NDEBUG
    MCP_API::Checkbox("Trace", &LogSettings::log_trace);
#endif
    MCP_API::SameLine();
    MCP_API::Checkbox("Info", &LogSettings::log_info);
    MCP_API::SameLine();
    MCP_API::Checkbox("Warning", &LogSettings::log_warning);
    MCP_API::SameLine();
    MCP_API::Checkbox("Error", &LogSettings::log_error);

    // if "Generate Log" button is pressed, read the log file
    if (MCP_API::Button("Generate Log")) logLines = ReadLogFile();

    // Display each line in a new ImGui::Text() element
    for (const auto& line : logLines) {
        if (!LogSettings::log_trace && line.find("trace") != std::string::npos) continue;
        if (!LogSettings::log_info && line.find("info") != std::string::npos) continue;
        if (!LogSettings::log_warning && line.find("warning") != std::string::npos) continue;
        if (!LogSettings::log_error && line.find("error") != std::string::npos) continue;
        MCP_API::Text(line.c_str());
    }
}

void MCP::Register()
{
	if (!SKSEMenuFramework::IsInstalled()) {
        return;
    }

	log_path = GetLogPath().string();

    SKSEMenuFramework::SetSection(mod_name);
    SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);
	SKSEMenuFramework::AddSectionItem("Controls", RenderControls);
	SKSEMenuFramework::AddSectionItem("Log", RenderLog);
}

bool MCP::Settings::IsEnabled(const Input::DEVICE a_device)
{
	if (enabled_devices.contains(a_device)) {
		if (const auto gamepad_type = RE::ControlMap::GetSingleton()->GetGamePadType(); 
			gamepad_type == RE::PC_GAMEPAD_TYPE::kOrbis) {
			if (a_device == Input::DEVICE::kGamepadDirectX) {
				return false;
			}
		}
		else if (gamepad_type == RE::PC_GAMEPAD_TYPE::kDirectX) {
			if (a_device == Input::DEVICE::kGamepadOrbis) {
				return false;
			}
		}
		else if (gamepad_type == RE::PC_GAMEPAD_TYPE::kTotal) {
			if (a_device == Input::DEVICE::kGamepadDirectX || a_device == Input::DEVICE::kGamepadOrbis) {
				return false;
			}
		}
		return enabled_devices.at(a_device);
	}
	return false;
}

void MCP::Settings::to_json()
{
	using namespace rapidjson;

	Document doc;
	doc.SetObject();

	Document::AllocatorType& allocator = doc.GetAllocator();

	Value root(kObjectType);

	root.AddMember("fadeSpeed", fadeSpeed, allocator);
	root.AddMember("xPercent", xPercent, allocator);
	root.AddMember("yPercent", yPercent, allocator);
	root.AddMember("prompt_size", prompt_size, allocator);
	root.AddMember("icon2font_ratio", icon2font_ratio, allocator);
	root.AddMember("linespacing", linespacing, allocator);
	root.AddMember("progress_speed", progress_speed, allocator);
	root.AddMember("lifetime", lifetime, allocator);

	// special commands
	Value special_commands(kObjectType);
	special_commands.AddMember("visualize", SpecialCommands::visualize, allocator);
	special_commands.AddMember("responsiveness", SpecialCommands::responsiveness, allocator);
	root.AddMember("special_commands", special_commands, allocator);

	// enabled devices
	Value enabled_devices_(kObjectType);
	for (const auto& [device, enabled] : enabled_devices) {
		const auto device_str = device_to_string(device);
		Value device_json(device_str.c_str(), allocator); // Convert std::string to RapidJSON string
		enabled_devices_.AddMember(device_json, enabled, allocator);
	}
	root.AddMember("enabled_devices", enabled_devices_, allocator);

	// n_max_buttons
	root.AddMember("n_max_buttons", n_max_buttons, allocator);

	// keys
	Value prompt_keys_json(kObjectType);
	for (const auto& [device, keys] : prompt_keys) {
		const auto device_str = device_to_string(device);
		// need array of keys for each device
		Value device_keys(kArrayType);
		for (const auto key : keys) {
			device_keys.PushBack(key, allocator);
		}
		Value device_json(device_str.c_str(), allocator); // Convert std::string to RapidJSON string
		prompt_keys_json.AddMember(device_json, device_keys, allocator);
	}
	root.AddMember("keys", prompt_keys_json, allocator);

	// version

	Value version(kObjectType);
	auto plugin_version = SKSE::PluginDeclaration::GetSingleton()->GetVersion();
    version.AddMember("major", plugin_version.major(), allocator);
    version.AddMember("minor", plugin_version.minor(), allocator);
    version.AddMember("patch", plugin_version.patch(), allocator);
    version.AddMember("build", plugin_version.build(), allocator);

    root.AddMember("version", version, allocator);

	doc.AddMember("MCP", root, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	doc.Accept(writer);

	// save to mod folder
	std::ofstream file(mod_folder + "settings.json");
	file << buffer.GetString();
	file.close();
}

void MCP::Settings::from_json()
{
	auto json = mod_folder + "settings.json";
	if (!std::filesystem::exists(json)) {
		return to_json();
	}
	std::ifstream file(json);
	std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	rapidjson::Document doc;
	doc.Parse(str.c_str());
	if (doc.HasParseError()) {
		logger::error("Failed to parse settings.json");
		return;
	}
	if (!doc.HasMember("MCP")) {
		logger::error("Failed to find MCP in settings.json");
		return;
	}
	auto& mcp = doc["MCP"];
	if (mcp.HasMember("fadeSpeed")) {
		fadeSpeed = mcp["fadeSpeed"].GetFloat();
	}
	if (mcp.HasMember("xPercent")) {
		xPercent = mcp["xPercent"].GetFloat();
	}
	if (mcp.HasMember("yPercent")) {
		yPercent = mcp["yPercent"].GetFloat();
	}
	if (mcp.HasMember("prompt_size")) {
		prompt_size = mcp["prompt_size"].GetFloat();
	}
	if (mcp.HasMember("icon2font_ratio")) {
		icon2font_ratio = mcp["icon2font_ratio"].GetFloat();
	}
	if (mcp.HasMember("linespacing")) {
		linespacing = mcp["linespacing"].GetFloat();
	}
	if (mcp.HasMember("progress_speed")) {
		progress_speed = mcp["progress_speed"].GetFloat();
	}
	if (mcp.HasMember("lifetime")) {
		lifetime = mcp["lifetime"].GetFloat();
	}

	// enabled devices
	if (mcp.HasMember("enabled_devices")) {
		auto& enabled_devices_ = mcp["enabled_devices"];
		for (auto it = enabled_devices_.MemberBegin(); it != enabled_devices_.MemberEnd(); ++it) {
			const auto device = Input::from_string_to_device(it->name.GetString());
			if (device == Input::DEVICE::kUnknown) {
				logger::error("Unknown device in settings.json");
				continue;
			}
			const auto enabled = it->value.GetBool();
			if (enabled_devices.contains(device)) {
				enabled_devices.at(device) = enabled;
			}
		}
	}

	// n_max_buttons
	if (mcp.HasMember("n_max_buttons")) {
		n_max_buttons = mcp["n_max_buttons"].GetInt();
	}

	// prompt keys
	if (mcp.HasMember("keys")) {
		auto& prompt_keys_json = mcp["keys"];
		for (auto it = prompt_keys_json.MemberBegin(); it != prompt_keys_json.MemberEnd(); ++it) {
			const auto device = Input::from_string_to_device(it->name.GetString());
			if (device == Input::DEVICE::kUnknown) {
				logger::error("Unknown device in settings.json");
				continue;
			}
			std::vector<uint32_t> keys;
			if (it->value.IsArray()) {
			    for (auto& key : it->value.GetArray()) {
				    keys.push_back(key.GetUint());
			    }
			    if (prompt_keys.contains(device)) {
				    prompt_keys.at(device) = keys;
			    }
			}
		}
	}

	// special commands
	if (mcp.HasMember("special_commands")) {
		auto& special_commands = mcp["special_commands"];
		if (special_commands.HasMember("visualize")) {
			SpecialCommands::visualize = special_commands["visualize"].GetBool();
		}
		if (special_commands.HasMember("responsiveness")) {
			SpecialCommands::responsiveness = special_commands["responsiveness"].GetFloat();
		}
	}

}

namespace {
    void ControlBox(const char* label, const Input::DEVICE selected_device, uint32_t& selected_key)
    {

	     //dropdown with keys for selected device
	    const auto device_str = std::string(device_to_string(selected_device));
	    const auto converted_key = Input::Manager::Convert(selected_key, selected_device);
	    if (MCP_API::BeginCombo(label, SKSE::InputMap::GetKeyName(converted_key).c_str())) {
		    for (const auto& key_code : Input::Manager::GetKeys(selected_device)) {
			    const auto converted_keycode = Input::Manager::Convert(key_code, selected_device);
			    const auto key_name = SKSE::InputMap::GetKeyName(converted_keycode);
			    if (key_name.empty()) {
				    continue;
			    }
			    const bool isSelected = converted_key == converted_keycode;
			    if (MCP_API::Selectable((key_name+std::format("##{}",converted_key)).c_str(), isSelected)) {
				    if (!isSelected) {
					    selected_key = key_code;
				    }
			    }
			    if (isSelected) MCP_API::SetItemDefaultFocus();
		    }
		    MCP_API::EndCombo();
	    }

    }

    void DeviceBox(const char* label)
    {
		size_t index = 0; 
		while (!MCP::Settings::IsEnabled(MCP::current_device)) {
			auto it = MCP::Settings::prompt_keys.begin();
			std::advance(it, index);
			if (it == MCP::Settings::prompt_keys.end()) {
				MCP::current_device = Input::DEVICE::kUnknown;
				break;
			}
			MCP::current_device = it->first;
			++index;
		}
	    if (MCP_API::BeginCombo(label, device_to_string(MCP::current_device).data())) {
		    for (const auto& device : MCP::Settings::prompt_keys | std::views::keys) {
				if (!MCP::Settings::IsEnabled(device)) {
					continue;
				}
			    const bool isSelected = MCP::current_device == device;
			    if (MCP_API::Selectable(device_to_string(device).data(), isSelected)) {
				    if (!isSelected) {
                        MCP::current_device = device;
				    }
			    }
			    if (isSelected) MCP_API::SetItemDefaultFocus();
		    }
		    MCP_API::EndCombo();
	    }
    }

    void RenderControl(std::map<Input::DEVICE, uint32_t>& curr_controls, const char* label, const char* help=nullptr)
    {
	    MCP_API::Text(label);
	    MCP_API::SameLine();
	    MCP_API::SetCursorPosX(200.f);
	    MCP_API::SetNextItemWidth(MCP_API::GetWindowWidth() * 0.30f);
	    ControlBox(("##"+std::string(label)).c_str(), MCP::current_device, curr_controls.at(MCP::current_device));
		if (help) {
		    MCP_API::SameLine();    
            HelpMarker(help);
		}
    }
};

void __stdcall MCP::RenderControls()
{
	// Checkbox for each device
	bool settingsChanged = false;
	for (const auto& device : Settings::enabled_devices | std::views::keys) {
		const auto device_str = device_to_string(device);
		if (MCP_API::Checkbox((device_str+"##enabled").c_str(), &Settings::enabled_devices.at(device))) {
			settingsChanged = true;
		}
		if (device != Settings::enabled_devices.rbegin()->first) {
		    MCP_API::SameLine();
		}
	}

	// need max number of buttons slider
	if (!MCP_API::SliderInt("Max Buttons", &Settings::n_max_buttons, 1, 4)) {
		if (MCP_API::IsItemDeactivatedAfterEdit()) {
			settingsChanged = true;
		}
	}

	const auto prompt_keys_before = Settings::prompt_keys;


	MCP_API::Text("Device Selection:");
	MCP_API::SameLine();
	MCP_API::SetCursorPosX(200.f);
    MCP_API::SetNextItemWidth(MCP_API::GetWindowWidth() * 0.25f);
	DeviceBox("##device_selection");

	if (current_device != Input::DEVICE::kUnknown) {
	    for (auto i = 0; i < Settings::n_max_buttons; i++) {
		    std::map<Input::DEVICE,uint32_t> curr_controls;
		    for (const auto& [device, key] : Settings::prompt_keys) {
			    curr_controls[device] = key.at(i);
		    }
		    RenderControl(curr_controls, std::format("Button {}", i+1).c_str());
		    for (auto& [device, key] : Settings::prompt_keys) {
			    key.at(i) = curr_controls[device];
		    }
	    }
	}


	if (settingsChanged || prompt_keys_before != Settings::prompt_keys) {
		Settings::to_json();
	}

	MCP_API::Text("");
	Settings::SpecialCommands::Render();
}

void MCP::Settings::SpecialCommands::Render()
{
	// double press: delete current prompt
	// triple press: cycle through prompts
	// triple press and hold: delete all prompts
	// explain what special commands are

	MCP_API::Text("Special Commands");
	MCP_API::SameLine();
	HelpMarker("Double press: delete current prompt\nTriple press: cycle through prompts\nTriple press and hold: delete all prompts");

	if (MCP_API::Checkbox("Visualize", &visualize)) {
		to_json();
	}
	MCP_API::SameLine();
	HelpMarker("Visualization of the special commands");

	if (!MCP_API::SliderFloat("Responsiveness", &responsiveness, 0.0f, 1.0f)) {
		if (MCP_API::IsItemDeactivatedAfterEdit()) {
			to_json();
            ImGui::Renderer::UpdateMaxIntervalBetweenPresses();
		}
	}
	MCP_API::SameLine();
	HelpMarker("Higher values gives you less time to press the next key in return for faster response");

}


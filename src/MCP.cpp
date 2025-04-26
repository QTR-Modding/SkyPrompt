#include "MCP.h"
#include "Utils.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "Hooks.h"
#include "Settings.h"


static void HelpMarker(const char* desc) {
    ImGuiMCP::TextDisabled("(?)");
    if (ImGuiMCP::BeginItemTooltip()) {
        ImGuiMCP::PushTextWrapPos(ImGuiMCP::GetFontSize() * 35.0f);
        ImGuiMCP::TextUnformatted(desc);
        ImGuiMCP::PopTextWrapPos();
        ImGuiMCP::EndTooltip();
    }
}

void __stdcall MCP::RenderSettings()
{
    bool settingsChanged = false; // Flag to track changes

    // Checkbox for enable/disable mod
    bool enabled = Settings::initialized.load();
    if (ImGuiMCP::Checkbox("Enable Mod", &enabled)) {
        Settings::initialized.store(enabled);
    }
#ifndef NDEBUG
	// Checkbox for debug mode
	ImGuiMCP::SameLine();
	ImGuiMCP::Checkbox("Draw Debug", &Settings::draw_debug);
#endif

	// Checkbox for multi-threading
	bool multithreaded = Settings::multi_threaded.load();
	if (ImGuiMCP::Checkbox("Multi-threaded", &multithreaded )) {
		Settings::multi_threaded.store(multithreaded);
    }

    // Slider for fade speed
    if (!ImGuiMCP::SliderFloat("Fade Speed", &Settings::fadeSpeed, 0.0f, 0.1f)) {
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for X Percent
    if (!ImGuiMCP::SliderFloat("X Percent", &Settings::xPercent, 0.0f, 1.0f)) {
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for Y Percent
    if (!ImGuiMCP::SliderFloat("Y Percent", &Settings::yPercent, 0.0f, 1.0f)) {
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for Prompt Size
    if (!ImGuiMCP::SliderFloat("Prompt Size", &Settings::prompt_size, 15.0f, 100.0f)) {
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::shouldReloadPromptSize.store(true);
            settingsChanged = true;
        }
    }

	// Slider for Icon2Font Ratio
	if (!ImGuiMCP::SliderFloat("Icon2Font Ratio", &Settings::icon2font_ratio, 0.5f, 2.0f)) {
		if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
		    Settings::shouldReloadPromptSize.store(true);
	        settingsChanged = true;
		}
	}

    // Slider for Progress Speed
    if (!ImGuiMCP::SliderFloat("Progress Speed", &Settings::progress_speed, 0.0f, 1.0f)) {
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) settingsChanged = true;
    }

    // Slider for Lifetime
    if (!ImGuiMCP::SliderFloat("Lifetime", &Settings::lifetime, 1.0f, 30.0f)) {
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::shouldReloadLifetime.store(true);
            settingsChanged = true;
        }
        
    }

    // Combo box for update speed
    if (ImGuiMCP::BeginCombo("Update Speed", to_string(Settings::update_speed).c_str())) {
        for (int i = 0; i < static_cast<int>(Settings::UpdateSpeed::kTotal); i++) {
            const bool isSelected = Settings::update_speed == static_cast<Settings::UpdateSpeed>(i);
            if (ImGuiMCP::Selectable(to_string(static_cast<Settings::UpdateSpeed>(i)).c_str(), isSelected)) {
                if (!isSelected) {
                    Settings::update_speed = static_cast<Settings::UpdateSpeed>(i);
                    Settings::update_interval = to_seconds(Settings::update_speed);
                    settingsChanged = true;
                }
            }
            if (isSelected) ImGuiMCP::SetItemDefaultFocus();
        }
        ImGuiMCP::EndCombo();
    }

    if (settingsChanged) {
		Settings::to_json();
    }
}



void __stdcall MCP::RenderLog()
{
#ifndef NDEBUG
    ImGuiMCP::Checkbox("Trace", &LogSettings::log_trace);
#endif
    ImGuiMCP::SameLine();
    ImGuiMCP::Checkbox("Info", &LogSettings::log_info);
    ImGuiMCP::SameLine();
    ImGuiMCP::Checkbox("Warning", &LogSettings::log_warning);
    ImGuiMCP::SameLine();
    ImGuiMCP::Checkbox("Error", &LogSettings::log_error);

    // if "Generate Log" button is pressed, read the log file
    if (ImGuiMCP::Button("Generate Log")) logLines = ReadLogFile();

    // Display each line in a new ImGui::Text() element
    for (const auto& line : logLines) {
        if (!LogSettings::log_trace && line.find("trace") != std::string::npos) continue;
        if (!LogSettings::log_info && line.find("info") != std::string::npos) continue;
        if (!LogSettings::log_warning && line.find("warning") != std::string::npos) continue;
        if (!LogSettings::log_error && line.find("error") != std::string::npos) continue;
        ImGuiMCP::Text(line.c_str());
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


float MCP::Settings::to_seconds(const UpdateSpeed a_speed) {
    switch (a_speed) {
        case kSlow:
            return 2.f;
        case kNormal:
            return 1.f;
        case kFast:
            return 0.5f;
        case kFaster:
            return 0.3f;
        case kFastest:
            return 0.1f;
        case kTotal:
			return 0.f;
        default:
            return 1.f;
    }
}

std::string MCP::Settings::to_string(const UpdateSpeed a_speed)
{
	switch (a_speed) {
	case kSlow:
		return "Slow";
	case kNormal:
		return "Normal";
	case kFast:
		return "Fast";
	case kFaster:
		return "Faster";
	case kFastest:
		return "Fastest";
#ifndef NDEBUG
	case UpdateSpeed::kTotal:
		return "Every Frame";
#endif
	default:
		return "Unknown";
	}
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
	root.AddMember("progress_speed", progress_speed, allocator);
	root.AddMember("lifetime", lifetime, allocator);
	root.AddMember("update_speed", static_cast<int>(update_speed), allocator);

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
	if (mcp.HasMember("progress_speed")) {
		progress_speed = mcp["progress_speed"].GetFloat();
	}
	if (mcp.HasMember("lifetime")) {
		lifetime = mcp["lifetime"].GetFloat();
	}
	if (mcp.HasMember("update_speed")) {
		update_speed = static_cast<UpdateSpeed>(mcp["update_speed"].GetInt());
		update_interval = to_seconds(update_speed);
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
	    if (ImGuiMCP::BeginCombo(label, SKSE::InputMap::GetKeyName(converted_key).c_str())) {
		    for (const auto& key_code : Input::Manager::GetKeys(selected_device)) {
			    const auto converted_keycode = Input::Manager::Convert(key_code, selected_device);
			    const auto key_name = SKSE::InputMap::GetKeyName(converted_keycode);
			    if (key_name.empty()) {
				    continue;
			    }
			    const bool isSelected = converted_key == converted_keycode;
			    if (ImGuiMCP::Selectable((key_name+std::format("##{}",converted_key)).c_str(), isSelected)) {
				    if (!isSelected) {
					    selected_key = key_code;
				    }
			    }
			    if (isSelected) ImGuiMCP::SetItemDefaultFocus();
		    }
		    ImGuiMCP::EndCombo();
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
	    if (ImGuiMCP::BeginCombo(label, device_to_string(MCP::current_device).data())) {
		    for (const auto& device : MCP::Settings::prompt_keys | std::views::keys) {
				if (!MCP::Settings::IsEnabled(device)) {
					continue;
				}
			    const bool isSelected = MCP::current_device == device;
			    if (ImGuiMCP::Selectable(device_to_string(device).data(), isSelected)) {
				    if (!isSelected) {
                        MCP::current_device = device;
				    }
			    }
			    if (isSelected) ImGuiMCP::SetItemDefaultFocus();
		    }
		    ImGuiMCP::EndCombo();
	    }
    }

    void RenderControl(std::map<Input::DEVICE, uint32_t>& curr_controls, const char* label, const char* help=nullptr)
    {
	    ImGuiMCP::Text(label);
	    ImGuiMCP::SameLine();
	    ImGuiMCP::SetCursorPosX(200.f);
	    ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.30f);
	    ControlBox(("##"+std::string(label)).c_str(), MCP::current_device, curr_controls.at(MCP::current_device));
		if (help) {
		    ImGuiMCP::SameLine();    
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
		if (ImGuiMCP::Checkbox((device_str+"##enabled").c_str(), &Settings::enabled_devices.at(device))) {
			settingsChanged = true;
		}
		if (device != Settings::enabled_devices.rbegin()->first) {
		    ImGuiMCP::SameLine();
		}
	}

	// need max number of buttons slider
	if (!ImGuiMCP::SliderInt("Max Buttons", &Settings::n_max_buttons, 1, 4)) {
		if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
			settingsChanged = true;
		}
	}

	const auto prompt_keys_before = Settings::prompt_keys;


	ImGuiMCP::Text("Device Selection:");
	ImGuiMCP::SameLine();
	ImGuiMCP::SetCursorPosX(200.f);
    ImGuiMCP::SetNextItemWidth(ImGuiMCP::GetWindowWidth() * 0.25f);
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

	ImGuiMCP::Text("");
	Settings::SpecialCommands::Render();
}

void MCP::Settings::SpecialCommands::Render()
{
	// double press: delete current prompt
	// triple press: cycle through prompts
	// triple press and hold: delete all prompts
	// explain what special commands are

	ImGuiMCP::Text("Special Commands");
	ImGuiMCP::SameLine();
	HelpMarker("Double press: delete current prompt\nTriple press: cycle through prompts\nTriple press and hold: delete all prompts");

	if (ImGuiMCP::Checkbox("Visualize", &visualize)) {
		to_json();
	}
	ImGuiMCP::SameLine();
	HelpMarker("Visualization of the special commands");

	if (!ImGuiMCP::SliderFloat("Responsiveness", &responsiveness, 0.0f, 1.0f)) {
		if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
			to_json();
            ImGui::Renderer::UpdateMaxIntervalBetweenPresses();
		}
	}
	ImGuiMCP::SameLine();
	HelpMarker("Higher values gives you less time to press the next key in return for faster response");

}


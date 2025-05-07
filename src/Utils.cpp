#include "Utils.h"
#include "MCP.h"
#include "imgui.h"

std::filesystem::path GetLogPath()
{
    const auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    return logFilePath;
}

std::vector<std::string> ReadLogFile() {
    std::vector<std::string> logLines;

    // Open the log file
    std::ifstream file(GetLogPath().c_str());
    if (!file.is_open()) {
        // Handle error
        return logLines;
    }

    // Read and store each line from the file
    std::string line;
    while (std::getline(file, line)) {
        logLines.push_back(line);
    }

    file.close();

    return logLines;
}

void BeginImGuiWindow(const char* window_name)
{
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.4f*MCP::Settings::prompt_size, 0.4f*MCP::Settings::prompt_size)); // Padding for cleaner layout


    ImGui::Begin(window_name, nullptr,
#ifndef NDEBUG
        ImGuiWindowFlags_AlwaysAutoResize
#else
        ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground|
		ImGuiWindowFlags_NoNavFocus | 
		ImGuiWindowFlags_NoDecoration | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoInputs |
	    ImGuiWindowFlags_NoNav |
	    ImGuiWindowFlags_NoBringToFrontOnFocus |
	    ImGuiWindowFlags_NoFocusOnAppearing
#endif
	);
}

void EndImGuiWindow()
{
	ImGui::End();
	ImGui::PopStyleVar(2);
}

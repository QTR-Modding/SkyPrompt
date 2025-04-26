#include "Utils.h"

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

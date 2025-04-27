#pragma once

std::filesystem::path GetLogPath();
std::vector<std::string> ReadLogFile();
const auto mod_name = static_cast<std::string>(SKSE::PluginDeclaration::GetSingleton()->GetName());
const std::string mod_folder = "Data/SKSE/Plugins/" + mod_name + "/";

class SpeedProfiler {
	std::chrono::time_point<std::chrono::steady_clock> start_time;
	std::chrono::time_point<std::chrono::steady_clock> end_time;
	std::string name;
public:
    explicit SpeedProfiler(const std::string& name) {
		start_time = std::chrono::steady_clock::now();
		this->name = name;
	}
	~SpeedProfiler() {
		end_time = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed_seconds = end_time - start_time;
		logger::info("{}: Elapsed time: {}", name, elapsed_seconds.count());
	}
};

void BeginImGuiWindow(const char* window_name);
void EndImGuiWindow();
#include "Utils.h"
#include "IconsFonts.h"
#include "Renderer.h"
#include "imgui.h"

namespace {
    bool IsTokenChar(const char a_char) {
        const auto byte = static_cast<unsigned char>(a_char);
        return std::isalnum(byte) || a_char == '_' || a_char == ':' || a_char == '.' || a_char == '-';
    }
}

std::filesystem::path GetLogPath() {
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

void BeginImGuiWindow(const char* window_name) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(0.4f * Theme::last_theme->prompt_size, 0.4f * Theme::last_theme->prompt_size));
    // Padding for cleaner layout

    ImGui::Begin(window_name, nullptr,
                 #ifndef NDEBUG
                 ImGuiWindowFlags_AlwaysAutoResize
                 #else
                 ImGuiWindowFlags_NoTitleBar |
                 //ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoBackground |
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

void EndImGuiWindow() {
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void TranslateEmbedded(std::string& a_text) {
    if (a_text.starts_with('$')) {
        std::string full = a_text;
        if (SKSE::Translation::Translate(full, full)) {
            a_text = std::move(full);
        }
    }

    std::size_t searchPos = 0;
    while ((searchPos = a_text.find('$', searchPos)) != std::string::npos) {
        const auto tokenStart = searchPos;
        auto tokenEnd = tokenStart + 1;

        if (tokenEnd >= a_text.size()) {
            break;
        }

        while (tokenEnd < a_text.size() && IsTokenChar(a_text[tokenEnd])) {
            ++tokenEnd;
        }

        while (tokenEnd < a_text.size() && a_text[tokenEnd] == '{') {
            int braceLevel = 0;
            size_t bracePos = tokenEnd;
            for (; bracePos < a_text.size(); ++bracePos) {
                if (a_text[bracePos] == '{') {
                    ++braceLevel;
                } else if (a_text[bracePos] == '}') {
                    --braceLevel;
                    if (braceLevel == 0) {
                        ++bracePos;
                        break;
                    }
                }
            }

            if (braceLevel != 0) {
                break;
            }

            tokenEnd = bracePos;
        }

        if (tokenEnd == tokenStart + 1) {
            searchPos = tokenEnd;
            continue;
        }

        std::string token = a_text.substr(tokenStart, tokenEnd - tokenStart);
        std::string replacement = token;
        if (SKSE::Translation::Translate(token, replacement)) {
            a_text.replace(tokenStart, tokenEnd - tokenStart, replacement);
            searchPos = tokenStart + replacement.size();
        } else {
            searchPos = tokenEnd;
        }
    }
}

void SkyrimMessageBox::Show(const std::string& bodyText, const std::vector<std::string>& buttonTextValues,
                            std::function<void(unsigned int)> callback) {
    const auto* factoryManager = RE::MessageDataFactoryManager::GetSingleton();
    const auto* uiStringHolder = RE::InterfaceStrings::GetSingleton();
    auto* factory = factoryManager->GetCreator<RE::MessageBoxData>(
        uiStringHolder->messageBoxData); // "MessageBoxData" <--- can we just use this string?
    auto* messagebox = factory->Create();
    const RE::BSTSmartPointer<RE::IMessageBoxCallback> messageCallback = RE::make_smart<
        MessageBoxResultCallback>(callback);
    messagebox->callback = messageCallback;
    messagebox->bodyText = bodyText;
    for (auto& text : buttonTextValues) messagebox->buttonText.push_back(text.c_str());
    messagebox->QueueMessage();
}
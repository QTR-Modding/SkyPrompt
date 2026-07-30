#pragma once

#include <format>
#include <string>
#include <string_view>

namespace Translations {
    void Load();

    [[nodiscard]] const std::string* TryGet(std::string_view a_key);
    [[nodiscard]] const std::string& Get(std::string_view a_key);
    [[nodiscard]] const std::string& GetEnglish(std::string_view a_key);
    [[nodiscard]] std::string GlyphText();
    [[nodiscard]] std::string FormatArgs(std::string_view a_key, std::format_args a_args);
    [[nodiscard]] std::string WithID(std::string_view a_visible_text, std::string_view a_stable_id);
    [[nodiscard]] std::string ImGuiLabel(std::string_view a_key, std::string_view a_stable_id);
    [[nodiscard]] std::string MenuToggleMode(std::string_view a_mode);

    template <class... Args>
    [[nodiscard]] std::string Format(const std::string_view a_key, Args&&... a_args) {
        return FormatArgs(a_key, std::make_format_args(a_args...));
    }
}

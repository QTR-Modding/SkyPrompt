#include "Theme.h"


Theme::PromptAlignment Theme::toPromptAlignment(const std::string& alignment) {
    if (alignment == "radial") return kRadial;
    if (alignment == "horizontal") return kHorizontal;
    if (alignment == "vertical") return kVertical;
    return kVertical; // default
}

Theme::Theme::Theme(const ThemeBlock& block) {
    theme_name = block.theme_name.get();
    theme_description = block.theme_description.get();
    theme_author = block.theme_author.get();
    theme_version = block.theme_version.get();

    n_max_buttons = block.n_max_buttons.get();
    marginX = block.marginX.get();
    marginY = block.marginY.get();
    xPercent = block.xPercent.get();
    yPercent = block.yPercent.get();

    prompt_size = block.prompt_size.get();
    icon2font_ratio = block.icon2font_ratio.get();
    linespacing = block.linespacing.get();

    font_name = block.font_name.get();
    font_shadow = block.font_shadow.get();
    progress_speed = block.progress_speed.get();
    fadeSpeed = block.fadeSpeed.get();

    prompt_alignment = toPromptAlignment(block.prompt_alignment.get());

    special_effect = block.special_effect.get();

    special_integers = block.special_integers.get();
    special_strings = block.special_strings.get();
    special_floats = block.special_floats.get();
    for (const auto& a_bool : block.special_bools.get()) {
        special_bools.push_back(a_bool);
    }

    hide_in_menu = block.hide_in_menu.get();
}

void Theme::Theme::ReLoad(std::string_view a_filename) {
    constexpr std::string_view themesFolder = R"(Data\SKSE\Plugins\SkyPrompt\themes)";
    if (!std::filesystem::exists(themesFolder)) {
        logger::error("Mod folder does not exist: {}", themesFolder);
        return;
    }
    for (const auto& file : std::filesystem::directory_iterator(themesFolder)) {
        if (!file.is_regular_file() || file.path().extension() != ".json") {
            continue; // Skip non-JSON files
        }
        if (a_filename != file.path().stem().string()) {
            continue; // Skip if the theme name does not match the file name
        }
        rapidjson::Document doc;
        // Load the JSON file
        std::ifstream ifs(file.path());
        if (!ifs.is_open()) {
            logger::error("Failed to open file: {}", file.path().string());
            return;
        }
        std::string json_str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        doc.Parse(json_str.c_str());
        if (doc.HasParseError()) {
            logger::error("JSON Parse Error at offset {}: {}", doc.GetErrorOffset(), doc.GetParseError());
            return;
        }
        ThemeBlock data;
        data.load(doc);
        *this = Theme(data); // Update the theme with the new data

        return;
    }
}

void Theme::LoadThemes() {
    constexpr std::string_view themesFolder = R"(Data\SKSE\Plugins\SkyPrompt\themes)";
    if (!std::filesystem::exists(themesFolder)) {
        logger::error("Mod folder does not exist: {}", themesFolder);
        return;
    }

    for (const auto& file : std::filesystem::directory_iterator(themesFolder)) {
        if (!file.is_regular_file() || file.path().extension() != ".json") {
            continue; // Skip non-JSON files
        }

        const auto filename = file.path().stem().string();
        if (filename.empty()) {
            continue;
        }
        logger::info("Found JSON file: {}", filename);
        rapidjson::Document doc;
        // Load the JSON file
        std::ifstream ifs(file.path());
        if (!ifs.is_open()) {
            logger::error("Failed to open file: {}", file.path().string());
            continue;
        }
        std::string json_str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        doc.Parse(json_str.c_str());
        if (doc.HasParseError()) {
            logger::error("JSON Parse Error at offset {}: {}", doc.GetErrorOffset(), doc.GetParseError());
            continue;
        }

        ThemeBlock data;
        data.load(doc);
        Theme a_theme(data);

        if (auto& a_name = filename; !themes_loaded.contains(a_name)) {
            themes_loaded[a_name] = a_theme;
            logger::info("Loaded theme: {}", a_name);
        } else {
            logger::error("Theme name is empty or already exists: {}", a_name);
        }
    }
}
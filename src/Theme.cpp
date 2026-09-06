#include "Theme.h"
#include "MCP.h"
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>


Theme::PromptAlignment Theme::toPromptAlignment(const std::string& alignment) {
    if (alignment == "radial") return kRadial;
    if (alignment == "horizontal") return kHorizontal;
    if (alignment == "vertical") return kVertical;
    if (alignment == "diamond") return kDiamond;
    if (alignment == "list") return kList;
    return kVertical; // default
}

std::string_view Theme::toPromptAlignmentString(const PromptAlignment alignment) {
    switch (alignment) {
        case kRadial:
            return "radial";
        case kHorizontal:
            return "horizontal";
        case kDiamond:
            return "diamond";
        case kList:
            return "list";
        case kVertical:
        default:
            return "vertical";
    }
}

Theme::PromptOrder Theme::toPromptOrder(const std::string& value) {
    if (value == "text-first" || value == "text_first" || value == "textfirst") {
        return kTextFirst;
    }
    if (value == "icon-first" || value == "icon_first" || value == "iconfirst") {
        return kIconFirst;
    }
    return kIconFirst;
}

std::string_view Theme::toPromptOrderString(const PromptOrder order) {
    switch (order) {
        case kTextFirst:
            return "text-first";
        case kIconFirst:
        default:
            return "icon-first";
    }
}

Theme::PromptPivot Theme::toPromptPivot(const std::string& value) {
    if (value == "top-left" || value == "top_left" || value == "topleft") {
        return kTopLeft;
    }
    if (value == "top-right" || value == "top_right" || value == "topright") {
        return kTopRight;
    }
    if (value == "bottom-left" || value == "bottom_left" || value == "bottomleft") {
        return kBottomLeft;
    }
    if (value == "center") {
        return kCenter;
    }
    if (value == "bottom-right" || value == "bottom_right" || value == "bottomright") {
        return kBottomRight;
    }
    return kBottomRight;
}

std::string_view Theme::toPromptPivotString(const PromptPivot pivot) {
    switch (pivot) {
        case kTopLeft:
            return "top-left";
        case kTopRight:
            return "top-right";
        case kBottomLeft:
            return "bottom-left";
        case kCenter:
            return "center";
        case kBottomRight:
        default:
            return "bottom-right";
    }
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
    prompt_order = toPromptOrder(block.prompt_order.get());
    prompt_pivot = toPromptPivot(block.prompt_pivot.get());

    special_effect = block.special_effect.get();

    special_integers = block.special_integers.get();
    special_strings = block.special_strings.get();
    special_floats = block.special_floats.get();
    for (const auto& a_bool : block.special_bools.get()) {
        special_bools.push_back(a_bool);
    }

    hide_in_menu = block.hide_in_menu.get();
}

void Theme::Theme::UpdateSettings(rapidjson::Document& a_document) const {
    auto& allocator = a_document.GetAllocator();
    const auto set = [&](const char* a_key, const auto& a_value) {
        rapidjson::Value value;
        if constexpr (std::is_convertible_v<decltype(a_value), std::string_view>) {
            const std::string_view text = a_value;
            value.SetString(text.data(), static_cast<rapidjson::SizeType>(text.size()), allocator);
        } else {
            value.Set(a_value);
        }
        if (const auto member = a_document.FindMember(a_key); member != a_document.MemberEnd()) {
            member->value = std::move(value);
        } else {
            a_document.AddMember(rapidjson::Value(a_key, allocator), value, allocator);
        }
    };

    set("n_max_buttons", n_max_buttons);
    set("marginX", marginX);
    set("marginY", marginY);
    set("xPercent", xPercent);
    set("yPercent", yPercent);
    set("prompt_size", prompt_size);
    set("icon2font_ratio", icon2font_ratio);
    set("linespacing", linespacing);
    set("progress_speed", progress_speed);
    set("fadeSpeed", fadeSpeed);
    set("font_name", font_name);
    set("font_shadow", font_shadow);
    set("prompt_alignment", toPromptAlignmentString(prompt_alignment));
    set("prompt_order", toPromptOrderString(prompt_order));
    set("prompt_pivot", toPromptPivotString(prompt_pivot));
}

bool Theme::WriteThemeFile(const std::filesystem::path& a_path, const rapidjson::Document& a_document) {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    if (!a_document.Accept(writer)) {
        logger::error("Failed to serialize theme: {}", a_path.string());
        return false;
    }

    auto temporary = a_path;
    temporary += ".tmp";
    std::ofstream file(temporary, std::ios::binary | std::ios::out | std::ios::noreplace);
    if (!file.is_open()) {
        logger::error("Failed to open theme write path: {}", temporary.string());
        return false;
    }
    file.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
    file.put('\n');
    file.close();
    const bool saved = !file.fail() && MoveFileExW(temporary.c_str(), a_path.c_str(), MOVEFILE_REPLACE_EXISTING);
    if (!saved) {
        logger::error("Failed to write theme: {}", a_path.string());
        std::error_code error;
        std::filesystem::remove(temporary, error);
    }
    return saved;
}

bool Theme::Theme::Save(const std::filesystem::path& a_path) const {
    std::ifstream file(a_path);
    const std::string json((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());
    rapidjson::Document document;
    document.Parse(json.c_str());
    if (!file.is_open() || file.bad() || document.HasParseError() || !document.IsObject()) {
        logger::error("Failed to read theme for saving: {}", a_path.string());
        return false;
    }
    file.close();
    UpdateSettings(document);
    return WriteThemeFile(a_path, document);
}

void Theme::Theme::ReLoad(std::string_view a_filename) {
    if (!std::filesystem::exists(themes_folder)) {
        logger::error("Mod folder does not exist: {}", themes_folder);
        return;
    }
    for (const auto& file : std::filesystem::directory_iterator(themes_folder)) {
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
        std::string json_str((std::istreambuf_iterator(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        doc.Parse(json_str.c_str());
        if (doc.HasParseError()) {
            logger::error("JSON Parse Error at offset {}: {}", doc.GetErrorOffset(),
                          rapidjson::GetParseError_En(doc.GetParseError()));
            return;
        }
        ThemeBlock data;
        data.load(doc);
        *this = Theme(data); // Update the theme with the new data

        return;
    }
}

void Theme::LoadThemes() {
    if (!std::filesystem::exists(themes_folder)) {
        logger::error("Mod folder does not exist: {}", themes_folder);
        return;
    }

    for (const auto& file : std::filesystem::directory_iterator(themes_folder)) {
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
        std::string json_str((std::istreambuf_iterator(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        doc.Parse(json_str.c_str());
        if (doc.HasParseError()) {
            logger::error("JSON Parse Error at offset {}: {}", doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
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

void Theme::ReLoadDefaultTheme() {
    default_theme = Theme();
    last_theme = &default_theme;
    MCP::refreshStyle.store(true);
}

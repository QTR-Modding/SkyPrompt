#include "Theme.h"

#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "MCP.h"

namespace {
    bool EqualPathNoCase(const std::filesystem::path& a_left, const std::filesystem::path& a_right) {
        const auto& left = a_left.native();
        const auto& right = a_right.native();
        return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(),
                                    static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
    }

    bool RegistryNameExists(const std::string_view a_name) {
        const auto candidate = std::filesystem::path(std::string(a_name));
        return std::ranges::any_of(Theme::themes_loaded, [&](const auto& entry) {
            return EqualPathNoCase(std::filesystem::path(entry.first), candidate);
        });
    }

    bool FileNameExists(const std::string_view a_name) {
        const auto candidate = std::filesystem::path(std::format("{}.json", a_name));
        std::error_code error;
        const auto folder = std::filesystem::path(Theme::themes_folder);
        for (std::filesystem::directory_iterator iterator(folder, error), end; !error && iterator != end;
             iterator.increment(error)) {
            if (EqualPathNoCase(iterator->path().filename(), candidate)) {
                return true;
            }
        }
        return false;
    }

    bool IsReservedWindowsName(const std::string_view a_name) {
        auto base = std::string(a_name.substr(0, a_name.find('.')));
        std::ranges::transform(base, base.begin(), [](const unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });

        constexpr std::array reserved = {"CON"sv, "PRN"sv, "AUX"sv, "NUL"sv};
        if (std::ranges::find(reserved, base) != reserved.end()) {
            return true;
        }
        return base.size() == 4 && (base.starts_with("COM") || base.starts_with("LPT")) && base.back() >= '1' &&
               base.back() <= '9';
    }
}

Theme::PromptAlignment Theme::toPromptAlignment(const std::string& alignment) {
    if (alignment == "radial") return kRadial;
    if (alignment == "horizontal") return kHorizontal;
    if (alignment == "vertical") return kVertical;
    return kVertical; // default
}

std::string_view Theme::toPromptAlignmentString(const PromptAlignment alignment) {
    switch (alignment) {
        case kRadial:
            return "radial";
        case kHorizontal:
            return "horizontal";
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

Theme::ExportNameStatus Theme::ValidateExportName(const std::string_view a_name) {
    if (a_name.empty() || std::ranges::all_of(a_name, [](const unsigned char character) { return character == ' '; })) {
        return ExportNameStatus::kEmpty;
    }

    constexpr std::string_view invalid_characters = R"(<>:"/\|?*)";
    if (a_name.size() > 240 || a_name == "." || a_name == ".." || a_name.back() == '.' || a_name.back() == ' ' ||
        std::ranges::any_of(a_name,
                            [&](const unsigned char character) {
                                return character < 32 || invalid_characters.contains(static_cast<char>(character));
                            }) ||
        IsReservedWindowsName(a_name)) {
        return ExportNameStatus::kInvalid;
    }

    {
        std::shared_lock lock(m_theme_);
        if (RegistryNameExists(a_name)) {
            return ExportNameStatus::kExists;
        }
    }
    return FileNameExists(a_name) ? ExportNameStatus::kExists : ExportNameStatus::kValid;
}

std::string Theme::NextExportName() {
    for (std::uint64_t number = 1;; ++number) {
        auto name = std::format("MyTheme{}", number);
        if (ValidateExportName(name) == ExportNameStatus::kValid) {
            return name;
        }
    }
}

std::optional<std::filesystem::path> Theme::ExportTheme(const Theme& a_theme, const std::string_view a_name) {
    using namespace rapidjson;

    if (ValidateExportName(a_name) != ExportNameStatus::kValid) {
        return std::nullopt;
    }

    Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();
    const auto add_string = [&](const char* a_key, const std::string_view a_value) {
        Value key;
        key.SetString(a_key, allocator);
        Value value;
        value.SetString(a_value.data(), static_cast<SizeType>(a_value.size()), allocator);
        document.AddMember(key, value, allocator);
    };

    add_string("name", a_theme.theme_name);
    add_string("description", a_theme.theme_description);
    add_string("author", a_theme.theme_author);
    add_string("version", a_theme.theme_version);
    document.AddMember("marginX", a_theme.marginX, allocator);
    document.AddMember("marginY", a_theme.marginY, allocator);
    document.AddMember("xPercent", a_theme.xPercent, allocator);
    document.AddMember("yPercent", a_theme.yPercent, allocator);
    document.AddMember("prompt_size", a_theme.prompt_size, allocator);
    document.AddMember("icon2font_ratio", a_theme.icon2font_ratio, allocator);
    document.AddMember("linespacing", a_theme.linespacing, allocator);
    document.AddMember("progress_speed", a_theme.progress_speed, allocator);
    document.AddMember("fadeSpeed", a_theme.fadeSpeed, allocator);
    add_string("font_name", a_theme.font_name);
    document.AddMember("font_shadow", a_theme.font_shadow, allocator);
    add_string("prompt_alignment", toPromptAlignmentString(a_theme.prompt_alignment));
    add_string("prompt_order", toPromptOrderString(a_theme.prompt_order));
    add_string("prompt_pivot", toPromptPivotString(a_theme.prompt_pivot));
    document.AddMember("special_effect", a_theme.special_effect, allocator);

    Value integers(kArrayType);
    for (const auto value : a_theme.special_integers) {
        integers.PushBack(value, allocator);
    }
    document.AddMember("special_integers", integers, allocator);

    Value strings(kArrayType);
    for (const auto& string : a_theme.special_strings) {
        Value value;
        value.SetString(string.data(), static_cast<SizeType>(string.size()), allocator);
        strings.PushBack(value, allocator);
    }
    document.AddMember("special_strings", strings, allocator);

    Value floats(kArrayType);
    for (const auto value : a_theme.special_floats) {
        floats.PushBack(value, allocator);
    }
    document.AddMember("special_floats", floats, allocator);

    Value bools(kArrayType);
    for (const auto value : a_theme.special_bools) {
        bools.PushBack(value != 0, allocator);
    }
    document.AddMember("special_bools", bools, allocator);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    const auto folder = std::filesystem::path(themes_folder);
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (error) {
        logger::error("Failed to create theme folder: {}", error.message());
        return std::nullopt;
    }

    const auto name = std::string(a_name);
    const auto path = folder / std::format("{}.json", name);
    auto exported_theme = a_theme;
    const Theme defaults;
    exported_theme.n_max_buttons = defaults.n_max_buttons;
    exported_theme.hide_in_menu = defaults.hide_in_menu;

    std::unique_lock lock(m_theme_);
    if (RegistryNameExists(name)) {
        return std::nullopt;
    }
    const auto [theme, inserted] = themes_loaded.try_emplace(name, std::move(exported_theme));
    if (!inserted) {
        return std::nullopt;
    }

    std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::noreplace);
    if (!file.is_open()) {
        themes_loaded.erase(theme);
        logger::error("Failed to open theme export path: {}", path.string());
        return std::nullopt;
    }
    file.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
    file.put('\n');
    file.close();
    if (file.fail()) {
        themes_loaded.erase(theme);
        std::filesystem::remove(path, error);
        logger::error("Failed to export theme: {}", path.string());
        return std::nullopt;
    }

    logger::info("Exported theme: {}", path.string());
    return path;
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

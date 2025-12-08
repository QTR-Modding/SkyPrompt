#pragma once

std::filesystem::path GetLogPath();
std::vector<std::string> ReadLogFile();
const auto mod_name = static_cast<std::string>(SKSE::PluginDeclaration::GetSingleton()->GetName());
const std::string mod_folder = "Data/SKSE/Plugins/" + mod_name + "/";
inline auto json_folder = mod_folder + "settings.json";

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

namespace WorldObjects {
    inline RE::bhkRigidBody* GetRigidBody(const RE::TESObjectREFR* refr) {
        const auto object3D = refr->Get3D();

        if (!object3D) {
            return nullptr;
        }
        const auto collision = object3D->GetCollisionObject();

        if (!collision) {
            return nullptr;
        }

        const auto body = collision->GetRigidBody();

        return body;
    }

    inline RE::NiPoint3 GetPosition(const RE::TESObjectREFR* obj) {
        const auto body = GetRigidBody(obj);
        if (!body) return obj->GetPosition();
        RE::hkVector4 havockPosition;
        body->GetPosition(havockPosition);
        float components[4];
        _mm_store_ps(components, havockPosition.quad);
        RE::NiPoint3 newPosition = {components[0], components[1], components[2]};
        constexpr float havockToSkyrimConversionRate = 69.9915f;
        newPosition *= havockToSkyrimConversionRate;
        return newPosition;
    }
}

// https://github.com/SkyrimScripting/MessageBox/blob/ac0ea32af02766582209e784689eb0dd7d731d57/include/SkyrimScripting/MessageBox.h#L9
class SkyrimMessageBox {
    class MessageBoxResultCallback final : public RE::IMessageBoxCallback {
        std::function<void(unsigned int)> _callback;

    public:
        ~MessageBoxResultCallback() override = default;

        explicit MessageBoxResultCallback(const std::function<void(unsigned int)>& callback) : _callback(callback) {
        }

        void Run(Message message) override {
            _callback(static_cast<unsigned int>(message));
        }
    };

public:
    static void Show(const std::string& bodyText, const std::vector<std::string>& buttonTextValues,
                     std::function<void(unsigned int)> callback);
};

inline void ShowMessageBox(const std::string& bodyText, const std::vector<std::string>& buttonTextValues,
                           const std::function<void(unsigned int)>& callback) {
    SkyrimMessageBox::Show(bodyText, buttonTextValues, callback);
}
#pragma once
#include <string>
#include <filesystem>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Структуры конфигураций
    struct MainConfig {
        struct Menu {
            float opacity = 0.9f;
            float scale = 1.0f;
            bool opened = false;
        } menu;
        
        struct Overlay {
            bool enabled = true;
            bool clickthroughWhenClosed = true;
            bool focusOnly = true;
            bool fps = true;
            bool stats = true;
            bool watermark = true;
            bool vsync = false;
        } overlay;
    };
    
    struct VisualConfig {
        struct Main {
            std::string section = "default";
            float masterColor[4] = {1.0f, 0.65f, 0.0f, 1.0f};
            float scale = 1.0f;
            bool preview = false;
        } main;
        
        struct Sections {
            struct Aimbot {
                float fovColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
                float targetColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            } aimbot;
            
            struct ESP {
                float pedColor[4] = {0.0f, 1.0f, 1.0f, 1.0f};
                float vehicleColor[4] = {1.0f, 1.0f, 0.0f, 1.0f};
                float objectColor[4] = {1.0f, 0.0f, 1.0f, 1.0f};
                float pickupColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
                float friendColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            } esp;
            
            struct Overlay {
                float backgroundColor[4] = {0.0f, 0.0f, 0.0f, 0.59f};
                float textColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                float accentColor[4] = {1.0f, 0.65f, 0.0f, 1.0f};
            } overlay;
        } sections;
        
        struct Sidebar {
            float width = 300.0f;
            bool collapsed = false;
        } sidebar;
        
        struct Settings {
            std::string theme = "dark";
            std::string font = "ProggyClean.ttf";
            float fontSize = 13.0f;
        } settings;
    };
    
    // Инициализация
    bool Initialize();
    void Shutdown();
    
    // Загрузка/сохранение
    bool LoadAllConfigs();
    bool SaveAllConfigs();
    
    bool LoadConfig(const std::string& name);
    bool SaveConfig(const std::string& name);
    
    // Геттеры/сеттеры
    const MainConfig& GetMainConfig() const { return m_mainConfig; }
    void SetMainConfig(const MainConfig& config) { m_mainConfig = config; }
    
    const VisualConfig& GetVisualConfig() const { return m_visualConfig; }
    void SetVisualConfig(const VisualConfig& config) { m_visualConfig = config; }
    
    // Профили
    bool CreateProfile(const std::string& name);
    bool DeleteProfile(const std::string& name);
    bool LoadProfile(const std::string& name);
    bool SaveProfile(const std::string& name);
    std::vector<std::string> GetProfiles() const;
    
    // Утилиты
    std::string GetConfigPath() const;
    std::string GetProfilesPath() const;
    std::string GetResourcesPath() const;
    
    bool FileExists(const std::string& path) const;
    bool DirectoryExists(const std::string& path) const;
    
private:
    MainConfig m_mainConfig;
    VisualConfig m_visualConfig;
    
    std::string m_configPath;
    std::string m_profilesPath;
    std::string m_resourcesPath;
    
    bool m_initialized = false;
    
    // Вспомогательные методы
    bool CreateDirectories();
    bool LoadJSON(const std::string& path, json& data);
    bool SaveJSON(const std::string& path, const json& data);
    
    // Сериализация/десериализация
    void FromJSON(const json& j, MainConfig& config);
    void ToJSON(json& j, const MainConfig& config);
    
    void FromJSON(const json& j, VisualConfig& config);
    void ToJSON(json& j, const VisualConfig& config);
    
    // Валидация
    bool ValidateConfig(const MainConfig& config);
    bool ValidateConfig(const VisualConfig& config);
    
    // Миграция версий
    bool MigrateConfig(json& data, int fromVersion, int toVersion);
    
    // Резервное копирование
    bool CreateBackup(const std::string& configName);
    bool RestoreBackup(const std::string& configName, const std::string& backupName);
    std::vector<std::string> GetBackups(const std::string& configName) const;
};
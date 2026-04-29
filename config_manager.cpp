#include "config_manager.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

ConfigManager::ConfigManager() {
    SPDLOG_INFO("Создание ConfigManager");
}

ConfigManager::~ConfigManager() {
    SPDLOG_INFO("Уничтожение ConfigManager");
    Shutdown();
}

bool ConfigManager::Initialize() {
    if (m_initialized) {
        SPDLOG_WARN("ConfigManager уже инициализирован");
        return true;
    }
    
    SPDLOG_INFO("Инициализация ConfigManager");
    
    try {
        // Установка путей
        m_configPath = "config/";
        m_profilesPath = m_configPath + "profiles/";
        m_resourcesPath = "resources/";
        
        // Создание директорий
        if (!CreateDirectories()) {
            SPDLOG_ERROR("Не удалось создать директории конфигураций");
            return false;
        }
        
        // Настройки по умолчанию
        m_mainConfig.menu.opacity = 0.9f;
        m_mainConfig.menu.scale = 1.0f;
        m_mainConfig.menu.opened = false;
        
        m_mainConfig.overlay.enabled = true;
        m_mainConfig.overlay.clickthroughWhenClosed = true;
        m_mainConfig.overlay.focusOnly = true;
        m_mainConfig.overlay.fps = true;
        m_mainConfig.overlay.stats = true;
        m_mainConfig.overlay.watermark = true;
        m_mainConfig.overlay.vsync = false;
        
        m_visualConfig.main.section = "default";
        m_visualConfig.main.masterColor[0] = 1.0f;
        m_visualConfig.main.masterColor[1] = 0.65f;
        m_visualConfig.main.masterColor[2] = 0.0f;
        m_visualConfig.main.masterColor[3] = 1.0f;
        m_visualConfig.main.scale = 1.0f;
        m_visualConfig.main.preview = false;
        
        // Цвета для aimbot
        m_visualConfig.sections.aimbot.fovColor[0] = 1.0f;
        m_visualConfig.sections.aimbot.fovColor[1] = 0.0f;
        m_visualConfig.sections.aimbot.fovColor[2] = 0.0f;
        m_visualConfig.sections.aimbot.fovColor[3] = 1.0f;
        
        m_visualConfig.sections.aimbot.targetColor[0] = 0.0f;
        m_visualConfig.sections.aimbot.targetColor[1] = 1.0f;
        m_visualConfig.sections.aimbot.targetColor[2] = 0.0f;
        m_visualConfig.sections.aimbot.targetColor[3] = 1.0f;
        
        // Цвета для ESP
        m_visualConfig.sections.esp.pedColor[0] = 0.0f;
        m_visualConfig.sections.esp.pedColor[1] = 1.0f;
        m_visualConfig.sections.esp.pedColor[2] = 1.0f;
        m_visualConfig.sections.esp.pedColor[3] = 1.0f;
        
        m_visualConfig.sections.esp.vehicleColor[0] = 1.0f;
        m_visualConfig.sections.esp.vehicleColor[1] = 1.0f;
        m_visualConfig.sections.esp.vehicleColor[2] = 0.0f;
        m_visualConfig.sections.esp.vehicleColor[3] = 1.0f;
        
        m_visualConfig.sections.esp.objectColor[0] = 1.0f;
        m_visualConfig.sections.esp.objectColor[1] = 0.0f;
        m_visualConfig.sections.esp.objectColor[2] = 1.0f;
        m_visualConfig.sections.esp.objectColor[3] = 1.0f;
        
        m_visualConfig.sections.esp.pickupColor[0] = 0.0f;
        m_visualConfig.sections.esp.pickupColor[1] = 1.0f;
        m_visualConfig.sections.esp.pickupColor[2] = 0.0f;
        m_visualConfig.sections.esp.pickupColor[3] = 1.0f;
        
        m_visualConfig.sections.esp.friendColor[0] = 0.0f;
        m_visualConfig.sections.esp.friendColor[1] = 0.0f;
        m_visualConfig.sections.esp.friendColor[2] = 1.0f;
        m_visualConfig.sections.esp.friendColor[3] = 1.0f;
        
        // Цвета для оверлея
        m_visualConfig.sections.overlay.backgroundColor[0] = 0.0f;
        m_visualConfig.sections.overlay.backgroundColor[1] = 0.0f;
        m_visualConfig.sections.overlay.backgroundColor[2] = 0.0f;
        m_visualConfig.sections.overlay.backgroundColor[3] = 0.59f;
        
        m_visualConfig.sections.overlay.textColor[0] = 1.0f;
        m_visualConfig.sections.overlay.textColor[1] = 1.0f;
        m_visualConfig.sections.overlay.textColor[2] = 1.0f;
        m_visualConfig.sections.overlay.textColor[3] = 1.0f;
        
        m_visualConfig.sections.overlay.accentColor[0] = 1.0f;
        m_visualConfig.sections.overlay.accentColor[1] = 0.65f;
        m_visualConfig.sections.overlay.accentColor[2] = 0.0f;
        m_visualConfig.sections.overlay.accentColor[3] = 1.0f;
        
        // Настройки сайдбара
        m_visualConfig.sidebar.width = 300.0f;
        m_visualConfig.sidebar.collapsed = false;
        
        // Настройки приложения
        m_visualConfig.settings.theme = "dark";
        m_visualConfig.settings.font = "ProggyClean.ttf";
        m_visualConfig.settings.fontSize = 13.0f;
        
        m_initialized = true;
        SPDLOG_INFO("ConfigManager успешно инициализирован");
        return true;
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при инициализации ConfigManager: {}", e.what());
        return false;
    }
}

void ConfigManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    SPDLOG_INFO("Завершение работы ConfigManager");
    
    // Сохранение конфигураций
    SaveAllConfigs();
    
    m_initialized = false;
    SPDLOG_INFO("ConfigManager завершен");
}

bool ConfigManager::LoadAllConfigs() {
    SPDLOG_INFO("Загрузка всех конфигураций");
    
    bool success = true;
    
    // Загрузка основных настроек
    success &= LoadConfig("settings");
    
    // Загрузка визуальных настроек
    success &= LoadConfig("visual");
    
    // Загрузка профилей
    std::vector<std::string> profiles = GetProfiles();
    for (const auto& profile : profiles) {
        SPDLOG_INFO("Загружен профиль: {}", profile);
    }
    
    if (success) {
        SPDLOG_INFO("Все конфигурации успешно загружены");
    } else {
        SPDLOG_WARN("Не все конфигурации удалось загрузить");
    }
    
    return success;
}

bool ConfigManager::SaveAllConfigs() {
    SPDLOG_INFO("Сохранение всех конфигураций");
    
    bool success = true;
    
    // Сохранение основных настроек
    success &= SaveConfig("settings");
    
    // Сохранение визуальных настроек
    success &= SaveConfig("visual");
    
    if (success) {
        SPDLOG_INFO("Все конфигурации успешно сохранены");
    } else {
        SPDLOG_WARN("Не все конфигурации удалось сохранить");
    }
    
    return success;
}

bool ConfigManager::LoadConfig(const std::string& name) {
    std::string path = m_configPath + name + ".json";
    
    if (!FileExists(path)) {
        SPDLOG_WARN("Конфигурационный файл не найден: {}", path);
        return false;
    }
    
    json data;
    if (!LoadJSON(path, data)) {
        SPDLOG_ERROR("Не удалось загрузить JSON из: {}", path);
        return false;
    }
    
    try {
        if (name == "settings") {
            FromJSON(data, m_mainConfig);
            SPDLOG_INFO("Основные настройки загружены из: {}", path);
        } else if (name == "visual") {
            FromJSON(data, m_visualConfig);
            SPDLOG_INFO("Визуальные настройки загружены из: {}", path);
        } else {
            SPDLOG_WARN("Неизвестный тип конфигурации: {}", name);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при загрузке конфигурации {}: {}", name, e.what());
        return false;
    }
}

bool ConfigManager::SaveConfig(const std::string& name) {
    std::string path = m_configPath + name + ".json";
    
    json data;
    
    try {
        if (name == "settings") {
            ToJSON(data, m_mainConfig);
            SPDLOG_INFO("Основные настройки сохранены в: {}", path);
        } else if (name == "visual") {
            ToJSON(data, m_visualConfig);
            SPDLOG_INFO("Визуальные настройки сохранены в: {}", path);
        } else {
            SPDLOG_WARN("Неизвестный тип конфигурации: {}", name);
            return false;
        }
        
        return SaveJSON(path, data);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при сохранении конфигурации {}: {}", name, e.what());
        return false;
    }
}

bool ConfigManager::CreateProfile(const std::string& name) {
    std::string path = m_profilesPath + name + ".json";
    
    if (FileExists(path)) {
        SPDLOG_WARN("Профиль уже существует: {}", name);
        return false;
    }
    
    // Создание профиля с текущими настройками
    json profileData;
    
    // Сохранение основных настроек
    json mainConfigJson;
    ToJSON(mainConfigJson, m_mainConfig);
    profileData["main"] = mainConfigJson;
    
    // Сохранение визуальных настроек
    json visualConfigJson;
    ToJSON(visualConfigJson, m_visualConfig);
    profileData["visual"] = visualConfigJson;
    
    // Сохранение времени создания
    profileData["created"] = time(nullptr);
    profileData["name"] = name;
    
    if (SaveJSON(path, profileData)) {
        SPDLOG_INFO("Профиль создан: {}", name);
        return true;
    }
    
    SPDLOG_ERROR("Не удалось создать профиль: {}", name);
    return false;
}

bool ConfigManager::DeleteProfile(const std::string& name) {
    std::string path = m_profilesPath + name + ".json";
    
    if (!FileExists(path)) {
        SPDLOG_WARN("Профиль не найден: {}", name);
        return false;
    }
    
    try {
        fs::remove(path);
        SPDLOG_INFO("Профиль удален: {}", name);
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при удалении профиля {}: {}", name, e.what());
        return false;
    }
}

bool ConfigManager::LoadProfile(const std::string& name) {
    std::string path = m_profilesPath + name + ".json";
    
    if (!FileExists(path)) {
        SPDLOG_WARN("Профиль не найден: {}", name);
        return false;
    }
    
    json data;
    if (!LoadJSON(path, data)) {
        SPDLOG_ERROR("Не удалось загрузить профиль: {}", name);
        return false;
    }
    
    try {
        // Загрузка основных настроек
        if (data.contains("main")) {
            FromJSON(data["main"], m_mainConfig);
        }
        
        // Загрузка визуальных настроек
        if (data.contains("visual")) {
            FromJSON(data["visual"], m_visualConfig);
        }
        
        SPDLOG_INFO("Профиль загружен: {}", name);
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при загрузке профиля {}: {}", name, e.what());
        return false;
    }
}

bool ConfigManager::SaveProfile(const std::string& name) {
    return CreateProfile(name); // Сохранение профиля аналогично созданию
}

std::vector<std::string> ConfigManager::GetProfiles() const {
    std::vector<std::string> profiles;
    
    try {
        if (fs::exists(m_profilesPath) && fs::is_directory(m_profilesPath)) {
            for (const auto& entry : fs::directory_iterator(m_profilesPath)) {
                if (entry.path().extension() == ".json") {
                    profiles.push_back(entry.path().stem().string());
                }
            }
        }
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при получении списка профилей: {}", e.what());
    }
    
    return profiles;
}

std::string ConfigManager::GetConfigPath() const {
    return m_configPath;
}

std::string ConfigManager::GetProfilesPath() const {
    return m_profilesPath;
}

std::string ConfigManager::GetResourcesPath() const {
    return m_resourcesPath;
}

bool ConfigManager::FileExists(const std::string& path) const {
    try {
        return fs::exists(path);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при проверке существования файла {}: {}", path, e.what());
        return false;
    }
}

bool ConfigManager::DirectoryExists(const std::string& path) const {
    try {
        return fs::exists(path) && fs::is_directory(path);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при проверке существования директории {}: {}", path, e.what());
        return false;
    }
}

bool ConfigManager::CreateDirectories() {
    try {
        // Создание основных директорий
        fs::create_directories(m_configPath);
        fs::create_directories(m_profilesPath);
        fs::create_directories(m_resourcesPath);
        
        // Создание резервных директорий
        fs::create_directories(m_configPath + "backups/");
        
        SPDLOG_INFO("Директории конфигураций созданы");
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при создании директорий: {}", e.what());
        return false;
    }
}

bool ConfigManager::LoadJSON(const std::string& path, json& data) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            SPDLOG_ERROR("Не удалось открыть файл: {}", path);
            return false;
        }
        
        file >> data;
        file.close();
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при загрузке JSON из {}: {}", path, e.what());
        return false;
    }
}

bool ConfigManager::SaveJSON(const std::string& path, const json& data) {
    try {
        // Создание резервной копии если файл существует
        if (FileExists(path)) {
            CreateBackup(fs::path(path).stem().string());
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            SPDLOG_ERROR("Не удалось создать файл: {}", path);
            return false;
        }
        
        file << data.dump(4); // Красивый вывод с отступами
        file.close();
        return true;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при сохранении JSON в {}: {}", path, e.what());
        return false;
    }
}

void ConfigManager::FromJSON(const json& j, MainConfig& config) {
    if (j.contains("menu")) {
        const auto& menu = j["menu"];
        if (menu.contains("opacity")) config.menu.opacity = menu["opacity"];
        if (menu.contains("scale")) config.menu.scale = menu["scale"];
        if (menu.contains("opened")) config.menu.opened = menu["opened"];
    }
    
    if (j.contains("overlay")) {
        const auto& overlay = j["overlay"];
        if (overlay.contains("enabled")) config.overlay.enabled = overlay["enabled"];
        if (overlay.contains("clickthrough_when_closed")) config.overlay.clickthroughWhenClosed = overlay["clickthrough_when_closed"];
        if (overlay.contains("focus_only")) config.overlay.focusOnly = overlay["focus_only"];
        if (overlay.contains("fps")) config.overlay.fps = overlay["fps"];
        if (overlay.contains("stats")) config.overlay.stats = overlay["stats"];
        if (overlay.contains("watermark")) config.overlay.watermark = overlay["watermark"];
        if (overlay.contains("vsync")) config.overlay.vsync = overlay["vsync"];
    }
}

void ConfigManager::ToJSON(json& j, const MainConfig& config) {
    j["menu"]["opacity"] = config.menu.opacity;
    j["menu"]["scale"] = config.menu.scale;
    j["menu"]["opened"] = config.menu.opened;
    
    j["overlay"]["enabled"] = config.overlay.enabled;
    j["overlay"]["clickthrough_when_closed"] = config.overlay.clickthroughWhenClosed;
    j["overlay"]["focus_only"] = config.overlay.focusOnly;
    j["overlay"]["fps"] = config.overlay.fps;
    j["overlay"]["stats"] = config.overlay.stats;
    j["overlay"]["watermark"] = config.overlay.watermark;
    j["overlay"]["vsync"] = config.overlay.vsync;
}

void ConfigManager::FromJSON(const json& j, VisualConfig& config) {
    // Реализация загрузки визуальных настроек
    // Временная заглушка
}

void ConfigManager::ToJSON(json& j, const VisualConfig& config) {
    // Реализация сохранения визуальных настроек
    // Временная заглушка
}

bool ConfigManager::ValidateConfig(const MainConfig& config) {
    // Валидация основных настроек
    if (config.menu.opacity < 0.0f || config.menu.opacity > 1.0f) {
        SPDLOG_WARN("Некорректная прозрачность меню: {}", config.menu.opacity);
        return false;
    }
    
    if (config.menu.scale < 0.5f || config.menu.scale > 2.0f) {
        SPDLOG_WARN("Некорректный масштаб меню: {}", config.menu.scale);
        return false;
    }
    
    return true;
}

bool ConfigManager::ValidateConfig(const VisualConfig& config) {
    // Валидация визуальных настроек
    // Временная заглушка
    return true;
}

bool ConfigManager::MigrateConfig(json& data, int fromVersion, int toVersion) {
    // Миграция конфигураций между версиями
    // Временная заглушка
    return true;
}

bool ConfigManager::CreateBackup(const std::string& configName) {
    // Создание резервной копии конфигурации
    // Временная заглушка
    return true;
}

bool ConfigManager::RestoreBackup(const std::string& configName, const std::string& backupName) {
    // Восстановление из резервной копии
    // Временная заглушка
    return true;
}

std::vector<std::string> ConfigManager::GetBackups(const std::string& configName) const {
    // Получение списка резервных копий
    // Временная заглушка
    return {};
}
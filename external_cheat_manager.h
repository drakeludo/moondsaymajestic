#pragma once
#include "external_memory_manager.h"
#ifdef DrawText
#undef DrawText
#endif
#include "aimbot.h"
#include "esp.h"
#include "weapon_mods.h"
#include "overlay.h"
#include "config_manager.h"
#include <memory>
#include <spdlog/spdlog.h>

// Forward declaration для глобального указателя
class ExternalCheatManager;
extern ExternalCheatManager* g_cheatManager;

class ExternalCheatManager {
public:
    static ExternalCheatManager& GetInstance();
    
    bool Initialize();
    void Shutdown();
    void Update();
    void Render();
    
    // Состояние
    bool IsInitialized() const { return m_initialized; }
    bool IsMenuOpen() const { return m_menuOpen; }
    void ToggleMenu() { m_menuOpen = !m_menuOpen; }
    
    // Модули
    std::shared_ptr<Aimbot> GetAimbot() const { return m_aimbot; }
    std::shared_ptr<ESP> GetESP() const { return m_esp; }
    std::shared_ptr<WeaponMods> GetWeaponMods() const { return m_weaponMods; }
    std::shared_ptr<Overlay> GetOverlay() const { return m_overlay; }
    std::shared_ptr<ConfigManager> GetConfigManager() const { return m_configManager; }
    std::shared_ptr<ExternalMemoryManager> GetMemoryManager() const { return m_memoryManager; }
    
    // External cheat методы
    bool AttachToGame(const std::string& processName);
    bool DetachFromGame();
    bool IsGameAttached() const { return m_gameAttached; }
    
    // Утилиты для external доступа
    uintptr_t GetPlayerBaseAddress();
    uintptr_t GetWorldAddress();
    uintptr_t GetCameraAddress();
    
    // Чтение игровых данных
    Vector3 GetPlayerPosition();
    Vector3 GetCameraPosition();
    Vector3 GetViewAngles();
    std::vector<uintptr_t> GetPlayerList();
    std::vector<uintptr_t> GetVehicleList();
    
private:
    static ExternalCheatManager* s_instance;

    ExternalCheatManager();
    ~ExternalCheatManager();
    
    bool m_initialized = false;
    bool m_gameAttached = false;
    bool m_menuOpen = false;
    
    std::shared_ptr<ExternalMemoryManager> m_memoryManager;
    std::shared_ptr<Aimbot> m_aimbot;
    std::shared_ptr<ESP> m_esp;
    std::shared_ptr<WeaponMods> m_weaponMods;
    std::shared_ptr<Overlay> m_overlay;
    std::shared_ptr<ConfigManager> m_configManager;
    
    // Игровые адреса (кэшированные)
    uintptr_t m_playerBase = 0;
    uintptr_t m_worldAddress = 0;
    uintptr_t m_cameraAddress = 0;
    
    // Запрет копирования
    ExternalCheatManager(const ExternalCheatManager&) = delete;
    ExternalCheatManager& operator=(const ExternalCheatManager&) = delete;
    
    // Вспомогательные методы
    bool FindGameAddresses();
    bool CacheGameAddresses();
    bool ValidateGameAddresses();
    
    // External чтение данных
    bool ReadGameStructure(uintptr_t address, void* structure, size_t size);
    bool WriteGameStructure(uintptr_t address, const void* structure, size_t size);
    
    // Поиск объектов в игре
    bool ScanForPlayers();
    bool ScanForVehicles();
    bool ScanForObjects();
    
    // Обновление кэша
    void UpdatePlayerCache();
    void UpdateVehicleCache();
    void UpdateObjectCache();
    
    // Кэшированные данные
    std::vector<uintptr_t> m_cachedPlayers;
    std::vector<uintptr_t> m_cachedVehicles;
    std::vector<uintptr_t> m_cachedObjects;
    
    float m_lastCacheUpdate = 0.0f;
    const float CACHE_UPDATE_INTERVAL = 0.5f; // секунды
};

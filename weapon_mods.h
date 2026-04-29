#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class WeaponMods {
public:
    WeaponMods();
    ~WeaponMods();
    
    // Настройки
    struct Settings {
        bool noRecoil = true;
        bool noSpread = true;
        bool oneShotKill = false;
        bool neverWanted = true;
        bool infiniteAmmo = false;
        bool rapidFire = false;
        
        // Модификации конкретного оружия
        struct WeaponSettings {
            float damageMultiplier = 1.0f;
            float recoilMultiplier = 0.0f;
            float spreadMultiplier = 0.0f;
            float fireRateMultiplier = 1.0f;
            float rangeMultiplier = 1.0f;
            bool instantReload = false;
        };
        
        std::unordered_map<std::string, WeaponSettings> weaponSpecific;
    };
    
    // Инициализация
    bool Initialize();
    void Shutdown();
    
    // Обновление
    void Update();
    
    // Применение модификаций
    void ApplyMods();
    void ApplyNoRecoil();
    void ApplyNoSpread();
    void ApplyOneShotKill();
    void ApplyNeverWanted();
    void ApplyInfiniteAmmo();
    void ApplyRapidFire();
    
    // Геттеры/сеттеры
    const Settings& GetSettings() const { return m_settings; }
    void SetSettings(const Settings& settings) { m_settings = settings; }
    
    // Утилиты
    std::string GetCurrentWeapon() const;
    bool IsWeaponValid(const std::string& weapon) const;
    
private:
    Settings m_settings;
    bool m_initialized = false;
    
    // Текущее состояние
    std::string m_currentWeapon;
    uintptr_t m_playerPed;
    uintptr_t m_weaponManager;
    
    // Оригинальные значения (для восстановления)
    struct OriginalValues {
        float recoil;
        float spread;
        float damage;
        float fireRate;
        float range;
        int ammo;
        int maxAmmo;
        bool infiniteAmmo;
    };
    
    std::unordered_map<std::string, OriginalValues> m_originalValues;
    
    // Вспомогательные методы
    bool GetPlayerPed();
    bool GetWeaponManager();
    bool GetCurrentWeaponData();
    void SaveOriginalValues(const std::string& weapon);
    void RestoreOriginalValues(const std::string& weapon);
    void PatchMemory(uintptr_t address, const std::vector<uint8_t>& bytes);
    void RestoreMemory(uintptr_t address, const std::vector<uint8_t>& original);
    
    // Патчи
    std::unordered_map<uintptr_t, std::vector<uint8_t>> m_appliedPatches;
    std::unordered_map<uintptr_t, std::vector<uint8_t>> m_originalPatches;
};
#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include "math/vector.h"

struct TargetInfo {
    uintptr_t address;
    Vector3 position;
    Vector3 headPosition;
    float distance;
    bool isVisible;
    bool isFriend;
    std::string name;
    int health;
    int armor;
};

class Aimbot {
public:
    Aimbot();
    ~Aimbot();
    
    // Настройки
    struct Settings {
        // Основные
        bool enabled = true;
        int key = 0x06; // VK_XBUTTON1
        bool toggleMode = false;
        
        // FOV
        float fov = 90.0f;
        bool fovCheck = true;
        bool fovDraw = true;
        float fovColor[4] = {1.0f, 0.0f, 0.0f, 0.3f};
        
        // Сглаживание
        float smooth = 0.5f;
        bool adaptiveSmooth = true;
        float smoothMin = 0.1f;
        float smoothMax = 2.0f;
        
        // No Recoil
        bool noRecoil = true;
        float recoilMultiplier = 0.0f;  // 0.0 = нет отдачи, 1.0 = обычная
        
        // Цель
        std::string targetBone = "Head";
        float maxDistance = 300.0f;
        
        // Фильтры
        bool playersOnly = true;
        bool ignoreFriends = true;
        bool ignoreDead = true;
        
        // Визуальные
        bool drawTarget = true;
        float targetColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};
        bool drawDistance = true;
    };
    
    // Инициализация
    bool Initialize();
    void Shutdown();
    
    // Обновление
    void Update();
    void FindTargets();
    void CalculateAimAngle();
    
    // Рендеринг
    void Render();
    
    // Геттеры/сеттеры
    const Settings& GetSettings() const;
    void SetSettings(const Settings& settings);
    
    bool IsActive() const;
    const TargetInfo* GetCurrentTarget() const;
    
private:
    Settings m_settings;
    bool m_active = false;
    bool m_initialized = false;
    
    std::vector<TargetInfo> m_targets;
    const TargetInfo* m_currentTarget = nullptr;
    Vector3 m_aimAngle;
};

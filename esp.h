#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include "math/vector.h"

struct ESPObject {
    enum class Type {
        Player,
        Vehicle,
        Pickup,
        Object,
        NPC
    };
    
    Type type;
    uintptr_t address;
    Vector3 position;
    Vector3 screenPosition;
    std::string name;
    float distance;
    bool isVisible;
    bool isFriend;
    
    // Дополнительные данные
    struct {
        struct {
            int health;
            int armor;
            int wantedLevel;
            std::string weapon;
        } player;
        
        struct {
            std::string model;
            float health;
            int seats;
        } vehicle;
        
        struct {
            std::string itemName;
            int amount;
        } pickup;
    };
};

class ESP {
public:
    ESP();
    ~ESP();
    
    // Настройки
    struct Settings {
        bool enabled = true;
        
        struct {
            bool enable = true;
            bool box = true;
            bool bone = false;
            bool text = true;
            bool excludeSelf = true;
            float maxDistance = 300.0f;
        } ped;
        
        struct {
            bool enable = true;
            bool line = true;
            bool text = true;
            float maxDistance = 500.0f;
        } vehicle;
        
        struct {
            bool enable = true;
            bool line = true;
            bool text = true;
            float maxDistance = 200.0f;
        } pickup;
        
        struct {
            bool enable = true;
            bool line = true;
            bool text = true;
            float maxDistance = 150.0f;
        } object;
        
        // Цвета
        struct {
            float ped[4] = {0.0f, 1.0f, 1.0f, 1.0f};
            float vehicle[4] = {1.0f, 1.0f, 0.0f, 1.0f};
            float pickup[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            float object[4] = {1.0f, 0.0f, 1.0f, 1.0f};
            float friendColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            float self[4] = {1.0f, 0.5f, 0.0f, 1.0f};
        } colors;
    };
    
    // Инициализация
    bool Initialize();
    void Shutdown();
    
    // Обновление
    void Update();
    void FindObjects();
    
    // Рендеринг
    void Render();
    void DrawBox(const ESPObject& obj);
    void DrawLine(const ESPObject& obj);
    void DrawText(const ESPObject& obj);
    void DrawBone(const ESPObject& obj);
    
    // Геттеры/сеттеры
    const Settings& GetSettings() const { return m_settings; }
    void SetSettings(const Settings& settings) { m_settings = settings; }
    
    const std::vector<ESPObject>& GetObjects() const { return m_objects; }
    
private:
    Settings m_settings;
    bool m_initialized = false;
    
    std::vector<ESPObject> m_objects;
    
    // Вспомогательные методы
    bool WorldToScreen(const Vector3& worldPos, Vector3& screenPos) const;
    float CalculateDistance(const Vector3& pos1, const Vector3& pos2) const;
    bool IsVisible(const ESPObject& obj) const;
    std::string GetObjectName(const ESPObject& obj) const;
    void GetBonePositions(const ESPObject& obj, std::vector<Vector3>& bones) const;
    
    // Обнаружение объектов
    void FindPlayers();
    void FindVehicles();
    void FindPickups();
    void FindWorldObjects();
};

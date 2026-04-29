#include "esp.h"
#include "game_offsets.h"
#include "external_cheat_manager.h"
#include "math/vector.h"
#include <spdlog/spdlog.h>
#include <algorithm>

#ifdef DrawText
#undef DrawText
#endif

// Глобальный указатель на менеджер читов
extern ExternalCheatManager* g_cheatManager;

ESP::ESP() {
    SPDLOG_INFO("Создание ESP");
}

ESP::~ESP() {
    SPDLOG_INFO("Уничтожение ESP");
    Shutdown();
}

bool ESP::Initialize() {
    if (m_initialized) {
        SPDLOG_WARN("ESP уже инициализирован");
        return true;
    }
    
    SPDLOG_INFO("Инициализация ESP для Alt:V");
    
    try {
        // Настройки по умолчанию
        m_settings.enabled = true;
        
        // Настройки пешеходов
        m_settings.ped.enable = true;
        m_settings.ped.box = true;
        m_settings.ped.bone = false;
        m_settings.ped.text = true;
        m_settings.ped.excludeSelf = true;
        m_settings.ped.maxDistance = 300.0f;
        
        // Настройки транспорта
        m_settings.vehicle.enable = true;
        m_settings.vehicle.line = true;
        m_settings.vehicle.text = true;
        m_settings.vehicle.maxDistance = 500.0f;
        
        // Настройки предметов
        m_settings.pickup.enable = true;
        m_settings.pickup.line = true;
        m_settings.pickup.text = true;
        m_settings.pickup.maxDistance = 200.0f;
        
        // Настройки объектов
        m_settings.object.enable = true;
        m_settings.object.line = true;
        m_settings.object.text = true;
        m_settings.object.maxDistance = 150.0f;
        
        // Цвета по умолчанию
        // Ped color (cyan)
        m_settings.colors.ped[0] = 0.0f; m_settings.colors.ped[1] = 1.0f;
        m_settings.colors.ped[2] = 1.0f; m_settings.colors.ped[3] = 1.0f;
        
        // Vehicle color (yellow)
        m_settings.colors.vehicle[0] = 1.0f; m_settings.colors.vehicle[1] = 1.0f;
        m_settings.colors.vehicle[2] = 0.0f; m_settings.colors.vehicle[3] = 1.0f;
        
        // Pickup color (green)
        m_settings.colors.pickup[0] = 0.0f; m_settings.colors.pickup[1] = 1.0f;
        m_settings.colors.pickup[2] = 0.0f; m_settings.colors.pickup[3] = 1.0f;
        
        // Object color (magenta)
        m_settings.colors.object[0] = 1.0f; m_settings.colors.object[1] = 0.0f;
        m_settings.colors.object[2] = 1.0f; m_settings.colors.object[3] = 1.0f;
        
        // Friend color (blue)
        m_settings.colors.friendColor[0] = 0.0f; m_settings.colors.friendColor[1] = 0.0f;
        m_settings.colors.friendColor[2] = 1.0f; m_settings.colors.friendColor[3] = 1.0f;
        
        // Self color (orange)
        m_settings.colors.self[0] = 1.0f; m_settings.colors.self[1] = 0.5f;
        m_settings.colors.self[2] = 0.0f; m_settings.colors.self[3] = 1.0f;
        
        m_initialized = true;
        SPDLOG_INFO("ESP успешно инициализирован");
        return true;
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при инициализации ESP: {}", e.what());
        return false;
    }
}

void ESP::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    SPDLOG_INFO("Завершение работы ESP");
    m_initialized = false;
    m_objects.clear();
}

void ESP::Update() {
    if (!m_initialized || !m_settings.enabled) {
        return;
    }
    
    // Поиск объектов
    FindObjects();
}

void ESP::FindObjects() {
    m_objects.clear();
    
    // Поиск игроков
    if (m_settings.ped.enable) {
        FindPlayers();
    }
    
    // Поиск транспорта
    if (m_settings.vehicle.enable) {
        FindVehicles();
    }
    
    // Поиск предметов
    if (m_settings.pickup.enable) {
        FindPickups();
    }
    
    // Поиск объектов
    if (m_settings.object.enable) {
        FindWorldObjects();
    }
    
    // Фильтрация по расстоянию
    auto it = std::remove_if(m_objects.begin(), m_objects.end(),
        [this](const ESPObject& obj) {
            switch (obj.type) {
                case ESPObject::Type::Player:
                    return obj.distance > m_settings.ped.maxDistance;
                case ESPObject::Type::Vehicle:
                    return obj.distance > m_settings.vehicle.maxDistance;
                case ESPObject::Type::Pickup:
                    return obj.distance > m_settings.pickup.maxDistance;
                case ESPObject::Type::Object:
                    return obj.distance > m_settings.object.maxDistance;
                default:
                    return false;
            }
        });
    
    m_objects.erase(it, m_objects.end());
}

void ESP::Render() {
    if (!m_initialized || !m_settings.enabled) {
        return;
    }
    
    for (const auto& obj : m_objects) {
        // Пропуск объектов за пределами экрана
        if (obj.screenPosition.z <= 0.0f) {
            continue;
        }
        
        // Выбор метода отрисовки в зависимости от типа
        switch (obj.type) {
            case ESPObject::Type::Player:
                if (m_settings.ped.box) {
                    DrawBox(obj);
                }
                if (m_settings.ped.bone) {
                    DrawBone(obj);
                }
                if (m_settings.ped.text) {
                    DrawText(obj);
                }
                break;
                
            case ESPObject::Type::Vehicle:
                if (m_settings.vehicle.line) {
                    DrawLine(obj);
                }
                if (m_settings.vehicle.text) {
                    DrawText(obj);
                }
                break;
                
            case ESPObject::Type::Pickup:
                if (m_settings.pickup.line) {
                    DrawLine(obj);
                }
                if (m_settings.pickup.text) {
                    DrawText(obj);
                }
                break;
                
            case ESPObject::Type::Object:
                if (m_settings.object.line) {
                    DrawLine(obj);
                }
                if (m_settings.object.text) {
                    DrawText(obj);
                }
                break;
                
            default:
                break;
        }
    }
}

void ESP::DrawBox(const ESPObject& obj) {
    // Красивый бокс с закруглёнными углами
    float* c = m_settings.colors.ped;
    ImGui::GetBackgroundDrawList()->AddRect(
        ImVec2(obj.screenPosition.x - 35, obj.screenPosition.y - 75),
        ImVec2(obj.screenPosition.x + 35, obj.screenPosition.y + 35),
        ImColor(c[0], c[1], c[2], c[3]),
        6.0f, 0, 2.5f
    );
}

void ESP::DrawLine(const ESPObject& obj) {
    // Линия от центра экрана к объекту
    float* c = m_settings.colors.ped;
    ImGui::GetBackgroundDrawList()->AddLine(
        ImVec2(640.0f, 360.0f), // Центр экрана (1280x720)
        ImVec2(obj.screenPosition.x, obj.screenPosition.y),
        ImColor(c[0], c[1], c[2], c[3]),
        1.5f
    );
}

void ESP::DrawText(const ESPObject& obj) {
    // Текст с информацией об объекте
    std::string txt = obj.name + " [" + std::to_string(obj.player.health) + "]";
    
    ImGui::GetBackgroundDrawList()->AddText(
        ImVec2(obj.screenPosition.x - 40, obj.screenPosition.y - 90),
        IM_COL32(255, 255, 255, 255),
        txt.c_str()
    );
}

void ESP::DrawBone(const ESPObject& obj) {
    // Простой скелет (вертикальная линия)
    float* c = m_settings.colors.ped;
    ImGui::GetBackgroundDrawList()->AddLine(
        ImVec2(obj.screenPosition.x, obj.screenPosition.y - 55),
        ImVec2(obj.screenPosition.x, obj.screenPosition.y + 25),
        ImColor(c[0], c[1], c[2], 0.9f),
        2.5f
    );
}

bool ESP::WorldToScreen(const Vector3& worldPos, Vector3& screenPos) const {
    if (!g_cheatManager || !g_cheatManager->IsGameAttached()) {
        screenPos = Vector3(0, 0, 0);
        return false;
    }
    
    auto mem = g_cheatManager->GetMemoryManager();
    if (!mem) {
        screenPos = Vector3(0, 0, 0);
        return false;
    }
    
    // Получаем адрес камеры
    uintptr_t cameraAddress = g_cheatManager->GetCameraAddress();
    if (!cameraAddress) {
        screenPos = Vector3(0, 0, 0);
        return false;
    }
    
    // Читаем позицию камеры
    Vector3 cameraPos = mem->Read<Vector3>(cameraAddress + Offsets::Camera::Position);
    
    // Читаем углы камеры (view matrix)
    Vector3 viewAngles = mem->Read<Vector3>(cameraAddress + Offsets::Camera::ViewAngles);
    
    // Вычисляем относительную позицию
    Vector3 delta;
    delta.x = worldPos.x - cameraPos.x;
    delta.y = worldPos.y - cameraPos.y;
    delta.z = worldPos.z - cameraPos.z;
    
    // Простая проекция (улучшенная версия)
    // Используем углы камеры для правильной проекции
    float distance = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    
    if (distance < 0.1f) {
        screenPos = Vector3(0, 0, -1);
        return false;
    }
    
    // Проекция на экран (1280x720 по умолчанию)
    const float screenWidth = 1280.0f;
    const float screenHeight = 720.0f;
    const float fov = 90.0f; // Field of view
    
    // Вычисляем углы к объекту
    float angleX = atan2f(delta.y, delta.x);
    float angleY = atan2f(delta.z, sqrtf(delta.x * delta.x + delta.y * delta.y));
    
    // Нормализуем относительно углов камеры
    float relativeAngleX = angleX - viewAngles.y;
    float relativeAngleY = angleY - viewAngles.x;
    
    // Проверка что объект перед камерой
    float dotProduct = delta.x * cosf(viewAngles.y) + delta.y * sinf(viewAngles.y);
    if (dotProduct < 0) {
        screenPos = Vector3(0, 0, -1);
        return false;
    }
    
    // Проекция на экран
    float fovScale = tanf(fov * 0.5f * 3.14159f / 180.0f);
    screenPos.x = screenWidth * 0.5f + (relativeAngleX / fovScale) * screenWidth * 0.5f;
    screenPos.y = screenHeight * 0.5f - (relativeAngleY / fovScale) * screenHeight * 0.5f;
    screenPos.z = distance;
    
    // Проверка что объект в пределах экрана
    if (screenPos.x < 0 || screenPos.x > screenWidth || 
        screenPos.y < 0 || screenPos.y > screenHeight) {
        return false;
    }
    
    return true;
}

float ESP::CalculateDistance(const Vector3& pos1, const Vector3& pos2) const {
    return pos1.Distance(pos2);
}

bool ESP::IsVisible(const ESPObject& obj) const {
    // Здесь будет проверка видимости объекта
    // Временная заглушка
    return obj.isVisible;
}

std::string ESP::GetObjectName(const ESPObject& obj) const {
    switch (obj.type) {
        case ESPObject::Type::Player:
            return obj.name.empty() ? "Player" : obj.name;
        case ESPObject::Type::Vehicle:
            return obj.vehicle.model.empty() ? "Vehicle" : obj.vehicle.model;
        case ESPObject::Type::Pickup:
            return obj.pickup.itemName.empty() ? "Pickup" : obj.pickup.itemName;
        case ESPObject::Type::Object:
            return "Object";
        default:
            return "Unknown";
    }
}

void ESP::GetBonePositions(const ESPObject& obj, std::vector<Vector3>& bones) const {
    // Здесь будет получение позиций костей
    // Временная заглушка
    bones.clear();
    if (obj.type == ESPObject::Type::Player) {
        bones.push_back(obj.position);
    }
}

void ESP::FindPlayers() {
    // КРИТИЧЕСКАЯ ОШИБКА ИСПРАВЛЕНА: удален m_objects.clear() из FindPlayers()
    // Теперь объекты накапливаются, а не перезаписываются
    
    if (!g_cheatManager || !g_cheatManager->IsGameAttached()) {
        return;
    }
    
    auto mem = g_cheatManager->GetMemoryManager();
    if (!mem) {
        return;
    }
    
    uintptr_t gameBase = mem->GetGameBase();
    if (!gameBase) {
        return;
    }
    
    // Чтение World
    uintptr_t world = mem->Read<uintptr_t>(gameBase + Offsets::Base::World);
    if (!world) {
        return;
    }
    
    // Чтение ReplayInterface
    uintptr_t replay = mem->Read<uintptr_t>(world + 0x8);
    if (!replay) {
        return;
    }
    
    // Чтение PedInterface
    uintptr_t pedInterface = mem->Read<uintptr_t>(replay + 0x18);
    if (!pedInterface) {
        return;
    }
    
    // Чтение списка педов
    uintptr_t pedList = mem->Read<uintptr_t>(pedInterface + 0x100);
    int maxPeds = mem->Read<int>(pedInterface + 0x110);
    
    if (!pedList || maxPeds <= 0) {
        return;
    }
    
    Vector3 localPos = g_cheatManager->GetPlayerPosition();
    
    for (int i = 0; i < maxPeds && i < 300; i++) {
        uintptr_t ped = mem->Read<uintptr_t>(pedList + i * 0x10);
        if (!ped) {
            continue;
        }
        
        Vector3 pos = mem->Read<Vector3>(ped + Offsets::Ped::Position);
        float dist = pos.Distance(localPos);
        
        // Фильтр: минимум 3м чтобы не показывать себя
        if (dist > m_settings.ped.maxDistance || dist < 3.0f) {
            continue;
        }
        
        ESPObject obj{};
        obj.type = ESPObject::Type::Player;
        obj.address = ped;
        obj.position = pos;
        obj.distance = dist;
        obj.player.health = static_cast<int>(mem->Read<float>(ped + Offsets::Ped::Health));
        obj.player.armor = static_cast<int>(mem->Read<float>(ped + Offsets::Ped::Armor));
        obj.isVisible = true;
        obj.name = "Player";
        obj.isFriend = false;
        obj.player.wantedLevel = 0;
        obj.player.weapon = "";
        
        // WorldToScreen (пока упрощённый)
        WorldToScreen(pos, obj.screenPosition);
        
        m_objects.push_back(obj);
    }
    
    if (!m_objects.empty()) {
        SPDLOG_DEBUG("ESP: Found {} players", m_objects.size());
    }
}

void ESP::FindVehicles() {
    // Получение менеджера читов
    auto& cheatManager = ExternalCheatManager::GetInstance();
    if (!cheatManager.IsGameAttached()) {
        return;
    }
    
    auto memoryManager = cheatManager.GetMemoryManager();
    if (!memoryManager) {
        return;
    }
    
    // Получение локального игрока
    uintptr_t localPlayer = cheatManager.GetPlayerBaseAddress();
    if (!localPlayer) {
        return;
    }
    
    // Чтение позиции локального игрока
    Vector3 localPos;
    if (!memoryManager->ReadMemory(localPlayer + Offsets::Ped::Position, &localPos, sizeof(Vector3))) {
        return;
    }
    
    // Получение списка транспорта
    std::vector<uintptr_t> vehicles = cheatManager.GetVehicleList();
    
    for (uintptr_t vehicleAddress : vehicles) {
        if (vehicleAddress == 0) {
            continue;
        }
        
        // Чтение позиции транспорта
        Vector3 vehiclePos;
        if (!memoryManager->ReadMemory(vehicleAddress + Offsets::Vehicle::Position, &vehiclePos, sizeof(Vector3))) {
            continue;
        }
        
        // Расчет расстояния
        float distance = localPos.Distance(vehiclePos);
        
        // Проверка максимальной дистанции
        if (distance > m_settings.vehicle.maxDistance) {
            continue;
        }
        
        // Чтение здоровья транспорта
        float health = 0.0f;
        memoryManager->ReadMemory(vehicleAddress + Offsets::Vehicle::Health, &health, sizeof(float));
        
        // TODO: Преобразование мировых координат в экранные
        Vector3 screenPos = vehiclePos; // Временно используем мировые координаты
        
        // Создание объекта ESP
        ESPObject vehicle;
        vehicle.type = ESPObject::Type::Vehicle;
        vehicle.address = vehicleAddress;
        vehicle.position = vehiclePos;
        vehicle.screenPosition = screenPos;
        vehicle.name = "Vehicle";
        vehicle.distance = distance;
        vehicle.isVisible = true; // TODO: Добавить проверку видимости
        vehicle.isFriend = false;
        vehicle.vehicle.model = "Unknown";
        vehicle.vehicle.health = health;
        vehicle.vehicle.seats = 4;
        
        if (IsVisible(vehicle)) {
            m_objects.push_back(vehicle);
        }
    }
}

void ESP::FindPickups() {
    // Здесь будет реализация поиска предметов
    // Временная заглушка для тестирования
    ESPObject pickup;
    pickup.type = ESPObject::Type::Pickup;
    pickup.address = 0xABCDEF12;
    pickup.position = Vector3(15.0f, 25.0f, 1.0f);
    pickup.screenPosition = Vector3(300.0f, 400.0f, 1.0f);
    pickup.name = "Test Pickup";
    pickup.distance = 10.0f;
    pickup.isVisible = true;
    pickup.isFriend = false;
    pickup.pickup.itemName = "Health Pack";
    pickup.pickup.amount = 1;
    
    if (IsVisible(pickup)) {
        m_objects.push_back(pickup);
    }
}

void ESP::FindWorldObjects() {
    // Здесь будет реализация поиска объектов
    // Временная заглушка для тестирования
    ESPObject object;
    object.type = ESPObject::Type::Object;
    object.address = 0xFEDCBA98;
    object.position = Vector3(5.0f, 10.0f, 3.0f);
    object.screenPosition = Vector3(200.0f, 100.0f, 1.0f);
    object.name = "Test Object";
    object.distance = 8.0f;
    object.isVisible = true;
    object.isFriend = false;
    
    if (IsVisible(object)) {
        m_objects.push_back(object);
    }
}

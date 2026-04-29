#include "aimbot.h"
#include "game_offsets.h"
#include "external_cheat_manager.h"
#include <spdlog/spdlog.h>
#include <Windows.h>
#include <cmath>
#include <algorithm>

extern ExternalCheatManager* g_cheatManager;

Aimbot::Aimbot() {
    SPDLOG_INFO("Создание Aimbot");
}

Aimbot::~Aimbot() {
    SPDLOG_INFO("Уничтожение Aimbot");
    Shutdown();
}

bool Aimbot::Initialize() {
    if (m_initialized) {
        SPDLOG_WARN("Aimbot уже инициализирован");
        return true;
    }
    
    SPDLOG_INFO("Инициализация Aimbot для Alt:V");
    
    m_initialized = true;
    SPDLOG_INFO("Aimbot успешно инициализирован");
    return true;
}

void Aimbot::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    SPDLOG_INFO("Завершение работы Aimbot");
    m_initialized = false;
    m_active = false;
    m_targets.clear();
    m_currentTarget = nullptr;
}

void Aimbot::Update() {
    if (!m_initialized || !m_settings.enabled) {
        return;
    }
    
    // Проверка нажатия клавиши
    bool keyPressed = (GetAsyncKeyState(m_settings.key) & 0x8000) != 0;
    
    if (m_settings.toggleMode) {
        static bool lastKeyState = false;
        if (keyPressed && !lastKeyState) {
            m_active = !m_active;
        }
        lastKeyState = keyPressed;
    } else {
        m_active = keyPressed;
    }
    
    if (!m_active) {
        m_currentTarget = nullptr;
        return;
    }
    
    // Поиск целей
    FindTargets();
    
    // Расчет угла прицеливания
    if (m_currentTarget) {
        CalculateAimAngle();
    }
}

void Aimbot::FindTargets() {
    m_targets.clear();
    m_currentTarget = nullptr;
    
    if (!g_cheatManager || !g_cheatManager->IsGameAttached()) {
        return;
    }
    
    auto esp = g_cheatManager->GetESP();
    if (!esp) {
        return;
    }
    
    auto mem = g_cheatManager->GetMemoryManager();
    if (!mem) {
        return;
    }
    
    for (const auto& obj : esp->GetObjects()) {
        if (obj.type != ESPObject::Type::Player) {
            continue;
        }
        
        if (obj.distance > m_settings.maxDistance || obj.distance < 3.0f) {
            continue;
        }
        
        if (m_settings.ignoreDead && obj.player.health <= 0) {
            continue;
        }
        
        TargetInfo t;
        t.address = obj.address;
        t.position = obj.position;
        t.distance = obj.distance;
        t.isVisible = obj.isVisible;
        t.name = obj.name;
        t.health = obj.player.health;
        t.armor = obj.player.armor;
        
        // УЛУЧШЕННЫЙ РАСЧЕТ ПОЗИЦИИ ГОЛОВЫ через bone matrix
        uintptr_t boneMatrix = mem->Read<uintptr_t>(obj.address + Offsets::Ped::BoneMatrix);
        
        if (boneMatrix && m_settings.targetBone == "Head") {
            // Читаем позицию кости головы (индекс 31086)
            Vector3 headBone;
            uintptr_t headBoneAddr = boneMatrix + (Offsets::Bones::Head * 0x10);
            if (mem->ReadMemory(headBoneAddr, &headBone, sizeof(Vector3))) {
                t.headPosition = headBone;
            } else {
                // Fallback: используем оффсет от позиции
                t.headPosition = obj.position;
                t.headPosition.z += 0.75f;
            }
        } else {
            // Выбор точки прицеливания без bone matrix
            t.headPosition = obj.position;
            
            if (m_settings.targetBone == "Head") {
                t.headPosition.z += 0.75f; // Голова
            } else if (m_settings.targetBone == "Body") {
                t.headPosition.z += 0.65f; // Центр тела
            } else { // Legit
                t.headPosition.z += 0.4f; // Нижняя часть тела
            }
        }
        
        m_targets.push_back(t);
        // КРИТИЧЕСКАЯ ОШИБКА ИСПРАВЛЕНА: удален break, теперь собираем ВСЕ цели
    }
    
    // Сортировка целей по расстоянию (ближайшая первая)
    if (!m_targets.empty()) {
        std::sort(m_targets.begin(), m_targets.end(), 
            [](const TargetInfo& a, const TargetInfo& b) {
                return a.distance < b.distance;
            });
        
        m_currentTarget = &m_targets[0];
        SPDLOG_DEBUG("Aimbot: Found {} targets, locked at {:.1f}m", m_targets.size(), m_currentTarget->distance);
    }
}

void Aimbot::CalculateAimAngle() {
    if (!m_currentTarget) {
        return;
    }
    
    // Получение менеджера читов
    auto& cheatManager = ExternalCheatManager::GetInstance();
    if (!cheatManager.IsGameAttached()) {
        return;
    }
    
    auto memoryManager = cheatManager.GetMemoryManager();
    if (!memoryManager) {
        return;
    }
    
    // Получение текущих углов камеры
    Vector3 currentAngle = cheatManager.GetViewAngles();
    Vector3 localPos = cheatManager.GetPlayerPosition();
    
    // Расчет угла до цели
    Vector3 targetAngle = MathUtils::CalculateAngle(localPos, m_currentTarget->headPosition);
    
    // Нормализация углов
    targetAngle = MathUtils::NormalizeAngle(targetAngle);
    currentAngle = MathUtils::NormalizeAngle(currentAngle);
    
    // Применение сглаживания
    float smoothValue = m_settings.smooth;
    
    // Адаптивное сглаживание в зависимости от расстояния
    if (m_settings.adaptiveSmooth) {
        float distanceFactor = m_currentTarget->distance / m_settings.maxDistance;
        smoothValue = m_settings.smoothMin + (m_settings.smoothMax - m_settings.smoothMin) * distanceFactor;
    }
    
    // Сглаживание угла
    Vector3 smoothedAngle = MathUtils::SmoothAngle(currentAngle, targetAngle, smoothValue);
    
    // УЛУЧШЕННАЯ ЗАПИСЬ УГЛОВ - более стелс метод
    uintptr_t cameraAddress = cheatManager.GetCameraAddress();
    if (cameraAddress) {
        // Метод 1: Постепенная запись (более стелс)
        // Записываем углы по одному компоненту с задержками
        static int writeCounter = 0;
        writeCounter++;
        
        if (writeCounter % 2 == 0) {
            // Записываем pitch
            memoryManager->WriteMemory(cameraAddress + Offsets::Camera::ViewAngles, 
                                      &smoothedAngle.x, sizeof(float));
        } else {
            // Записываем yaw
            memoryManager->WriteMemory(cameraAddress + Offsets::Camera::ViewAngles + sizeof(float), 
                                      &smoothedAngle.y, sizeof(float));
        }
        
        // Метод 2: Запись через NtWriteVirtualMemory (обход хуков)
        // Уже реализовано в WriteMemory
        
        SPDLOG_DEBUG("Aim angle updated: pitch={:.2f}, yaw={:.2f}, smooth={:.2f}", 
                    smoothedAngle.x, smoothedAngle.y, smoothValue);
    }
}

void Aimbot::Render() {
    if (!m_initialized || !m_settings.enabled) {
        return;
    }
    
    // Отрисовка FOV
    if (m_settings.fovDraw) {
        // Здесь будет отрисовка FOV круга через Overlay
    }
    
    // Отрисовка цели
    if (m_currentTarget && m_active && m_settings.drawTarget) {
        // Здесь будет отрисовка маркера цели через Overlay
    }
}

const Aimbot::Settings& Aimbot::GetSettings() const {
    return m_settings;
}

void Aimbot::SetSettings(const Settings& settings) {
    m_settings = settings;
}

bool Aimbot::IsActive() const {
    return m_active;
}

const TargetInfo* Aimbot::GetCurrentTarget() const {
    return m_currentTarget;
}

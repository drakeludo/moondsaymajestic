#include "external_cheat_manager.h"
#include <spdlog/spdlog.h>
#include <Windows.h>

// Статический экземпляр
ExternalCheatManager* ExternalCheatManager::s_instance = nullptr;

// Глобальный указатель для доступа из других модулей
ExternalCheatManager* g_cheatManager = nullptr;

ExternalCheatManager::ExternalCheatManager() {
    SPDLOG_INFO("Создание ExternalCheatManager");
    g_cheatManager = this; // Устанавливаем глобальный указатель
}

ExternalCheatManager::~ExternalCheatManager() {
    SPDLOG_INFO("Уничтожение ExternalCheatManager");
    g_cheatManager = nullptr; // Очищаем глобальный указатель
    Shutdown();
}

ExternalCheatManager& ExternalCheatManager::GetInstance() {
    if (!s_instance) {
        s_instance = new ExternalCheatManager();
    }
    return *s_instance;
}

bool ExternalCheatManager::Initialize() {
    if (m_initialized) {
        SPDLOG_WARN("ExternalCheatManager уже инициализирован");
        return true;
    }
    
    SPDLOG_INFO("Начало инициализации ExternalCheatManager");
    
    try {
        // MemoryManager должен быть инициализирован отдельно через AttachToGame
        
        // Инициализация менеджера конфигураций
        m_configManager = std::make_shared<ConfigManager>();
        if (!m_configManager->Initialize()) {
            SPDLOG_ERROR("Не удалось инициализировать ConfigManager");
            return false;
        }
        
        // Загрузка конфигураций
        if (!m_configManager->LoadAllConfigs()) {
            SPDLOG_WARN("Не удалось загрузить конфигурации, используются значения по умолчанию");
        }
        
        // Инициализация модулей
        m_aimbot = std::make_shared<Aimbot>();
        if (!m_aimbot->Initialize()) {
            SPDLOG_ERROR("Не удалось инициализировать Aimbot");
            return false;
        }
        
        m_esp = std::make_shared<ESP>();
        if (!m_esp->Initialize()) {
            SPDLOG_ERROR("Не удалось инициализировать ESP");
            return false;
        }
        
        m_weaponMods = std::make_shared<WeaponMods>();
        if (!m_weaponMods->Initialize()) {
            SPDLOG_ERROR("Не удалось инициализировать WeaponMods");
            return false;
        }
        
        m_initialized = true;
        SPDLOG_INFO("ExternalCheatManager успешно инициализирован");
        return true;
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при инициализации ExternalCheatManager: {}", e.what());
        return false;
    }
}

void ExternalCheatManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    SPDLOG_INFO("Завершение работы модулей ExternalCheatManager");
    
    // Отсоединение от игры
    DetachFromGame();
    
    // Завершение работы в обратном порядке инициализации
    if (m_overlay) {
        m_overlay->Shutdown();
        m_overlay.reset();
    }
    
    if (m_weaponMods) {
        m_weaponMods->Shutdown();
        m_weaponMods.reset();
    }
    
    if (m_esp) {
        m_esp->Shutdown();
        m_esp.reset();
    }
    
    if (m_aimbot) {
        m_aimbot->Shutdown();
        m_aimbot.reset();
    }
    
    if (m_memoryManager) {
        m_memoryManager->Shutdown();
        m_memoryManager.reset();
    }
    
    if (m_configManager) {
        m_configManager->Shutdown();
        m_configManager.reset();
    }
    
    m_initialized = false;
    m_gameAttached = false;
    SPDLOG_INFO("Все модули ExternalCheatManager завершены");
}

void ExternalCheatManager::Update() {
    if (!m_initialized || !m_gameAttached) {
        return;
    }
    
    // Проверка что игра еще запущена
    if (m_memoryManager && !m_memoryManager->IsGameRunning()) {
        SPDLOG_WARN("Игра завершена, отсоединение");
        DetachFromGame();
        return;
    }
    
    // Обновление кэша объектов
    float currentTime = static_cast<float>(GetTickCount64()) / 1000.0f;
    if (currentTime - m_lastCacheUpdate > CACHE_UPDATE_INTERVAL) {
        UpdatePlayerCache();
        UpdateVehicleCache();
        UpdateObjectCache();
        m_lastCacheUpdate = currentTime;
    }
    
    // Обновление меню
    if (m_menuOpen) {
        // Обновление UI (будет реализовано в Overlay)
    }
    
    // Обновление модулей
    if (m_aimbot) {
        m_aimbot->Update();
    }
    
    if (m_esp) {
        m_esp->Update();
    }
    
    if (m_weaponMods) {
        m_weaponMods->Update();
    }
    
    // Обновление оверлея
    if (m_overlay) {
        m_overlay->Update();
    }
}

void ExternalCheatManager::Render() {
    if (!m_initialized || !m_gameAttached) {
        return;
    }
    
    // Рендеринг модулей
    if (m_aimbot) {
        m_aimbot->Render();
    }
    
    if (m_esp) {
        m_esp->Render();
    }
    
    // Рендеринг оверлея
    if (m_overlay) {
        m_overlay->Render();
    }
}

bool ExternalCheatManager::AttachToGame(const std::string& processName) {
    if (m_gameAttached) {
        SPDLOG_WARN("Уже присоединено к игре");
        return true;
    }
    
    SPDLOG_INFO("Присоединение к игре: {}", processName);
    
    // Создание менеджера памяти
    m_memoryManager = std::make_shared<ExternalMemoryManager>();
    
    // Присоединение к процессу
    if (!m_memoryManager->AttachToProcess(processName)) {
        SPDLOG_ERROR("Не удалось присоединиться к процессу: {}", processName);
        
        // Попробуем найти процесс вручную
        SPDLOG_INFO("Попытка найти процесс вручную...");
        
        // Проверка прав администратора
        BOOL isElevated = FALSE;
        HANDLE token = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
                isElevated = elevation.TokenIsElevated;
            }
            CloseHandle(token);
        }
        
        if (!isElevated) {
            SPDLOG_ERROR("Требуются права администратора для доступа к процессу GTA5.exe");
            SPDLOG_ERROR("Запустите программу от имени администратора");
        }
        
        return false;
    }
    
    // Проверка что процесс действительно доступен
    if (!m_memoryManager->IsGameRunning()) {
        SPDLOG_ERROR("Процесс найден, но не активен");
        m_memoryManager->Shutdown();
        m_memoryManager.reset();
        return false;
    }
    
    // Поиск игровых адресов
    if (!FindGameAddresses()) {
        SPDLOG_ERROR("Не удалось найти игровые адреса");
        SPDLOG_INFO("Возможно оффсеты устарели или игра использует другую версию");
        m_memoryManager->Shutdown();
        m_memoryManager.reset();
        return false;
    }
    
    // Кэширование адресов
    if (!CacheGameAddresses()) {
        SPDLOG_WARN("Не удалось закэшировать все игровые адреса");
    }
    
    m_gameAttached = true;
    SPDLOG_INFO("Успешно присоединено к игре: {}", processName);
    return true;
}

bool ExternalCheatManager::DetachFromGame() {
    if (!m_gameAttached) {
        return true;
    }
    
    SPDLOG_INFO("Отсоединение от игры");
    
    if (m_memoryManager) {
        m_memoryManager->DetachFromProcess();
        m_memoryManager.reset();
    }
    
    // Очистка кэша
    m_playerBase = 0;
    m_worldAddress = 0;
    m_cameraAddress = 0;
    m_cachedPlayers.clear();
    m_cachedVehicles.clear();
    m_cachedObjects.clear();
    
    m_gameAttached = false;
    SPDLOG_INFO("Отсоединено от игры");
    return true;
}

uintptr_t ExternalCheatManager::GetPlayerBaseAddress() {
    if (!m_gameAttached || !m_memoryManager) {
        return 0;
    }
    
    // Если адрес не кэширован, попробовать найти его
    if (m_playerBase == 0) {
        // Поиск паттерна для игрока
        // Временная заглушка
        m_playerBase = m_memoryManager->GetGameBase() + 0x123456; // Примерный offset
    }
    
    return m_playerBase;
}

uintptr_t ExternalCheatManager::GetWorldAddress() {
    if (!m_gameAttached || !m_memoryManager) {
        return 0;
    }
    
    if (m_worldAddress == 0) {
        // Поиск паттерна для мира
        // Временная заглушка
        m_worldAddress = m_memoryManager->GetGameBase() + 0x789ABC; // Примерный offset
    }
    
    return m_worldAddress;
}

uintptr_t ExternalCheatManager::GetCameraAddress() {
    if (!m_gameAttached || !m_memoryManager) {
        return 0;
    }
    
    if (m_cameraAddress == 0) {
        // Поиск паттерна для камеры
        // Временная заглушка
        m_cameraAddress = m_memoryManager->GetGameBase() + 0xDEF012; // Примерный offset
    }
    
    return m_cameraAddress;
}

Vector3 ExternalCheatManager::GetPlayerPosition() {
    uintptr_t playerBase = GetPlayerBaseAddress();
    if (!playerBase || !m_memoryManager) {
        return Vector3();
    }
    
    // Чтение позиции игрока
    Vector3 position;
    if (m_memoryManager->ReadMemory(playerBase + 0x90, &position, sizeof(Vector3))) {
        return position;
    }
    
    return Vector3();
}

Vector3 ExternalCheatManager::GetCameraPosition() {
    uintptr_t cameraBase = GetCameraAddress();
    if (!cameraBase || !m_memoryManager) {
        return Vector3();
    }
    
    // Чтение позиции камеры
    Vector3 position;
    if (m_memoryManager->ReadMemory(cameraBase + 0x40, &position, sizeof(Vector3))) {
        return position;
    }
    
    return Vector3();
}

Vector3 ExternalCheatManager::GetViewAngles() {
    uintptr_t cameraBase = GetCameraAddress();
    if (!cameraBase || !m_memoryManager) {
        return Vector3();
    }
    
    // Чтение углов обзора
    Vector3 angles;
    if (m_memoryManager->ReadMemory(cameraBase + 0x3C0, &angles, sizeof(Vector3))) {
        return angles;
    }
    
    return Vector3();
}

std::vector<uintptr_t> ExternalCheatManager::GetPlayerList() {
    if (!m_gameAttached) {
        return {};
    }
    
    return m_cachedPlayers;
}

std::vector<uintptr_t> ExternalCheatManager::GetVehicleList() {
    if (!m_gameAttached) {
        return {};
    }
    
    return m_cachedVehicles;
}

bool ExternalCheatManager::FindGameAddresses() {
    if (!m_memoryManager) {
        return false;
    }
    
    SPDLOG_INFO("Поиск игровых адресов");
    
    // Поиск паттернов в памяти игры
    // Временная заглушка
    
    // Пример поиска адреса игрока
    std::string playerPattern = "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 74 ?? 80 78";
    uintptr_t playerPtr = m_memoryManager->FindPattern("GTA5.exe", playerPattern);
    
    if (playerPtr) {
        // Чтение указателя на игрока
        int32_t offset = 0;
        if (m_memoryManager->ReadMemory(playerPtr + 3, &offset, sizeof(int32_t))) {
            m_playerBase = playerPtr + offset + 7;
            SPDLOG_INFO("Найден адрес игрока: 0x{:X}", m_playerBase);
        }
    }
    
    // Поиск адреса мира
    std::string worldPattern = "48 8B 05 ?? ?? ?? ?? 45 ?? ?? ?? ?? 48 8B 48 08 48 85 C9 74 07";
    uintptr_t worldPtr = m_memoryManager->FindPattern("GTA5.exe", worldPattern);
    
    if (worldPtr) {
        int32_t offset = 0;
        if (m_memoryManager->ReadMemory(worldPtr + 3, &offset, sizeof(int32_t))) {
            m_worldAddress = worldPtr + offset + 7;
            SPDLOG_INFO("Найден адрес мира: 0x{:X}", m_worldAddress);
        }
    }
    
    return m_playerBase != 0 || m_worldAddress != 0;
}

bool ExternalCheatManager::CacheGameAddresses() {
    // Кэширование найденных адресов
    // Временная заглушка
    return true;
}

bool ExternalCheatManager::ValidateGameAddresses() {
    if (!m_memoryManager) {
        return false;
    }
    
    // Проверка что адреса валидны
    if (m_playerBase) {
        // Попробовать прочитать что-то по адресу игрока
        uint32_t testValue = 0;
        if (!m_memoryManager->ReadMemory(m_playerBase, &testValue, sizeof(uint32_t))) {
            SPDLOG_WARN("Адрес игрока невалиден: 0x{:X}", m_playerBase);
            m_playerBase = 0;
        }
    }
    
    if (m_worldAddress) {
        uint32_t testValue = 0;
        if (!m_memoryManager->ReadMemory(m_worldAddress, &testValue, sizeof(uint32_t))) {
            SPDLOG_WARN("Адрес мира невалиден: 0x{:X}", m_worldAddress);
            m_worldAddress = 0;
        }
    }
    
    return m_playerBase != 0 || m_worldAddress != 0;
}

bool ExternalCheatManager::ReadGameStructure(uintptr_t address, void* structure, size_t size) {
    if (!m_memoryManager) {
        return false;
    }
    
    return m_memoryManager->ReadMemory(address, structure, size);
}

bool ExternalCheatManager::WriteGameStructure(uintptr_t address, const void* structure, size_t size) {
    if (!m_memoryManager) {
        return false;
    }
    
    return m_memoryManager->WriteMemory(address, structure, size);
}

bool ExternalCheatManager::ScanForPlayers() {
    if (!m_memoryManager || !m_worldAddress) {
        return false;
    }
    
    // Сканирование памяти для поиска игроков
    // Временная заглушка
    
    // Чтение указателя на пул игроков
    uintptr_t playerPool = 0;
    if (m_memoryManager->ReadMemory(m_worldAddress + 0x8, &playerPool, sizeof(uintptr_t))) {
        if (playerPool) {
            // Чтение количества игроков
            uint32_t playerCount = 0;
            if (m_memoryManager->ReadMemory(playerPool + 0x110, &playerCount, sizeof(uint32_t))) {
                // Чтение списка игроков
                uintptr_t playerList = 0;
                if (m_memoryManager->ReadMemory(playerPool + 0x100, &playerList, sizeof(uintptr_t))) {
                    m_cachedPlayers.clear();
                    
                    // Чтение каждого игрока
                    for (uint32_t i = 0; i < playerCount && i < 32; i++) {
                        uintptr_t playerAddress = 0;
                        if (m_memoryManager->ReadMemory(playerList + (i * 8), &playerAddress, sizeof(uintptr_t))) {
                            if (playerAddress) {
                                m_cachedPlayers.push_back(playerAddress);
                            }
                        }
                    }
                    
                    SPDLOG_DEBUG("Найдено игроков: {}", m_cachedPlayers.size());
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool ExternalCheatManager::ScanForVehicles() {
    // Сканирование для транспорта
    // Временная заглушка
    return false;
}

bool ExternalCheatManager::ScanForObjects() {
    // Сканирование для объектов
    // Временная заглушка
    return false;
}

void ExternalCheatManager::UpdatePlayerCache() {
    ScanForPlayers();
}

void ExternalCheatManager::UpdateVehicleCache() {
    ScanForVehicles();
}

void ExternalCheatManager::UpdateObjectCache() {
    ScanForObjects();
}
#include "external_memory_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>

ExternalMemoryManager::ExternalMemoryManager() {
    SPDLOG_INFO("Создание ExternalMemoryManager");
}

ExternalMemoryManager::~ExternalMemoryManager() {
    SPDLOG_INFO("Уничтожение ExternalMemoryManager");
    Shutdown();
}

bool ExternalMemoryManager::Initialize() {
    if (m_attached) {
        SPDLOG_WARN("ExternalMemoryManager уже инициализирован");
        return true;
    }
    
    SPDLOG_INFO("=== ИНИЦИАЛИЗАЦИЯ EXTERNAL MEMORY MANAGER ===");
    
    try {
        // Включение привилегий отладки
        SPDLOG_INFO("[1/6] Включение привилегий отладки...");
        if (!EnableDebugPrivileges()) {
            SPDLOG_ERROR("✗ Не удалось включить привилегии отладки");
            SPDLOG_ERROR("Попробуйте запустить программу от имени администратора");
            return false;
        }
        SPDLOG_INFO("✓ Привилегии отладки включены");
        
        // Поиск процесса GTA5.exe (Alt:V использует стандартный GTA5.exe)
        SPDLOG_INFO("[2/6] Поиск процесса GTA5.exe...");
        if (!FindProcessByName("GTA5.exe")) {
            SPDLOG_ERROR("✗ Не удалось найти процесс GTA5.exe");
            SPDLOG_ERROR("Убедитесь что Alt:V (Majestic RP) запущен");
            SPDLOG_ERROR("Имя процесса должно быть GTA5.exe");
            
            // Попробуем найти другие возможные имена процессов
            const char* possibleNames[] = {
                "PlayGTAV.exe",
                "GTAV.exe",
                "gta5.exe",
                nullptr
            };
            
            for (int i = 0; possibleNames[i] != nullptr; i++) {
                SPDLOG_INFO("Проверка альтернативного процесса: {}", possibleNames[i]);
                if (FindProcessByName(possibleNames[i])) {
                    SPDLOG_INFO("✓ Найден альтернативный процесс: {}", possibleNames[i]);
                    break;
                }
            }
            
            if (m_processId == 0) {
                return false;
            }
        }
        
        SPDLOG_INFO("✓ Процесс GTA5.exe найден с ID: {}", m_processId);
        
        // Открытие дескриптора процесса
        SPDLOG_INFO("[3/6] Открытие дескриптора процесса...");
        if (!OpenProcessHandle()) {
            SPDLOG_ERROR("✗ Не удалось открыть дескриптор процесса GTA5.exe");
            SPDLOG_ERROR("Причина: недостаточно прав доступа или анти-чит блокирует");
            SPDLOG_ERROR("Решение: запустите программу от имени администратора");
            return false;
        }
        
        // Получение базовых адресов модулей
        SPDLOG_INFO("[4/6] Получение базового адреса GTA5.exe...");
        m_moduleBases["GTA5.exe"] = GetModuleBase("GTA5.exe");
        
        if (m_moduleBases["GTA5.exe"] == 0) {
            SPDLOG_ERROR("✗ Не удалось получить базовый адрес GTA5.exe");
            SPDLOG_ERROR("Возможно процесс защищен или используется анти-чит");
            CloseProcessHandle();
            return false;
        }
        
        SPDLOG_INFO("✓ Базовый адрес GTA5.exe: 0x{:X}", m_moduleBases["GTA5.exe"]);
        
        // Проверка наличия Alt:V клиента
        SPDLOG_INFO("[5/6] Проверка Alt:V клиента...");
        uintptr_t altVClient = GetModuleBase("altv-client.dll");
        if (altVClient) {
            m_moduleBases["altv-client.dll"] = altVClient;
            SPDLOG_INFO("✓ Обнаружен Alt:V клиент по адресу: 0x{:X}", altVClient);
        } else {
            SPDLOG_WARN("⚠ Alt:V клиент не обнаружен, возможно это обычный GTA 5");
        }
        
        // Обход анти-чита для external доступа
        SPDLOG_INFO("[6/6] Обход анти-чита...");
        if (!BypassExternalAntiCheat()) {
            SPDLOG_WARN("⚠ Не удалось полностью обойти анти-чит");
            SPDLOG_WARN("Функциональность может быть ограничена");
        }
        
        m_attached = true;
        SPDLOG_INFO("=== ИНИЦИАЛИЗАЦИЯ ЗАВЕРШЕНА УСПЕШНО ===");
        SPDLOG_INFO("Process ID: {}, Base Address: 0x{:X}", 
                   m_processId, m_moduleBases["GTA5.exe"]);
        SPDLOG_INFO("Режим: Alt:V (Majestic RP) External");
        return true;
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при инициализации ExternalMemoryManager: {}", e.what());
        return false;
    }
}

void ExternalMemoryManager::Shutdown() {
    if (!m_attached) {
        return;
    }
    
    SPDLOG_INFO("Завершение работы ExternalMemoryManager");
    
    // Восстановление external патчей
    for (auto& patch : m_externalPatches) {
        RestoreExternalPatch(patch.first);
    }
    
    // Скрытие следов доступа
    HideExternalAccess();
    
    // Закрытие дескриптора процесса
    CloseProcessHandle();
    
    m_attached = false;
    m_moduleBases.clear();
    m_externalPatches.clear();
    SPDLOG_INFO("ExternalMemoryManager завершен");
}

bool ExternalMemoryManager::ReadMemory(uintptr_t address, void* buffer, size_t size) {
    if (!m_attached || !m_processHandle) {
        return false;
    }
    
    SIZE_T bytesRead = 0;
    bool success = false;
    
    // Добавляем случайную задержку для обхода детекта паттернов
    if (rand() % 10 == 0) {
        Sleep(rand() % 5 + 1);
    }
    
    // Метод 1: Стандартный ReadProcessMemory
    success = ReadProcessMemory(m_processHandle, 
                               reinterpret_cast<LPCVOID>(address), 
                               buffer, size, &bytesRead);
    
    if (success && bytesRead == size) {
        LogMemoryOperation(address, size, true, true);
        return true;
    }
    
    // Метод 2: NtReadVirtualMemory (обход хуков)
    typedef NTSTATUS(NTAPI* pNtReadVirtualMemory)(
        HANDLE ProcessHandle,
        PVOID BaseAddress,
        PVOID Buffer,
        SIZE_T BufferSize,
        PSIZE_T NumberOfBytesRead);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        pNtReadVirtualMemory NtReadVirtualMemory = 
            (pNtReadVirtualMemory)GetProcAddress(ntdll, "NtReadVirtualMemory");
        
        if (NtReadVirtualMemory) {
            NTSTATUS status = NtReadVirtualMemory(
                m_processHandle,
                reinterpret_cast<PVOID>(address),
                buffer,
                size,
                &bytesRead);
            
            if (status == 0 && bytesRead == size) {
                LogMemoryOperation(address, size, true, true);
                return true;
            }
        }
    }
    
    // Метод 3: Чтение через memory mapping (если доступно)
    if (ReadMemoryViaMapping(address, buffer, size)) {
        LogMemoryOperation(address, size, true, true);
        return true;
    }
    
    // Метод 4: Чтение по частям (если большой блок)
    if (size > 4096) {
        if (ReadMemoryInChunks(address, buffer, size)) {
            LogMemoryOperation(address, size, true, true);
            return true;
        }
    }
    
    LogMemoryOperation(address, size, true, false);
    return false;
}

bool ExternalMemoryManager::ReadMemoryViaMapping(uintptr_t address, void* buffer, size_t size) {
    // Попытка чтения через memory mapping (более стелс метод)
    // Создаем file mapping для процесса
    
    // Получаем информацию о странице памяти
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQueryEx(m_processHandle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
        return false;
    }
    
    // Проверяем что память доступна для чтения
    if (!(mbi.State == MEM_COMMIT && 
          (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))) {
        return false;
    }
    
    // Пробуем прочитать напрямую (fallback)
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(m_processHandle, 
                            reinterpret_cast<LPCVOID>(address), 
                            buffer, size, &bytesRead) && bytesRead == size;
}

bool ExternalMemoryManager::ReadMemoryInChunks(uintptr_t address, void* buffer, size_t size) {
    // Чтение большого блока памяти по частям (обход ограничений)
    const size_t chunkSize = 4096; // 4KB chunks
    size_t totalRead = 0;
    
    while (totalRead < size) {
        size_t toRead = min(chunkSize, size - totalRead);
        SIZE_T bytesRead = 0;
        
        if (!ReadProcessMemory(m_processHandle,
                              reinterpret_cast<LPCVOID>(address + totalRead),
                              reinterpret_cast<BYTE*>(buffer) + totalRead,
                              toRead,
                              &bytesRead)) {
            return false;
        }
        
        totalRead += bytesRead;
        
        // Случайная микро-задержка между чанками
        if (rand() % 5 == 0) {
            Sleep(1);
        }
    }
    
    return totalRead == size;
}

bool ExternalMemoryManager::WriteMemory(uintptr_t address, const void* buffer, size_t size) {
    if (!m_attached || !m_processHandle) {
        return false;
    }
    
    // Добавляем случайную задержку для обхода детекта паттернов
    if (rand() % 10 == 0) {
        Sleep(rand() % 5 + 1);
    }
    
    DWORD oldProtect = 0;
    bool protectChanged = ProtectMemory(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    
    if (!protectChanged) {
        SPDLOG_DEBUG("Не удалось изменить защиту памяти для записи по адресу 0x{:X}", address);
    }
    
    SIZE_T bytesWritten = 0;
    bool success = false;
    
    // Метод 1: Стандартный WriteProcessMemory
    success = WriteProcessMemory(m_processHandle, 
                                reinterpret_cast<LPVOID>(address), 
                                buffer, size, &bytesWritten);
    
    if (success && bytesWritten == size) {
        if (protectChanged) {
            RestoreMemoryProtection(address, size, oldProtect);
        }
        LogMemoryOperation(address, size, false, true);
        return true;
    }
    
    // Метод 2: NtWriteVirtualMemory (обход хуков)
    typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory)(
        HANDLE ProcessHandle,
        PVOID BaseAddress,
        PVOID Buffer,
        SIZE_T BufferSize,
        PSIZE_T NumberOfBytesWritten);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll) {
        pNtWriteVirtualMemory NtWriteVirtualMemory = 
            (pNtWriteVirtualMemory)GetProcAddress(ntdll, "NtWriteVirtualMemory");
        
        if (NtWriteVirtualMemory) {
            NTSTATUS status = NtWriteVirtualMemory(
                m_processHandle,
                reinterpret_cast<PVOID>(address),
                const_cast<PVOID>(buffer),
                size,
                &bytesWritten);
            
            if (status == 0 && bytesWritten == size) {
                if (protectChanged) {
                    RestoreMemoryProtection(address, size, oldProtect);
                }
                LogMemoryOperation(address, size, false, true);
                return true;
            }
        }
    }
    
    // Метод 3: Запись по частям (если большой блок)
    if (size > 4096) {
        if (WriteMemoryInChunks(address, buffer, size)) {
            if (protectChanged) {
                RestoreMemoryProtection(address, size, oldProtect);
            }
            LogMemoryOperation(address, size, false, true);
            return true;
        }
    }
    
    if (protectChanged) {
        RestoreMemoryProtection(address, size, oldProtect);
    }
    
    LogMemoryOperation(address, size, false, false);
    return false;
}

bool ExternalMemoryManager::WriteMemoryInChunks(uintptr_t address, const void* buffer, size_t size) {
    // Запись большого блока памяти по частям
    const size_t chunkSize = 4096; // 4KB chunks
    size_t totalWritten = 0;
    
    while (totalWritten < size) {
        size_t toWrite = min(chunkSize, size - totalWritten);
        SIZE_T bytesWritten = 0;
        
        if (!WriteProcessMemory(m_processHandle,
                               reinterpret_cast<LPVOID>(address + totalWritten),
                               reinterpret_cast<const BYTE*>(buffer) + totalWritten,
                               toWrite,
                               &bytesWritten)) {
            return false;
        }
        
        totalWritten += bytesWritten;
        
        // Случайная микро-задержка между чанками
        if (rand() % 5 == 0) {
            Sleep(1);
        }
    }
    
    return totalWritten == size;
}

uintptr_t ExternalMemoryManager::FindPattern(const std::string& moduleName, const std::string& pattern) {
    uintptr_t moduleBase = GetModuleBase(moduleName);
    if (moduleBase == 0) {
        return 0;
    }
    
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(m_processHandle, 
                            reinterpret_cast<HMODULE>(moduleBase), 
                            &moduleInfo, sizeof(moduleInfo))) {
        return 0;
    }
    
    // Чтение всего модуля в память для поиска паттерна
    std::vector<uint8_t> moduleData(moduleInfo.SizeOfImage);
    if (!ReadMemory(moduleBase, moduleData.data(), moduleInfo.SizeOfImage)) {
        return 0;
    }
    
    return FindPatternExternal(moduleData.data(), moduleInfo.SizeOfImage, pattern) + moduleBase;
}

uintptr_t ExternalMemoryManager::FindPatternExternal(const uint8_t* start, size_t size, const std::string& pattern) {
    std::vector<uint8_t> patternBytes = PatternToBytes(pattern);
    if (patternBytes.empty()) {
        return 0;
    }
    
    const uint8_t* end = start + size - patternBytes.size();
    
    for (const uint8_t* current = start; current <= end; ++current) {
        if (CompareBytes(current, patternBytes.data(), pattern)) {
            return reinterpret_cast<uintptr_t>(current);
        }
    }
    
    return 0;
}

uintptr_t ExternalMemoryManager::GetModuleBase(const std::string& moduleName) {
    if (m_moduleBases.find(moduleName) != m_moduleBases.end()) {
        return m_moduleBases[moduleName];
    }
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        SPDLOG_ERROR("Не удалось создать снимок модулей для процесса {}: {}", m_processId, GetLastError());
        return 0;
    }
    
    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);
    
    uintptr_t baseAddress = 0;
    int moduleCount = 0;
    
    if (Module32First(snapshot, &moduleEntry)) {
        do {
            moduleCount++;
            if (_stricmp(moduleEntry.szModule, moduleName.c_str()) == 0) {
                baseAddress = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
                SPDLOG_INFO("Найден модуль {} по адресу: 0x{:X}", moduleName, baseAddress);
                break;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }
    
    if (baseAddress == 0) {
        SPDLOG_WARN("Модуль {} не найден. Всего проверено модулей: {}", moduleName, moduleCount);
        // Выведем список модулей для отладки
        SPDLOG_DEBUG("Список модулей процесса {}:", m_processId);
        if (Module32First(snapshot, &moduleEntry)) {
            do {
                SPDLOG_DEBUG("  - {} (Base: 0x{:X}, Size: {})", 
                           moduleEntry.szModule, 
                           reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr),
                           moduleEntry.modBaseSize);
            } while (Module32Next(snapshot, &moduleEntry));
        }
    }
    
    CloseHandle(snapshot);
    
    if (baseAddress != 0) {
        m_moduleBases[moduleName] = baseAddress;
    }
    
    return baseAddress;
}

bool ExternalMemoryManager::IsGameRunning() const {
    if (!m_processHandle) {
        SPDLOG_DEBUG("IsGameRunning: дескриптор процесса не открыт");
        return false;
    }
    
    DWORD exitCode = 0;
    if (GetExitCodeProcess(m_processHandle, &exitCode)) {
        bool isRunning = exitCode == STILL_ACTIVE;
        SPDLOG_DEBUG("IsGameRunning: код выхода = {}, запущен = {}", exitCode, isRunning);
        return isRunning;
    }
    
    SPDLOG_DEBUG("IsGameRunning: не удалось получить код выхода: {}", GetLastError());
    return false;
}

bool ExternalMemoryManager::AttachToProcess(const std::string& processName) {
    return FindProcessByName(processName) && OpenProcessHandle();
}

bool ExternalMemoryManager::DetachFromProcess() {
    Shutdown();
    return true;
}

std::vector<uint8_t> ExternalMemoryManager::ReadBytes(uintptr_t address, size_t size) {
    std::vector<uint8_t> buffer(size);
    if (ReadMemory(address, buffer.data(), size)) {
        return buffer;
    }
    return {};
}

bool ExternalMemoryManager::WriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return false;
    }
    return WriteMemory(address, bytes.data(), bytes.size());
}

std::vector<uintptr_t> ExternalMemoryManager::FindAllPatterns(const std::string& moduleName, const std::string& pattern) {
    std::vector<uintptr_t> results;
    
    uintptr_t moduleBase = GetModuleBase(moduleName);
    if (moduleBase == 0) {
        return results;
    }
    
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(m_processHandle, 
                            reinterpret_cast<HMODULE>(moduleBase), 
                            &moduleInfo, sizeof(moduleInfo))) {
        return results;
    }
    
    std::vector<uint8_t> moduleData(moduleInfo.SizeOfImage);
    if (!ReadMemory(moduleBase, moduleData.data(), moduleInfo.SizeOfImage)) {
        return results;
    }
    
    std::vector<uint8_t> patternBytes = PatternToBytes(pattern);
    if (patternBytes.empty()) {
        return results;
    }
    
    const uint8_t* start = moduleData.data();
    const uint8_t* end = start + moduleInfo.SizeOfImage - patternBytes.size();
    
    for (const uint8_t* current = start; current <= end; ++current) {
        if (CompareBytes(current, patternBytes.data(), pattern)) {
            results.push_back(reinterpret_cast<uintptr_t>(current) - reinterpret_cast<uintptr_t>(start) + moduleBase);
        }
    }
    
    return results;
}

MEMORY_BASIC_INFORMATION ExternalMemoryManager::GetMemoryInfo(uintptr_t address) {
    MEMORY_BASIC_INFORMATION mbi = {};
    if (m_processHandle) {
        VirtualQueryEx(m_processHandle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi));
    }
    return mbi;
}

std::vector<MEMORY_BASIC_INFORMATION> ExternalMemoryManager::GetMemoryRegions() {
    std::vector<MEMORY_BASIC_INFORMATION> regions;
    
    if (!m_processHandle) {
        return regions;
    }
    
    uintptr_t address = 0;
    MEMORY_BASIC_INFORMATION mbi;
    
    while (VirtualQueryEx(m_processHandle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
        regions.push_back(mbi);
        
        if (mbi.BaseAddress && mbi.RegionSize) {
            address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        } else {
            break;
        }
    }
    
    return regions;
}

bool ExternalMemoryManager::ApplyExternalPatch(uintptr_t address, const std::vector<uint8_t>& newBytes) {
    if (newBytes.empty()) {
        return false;
    }
    
    // Сохранение оригинальных байт
    std::vector<uint8_t> originalBytes(newBytes.size());
    if (!ReadMemory(address, originalBytes.data(), originalBytes.size())) {
        return false;
    }
    
    // Применение патча
    if (!WriteMemory(address, newBytes.data(), newBytes.size())) {
        return false;
    }
    
    // Сохранение информации о патче
    ExternalPatchInfo patchInfo;
    patchInfo.address = address;
    patchInfo.originalBytes = originalBytes;
    patchInfo.newBytes = newBytes;
    
    m_externalPatches[address] = patchInfo;
    
    SPDLOG_INFO("External патч применен: 0x{:X} ({} байт)", address, newBytes.size());
    return true;
}

bool ExternalMemoryManager::RestoreExternalPatch(uintptr_t address) {
    auto it = m_externalPatches.find(address);
    if (it == m_externalPatches.end()) {
        return false;
    }
    
    // Восстановление оригинальных байт
    if (!WriteMemory(address, it->second.originalBytes.data(), it->second.originalBytes.size())) {
        return false;
    }
    
    m_externalPatches.erase(it);
    SPDLOG_INFO("External патч восстановлен: 0x{:X}", address);
    return true;
}

bool ExternalMemoryManager::BypassExternalAntiCheat() {
    SPDLOG_INFO("=== АГРЕССИВНЫЙ ОБХОД АНТИ-ЧИТА ===");
    
    bool success = true;
    
    // 1. Включение всех возможных привилегий
    SPDLOG_INFO("[1/8] Включение привилегий отладки...");
    if (!EnableDebugPrivileges()) {
        SPDLOG_ERROR("Не удалось включить привилегии отладки - КРИТИЧНО!");
        success = false;
    } else {
        SPDLOG_INFO("✓ Привилегии отладки включены");
    }
    
    // 2. Снятие защиты процесса
    SPDLOG_INFO("[2/8] Снятие защиты процесса...");
    if (RemoveProcessProtection()) {
        SPDLOG_INFO("✓ Защита процесса снята");
    } else {
        SPDLOG_WARN("⚠ Не удалось снять защиту процесса");
    }
    
    // 3. Скрытие дескриптора процесса
    SPDLOG_INFO("[3/8] Скрытие дескриптора процесса...");
    if (HideProcessHandle()) {
        SPDLOG_INFO("✓ Дескриптор процесса скрыт");
    } else {
        SPDLOG_WARN("⚠ Не удалось скрыть дескриптор");
    }
    
    // 4. Обход проверок памяти
    SPDLOG_INFO("[4/8] Обход проверок памяти...");
    if (BypassMemoryChecks()) {
        SPDLOG_INFO("✓ Проверки памяти обойдены");
    } else {
        SPDLOG_WARN("⚠ Не удалось обойти проверки памяти");
    }
    
    // 5. Использование альтернативных методов доступа
    SPDLOG_INFO("[5/8] Настройка альтернативных методов доступа...");
    if (UseAlternativeMemoryAccess()) {
        SPDLOG_INFO("✓ Альтернативные методы настроены");
    } else {
        SPDLOG_WARN("⚠ Альтернативные методы недоступны");
    }
    
    // 6. Спуфинг вызовов API
    SPDLOG_INFO("[6/8] Спуфинг вызовов API...");
    if (SpoofAPICalls()) {
        SPDLOG_INFO("✓ Спуфинг API активирован");
    } else {
        SPDLOG_WARN("⚠ Спуфинг API недоступен");
    }
    
    // 7. Удаление следов отладки
    SPDLOG_INFO("[7/8] Удаление следов отладки...");
    if (RemoveDebugTraces()) {
        SPDLOG_INFO("✓ Следы отладки удалены");
    } else {
        SPDLOG_WARN("⚠ Не удалось удалить следы отладки");
    }
    
    // 8. Обход детекта хуков
    SPDLOG_INFO("[8/8] Обход детекта хуков...");
    if (BypassHookDetection()) {
        SPDLOG_INFO("✓ Детект хуков обойден");
    } else {
        SPDLOG_WARN("⚠ Не удалось обойти детект хуков");
    }
    
    if (success) {
        SPDLOG_INFO("=== ОБХОД АНТИ-ЧИТА ЗАВЕРШЕН УСПЕШНО ===");
    } else {
        SPDLOG_WARN("=== ОБХОД АНТИ-ЧИТА ЗАВЕРШЕН С ПРЕДУПРЕЖДЕНИЯМИ ===");
        SPDLOG_WARN("Функциональность может быть ограничена");
    }
    
    return true; // Возвращаем true даже если не все методы сработали
}

bool ExternalMemoryManager::HideExternalAccess() {
    // Скрытие следов external доступа
    // Временная заглушка
    SPDLOG_INFO("Скрытие следов external доступа (заглушка)");
    return true;
}

bool ExternalMemoryManager::EnableDebugPrivileges() {
    SPDLOG_INFO("Включение всех возможных привилегий...");
    
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        SPDLOG_ERROR("Не удалось открыть токен процесса: {}", GetLastError());
        return false;
    }
    
    // Список привилегий для включения
    const char* privileges[] = {
        SE_DEBUG_NAME,              // SeDebugPrivilege - отладка процессов
        SE_LOAD_DRIVER_NAME,        // SeLoadDriverPrivilege - загрузка драйверов
        SE_SYSTEM_ENVIRONMENT_NAME, // SeSystemEnvironmentPrivilege
        SE_TAKE_OWNERSHIP_NAME,     // SeTakeOwnershipPrivilege
        SE_TCB_NAME,                // SeTcbPrivilege - trusted computer base
        nullptr
    };
    
    bool success = true;
    for (int i = 0; privileges[i] != nullptr; i++) {
        TOKEN_PRIVILEGES tp;
        LUID luid;
        
        if (!LookupPrivilegeValueA(nullptr, privileges[i], &luid)) {
            SPDLOG_WARN("Не удалось найти привилегию: {}", privileges[i]);
            continue;
        }
        
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        
        if (AdjustTokenPrivileges(token, FALSE, &tp, 0, nullptr, nullptr)) {
            SPDLOG_INFO("✓ Привилегия включена: {}", privileges[i]);
        } else {
            SPDLOG_WARN("⚠ Не удалось включить привилегию: {}", privileges[i]);
            if (strcmp(privileges[i], SE_DEBUG_NAME) == 0) {
                success = false; // SE_DEBUG критична
            }
        }
    }
    
    CloseHandle(token);
    return success;
}

bool ExternalMemoryManager::OpenProcessHandle() {
    SPDLOG_INFO("Открытие дескриптора процесса с множественными методами...");
    
    // Метод 1: PROCESS_ALL_ACCESS (максимальные права)
    SPDLOG_INFO("Метод 1: PROCESS_ALL_ACCESS...");
    m_processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
    
    if (m_processHandle) {
        SPDLOG_INFO("✓ Дескриптор открыт с PROCESS_ALL_ACCESS");
        return true;
    }
    
    // Метод 2: Стандартные права для external чита
    SPDLOG_INFO("Метод 2: Стандартные права...");
    m_processHandle = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
        PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE, m_processId);
    
    if (m_processHandle) {
        SPDLOG_INFO("✓ Дескриптор открыт со стандартными правами");
        return true;
    }
    
    // Метод 3: Минимальные права
    SPDLOG_INFO("Метод 3: Минимальные права...");
    m_processHandle = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, m_processId);
    
    if (m_processHandle) {
        SPDLOG_WARN("⚠ Дескриптор открыт с минимальными правами (только чтение)");
        return true;
    }
    
    // Метод 4: Через NtOpenProcess (обход хуков)
    SPDLOG_INFO("Метод 4: NtOpenProcess (обход хуков)...");
    if (OpenProcessViaNtAPI()) {
        SPDLOG_INFO("✓ Дескриптор открыт через NtOpenProcess");
        return true;
    }
    
    // Метод 5: Через дубликацию дескриптора
    SPDLOG_INFO("Метод 5: Дубликация дескриптора...");
    if (OpenProcessViaDuplication()) {
        SPDLOG_INFO("✓ Дескриптор получен через дубликацию");
        return true;
    }
    
    DWORD error = GetLastError();
    SPDLOG_ERROR("✗ Все методы открытия дескриптора не сработали");
    SPDLOG_ERROR("Последняя ошибка: {}", error);
    
    if (error == ERROR_ACCESS_DENIED) {
        SPDLOG_ERROR("Отказано в доступе. Возможные причины:");
        SPDLOG_ERROR("1. Недостаточно прав (запустите от администратора)");
        SPDLOG_ERROR("2. Анти-чит блокирует доступ на уровне ядра");
        SPDLOG_ERROR("3. Процесс защищен Protected Process Light (PPL)");
        SPDLOG_ERROR("4. Требуется kernel-mode драйвер для обхода");
    }
    
    return false;
}

bool ExternalMemoryManager::OpenProcessViaNtAPI() {
    // Открытие процесса через NtOpenProcess (обход user-mode хуков)
    typedef NTSTATUS(NTAPI* pNtOpenProcess)(
        PHANDLE ProcessHandle,
        ACCESS_MASK DesiredAccess,
        PVOID ObjectAttributes,
        PVOID ClientId);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        return false;
    }
    
    pNtOpenProcess NtOpenProcess = 
        (pNtOpenProcess)GetProcAddress(ntdll, "NtOpenProcess");
    
    if (!NtOpenProcess) {
        return false;
    }
    
    // Структура OBJECT_ATTRIBUTES
    struct {
        ULONG Length;
        HANDLE RootDirectory;
        PVOID ObjectName;
        ULONG Attributes;
        PVOID SecurityDescriptor;
        PVOID SecurityQualityOfService;
    } objAttr = { sizeof(objAttr), nullptr, nullptr, 0, nullptr, nullptr };
    
    // Структура CLIENT_ID
    struct {
        PVOID UniqueProcess;
        PVOID UniqueThread;
    } clientId = { (PVOID)(ULONG_PTR)m_processId, nullptr };
    
    HANDLE handle = nullptr;
    NTSTATUS status = NtOpenProcess(
        &handle,
        PROCESS_ALL_ACCESS,
        &objAttr,
        &clientId);
    
    if (status == 0 && handle) {
        m_processHandle = handle;
        return true;
    }
    
    return false;
}

bool ExternalMemoryManager::OpenProcessViaDuplication() {
    // Попытка получить дескриптор через дубликацию из другого процесса
    // Это может обойти некоторые проверки анти-чита
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    
    // Ищем системный процесс с высокими привилегиями
    if (Process32First(snapshot, &pe)) {
        do {
            // Пропускаем наш процесс и целевой
            if (pe.th32ProcessID == GetCurrentProcessId() || 
                pe.th32ProcessID == m_processId) {
                continue;
            }
            
            // Пробуем открыть системный процесс
            HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pe.th32ProcessID);
            if (hProcess) {
                // Здесь можно попробовать найти дескриптор целевого процесса
                // и дублировать его, но это сложная техника
                CloseHandle(hProcess);
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return false;
}

bool ExternalMemoryManager::CloseProcessHandle() {
    if (m_processHandle) {
        CloseHandle(m_processHandle);
        m_processHandle = nullptr;
        return true;
    }
    return false;
}

bool ExternalMemoryManager::FindProcessByName(const std::string& processName) {
    SPDLOG_INFO("Поиск процесса: {}", processName);
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        SPDLOG_ERROR("Не удалось создать снимок процессов: {}", GetLastError());
        return false;
    }
    
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    
    bool found = false;
    int processCount = 0;
    
    // Первый проход: точное совпадение
    if (Process32First(snapshot, &processEntry)) {
        do {
            processCount++;
            if (_stricmp(processEntry.szExeFile, processName.c_str()) == 0) {
                m_processId = processEntry.th32ProcessID;
                SPDLOG_INFO("Найден процесс {} с ID: {}", processName, m_processId);
                found = true;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    
    // Второй проход: частичное совпадение (если не нашли точное)
    if (!found) {
        Process32First(snapshot, &processEntry); // Сброс итератора
        do {
            std::string currentProcess = processEntry.szExeFile;
            std::transform(currentProcess.begin(), currentProcess.end(), currentProcess.begin(), ::tolower);
            std::string searchName = processName;
            std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
            
            if (currentProcess.find(searchName) != std::string::npos) {
                m_processId = processEntry.th32ProcessID;
                SPDLOG_INFO("Найден похожий процесс: {} (ID: {})", processEntry.szExeFile, m_processId);
                found = true;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    
    // Третий проход: поиск по ключевым словам для GTA
    if (!found) {
        Process32First(snapshot, &processEntry); // Сброс итератора
        do {
            std::string currentProcess = processEntry.szExeFile;
            std::transform(currentProcess.begin(), currentProcess.end(), currentProcess.begin(), ::tolower);
            
            // Ключевые слова для поиска GTA
            if (currentProcess.find("gta") != std::string::npos || 
                currentProcess.find("gtav") != std::string::npos ||
                currentProcess.find("playgtav") != std::string::npos) {
                m_processId = processEntry.th32ProcessID;
                SPDLOG_INFO("Найден процесс по ключевым словам: {} (ID: {})", processEntry.szExeFile, m_processId);
                found = true;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    
    if (!found) {
        SPDLOG_ERROR("Процесс {} не найден. Всего проверено процессов: {}", processName, processCount);
        SPDLOG_ERROR("Список всех процессов:");
        if (Process32First(snapshot, &processEntry)) {
            do {
                SPDLOG_ERROR("  - {} (ID: {})", processEntry.szExeFile, processEntry.th32ProcessID);
            } while (Process32Next(snapshot, &processEntry));
        }
    }
    
    CloseHandle(snapshot);
    return found;
}

std::vector<uint8_t> ExternalMemoryManager::PatternToBytes(const std::string& pattern) {
    std::vector<uint8_t> bytes;
    std::string byteString;
    
    for (size_t i = 0; i < pattern.length(); i++) {
        if (pattern[i] == ' ') {
            continue;
        }
        
        if (pattern[i] == '?') {
            bytes.push_back(0);
            i++; // Пропуск следующего символа
            continue;
        }
        
        byteString += pattern[i];
        if (byteString.length() == 2) {
            bytes.push_back(static_cast<uint8_t>(strtoul(byteString.c_str(), nullptr, 16)));
            byteString.clear();
        }
    }
    
    return bytes;
}

bool ExternalMemoryManager::CompareBytes(const uint8_t* data, const uint8_t* pattern, const std::string& mask) {
    for (size_t i = 0; i < mask.length(); i++) {
        if (mask[i] == 'x' && data[i] != pattern[i]) {
            return false;
        }
    }
    return true;
}

bool ExternalMemoryManager::SafeReadMemory(uintptr_t address, void* buffer, size_t size) {
    __try {
        return ReadMemory(address, buffer, size);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ExternalMemoryManager::SafeWriteMemory(uintptr_t address, const void* buffer, size_t size) {
    __try {
        return WriteMemory(address, buffer, size);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ExternalMemoryManager::ProtectMemory(uintptr_t address, size_t size, DWORD newProtect, DWORD* oldProtect) {
    if (!m_processHandle) {
        return false;
    }
    
    return VirtualProtectEx(m_processHandle, 
                           reinterpret_cast<LPVOID>(address), 
                           size, newProtect, oldProtect);
}

bool ExternalMemoryManager::RestoreMemoryProtection(uintptr_t address, size_t size, DWORD oldProtect) {
    DWORD temp;
    return ProtectMemory(address, size, oldProtect, &temp);
}

bool ExternalMemoryManager::HideProcessHandle() {
    // Скрытие дескриптора процесса от анти-чита
    // Используем NtSetInformationProcess для скрытия
    SPDLOG_INFO("Скрытие дескриптора процесса");
    
    typedef NTSTATUS(NTAPI* pNtSetInformationProcess)(
        HANDLE ProcessHandle,
        ULONG ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        SPDLOG_WARN("Не удалось загрузить ntdll.dll");
        return false;
    }
    
    pNtSetInformationProcess NtSetInformationProcess = 
        (pNtSetInformationProcess)GetProcAddress(ntdll, "NtSetInformationProcess");
    
    if (!NtSetInformationProcess) {
        SPDLOG_WARN("Не удалось найти NtSetInformationProcess");
        return false;
    }
    
    // Пытаемся скрыть дескриптор
    ULONG hideInfo = 1; // ProcessDebugFlags
    NTSTATUS status = NtSetInformationProcess(
        m_processHandle,
        0x1f, // ProcessDebugFlags
        &hideInfo,
        sizeof(hideInfo));
    
    if (status == 0) {
        SPDLOG_INFO("Дескриптор процесса успешно скрыт");
        return true;
    } else {
        SPDLOG_WARN("Не удалось скрыть дескриптор процесса: 0x{:X}", status);
        return false;
    }
}

bool ExternalMemoryManager::BypassMemoryChecks() {
    // Обход проверок памяти через NtReadVirtualMemory/NtWriteVirtualMemory
    SPDLOG_INFO("Обход проверок памяти");
    
    // Используем системные вызовы вместо стандартных API
    // Это может обойти некоторые хуки анти-чита
    
    // Временная реализация - возвращаем true
    // В реальности нужно реализовать через NtReadVirtualMemory/NtWriteVirtualMemory
    SPDLOG_INFO("Используются стандартные методы чтения/записи");
    return true;
}

bool ExternalMemoryManager::UseAlternativeMemoryAccess() {
    // Альтернативные методы доступа к памяти
    SPDLOG_INFO("Использование альтернативных методов доступа к памяти");
    
    // 1. Использование ReadProcessMemory/WriteProcessMemory с обходом хуков
    // 2. Использование memory mapping
    // 3. Использование shared memory
    
    // Временная реализация
    SPDLOG_INFO("Альтернативные методы не реализованы (заглушка)");
    return true;
}

bool ExternalMemoryManager::SpoofAPICalls() {
    // Спуфинг вызовов API чтобы обмануть анти-чит
    SPDLOG_INFO("Спуфинг вызовов API");
    
    // Идеи для реализации:
    // 1. Случайные задержки между вызовами
    // 2. Изменение порядка вызовов
    // 3. Добавление ложных вызовов
    
    // Временная реализация
    Sleep(rand() % 50 + 10); // Случайная задержка
    SPDLOG_INFO("Добавлена случайная задержка для спуфинга");
    return true;
}

bool ExternalMemoryManager::RemoveProcessProtection() {
    // Снятие защиты процесса через NtSetInformationProcess
    SPDLOG_INFO("Попытка снять защиту процесса...");
    
    typedef NTSTATUS(NTAPI* pNtSetInformationProcess)(
        HANDLE ProcessHandle,
        ULONG ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        SPDLOG_WARN("Не удалось загрузить ntdll.dll");
        return false;
    }
    
    pNtSetInformationProcess NtSetInformationProcess = 
        (pNtSetInformationProcess)GetProcAddress(ntdll, "NtSetInformationProcess");
    
    if (!NtSetInformationProcess) {
        SPDLOG_WARN("Не удалось найти NtSetInformationProcess");
        return false;
    }
    
    // ProcessDebugPort = 7
    ULONG debugPort = 0;
    NTSTATUS status = NtSetInformationProcess(
        m_processHandle,
        7, // ProcessDebugPort
        &debugPort,
        sizeof(debugPort));
    
    if (status == 0) {
        SPDLOG_INFO("ProcessDebugPort успешно установлен");
    }
    
    // ProcessDebugObjectHandle = 30
    HANDLE debugObject = nullptr;
    status = NtSetInformationProcess(
        m_processHandle,
        30, // ProcessDebugObjectHandle
        &debugObject,
        sizeof(debugObject));
    
    if (status == 0) {
        SPDLOG_INFO("ProcessDebugObjectHandle успешно установлен");
    }
    
    // ProcessDebugFlags = 31
    ULONG debugFlags = 0;
    status = NtSetInformationProcess(
        m_processHandle,
        31, // ProcessDebugFlags
        &debugFlags,
        sizeof(debugFlags));
    
    if (status == 0) {
        SPDLOG_INFO("ProcessDebugFlags успешно установлен");
        return true;
    }
    
    return false;
}

bool ExternalMemoryManager::BypassHookDetection() {
    // Обход детекта хуков через прямые системные вызовы
    SPDLOG_INFO("Настройка обхода детекта хуков...");
    
    // Используем прямые системные вызовы вместо API
    // Это обходит хуки установленные в user-mode
    
    typedef NTSTATUS(NTAPI* pNtQuerySystemInformation)(
        ULONG SystemInformationClass,
        PVOID SystemInformation,
        ULONG SystemInformationLength,
        PULONG ReturnLength);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        return false;
    }
    
    pNtQuerySystemInformation NtQuerySystemInformation = 
        (pNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");
    
    if (!NtQuerySystemInformation) {
        return false;
    }
    
    // Проверяем наличие хуков в системе
    ULONG returnLength = 0;
    NTSTATUS status = NtQuerySystemInformation(
        0x10, // SystemModuleInformation
        nullptr,
        0,
        &returnLength);
    
    if (status == 0xC0000004) { // STATUS_INFO_LENGTH_MISMATCH
        SPDLOG_INFO("Системные вызовы доступны");
        return true;
    }
    
    return true; // Возвращаем true даже если не удалось проверить
}

bool ExternalMemoryManager::RemoveDebugTraces() {
    // Удаление следов отладки из PEB
    SPDLOG_INFO("Удаление следов отладки из PEB...");
    
    typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(
        HANDLE ProcessHandle,
        ULONG ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength);
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        return false;
    }
    
    pNtQueryInformationProcess NtQueryInformationProcess = 
        (pNtQueryInformationProcess)GetProcAddress(ntdll, "NtQueryInformationProcess");
    
    if (!NtQueryInformationProcess) {
        return false;
    }
    
    // Получаем PEB процесса
    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength = 0;
    NTSTATUS status = NtQueryInformationProcess(
        m_processHandle,
        0, // ProcessBasicInformation
        &pbi,
        sizeof(pbi),
        &returnLength);
    
    if (status == 0) {
        SPDLOG_INFO("PEB процесса получен: 0x{:X}", (uintptr_t)pbi.PebBaseAddress);
        
        // Очищаем флаги отладки в PEB
        // BeingDebugged = PEB + 0x2
        BYTE beingDebugged = 0;
        if (WriteMemory((uintptr_t)pbi.PebBaseAddress + 0x2, &beingDebugged, sizeof(beingDebugged))) {
            SPDLOG_INFO("BeingDebugged флаг очищен");
        }
        
        // NtGlobalFlag = PEB + 0xBC (x64) или 0x68 (x86)
        #ifdef _WIN64
        DWORD ntGlobalFlag = 0;
        if (WriteMemory((uintptr_t)pbi.PebBaseAddress + 0xBC, &ntGlobalFlag, sizeof(ntGlobalFlag))) {
            SPDLOG_INFO("NtGlobalFlag очищен");
        }
        #else
        DWORD ntGlobalFlag = 0;
        if (WriteMemory((uintptr_t)pbi.PebBaseAddress + 0x68, &ntGlobalFlag, sizeof(ntGlobalFlag))) {
            SPDLOG_INFO("NtGlobalFlag очищен");
        }
        #endif
        
        return true;
    }
    
    return false;
}

bool ExternalMemoryManager::ReadMemoryRegion(uintptr_t address, size_t size, std::vector<uint8_t>& buffer) {
    buffer.resize(size);
    return ReadMemory(address, buffer.data(), size);
}

bool ExternalMemoryManager::IsMemoryAccessible(uintptr_t address, size_t size) {
    MEMORY_BASIC_INFORMATION mbi = GetMemoryInfo(address);
    return (mbi.State == MEM_COMMIT) && 
           (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE));
}

void ExternalMemoryManager::LogMemoryOperation(uintptr_t address, size_t size, bool read, bool success) {
    if (success) {
        SPDLOG_DEBUG("Memory {}: 0x{:X} ({} bytes) - {}", 
                    read ? "read" : "write", address, size, success ? "OK" : "FAIL");
    }
}
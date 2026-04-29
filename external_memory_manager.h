#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")

// Структуры для работы с NT API
typedef LONG NTSTATUS;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PVOID PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;

class ExternalMemoryManager {
public:
    ExternalMemoryManager();
    ~ExternalMemoryManager();
    
    // Инициализация для external доступа
    bool Initialize();
    void Shutdown();
    
    // Основные операции с памятью
    bool ReadMemory(uintptr_t address, void* buffer, size_t size);
    bool WriteMemory(uintptr_t address, const void* buffer, size_t size);
    
    // Чтение различных типов данных
    template<typename T>
    T Read(uintptr_t address) {
        T value;
        if (ReadMemory(address, &value, sizeof(T))) {
            return value;
        }
        return T();
    }
    
    template<typename T>
    bool Write(uintptr_t address, const T& value) {
        return WriteMemory(address, &value, sizeof(T));
    }
    
    // Поиск паттернов в памяти (external версия)
    uintptr_t FindPattern(const std::string& moduleName, const std::string& pattern);
    uintptr_t FindPatternExternal(const uint8_t* start, size_t size, const std::string& pattern);
    
    // Получение информации о процессе
    uintptr_t GetModuleBase(const std::string& moduleName);
    uintptr_t GetGameBase() { return GetModuleBase("GTA5.exe"); }
    
    DWORD GetProcessId() const { return m_processId; }
    HANDLE GetProcessHandle() const { return m_processHandle; }
    
    // Проверка состояния
    bool IsAttached() const { return m_attached; }
    bool IsGameRunning() const;
    
    // External cheat методы
    bool AttachToProcess(const std::string& processName);
    bool DetachFromProcess();
    
    // Утилиты для external доступа
    std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size);
    bool WriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes);
    
    // Поиск данных в памяти
    std::vector<uintptr_t> FindAllPatterns(const std::string& moduleName, const std::string& pattern);
    
    // Получение информации о памяти
    MEMORY_BASIC_INFORMATION GetMemoryInfo(uintptr_t address);
    std::vector<MEMORY_BASIC_INFORMATION> GetMemoryRegions();
    
    // External патчи (без хуков)
    bool ApplyExternalPatch(uintptr_t address, const std::vector<uint8_t>& newBytes);
    bool RestoreExternalPatch(uintptr_t address);
    
    // Обход анти-чита для external
    bool BypassExternalAntiCheat();
    bool HideExternalAccess();
    
private:
    DWORD m_processId = 0;
    HANDLE m_processHandle = nullptr;
    bool m_attached = false;
    
    // Информация о модулях
    std::unordered_map<std::string, uintptr_t> m_moduleBases;
    
    // Примененные external патчи
    struct ExternalPatchInfo {
        uintptr_t address;
        std::vector<uint8_t> originalBytes;
        std::vector<uint8_t> newBytes;
    };
    
    std::unordered_map<uintptr_t, ExternalPatchInfo> m_externalPatches;
    
    // Вспомогательные методы
    bool EnableDebugPrivileges();
    bool OpenProcessHandle();
    bool OpenProcessViaNtAPI();
    bool OpenProcessViaDuplication();
    bool CloseProcessHandle();
    
    // Поиск процесса
    bool FindProcessByName(const std::string& processName);
    
    // Утилиты для работы с паттернами
    static std::vector<uint8_t> PatternToBytes(const std::string& pattern);
    static bool CompareBytes(const uint8_t* data, const uint8_t* pattern, const std::string& mask);
    
    // Безопасное чтение/запись
    bool SafeReadMemory(uintptr_t address, void* buffer, size_t size);
    bool SafeWriteMemory(uintptr_t address, const void* buffer, size_t size);
    
    // Временная защита памяти
    bool ProtectMemory(uintptr_t address, size_t size, DWORD newProtect, DWORD* oldProtect);
    bool RestoreMemoryProtection(uintptr_t address, size_t size, DWORD oldProtect);
    
    // Методы для обхода анти-чита
    bool RemoveProcessProtection();
    bool HideProcessHandle();
    bool BypassMemoryChecks();
    bool UseAlternativeMemoryAccess();
    bool SpoofAPICalls();
    bool RemoveDebugTraces();
    bool BypassHookDetection();
    
    // External методы доступа
    bool ReadMemoryRegion(uintptr_t address, size_t size, std::vector<uint8_t>& buffer);
    bool ReadMemoryViaMapping(uintptr_t address, void* buffer, size_t size);
    bool ReadMemoryInChunks(uintptr_t address, void* buffer, size_t size);
    bool WriteMemoryInChunks(uintptr_t address, const void* buffer, size_t size);
    bool IsMemoryAccessible(uintptr_t address, size_t size);
    
    // Логирование
    void LogMemoryOperation(uintptr_t address, size_t size, bool read, bool success);
};
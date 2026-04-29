#include "external_cheat_manager.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <Windows.h>
#include <chrono>
#include <memory>
#include <thread>

std::unique_ptr<Overlay> g_overlay;
bool g_running = true;

void ConfigureTextEncoding() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
}

void ShowError(const wchar_t* text) {
    MessageBoxW(nullptr, text, L"Moonsday", MB_ICONERROR);
}

void InitializeLogger() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("moonsday", console_sink);

        logger->set_level(spdlog::level::debug);
        logger->set_pattern("[%H:%M:%S] [%^%l%$] %v");

        spdlog::set_default_logger(logger);
        SPDLOG_INFO("Логгер Moonsday инициализирован");
    } catch (const spdlog::spdlog_ex& ex) {
        MessageBoxA(nullptr, ex.what(), "Moonsday", MB_ICONERROR);
    }
}

void MainLoop() {
    SPDLOG_INFO("Основной цикл запущен");

    while (g_running) {
        if (g_cheatManager && !g_cheatManager->IsGameAttached()) {
            SPDLOG_WARN("Целевой процесс отсоединен, выход");
            break;
        }

        if (g_cheatManager) {
            g_cheatManager->Update();
        }

        // Оверлей опционален - обновляем только если он есть
        if (g_overlay) {
            g_overlay->Update();
            g_overlay->Render();
        }

        // Рендеринг модулей чита (ESP и т.д.)
        if (g_cheatManager) {
            g_cheatManager->Render();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYDOWN && kbStruct->vkCode == VK_INSERT) {
            if (g_cheatManager) {
                g_cheatManager->ToggleMenu();
                if (g_overlay) {
                    SPDLOG_INFO("Меню переключено");
                } else {
                    SPDLOG_INFO("Insert нажат, но оверлей не найден");
                    SPDLOG_INFO("Меню недоступно без окна игры");
                }
            }
        }

        if (wParam == WM_KEYDOWN && kbStruct->vkCode == VK_END) {
            g_running = false;
            SPDLOG_INFO("Запрошен выход по клавише End");
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

HHOOK SetupKeyboardHook() {
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, nullptr, 0);
    if (!keyboardHook) {
        SPDLOG_ERROR("Не удалось установить хук клавиатуры: {}", GetLastError());
    } else {
        SPDLOG_INFO("Хук клавиатуры установлен");
    }
    return keyboardHook;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // АВТОЗАПУСК ОТ ИМЕНИ АДМИНИСТРАТОРА
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
        // Перезапускаем с правами администратора
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = exePath;
        sei.hwnd = nullptr;
        sei.nShow = SW_NORMAL;
        
        if (ShellExecuteExA(&sei)) {
            return 0; // Успешно перезапустили
        } else {
            MessageBoxA(nullptr, "Требуются права администратора!\nЗапустите программу от имени администратора.", "Moonsday", MB_ICONERROR);
            return 1;
        }
    }

    ConfigureTextEncoding();
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    InitializeLogger();

    SPDLOG_INFO("Запуск Moonsday");
    SPDLOG_INFO("Версия: 1.0.0");
    SPDLOG_INFO("Программа запущена с правами администратора");


    g_cheatManager = &ExternalCheatManager::GetInstance();

    if (!g_cheatManager->Initialize()) {
        SPDLOG_ERROR("Не удалось инициализировать менеджер Moonsday");
        ShowError(L"Не удалось инициализировать Moonsday.");
        return 1;
    }

    if (!g_cheatManager->AttachToGame("GTA5.exe")) {
        SPDLOG_ERROR("Не удалось подключиться к GTA5.exe");
        SPDLOG_ERROR("Возможные причины:");
        SPDLOG_ERROR("1. Игра не запущена");
        SPDLOG_ERROR("2. Недостаточно прав (запустите от администратора)");
        SPDLOG_ERROR("3. Анти-чит блокирует доступ");
        ShowError(L"Не удалось подключиться к GTA5.exe.\n\nВозможные причины:\n1. Игра не запущена\n2. Недостаточно прав\n3. Анти-чит блокирует доступ\n\nЗапустите программу от имени администратора.");
        return 1;
    }

    // Оверлей полностью опционален - нужен только для меню и визуальных эффектов
    // Основной функционал чита работает через доступ к памяти процесса
    
    HWND gtaWindow = nullptr;
    
    // Пробуем найти окно игры (только для оверлея)
    // Если не найдем - не страшно, чит все равно будет работать
    const char* windowNames[] = {
        "Majestic Multiplayer",
        "Grand Theft Auto V", 
        "GTA5",
        "Alt:V",
        "GTA V",
        nullptr
    };
    
    for (int i = 0; windowNames[i] != nullptr; i++) {
        gtaWindow = FindWindowA(nullptr, windowNames[i]);
        if (gtaWindow) {
            SPDLOG_INFO("Найдено окно игры для оверлея: {}", windowNames[i]);
            break;
        }
    }
    
    // Создаем оверлей только если нашли окно
    if (gtaWindow) {
        g_overlay = std::make_unique<Overlay>();
        if (!g_overlay->Initialize(gtaWindow)) {
            SPDLOG_WARN("Не удалось инициализировать оверлей, продолжаем без него");
            g_overlay.reset();
        } else {
            SPDLOG_INFO("Оверлей успешно инициализирован");
            SPDLOG_INFO("Меню доступно по клавише Insert");
        }
    } else {
        SPDLOG_INFO("Окно игры не найдено, работаем без оверлея");
        SPDLOG_INFO("ОСНОВНОЙ ФУНКЦИОНАЛ ЧИТА РАБОТАЕТ:");
        SPDLOG_INFO("  • Aimbot - автоприцеливание");
        SPDLOG_INFO("  • ESP - отображение игроков и объектов");
        SPDLOG_INFO("  • Weapon Mods - модификации оружия");
        SPDLOG_INFO("  • Конфигурации - загрузка/сохранение настроек");
        SPDLOG_INFO("");
        SPDLOG_INFO("Меню будет недоступно, но функции можно включить через конфиг");
        SPDLOG_INFO("Или модифицировать код для постоянной работы функций");
    }

    HHOOK keyboardHook = SetupKeyboardHook();
    MainLoop();

    SPDLOG_INFO("Завершение работы Moonsday");

    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        SPDLOG_INFO("Хук клавиатуры удален");
    }

    if (g_cheatManager) {
        g_cheatManager->Shutdown();
    }

    if (g_overlay) {
        g_overlay->Shutdown();
    }

    SPDLOG_INFO("Moonsday завершен");
    return 0;
}

int main() {
    return WinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineA(), SW_SHOW);
}

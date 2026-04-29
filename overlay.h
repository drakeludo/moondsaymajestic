#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <Windows.h>
#ifdef DrawText
#undef DrawText
#endif
#include <d3d11.h>
#include <imgui.h>

class Overlay {
public:
    Overlay();
    ~Overlay();
    
    // Настройки
    struct Settings {
        bool enabled = true;
        bool clickthroughWhenClosed = true;
        bool focusOnly = true;
        bool showFPS = true;
        bool showStats = true;
        bool showWatermark = true;
        bool vsync = false;
        float opacity = 0.9f;
        float scale = 1.0f;
        
        // Цвета
        struct {
            float background[4] = {0.0f, 0.0f, 0.0f, 0.59f};
            float text[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            float accent[4] = {1.0f, 0.65f, 0.0f, 1.0f};
        } colors;
    };
    
    // Инициализация
    bool Initialize(HWND targetWindow);
    void Shutdown();
    
    // Основной цикл
    void Update();
    void Render();
    
    // Управление окном
    bool CreateOverlayWindow();
    bool DestroyOverlayWindow();
    
    void SetPosition(int x, int y);
    void SetSize(int width, int height);
    void SetTransparency(float alpha);
    
    // Состояние
    bool IsInitialized() const { return m_initialized; }
    bool IsMenuOpen() const { return m_menuOpen; }
    void ToggleMenu() { m_menuOpen = !m_menuOpen; }
    
    // Геттеры/сеттеры
    const Settings& GetSettings() const { return m_settings; }
    void SetSettings(const Settings& settings) { m_settings = settings; }
    
    HWND GetWindowHandle() const { return m_overlayWindow; }
    
    // Утилиты для рисования
    void DrawText(int x, int y, const std::string& text, const float color[4]);
    void DrawRect(int x, int y, int width, int height, const float color[4]);
    void DrawLine(int x1, int y1, int x2, int y2, const float color[4]);
    void DrawCircle(int x, int y, int radius, const float color[4]);
    
    // Элементы интерфейса
    void DrawMenu();
    void DrawWatermark();
    void DrawFPS();
    void DrawStats();
    
    // Callbacks
    void SetRenderCallback(std::function<void()> callback) { m_renderCallback = callback; }
    void SetUpdateCallback(std::function<void()> callback) { m_updateCallback = callback; }
    
private:
    Settings m_settings;
    bool m_initialized = false;
    bool m_menuOpen = false;
    
    // Окно и графический контекст
    HWND m_targetWindow = nullptr;
    HWND m_overlayWindow = nullptr;
    
    // DirectX 11
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_deviceContext = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    
    // ImGui контекст
    ImGuiContext* m_imguiContext = nullptr;
    
    // Время и FPS
    float m_deltaTime = 0.0f;
    float m_lastFrameTime = 0.0f;
    int m_fps = 0;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    
    // Callbacks
    std::function<void()> m_renderCallback;
    std::function<void()> m_updateCallback;
    
    // Вспомогательные методы
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    
    bool CreateRenderTarget();
    void CleanupRenderTarget();
    
    bool SetupImGui();
    void CleanupImGui();
    
    // Обработка сообщений
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Управление окном
    bool SetupWindowStyles();
    bool MakeWindowTransparent();
    bool MakeWindowClickthrough(bool clickthrough);
    
    // Утилиты для работы с окнами
    bool GetTargetWindowRect(RECT& rect);
    bool IsTargetWindowFocused();
    void UpdateWindowPosition();
    
    // Рендеринг элементов
    void RenderImGui();
    void RenderDirectX();
    
    // Элементы меню
    void DrawModernMenu();
    void DrawAimbotContent();
    void DrawPlayerContent();
    void DrawVehicleContent();
    void DrawVisualContent();
    void DrawWeaponContent();
    void DrawMiscContent();
    void DrawExecutorContent();
    void DrawFriendlyContent();
    void DrawConfigContent();
    void DrawSettingsContent();
    
    // Утилиты для времени
    void UpdateDeltaTime();
    void UpdateFPS();
    
    // Безопасность
    bool CheckAntiOverlay();
    bool BypassOverlayDetection();
    
    // Логирование
    void LogError(const std::string& error);
    void LogInfo(const std::string& info);
};

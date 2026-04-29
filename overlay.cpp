#include "overlay.h"
#include <spdlog/spdlog.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Overlay::Overlay() {
    SPDLOG_INFO("Создание Overlay");
}

Overlay::~Overlay() {
    SPDLOG_INFO("Уничтожение Overlay");
    Shutdown();
}

bool Overlay::Initialize(HWND targetWindow) {
    if (m_initialized) {
        SPDLOG_WARN("Overlay уже инициализирован");
        return true;
    }
    
    SPDLOG_INFO("Инициализация Overlay");
    
    try {
        m_targetWindow = targetWindow;
        
        // Создание окна оверлея
        if (!CreateOverlayWindow()) {
            SPDLOG_ERROR("Не удалось создать окно оверлея");
            return false;
        }
        
        // Инициализация DirectX 11
        if (!CreateDeviceD3D()) {
            SPDLOG_ERROR("Не удалось инициализировать DirectX 11");
            return false;
        }
        
        // Создание цели рендеринга
        if (!CreateRenderTarget()) {
            SPDLOG_ERROR("Не удалось создать цель рендеринга");
            return false;
        }
        
        // Настройка ImGui
        if (!SetupImGui()) {
            SPDLOG_ERROR("Не удалось настроить ImGui");
            return false;
        }
        
        // Настройка стилей окна
        if (!SetupWindowStyles()) {
            SPDLOG_WARN("Не удалось настроить стили окна");
        }
        
        // Проверка анти-оверлея
        if (!CheckAntiOverlay()) {
            SPDLOG_WARN("Обнаружен анти-оверлей, попытка обхода");
            if (!BypassOverlayDetection()) {
                SPDLOG_ERROR("Не удалось обойти анти-оверлей");
                return false;
            }
        }
        
        m_initialized = true;
        SPDLOG_INFO("Overlay успешно инициализирован");
        return true;
        
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Ошибка при инициализации Overlay: {}", e.what());
        return false;
    }
}

void Overlay::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    SPDLOG_INFO("Завершение работы Overlay");
    
    // Очистка ImGui
    CleanupImGui();
    
    // Очистка цели рендеринга
    CleanupRenderTarget();
    
    // Очистка DirectX
    CleanupDeviceD3D();
    
    // Уничтожение окна
    if (m_overlayWindow) {
        DestroyOverlayWindow();
    }
    
    m_initialized = false;
    SPDLOG_INFO("Overlay завершен");
}

void Overlay::Update() {
    if (!m_initialized || !m_settings.enabled) {
        return;
    }
    
    // Проверка фокуса окна
    if (m_settings.focusOnly && !IsTargetWindowFocused()) {
        return;
    }
    
    // Обновление времени и FPS
    UpdateDeltaTime();
    UpdateFPS();
    
    // Обновление позиции окна
    UpdateWindowPosition();
    
    // Вызов callback'а обновления
    if (m_updateCallback) {
        m_updateCallback();
    }
}

void Overlay::Render() {
    if (!m_initialized || !m_settings.enabled) {
        return;
    }
    
    // Проверка фокуса окна
    if (m_settings.focusOnly && !IsTargetWindowFocused()) {
        return;
    }
    
    // Начало нового кадра ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Отрисовка меню
    if (m_menuOpen) {
        DrawMenu();
    }
    
    // Отрисовка водяного знака
    if (m_settings.showWatermark) {
        DrawWatermark();
    }
    
    // Отрисовка FPS
    if (m_settings.showFPS) {
        DrawFPS();
    }
    
    // Отрисовка статистики
    if (m_settings.showStats) {
        DrawStats();
    }
    
    // Вызов callback'а рендеринга
    if (m_renderCallback) {
        m_renderCallback();
    }
    
    // Рендеринг ImGui
    RenderImGui();
    
    // Рендеринг DirectX
    RenderDirectX();
}

bool Overlay::CreateOverlayWindow() {
    // Получение размеров целевого окна
    RECT targetRect;
    if (!GetTargetWindowRect(targetRect)) {
        SPDLOG_ERROR("Не удалось получить размеры целевого окна");
        return false;
    }
    
    // Регистрация класса окна
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "MoonsdayOverlay";
    
    if (!RegisterClassEx(&wc)) {
        SPDLOG_ERROR("Не удалось зарегистрировать класс окна: {}", GetLastError());
        return false;
    }
    
    // Создание окна
    m_overlayWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        "Moonsday Overlay",
        WS_POPUP,
        targetRect.left,
        targetRect.top,
        targetRect.right - targetRect.left,
        targetRect.bottom - targetRect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        this
    );
    
    if (!m_overlayWindow) {
        SPDLOG_ERROR("Не удалось создать окно оверлея: {}", GetLastError());
        return false;
    }
    
    // Установка прозрачности
    SetTransparency(m_settings.opacity);
    
    // Установка кликабельности
    MakeWindowClickthrough(!m_menuOpen && m_settings.clickthroughWhenClosed);
    
    // Показать окно
    ShowWindow(m_overlayWindow, SW_SHOW);
    UpdateWindow(m_overlayWindow);
    
    SPDLOG_INFO("Окно оверлея создано");
    return true;
}

bool Overlay::DestroyOverlayWindow() {
    if (!m_overlayWindow) {
        return true;
    }
    
    DestroyWindow(m_overlayWindow);
    m_overlayWindow = nullptr;
    
    // Отмена регистрации класса окна
    UnregisterClass("MoonsdayOverlay", GetModuleHandle(nullptr));
    
    SPDLOG_INFO("Окно оверлея уничтожено");
    return true;
}

void Overlay::SetPosition(int x, int y) {
    if (!m_overlayWindow) {
        return;
    }
    
    SetWindowPos(m_overlayWindow, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Overlay::SetSize(int width, int height) {
    if (!m_overlayWindow) {
        return;
    }
    
    SetWindowPos(m_overlayWindow, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void Overlay::SetTransparency(float alpha) {
    if (!m_overlayWindow) {
        return;
    }
    
    // Установка прозрачности через layered window
    SetLayeredWindowAttributes(m_overlayWindow, 0, static_cast<BYTE>(alpha * 255), LWA_ALPHA);
}

void Overlay::DrawText(int x, int y, const std::string& text, const float color[4]) {
    // Здесь будет реализация отрисовки текста через ImGui
    // Временная заглушка
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(static_cast<float>(x), static_cast<float>(y)), 
                                          ImColor(color[0], color[1], color[2], color[3]), 
                                          text.c_str());
}

void Overlay::DrawRect(int x, int y, int width, int height, const float color[4]) {
    // Здесь будет реализация отрисовки прямоугольника через ImGui
    // Временная заглушка
    ImGui::GetBackgroundDrawList()->AddRect(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                                          ImVec2(static_cast<float>(x + width), static_cast<float>(y + height)),
                                          ImColor(color[0], color[1], color[2], color[3]));
}

void Overlay::DrawLine(int x1, int y1, int x2, int y2, const float color[4]) {
    // Здесь будет реализация отрисовки линии через ImGui
    // Временная заглушка
    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(static_cast<float>(x1), static_cast<float>(y1)),
                                          ImVec2(static_cast<float>(x2), static_cast<float>(y2)),
                                          ImColor(color[0], color[1], color[2], color[3]));
}

void Overlay::DrawCircle(int x, int y, int radius, const float color[4]) {
    // Здесь будет реализация отрисовки круга через ImGui
    // Временная заглушка
    ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                                            static_cast<float>(radius),
                                            ImColor(color[0], color[1], color[2], color[3]));
}

void Overlay::DrawMenu() {
    // Вызов современного меню из modern_gui.cpp
    DrawModernMenu();
}

void Overlay::DrawWatermark() {
    // Отрисовка водяного знака в углу экрана
    std::string watermark = "Moonsday v1.0.0";
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, 10), 
                                          ImColor(m_settings.colors.text[0], m_settings.colors.text[1], 
                                                  m_settings.colors.text[2], m_settings.colors.text[3]), 
                                          watermark.c_str());
}

void Overlay::DrawFPS() {
    // Отрисовка FPS в углу экрана
    std::string fpsText = "FPS: " + std::to_string(m_fps);
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, 30), 
                                          ImColor(m_settings.colors.text[0], m_settings.colors.text[1], 
                                                  m_settings.colors.text[2], m_settings.colors.text[3]), 
                                          fpsText.c_str());
}

void Overlay::DrawStats() {
    // Отрисовка статистики
    // Временная заглушка
    std::string stats = "Статус: активно";
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, 50), 
                                          ImColor(m_settings.colors.text[0], m_settings.colors.text[1], 
                                                  m_settings.colors.text[2], m_settings.colors.text[3]), 
                                          stats.c_str());
}

bool Overlay::CreateDeviceD3D() {
    // Создание устройства и контекста DirectX 11
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_overlayWindow;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_deviceContext
    );
    
    if (FAILED(hr)) {
        SPDLOG_ERROR("Не удалось создать устройство DirectX 11: 0x{:X}", hr);
        return false;
    }
    
    SPDLOG_INFO("Устройство DirectX 11 создано");
    return true;
}

void Overlay::CleanupDeviceD3D() {
    if (m_swapChain) {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    
    if (m_deviceContext) {
        m_deviceContext->Release();
        m_deviceContext = nullptr;
    }
    
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    
    SPDLOG_INFO("Устройство DirectX 11 очищено");
}

bool Overlay::CreateRenderTarget() {
    if (!m_swapChain) {
        return false;
    }
    
    ID3D11Texture2D* pBackBuffer = nullptr;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (!pBackBuffer) {
        SPDLOG_ERROR("Не удалось получить back buffer");
        return false;
    }
    
    HRESULT hr = m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_renderTargetView);
    pBackBuffer->Release();
    
    if (FAILED(hr)) {
        SPDLOG_ERROR("Не удалось создать render target view: 0x{:X}", hr);
        return false;
    }
    
    SPDLOG_INFO("Render target создан");
    return true;
}

void Overlay::CleanupRenderTarget() {
    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
        SPDLOG_INFO("Render target очищен");
    }
}

bool Overlay::SetupImGui() {
    // Создание контекста ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_imguiContext = ImGui::GetCurrentContext();
    
    // Настройка стиля ImGui
    ImGui::StyleColorsDark();
    
    // Настройка масштаба
    ImGui::GetStyle().ScaleAllSizes(m_settings.scale);
    
    // Инициализация бэкендов
    if (!ImGui_ImplWin32_Init(m_overlayWindow)) {
        SPDLOG_ERROR("Не удалось инициализировать ImGui Win32");
        return false;
    }
    
    if (!ImGui_ImplDX11_Init(m_device, m_deviceContext)) {
        SPDLOG_ERROR("Не удалось инициализировать ImGui DX11");
        return false;
    }
    
    SPDLOG_INFO("ImGui инициализирован");
    return true;
}

void Overlay::CleanupImGui() {
    if (m_imguiContext) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_imguiContext = nullptr;
        SPDLOG_INFO("ImGui очищен");
    }
}

LRESULT WINAPI Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }
    
    // Получение указателя на объект Overlay
    Overlay* overlay = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        overlay = static_cast<Overlay*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(overlay));
    } else {
        overlay = reinterpret_cast<Overlay*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    
    if (overlay) {
        return overlay->HandleMessage(hWnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT Overlay::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            if (m_device && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                m_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_KEYDOWN:
            // Обработка горячих клавиш
            if (wParam == VK_INSERT) {
                ToggleMenu();
                MakeWindowClickthrough(!m_menuOpen && m_settings.clickthroughWhenClosed);
                return 0;
            }
            break;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool Overlay::SetupWindowStyles() {
    if (!m_overlayWindow) {
        return false;
    }
    
    // Установка extended style
    SetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE, 
                     GetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE) | 
                     WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
    
    // Обновление окна
    SetWindowPos(m_overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    
    return true;
}

bool Overlay::MakeWindowTransparent() {
    SetTransparency(m_settings.opacity);
    return true;
}

bool Overlay::MakeWindowClickthrough(bool clickthrough) {
    if (!m_overlayWindow) {
        return false;
    }
    
    LONG_PTR style = GetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE);
    
    if (clickthrough) {
        style |= WS_EX_TRANSPARENT;
    } else {
        style &= ~WS_EX_TRANSPARENT;
    }
    
    SetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE, style);
    return true;
}

bool Overlay::GetTargetWindowRect(RECT& rect) {
    if (!m_targetWindow) {
        return false;
    }
    
    return GetWindowRect(m_targetWindow, &rect);
}

bool Overlay::IsTargetWindowFocused() {
    if (!m_targetWindow) {
        return false;
    }
    
    return GetForegroundWindow() == m_targetWindow;
}

void Overlay::UpdateWindowPosition() {
    if (!m_targetWindow || !m_overlayWindow) {
        return;
    }
    
    RECT targetRect;
    if (GetTargetWindowRect(targetRect)) {
        SetWindowPos(m_overlayWindow, nullptr, 
                    targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left,
                    targetRect.bottom - targetRect.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void Overlay::UpdateDeltaTime() {
    float currentTime = static_cast<float>(GetTickCount64()) / 1000.0f;
    m_deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;
}

void Overlay::UpdateFPS() {
    m_frameCount++;
    m_fpsTimer += m_deltaTime;
    
    if (m_fpsTimer >= 1.0f) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }
}

void Overlay::RenderImGui() {
    // Рендеринг ImGui
    ImGui::Render();
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    
    // Очистка экрана
    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, clearColor);
    
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Overlay::RenderDirectX() {
    // Презентация кадра
    m_swapChain->Present(m_settings.vsync ? 1 : 0, 0);
}

bool Overlay::CheckAntiOverlay() {
    // Проверка на анти-оверлей
    // Временная заглушка
    return true;
}

bool Overlay::BypassOverlayDetection() {
    // Обход детекции оверлея
    // Временная заглушка
    return true;
}

void Overlay::LogError(const std::string& error) {
    SPDLOG_ERROR("Overlay: {}", error);
}

void Overlay::LogInfo(const std::string& info) {
    SPDLOG_INFO("Overlay: {}", info);
}

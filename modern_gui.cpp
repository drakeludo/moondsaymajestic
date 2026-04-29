#include "overlay.h"
#include <imgui.h>

namespace {
namespace UiColor {
    const ImVec4 Background(0.055f, 0.058f, 0.070f, 1.0f);
    const ImVec4 Sidebar(0.070f, 0.075f, 0.092f, 1.0f);
    const ImVec4 Panel(0.090f, 0.095f, 0.115f, 1.0f);
    const ImVec4 PanelAlt(0.120f, 0.125f, 0.150f, 1.0f);
    const ImVec4 Control(0.135f, 0.140f, 0.170f, 1.0f);
    const ImVec4 ControlHover(0.170f, 0.180f, 0.215f, 1.0f);
    const ImVec4 Accent(0.430f, 0.560f, 0.840f, 1.0f);
    const ImVec4 AccentSoft(0.310f, 0.390f, 0.590f, 1.0f);
    const ImVec4 Text(0.830f, 0.855f, 0.910f, 1.0f);
    const ImVec4 Muted(0.520f, 0.560f, 0.670f, 1.0f);
    const ImVec4 Warning(1.000f, 0.660f, 0.250f, 1.0f);
}

struct TabItem {
    const char* icon;
    const char* title;
};

const TabItem kTabs[] = {
    {"A", "Наведение"},
    {"P", "Игрок"},
    {"V", "Транспорт"},
    {"E", "Визуал"},
    {"W", "Оружие"},
    {"M", "Разное"},
    {"X", "Исполнитель"},
    {"F", "Друзья"},
    {"C", "Конфиг"},
    {"S", "Настройки"},
};

void ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_WindowBg] = UiColor::Background;
    style.Colors[ImGuiCol_ChildBg] = UiColor::Panel;
    style.Colors[ImGuiCol_PopupBg] = UiColor::Panel;
    style.Colors[ImGuiCol_Border] = ImVec4(0.170f, 0.180f, 0.220f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_FrameBg] = UiColor::Control;
    style.Colors[ImGuiCol_FrameBgHovered] = UiColor::ControlHover;
    style.Colors[ImGuiCol_FrameBgActive] = UiColor::AccentSoft;
    style.Colors[ImGuiCol_Button] = UiColor::Control;
    style.Colors[ImGuiCol_ButtonHovered] = UiColor::ControlHover;
    style.Colors[ImGuiCol_ButtonActive] = UiColor::AccentSoft;
    style.Colors[ImGuiCol_Header] = UiColor::Control;
    style.Colors[ImGuiCol_HeaderHovered] = UiColor::ControlHover;
    style.Colors[ImGuiCol_HeaderActive] = UiColor::AccentSoft;
    style.Colors[ImGuiCol_CheckMark] = UiColor::Accent;
    style.Colors[ImGuiCol_SliderGrab] = UiColor::Accent;
    style.Colors[ImGuiCol_SliderGrabActive] = UiColor::Accent;
    style.Colors[ImGuiCol_Separator] = ImVec4(0.160f, 0.170f, 0.210f, 1.0f);
    style.Colors[ImGuiCol_Text] = UiColor::Text;
    style.Colors[ImGuiCol_TextDisabled] = UiColor::Muted;

    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(10, 8);
    style.ItemSpacing = ImVec2(10, 10);
    style.ItemInnerSpacing = ImVec2(8, 7);
    style.WindowRounding = 0.0f;
    style.ChildRounding = 7.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
}

void TextMuted(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, UiColor::Muted);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

bool SidebarButton(const TabItem& tab, bool selected) {
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? UiColor::PanelAlt : UiColor::Sidebar);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UiColor::PanelAlt);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UiColor::AccentSoft);
    ImGui::PushStyleColor(ImGuiCol_Text, selected ? UiColor::Accent : UiColor::Muted);

    char label[96];
    snprintf(label, sizeof(label), "%s  %s", tab.icon, tab.title);
    bool clicked = ImGui::Button(label, ImVec2(148, 38));

    ImGui::PopStyleColor(4);
    return clicked;
}

void BeginPanel(const char* title) {
    ImGui::BeginChild(title, ImVec2(286, 0), true);
    TextMuted(title);
    ImGui::Separator();
    ImGui::Spacing();
}

void EndPanel() {
    ImGui::EndChild();
}

bool WideButton(const char* label) {
    return ImGui::Button(label, ImVec2(254, 36));
}

void DrawHealthPanel() {
    static bool godMode = false;
    static bool semiGodMode = false;
    static float health = 100.0f;
    static float armour = 100.0f;

    BeginPanel("Здоровье");
    ImGui::Checkbox("Бессмертие", &godMode);
    ImGui::Checkbox("Полубессмертие", &semiGodMode);
    WideButton("Установить здоровье");
    TextMuted("Здоровье");
    ImGui::SliderFloat("##health", &health, 0.0f, 100.0f, "%.0f");
    ImGui::Spacing();
    WideButton("Установить броню");
    TextMuted("Броня");
    ImGui::SliderFloat("##armour", &armour, 0.0f, 100.0f, "%.0f");
    EndPanel();
}

void DrawResetPanel() {
    BeginPanel("Сброс");
    WideButton("Самоуничтожение");
    WideButton("Броня 0%");
    WideButton("Здоровье 100%");
    EndPanel();
}

void DrawGenericPanel(const char* title, const char* a, const char* b, const char* c) {
    static bool enabled = true;
    static bool optionA = false;
    static bool optionB = true;
    static float value = 50.0f;

    BeginPanel(title);
    ImGui::Checkbox("Включено", &enabled);
    ImGui::Checkbox(a, &optionA);
    ImGui::Checkbox(b, &optionB);
    TextMuted(c);
    ImGui::SliderFloat("##value", &value, 0.0f, 100.0f, "%.0f");
    WideButton("Применить");
    EndPanel();
}

void DrawHeader(const char* icon, const char* title, const char* subtitle) {
    ImGui::PushStyleColor(ImGuiCol_Text, UiColor::Accent);
    ImGui::Text("%s", icon);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("%s", title);
    TextMuted(subtitle);
    ImGui::Spacing();
}
}

void Overlay::DrawModernMenu() {
    static int selectedTab = 1;

    ApplyStyle();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Moonsday", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, UiColor::Sidebar);
    ImGui::BeginChild("Левая панель", ImVec2(176, 0), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, UiColor::Accent);
    ImGui::SetCursorPosX(24);
    ImGui::SetCursorPosY(18);
    ImGui::SetWindowFontScale(2.2f);
    ImGui::Text("A");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    for (int i = 0; i < IM_ARRAYSIZE(kTabs); ++i) {
        if (SidebarButton(kTabs[i], selectedTab == i)) {
            selectedTab = i;
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::BeginChild("Контент", ImVec2(0, 0), false);
    DrawHeader(kTabs[selectedTab].icon, kTabs[selectedTab].title, "Moonsday");

    switch (selectedTab) {
        case 1:
            DrawHealthPanel();
            ImGui::SameLine();
            DrawResetPanel();
            break;
        case 2:
            DrawGenericPanel("Транспорт", "Невидимый корпус", "Улучшенное управление", "Прочность");
            ImGui::SameLine();
            DrawGenericPanel("Двигатель", "Быстрый запуск", "Стабилизация", "Мощность");
            break;
        case 3:
            DrawGenericPanel("Визуал", "Показывать подсказки", "Компактный режим", "Дальность");
            ImGui::SameLine();
            DrawGenericPanel("Оверлей", "Водяной знак", "FPS", "Прозрачность");
            break;
        case 4:
            DrawGenericPanel("Оружие", "Профиль 1", "Профиль 2", "Интенсивность");
            ImGui::SameLine();
            DrawGenericPanel("Параметры", "Плавность", "Контроль", "Значение");
            break;
        case 8:
            DrawGenericPanel("Конфиг", "Автосохранение", "Резервная копия", "Период");
            ImGui::SameLine();
            DrawGenericPanel("Профиль", "Загружать при старте", "Синхронизация", "Слот");
            break;
        case 9:
            DrawGenericPanel("Настройки", "Клик сквозь окно", "Только при фокусе", "Масштаб");
            ImGui::SameLine();
            DrawGenericPanel("Интерфейс", "Показывать статистику", "Показывать FPS", "Прозрачность");
            break;
        default:
            DrawGenericPanel(kTabs[selectedTab].title, "Основной режим", "Дополнительный режим", "Значение");
            ImGui::SameLine();
            DrawGenericPanel("Действия", "Подтверждать команды", "Тихий режим", "Задержка");
            break;
    }

    ImGui::EndChild();
    ImGui::End();
}

void Overlay::DrawAimbotContent() {
    DrawGenericPanel("Наведение", "Плавный режим", "Ограничение области", "Плавность");
}

void Overlay::DrawPlayerContent() {
    DrawHealthPanel();
    ImGui::SameLine();
    DrawResetPanel();
}

void Overlay::DrawVehicleContent() {
    DrawGenericPanel("Транспорт", "Невидимый корпус", "Улучшенное управление", "Прочность");
}

void Overlay::DrawVisualContent() {
    DrawGenericPanel("Визуал", "Показывать подсказки", "Компактный режим", "Дальность");
}

void Overlay::DrawWeaponContent() {
    DrawGenericPanel("Оружие", "Профиль 1", "Профиль 2", "Интенсивность");
}

void Overlay::DrawMiscContent() {
    DrawGenericPanel("Разное", "Основной режим", "Дополнительный режим", "Значение");
}

void Overlay::DrawExecutorContent() {
    DrawGenericPanel("Исполнитель", "Подтверждать команды", "Тихий режим", "Задержка");
}

void Overlay::DrawFriendlyContent() {
    DrawGenericPanel("Друзья", "Показывать список", "Выделять цветом", "Размер списка");
}

void Overlay::DrawConfigContent() {
    DrawGenericPanel("Конфиг", "Автосохранение", "Резервная копия", "Период");
}

void Overlay::DrawSettingsContent() {
    DrawGenericPanel("Настройки", "Клик сквозь окно", "Только при фокусе", "Масштаб");
}

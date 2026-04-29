#pragma once
#include <cstdint>

// Оффсеты для GTA 5 Alt:V (Majestic RP)
// Build 3258 - актуально на 29 апреля 2026

namespace Offsets {
    // Базовые адреса (Build 3258)
    namespace Base {
        constexpr uintptr_t World = 0x25B14B0;              // Указатель на мир
        constexpr uintptr_t ReplayInterface = 0x1FBD4F0;    // Replay Interface
        constexpr uintptr_t Camera = 0x201E7D0;             // Камера
    }
    
    // Оффсеты для Ped (игрок/NPC)
    namespace Ped {
        constexpr uintptr_t Position = 0x90;                // Позиция (Vector3)
        constexpr uintptr_t Health = 0x284;                 // Здоровье (float)
        constexpr uintptr_t Armor = 0x150C;                 // Броня (float)
        constexpr uintptr_t WeaponManager = 0x10B8;         // Менеджер оружия
        constexpr uintptr_t BoneMatrix = 0x410;             // Матрица костей
    }
    
    // Оффсеты для Camera
    namespace Vehicle {
        constexpr uintptr_t Position = 0x90;
        constexpr uintptr_t Health = 0x280;
    }

    namespace Camera {
        constexpr uintptr_t Position = 0x60;                // Позиция камеры (Vector3)
        constexpr uintptr_t ViewAngles = 0x3D0;             // Углы обзора (Vector3)
    }
    
    // Индексы костей для аима
    namespace Bones {
        constexpr int Head = 31086;                         // Голова
    }
}

// Структуры данных игры
namespace GameStructures {
    struct Vector3 {
        float x, y, z;
        float _padding;
    };
}

// Имена процессов для Alt:V
namespace ProcessNames {
    constexpr const char* GTA5 = "GTA5.exe";
    constexpr const char* AltVClient = "altv-client.dll";
}

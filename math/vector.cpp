#include "math/vector.h"
#include <cmath>

namespace MathUtils {

bool WorldToScreen(const Vector3& worldPos, Vector3& screenPos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, int screenWidth, int screenHeight) {
    // Преобразование мировых координат в клип-пространство
    Vector4 clipCoords;
    clipCoords.x = worldPos.x * viewMatrix.m[0][0] + worldPos.y * viewMatrix.m[0][1] + worldPos.z * viewMatrix.m[0][2] + viewMatrix.m[0][3];
    clipCoords.y = worldPos.x * viewMatrix.m[1][0] + worldPos.y * viewMatrix.m[1][1] + worldPos.z * viewMatrix.m[1][2] + viewMatrix.m[1][3];
    clipCoords.z = worldPos.x * viewMatrix.m[2][0] + worldPos.y * viewMatrix.m[2][1] + worldPos.z * viewMatrix.m[2][2] + viewMatrix.m[2][3];
    clipCoords.w = worldPos.x * viewMatrix.m[3][0] + worldPos.y * viewMatrix.m[3][1] + worldPos.z * viewMatrix.m[3][2] + viewMatrix.m[3][3];
    
    if (clipCoords.w < 0.1f) {
        return false;
    }
    
    // Перспективное деление
    Vector3 ndc;
    ndc.x = clipCoords.x / clipCoords.w;
    ndc.y = clipCoords.y / clipCoords.w;
    ndc.z = clipCoords.z / clipCoords.w;
    
    // Преобразование в экранные координаты
    screenPos.x = (screenWidth / 2.0f * ndc.x) + (ndc.x + screenWidth / 2.0f);
    screenPos.y = -(screenHeight / 2.0f * ndc.y) + (ndc.y + screenHeight / 2.0f);
    screenPos.z = ndc.z;
    
    return true;
}

Vector3 CalculateAngle(const Vector3& source, const Vector3& destination) {
    Vector3 direction = destination - source;
    Vector3 angle;
    
    // Расчет углов по осям
    angle.x = -atan2f(direction.z, sqrtf(direction.x * direction.x + direction.y * direction.y));
    angle.y = atan2f(direction.y, direction.x);
    angle.z = 0.0f;
    
    // Конвертация в градусы
    angle.x = angle.x * RAD2DEG;
    angle.y = angle.y * RAD2DEG;
    
    return angle;
}

Vector3 NormalizeAngle(const Vector3& angle) {
    Vector3 normalized = angle;
    
    // Нормализация углов
    while (normalized.x > 89.0f) normalized.x -= 180.0f;
    while (normalized.x < -89.0f) normalized.x += 180.0f;
    
    while (normalized.y > 180.0f) normalized.y -= 360.0f;
    while (normalized.y < -180.0f) normalized.y += 360.0f;
    
    normalized.z = 0.0f;
    
    return normalized;
}

Vector3 SmoothAngle(const Vector3& current, const Vector3& target, float smooth) {
    if (smooth <= 0.0f) {
        return target;
    }
    
    Vector3 delta = target - current;
    delta = NormalizeAngle(delta);
    
    Vector3 smoothed;
    smoothed.x = current.x + delta.x / smooth;
    smoothed.y = current.y + delta.y / smooth;
    smoothed.z = 0.0f;
    
    smoothed = NormalizeAngle(smoothed);
    
    return smoothed;
}

bool IsInFOV(const Vector3& angle, float fov) {
    // Упрощенная проверка FOV
    float angleDiff = sqrtf(angle.x * angle.x + angle.y * angle.y);
    return angleDiff <= fov / 2.0f;
}

} // namespace MathUtils
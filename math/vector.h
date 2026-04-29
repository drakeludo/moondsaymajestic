#pragma once
#include <cmath>
#include <algorithm>

struct Vector2 {
    float x, y;
    
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
    
    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }
    
    float Length() const { return std::sqrt(x*x + y*y); }
    float Distance(const Vector2& other) const { return (*this - other).Length(); }
    Vector2 Normalized() const { float len = Length(); return len > 0 ? *this / len : Vector2(); }
};

struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }
    
    float Length() const { return std::sqrt(x*x + y*y + z*z); }
    float Distance(const Vector3& other) const { return (*this - other).Length(); }
    Vector3 Normalized() const { float len = Length(); return len > 0 ? *this / len : Vector3(); }
    
    float Dot(const Vector3& other) const { return x*other.x + y*other.y + z*other.z; }
    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y*other.z - z*other.y,
            z*other.x - x*other.z,
            x*other.y - y*other.x
        );
    }
};

struct Vector4 {
    float x, y, z, w;
    
    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    
    Vector4 operator+(const Vector4& other) const { return Vector4(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vector4 operator-(const Vector4& other) const { return Vector4(x - other.x, y - other.y, z - other.z, w - other.w); }
    Vector4 operator*(float scalar) const { return Vector4(x * scalar, y * scalar, z * scalar, w * scalar); }
    Vector4 operator/(float scalar) const { return Vector4(x / scalar, y / scalar, z / scalar, w / scalar); }
    
    float Length() const { return std::sqrt(x*x + y*y + z*z + w*w); }
    Vector4 Normalized() const { float len = Length(); return len > 0 ? *this / len : Vector4(); }
};

// Матрица 4x4 для преобразований
struct Matrix4x4 {
    float m[4][4];
    
    Matrix4x4() {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }
    
    static Matrix4x4 Identity() { return Matrix4x4(); }
    
    Vector4 operator*(const Vector4& v) const {
        return Vector4(
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
            m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
        );
    }
    
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }
};

// Утилиты для работы с углами
namespace MathUtils {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG2RAD = PI / 180.0f;
    constexpr float RAD2DEG = 180.0f / PI;
    
    inline float ToRadians(float degrees) { return degrees * DEG2RAD; }
    inline float ToDegrees(float radians) { return radians * RAD2DEG; }
    
    inline float Clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }
    
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return Vector3(
            Lerp(a.x, b.x, t),
            Lerp(a.y, b.y, t),
            Lerp(a.z, b.z, t)
        );
    }
    
    // Преобразование мировых координат в экранные
    bool WorldToScreen(const Vector3& worldPos, Vector3& screenPos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, int screenWidth, int screenHeight);
    
    // Расчет угла между двумя точками
    Vector3 CalculateAngle(const Vector3& source, const Vector3& destination);
    
    // Нормализация угла
    Vector3 NormalizeAngle(const Vector3& angle);
    
    // Сглаживание угла
    Vector3 SmoothAngle(const Vector3& current, const Vector3& target, float smooth);
    
    // Проверка нахождения в FOV
    bool IsInFOV(const Vector3& angle, float fov);
}
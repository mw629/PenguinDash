#pragma once
#include <cmath>

/// <summary>
/// 4次元ベクトル (x, y, z, w / r, g, b, a)
/// </summary>
struct Vector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr Vector4() = default;
    constexpr Vector4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w) {}

    // 単一グレースケール値 + アルファ値での初期化
    constexpr Vector4(float rgb, float a = 1.0f)
        : x(rgb), y(rgb), z(rgb), w(a) {}

    // 演算子
    constexpr Vector4 operator+(const Vector4& other) const {
        return { x + other.x, y + other.y, z + other.z, w + other.w };
    }
    constexpr Vector4 operator-(const Vector4& other) const {
        return { x - other.x, y - other.y, z - other.z, w - other.w };
    }
    constexpr Vector4 operator-() const {
        return { -x, -y, -z, -w };
    }
    constexpr Vector4 operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar, w * scalar };
    }
    constexpr Vector4 operator*(const Vector4& other) const {
        return { x * other.x, y * other.y, z * other.z, w * other.w };
    }
    constexpr Vector4 operator/(float scalar) const {
        return { x / scalar, y / scalar, z / scalar, w / scalar };
    }
    constexpr Vector4 operator/(const Vector4& other) const {
        return { x / other.x, y / other.y, z / other.z, w / other.w };
    }

    constexpr Vector4& operator+=(const Vector4& other) {
        x += other.x; y += other.y; z += other.z; w += other.w; return *this;
    }
    constexpr Vector4& operator-=(const Vector4& other) {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this;
    }
    constexpr Vector4& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this;
    }
    constexpr Vector4& operator*=(const Vector4& other) {
        x *= other.x; y *= other.y; z *= other.z; w *= other.w; return *this;
    }
    constexpr Vector4& operator/=(float scalar) {
        x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this;
    }

    constexpr bool operator==(const Vector4& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
    constexpr bool operator!=(const Vector4& other) const {
        return !(*this == other);
    }

    /// <summary>
    /// アルファ値を変更した新しいVector4を返します
    /// </summary>
    constexpr Vector4 WithAlpha(float newAlpha) const {
        return { x, y, z, newAlpha };
    }

    // 主要なカラー定数 (Vector4::Red などの形式でもアクセス可能)
    static const Vector4 Red;
    static const Vector4 Green;
    static const Vector4 Blue;
    static const Vector4 Yellow;
    static const Vector4 Cyan;
    static const Vector4 Magenta;
    static const Vector4 White;
    static const Vector4 Black;
    static const Vector4 Gray;
    static const Vector4 Clear;
};

// スカラー * Vector4
inline constexpr Vector4 operator*(float scalar, const Vector4& v) {
    return v * scalar;
}

// 静的カラー定数の実体定義
inline constexpr Vector4 Vector4::Red{ 1.0f, 0.0f, 0.0f, 1.0f };
inline constexpr Vector4 Vector4::Green{ 0.0f, 1.0f, 0.0f, 1.0f };
inline constexpr Vector4 Vector4::Blue{ 0.0f, 0.0f, 1.0f, 1.0f };
inline constexpr Vector4 Vector4::Yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
inline constexpr Vector4 Vector4::Cyan{ 0.0f, 1.0f, 1.0f, 1.0f };
inline constexpr Vector4 Vector4::Magenta{ 1.0f, 0.0f, 1.0f, 1.0f };
inline constexpr Vector4 Vector4::White{ 1.0f, 1.0f, 1.0f, 1.0f };
inline constexpr Vector4 Vector4::Black{ 0.0f, 0.0f, 0.0f, 1.0f };
inline constexpr Vector4 Vector4::Gray{ 0.5f, 0.5f, 0.5f, 1.0f };
inline constexpr Vector4 Vector4::Clear{ 0.0f, 0.0f, 0.0f, 0.0f };

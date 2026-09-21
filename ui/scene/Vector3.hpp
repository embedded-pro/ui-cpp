#pragma once

#include <algorithm>
#include <cmath>

namespace ui::scene
{
    struct Vector3
    {
        float x{ 0.0f };
        float y{ 0.0f };
        float z{ 0.0f };

        [[nodiscard]] constexpr bool operator==(const Vector3& other) const = default;
    };

    [[nodiscard]] constexpr Vector3 operator+(Vector3 a, Vector3 b)
    {
        return Vector3{ a.x + b.x, a.y + b.y, a.z + b.z };
    }

    [[nodiscard]] constexpr Vector3 operator-(Vector3 a, Vector3 b)
    {
        return Vector3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }

    [[nodiscard]] constexpr Vector3 operator*(Vector3 value, float scale)
    {
        return Vector3{ value.x * scale, value.y * scale, value.z * scale };
    }

    [[nodiscard]] constexpr float Dot(Vector3 a, Vector3 b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    [[nodiscard]] constexpr Vector3 Cross(Vector3 a, Vector3 b)
    {
        return Vector3{ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }

    [[nodiscard]] inline float Length(Vector3 value)
    {
        return std::sqrt(Dot(value, value));
    }

    // Clamps the length rather than returning a zero vector, matching the guard the ported Qt
    // widget used: a degenerate direction keeps its components and simply stops shrinking.
    [[nodiscard]] inline Vector3 Normalized(Vector3 value, float epsilon = 1e-6f)
    {
        return value * (1.0f / std::max(Length(value), epsilon));
    }

    inline constexpr Vector3 worldUp{ 0.0f, 0.0f, 1.0f };
}

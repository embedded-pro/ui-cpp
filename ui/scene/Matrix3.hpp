#pragma once

#include "ui/scene/Vector3.hpp"
#include <array>
#include <cmath>

namespace ui::scene
{
    struct Matrix3
    {
        std::array<Vector3, 3> columns{
            Vector3{ 1.0f, 0.0f, 0.0f },
            Vector3{ 0.0f, 1.0f, 0.0f },
            Vector3{ 0.0f, 0.0f, 1.0f }
        };

        [[nodiscard]] constexpr bool operator==(const Matrix3& other) const = default;

        [[nodiscard]] static constexpr Matrix3 Identity()
        {
            return Matrix3{};
        }

        [[nodiscard]] static Matrix3 AxisAngle(Vector3 axis, float radians);
        [[nodiscard]] static Matrix3 RotationX(float radians);
        [[nodiscard]] static Matrix3 RotationY(float radians);
        [[nodiscard]] static Matrix3 RotationZ(float radians);

        // Fixed-axis roll about X, then pitch about Y, then yaw about Z: the URDF convention.
        [[nodiscard]] static Matrix3 FromRpy(float roll, float pitch, float yaw);
    };

    [[nodiscard]] constexpr Vector3 operator*(const Matrix3& matrix, Vector3 value)
    {
        return matrix.columns[0] * value.x + matrix.columns[1] * value.y + matrix.columns[2] * value.z;
    }

    [[nodiscard]] constexpr Matrix3 operator*(const Matrix3& a, const Matrix3& b)
    {
        return Matrix3{ { a * b.columns[0], a * b.columns[1], a * b.columns[2] } };
    }

    [[nodiscard]] constexpr Matrix3 Transposed(const Matrix3& matrix)
    {
        const auto& c = matrix.columns;

        return Matrix3{ { Vector3{ c[0].x, c[1].x, c[2].x },
            Vector3{ c[0].y, c[1].y, c[2].y },
            Vector3{ c[0].z, c[1].z, c[2].z } } };
    }

    inline Matrix3 Matrix3::AxisAngle(Vector3 axis, float radians)
    {
        const auto k = Normalized(axis);
        const auto c = std::cos(radians);
        const auto s = std::sin(radians);

        const auto column = [&](Vector3 unit, float kComponent)
        {
            return unit * c + Cross(k, unit) * s + k * ((1.0f - c) * kComponent);
        };

        return Matrix3{ { column(Vector3{ 1.0f, 0.0f, 0.0f }, k.x),
            column(Vector3{ 0.0f, 1.0f, 0.0f }, k.y),
            column(Vector3{ 0.0f, 0.0f, 1.0f }, k.z) } };
    }

    inline Matrix3 Matrix3::RotationX(float radians)
    {
        return AxisAngle(Vector3{ 1.0f, 0.0f, 0.0f }, radians);
    }

    inline Matrix3 Matrix3::RotationY(float radians)
    {
        return AxisAngle(Vector3{ 0.0f, 1.0f, 0.0f }, radians);
    }

    inline Matrix3 Matrix3::RotationZ(float radians)
    {
        return AxisAngle(Vector3{ 0.0f, 0.0f, 1.0f }, radians);
    }

    inline Matrix3 Matrix3::FromRpy(float roll, float pitch, float yaw)
    {
        return RotationZ(yaw) * RotationY(pitch) * RotationX(roll);
    }
}

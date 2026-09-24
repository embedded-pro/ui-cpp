#pragma once

#include "ui/scene/Matrix3.hpp"
#include <cmath>
#include <numbers>
#include <span>

namespace ui::scene
{
    // Rigid only: rotation and translation, no scale. Inverting is a transpose, and a normal
    // transforms by the rotation alone. Scale lives on the part that is drawn, not the frame.
    struct Transform3
    {
        Matrix3 rotation;
        Vector3 translation;

        [[nodiscard]] constexpr bool operator==(const Transform3& other) const = default;

        [[nodiscard]] constexpr Vector3 Apply(Vector3 point) const
        {
            return rotation * point + translation;
        }

        [[nodiscard]] constexpr Vector3 ApplyDirection(Vector3 direction) const
        {
            return rotation * direction;
        }

        [[nodiscard]] constexpr Transform3 Inverse() const
        {
            const auto inverseRotation = Transposed(rotation);

            return Transform3{ inverseRotation, inverseRotation * (translation * -1.0f) };
        }

        [[nodiscard]] static constexpr Transform3 Translation(Vector3 offset)
        {
            return Transform3{ Matrix3{}, offset };
        }

        [[nodiscard]] static constexpr Transform3 Rotation(const Matrix3& rotation)
        {
            return Transform3{ rotation, Vector3{} };
        }

        // Origin at `from`, local +Z pointing at `to`: how a unit cylinder becomes a link.
        [[nodiscard]] static Transform3 AlignZ(Vector3 from, Vector3 to);

        // Classic (distal) Denavit-Hartenberg: Rz(theta) Tz(d) Tx(a) Rx(alpha).
        [[nodiscard]] static Transform3 FromDh(float a, float alpha, float d, float theta);

        // A 3x4 [R | t] in row-major order, the layout most kinematics libraries hand out.
        [[nodiscard]] static constexpr Transform3 FromRowMajor(std::span<const float, 12> values)
        {
            return Transform3{
                Matrix3{ { Vector3{ values[0], values[4], values[8] },
                    Vector3{ values[1], values[5], values[9] },
                    Vector3{ values[2], values[6], values[10] } } },
                Vector3{ values[3], values[7], values[11] }
            };
        }

        [[nodiscard]] friend constexpr Transform3 operator*(const Transform3& a, const Transform3& b)
        {
            return Transform3{ a.rotation * b.rotation, a.Apply(b.translation) };
        }
    };

    inline Transform3 Transform3::AlignZ(Vector3 from, Vector3 to)
    {
        constexpr Vector3 unitZ{ 0.0f, 0.0f, 1.0f };
        constexpr float parallelEpsilon{ 1e-6f };

        const auto direction = Normalized(to - from);
        const auto axis = Cross(unitZ, direction);
        const auto sine = Length(axis);
        const auto cosine = Dot(unitZ, direction);

        if (sine > parallelEpsilon)
            return Transform3{ Matrix3::AxisAngle(axis, std::atan2(sine, cosine)), from };

        if (cosine > 0.0f)
            return Translation(from);

        return Transform3{ Matrix3::RotationX(std::numbers::pi_v<float>), from };
    }

    inline Transform3 Transform3::FromDh(float a, float alpha, float d, float theta)
    {
        const auto ct = std::cos(theta);
        const auto st = std::sin(theta);
        const auto ca = std::cos(alpha);
        const auto sa = std::sin(alpha);

        return Transform3{
            Matrix3{ { Vector3{ ct, st, 0.0f },
                Vector3{ -st * ca, ct * ca, sa },
                Vector3{ st * sa, -ct * sa, ca } } },
            Vector3{ a * ct, a * st, d }
        };
    }
}

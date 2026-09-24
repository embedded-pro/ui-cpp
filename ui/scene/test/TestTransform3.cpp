#include "ui/scene/Transform3.hpp"
#include <array>
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::scene::Matrix3;
    using ui::scene::Transform3;
    using ui::scene::Vector3;

    constexpr float halfPi{ std::numbers::pi_v<float> / 2.0f };

    class Transform3Test
        : public ::testing::Test
    {
    protected:
        static void ExpectNear(Vector3 actual, Vector3 expected, float tolerance = 1e-5f)
        {
            EXPECT_NEAR(actual.x, expected.x, tolerance);
            EXPECT_NEAR(actual.y, expected.y, tolerance);
            EXPECT_NEAR(actual.z, expected.z, tolerance);
        }
    };
}

TEST_F(Transform3Test, ApplyRotatesThenTranslates)
{
    const Transform3 transform{ Matrix3::RotationZ(halfPi), Vector3{ 1.0f, 0.0f, 0.0f } };

    ExpectNear(transform.Apply(Vector3{ 1.0f, 0.0f, 0.0f }), Vector3{ 1.0f, 1.0f, 0.0f });
}

TEST_F(Transform3Test, ADirectionIgnoresTheTranslation)
{
    const Transform3 transform{ Matrix3{}, Vector3{ 5.0f, 5.0f, 5.0f } };

    ExpectNear(transform.ApplyDirection(Vector3{ 0.0f, 0.0f, 1.0f }), Vector3{ 0.0f, 0.0f, 1.0f });
}

TEST_F(Transform3Test, ComposingAppliesTheRightOperandFirst)
{
    const auto parent = Transform3::Translation(Vector3{ 0.0f, 0.0f, 1.0f });
    const auto child = Transform3::Rotation(Matrix3::RotationY(halfPi));

    ExpectNear((parent * child).Apply(Vector3{ 0.0f, 0.0f, 1.0f }), Vector3{ 1.0f, 0.0f, 1.0f });
}

TEST_F(Transform3Test, AComposedInverseRoundTrips)
{
    const Transform3 transform{ Matrix3::FromRpy(0.4f, -1.1f, 2.0f), Vector3{ 0.3f, -2.0f, 1.5f } };
    const Vector3 point{ 0.7f, 0.1f, -0.9f };

    ExpectNear(transform.Inverse().Apply(transform.Apply(point)), point);
    ExpectNear((transform * transform.Inverse()).translation, Vector3{});
}

TEST_F(Transform3Test, AlignZPointsLocalZAtTheTarget)
{
    const Vector3 from{ 1.0f, 1.0f, 0.0f };
    const Vector3 to{ 1.0f, 3.0f, 2.0f };
    const auto aligned = Transform3::AlignZ(from, to);

    ExpectNear(aligned.Apply(Vector3{}), from);
    ExpectNear(aligned.ApplyDirection(Vector3{ 0.0f, 0.0f, 1.0f }), ui::scene::Normalized(to - from));
}

TEST_F(Transform3Test, AlignZHandlesBothParallelCases)
{
    ExpectNear(Transform3::AlignZ(Vector3{}, Vector3{ 0.0f, 0.0f, 2.0f }).ApplyDirection(Vector3{ 0.0f, 0.0f, 1.0f }),
        Vector3{ 0.0f, 0.0f, 1.0f });
    ExpectNear(Transform3::AlignZ(Vector3{}, Vector3{ 0.0f, 0.0f, -2.0f }).ApplyDirection(Vector3{ 0.0f, 0.0f, 1.0f }),
        Vector3{ 0.0f, 0.0f, -1.0f });
}

TEST_F(Transform3Test, AZeroDhRowIsTheIdentity)
{
    const auto dh = Transform3::FromDh(0.0f, 0.0f, 0.0f, 0.0f);

    ExpectNear(dh.Apply(Vector3{ 1.0f, 2.0f, 3.0f }), Vector3{ 1.0f, 2.0f, 3.0f });
}

TEST_F(Transform3Test, ADhRowMatchesItsElementaryComposition)
{
    const auto a = 0.4f;
    const auto alpha = -0.6f;
    const auto d = 0.25f;
    const auto theta = 1.3f;

    const auto expected = Transform3::Rotation(Matrix3::RotationZ(theta)) * Transform3::Translation(Vector3{ 0.0f, 0.0f, d }) *
                          Transform3::Translation(Vector3{ a, 0.0f, 0.0f }) * Transform3::Rotation(Matrix3::RotationX(alpha));
    const auto actual = Transform3::FromDh(a, alpha, d, theta);
    const Vector3 probe{ 0.2f, -0.3f, 0.5f };

    ExpectNear(actual.Apply(probe), expected.Apply(probe));
}

TEST_F(Transform3Test, ARowMajorMatrixReadsRowsIntoColumns)
{
    constexpr std::array<float, 12> values{
        0.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 2.0f,
        0.0f, 0.0f, 1.0f, 3.0f
    };

    const auto transform = Transform3::FromRowMajor(values);

    ExpectNear(transform.Apply(Vector3{ 1.0f, 0.0f, 0.0f }), Vector3{ 1.0f, 3.0f, 3.0f });
}

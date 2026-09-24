#include "ui/scene/Matrix3.hpp"
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::scene::Matrix3;
    using ui::scene::Vector3;

    constexpr float halfPi{ std::numbers::pi_v<float> / 2.0f };

    class Matrix3Test
        : public ::testing::Test
    {
    protected:
        static void ExpectNear(Vector3 actual, Vector3 expected, float tolerance = 1e-5f)
        {
            EXPECT_NEAR(actual.x, expected.x, tolerance);
            EXPECT_NEAR(actual.y, expected.y, tolerance);
            EXPECT_NEAR(actual.z, expected.z, tolerance);
        }

        static void ExpectNear(const Matrix3& actual, const Matrix3& expected)
        {
            for (std::size_t i = 0; i < 3; ++i)
                ExpectNear(actual.columns[i], expected.columns[i]);
        }
    };
}

TEST_F(Matrix3Test, ADefaultMatrixIsTheIdentity)
{
    const Vector3 value{ 1.0f, -2.0f, 3.0f };

    EXPECT_EQ(Matrix3{} * value, value);
    EXPECT_EQ(Matrix3::Identity(), Matrix3{});
}

TEST_F(Matrix3Test, AQuarterTurnAboutZTakesXToY)
{
    ExpectNear(Matrix3::RotationZ(halfPi) * Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f });
}

TEST_F(Matrix3Test, AQuarterTurnAboutXTakesYToZ)
{
    ExpectNear(Matrix3::RotationX(halfPi) * Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f });
}

TEST_F(Matrix3Test, AQuarterTurnAboutYTakesZToX)
{
    ExpectNear(Matrix3::RotationY(halfPi) * Vector3{ 0.0f, 0.0f, 1.0f }, Vector3{ 1.0f, 0.0f, 0.0f });
}

TEST_F(Matrix3Test, AnUnnormalisedAxisIsNormalisedFirst)
{
    ExpectNear(Matrix3::AxisAngle(Vector3{ 0.0f, 0.0f, 5.0f }, 0.7f), Matrix3::RotationZ(0.7f));
}

TEST_F(Matrix3Test, TheTransposeOfARotationIsItsInverse)
{
    const auto rotation = Matrix3::AxisAngle(Vector3{ 1.0f, 2.0f, -0.5f }, 1.1f);

    ExpectNear(Transposed(rotation) * rotation, Matrix3{});
}

TEST_F(Matrix3Test, RollPitchYawAppliesRollFirst)
{
    const auto rpy = Matrix3::FromRpy(0.3f, -0.4f, 1.2f);
    const auto composed = Matrix3::RotationZ(1.2f) * Matrix3::RotationY(-0.4f) * Matrix3::RotationX(0.3f);

    ExpectNear(rpy, composed);
}

TEST_F(Matrix3Test, ARotationPreservesLength)
{
    const Vector3 value{ 0.3f, -1.7f, 2.2f };

    EXPECT_NEAR(ui::scene::Length(Matrix3::FromRpy(0.9f, 0.2f, -2.4f) * value), ui::scene::Length(value), 1e-5f);
}

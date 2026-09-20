#include "ui/scene/Vector3.hpp"
#include <gmock/gmock.h>

namespace
{
    using ui::scene::Cross;
    using ui::scene::Dot;
    using ui::scene::Length;
    using ui::scene::Normalized;
    using ui::scene::Vector3;

    class Vector3Test
        : public ::testing::Test
    {
    protected:
        static void ExpectNear(Vector3 actual, Vector3 expected)
        {
            EXPECT_NEAR(actual.x, expected.x, 1e-6f);
            EXPECT_NEAR(actual.y, expected.y, 1e-6f);
            EXPECT_NEAR(actual.z, expected.z, 1e-6f);
        }
    };
}

TEST_F(Vector3Test, ArithmeticIsComponentwise)
{
    ExpectNear(Vector3{ 1.0f, 2.0f, 3.0f } + Vector3{ 4.0f, 5.0f, 6.0f }, Vector3{ 5.0f, 7.0f, 9.0f });
    ExpectNear(Vector3{ 4.0f, 5.0f, 6.0f } - Vector3{ 1.0f, 2.0f, 3.0f }, Vector3{ 3.0f, 3.0f, 3.0f });
    ExpectNear(Vector3{ 1.0f, 2.0f, 3.0f } * 2.0f, Vector3{ 2.0f, 4.0f, 6.0f });
}

TEST_F(Vector3Test, DotIsZeroForPerpendicularVectors)
{
    EXPECT_NEAR(Dot(Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }), 0.0f, 1e-6f);
    EXPECT_NEAR(Dot(Vector3{ 1.0f, 2.0f, 3.0f }, Vector3{ 1.0f, 2.0f, 3.0f }), 14.0f, 1e-5f);
}

TEST_F(Vector3Test, CrossFollowsTheRightHandRule)
{
    ExpectNear(Cross(Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }), Vector3{ 0.0f, 0.0f, 1.0f });
    ExpectNear(Cross(Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }), Vector3{ 0.0f, 0.0f, -1.0f });
}

// The identity the camera's reference values rest on: crossing with the world up vector reduces
// to a component swap, so the generic Cross and the Qt original's hand-inlined version produce
// bit-identical floats rather than merely close ones.
TEST_F(Vector3Test, CrossingWithWorldUpIsExactlyAComponentSwap)
{
    const Vector3 forward{ 0.3f, -0.7f, 0.2f };
    const auto crossed = Cross(forward, ui::scene::worldUp);

    EXPECT_FLOAT_EQ(crossed.x, forward.y);
    EXPECT_FLOAT_EQ(crossed.y, -forward.x);
    EXPECT_FLOAT_EQ(crossed.z, 0.0f);
}

TEST_F(Vector3Test, CrossingAParallelVectorIsZero)
{
    ExpectNear(Cross(ui::scene::worldUp, ui::scene::worldUp), Vector3{});
}

TEST_F(Vector3Test, LengthIsTheEuclideanNorm)
{
    EXPECT_NEAR(Length(Vector3{ 3.0f, 4.0f, 0.0f }), 5.0f, 1e-6f);
    EXPECT_NEAR(Length(Vector3{}), 0.0f, 1e-6f);
}

TEST_F(Vector3Test, NormalizingLeavesAUnitVectorAlone)
{
    ExpectNear(Normalized(Vector3{ 0.0f, 0.0f, 1.0f }), Vector3{ 0.0f, 0.0f, 1.0f });
    EXPECT_NEAR(Length(Normalized(Vector3{ 3.0f, 4.0f, 0.0f })), 1.0f, 1e-6f);
}

TEST_F(Vector3Test, NormalizingADegenerateVectorDividesByTheFloorRatherThanProducingNaN)
{
    const auto normalized = Normalized(Vector3{});

    EXPECT_FALSE(std::isnan(normalized.x));
    EXPECT_FALSE(std::isnan(normalized.y));
    EXPECT_FALSE(std::isnan(normalized.z));
    EXPECT_NEAR(Length(normalized), 0.0f, 1e-6f);
}

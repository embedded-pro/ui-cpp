#include "ui/core/Format.hpp"
#include "ui/core/Geometry.hpp"
#include <gmock/gmock.h>
#include <limits>

namespace
{
    class GeometryTest
        : public ::testing::Test
    {
    protected:
        static constexpr float tolerance{ 1e-5f };
    };
}

TEST_F(GeometryTest, RectBottomIsTopPlusHeight)
{
    const ui::Rect rect{ 10.0f, 20.0f, 100.0f, 50.0f };

    EXPECT_NEAR(rect.Left(), 10.0f, tolerance);
    EXPECT_NEAR(rect.Top(), 20.0f, tolerance);
    EXPECT_NEAR(rect.Right(), 110.0f, tolerance);
    EXPECT_NEAR(rect.Bottom(), 70.0f, tolerance);
}

TEST_F(GeometryTest, RectCenterIsMidpoint)
{
    const ui::Rect rect{ 0.0f, 0.0f, 100.0f, 40.0f };

    EXPECT_NEAR(rect.Center().x, 50.0f, tolerance);
    EXPECT_NEAR(rect.Center().y, 20.0f, tolerance);
}

TEST_F(GeometryTest, RectContainsPointsOnTheBoundary)
{
    const ui::Rect rect{ 0.0f, 0.0f, 10.0f, 10.0f };

    EXPECT_TRUE(rect.Contains(ui::Point{ 5.0f, 5.0f }));
    EXPECT_TRUE(rect.Contains(ui::Point{ 0.0f, 0.0f }));
    EXPECT_TRUE(rect.Contains(ui::Point{ 10.0f, 10.0f }));
    EXPECT_FALSE(rect.Contains(ui::Point{ 10.1f, 5.0f }));
}

TEST_F(GeometryTest, RectIntersectedReturnsEmptyWhenDisjoint)
{
    const ui::Rect left{ 0.0f, 0.0f, 10.0f, 10.0f };
    const ui::Rect right{ 20.0f, 0.0f, 10.0f, 10.0f };

    EXPECT_TRUE(left.Intersected(right).IsEmpty());
}

TEST_F(GeometryTest, RectIntersectedReturnsOverlap)
{
    const ui::Rect left{ 0.0f, 0.0f, 10.0f, 10.0f };
    const ui::Rect right{ 5.0f, 5.0f, 10.0f, 10.0f };
    const auto overlap = left.Intersected(right);

    EXPECT_NEAR(overlap.x, 5.0f, tolerance);
    EXPECT_NEAR(overlap.y, 5.0f, tolerance);
    EXPECT_NEAR(overlap.width, 5.0f, tolerance);
    EXPECT_NEAR(overlap.height, 5.0f, tolerance);
}

TEST_F(GeometryTest, RectAdjustedShrinksByMargins)
{
    const ui::Rect rect{ 0.0f, 0.0f, 100.0f, 100.0f };
    const auto inner = rect.Adjusted(65.0f, 15.0f, -20.0f, -35.0f);

    EXPECT_NEAR(inner.x, 65.0f, tolerance);
    EXPECT_NEAR(inner.y, 15.0f, tolerance);
    EXPECT_NEAR(inner.width, 15.0f, tolerance);
    EXPECT_NEAR(inner.height, 50.0f, tolerance);
}

TEST_F(GeometryTest, ClampBoundsValue)
{
    EXPECT_NEAR(ui::Clamp(-1.0f, 0.0f, 10.0f), 0.0f, tolerance);
    EXPECT_NEAR(ui::Clamp(11.0f, 0.0f, 10.0f), 10.0f, tolerance);
    EXPECT_NEAR(ui::Clamp(5.0f, 0.0f, 10.0f), 5.0f, tolerance);
}

TEST_F(GeometryTest, LerpInterpolates)
{
    EXPECT_NEAR(ui::Lerp(0.0f, 10.0f, 0.25f), 2.5f, tolerance);
}

TEST_F(GeometryTest, TransformRotatesAboutTheOrigin)
{
    const ui::Transform transform{ 0.0f, 0.0f, 90.0f, 1.0f, 1.0f };
    const auto rotated = transform.Apply(ui::Point{ 1.0f, 0.0f });

    EXPECT_NEAR(rotated.x, 0.0f, 1e-4f);
    EXPECT_NEAR(rotated.y, 1.0f, 1e-4f);
}

TEST_F(GeometryTest, TransformAppliesScaleThenTranslation)
{
    const ui::Transform transform{ 5.0f, 7.0f, 0.0f, 2.0f, 3.0f };
    const auto mapped = transform.Apply(ui::Point{ 1.0f, 1.0f });

    EXPECT_NEAR(mapped.x, 7.0f, tolerance);
    EXPECT_NEAR(mapped.y, 10.0f, tolerance);
}

TEST_F(GeometryTest, FormatFixedHonoursDecimals)
{
    EXPECT_EQ(ui::FormatFixed(3.14159f, 2), "3.14");
}

TEST_F(GeometryTest, FormatEngineeringAppliesPrefix)
{
    EXPECT_EQ(ui::FormatEngineering(1500.0f, 1), "1.5k");
    EXPECT_EQ(ui::FormatEngineering(0.002f, 1), "2.0m");
}

// log10 of zero is undefined and log10 of a non-finite value is meaningless, so neither may reach
// the exponent arithmetic; both fall back to plain fixed formatting.
TEST_F(GeometryTest, FormatEngineeringFallsBackForZeroAndNonFinite)
{
    EXPECT_EQ(ui::FormatEngineering(0.0f, 1), ui::FormatFixed(0.0f, 1));
    EXPECT_EQ(ui::FormatEngineering(std::numeric_limits<float>::infinity(), 1),
        ui::FormatFixed(std::numeric_limits<float>::infinity(), 1));
}

// The motivating case: one read-out column carries values several decades apart, where a fixed
// count of places is either all zeros at the small end or all noise at the large one.
TEST_F(GeometryTest, SignificantKeepsDigitsRatherThanPlaces)
{
    ui::FormatBuffer<32> buffer;

    EXPECT_EQ(buffer.Significant(0.00022f, 4), "0.00022");
    EXPECT_EQ(buffer.Significant(1234.5f, 4), "1234");
    EXPECT_EQ(buffer.Significant(0.5f, 4), "0.5");
    EXPECT_EQ(buffer.Fixed(0.00022f, 4), "0.0002");
}

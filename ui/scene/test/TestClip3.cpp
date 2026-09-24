#include "ui/scene/Clip3.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::scene::Vector3;

    constexpr float nearDistance{ 0.1f };

    class Clip3Test
        : public ::testing::Test
    {
    protected:
        std::array<Vector3, 5> out{};
    };
}

TEST_F(Clip3Test, APolygonEntirelyInFrontIsUnchanged)
{
    const std::array<Vector3, 3> triangle{ Vector3{ 0.0f, 0.0f, 1.0f }, Vector3{ 1.0f, 0.0f, 1.0f }, Vector3{ 0.0f, 1.0f, 2.0f } };

    ASSERT_EQ(ui::scene::ClipPolygonNear(triangle, nearDistance, out), 3u);
    EXPECT_EQ(out[0], triangle[0]);
    EXPECT_EQ(out[2], triangle[2]);
}

TEST_F(Clip3Test, APolygonEntirelyBehindVanishes)
{
    const std::array<Vector3, 3> triangle{ Vector3{ 0.0f, 0.0f, -1.0f }, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.05f } };

    EXPECT_EQ(ui::scene::ClipPolygonNear(triangle, nearDistance, out), 0u);
}

TEST_F(Clip3Test, ATriangleWithOneVertexBehindBecomesAQuadOnThePlane)
{
    const std::array<Vector3, 3> triangle{ Vector3{ 0.0f, 0.0f, -1.0f }, Vector3{ 1.0f, 0.0f, 1.0f }, Vector3{ -1.0f, 0.0f, 1.0f } };

    const auto count = ui::scene::ClipPolygonNear(triangle, nearDistance, out);

    ASSERT_EQ(count, 4u);

    for (std::size_t i = 0; i < count; ++i)
        EXPECT_GE(out[i].z, nearDistance - 1e-6f);
}

TEST_F(Clip3Test, TheCrossingPointIsInterpolatedAlongTheEdge)
{
    const std::array<Vector3, 3> triangle{ Vector3{ 0.0f, 0.0f, -0.9f }, Vector3{ 2.0f, 0.0f, 1.1f }, Vector3{ 0.0f, 0.0f, 1.1f } };

    ASSERT_EQ(ui::scene::ClipPolygonNear(triangle, nearDistance, out), 4u);
    EXPECT_NEAR(out[0].x, 1.0f, 1e-5f);
    EXPECT_NEAR(out[0].z, nearDistance, 1e-6f);
}

TEST_F(Clip3Test, AShortOutputTruncatesRatherThanOverruns)
{
    const std::array<Vector3, 3> triangle{ Vector3{ 0.0f, 0.0f, -1.0f }, Vector3{ 1.0f, 0.0f, 1.0f }, Vector3{ -1.0f, 0.0f, 1.0f } };
    std::array<Vector3, 2> small{};

    EXPECT_EQ(ui::scene::ClipPolygonNear(triangle, nearDistance, small), 2u);
}

TEST_F(Clip3Test, ASegmentCrossingThePlaneIsShortened)
{
    Vector3 from{ 0.0f, 0.0f, -1.9f };
    Vector3 to{ 0.0f, 2.0f, 2.1f };

    ASSERT_TRUE(ui::scene::ClipSegmentNear(from, to, nearDistance));
    EXPECT_NEAR(from.z, nearDistance, 1e-6f);
    EXPECT_NEAR(from.y, 1.0f, 1e-5f);
    EXPECT_EQ(to, (Vector3{ 0.0f, 2.0f, 2.1f }));
}

TEST_F(Clip3Test, ASegmentBehindThePlaneIsRejected)
{
    Vector3 from{ 0.0f, 0.0f, -1.0f };
    Vector3 to{ 0.0f, 0.0f, 0.0f };

    EXPECT_FALSE(ui::scene::ClipSegmentNear(from, to, nearDistance));
}

#include "MeshTestSupport.hpp"
#include "ui/stage/Primitives.hpp"
#include <array>
#include <gmock/gmock.h>
#include <span>

namespace
{
    using ui::stage::Mesh;
    using ui::stage::Tessellation;
    using ui::stage::Vector3;

    class PrimitivesTest
        : public ::testing::Test
    {
    protected:
        static void ExpectOutwardNormals(const Mesh& mesh, Vector3 centre)
        {
            for (const auto& face : mesh.faces)
            {
                const auto outward = ui::stage::test::Centroid(mesh, face) - centre;
                EXPECT_GT(ui::scene::Dot(face.normal, outward), 0.0f);
                EXPECT_NEAR(ui::scene::Length(face.normal), 1.0f, 1e-4f);
            }
        }
    };
}

TEST_F(PrimitivesTest, TheBoxHasEightCornersSixQuadsAndTwelveEdges)
{
    const auto box = ui::stage::MakeBox();

    EXPECT_EQ(box.vertices.size(), 8u);
    EXPECT_EQ(box.faces.size(), 6u);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(box), 24u);
    ExpectOutwardNormals(box, Vector3{});
}

TEST_F(PrimitivesTest, TheBoxIsAUnitCubeAboutTheOrigin)
{
    const auto box = ui::stage::MakeBox();

    EXPECT_NEAR(box.boundsMin.x, -0.5f, 1e-6f);
    EXPECT_NEAR(box.boundsMax.z, 0.5f, 1e-6f);
}

TEST_F(PrimitivesTest, TheCylinderOutlinesOnlyItsRims)
{
    const auto cylinder = ui::stage::MakeCylinder(Tessellation{ 20, 10 });

    EXPECT_EQ(cylinder.vertices.size(), 42u);
    EXPECT_EQ(cylinder.faces.size(), 60u);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(cylinder), 80u);
    ExpectOutwardNormals(cylinder, Vector3{ 0.0f, 0.0f, 0.5f });
}

TEST_F(PrimitivesTest, TheCylinderStandsOnTheOriginAndReachesUnitHeight)
{
    const auto cylinder = ui::stage::MakeCylinder();

    EXPECT_NEAR(cylinder.boundsMin.z, 0.0f, 1e-6f);
    EXPECT_NEAR(cylinder.boundsMax.z, 1.0f, 1e-6f);
    EXPECT_NEAR(cylinder.boundsMax.x, 1.0f, 1e-6f);
}

TEST_F(PrimitivesTest, TheConeOutlinesOnlyItsBase)
{
    const auto cone = ui::stage::MakeCone(Tessellation{ 20, 10 });

    EXPECT_EQ(cone.vertices.size(), 22u);
    EXPECT_EQ(cone.faces.size(), 40u);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(cone), 40u);
    ExpectOutwardNormals(cone, Vector3{ 0.0f, 0.0f, 0.25f });
}

TEST_F(PrimitivesTest, TheSphereIsSmoothAndClosed)
{
    const auto sphere = ui::stage::MakeSphere(Tessellation{ 20, 10 });

    EXPECT_EQ(sphere.vertices.size(), 182u);
    EXPECT_EQ(sphere.faces.size(), 200u);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(sphere), 0u);
    ExpectOutwardNormals(sphere, Vector3{});
}

TEST_F(PrimitivesTest, TessellationIsClampedAtBothEnds)
{
    EXPECT_EQ(ui::stage::MakeCylinder(Tessellation{ 1, 10 }).faces.size(), 3u * Tessellation::minimumSegments);
    EXPECT_EQ(ui::stage::MakeCylinder(Tessellation{ 250, 10 }).faces.size(), 3u * Tessellation::maximumSegments);
    EXPECT_EQ(ui::stage::MakeSphere(Tessellation{ 4, 0 }).faces.size(), 2u * 4u);
}

TEST_F(PrimitivesTest, TheCapsuleSpansBothHemispheres)
{
    const auto capsule = ui::stage::MakeCapsule(0.1f, 0.5f, Tessellation{ 16, 8 });

    EXPECT_NEAR(capsule.boundsMin.z, -0.1f, 1e-6f);
    EXPECT_NEAR(capsule.boundsMax.z, 0.6f, 1e-6f);
    EXPECT_NEAR(capsule.boundsMax.x, 0.1f, 1e-6f);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(capsule), 0u);
}

TEST_F(PrimitivesTest, ThePlaneIsOneQuadWithAnOutlinedBoundary)
{
    const auto plane = ui::stage::MakePlane();

    ASSERT_EQ(plane.faces.size(), 1u);
    EXPECT_EQ(plane.faces[0].count, 4u);
    EXPECT_NEAR(plane.faces[0].normal.z, 1.0f, 1e-6f);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(plane), 4u);
}

TEST_F(PrimitivesTest, ALatheTurnsAProfileIntoAClosedSolidWithOutwardFaces)
{
    const std::array<ui::stage::ProfilePoint, 6> profile{ { { 0.0f, 1.0f }, { 0.8f, 1.0f }, { 1.0f, 0.8f }, { 1.0f, 0.2f }, { 0.8f, 0.0f }, { 0.0f, 0.0f } } };
    const auto hub = ui::stage::MakeLathe(profile, Tessellation{ 16, 2 });

    EXPECT_EQ(hub.vertices.size(), 2u + 4u * 16u);
    EXPECT_EQ(hub.faces.size(), 2u * 16u + 3u * 16u);
    EXPECT_NEAR(hub.boundsMax.x, 1.0f, 1e-6f);
    EXPECT_NEAR(hub.boundsMax.z, 1.0f, 1e-6f);
    ExpectOutwardNormals(hub, Vector3{ 0.0f, 0.0f, 0.5f });
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(hub), 2u * 4u * 16u);

    EXPECT_TRUE(ui::stage::MakeLathe(std::span(profile).first(2)).faces.empty());
}

TEST_F(PrimitivesTest, AnExtrusionCapsBothEndsAndOutlinesOnlyItsStraightCorners)
{
    const auto outline = ui::stage::RoundedRectangle(2.0f, 1.0f, 0.3f, 6);
    const auto bar = ui::stage::MakeExtrusion(outline, -0.5f, 0.5f);
    const auto sides = outline.size();

    EXPECT_EQ(bar.faces.size(), 3u * sides);
    EXPECT_NEAR(bar.boundsMin.x, -1.0f, 1e-6f);
    EXPECT_NEAR(bar.boundsMax.y, 0.5f, 1e-6f);
    ExpectOutwardNormals(bar, Vector3{});
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(bar), 2u * 2u * sides);

    const auto square = ui::stage::MakeExtrusion(ui::stage::RoundedRectangle(1.0f, 1.0f, 0.0f, 4), 0.0f, 1.0f);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(square), 2u * (2u * 4u + 4u));
    EXPECT_TRUE(ui::stage::MakeExtrusion(std::span(outline).first(2), 0.0f, 1.0f).faces.empty());
}

TEST_F(PrimitivesTest, OutlinesRunCounterClockwiseAroundTheirCentre)
{
    for (const auto& outline : { ui::stage::Stadium(3.0f, 1.0f, 8), ui::stage::RegularPolygon(6, 1.0f), ui::stage::RoundedRectangle(1.0f, 2.0f, 0.2f, 4) })
    {
        float area{ 0.0f };
        for (std::size_t i = 0; i < outline.size(); ++i)
        {
            const auto& p = outline[i];
            const auto& q = outline[(i + 1) % outline.size()];
            area += p.x * q.y - q.x * p.y;
        }
        EXPECT_GT(area, 0.0f);
    }

    const auto slot = ui::stage::Stadium(3.0f, 1.0f, 8);
    EXPECT_EQ(slot.size(), 18u);
    EXPECT_NEAR(slot.back().y, 3.0f, 1e-6f);
    EXPECT_EQ(ui::stage::RegularPolygon(6, 1.0f).size(), 6u);
}

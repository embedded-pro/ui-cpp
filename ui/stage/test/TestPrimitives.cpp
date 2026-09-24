#include "MeshTestSupport.hpp"
#include "ui/stage/Primitives.hpp"
#include <gmock/gmock.h>

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

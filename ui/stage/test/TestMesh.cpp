#include "MeshTestSupport.hpp"
#include "ui/stage/Mesh.hpp"
#include <cmath>
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::stage::Mesh;
    using ui::stage::Vector3;

    class MeshTest
        : public ::testing::Test
    {
    protected:
        // Two triangles sharing the edge 0-2, the second lifted out of plane by `fold` radians.
        static Mesh Hinge(float fold)
        {
            Mesh mesh;
            mesh.AddVertex(Vector3{ 0.0f, 0.0f, 0.0f });
            mesh.AddVertex(Vector3{ 1.0f, 0.0f, 0.0f });
            mesh.AddVertex(Vector3{ 0.0f, 1.0f, 0.0f });
            mesh.AddVertex(Vector3{ -std::cos(fold), 0.5f, std::sin(fold) });
            mesh.AddTriangle(0, 1, 2);
            mesh.AddTriangle(0, 2, 3);
            mesh.Finish();
            return mesh;
        }
    };
}

TEST_F(MeshTest, ACounterClockwiseTriangleFacesPositiveZ)
{
    const auto mesh = Hinge(0.0f);

    EXPECT_NEAR(mesh.faces[0].normal.z, 1.0f, 1e-6f);
    EXPECT_NEAR(mesh.faces[1].normal.z, 1.0f, 1e-6f);
}

TEST_F(MeshTest, ACoplanarSharedEdgeIsNotAFeature)
{
    const auto mesh = Hinge(0.0f);

    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(mesh), 4u);
    EXPECT_EQ(mesh.faces[0].featureMask & std::byte{ 0b100 }, std::byte{ 0 });
}

TEST_F(MeshTest, ASharpFoldIsAFeatureOnBothSides)
{
    const auto mesh = Hinge(std::numbers::pi_v<float> / 2.0f);

    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(mesh), 6u);
}

TEST_F(MeshTest, BoundsEncloseEveryVertex)
{
    const auto mesh = Hinge(std::numbers::pi_v<float> / 2.0f);

    EXPECT_NEAR(mesh.boundsMin.x, 0.0f, 1e-6f);
    EXPECT_NEAR(mesh.boundsMax.x, 1.0f, 1e-6f);
    EXPECT_NEAR(mesh.boundsMax.y, 1.0f, 1e-6f);
    EXPECT_NEAR(mesh.boundsMax.z, 1.0f, 1e-6f);
}

TEST_F(MeshTest, CornersCountQuadsAsFour)
{
    Mesh mesh;

    for (auto i = 0; i < 4; ++i)
        mesh.AddVertex(Vector3{});

    mesh.AddQuad(0, 1, 2, 3);
    mesh.AddTriangle(0, 1, 2);

    EXPECT_EQ(mesh.CornerCount(), 7u);
}

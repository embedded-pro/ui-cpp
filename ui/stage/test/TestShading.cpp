#include "ui/stage/Primitives.hpp"
#include "ui/stage/Shading.hpp"
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::stage::ShadedMesh;
    using ui::stage::Vector3;

    class ShadingTest
        : public ::testing::Test
    {
    protected:
        static void ExpectUnitNormalsFacingTheirTriangles(const ShadedMesh& mesh)
        {
            for (std::size_t t = 0; t < mesh.Triangles(); ++t)
            {
                const auto& a = mesh.vertices[mesh.indices[3 * t]];
                const auto& b = mesh.vertices[mesh.indices[3 * t + 1]];
                const auto& c = mesh.vertices[mesh.indices[3 * t + 2]];
                const auto face = ui::scene::Cross(b.position - a.position, c.position - a.position);
                if (ui::scene::Length(face) < 1e-9f)
                    continue;
                for (const auto* corner : { &a, &b, &c })
                {
                    EXPECT_NEAR(ui::scene::Length(corner->normal), 1.0f, 1e-4f);
                    EXPECT_GT(ui::scene::Dot(corner->normal, face), 0.0f);
                }
            }
        }
    };
}

TEST_F(ShadingTest, ABoxKeepsAHardNormalOnEveryFace)
{
    const auto box = ui::stage::Shade(ui::stage::MakeBox());

    EXPECT_EQ(box.vertices.size(), 24u);
    EXPECT_EQ(box.Triangles(), 12u);
    EXPECT_NEAR(box.boundsMax.x, 0.5f, 1e-6f);
    ExpectUnitNormalsFacingTheirTriangles(box);
}

TEST_F(ShadingTest, ACylinderIsRoundAlongItsSideAndSharpAtItsRims)
{
    const auto cylinder = ui::stage::Shade(ui::stage::MakeCylinder(ui::stage::Tessellation{ 24, 2 }));

    EXPECT_EQ(cylinder.Triangles(), 24u * 4u);
    EXPECT_EQ(cylinder.vertices.size(), 2u + 2u * 24u + 2u * 24u);
    ExpectUnitNormalsFacingTheirTriangles(cylinder);

    for (const auto& vertex : cylinder.vertices)
    {
        const auto onSide = std::abs(vertex.normal.z) < 1e-5f;
        const auto onCap = std::abs(std::abs(vertex.normal.z) - 1.0f) < 1e-5f;
        EXPECT_TRUE(onSide || onCap);
        if (onSide)
        {
            EXPECT_NEAR(vertex.normal.x, vertex.position.x, 1e-4f);
        }
    }
}

TEST_F(ShadingTest, AWiderCreaseAngleSmoothsMoreCorners)
{
    const auto mesh = ui::stage::MakeExtrusion(ui::stage::RegularPolygon(6, 1.0f), 0.0f, 1.0f);

    EXPECT_GT(ui::stage::Shade(mesh, 30.0f).vertices.size(), ui::stage::Shade(mesh, 70.0f).vertices.size());
}

TEST_F(ShadingTest, AnAppendedPartKeepsItsShapeAtItsPlacement)
{
    ui::stage::Mesh assembly;
    assembly.Append(ui::stage::MakeBox(), ui::scene::Transform3::AlignZ(Vector3{ 2.0f, 0.0f, 0.0f }, Vector3{ 2.0f, 1.0f, 0.0f }));
    assembly.Append(ui::stage::MakeBox(), ui::scene::Transform3::Translation(Vector3{ -2.0f, 0.0f, 0.0f }));
    assembly.Finish();

    EXPECT_EQ(assembly.vertices.size(), 16u);
    EXPECT_NEAR(assembly.boundsMin.x, -2.5f, 1e-5f);
    EXPECT_NEAR(assembly.boundsMax.x, 2.5f, 1e-5f);
    const auto shaded = ui::stage::Shade(assembly);
    EXPECT_EQ(shaded.vertices.size(), 48u);
    ExpectUnitNormalsFacingTheirTriangles(shaded);
}

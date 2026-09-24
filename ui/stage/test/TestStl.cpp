#include "MeshTestSupport.hpp"
#include "ui/stage/Stl.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <gmock/gmock.h>
#include <limits>
#include <string_view>
#include <vector>

namespace
{
    using ui::stage::StlOptions;
    using ui::stage::Vector3;

    // A unit right tetrahedron, wound outward: four facets over four distinct corners.
    constexpr std::array<std::array<Vector3, 3>, 4> tetrahedron{ {
        { Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f } },
        { Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f } },
        { Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f }, Vector3{ 0.0f, 1.0f, 0.0f } },
        { Vector3{ 1.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, Vector3{ 0.0f, 0.0f, 1.0f } },
    } };

    constexpr std::string_view asciiTetrahedron{
        "solid tetra\n"
        "  facet normal 0 0 0\n    outer loop\n"
        "      vertex 0 0 0\n      vertex 0 1.0 0\n      vertex 1e0 0 0\n"
        "    endloop\n  endfacet\n"
        "  facet normal 0 0 0\n    outer loop\n"
        "      vertex 0 0 0\n      vertex 1 0 0\n      vertex 0 0 +1\n"
        "    endloop\n  endfacet\n"
        "  facet normal 0 0 0\n    outer loop\n"
        "      vertex 0.0 0.0 0.0\n      vertex 0 0 1\n      vertex 0 10.0E-1 0\n"
        "    endloop\n  endfacet\n"
        "  facet normal 0 0 0\n    outer loop\n"
        "      vertex 1 0 0\n      vertex 0 1 0\n      vertex 0 0 1\n"
        "    endloop\n  endfacet\n"
        "endsolid tetra\n"
    };

    class StlTest
        : public ::testing::Test
    {
    protected:
        static void Append(std::vector<std::byte>& bytes, std::uint32_t value)
        {
            for (auto i = 0; i < 4; ++i)
                bytes.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFu));
        }

        static void Append(std::vector<std::byte>& bytes, float value)
        {
            Append(bytes, std::bit_cast<std::uint32_t>(value));
        }

        static std::vector<std::byte> Binary(float poison = 0.0f)
        {
            std::vector<std::byte> bytes(80, std::byte{ 0 });
            Append(bytes, static_cast<std::uint32_t>(tetrahedron.size()));

            for (const auto& facet : tetrahedron)
            {
                for (auto i = 0; i < 3; ++i)
                    Append(bytes, 0.0f);

                for (const auto& corner : facet)
                {
                    Append(bytes, corner.x + poison);
                    Append(bytes, corner.y);
                    Append(bytes, corner.z);
                }

                bytes.push_back(std::byte{ 0 });
                bytes.push_back(std::byte{ 0 });
            }

            return bytes;
        }

        static std::span<const std::byte> Bytes(std::string_view text)
        {
            return std::as_bytes(std::span{ text.data(), text.size() });
        }
    };
}

TEST_F(StlTest, ABinaryTetrahedronWeldsToFourCorners)
{
    const auto mesh = ui::stage::ParseStl(Binary());

    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->faces.size(), 4u);
    EXPECT_EQ(mesh->vertices.size(), 4u);
    EXPECT_EQ(ui::stage::test::FeatureEdgeUses(*mesh), 12u);
}

TEST_F(StlTest, NormalsAreRecomputedFromTheWinding)
{
    const auto mesh = ui::stage::ParseStl(Binary());

    ASSERT_TRUE(mesh.has_value());

    for (const auto& face : mesh->faces)
    {
        const auto outward = ui::stage::test::Centroid(*mesh, face) - Vector3{ 0.25f, 0.25f, 0.25f };
        EXPECT_GT(ui::scene::Dot(face.normal, outward), 0.0f);
    }
}

TEST_F(StlTest, WithoutWeldingEveryCornerIsItsOwnVertex)
{
    const auto mesh = ui::stage::ParseStl(Binary(), StlOptions{ 1.0f, false });

    ASSERT_TRUE(mesh.has_value());
    EXPECT_EQ(mesh->vertices.size(), 12u);
}

TEST_F(StlTest, TheScaleConvertsUnits)
{
    const auto mesh = ui::stage::ParseStl(Binary(), StlOptions{ 0.001f });

    ASSERT_TRUE(mesh.has_value());
    EXPECT_NEAR(mesh->boundsMax.x, 0.001f, 1e-9f);
}

TEST_F(StlTest, AnAsciiTetrahedronMatchesTheBinaryOne)
{
    const auto ascii = ui::stage::ParseStl(Bytes(asciiTetrahedron));
    const auto binary = ui::stage::ParseStl(Binary());

    ASSERT_TRUE(ascii.has_value());
    ASSERT_TRUE(binary.has_value());
    EXPECT_EQ(ascii->faces.size(), binary->faces.size());
    EXPECT_EQ(ascii->vertices.size(), binary->vertices.size());
    EXPECT_NEAR(ascii->boundsMax.y, 1.0f, 1e-6f);
}

TEST_F(StlTest, ATruncatedBinaryIsRejected)
{
    auto bytes = Binary();
    bytes.resize(bytes.size() - 10);

    EXPECT_FALSE(ui::stage::ParseStl(bytes).has_value());
}

TEST_F(StlTest, ANonFiniteCoordinateIsRejected)
{
    EXPECT_FALSE(ui::stage::ParseStl(Binary(std::numeric_limits<float>::quiet_NaN())).has_value());
}

TEST_F(StlTest, TooManyTrianglesAreRejected)
{
    StlOptions options;
    options.maximumTriangles = 3;

    EXPECT_FALSE(ui::stage::ParseStl(Binary(), options).has_value());
    EXPECT_FALSE(ui::stage::ParseStl(Bytes(asciiTetrahedron), options).has_value());
}

TEST_F(StlTest, AMalformedAsciiNumberIsRejected)
{
    EXPECT_FALSE(ui::stage::ParseStl(Bytes("solid x\nfacet normal 0 0 0\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1.2.3 0\n")).has_value());
}

TEST_F(StlTest, AnAsciiFileWithAPartialFacetIsRejected)
{
    EXPECT_FALSE(ui::stage::ParseStl(Bytes("solid x\nvertex 0 0 0\nvertex 1 0 0\n")).has_value());
}

TEST_F(StlTest, NeitherFormatIsRejected)
{
    EXPECT_FALSE(ui::stage::ParseStl(Bytes("not an stl at all")).has_value());
    EXPECT_FALSE(ui::stage::ParseStl(std::span<const std::byte>{}).has_value());
}

TEST_F(StlTest, AMissingFileIsNothing)
{
    EXPECT_FALSE(ui::stage::LoadStlFile("/nonexistent/definitely/missing.stl").has_value());
}

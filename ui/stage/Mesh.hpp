#pragma once

#include "ui/scene/Vector3.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ui::stage
{
    using scene::Vector3;

    // A triangle or a planar convex quad, wound counter-clockwise seen from outside. Bit i of
    // featureMask marks edge index[i] -> index[(i + 1) % count] as a hard edge: a crease sharper
    // than the build threshold, or a boundary. Outlines draw only those, so a cylinder shows its
    // rims and not every facet seam.
    struct Face
    {
        std::array<std::uint32_t, 4> index{};
        std::uint8_t count{ 3 };
        Vector3 normal;
        std::byte featureMask{ 0 };
    };

    struct Mesh
    {
        std::vector<Vector3> vertices;
        std::vector<Face> faces;
        Vector3 boundsMin;
        Vector3 boundsMax;

        std::uint32_t AddVertex(Vector3 position);
        void AddTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c);
        void AddQuad(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d);

        // Normals, bounds and feature edges, once all faces are in. Allocates; call at setup.
        void Finish(float featureAngleDegrees = 30.0f);

        [[nodiscard]] std::size_t CornerCount() const;
    };
}

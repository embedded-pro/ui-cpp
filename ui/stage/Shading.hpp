#pragma once

#include "ui/stage/Mesh.hpp"
#include <cstdint>
#include <vector>

namespace ui::stage
{
    struct ShadedVertex
    {
        Vector3 position;
        Vector3 normal;

        [[nodiscard]] constexpr bool operator==(const ShadedVertex& other) const = default;
    };

    // Triangles with a normal per corner, the form a GPU renderer takes: interleaved position and
    // normal, 32-bit indices.
    struct ShadedMesh
    {
        std::vector<ShadedVertex> vertices;
        std::vector<std::uint32_t> indices;
        Vector3 boundsMin;
        Vector3 boundsMax;

        [[nodiscard]] std::size_t Triangles() const;
        [[nodiscard]] bool operator==(const ShadedMesh& other) const = default;
    };

    // Smooths each corner over the faces round its vertex that lie within the crease angle of the
    // corner's own face, so a cylinder is round along its side and sharp at its rims. The mesh must be
    // finished. Allocates; call at setup.
    [[nodiscard]] ShadedMesh Shade(const Mesh& mesh, float creaseAngleDegrees = 35.0f);
}

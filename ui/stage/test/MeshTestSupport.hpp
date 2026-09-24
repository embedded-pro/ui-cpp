#pragma once

#include "ui/stage/Mesh.hpp"
#include <bit>
#include <cstddef>

namespace ui::stage::test
{
    [[nodiscard]] inline std::size_t FeatureEdgeUses(const Mesh& mesh)
    {
        std::size_t uses{ 0 };

        for (const auto& face : mesh.faces)
            uses += static_cast<std::size_t>(std::popcount(std::to_integer<unsigned>(face.featureMask)));

        return uses;
    }

    [[nodiscard]] inline Vector3 Centroid(const Mesh& mesh, const Face& face)
    {
        Vector3 sum;

        for (std::uint8_t i = 0; i < face.count; ++i)
            sum = sum + mesh.vertices[face.index[i]];

        return sum * (1.0f / static_cast<float>(face.count));
    }
}

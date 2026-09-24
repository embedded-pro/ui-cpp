#pragma once

#include "ui/stage/Mesh.hpp"
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace ui::stage
{
    struct StlOptions
    {
        // STL carries no units. CAD exports are usually millimetres; 0.001 brings them to metres.
        float scale{ 1.0f };

        // Merges coincident corners so neighbouring facets share vertices. Without it every edge
        // is a boundary and an outline draws every facet.
        bool weld{ true };
        float featureAngleDegrees{ 30.0f };

        std::size_t maximumTriangles{ 200000 };
    };

    // Binary when the size is exactly 84 + 50n for the count in the header, ASCII otherwise.
    // Facet normals in the file are ignored and recomputed from the winding, because exporters
    // routinely write zeros. Nothing on truncation, a malformed number, a non-finite coordinate,
    // or more triangles than the options allow.
    [[nodiscard]] std::optional<Mesh> ParseStl(std::span<const std::byte> data, const StlOptions& options = {});

    [[nodiscard]] std::optional<Mesh> LoadStlFile(std::string_view path, const StlOptions& options = {});
}

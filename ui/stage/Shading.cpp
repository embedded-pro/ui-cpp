#include "ui/stage/Shading.hpp"
#include <array>
#include <cmath>
#include <numbers>

namespace ui::stage
{
    namespace
    {
        struct Adjacency
        {
            std::vector<std::uint32_t> start;
            std::vector<std::uint32_t> faces;
        };

        [[nodiscard]] Adjacency FacesAroundVertices(const Mesh& mesh)
        {
            Adjacency adjacency;
            adjacency.start.assign(mesh.vertices.size() + 1, 0);
            for (const auto& face : mesh.faces)
                for (std::uint8_t i = 0; i < face.count; ++i)
                    ++adjacency.start[face.index[i] + 1];

            for (std::size_t vertex = 1; vertex < adjacency.start.size(); ++vertex)
                adjacency.start[vertex] += adjacency.start[vertex - 1];

            adjacency.faces.resize(adjacency.start.back());
            auto fill = adjacency.start;
            for (std::uint32_t f = 0; f < mesh.faces.size(); ++f)
            {
                const auto& face = mesh.faces[f];
                for (std::uint8_t i = 0; i < face.count; ++i)
                    adjacency.faces[fill[face.index[i]]++] = f;
            }
            return adjacency;
        }

        [[nodiscard]] Vector3 CornerNormal(const Mesh& mesh, const Adjacency& adjacency, std::uint32_t vertex, const Face& own, float creaseCosine)
        {
            Vector3 sum;
            for (auto at = adjacency.start[vertex]; at < adjacency.start[vertex + 1]; ++at)
            {
                const auto& other = mesh.faces[adjacency.faces[at]].normal;
                if (scene::Dot(own.normal, other) >= creaseCosine)
                    sum = sum + other;
            }
            return scene::Length(sum) > 0.0f ? scene::Normalized(sum) : own.normal;
        }

        class Builder
        {
        public:
            explicit Builder(std::size_t vertexCount)
                : emitted(vertexCount)
            {}

            std::uint32_t Emit(ShadedMesh& shaded, std::uint32_t vertex, ShadedVertex value)
            {
                for (const auto index : emitted[vertex])
                    if (shaded.vertices[index] == value)
                        return index;

                const auto index = static_cast<std::uint32_t>(shaded.vertices.size());
                shaded.vertices.push_back(value);
                emitted[vertex].push_back(index);
                return index;
            }

        private:
            std::vector<std::vector<std::uint32_t>> emitted;
        };
    }

    std::size_t ShadedMesh::Triangles() const
    {
        return indices.size() / 3;
    }

    ShadedMesh Shade(const Mesh& mesh, float creaseAngleDegrees)
    {
        ShadedMesh shaded;
        shaded.boundsMin = mesh.boundsMin;
        shaded.boundsMax = mesh.boundsMax;

        const auto creaseCosine = std::cos(creaseAngleDegrees * std::numbers::pi_v<float> / 180.0f);
        const auto adjacency = FacesAroundVertices(mesh);
        Builder builder{ mesh.vertices.size() };

        for (const auto& face : mesh.faces)
        {
            std::array<std::uint32_t, 4> corners{};
            for (std::uint8_t i = 0; i < face.count; ++i)
            {
                const auto vertex = face.index[i];
                corners[i] = builder.Emit(shaded, vertex, ShadedVertex{ mesh.vertices[vertex], CornerNormal(mesh, adjacency, vertex, face, creaseCosine) });
            }

            for (std::uint8_t i = 1; i + 1 < face.count; ++i)
                shaded.indices.insert(shaded.indices.end(), { corners[0], corners[i], corners[i + 1] });
        }
        return shaded;
    }
}

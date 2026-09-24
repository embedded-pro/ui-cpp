#include "ui/stage/Mesh.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace ui::stage
{
    namespace
    {
        struct EdgeUse
        {
            std::uint32_t low{ 0 };
            std::uint32_t high{ 0 };
            std::uint32_t face{ 0 };
            std::uint8_t edge{ 0 };
        };

        [[nodiscard]] bool SameEdge(const EdgeUse& a, const EdgeUse& b)
        {
            return a.low == b.low && a.high == b.high;
        }

        // Newell's method: robust for a slightly non-planar quad, and zero only for a degenerate face.
        [[nodiscard]] Vector3 NormalOf(const Mesh& mesh, const Face& face)
        {
            Vector3 sum;

            for (std::uint8_t i = 0; i < face.count; ++i)
            {
                const auto current = mesh.vertices[face.index[i]];
                const auto next = mesh.vertices[face.index[(i + 1) % face.count]];

                sum = sum + Vector3{ (current.y - next.y) * (current.z + next.z), (current.z - next.z) * (current.x + next.x),
                    (current.x - next.x) * (current.y + next.y) };
            }

            return scene::Normalized(sum);
        }

        [[nodiscard]] std::vector<EdgeUse> CollectEdges(const Mesh& mesh)
        {
            std::vector<EdgeUse> edges;
            edges.reserve(mesh.CornerCount());

            for (std::uint32_t f = 0; f < mesh.faces.size(); ++f)
            {
                const auto& face = mesh.faces[f];

                for (std::uint8_t i = 0; i < face.count; ++i)
                {
                    const auto a = face.index[i];
                    const auto b = face.index[(i + 1) % face.count];
                    edges.push_back(EdgeUse{ std::min(a, b), std::max(a, b), f, i });
                }
            }

            std::sort(edges.begin(), edges.end(), [](const EdgeUse& a, const EdgeUse& b)
                {
                    return a.low != b.low ? a.low < b.low : a.high < b.high;
                });

            return edges;
        }

        void MarkFeatureEdges(Mesh& mesh, float featureAngleDegrees)
        {
            const auto creaseCosine = std::cos(featureAngleDegrees * std::numbers::pi_v<float> / 180.0f);
            const auto edges = CollectEdges(mesh);

            for (std::size_t first = 0; first < edges.size();)
            {
                auto last = first + 1;

                while (last < edges.size() && SameEdge(edges[first], edges[last]))
                    ++last;

                const auto smooth = last - first == 2 &&
                                    scene::Dot(mesh.faces[edges[first].face].normal, mesh.faces[edges[first + 1].face].normal) >= creaseCosine;

                if (!smooth)
                    for (auto i = first; i < last; ++i)
                        mesh.faces[edges[i].face].featureMask |= static_cast<std::uint8_t>(1u << edges[i].edge);

                first = last;
            }
        }
    }

    std::uint32_t Mesh::AddVertex(Vector3 position)
    {
        vertices.push_back(position);

        return static_cast<std::uint32_t>(vertices.size() - 1);
    }

    void Mesh::AddTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c)
    {
        faces.push_back(Face{ { a, b, c, 0 }, 3, Vector3{}, 0 });
    }

    void Mesh::AddQuad(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d)
    {
        faces.push_back(Face{ { a, b, c, d }, 4, Vector3{}, 0 });
    }

    void Mesh::Finish(float featureAngleDegrees)
    {
        for (auto& face : faces)
        {
            face.normal = NormalOf(*this, face);
            face.featureMask = 0;
        }

        MarkFeatureEdges(*this, featureAngleDegrees);

        if (vertices.empty())
            return;

        boundsMin = vertices.front();
        boundsMax = vertices.front();

        for (const auto& vertex : vertices)
        {
            boundsMin = Vector3{ std::min(boundsMin.x, vertex.x), std::min(boundsMin.y, vertex.y), std::min(boundsMin.z, vertex.z) };
            boundsMax = Vector3{ std::max(boundsMax.x, vertex.x), std::max(boundsMax.y, vertex.y), std::max(boundsMax.z, vertex.z) };
        }
    }

    std::size_t Mesh::CornerCount() const
    {
        std::size_t corners{ 0 };

        for (const auto& face : faces)
            corners += face.count;

        return corners;
    }
}

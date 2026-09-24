#include "ui/stage/Primitives.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>

namespace ui::stage
{
    namespace
    {
        constexpr float pi{ std::numbers::pi_v<float> };

        // A radius of zero is a pole: one vertex rather than a ring. The first and last profile
        // points must be poles, so every lathed solid is closed.
        struct ProfilePoint
        {
            float radius{ 0.0f };
            float z{ 0.0f };
        };

        [[nodiscard]] std::uint32_t Segments(Tessellation tessellation)
        {
            return std::clamp(tessellation.segments, Tessellation::minimumSegments, Tessellation::maximumSegments);
        }

        [[nodiscard]] std::uint32_t Rings(Tessellation tessellation)
        {
            return std::clamp(tessellation.rings, Tessellation::minimumRings, Tessellation::maximumRings);
        }

        std::uint32_t AddRing(Mesh& mesh, ProfilePoint point, std::uint32_t segments)
        {
            const auto first = static_cast<std::uint32_t>(mesh.vertices.size());

            for (std::uint32_t i = 0; i < segments; ++i)
            {
                const auto angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
                mesh.AddVertex(Vector3{ point.radius * std::cos(angle), point.radius * std::sin(angle), point.z });
            }

            return first;
        }

        void AddBand(Mesh& mesh, std::uint32_t upper, std::uint32_t lower, std::uint32_t segments)
        {
            for (std::uint32_t i = 0; i < segments; ++i)
            {
                const auto next = (i + 1) % segments;
                mesh.AddQuad(upper + i, lower + i, lower + next, upper + next);
            }
        }

        [[nodiscard]] Mesh Lathe(std::span<const ProfilePoint> profile, std::uint32_t segments)
        {
            Mesh mesh;
            const auto top = mesh.AddVertex(Vector3{ 0.0f, 0.0f, profile.front().z });
            const auto firstRing = static_cast<std::uint32_t>(mesh.vertices.size());
            const auto ringCount = static_cast<std::uint32_t>(profile.size() - 2);

            for (std::uint32_t k = 0; k < ringCount; ++k)
                AddRing(mesh, profile[k + 1], segments);

            const auto bottom = mesh.AddVertex(Vector3{ 0.0f, 0.0f, profile.back().z });
            const auto lastRing = firstRing + (ringCount - 1) * segments;

            for (std::uint32_t i = 0; i < segments; ++i)
                mesh.AddTriangle(top, firstRing + i, firstRing + (i + 1) % segments);

            for (std::uint32_t k = 0; k + 1 < ringCount; ++k)
                AddBand(mesh, firstRing + k * segments, firstRing + (k + 1) * segments, segments);

            for (std::uint32_t i = 0; i < segments; ++i)
                mesh.AddTriangle(bottom, lastRing + (i + 1) % segments, lastRing + i);

            mesh.Finish();
            return mesh;
        }
    }

    Mesh MakeBox()
    {
        Mesh mesh;

        for (std::uint32_t corner = 0; corner < 8; ++corner)
            mesh.AddVertex(Vector3{ (corner & 1u) != 0 ? 0.5f : -0.5f, (corner & 2u) != 0 ? 0.5f : -0.5f,
                (corner & 4u) != 0 ? 0.5f : -0.5f });

        mesh.AddQuad(0, 2, 3, 1);
        mesh.AddQuad(4, 5, 7, 6);
        mesh.AddQuad(0, 1, 5, 4);
        mesh.AddQuad(2, 6, 7, 3);
        mesh.AddQuad(0, 4, 6, 2);
        mesh.AddQuad(1, 3, 7, 5);

        mesh.Finish();
        return mesh;
    }

    Mesh MakeCylinder(Tessellation tessellation)
    {
        const std::array<ProfilePoint, 4> profile{
            ProfilePoint{ 0.0f, 1.0f }, ProfilePoint{ 1.0f, 1.0f }, ProfilePoint{ 1.0f, 0.0f }, ProfilePoint{ 0.0f, 0.0f }
        };

        return Lathe(profile, Segments(tessellation));
    }

    Mesh MakeCone(Tessellation tessellation)
    {
        const std::array<ProfilePoint, 3> profile{ ProfilePoint{ 0.0f, 1.0f }, ProfilePoint{ 1.0f, 0.0f }, ProfilePoint{ 0.0f, 0.0f } };

        return Lathe(profile, Segments(tessellation));
    }

    Mesh MakeSphere(Tessellation tessellation)
    {
        const auto rings = Rings(tessellation);
        std::vector<ProfilePoint> profile;
        profile.reserve(rings + 1);

        for (std::uint32_t k = 0; k <= rings; ++k)
        {
            const auto polar = pi * static_cast<float>(k) / static_cast<float>(rings);
            profile.push_back(ProfilePoint{ k == 0 || k == rings ? 0.0f : std::sin(polar), std::cos(polar) });
        }

        return Lathe(profile, Segments(tessellation));
    }

    Mesh MakeCapsule(float radius, float length, Tessellation tessellation)
    {
        const auto half = std::max<std::uint32_t>(1, Rings(tessellation) / 2);
        std::vector<ProfilePoint> profile;
        profile.reserve(2 * half + 2);
        profile.push_back(ProfilePoint{ 0.0f, length + radius });

        for (std::uint32_t k = 1; k <= half; ++k)
        {
            const auto polar = 0.5f * pi * static_cast<float>(k) / static_cast<float>(half);
            profile.push_back(ProfilePoint{ radius * std::sin(polar), length + radius * std::cos(polar) });
        }

        for (std::uint32_t k = 0; k < half; ++k)
        {
            const auto polar = 0.5f * pi * static_cast<float>(k) / static_cast<float>(half);
            profile.push_back(ProfilePoint{ radius * std::cos(polar), -radius * std::sin(polar) });
        }

        profile.push_back(ProfilePoint{ 0.0f, -radius });

        return Lathe(profile, Segments(tessellation));
    }

    Mesh MakePlane()
    {
        Mesh mesh;
        mesh.AddVertex(Vector3{ -0.5f, -0.5f, 0.0f });
        mesh.AddVertex(Vector3{ 0.5f, -0.5f, 0.0f });
        mesh.AddVertex(Vector3{ 0.5f, 0.5f, 0.0f });
        mesh.AddVertex(Vector3{ -0.5f, 0.5f, 0.0f });
        mesh.AddQuad(0, 1, 2, 3);

        mesh.Finish();
        return mesh;
    }
}

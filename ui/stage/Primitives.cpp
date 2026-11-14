#include "ui/stage/Primitives.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <span>

namespace ui::stage
{
    namespace
    {
        constexpr float pi{ std::numbers::pi_v<float> };

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

    Mesh MakeLathe(std::span<const ProfilePoint> profile, Tessellation tessellation)
    {
        if (profile.size() < 3)
            return Mesh{};

        return Lathe(profile, Segments(tessellation));
    }

    Mesh MakeExtrusion(std::span<const Point2> outline, float bottom, float top)
    {
        Mesh mesh;
        const auto count = static_cast<std::uint32_t>(outline.size());
        if (count < 3)
            return mesh;

        Point2 centre;
        for (const auto& point : outline)
        {
            mesh.AddVertex(Vector3{ point.x, point.y, bottom });
            centre = Point2{ centre.x + point.x / static_cast<float>(count), centre.y + point.y / static_cast<float>(count) };
        }
        for (const auto& point : outline)
            mesh.AddVertex(Vector3{ point.x, point.y, top });

        const auto lowerCentre = mesh.AddVertex(Vector3{ centre.x, centre.y, bottom });
        const auto upperCentre = mesh.AddVertex(Vector3{ centre.x, centre.y, top });

        for (std::uint32_t i = 0; i < count; ++i)
        {
            const auto next = (i + 1) % count;
            mesh.AddQuad(i, next, count + next, count + i);
            mesh.AddTriangle(upperCentre, count + i, count + next);
            mesh.AddTriangle(lowerCentre, next, i);
        }

        mesh.Finish();
        return mesh;
    }

    Outline RoundedRectangle(float width, float height, float radius, std::uint32_t cornerSegments)
    {
        const auto r = std::clamp(radius, 0.0f, std::min(width, height) / 2.0f);
        const auto x = width / 2.0f - r;
        const auto y = height / 2.0f - r;
        const auto segments = r > 0.0f ? std::max<std::uint32_t>(cornerSegments, 1) : 0;
        const std::array<std::array<float, 3>, 4> corners{ { { x, -y, -pi / 2.0f }, { x, y, 0.0f }, { -x, y, pi / 2.0f }, { -x, -y, pi } } };

        Outline outline;
        outline.reserve(4 * (segments + 1));
        for (const auto& [cx, cy, start] : corners)
            for (std::uint32_t step = 0; step <= segments; ++step)
            {
                const auto angle = start + (pi / 2.0f) * (segments == 0 ? 0.0f : static_cast<float>(step) / static_cast<float>(segments));
                outline.push_back(Point2{ cx + r * std::cos(angle), cy + r * std::sin(angle) });
            }
        return outline;
    }

    Outline Stadium(float length, float width, std::uint32_t endSegments)
    {
        const auto r = width / 2.0f;
        const auto segments = std::max<std::uint32_t>(endSegments, 1);

        Outline outline;
        outline.reserve(2 * (segments + 1));
        for (std::uint32_t step = 0; step <= segments; ++step)
        {
            const auto angle = pi + pi * static_cast<float>(step) / static_cast<float>(segments);
            outline.push_back(Point2{ r * std::cos(angle), r * std::sin(angle) });
        }
        for (std::uint32_t step = 0; step <= segments; ++step)
        {
            const auto angle = pi * static_cast<float>(step) / static_cast<float>(segments);
            outline.push_back(Point2{ r * std::cos(angle), length + r * std::sin(angle) });
        }
        return outline;
    }

    Outline RegularPolygon(std::uint32_t sides, float radius)
    {
        Outline outline;
        outline.reserve(sides);
        for (std::uint32_t side = 0; side < sides; ++side)
        {
            const auto angle = 2.0f * pi * static_cast<float>(side) / static_cast<float>(sides);
            outline.push_back(Point2{ radius * std::cos(angle), radius * std::sin(angle) });
        }
        return outline;
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

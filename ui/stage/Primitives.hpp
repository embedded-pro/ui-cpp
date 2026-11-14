#pragma once

#include "ui/stage/Mesh.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace ui::stage
{
    // Segments go around the axis, rings from pole to pole. Both are clamped: below the minimum
    // a cylinder stops being one, and above the maximum the painter's sort drowns in faces.
    struct Tessellation
    {
        std::uint8_t segments{ 20 };
        std::uint8_t rings{ 10 };

        static constexpr std::uint8_t minimumSegments{ 4 };
        static constexpr std::uint8_t maximumSegments{ 64 };
        static constexpr std::uint8_t minimumRings{ 2 };
        static constexpr std::uint8_t maximumRings{ 64 };
    };

    // Unit-sized so one mesh serves every instance; a part scales it. The box is centred on its
    // origin. The cylinder and cone stand on the XY plane and reach z = 1, so an unscaled part
    // starts exactly at its joint.
    [[nodiscard]] Mesh MakeBox();
    [[nodiscard]] Mesh MakeCylinder(Tessellation tessellation = {});
    [[nodiscard]] Mesh MakeCone(Tessellation tessellation = {});
    [[nodiscard]] Mesh MakeSphere(Tessellation tessellation = {});
    [[nodiscard]] Mesh MakePlane();

    // Not unit-sized: scaling a capsule non-uniformly would squash its hemispherical ends. The
    // hemisphere centres sit at z = 0 and z = length.
    [[nodiscard]] Mesh MakeCapsule(float radius, float length, Tessellation tessellation = {});

    struct ProfilePoint
    {
        float radius{ 0.0f };
        float z{ 0.0f };
    };

    // Turns a profile about Z, from the top pole down to the bottom pole: the first and last points
    // must have a radius of zero and no other may, so the solid is closed. Only the segment count of
    // the tessellation is used; the profile sets the rings.
    [[nodiscard]] Mesh MakeLathe(std::span<const ProfilePoint> profile, Tessellation tessellation = {});

    struct Point2
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    using Outline = std::vector<Point2>;

    // Sweeps a convex outline, counter-clockwise in XY, along Z from `bottom` to `top`, with both ends
    // capped. Rounded corners need enough points to stay under the feature angle, or they outline.
    [[nodiscard]] Mesh MakeExtrusion(std::span<const Point2> outline, float bottom, float top);

    [[nodiscard]] Outline RoundedRectangle(float width, float height, float radius, std::uint32_t cornerSegments);
    // A slot from the origin to (0, length): semicircular ends of diameter `width`.
    [[nodiscard]] Outline Stadium(float length, float width, std::uint32_t endSegments);
    [[nodiscard]] Outline RegularPolygon(std::uint32_t sides, float radius);
}

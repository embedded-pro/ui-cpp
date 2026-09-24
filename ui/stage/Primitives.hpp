#pragma once

#include "ui/stage/Mesh.hpp"
#include <cstdint>

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
}

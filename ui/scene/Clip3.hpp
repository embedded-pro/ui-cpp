#pragma once

#include "ui/scene/Vector3.hpp"
#include <cstddef>
#include <span>

namespace ui::scene
{
    // View-space clipping against z >= nearDistance, where z is depth along the camera's forward
    // axis (ViewFrame::ToView). Clipping a convex polygon adds at most one vertex, so `out` needs
    // in.size() + 1 entries; a shorter span truncates rather than overruns. Returns the count.
    [[nodiscard]] std::size_t ClipPolygonNear(std::span<const Vector3> in, float nearDistance, std::span<Vector3> out);

    // Moves whichever end is behind the plane onto it. False when the whole segment is behind.
    [[nodiscard]] bool ClipSegmentNear(Vector3& from, Vector3& to, float nearDistance);
}

#include "ui/scene/Clip3.hpp"

namespace ui::scene
{
    namespace
    {
        [[nodiscard]] Vector3 CrossingPoint(Vector3 from, Vector3 to, float nearDistance)
        {
            const auto t = (nearDistance - from.z) / (to.z - from.z);

            return Vector3{ from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t, nearDistance };
        }
    }

    std::size_t ClipPolygonNear(std::span<const Vector3> in, float nearDistance, std::span<Vector3> out)
    {
        std::size_t count{ 0 };

        const auto emit = [&](Vector3 vertex)
        {
            if (count < out.size())
                out[count++] = vertex;
        };

        for (std::size_t i = 0; i < in.size(); ++i)
        {
            const auto current = in[i];
            const auto next = in[(i + 1) % in.size()];
            const auto currentInside = current.z >= nearDistance;
            const auto nextInside = next.z >= nearDistance;

            if (currentInside)
                emit(current);

            if (currentInside != nextInside)
                emit(CrossingPoint(current, next, nearDistance));
        }

        return count;
    }

    bool ClipSegmentNear(Vector3& from, Vector3& to, float nearDistance)
    {
        const auto fromInside = from.z >= nearDistance;
        const auto toInside = to.z >= nearDistance;

        if (!fromInside && !toInside)
            return false;

        if (!fromInside)
            from = CrossingPoint(from, to, nearDistance);
        else if (!toInside)
            to = CrossingPoint(from, to, nearDistance);

        return true;
    }
}

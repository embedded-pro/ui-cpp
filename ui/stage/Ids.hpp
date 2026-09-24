#pragma once

#include <cstdint>
#include <limits>

namespace ui::stage
{
    // A strong index rather than a pointer: the stage stores its parts in vectors that grow at
    // setup time, and a handle survives that growth where a pointer would dangle.
    template<class Tag>
    struct Id
    {
        static constexpr std::uint16_t invalid{ std::numeric_limits<std::uint16_t>::max() };

        std::uint16_t value{ invalid };

        [[nodiscard]] constexpr bool Valid() const
        {
            return value != invalid;
        }

        [[nodiscard]] constexpr bool operator==(const Id& other) const = default;
    };

    using NodeId = Id<struct NodeTag>;
    using PartId = Id<struct PartTag>;
    using MeshId = Id<struct MeshTag>;
    using MaterialId = Id<struct MaterialTag>;
    using LabelId = Id<struct LabelTag>;
    using TrailId = Id<struct TrailTag>;
}

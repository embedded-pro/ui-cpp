#include "ui/stage/Stage.hpp"
#include <algorithm>
#include <cstring>
#include <iterator>

namespace ui::stage
{
    namespace
    {
        template<class Handle, class Container>
        [[nodiscard]] bool Knows(const Container& container, Handle id)
        {
            return id.Valid() && id.value < std::size(container);
        }

        template<class Handle, class Container>
        [[nodiscard]] Handle NextId(const Container& container)
        {
            if (std::size(container) >= Handle::invalid)
                return Handle{};

            return Handle{ static_cast<std::uint16_t>(std::size(container)) };
        }

        template<class Container, class Handle>
        [[nodiscard]] auto* Find(Container& container, Handle id)
        {
            return Knows(container, id) ? &container[id.value] : nullptr;
        }

        [[nodiscard]] Vector3 Minimum(Vector3 a, Vector3 b)
        {
            return Vector3{ std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
        }

        [[nodiscard]] Vector3 Maximum(Vector3 a, Vector3 b)
        {
            return Vector3{ std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
        }

        [[nodiscard]] Vector3 Scaled(Vector3 value, Vector3 scale)
        {
            return Vector3{ value.x * scale.x, value.y * scale.y, value.z * scale.z };
        }
    }

    Stage::Stage(Tessellation tessellation)
        : tessellation(tessellation)
    {}

    SceneGraph& Stage::Graph()
    {
        return graph;
    }

    const SceneGraph& Stage::Graph() const
    {
        return graph;
    }

    Lighting& Stage::Lights()
    {
        return lighting;
    }

    const Lighting& Stage::Lights() const
    {
        return lighting;
    }

    MeshId Stage::AddMesh(Mesh mesh)
    {
        const auto id = NextId<MeshId>(meshes);

        if (id.Valid())
            meshes.push_back(std::move(mesh));

        return id;
    }

    template<class Make>
    MeshId Stage::SharedMesh(std::optional<MeshId>& cache, Make make)
    {
        if (!cache)
            cache = AddMesh(make(tessellation));

        return *cache;
    }

    MeshId Stage::BoxMesh()
    {
        return SharedMesh(box, [](Tessellation)
            {
                return MakeBox();
            });
    }

    MeshId Stage::CylinderMesh()
    {
        return SharedMesh(cylinder, &MakeCylinder);
    }

    MeshId Stage::ConeMesh()
    {
        return SharedMesh(cone, &MakeCone);
    }

    MeshId Stage::SphereMesh()
    {
        return SharedMesh(sphere, &MakeSphere);
    }

    MeshId Stage::PlaneMesh()
    {
        return SharedMesh(plane, [](Tessellation)
            {
                return MakePlane();
            });
    }

    MaterialId Stage::AddMaterial(const Material& material)
    {
        const auto id = NextId<MaterialId>(materials);

        if (id.Valid())
            materials.push_back(material);

        return id;
    }

    PartId Stage::AddPart(const Part& part)
    {
        const auto id = NextId<PartId>(parts);

        if (!id.Valid() || !graph.Contains(part.node) || !Knows(meshes, part.mesh) || !Knows(materials, part.material))
            return PartId{};

        parts.push_back(part);
        return id;
    }

    LabelId Stage::AddLabel(NodeId node, Vector3 offset, std::string_view text, const LabelStyle& style)
    {
        const auto id = NextId<LabelId>(labels);

        if (!id.Valid() || !graph.Contains(node))
            return LabelId{};

        labels.push_back(Label{ node, offset, {}, 0, style, true });
        SetLabelText(id, text);
        return id;
    }

    bool Stage::SetLabelText(LabelId label, std::string_view text)
    {
        auto* target = FindLabel(label);

        if (target == nullptr)
            return false;

        const auto length = std::min(text.size(), Label::capacity);
        std::memcpy(target->text.data(), text.data(), length);
        target->length = static_cast<std::uint8_t>(length);
        return true;
    }

    TrailId Stage::AddTrail(std::size_t capacity, const TrailStyle& style, NodeId frame)
    {
        const auto id = NextId<TrailId>(trails);

        if (!id.Valid() || (frame.Valid() && !graph.Contains(frame)))
            return TrailId{};

        trails.emplace_back(capacity, style, frame);
        return id;
    }

    void Stage::AddFrameMarker(NodeId node, float length, float width)
    {
        if (graph.Contains(node))
            frameMarkers.push_back(FrameMarker{ node, length, width, true });
    }

    Material* Stage::FindMaterial(MaterialId material)
    {
        return Find(materials, material);
    }

    Part* Stage::FindPart(PartId part)
    {
        return Find(parts, part);
    }

    Label* Stage::FindLabel(LabelId label)
    {
        return Find(labels, label);
    }

    Trail* Stage::FindTrail(TrailId trail)
    {
        return Find(trails, trail);
    }

    std::span<const Mesh> Stage::Meshes() const
    {
        return meshes;
    }

    std::span<const Material> Stage::Materials() const
    {
        return materials;
    }

    std::span<const Part> Stage::Parts() const
    {
        return parts;
    }

    std::span<const Label> Stage::Labels() const
    {
        return labels;
    }

    std::span<const Trail> Stage::Trails() const
    {
        return trails;
    }

    std::span<const FrameMarker> Stage::FrameMarkers() const
    {
        return frameMarkers;
    }

    StageCapacity Stage::Capacity() const
    {
        StageCapacity capacity;

        for (const auto& part : parts)
        {
            capacity.vertices += meshes[part.mesh.value].vertices.size();
            capacity.faces += meshes[part.mesh.value].faces.size();
        }

        for (const auto& trail : trails)
            capacity.trailPoints += trail.Capacity();

        return capacity;
    }

    std::optional<BoundingSphere> Stage::Bounds() const
    {
        std::optional<std::pair<Vector3, Vector3>> extent;

        for (const auto& part : parts)
        {
            const auto& mesh = meshes[part.mesh.value];

            if (!part.visible || mesh.vertices.empty())
                continue;

            const auto transform = graph.World(part.node) * part.offset;

            for (std::uint32_t corner = 0; corner < 8; ++corner)
            {
                const Vector3 local{ (corner & 1u) != 0 ? mesh.boundsMax.x : mesh.boundsMin.x,
                    (corner & 2u) != 0 ? mesh.boundsMax.y : mesh.boundsMin.y, (corner & 4u) != 0 ? mesh.boundsMax.z : mesh.boundsMin.z };
                const auto world = transform.Apply(Scaled(local, part.scale));

                extent = extent ? std::pair{ Minimum(extent->first, world), Maximum(extent->second, world) } : std::pair{ world, world };
            }
        }

        if (!extent)
            return std::nullopt;

        return BoundingSphere{ (extent->first + extent->second) * 0.5f, scene::Length(extent->second - extent->first) * 0.5f };
    }
}

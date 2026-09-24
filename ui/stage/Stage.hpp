#pragma once

#include "ui/core/Geometry.hpp"
#include "ui/stage/Material.hpp"
#include "ui/stage/Mesh.hpp"
#include "ui/stage/Primitives.hpp"
#include "ui/stage/SceneGraph.hpp"
#include "ui/stage/Trail.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ui::stage
{
    // A mesh drawn in a node's frame: world(node) * offset * scale. Scale is per axis, so one
    // unit cylinder serves every link.
    struct Part
    {
        NodeId node;
        MeshId mesh;
        MaterialId material;
        Transform3 offset;
        Vector3 scale{ 1.0f, 1.0f, 1.0f };
        bool visible{ true };
        bool pickable{ true };
    };

    struct LabelStyle
    {
        std::optional<theme::ColorRole> role{ theme::ColorRole::SceneLabel };
        Color color{ colors::black };
        theme::FontRole font{ theme::FontRole::Small };
        Point pixelOffset{ 6.0f, -6.0f };
    };

    // Fixed-size text so relabelling at run time (a joint angle, say) never allocates.
    struct Label
    {
        static constexpr std::size_t capacity{ 32 };

        NodeId node;
        Vector3 offset;
        std::array<char, capacity> text{};
        std::uint8_t length{ 0 };
        LabelStyle style;
        bool visible{ true };

        [[nodiscard]] std::string_view Text() const
        {
            return std::string_view{ text.data(), length };
        }
    };

    // An RGB axis triad drawn at a node: a joint frame, a tool centre point.
    struct FrameMarker
    {
        NodeId node;
        float length{ 0.1f };
        float width{ 2.0f };
        bool visible{ true };
    };

    struct BoundingSphere
    {
        Vector3 centre;
        float radius{ 0.0f };
    };

    // What a renderer must hold to draw the stage without allocating.
    struct StageCapacity
    {
        std::size_t vertices{ 0 };
        std::size_t faces{ 0 };
        std::size_t trailPoints{ 0 };
    };

    // The model behind a StageView: the node tree, shared meshes, materials, and what hangs off
    // the nodes. Adding things allocates and belongs to setup; moving joints, pushing trail points,
    // recolouring and relabelling do not.
    class Stage
    {
    public:
        explicit Stage(Tessellation tessellation = {});

        [[nodiscard]] SceneGraph& Graph();
        [[nodiscard]] const SceneGraph& Graph() const;
        [[nodiscard]] Lighting& Lights();
        [[nodiscard]] const Lighting& Lights() const;

        MeshId AddMesh(Mesh mesh);
        [[nodiscard]] MeshId BoxMesh();
        [[nodiscard]] MeshId CylinderMesh();
        [[nodiscard]] MeshId ConeMesh();
        [[nodiscard]] MeshId SphereMesh();
        [[nodiscard]] MeshId PlaneMesh();

        MaterialId AddMaterial(const Material& material);

        // Invalid when the node, mesh or material is unknown.
        PartId AddPart(const Part& part);

        LabelId AddLabel(NodeId node, Vector3 offset, std::string_view text, const LabelStyle& style = {});

        // Truncates to Label::capacity bytes.
        bool SetLabelText(LabelId label, std::string_view text);

        TrailId AddTrail(std::size_t capacity, const TrailStyle& style = {}, NodeId frame = {});
        void AddFrameMarker(NodeId node, float length = 0.1f, float width = 2.0f);

        // Null for an unknown id. Valid until the next Add of the same kind.
        [[nodiscard]] Material* FindMaterial(MaterialId material);
        [[nodiscard]] Part* FindPart(PartId part);
        [[nodiscard]] Label* FindLabel(LabelId label);
        [[nodiscard]] Trail* FindTrail(TrailId trail);

        [[nodiscard]] std::span<const Mesh> Meshes() const;
        [[nodiscard]] std::span<const Material> Materials() const;
        [[nodiscard]] std::span<const Part> Parts() const;
        [[nodiscard]] std::span<const Label> Labels() const;
        [[nodiscard]] std::span<const Trail> Trails() const;
        [[nodiscard]] std::span<const FrameMarker> FrameMarkers() const;

        [[nodiscard]] StageCapacity Capacity() const;

        // Around every visible part at the current joint values; nothing for an empty stage.
        [[nodiscard]] std::optional<BoundingSphere> Bounds() const;

    private:
        MeshId SharedMesh(std::optional<MeshId>& cache, Mesh (*make)(Tessellation));

        Tessellation tessellation;
        SceneGraph graph;
        Lighting lighting;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<Part> parts;
        std::vector<Label> labels;
        std::vector<Trail> trails;
        std::vector<FrameMarker> frameMarkers;

        std::optional<MeshId> box;
        std::optional<MeshId> cylinder;
        std::optional<MeshId> cone;
        std::optional<MeshId> sphere;
        std::optional<MeshId> plane;
    };
}

#pragma once

#include "ui/core/Canvas.hpp"
#include "ui/scene/Camera3D.hpp"
#include "ui/scene/SceneGizmos.hpp"
#include "ui/stage/Stage.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ui::stage
{
    struct RenderOptions
    {
        bool showGrid{ true };
        scene::GroundGridConfig grid;
        bool showOriginTriad{ true };
        scene::AxisTriadConfig originTriad;
        bool showFrameMarkers{ true };
        bool showLabels{ true };

        // Parts on this node are tinted with the SceneSelection role.
        NodeId selection;
        float selectionTint{ 0.45f };
    };

    struct PickResult
    {
        NodeId node;
        PartId part;

        [[nodiscard]] constexpr bool operator==(const PickResult& other) const = default;
    };

    // Painter's algorithm over the 2D Canvas: every visible face is transformed, culled, clipped
    // at the near plane, flat-shaded, sorted far to near, and filled with one DrawPolygon. No
    // depth buffer, so parts that interpenetrate can sort wrongly; see doc/scene3d.md.
    //
    // Scratch buffers grow only when the stage has grown since the last render, so a stage that
    // is fully built before its first paint never allocates while painting.
    class StageRenderer
    {
    public:
        void Render(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame, const RenderOptions& options = {});

        // Against what the last Render drew, nearest first; nothing over the background.
        [[nodiscard]] std::optional<PickResult> Pick(Point point) const;

        [[nodiscard]] std::size_t DrawnFaces() const;

    private:
        enum class ItemKind : std::uint8_t
        {
            Face,
            Trail
        };

        struct DrawItem
        {
            ItemKind kind{ ItemKind::Face };
            std::uint8_t count{ 0 };
            std::uint8_t featureMask{ 0 };
            std::array<Point, 5> points{};
            Color fill;
            MaterialId material;
            PartId part;
            NodeId node;
            TrailId trail;
            bool pickable{ false };
            float depth{ 0.0f };
            std::uint32_t firstPoint{ 0 };
            std::uint32_t pointCount{ 0 };
        };

        struct SortKey
        {
            float depth{ 0.0f };
            std::uint32_t item{ 0 };
        };

        struct TrailRun
        {
            std::uint32_t start{ 0 };
            float depthSum{ 0.0f };
        };

        struct ShadingContext;
        struct PartContext;

        void Reserve(const Stage& stage);
        void Prepare(const Stage& stage, const scene::ViewFrame& frame, const RenderOptions& options);
        void CollectPart(const Stage& stage, PartId id, const ShadingContext& context);
        void CollectFace(const Face& face, const PartContext& part, const ShadingContext& context);
        void CollectTrail(const Stage& stage, TrailId id, const scene::ViewFrame& frame);
        void PushTrailPoint(const scene::ViewFrame& frame, Vector3 view, TrailRun& run);
        void CloseTrailRun(TrailId id, TrailRun& run, bool onTop);
        void Emit(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame, const RenderOptions& options);
        void EmitFace(Canvas& canvas, const Stage& stage, const DrawItem& item);
        void EmitTrail(Canvas& canvas, const Stage& stage, const DrawItem& item);
        void EmitFrameMarkers(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame);
        void EmitLabels(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame);
        void ApplyPen(Canvas& canvas, const Pen& pen);
        void ApplyBrush(Canvas& canvas, const Brush& brush);

        std::vector<Vector3> viewVertices;
        std::vector<Point> trailPoints;
        std::vector<DrawItem> items;
        std::vector<SortKey> order;
        StageCapacity reserved;
        std::size_t drawnFaces{ 0 };

        std::optional<Pen> currentPen;
        std::optional<Brush> currentBrush;
    };
}

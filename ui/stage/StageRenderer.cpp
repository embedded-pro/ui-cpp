#include "ui/stage/StageRenderer.hpp"
#include "ui/scene/Clip3.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ui::stage
{
    namespace
    {
        constexpr std::uint32_t trailChunkSegments{ 16 };
        constexpr float inverseScaleFloor{ 1e-6f };

        [[nodiscard]] Vector3 ToViewDirection(const scene::ViewFrame& frame, Vector3 direction)
        {
            return Vector3{ scene::Dot(frame.Right(), direction), scene::Dot(frame.Up(), direction), scene::Dot(frame.Forward(), direction) };
        }

        [[nodiscard]] float SafeInverse(float value)
        {
            const auto magnitude = std::max(std::abs(value), inverseScaleFloor);

            return value < 0.0f ? -1.0f / magnitude : 1.0f / magnitude;
        }

        [[nodiscard]] Vector3 Scaled(Vector3 value, Vector3 scale)
        {
            return Vector3{ value.x * scale.x, value.y * scale.y, value.z * scale.z };
        }

        [[nodiscard]] Color Resolve(const std::optional<theme::ColorRole>& role, Color fallback)
        {
            return role ? theme::Current().Get(*role) : fallback;
        }

        [[nodiscard]] Color Mix(Color from, Color to, float ratio)
        {
            const auto channel = [ratio](std::uint8_t a, std::uint8_t b)
            {
                return static_cast<std::uint8_t>(std::lround(Lerp(a, b, ratio)));
            };

            return Color{ channel(from.red, to.red), channel(from.green, to.green), channel(from.blue, to.blue), from.alpha };
        }

        [[nodiscard]] Color Shade(Color base, const Material& material, float lit, float highlight)
        {
            const auto intensity = material.ambient + material.diffuse * std::min(lit, 1.0f);
            const auto specular = 255.0f * material.specular * highlight;
            const auto channel = [intensity, specular](std::uint8_t value)
            {
                return static_cast<std::uint8_t>(std::lround(Clamp(static_cast<float>(value) * intensity + specular, 0.0f, 255.0f)));
            };

            return Color{ channel(base.red), channel(base.green), channel(base.blue), material.opacity };
        }

        [[nodiscard]] Pen EdgePen(const Material& material)
        {
            return Pen{ Resolve(material.outlineRole, material.outlineColor), material.outlineWidth };
        }

        [[nodiscard]] bool Contains(std::span<const Point> polygon, Point point)
        {
            auto inside = false;

            for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
            {
                const auto& a = polygon[i];
                const auto& b = polygon[j];

                if ((a.y > point.y) != (b.y > point.y) && point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)
                    inside = !inside;
            }

            return inside;
        }
    }

    struct StageRenderer::ShadingContext
    {
        const scene::ViewFrame& frame;
        Vector3 towardsKey;
        bool headlight{ false };
        float fill{ 0.0f };
        NodeId selection;
        float selectionTint{ 0.0f };
    };

    struct StageRenderer::PartContext
    {
        const Part& part;
        PartId id;
        const Material& material;
        Color base;
        std::uint32_t firstVertex{ 0 };
        std::array<Vector3, 3> normalColumns{};
        bool keepBackFaces{ false };
    };

    void StageRenderer::Render(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame, const RenderOptions& options)
    {
        Reserve(stage);
        Prepare(stage, frame, options);
        Emit(canvas, stage, frame, options);
    }

    std::optional<PickResult> StageRenderer::Pick(Point point) const
    {
        for (auto key = order.rbegin(); key != order.rend(); ++key)
        {
            const auto& item = items[key->item];

            if (item.kind == ItemKind::Face && item.pickable && Contains(std::span{ item.points.data(), item.count }, point))
                return PickResult{ item.node, item.part };
        }

        return std::nullopt;
    }

    std::size_t StageRenderer::DrawnFaces() const
    {
        return drawnFaces;
    }

    void StageRenderer::Reserve(const Stage& stage)
    {
        const auto needed = stage.Capacity();

        if (needed.vertices <= reserved.vertices && needed.faces <= reserved.faces && needed.trailPoints <= reserved.trailPoints)
            return;

        viewVertices.reserve(needed.vertices);
        trailPoints.reserve(3 * needed.trailPoints + 2);
        items.reserve(needed.faces + needed.trailPoints + 1);
        order.reserve(needed.faces + needed.trailPoints + 1);
        reserved = needed;
    }

    void StageRenderer::Prepare(const Stage& stage, const scene::ViewFrame& frame, const RenderOptions& options)
    {
        viewVertices.clear();
        trailPoints.clear();
        items.clear();
        order.clear();

        const auto& lights = stage.Lights();
        const ShadingContext context{ frame, scene::Normalized(ToViewDirection(frame, lights.towardsKey)), lights.headlight, lights.fill,
            options.selection, options.selectionTint };

        for (std::uint16_t i = 0; i < stage.Parts().size(); ++i)
            CollectPart(stage, PartId{ i }, context);

        drawnFaces = items.size();

        for (std::uint16_t i = 0; i < stage.Trails().size(); ++i)
            CollectTrail(stage, TrailId{ i }, frame);

        for (std::uint32_t i = 0; i < items.size(); ++i)
            order.push_back(SortKey{ items[i].depth, i });

        std::sort(order.begin(), order.end(), [](const SortKey& a, const SortKey& b)
            {
                return a.depth != b.depth ? a.depth > b.depth : a.item < b.item;
            });
    }

    void StageRenderer::CollectPart(const Stage& stage, PartId id, const ShadingContext& context)
    {
        const auto& part = stage.Parts()[id.value];

        if (!part.visible)
            return;

        const auto& mesh = stage.Meshes()[part.mesh.value];
        const auto& material = stage.Materials()[part.material.value];
        const auto transform = stage.Graph().World(part.node) * part.offset;
        const auto firstVertex = static_cast<std::uint32_t>(viewVertices.size());

        for (const auto& vertex : mesh.vertices)
            viewVertices.push_back(context.frame.ToView(transform.Apply(Scaled(vertex, part.scale))));

        auto base = Resolve(material.role, material.color);

        if (context.selection.Valid() && part.node == context.selection)
            base = Mix(base, theme::Current().Get(theme::ColorRole::SceneSelection), context.selectionTint);

        const Vector3 inverseScale{ SafeInverse(part.scale.x), SafeInverse(part.scale.y), SafeInverse(part.scale.z) };
        const PartContext partContext{ part, id, material, base, firstVertex,
            { ToViewDirection(context.frame, transform.rotation.columns[0] * inverseScale.x),
                ToViewDirection(context.frame, transform.rotation.columns[1] * inverseScale.y),
                ToViewDirection(context.frame, transform.rotation.columns[2] * inverseScale.z) },
            material.doubleSided || material.opacity < 255 };

        for (const auto& face : mesh.faces)
            CollectFace(face, partContext, context);
    }

    void StageRenderer::CollectFace(const Face& face, const PartContext& part, const ShadingContext& context)
    {
        std::array<Vector3, 4> corners{};
        Vector3 centroid;

        for (std::uint8_t i = 0; i < face.count; ++i)
        {
            corners[i] = viewVertices[part.firstVertex + face.index[i]];
            centroid = centroid + corners[i];
        }

        centroid = centroid * (1.0f / static_cast<float>(face.count));

        auto normal = scene::Normalized(part.normalColumns[0] * face.normal.x + part.normalColumns[1] * face.normal.y +
                                        part.normalColumns[2] * face.normal.z);
        const auto toEye = scene::Normalized(centroid * -1.0f);

        if (scene::Dot(normal, toEye) <= 0.0f)
        {
            if (!part.keepBackFaces)
                return;

            normal = normal * -1.0f;
        }

        std::array<Vector3, 5> clipped{};
        const auto count = scene::ClipPolygonNear(std::span{ corners.data(), face.count }, context.frame.NearDistance(), clipped);

        if (count < 3)
            return;

        const auto towardsLight = context.headlight ? toEye : context.towardsKey;
        const auto lit = std::max(scene::Dot(normal, towardsLight), 0.0f) + context.fill * std::max(scene::Dot(normal, toEye), 0.0f);
        const auto halfway = scene::Normalized(towardsLight + toEye);
        const auto highlight = part.material.specular > 0.0f ? std::pow(std::max(scene::Dot(normal, halfway), 0.0f), part.material.shininess) : 0.0f;

        DrawItem item;
        item.kind = ItemKind::Face;
        item.count = static_cast<std::uint8_t>(count);
        item.featureMask = count == face.count ? face.featureMask : std::uint8_t{ 0 };
        item.fill = Shade(part.base, part.material, lit, highlight);
        item.material = part.part.material;
        item.part = part.id;
        item.node = part.part.node;
        item.pickable = part.part.pickable;
        item.depth = centroid.z;

        for (std::size_t i = 0; i < count; ++i)
            item.points[i] = context.frame.ProjectView(clipped[i]);

        items.push_back(item);
    }

    void StageRenderer::CollectTrail(const Stage& stage, TrailId id, const scene::ViewFrame& frame)
    {
        const auto& trail = stage.Trails()[id.value];
        const auto& transform = stage.Graph().World(trail.Frame());
        const auto near = frame.NearDistance();
        TrailRun run{ static_cast<std::uint32_t>(trailPoints.size()) };
        Vector3 previous;

        for (std::size_t i = 0; i < trail.Size(); ++i)
        {
            const auto current = frame.ToView(transform.Apply(trail.At(i)));
            auto from = previous;
            auto to = current;
            previous = current;

            if (i == 0)
                continue;

            if (!scene::ClipSegmentNear(from, to, near))
            {
                CloseTrailRun(id, run, trail.Style().drawOnTop);
                continue;
            }

            if (trailPoints.size() == run.start)
                PushTrailPoint(frame, from, run);

            PushTrailPoint(frame, to, run);

            // A clipped end means the next visible stretch starts elsewhere; a full chunk just
            // restarts at the same point so the chunks join up.
            if (current.z < near)
                CloseTrailRun(id, run, trail.Style().drawOnTop);
            else if (trailPoints.size() - run.start > trailChunkSegments)
            {
                CloseTrailRun(id, run, trail.Style().drawOnTop);
                PushTrailPoint(frame, to, run);
            }
        }

        CloseTrailRun(id, run, trail.Style().drawOnTop);
    }

    void StageRenderer::PushTrailPoint(const scene::ViewFrame& frame, Vector3 view, TrailRun& run)
    {
        trailPoints.push_back(frame.ProjectView(view));
        run.depthSum += view.z;
    }

    // Each chunk sorts on its own mean depth, so a long trail weaves in front of and behind the
    // solids rather than sitting entirely on one side of them.
    void StageRenderer::CloseTrailRun(TrailId id, TrailRun& run, bool onTop)
    {
        const auto count = static_cast<std::uint32_t>(trailPoints.size()) - run.start;

        if (count >= 2)
        {
            DrawItem item;
            item.kind = ItemKind::Trail;
            item.trail = id;
            item.firstPoint = run.start;
            item.pointCount = count;
            item.depth = onTop ? -std::numeric_limits<float>::infinity() : run.depthSum / static_cast<float>(count);
            items.push_back(item);
        }
        else
            trailPoints.resize(run.start);

        run = TrailRun{ static_cast<std::uint32_t>(trailPoints.size()) };
    }

    void StageRenderer::Emit(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame, const RenderOptions& options)
    {
        const CanvasStateGuard guard{ canvas };
        canvas.FillRect(frame.Viewport(), theme::Current().Get(theme::ColorRole::SceneBackground));

        if (options.showGrid)
            scene::DrawGroundGrid(canvas, frame, options.grid);

        if (options.showOriginTriad)
            scene::DrawAxisTriad(canvas, frame, options.originTriad);

        currentPen.reset();
        currentBrush.reset();

        for (const auto& key : order)
        {
            const auto& item = items[key.item];

            if (item.kind == ItemKind::Face)
                EmitFace(canvas, stage, item);
            else
                EmitTrail(canvas, stage, item);
        }

        if (options.showFrameMarkers)
            EmitFrameMarkers(canvas, stage, frame);

        if (options.showLabels)
            EmitLabels(canvas, stage, frame);
    }

    void StageRenderer::EmitFace(Canvas& canvas, const Stage& stage, const DrawItem& item)
    {
        const auto& material = stage.Materials()[item.material.value];
        const std::span points{ item.points.data(), item.count };
        const auto fullMask = static_cast<std::uint8_t>((1u << item.count) - 1u);

        if (material.wireframe)
        {
            ApplyBrush(canvas, Brush{});
            ApplyPen(canvas, EdgePen(material));
            canvas.DrawPolygon(points);
            return;
        }

        ApplyBrush(canvas, Brush{ item.fill });

        if (material.outline && item.featureMask == fullMask)
        {
            ApplyPen(canvas, EdgePen(material));
            canvas.DrawPolygon(points);
            return;
        }

        // Stroking in the fill colour closes the anti-aliased hairline seams between neighbouring
        // faces. A translucent face would double its alpha along that stroke, so it goes without.
        ApplyPen(canvas, item.fill.alpha == 255 ? Pen{ item.fill } : Pen{ item.fill, 1.0f, LineStyle::None });
        canvas.DrawPolygon(points);

        if (!material.outline || item.featureMask == 0)
            return;

        ApplyPen(canvas, EdgePen(material));

        for (std::uint8_t i = 0; i < item.count; ++i)
            if ((item.featureMask & (1u << i)) != 0)
                canvas.DrawLine(points[i], points[(i + 1) % item.count]);
    }

    void StageRenderer::EmitTrail(Canvas& canvas, const Stage& stage, const DrawItem& item)
    {
        const auto& style = stage.Trails()[item.trail.value].Style();

        ApplyBrush(canvas, Brush{});
        ApplyPen(canvas, Pen{ Resolve(style.role, style.color), style.width, LineStyle::Solid, LineCap::Round });
        canvas.DrawPolyline(std::span{ trailPoints }.subspan(item.firstPoint, item.pointCount));
    }

    void StageRenderer::EmitFrameMarkers(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame)
    {
        static constexpr std::array<theme::ColorRole, 3> roles{ theme::ColorRole::SceneAxisX, theme::ColorRole::SceneAxisY,
            theme::ColorRole::SceneAxisZ };

        for (const auto& marker : stage.FrameMarkers())
        {
            if (!marker.visible)
                continue;

            const auto& world = stage.Graph().World(marker.node);

            for (std::size_t axis = 0; axis < roles.size(); ++axis)
            {
                auto from = frame.ToView(world.translation);
                auto to = frame.ToView(world.Apply(Matrix3{}.columns[axis] * marker.length));

                if (!scene::ClipSegmentNear(from, to, frame.NearDistance()))
                    continue;

                ApplyPen(canvas, Pen{ theme::Current().Get(roles[axis]), marker.width, LineStyle::Solid, LineCap::Round });
                canvas.DrawLine(frame.ProjectView(from), frame.ProjectView(to));
            }
        }
    }

    void StageRenderer::EmitLabels(Canvas& canvas, const Stage& stage, const scene::ViewFrame& frame)
    {
        for (const auto& label : stage.Labels())
        {
            if (!label.visible || label.length == 0)
                continue;

            const auto projected = frame.ProjectWithDepth(stage.Graph().World(label.node).Apply(label.offset));

            if (!projected)
                continue;

            canvas.SetFont(theme::Current().Get(label.style.font));
            ApplyPen(canvas, Pen{ Resolve(label.style.role, label.style.color) });
            canvas.DrawText(projected->point.Translated(label.style.pixelOffset.x, label.style.pixelOffset.y), label.Text());
        }
    }

    void StageRenderer::ApplyPen(Canvas& canvas, const Pen& pen)
    {
        if (currentPen != pen)
        {
            canvas.SetPen(pen);
            currentPen = pen;
        }
    }

    void StageRenderer::ApplyBrush(Canvas& canvas, const Brush& brush)
    {
        if (currentBrush != brush)
        {
            canvas.SetBrush(brush);
            currentBrush = brush;
        }
    }
}

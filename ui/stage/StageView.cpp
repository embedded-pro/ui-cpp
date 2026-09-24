#include "ui/stage/StageView.hpp"
#include <cmath>

namespace ui::stage
{
    StageView::StageView(const StageViewConfig& config)
        : stage(config.tessellation)
        , camera(config.pose, config.limits, config.projection)
        , options(config.render)
        , clickSlop(config.clickSlop)
        , minimumSize(config.minimumSize)
    {}

    Stage& StageView::Scene()
    {
        return stage;
    }

    const Stage& StageView::Scene() const
    {
        return stage;
    }

    scene::OrbitCamera& StageView::Camera()
    {
        return camera;
    }

    const RenderOptions& StageView::Options() const
    {
        return options;
    }

    void StageView::SetOptions(const RenderOptions& newOptions)
    {
        options = newOptions;
        RequestRepaint();
    }

    void StageView::SetSelection(NodeId node)
    {
        if (node == selection)
            return;

        selection = node;
        RequestRepaint();

        if (onSelectionChanged)
            onSelectionChanged(selection);
    }

    NodeId StageView::Selection() const
    {
        return selection;
    }

    void StageView::FrameAll()
    {
        if (const auto bounds = stage.Bounds())
        {
            camera.Frame(bounds->centre, bounds->radius);
            RequestRepaint();
        }
    }

    void StageView::Refresh() const
    {
        RequestRepaint();
    }

    void StageView::Paint(Canvas& canvas, const Rect& bounds)
    {
        const CanvasStateGuard guard{ canvas };
        canvas.SetClip(bounds);

        lastBounds = bounds;
        options.selection = selection;
        renderer.Render(canvas, stage, camera.FrameFor(bounds), options);
    }

    Size StageView::MinimumSize() const
    {
        return minimumSize;
    }

    void StageView::OnMousePress(const MouseEvent& event)
    {
        using enum MouseButton;

        const auto pan = event.button == Right || event.button == Middle || (event.button == Left && event.modifiers.shift);

        if (event.button == None)
            return;

        drag = pan ? Drag::Pan : Drag::Orbit;
        pressPosition = event.position;
        dragged = false;

        if (pan)
            camera.StartPan(event.position);
        else
            camera.StartOrbit(event.position);
    }

    // The camera sees nothing until the cursor leaves the click slop, so a click never nudges the
    // view; once it does, the first update carries the whole distance since the press.
    void StageView::OnMouseMove(const MouseEvent& event)
    {
        if (drag == Drag::None || (!dragged && !BeyondClickSlop(event.position)))
            return;

        dragged = true;

        if (drag == Drag::Pan)
            camera.UpdatePan(event.position, lastBounds.height);
        else
            camera.UpdateOrbit(event.position);

        RequestRepaint();
    }

    void StageView::OnMouseRelease(const MouseEvent& event)
    {
        if (drag == Drag::None)
            return;

        const auto click = !dragged && drag == Drag::Orbit;
        EndDrag();

        if (!click)
            return;

        const auto picked = renderer.Pick(event.position);

        if (onPick)
            onPick(picked);

        SetSelection(picked ? picked->node : NodeId{});
    }

    void StageView::OnMouseDoubleClick(const MouseEvent&)
    {
        FrameAll();
    }

    void StageView::OnWheel(const WheelEvent& event)
    {
        camera.Zoom(event.delta);
        RequestRepaint();
    }

    void StageView::OnKeyPress(const KeyEvent& event)
    {
        if (event.key == Key::Escape)
            SetSelection(NodeId{});
        else if (event.key == Key::Home)
        {
            EndDrag();
            camera.Reset();
            RequestRepaint();
        }
    }

    bool StageView::BeyondClickSlop(Point position) const
    {
        return std::hypot(position.x - pressPosition.x, position.y - pressPosition.y) > clickSlop;
    }

    void StageView::EndDrag()
    {
        camera.EndOrbit();
        camera.EndPan();
        drag = Drag::None;
    }
}

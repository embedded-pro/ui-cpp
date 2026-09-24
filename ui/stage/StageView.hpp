#pragma once

#include "ui/core/Callback.hpp"
#include "ui/core/PaintedView.hpp"
#include "ui/scene/Camera3D.hpp"
#include "ui/stage/Stage.hpp"
#include "ui/stage/StageRenderer.hpp"
#include <cstdint>
#include <optional>

namespace ui::stage
{
    struct StageViewConfig
    {
        scene::CameraPose pose;
        scene::OrbitLimits limits;
        scene::ProjectionConfig projection;
        RenderOptions render;
        Tessellation tessellation;

        // A press and release closer than this is a click, not a drag, so a slightly shaky hand
        // still selects.
        float clickSlop{ 4.0f };
        Size minimumSize{ 200.0f, 150.0f };
    };

    // The 3D stage as a widget: owns the model, the camera and the renderer. Left-drag orbits;
    // right-, middle- or shift-drag pans; the wheel zooms; a click picks and selects; a double
    // click frames everything; Escape clears the selection and Home resets the camera.
    //
    // Camera and selection changes repaint on their own. Moving joints or feeding trails is data,
    // so the host's timer repaints, as for ScopeCore; call Refresh() for a one-off change.
    class StageView
        : public PaintedView
    {
    public:
        explicit StageView(const StageViewConfig& config = {});

        [[nodiscard]] Stage& Scene();
        [[nodiscard]] const Stage& Scene() const;
        [[nodiscard]] scene::OrbitCamera& Camera();
        [[nodiscard]] const RenderOptions& Options() const;
        void SetOptions(const RenderOptions& newOptions);

        // An invalid node clears the selection.
        void SetSelection(NodeId node);
        [[nodiscard]] NodeId Selection() const;

        void FrameAll();
        void Refresh() const;

        void Paint(Canvas& canvas, const Rect& bounds) override;
        [[nodiscard]] Size MinimumSize() const override;

        void OnMousePress(const MouseEvent& event) override;
        void OnMouseMove(const MouseEvent& event) override;
        void OnMouseRelease(const MouseEvent& event) override;
        void OnMouseDoubleClick(const MouseEvent& event) override;
        void OnWheel(const WheelEvent& event) override;
        void OnKeyPress(const KeyEvent& event) override;

        Callback<void(std::optional<PickResult>)> onPick;
        Callback<void(NodeId)> onSelectionChanged;

    private:
        enum class Drag : std::uint8_t
        {
            None,
            Orbit,
            Pan
        };

        [[nodiscard]] bool BeyondClickSlop(Point position) const;
        void EndDrag();

        Stage stage;
        StageRenderer renderer;
        scene::OrbitCamera camera;
        RenderOptions options;
        float clickSlop;
        Size minimumSize;

        NodeId selection;
        Rect lastBounds;
        Drag drag{ Drag::None };
        Point pressPosition;
        bool dragged{ false };
    };
}

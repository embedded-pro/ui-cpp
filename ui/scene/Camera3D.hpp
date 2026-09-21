#pragma once

#include "ui/core/Geometry.hpp"
#include "ui/scene/Vector3.hpp"

namespace ui::scene
{
    struct ProjectionConfig
    {
        float fieldOfViewDegrees{ 60.0f };

        // A clamp on view-space depth, not a near-plane clip: geometry behind the eye smears onto
        // the near plane rather than disappearing. Preserved from the Qt original deliberately.
        float nearDistance{ 0.01f };
    };

    struct OrbitLimits
    {
        float radiansPerPixel{ 0.008f };
        float distancePerWheelStep{ 0.3f };

        // One mouse notch as the backends report it; QtPaintedWidget forwards angleDelta().y()
        // unscaled, so a backend with a different quantum sets this rather than rescaling.
        float wheelDeltaPerStep{ 120.0f };

        float minimumElevation{ -0.2f };
        float maximumElevation{ 1.5f };
        float minimumDistance{ 1.0f };
        float maximumDistance{ 15.0f };
    };

    struct CameraPose
    {
        float azimuth{ 0.8f };
        float elevation{ 0.45f };
        float distance{ 3.5f };
        Vector3 lookAt{ 0.0f, 0.0f, 0.3f };
    };

    // Built once per repaint and then projects. The Qt original rebuilt this basis inside every
    // Project() call - 1092 times per frame at two degrees of freedom with a full trail.
    class ViewFrame
    {
    public:
        ViewFrame(const CameraPose& pose, const Rect& viewport, const ProjectionConfig& projection);

        [[nodiscard]] Point Project(Vector3 world) const;

        [[nodiscard]] Vector3 Eye() const;
        [[nodiscard]] Vector3 Forward() const;
        [[nodiscard]] Vector3 Right() const;
        [[nodiscard]] Vector3 Up() const;
        [[nodiscard]] const Rect& Viewport() const;

    private:
        Vector3 eye;
        Vector3 forward;
        Vector3 right;
        Vector3 up;
        Rect viewport;
        float tanHalfFieldOfView{ 1.0f };
        float aspect{ 1.0f };
        float nearDistance{ 0.01f };
    };

    // Deliberately not an InputHandler: a camera is a value you copy, store and compare, and
    // deciding which drag belongs to the camera is the view's job. ChartInteraction is the same
    // shape for the same reason.
    class OrbitCamera
    {
    public:
        OrbitCamera() = default;
        OrbitCamera(CameraPose pose, OrbitLimits limits, ProjectionConfig projection);

        [[nodiscard]] ViewFrame FrameFor(const Rect& viewport) const;

        void StartOrbit(Point position);
        void UpdateOrbit(Point position);
        void EndOrbit();
        [[nodiscard]] bool IsOrbiting() const;

        void Orbit(float deltaX, float deltaY);
        void Zoom(float wheelDelta);
        void Reset();

        void SetLookAt(Vector3 lookAt);
        void SetPose(const CameraPose& pose);
        [[nodiscard]] const CameraPose& Pose() const;
        [[nodiscard]] const OrbitLimits& Limits() const;

    private:
        CameraPose pose;
        CameraPose initialPose;
        OrbitLimits limits;
        ProjectionConfig projection;

        bool orbiting{ false };
        Point lastPosition;
    };
}

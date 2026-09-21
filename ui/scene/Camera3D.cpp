#include "ui/scene/Camera3D.hpp"
#include <cmath>
#include <numbers>

namespace ui::scene
{
    namespace
    {
        constexpr float degeneracyEpsilon{ 1e-6f };
        constexpr float twoPi{ 2.0f * std::numbers::pi_v<float> };

        float WrapAngle(float radians)
        {
            const auto wrapped = std::remainder(radians, twoPi);

            return wrapped;
        }
    }

    ViewFrame::ViewFrame(const CameraPose& pose, const Rect& viewport, const ProjectionConfig& projection)
        : viewport(viewport)
        , nearDistance(projection.nearDistance)
    {
        eye = Vector3{
            pose.distance * std::cos(pose.elevation) * std::cos(pose.azimuth) + pose.lookAt.x,
            pose.distance * std::cos(pose.elevation) * std::sin(pose.azimuth) + pose.lookAt.y,
            pose.distance * std::sin(pose.elevation) + pose.lookAt.z
        };

        forward = Normalized(pose.lookAt - eye, degeneracyEpsilon);

        right = Cross(forward, worldUp);
        const auto rightLength = Length(right);

        if (rightLength < degeneracyEpsilon)
            right = Vector3{ 1.0f, 0.0f, 0.0f };
        else
            right = right * (1.0f / rightLength);

        // Not renormalized: right and forward are unit and perpendicular, so their cross already
        // is. Only the gimbal fallback above breaks that, and only momentarily.
        up = Cross(right, forward);

        tanHalfFieldOfView = std::tan(projection.fieldOfViewDegrees * std::numbers::pi_v<float> / 180.0f * 0.5f);
        aspect = std::max(1.0f, viewport.width) / std::max(1.0f, viewport.height);
    }

    Point ViewFrame::Project(Vector3 world) const
    {
        const auto offset = world - eye;

        const auto viewX = Dot(right, offset);
        const auto viewY = Dot(up, offset);
        const auto viewZ = std::max(Dot(forward, offset), nearDistance);

        const auto normalizedX = viewX / (viewZ * tanHalfFieldOfView * aspect);
        const auto normalizedY = viewY / (viewZ * tanHalfFieldOfView);

        return Point{
            viewport.x + (normalizedX + 1.0f) * 0.5f * viewport.width,
            viewport.y + (1.0f - normalizedY) * 0.5f * viewport.height
        };
    }

    Vector3 ViewFrame::Eye() const
    {
        return eye;
    }

    Vector3 ViewFrame::Forward() const
    {
        return forward;
    }

    Vector3 ViewFrame::Right() const
    {
        return right;
    }

    Vector3 ViewFrame::Up() const
    {
        return up;
    }

    const Rect& ViewFrame::Viewport() const
    {
        return viewport;
    }

    OrbitCamera::OrbitCamera(CameraPose pose, OrbitLimits limits, ProjectionConfig projection)
        : pose(pose)
        , initialPose(pose)
        , limits(limits)
        , projection(projection)
    {}

    ViewFrame OrbitCamera::FrameFor(const Rect& viewport) const
    {
        return ViewFrame{ pose, viewport, projection };
    }

    void OrbitCamera::StartOrbit(Point position)
    {
        orbiting = true;
        lastPosition = position;
    }

    // Does nothing unless a press started the drag: the Qt adapter reports MouseButton::None on
    // every move event, and mouse tracking is on, so without this a hover would orbit the camera.
    void OrbitCamera::UpdateOrbit(Point position)
    {
        if (!orbiting)
            return;

        Orbit(position.x - lastPosition.x, position.y - lastPosition.y);
        lastPosition = position;
    }

    void OrbitCamera::EndOrbit()
    {
        orbiting = false;
    }

    bool OrbitCamera::IsOrbiting() const
    {
        return orbiting;
    }

    void OrbitCamera::Orbit(float deltaX, float deltaY)
    {
        pose.azimuth = WrapAngle(pose.azimuth - deltaX * limits.radiansPerPixel);
        pose.elevation = Clamp(pose.elevation + deltaY * limits.radiansPerPixel,
            limits.minimumElevation, limits.maximumElevation);
    }

    // Linear rather than multiplicative, matching the ported widget; ChartInteraction::Zoom is
    // multiplicative and reads only the sign, which is a different interaction, not an
    // inconsistency.
    void OrbitCamera::Zoom(float wheelDelta)
    {
        if (limits.wheelDeltaPerStep <= 0.0f)
            return;

        const auto steps = wheelDelta / limits.wheelDeltaPerStep;

        pose.distance = Clamp(pose.distance - steps * limits.distancePerWheelStep,
            limits.minimumDistance, limits.maximumDistance);
    }

    void OrbitCamera::Reset()
    {
        pose = initialPose;
        orbiting = false;
    }

    void OrbitCamera::SetLookAt(Vector3 lookAt)
    {
        pose.lookAt = lookAt;
    }

    void OrbitCamera::SetPose(const CameraPose& newPose)
    {
        pose = newPose;
    }

    const CameraPose& OrbitCamera::Pose() const
    {
        return pose;
    }

    const OrbitLimits& OrbitCamera::Limits() const
    {
        return limits;
    }
}

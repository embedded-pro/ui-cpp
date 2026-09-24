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

        float TanHalfFieldOfView(const ProjectionConfig& projection)
        {
            return std::tan(projection.fieldOfViewDegrees * std::numbers::pi_v<float> / 180.0f * 0.5f);
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

        tanHalfFieldOfView = TanHalfFieldOfView(projection);
        aspect = std::max(1.0f, viewport.width) / std::max(1.0f, viewport.height);
    }

    Point ViewFrame::Project(Vector3 world) const
    {
        return ProjectView(ToView(world));
    }

    Vector3 ViewFrame::ToView(Vector3 world) const
    {
        const auto offset = world - eye;

        return Vector3{ Dot(right, offset), Dot(up, offset), Dot(forward, offset) };
    }

    Point ViewFrame::ProjectView(Vector3 view) const
    {
        const auto viewZ = std::max(view.z, nearDistance);

        const auto normalizedX = view.x / (viewZ * tanHalfFieldOfView * aspect);
        const auto normalizedY = view.y / (viewZ * tanHalfFieldOfView);

        return Point{
            viewport.x + (normalizedX + 1.0f) * 0.5f * viewport.width,
            viewport.y + (1.0f - normalizedY) * 0.5f * viewport.height
        };
    }

    std::optional<ProjectedPoint> ViewFrame::ProjectWithDepth(Vector3 world) const
    {
        const auto view = ToView(world);

        if (view.z < nearDistance)
            return std::nullopt;

        return ProjectedPoint{ ProjectView(view), view.z };
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

    float ViewFrame::NearDistance() const
    {
        return nearDistance;
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

    void OrbitCamera::StartPan(Point position)
    {
        panning = true;
        lastPosition = position;
    }

    void OrbitCamera::UpdatePan(Point position, float viewportHeight)
    {
        if (!panning)
            return;

        Pan(position.x - lastPosition.x, position.y - lastPosition.y, viewportHeight);
        lastPosition = position;
    }

    void OrbitCamera::EndPan()
    {
        panning = false;
    }

    bool OrbitCamera::IsPanning() const
    {
        return panning;
    }

    void OrbitCamera::Pan(float deltaX, float deltaY, float viewportHeight)
    {
        if (viewportHeight <= 0.0f)
            return;

        const ViewFrame frame{ pose, Rect{ 0.0f, 0.0f, 1.0f, 1.0f }, projection };
        const auto worldPerPixel = 2.0f * pose.distance * TanHalfFieldOfView(projection) / viewportHeight;

        pose.lookAt = pose.lookAt - frame.Right() * (deltaX * worldPerPixel) + frame.Up() * (deltaY * worldPerPixel);
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
        panning = false;
    }

    void OrbitCamera::Frame(Vector3 centre, float radius)
    {
        const auto sineHalfFieldOfView = std::sin(projection.fieldOfViewDegrees * std::numbers::pi_v<float> / 180.0f * 0.5f);

        pose.lookAt = centre;

        if (sineHalfFieldOfView > 0.0f)
            pose.distance = Clamp(radius / sineHalfFieldOfView, limits.minimumDistance, limits.maximumDistance);
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

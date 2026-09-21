#include "ui/scene/Camera3D.hpp"
#include <array>
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::scene::CameraPose;
    using ui::scene::OrbitCamera;
    using ui::scene::OrbitLimits;
    using ui::scene::ProjectionConfig;
    using ui::scene::Vector3;
    using ui::scene::ViewFrame;

    constexpr ui::Rect standardViewport{ 0.0f, 0.0f, 800.0f, 600.0f };

    class ViewFrameTest
        : public ::testing::Test
    {
    protected:
        static ViewFrame FrameFor(const CameraPose& pose, const ui::Rect& viewport = standardViewport)
        {
            return ViewFrame{ pose, viewport, ProjectionConfig{} };
        }
    };

    class OrbitCameraTest
        : public ::testing::Test
    {
    protected:
        OrbitCamera camera{ CameraPose{}, OrbitLimits{}, ProjectionConfig{} };
    };
}

// Exact, and independent of every camera parameter: the look-at point lies along the forward
// axis, so its view-space x and y are zero and it lands dead centre.
TEST_F(ViewFrameTest, TheLookAtPointProjectsToTheViewportCentre)
{
    for (const auto& pose : std::array<CameraPose, 3>{
             CameraPose{},
             CameraPose{ 2.1f, 1.2f, 8.0f, Vector3{ 0.0f, 0.0f, 0.3f } },
             CameraPose{ -1.4f, -0.2f, 1.0f, Vector3{ 0.1f, -0.2f, 0.5f } } })
    {
        const auto projected = FrameFor(pose).Project(pose.lookAt);

        EXPECT_NEAR(projected.x, 400.0f, 1e-2f);
        EXPECT_NEAR(projected.y, 300.0f, 1e-2f);
    }
}

TEST_F(ViewFrameTest, TheViewportOriginOffsetsTheProjection)
{
    const CameraPose pose{};
    const auto projected = ViewFrame{ pose, ui::Rect{ 100.0f, 50.0f, 800.0f, 600.0f }, {} }.Project(pose.lookAt);

    EXPECT_NEAR(projected.x, 500.0f, 1e-2f);
    EXPECT_NEAR(projected.y, 350.0f, 1e-2f);
}

TEST_F(ViewFrameTest, AnAxisAlignedPoseGivesTheHandComputedBasis)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    EXPECT_NEAR(frame.Eye().x, 3.0f, 1e-5f);
    EXPECT_NEAR(frame.Eye().y, 0.0f, 1e-5f);
    EXPECT_NEAR(frame.Eye().z, 0.0f, 1e-5f);

    EXPECT_NEAR(frame.Forward().x, -1.0f, 1e-5f);
    EXPECT_NEAR(frame.Right().y, 1.0f, 1e-5f);
    EXPECT_NEAR(frame.Up().z, 1.0f, 1e-5f);
}

// aspect is width/height, so the width cancels out of the horizontal pixel offset entirely.
// Catches an inverted or dropped aspect, which a centred-point test cannot.
TEST_F(ViewFrameTest, TheHorizontalOffsetFromCentreDoesNotDependOnViewportWidth)
{
    const CameraPose pose{ 0.0f, 0.0f, 3.0f, Vector3{} };
    const Vector3 point{ 0.0f, 0.5f, 0.0f };

    const auto narrow = ViewFrame{ pose, ui::Rect{ 0.0f, 0.0f, 800.0f, 600.0f }, {} }.Project(point);
    const auto wide = ViewFrame{ pose, ui::Rect{ 0.0f, 0.0f, 1600.0f, 600.0f }, {} }.Project(point);

    EXPECT_NEAR(narrow.x - 400.0f, wide.x - 800.0f, 1e-3f);
}

TEST_F(ViewFrameTest, AHigherWorldZProjectsToASmallerScreenY)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    EXPECT_LT(frame.Project(Vector3{ 0.0f, 0.0f, 0.5f }).y, frame.Project(Vector3{ 0.0f, 0.0f, 0.0f }).y);
}

TEST_F(ViewFrameTest, AMoreDistantPointProjectsNearerTheCentre)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    const auto near = frame.Project(Vector3{ 0.0f, 0.5f, 0.0f });
    const auto far = frame.Project(Vector3{ -5.0f, 0.5f, 0.0f });

    EXPECT_LT(std::abs(far.x - 400.0f), std::abs(near.x - 400.0f));
}

// The near distance is a clamp, not a clip: geometry behind the eye smears onto the near plane
// instead of vanishing. Asserted so the artefact is preserved deliberately rather than by luck.
TEST_F(ViewFrameTest, PointsBehindTheEyeAreClampedRatherThanDropped)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    const auto firstBehind = frame.Project(Vector3{ 10.0f, 0.1f, 0.0f });
    const auto secondBehind = frame.Project(Vector3{ 20.0f, 0.1f, 0.0f });

    EXPECT_TRUE(std::isfinite(firstBehind.x));
    EXPECT_TRUE(std::isfinite(firstBehind.y));
    EXPECT_NEAR(firstBehind.y, secondBehind.y, 1e-3f);
}

// Only reachable by constructing a pose directly: OrbitLimits caps elevation at 1.5 rad, below
// the pi/2 at which forward becomes parallel to world up.
TEST_F(ViewFrameTest, LookingStraightDownFallsBackToAUsableRightVector)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, std::numbers::pi_v<float> / 2.0f, 3.0f, Vector3{} });

    EXPECT_NEAR(frame.Right().x, 1.0f, 1e-5f);
    EXPECT_NEAR(frame.Right().y, 0.0f, 1e-5f);
    EXPECT_TRUE(std::isfinite(frame.Project(Vector3{ 1.0f, 1.0f, 0.0f }).x));
}

TEST_F(ViewFrameTest, ADegenerateViewportProducesNoNaN)
{
    const auto projected = ViewFrame{ CameraPose{}, ui::Rect{}, {} }.Project(Vector3{ 1.0f, 1.0f, 1.0f });

    EXPECT_TRUE(std::isfinite(projected.x));
    EXPECT_TRUE(std::isfinite(projected.y));
}

// Reference values produced by running the Qt original's Project body verbatim against the same
// pose and viewport; the two agree to about five float ULPs, the difference being the original's
// truncated 3.14159265f against std::numbers::pi_v<float>.
TEST_F(ViewFrameTest, ProjectionMatchesTheQtOriginalItReplaced)
{
    struct Reference
    {
        Vector3 world;
        ui::Point screen;
    };

    static constexpr std::array<Reference, 7> references{ {
        { Vector3{ 0.0f, 0.0f, 0.0f }, ui::Point{ 400.0000f, 338.6631f } },
        { Vector3{ 0.4f, 0.0f, 0.0f }, ui::Point{ 355.8818f, 360.1714f } },
        { Vector3{ 0.0f, 0.4f, 0.0f }, ui::Point{ 442.9427f, 360.8577f } },
        { Vector3{ 0.0f, 0.0f, 0.4f }, ui::Point{ 400.0000f, 286.4636f } },
        { Vector3{ 2.0f, 2.0f, 0.0f }, ui::Point{ 380.2019f, 1019.2129f } },
        { Vector3{ -2.0f, -2.0f, 0.0f }, ui::Point{ 403.4741f, 219.2445f } },
        { Vector3{ 0.5f, 0.3f, 0.7f }, ui::Point{ 372.4083f, 278.7900f } },
    } };

    const auto frame = FrameFor(CameraPose{});

    for (const auto& reference : references)
    {
        const auto projected = frame.Project(reference.world);

        EXPECT_NEAR(projected.x, reference.screen.x, 1e-3f);
        EXPECT_NEAR(projected.y, reference.screen.y, 1e-3f);
    }
}

TEST_F(OrbitCameraTest, DraggingRightDecreasesAzimuth)
{
    camera.Orbit(10.0f, 0.0f);

    EXPECT_NEAR(camera.Pose().azimuth, 0.8f - 0.08f, 1e-5f);
}

TEST_F(OrbitCameraTest, DraggingDownIncreasesElevation)
{
    camera.Orbit(0.0f, 10.0f);

    EXPECT_NEAR(camera.Pose().elevation, 0.45f + 0.08f, 1e-5f);
}

TEST_F(OrbitCameraTest, ElevationSaturatesAtBothLimits)
{
    camera.Orbit(0.0f, 10000.0f);
    EXPECT_NEAR(camera.Pose().elevation, camera.Limits().maximumElevation, 1e-5f);

    camera.Orbit(0.0f, -10000.0f);
    EXPECT_NEAR(camera.Pose().elevation, camera.Limits().minimumElevation, 1e-5f);
}

TEST_F(OrbitCameraTest, AzimuthStaysBoundedAcrossManyDrags)
{
    for (auto i = 0; i < 10000; ++i)
        camera.Orbit(50.0f, 0.0f);

    EXPECT_LE(std::abs(camera.Pose().azimuth), std::numbers::pi_v<float> + 1e-4f);
}

TEST_F(OrbitCameraTest, ScrollingForwardZoomsIn)
{
    camera.Zoom(120.0f);

    EXPECT_NEAR(camera.Pose().distance, 3.5f - 0.3f, 1e-5f);
}

TEST_F(OrbitCameraTest, ScrollingBackZoomsOut)
{
    camera.Zoom(-120.0f);

    EXPECT_NEAR(camera.Pose().distance, 3.5f + 0.3f, 1e-5f);
}

// Half a notch moves half as far: the zoom honours the magnitude, unlike ChartInteraction::Zoom,
// which reads only the sign.
TEST_F(OrbitCameraTest, APartialWheelStepMovesProportionally)
{
    camera.Zoom(60.0f);

    EXPECT_NEAR(camera.Pose().distance, 3.5f - 0.15f, 1e-5f);
}

TEST_F(OrbitCameraTest, DistanceSaturatesAtBothLimits)
{
    camera.Zoom(100000.0f);
    EXPECT_NEAR(camera.Pose().distance, camera.Limits().minimumDistance, 1e-5f);

    camera.Zoom(-100000.0f);
    EXPECT_NEAR(camera.Pose().distance, camera.Limits().maximumDistance, 1e-5f);
}

// The Qt adapter reports MouseButton::None on every move event and enables mouse tracking, so
// without the orbiting flag a hover would spin the camera.
TEST_F(OrbitCameraTest, MovingWithoutAPressLeavesThePoseUntouched)
{
    const auto before = camera.Pose();

    camera.UpdateOrbit(ui::Point{ 10.0f, 10.0f });
    camera.UpdateOrbit(ui::Point{ 90.0f, 90.0f });

    EXPECT_NEAR(camera.Pose().azimuth, before.azimuth, 1e-6f);
    EXPECT_NEAR(camera.Pose().elevation, before.elevation, 1e-6f);
    EXPECT_FALSE(camera.IsOrbiting());
}

TEST_F(OrbitCameraTest, MovingAfterAPressOrbits)
{
    camera.StartOrbit(ui::Point{ 10.0f, 10.0f });
    EXPECT_TRUE(camera.IsOrbiting());

    camera.UpdateOrbit(ui::Point{ 20.0f, 10.0f });

    EXPECT_NEAR(camera.Pose().azimuth, 0.8f - 0.08f, 1e-5f);
}

TEST_F(OrbitCameraTest, MovingAfterReleaseLeavesThePoseUntouched)
{
    camera.StartOrbit(ui::Point{ 10.0f, 10.0f });
    camera.UpdateOrbit(ui::Point{ 20.0f, 10.0f });
    camera.EndOrbit();

    const auto after = camera.Pose();
    camera.UpdateOrbit(ui::Point{ 200.0f, 200.0f });

    EXPECT_NEAR(camera.Pose().azimuth, after.azimuth, 1e-6f);
}

TEST_F(OrbitCameraTest, ResettingRestoresTheConstructionPose)
{
    camera.StartOrbit(ui::Point{});
    camera.Orbit(100.0f, 50.0f);
    camera.Zoom(240.0f);

    camera.Reset();

    EXPECT_NEAR(camera.Pose().azimuth, 0.8f, 1e-6f);
    EXPECT_NEAR(camera.Pose().elevation, 0.45f, 1e-6f);
    EXPECT_NEAR(camera.Pose().distance, 3.5f, 1e-6f);
    EXPECT_FALSE(camera.IsOrbiting());
}

TEST_F(OrbitCameraTest, ThePoseAndTheViewportAreIndependent)
{
    const auto narrow = camera.FrameFor(ui::Rect{ 0.0f, 0.0f, 400.0f, 300.0f });
    const auto wide = camera.FrameFor(ui::Rect{ 0.0f, 0.0f, 1600.0f, 900.0f });

    EXPECT_NEAR(narrow.Eye().x, wide.Eye().x, 1e-6f);
    EXPECT_NEAR(narrow.Forward().z, wide.Forward().z, 1e-6f);
    EXPECT_NE(narrow.Project(Vector3{ 1.0f, 1.0f, 0.0f }).x, wide.Project(Vector3{ 1.0f, 1.0f, 0.0f }).x);
}

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

TEST_F(ViewFrameTest, TheFrameExposesTheBasisItProjectsWith)
{
    const auto frame = FrameFor(CameraPose{});

    // Right, up and forward are mutually orthogonal and unit length, which is what makes the
    // per-vertex projection a pair of dot products.
    EXPECT_NEAR(ui::scene::Length(frame.Up()), 1.0f, 1e-5f);
    EXPECT_NEAR(ui::scene::Dot(frame.Up(), frame.Right()), 0.0f, 1e-5f);
    EXPECT_NEAR(ui::scene::Dot(frame.Up(), frame.Forward()), 0.0f, 1e-5f);

    EXPECT_EQ(frame.Viewport().width, standardViewport.width);
    EXPECT_EQ(frame.Viewport().height, standardViewport.height);
}

// A backend that reports no wheel quantum must not divide by it.
TEST_F(OrbitCameraTest, AZeroWheelQuantumLeavesTheDistanceAlone)
{
    OrbitLimits limits;
    limits.wheelDeltaPerStep = 0.0f;

    OrbitCamera stubborn{ CameraPose{}, limits, ProjectionConfig{} };
    const auto before = stubborn.Pose().distance;

    stubborn.Zoom(120.0f);

    EXPECT_NEAR(stubborn.Pose().distance, before, 1e-6f);
}

TEST_F(OrbitCameraTest, SettingTheLookAtMovesOnlyTheTarget)
{
    const auto before = camera.Pose();

    camera.SetLookAt(Vector3{ 1.0f, 2.0f, 3.0f });

    EXPECT_NEAR(camera.Pose().lookAt.x, 1.0f, 1e-6f);
    EXPECT_NEAR(camera.Pose().lookAt.z, 3.0f, 1e-6f);
    EXPECT_NEAR(camera.Pose().azimuth, before.azimuth, 1e-6f);
    EXPECT_NEAR(camera.Pose().distance, before.distance, 1e-6f);
}

// Reset returns to the pose the camera was constructed with, not to whatever was set later.
TEST_F(OrbitCameraTest, SettingAPoseDoesNotMoveTheResetTarget)
{
    const auto original = camera.Pose();

    camera.SetPose(CameraPose{ 1.2f, 0.9f, 7.0f, Vector3{ 0.0f, 0.0f, 1.0f } });
    EXPECT_NEAR(camera.Pose().distance, 7.0f, 1e-6f);

    camera.Reset();

    EXPECT_NEAR(camera.Pose().azimuth, original.azimuth, 1e-6f);
    EXPECT_NEAR(camera.Pose().distance, original.distance, 1e-6f);
}

TEST_F(ViewFrameTest, ProjectIsProjectViewOfToView)
{
    const auto frame = FrameFor(CameraPose{ 0.4f, 0.3f, 4.0f, Vector3{ 0.1f, 0.2f, 0.3f } });
    const Vector3 point{ 0.5f, -0.4f, 0.8f };

    const auto direct = frame.Project(point);
    const auto staged = frame.ProjectView(frame.ToView(point));

    EXPECT_NEAR(direct.x, staged.x, 1e-4f);
    EXPECT_NEAR(direct.y, staged.y, 1e-4f);
}

TEST_F(ViewFrameTest, ViewSpaceDepthIsTheDistanceAlongForward)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    const auto view = frame.ToView(Vector3{ -1.0f, 0.5f, 0.25f });

    EXPECT_NEAR(view.x, 0.5f, 1e-5f);
    EXPECT_NEAR(view.y, 0.25f, 1e-5f);
    EXPECT_NEAR(view.z, 4.0f, 1e-5f);
}

TEST_F(ViewFrameTest, ProjectWithDepthReportsTheDepth)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    const auto projected = frame.ProjectWithDepth(Vector3{});

    ASSERT_TRUE(projected.has_value());
    EXPECT_NEAR(projected->depth, 3.0f, 1e-5f);
    EXPECT_NEAR(projected->point.x, 400.0f, 1e-2f);
}

TEST_F(ViewFrameTest, ProjectWithDepthRejectsPointsBehindTheEye)
{
    const auto frame = FrameFor(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    EXPECT_FALSE(frame.ProjectWithDepth(Vector3{ 10.0f, 0.1f, 0.0f }).has_value());
    EXPECT_NEAR(frame.NearDistance(), ProjectionConfig{}.nearDistance, 1e-9f);
}

TEST_F(OrbitCameraTest, PanningRightMovesTheLookAtAgainstTheCameraRight)
{
    const auto right = camera.FrameFor(standardViewport).Right();
    const auto before = camera.Pose().lookAt;

    camera.Pan(10.0f, 0.0f, 600.0f);

    const auto moved = camera.Pose().lookAt - before;

    EXPECT_LT(ui::scene::Dot(moved, right), 0.0f);
    EXPECT_NEAR(camera.Pose().distance, CameraPose{}.distance, 1e-6f);
}

// At the look-at depth, one pixel of drag is exactly one pixel of scene motion.
TEST_F(OrbitCameraTest, APanKeepsTheLookAtPointUnderTheCursor)
{
    const auto before = camera.FrameFor(standardViewport).Project(CameraPose{}.lookAt);

    camera.Pan(25.0f, -10.0f, standardViewport.height);

    const auto after = camera.FrameFor(standardViewport).Project(CameraPose{}.lookAt);

    EXPECT_NEAR(after.x - before.x, 25.0f, 0.05f);
    EXPECT_NEAR(after.y - before.y, -10.0f, 0.05f);
}

TEST_F(OrbitCameraTest, PanDragsOnlyAfterAPress)
{
    camera.UpdatePan(ui::Point{ 50.0f, 50.0f }, 600.0f);
    EXPECT_EQ(camera.Pose().lookAt, CameraPose{}.lookAt);

    camera.StartPan(ui::Point{ 0.0f, 0.0f });
    EXPECT_TRUE(camera.IsPanning());
    camera.UpdatePan(ui::Point{ 50.0f, 50.0f }, 600.0f);
    EXPECT_NE(camera.Pose().lookAt, CameraPose{}.lookAt);

    camera.EndPan();
    EXPECT_FALSE(camera.IsPanning());
}

TEST_F(OrbitCameraTest, FramingCentresAndFitsTheSphere)
{
    camera.Frame(Vector3{ 1.0f, 2.0f, 0.5f }, 1.0f);

    EXPECT_EQ(camera.Pose().lookAt, (Vector3{ 1.0f, 2.0f, 0.5f }));
    EXPECT_NEAR(camera.Pose().distance, 2.0f, 1e-4f);
}

TEST_F(OrbitCameraTest, FramingRespectsTheDistanceLimits)
{
    camera.Frame(Vector3{}, 100.0f);

    EXPECT_NEAR(camera.Pose().distance, OrbitLimits{}.maximumDistance, 1e-6f);
}

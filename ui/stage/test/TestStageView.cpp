#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/backend/recording/RecordingPaintedHost.hpp"
#include "ui/stage/StageBuilder.hpp"
#include "ui/stage/StageView.hpp"
#include <gmock/gmock.h>

namespace
{
    using ui::MouseButton;
    using ui::MouseEvent;
    using ui::Point;
    using ui::backend::recording::CommandKind;
    using ui::stage::NodeId;
    using ui::stage::PickResult;
    using ui::stage::StageView;
    using ui::stage::Vector3;

    constexpr ui::Rect bounds{ 0.0f, 0.0f, 800.0f, 600.0f };

    class StageViewTest
        : public ::testing::Test
    {
    protected:
        StageViewTest()
        {
            view.onPick = [this](std::optional<PickResult> result)
            {
                picks.Call(result);
            };
            view.onSelectionChanged = [this](NodeId node)
            {
                selections.Call(node);
            };

            ui::stage::AddBox(view.Scene(), box, Vector3{ 0.4f, 0.4f, 0.4f }, view.Scene().AddMaterial(ui::stage::materials::Aluminium()));
            view.Paint(canvas, bounds);
            host.ClearInvalidateCount();
        }

        void Click(Point position, MouseButton button = MouseButton::Left)
        {
            view.OnMousePress(MouseEvent{ position, button, {} });
            view.OnMouseRelease(MouseEvent{ position, button, {} });
        }

        void Drag(Point from, Point to, MouseButton button = MouseButton::Left, ui::Modifiers modifiers = {})
        {
            view.OnMousePress(MouseEvent{ from, button, modifiers });
            view.OnMouseMove(MouseEvent{ to, MouseButton::None, modifiers });
            view.OnMouseRelease(MouseEvent{ to, button, modifiers });
        }

        [[nodiscard]] Point BoxCentre()
        {
            return view.Camera().FrameFor(bounds).Project(Vector3{ 0.0f, 0.0f, 0.3f });
        }

        StageView view;
        NodeId box{ view.Scene().Graph().AddFrame(NodeId{}, ui::stage::Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.3f })) };
        ui::backend::recording::RecordingPaintedHost host{ view };
        ui::backend::recording::RecordingCanvas canvas;
        ::testing::StrictMock<::testing::MockFunction<void(std::optional<PickResult>)>> picks;
        ::testing::StrictMock<::testing::MockFunction<void(NodeId)>> selections;
    };
}

TEST_F(StageViewTest, PaintingClipsToTheBoundsAndBalancesState)
{
    canvas.Clear();
    view.Paint(canvas, bounds);

    ASSERT_GE(canvas.Commands().size(), 2u);
    EXPECT_EQ(canvas.Commands()[1].kind, CommandKind::SetClip);
    EXPECT_EQ(canvas.CountOf(CommandKind::Save), canvas.CountOf(CommandKind::Restore));
    EXPECT_GT(canvas.CountOf(CommandKind::DrawPolygon), 0u);
}

TEST_F(StageViewTest, ALeftDragOrbitsAndRepaints)
{
    const auto before = view.Camera().Pose().azimuth;

    Drag(Point{ 100.0f, 100.0f }, Point{ 160.0f, 100.0f });

    EXPECT_NE(view.Camera().Pose().azimuth, before);
    EXPECT_GT(host.InvalidateCount(), 0u);
}

TEST_F(StageViewTest, HoveringDoesNotMoveTheCamera)
{
    const auto before = view.Camera().Pose();

    view.OnMouseMove(MouseEvent{ Point{ 300.0f, 300.0f }, MouseButton::None, {} });

    EXPECT_EQ(view.Camera().Pose().azimuth, before.azimuth);
    EXPECT_EQ(host.InvalidateCount(), 0u);
}

TEST_F(StageViewTest, AShiftDragPansInsteadOfOrbiting)
{
    const auto before = view.Camera().Pose();

    Drag(Point{ 100.0f, 100.0f }, Point{ 160.0f, 120.0f }, MouseButton::Left, ui::Modifiers{ true, false, false });

    EXPECT_EQ(view.Camera().Pose().azimuth, before.azimuth);
    EXPECT_NE(view.Camera().Pose().lookAt, before.lookAt);
}

TEST_F(StageViewTest, MiddleAndRightDragsPan)
{
    const auto before = view.Camera().Pose().lookAt;

    Drag(Point{ 100.0f, 100.0f }, Point{ 140.0f, 100.0f }, MouseButton::Middle);
    const auto afterMiddle = view.Camera().Pose().lookAt;
    Drag(Point{ 100.0f, 100.0f }, Point{ 140.0f, 100.0f }, MouseButton::Right);

    EXPECT_NE(afterMiddle, before);
    EXPECT_NE(view.Camera().Pose().lookAt, afterMiddle);
}

TEST_F(StageViewTest, AClickOnAPartPicksAndSelectsItsNode)
{
    const ui::stage::PartId part{ 0 };

    EXPECT_CALL(picks, Call(std::optional{ PickResult{ box, part } }));
    EXPECT_CALL(selections, Call(box));

    Click(BoxCentre());

    EXPECT_EQ(view.Selection(), box);
    EXPECT_GT(host.InvalidateCount(), 0u);
}

TEST_F(StageViewTest, AWobbleWithinTheSlopIsStillAClick)
{
    EXPECT_CALL(picks, Call(::testing::Ne(std::nullopt)));
    EXPECT_CALL(selections, Call(box));

    const auto centre = BoxCentre();
    const auto azimuth = view.Camera().Pose().azimuth;

    view.OnMousePress(MouseEvent{ centre, MouseButton::Left, {} });
    view.OnMouseMove(MouseEvent{ centre.Translated(3.0f, 0.0f), MouseButton::None, {} });
    view.OnMouseRelease(MouseEvent{ centre.Translated(3.0f, 0.0f), MouseButton::Left, {} });

    EXPECT_EQ(view.Camera().Pose().azimuth, azimuth);
}

TEST_F(StageViewTest, ADragIsNotAClick)
{
    Drag(BoxCentre(), BoxCentre().Translated(40.0f, 0.0f));

    EXPECT_FALSE(view.Selection().Valid());
}

TEST_F(StageViewTest, AClickOnTheBackgroundClearsTheSelection)
{
    EXPECT_CALL(selections, Call(box));
    view.SetSelection(box);

    EXPECT_CALL(picks, Call(std::optional<PickResult>{}));
    EXPECT_CALL(selections, Call(NodeId{}));
    Click(Point{ 5.0f, 5.0f });

    EXPECT_FALSE(view.Selection().Valid());
}

TEST_F(StageViewTest, ReselectingTheSameNodeIsSilent)
{
    EXPECT_CALL(selections, Call(box)).Times(1);

    view.SetSelection(box);
    view.SetSelection(box);
}

TEST_F(StageViewTest, TheWheelZoomsIn)
{
    const auto before = view.Camera().Pose().distance;

    view.OnWheel(ui::WheelEvent{ Point{}, 120.0f, {} });

    EXPECT_LT(view.Camera().Pose().distance, before);
    EXPECT_EQ(host.InvalidateCount(), 1u);
}

TEST_F(StageViewTest, EscapeClearsTheSelectionAndHomeResetsTheCamera)
{
    EXPECT_CALL(selections, Call(box));
    view.SetSelection(box);
    Drag(Point{ 100.0f, 100.0f }, Point{ 200.0f, 150.0f });

    EXPECT_CALL(selections, Call(NodeId{}));
    view.OnKeyPress(ui::KeyEvent{ ui::Key::Escape, 0, {} });
    view.OnKeyPress(ui::KeyEvent{ ui::Key::Home, 0, {} });

    EXPECT_FALSE(view.Selection().Valid());
    EXPECT_EQ(view.Camera().Pose().azimuth, ui::scene::CameraPose{}.azimuth);
}

TEST_F(StageViewTest, ADoubleClickFramesTheScene)
{
    view.OnMouseDoubleClick(MouseEvent{ Point{}, MouseButton::Left, {} });

    const auto lookAt = view.Camera().Pose().lookAt;

    EXPECT_NEAR(lookAt.x, 0.0f, 1e-5f);
    EXPECT_NEAR(lookAt.z, 0.3f, 1e-5f);
    EXPECT_EQ(host.InvalidateCount(), 1u);
}

TEST_F(StageViewTest, TheSelectionIsTintedOnTheNextPaint)
{
    canvas.Clear();
    view.Paint(canvas, bounds);
    const auto plain = canvas.Commands();

    EXPECT_CALL(selections, Call(box));
    view.SetSelection(box);
    canvas.Clear();
    view.Paint(canvas, bounds);

    std::size_t differing{ 0 };

    for (std::size_t i = 0; i < std::min(plain.size(), canvas.Commands().size()); ++i)
        if (plain[i].kind == CommandKind::SetBrush && plain[i].brush != canvas.Commands()[i].brush)
            ++differing;

    EXPECT_GT(differing, 0u);
}

TEST_F(StageViewTest, OptionsAreAppliedAndRepaint)
{
    auto options = view.Options();
    options.showGrid = false;
    view.SetOptions(options);

    EXPECT_FALSE(view.Options().showGrid);
    EXPECT_EQ(host.InvalidateCount(), 1u);
}

TEST_F(StageViewTest, TheMinimumSizeComesFromTheConfig)
{
    EXPECT_NEAR(view.MinimumSize().width, 200.0f, 1e-6f);
    EXPECT_NEAR(view.MinimumSize().height, 150.0f, 1e-6f);
}

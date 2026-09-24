#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/stage/StageBuilder.hpp"
#include "ui/stage/StageRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::Command;
    using ui::backend::recording::CommandKind;
    using ui::scene::CameraPose;
    using ui::scene::ViewFrame;
    using ui::stage::MaterialId;
    using ui::stage::NodeId;
    using ui::stage::PickResult;
    using ui::stage::RenderOptions;
    using ui::stage::Stage;
    using ui::stage::StageRenderer;
    using ui::stage::Transform3;
    using ui::stage::Vector3;

    constexpr ui::Rect viewport{ 0.0f, 0.0f, 800.0f, 600.0f };

    class StageRendererTest
        : public ::testing::Test
    {
    protected:
        StageRendererTest()
        {
            ui::theme::SetCurrent(ui::theme::Light());
            options.showGrid = false;
            options.showOriginTriad = false;
        }

        ~StageRendererTest() override
        {
            ui::theme::SetCurrent(ui::theme::Light());
        }

        void Render(const CameraPose& pose = CameraPose{})
        {
            canvas.Clear();
            renderer.Render(canvas, stage, ViewFrame{ pose, viewport, {} }, options);
        }

        [[nodiscard]] std::vector<ui::Color> Fills() const
        {
            std::vector<ui::Color> fills;

            for (const auto& command : canvas.Commands())
                if (command.kind == CommandKind::SetBrush && command.brush.color.alpha != 0)
                    fills.push_back(command.brush.color);

            return fills;
        }

        [[nodiscard]] std::vector<std::size_t> IndicesOf(CommandKind kind) const
        {
            std::vector<std::size_t> indices;

            for (std::size_t i = 0; i < canvas.Commands().size(); ++i)
                if (canvas.Commands()[i].kind == kind)
                    indices.push_back(i);

            return indices;
        }

        [[nodiscard]] ui::Point Projected(Vector3 world, const CameraPose& pose = CameraPose{}) const
        {
            return ViewFrame{ pose, viewport, {} }.Project(world);
        }

        MaterialId Flat(ui::Color color)
        {
            auto material = ui::stage::materials::Matte(color);
            material.ambient = 1.0f;
            material.diffuse = 0.0f;
            return stage.AddMaterial(material);
        }

        Stage stage;
        NodeId base{ stage.Graph().AddFrame(NodeId{}) };
        StageRenderer renderer;
        RenderOptions options;
        ui::backend::recording::RecordingCanvas canvas;
    };
}

TEST_F(StageRendererTest, TheBackgroundIsFilledFirstWithTheSceneRole)
{
    Render();

    const auto fills = IndicesOf(CommandKind::FillRect);

    ASSERT_FALSE(fills.empty());
    EXPECT_EQ(canvas.Commands()[fills.front()].color, ui::theme::Light().Get(ui::theme::ColorRole::SceneBackground));
}

TEST_F(StageRendererTest, ABoxSeenFromAboveACornerShowsThreeFaces)
{
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, Flat(ui::Color::Rgb(0x808080)));

    Render();

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 3u);
    EXPECT_EQ(renderer.DrawnFaces(), 3u);
}

TEST_F(StageRendererTest, AHiddenPartDrawsNothing)
{
    const auto part = ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, Flat(ui::Color::Rgb(0x808080)));
    stage.FindPart(part)->visible = false;

    Render();

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 0u);
}

TEST_F(StageRendererTest, TheFartherBoxIsPaintedFirst)
{
    const CameraPose pose{};
    const auto away = ui::scene::Normalized(Vector3{ -std::cos(pose.azimuth), -std::sin(pose.azimuth), 0.0f });

    ui::stage::AddBox(stage, base, Vector3{ 0.3f, 0.3f, 0.3f }, Flat(ui::Color::Rgb(0x0000FF)));
    ui::stage::AddBox(stage, base, Vector3{ 0.3f, 0.3f, 0.3f }, Flat(ui::Color::Rgb(0xFF0000)), Transform3::Translation(away * 2.0f));

    Render(pose);

    const auto fills = Fills();

    ASSERT_EQ(fills.size(), 2u);
    EXPECT_EQ(fills.front(), ui::Color::Rgb(0xFF0000));
    EXPECT_EQ(fills.back(), ui::Color::Rgb(0x0000FF));
}

TEST_F(StageRendererTest, AFaceTurnedToTheKeyLightIsBrighter)
{
    stage.Lights().towardsKey = Vector3{ 0.0f, 0.0f, 1.0f };
    stage.Lights().fill = 0.0f;
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, stage.AddMaterial(ui::stage::materials::Matte(ui::colors::white)));

    Render();

    const auto fills = Fills();
    const auto [dimmest, brightest] = std::minmax_element(fills.begin(), fills.end(), [](ui::Color a, ui::Color b)
        {
            return a.red < b.red;
        });

    // Two side faces shade identically, and a repeated brush is not set twice.
    ASSERT_EQ(fills.size(), 2u);
    EXPECT_EQ(brightest->red, 255);
    EXPECT_NEAR(dimmest->red, 0.35f * 255.0f, 1.0f);
}

TEST_F(StageRendererTest, AHeadlightLightsWhateverFacesTheCamera)
{
    stage.Lights().headlight = true;
    stage.Lights().fill = 0.0f;
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, stage.AddMaterial(ui::stage::materials::Matte(ui::colors::white)));

    Render(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    const auto fills = Fills();

    ASSERT_FALSE(fills.empty());
    EXPECT_EQ(fills.back().red, 255);
}

TEST_F(StageRendererTest, AThemedMaterialFollowsTheActiveTheme)
{
    auto material = ui::stage::materials::Themed(ui::theme::ColorRole::SceneSurface);
    material.ambient = 1.0f;
    material.diffuse = 0.0f;
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, stage.AddMaterial(material));

    Render();
    EXPECT_EQ(Fills().front(), ui::theme::Light().Get(ui::theme::ColorRole::SceneSurface));

    ui::theme::SetCurrent(ui::theme::Instrument());
    Render();
    EXPECT_EQ(Fills().front(), ui::theme::Instrument().Get(ui::theme::ColorRole::SceneSurface));
}

TEST_F(StageRendererTest, TranslucentPartsKeepTheirBackFaces)
{
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, stage.AddMaterial(ui::stage::materials::Translucent(ui::colors::white)));

    Render();

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 6u);
    EXPECT_LT(Fills().front().alpha, 255);
}

TEST_F(StageRendererTest, AnOutlinedBoxStrokesEveryFaceWithTheEdgePen)
{
    auto material = ui::stage::materials::Matte(ui::colors::white);
    material.outline = true;
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, stage.AddMaterial(material));

    Render();

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 0u);
    EXPECT_EQ(canvas.CurrentPen().color, ui::theme::Light().Get(ui::theme::ColorRole::SceneEdge));
}

TEST_F(StageRendererTest, AnOutlinedCylinderDrawsOnlyItsRims)
{
    auto material = ui::stage::materials::Matte(ui::colors::white);
    material.outline = true;
    ui::stage::AddCylinder(stage, base, 0.3f, 0.6f, stage.AddMaterial(material));

    Render();

    EXPECT_GT(canvas.CountOf(CommandKind::DrawLine), 0u);
    EXPECT_LE(canvas.CountOf(CommandKind::DrawLine), 2u * ui::stage::Tessellation{}.segments);
}

TEST_F(StageRendererTest, AWireframeHasNoFill)
{
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, stage.AddMaterial(ui::stage::materials::Wireframe(ui::colors::white)));

    Render();

    EXPECT_TRUE(Fills().empty());
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolygon), 3u);
}

TEST_F(StageRendererTest, AnEyeInsideTheSceneProducesOnlyFiniteBoundedGeometry)
{
    ui::stage::AddBox(stage, base, Vector3{ 4.0f, 4.0f, 4.0f }, stage.AddMaterial(ui::stage::materials::Translucent(ui::colors::white)));
    ui::stage::AddSphere(stage, base, 0.2f, Flat(ui::colors::white), Transform3::Translation(Vector3{ -0.5f, 0.0f, 0.0f }));
    options.showGrid = true;

    Render(CameraPose{ 0.3f, 0.2f, 0.25f, Vector3{} });

    EXPECT_GT(canvas.CountOf(CommandKind::DrawPolygon), 0u);

    for (const auto& command : canvas.Commands())
        for (const auto& point : command.points)
        {
            ASSERT_TRUE(std::isfinite(point.x) && std::isfinite(point.y));
            EXPECT_LT(std::abs(point.x), 1e6f);
            EXPECT_LT(std::abs(point.y), 1e6f);
        }
}

TEST_F(StageRendererTest, ALongTrailIsSplitIntoJoinedChunks)
{
    const auto trail = stage.AddTrail(64);

    for (auto i = 0; i < 40; ++i)
        stage.FindTrail(trail)->Push(Vector3{ 0.0f, -1.0f + 0.05f * static_cast<float>(i), 0.3f });

    Render();

    const auto polylines = IndicesOf(CommandKind::DrawPolyline);

    ASSERT_EQ(polylines.size(), 3u);

    std::size_t points{ 0 };

    for (const auto index : polylines)
        points += canvas.Commands()[index].points.size();

    EXPECT_EQ(points, 40u + 2u);
    EXPECT_EQ(canvas.CurrentPen().color, ui::theme::Light().Get(ui::theme::ColorRole::SceneTrail));
}

TEST_F(StageRendererTest, ATrailPassingBehindTheEyeIsCutThere)
{
    const auto trail = stage.AddTrail(8);
    const CameraPose pose{ 0.0f, 0.0f, 3.0f, Vector3{} };

    stage.FindTrail(trail)->Push(Vector3{ 0.0f, -0.5f, 0.0f });
    stage.FindTrail(trail)->Push(Vector3{ 10.0f, 0.0f, 0.0f });
    stage.FindTrail(trail)->Push(Vector3{ 0.0f, 0.5f, 0.0f });

    Render(pose);

    ASSERT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 2u);

    for (const auto index : IndicesOf(CommandKind::DrawPolyline))
        for (const auto& point : canvas.Commands()[index].points)
            EXPECT_LT(std::abs(point.y), 1e6f);
}

TEST_F(StageRendererTest, AnOnTopTrailIsDrawnAfterEverySolid)
{
    ui::stage::AddBox(stage, base, Vector3{ 1.0f, 1.0f, 1.0f }, Flat(ui::colors::white));

    ui::stage::TrailStyle style;
    style.drawOnTop = true;
    const auto trail = stage.AddTrail(4, style);
    stage.FindTrail(trail)->Push(Vector3{ -3.0f, 0.0f, 0.0f });
    stage.FindTrail(trail)->Push(Vector3{ -3.0f, 0.5f, 0.0f });

    Render();

    ASSERT_EQ(IndicesOf(CommandKind::DrawPolyline).size(), 1u);
    EXPECT_GT(IndicesOf(CommandKind::DrawPolyline).front(), IndicesOf(CommandKind::DrawPolygon).back());
}

TEST_F(StageRendererTest, LabelsComeLastAndFollowTheirNode)
{
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, Flat(ui::colors::white));
    const auto slide = stage.Graph().AddPrismaticJoint(base, Transform3{}, Vector3{ 0.0f, 0.0f, 1.0f });
    stage.AddLabel(slide, Vector3{}, "TCP");
    stage.Graph().SetJointValue(slide, 0.5f);

    Render();

    const auto& commands = canvas.Commands();
    ASSERT_GE(commands.size(), 2u);
    EXPECT_EQ(commands.back().kind, CommandKind::Restore);

    const auto& last = commands[commands.size() - 2];
    const auto expected = Projected(Vector3{ 0.0f, 0.0f, 0.5f });

    ASSERT_EQ(last.kind, CommandKind::DrawText);
    EXPECT_EQ(last.text, "TCP");
    EXPECT_NEAR(last.from.x, expected.x + 6.0f, 1e-2f);
    EXPECT_NEAR(last.from.y, expected.y - 6.0f, 1e-2f);
}

TEST_F(StageRendererTest, ALabelBehindTheEyeIsSkipped)
{
    stage.AddLabel(base, Vector3{ 10.0f, 0.0f, 0.0f }, "behind");

    Render(CameraPose{ 0.0f, 0.0f, 3.0f, Vector3{} });

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawText), 0u);
}

TEST_F(StageRendererTest, AFrameMarkerIsThreeAxisColouredLines)
{
    stage.AddFrameMarker(base, 0.2f);

    Render();

    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 3u);
    EXPECT_EQ(canvas.CurrentPen().color, ui::theme::Light().Get(ui::theme::ColorRole::SceneAxisZ));
}

TEST_F(StageRendererTest, PickingFindsTheNearestPartUnderTheCursor)
{
    const CameraPose pose{ 0.0f, 0.0f, 3.0f, Vector3{} };
    const auto nearNode = stage.Graph().AddFrame(base, Transform3::Translation(Vector3{ 0.5f, 0.0f, 0.0f }));
    const auto farPart = ui::stage::AddBox(stage, base, Vector3{ 0.3f, 0.3f, 0.3f }, Flat(ui::colors::white));
    const auto nearPart = ui::stage::AddBox(stage, nearNode, Vector3{ 0.1f, 0.1f, 0.1f }, Flat(ui::colors::white));

    Render(pose);

    EXPECT_EQ(renderer.Pick(Projected(Vector3{}, pose)), (PickResult{ nearNode, nearPart }));
    EXPECT_EQ(renderer.Pick(Projected(Vector3{ 0.0f, 0.12f, 0.0f }, pose)), (PickResult{ base, farPart }));
    EXPECT_FALSE(renderer.Pick(ui::Point{ 5.0f, 5.0f }).has_value());
}

TEST_F(StageRendererTest, AnUnpickablePartIsSeenThrough)
{
    const CameraPose pose{ 0.0f, 0.0f, 3.0f, Vector3{} };
    const auto part = ui::stage::AddBox(stage, base, Vector3{ 0.3f, 0.3f, 0.3f }, Flat(ui::colors::white));
    stage.FindPart(part)->pickable = false;

    Render(pose);

    EXPECT_FALSE(renderer.Pick(Projected(Vector3{}, pose)).has_value());
}

TEST_F(StageRendererTest, TheSelectedNodeIsTinted)
{
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, Flat(ui::Color::Rgb(0x000000)));

    Render();
    const auto plain = Fills().front();

    options.selection = base;
    Render();
    const auto selected = Fills().front();

    EXPECT_NE(plain, selected);
    EXPECT_GT(selected.red, plain.red);
}

TEST_F(StageRendererTest, TheGridAndOriginTriadAreOptional)
{
    options.showGrid = true;
    options.showOriginTriad = true;
    Render();
    const auto withGizmos = canvas.CountOf(CommandKind::DrawLine);

    options.showGrid = false;
    options.showOriginTriad = false;
    Render();

    EXPECT_GT(withGizmos, 0u);
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawLine), 0u);
}

TEST_F(StageRendererTest, RenderingLeavesTheCanvasStateBalanced)
{
    ui::stage::AddBox(stage, base, Vector3{ 0.5f, 0.5f, 0.5f }, Flat(ui::colors::white));

    Render();

    EXPECT_EQ(canvas.CountOf(CommandKind::Save), canvas.CountOf(CommandKind::Restore));
}

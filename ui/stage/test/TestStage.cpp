#include "ui/stage/Stage.hpp"
#include "ui/stage/StageBuilder.hpp"
#include <gmock/gmock.h>
#include <string>

namespace
{
    using ui::stage::LabelId;
    using ui::stage::MaterialId;
    using ui::stage::MeshId;
    using ui::stage::NodeId;
    using ui::stage::Part;
    using ui::stage::Stage;
    using ui::stage::Trail;
    using ui::stage::Transform3;
    using ui::stage::Vector3;

    class StageTest
        : public ::testing::Test
    {
    protected:
        static void ExpectNear(Vector3 actual, Vector3 expected, float tolerance = 1e-5f)
        {
            EXPECT_NEAR(actual.x, expected.x, tolerance);
            EXPECT_NEAR(actual.y, expected.y, tolerance);
            EXPECT_NEAR(actual.z, expected.z, tolerance);
        }

        static Part PartOf(NodeId node, MeshId mesh, MaterialId material)
        {
            Part part;
            part.node = node;
            part.mesh = mesh;
            part.material = material;
            return part;
        }

        Stage stage;
        NodeId base{ stage.Graph().AddFrame(NodeId{}) };
        MaterialId grey{ stage.AddMaterial(ui::stage::materials::Matte(ui::Color::Rgb(0x808080))) };
    };
}

TEST_F(StageTest, SharedMeshesAreBuiltOnce)
{
    const auto first = stage.CylinderMesh();
    const auto second = stage.CylinderMesh();

    EXPECT_EQ(first, second);
    EXPECT_EQ(stage.Meshes().size(), 1u);
}

TEST_F(StageTest, APartNeedsAKnownNodeMeshAndMaterial)
{
    const auto box = stage.BoxMesh();

    EXPECT_TRUE(stage.AddPart(PartOf(base, box, grey)).Valid());
    EXPECT_FALSE(stage.AddPart(PartOf(NodeId{ 9 }, box, grey)).Valid());
    EXPECT_FALSE(stage.AddPart(PartOf(base, MeshId{ 9 }, grey)).Valid());
    EXPECT_FALSE(stage.AddPart(PartOf(base, box, MaterialId{ 9 })).Valid());
    EXPECT_EQ(stage.Parts().size(), 1u);
}

TEST_F(StageTest, ABoxIsScaledToItsSize)
{
    const auto id = ui::stage::AddBox(stage, base, Vector3{ 0.2f, 0.4f, 0.6f }, grey);
    const auto bounds = stage.Bounds();

    ASSERT_TRUE(id.Valid());
    ASSERT_TRUE(bounds.has_value());
    ExpectNear(bounds->centre, Vector3{});
    EXPECT_NEAR(bounds->radius, 0.5f * ui::scene::Length(Vector3{ 0.2f, 0.4f, 0.6f }), 1e-5f);
}

TEST_F(StageTest, ALinkSpansItsTwoEndpoints)
{
    ui::stage::AddLink(stage, base, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.0f, 1.0f, 0.0f }, 0.05f, grey);

    const auto& part = stage.Parts().front();
    const auto tip = part.offset.Apply(Vector3{ 0.0f, 0.0f, part.scale.z });

    ExpectNear(tip, Vector3{ 0.0f, 1.0f, 0.0f });
    EXPECT_NEAR(part.scale.x, 0.05f, 1e-6f);
}

TEST_F(StageTest, AJointHousingIsCentredOnTheJointAxis)
{
    const auto joint = stage.Graph().AddRevoluteJoint(base, Transform3{}, Vector3{ 1.0f, 0.0f, 0.0f });

    ui::stage::AddJointHousing(stage, joint, 0.1f, 0.2f, grey);

    const auto bounds = stage.Bounds();

    ASSERT_TRUE(bounds.has_value());
    ExpectNear(bounds->centre, Vector3{}, 1e-4f);
}

TEST_F(StageTest, BoundsFollowTheJoints)
{
    const auto slide = stage.Graph().AddPrismaticJoint(base, Transform3{}, Vector3{ 1.0f, 0.0f, 0.0f });
    ui::stage::AddSphere(stage, slide, 0.1f, grey);

    stage.Graph().SetJointValue(slide, 2.0f);

    ExpectNear(stage.Bounds()->centre, Vector3{ 2.0f, 0.0f, 0.0f }, 1e-4f);
}

TEST_F(StageTest, AnEmptyOrHiddenStageHasNoBounds)
{
    EXPECT_FALSE(stage.Bounds().has_value());

    const auto id = ui::stage::AddSphere(stage, base, 0.1f, grey);
    stage.FindPart(id)->visible = false;

    EXPECT_FALSE(stage.Bounds().has_value());
}

TEST_F(StageTest, CapacityCountsEveryPartInstance)
{
    ui::stage::AddBox(stage, base, Vector3{ 1.0f, 1.0f, 1.0f }, grey);
    ui::stage::AddBox(stage, base, Vector3{ 1.0f, 1.0f, 1.0f }, grey);
    stage.AddTrail(100);

    const auto capacity = stage.Capacity();

    EXPECT_EQ(capacity.vertices, 16u);
    EXPECT_EQ(capacity.faces, 12u);
    EXPECT_EQ(capacity.trailPoints, 100u);
}

TEST_F(StageTest, LabelsAreTruncatedToTheirFixedCapacity)
{
    const auto label = stage.AddLabel(base, Vector3{}, "TCP");

    ASSERT_TRUE(label.Valid());
    EXPECT_EQ(stage.Labels().front().Text(), "TCP");

    const std::string longText(40, 'x');
    ASSERT_TRUE(stage.SetLabelText(label, longText));
    EXPECT_EQ(stage.Labels().front().Text().size(), ui::stage::Label::capacity);
    EXPECT_FALSE(stage.SetLabelText(LabelId{ 7 }, "nope"));
}

TEST_F(StageTest, AMaterialCanBeRecolouredInPlace)
{
    stage.FindMaterial(grey)->color = ui::Color::Rgb(0xFF0000);

    EXPECT_EQ(stage.Materials().front().color, ui::Color::Rgb(0xFF0000));
    EXPECT_EQ(stage.FindMaterial(MaterialId{ 5 }), nullptr);
}

TEST_F(StageTest, FrameMarkersAndTrailsNeedAKnownNode)
{
    stage.AddFrameMarker(base);
    stage.AddFrameMarker(NodeId{ 12 });

    EXPECT_EQ(stage.FrameMarkers().size(), 1u);
    EXPECT_FALSE(stage.AddTrail(10, {}, NodeId{ 12 }).Valid());
}

TEST_F(StageTest, ATrailKeepsTheNewestPointsOnceFull)
{
    Trail trail{ 3 };

    for (auto i = 0; i < 5; ++i)
        trail.Push(Vector3{ static_cast<float>(i), 0.0f, 0.0f });

    ASSERT_EQ(trail.Size(), 3u);
    EXPECT_NEAR(trail.At(0).x, 2.0f, 1e-6f);
    EXPECT_NEAR(trail.At(2).x, 4.0f, 1e-6f);

    trail.Clear();
    EXPECT_EQ(trail.Size(), 0u);
}

TEST_F(StageTest, AZeroCapacityTrailIgnoresPoints)
{
    Trail trail{ 0 };
    trail.Push(Vector3{});

    EXPECT_EQ(trail.Size(), 0u);
}

TEST_F(StageTest, MaterialPresetsDifferInFinish)
{
    const auto metal = ui::stage::materials::Steel();
    const auto matte = ui::stage::materials::Matte(ui::Color::Rgb(0x8A9099));
    const auto glass = ui::stage::materials::Translucent(ui::Color::Rgb(0x88CCFF));

    EXPECT_GT(metal.specular, matte.specular);
    EXPECT_TRUE(metal.outline);
    EXPECT_FALSE(matte.role.has_value());
    EXPECT_LT(glass.opacity, 255);
    EXPECT_TRUE(glass.doubleSided);
    EXPECT_TRUE(ui::stage::materials::Wireframe(ui::colors::white).wireframe);
}

#include "ui/stage/SceneGraph.hpp"
#include <array>
#include <cmath>
#include <gmock/gmock.h>
#include <numbers>

namespace
{
    using ui::stage::JointType;
    using ui::stage::NodeId;
    using ui::stage::SceneGraph;
    using ui::stage::Transform3;
    using ui::stage::Vector3;

    constexpr float halfPi{ std::numbers::pi_v<float> / 2.0f };
    constexpr Vector3 unitZ{ 0.0f, 0.0f, 1.0f };

    class SceneGraphTest
        : public ::testing::Test
    {
    protected:
        static void ExpectNear(Vector3 actual, Vector3 expected, float tolerance = 1e-5f)
        {
            EXPECT_NEAR(actual.x, expected.x, tolerance);
            EXPECT_NEAR(actual.y, expected.y, tolerance);
            EXPECT_NEAR(actual.z, expected.z, tolerance);
        }

        // A planar two-link arm in XY: shoulder at the origin, elbow 0.5 out, tool 0.3 beyond.
        SceneGraphTest()
        {
            shoulder = graph.AddRevoluteJoint(NodeId{}, Transform3{}, unitZ);
            elbow = graph.AddRevoluteJoint(shoulder, Transform3::Translation(Vector3{ 0.5f, 0.0f, 0.0f }), unitZ);
            tool = graph.AddFrame(elbow, Transform3::Translation(Vector3{ 0.3f, 0.0f, 0.0f }));
        }

        SceneGraph graph;
        NodeId shoulder;
        NodeId elbow;
        NodeId tool;
    };
}

TEST_F(SceneGraphTest, AtZeroTheArmLiesAlongX)
{
    ExpectNear(graph.World(tool).translation, Vector3{ 0.8f, 0.0f, 0.0f });
}

TEST_F(SceneGraphTest, TheToolFollowsTheAnalyticForwardKinematics)
{
    const std::array<float, 2> q{ 0.4f, -1.1f };

    ASSERT_TRUE(graph.SetJointValues(q));

    const Vector3 expected{ 0.5f * std::cos(q[0]) + 0.3f * std::cos(q[0] + q[1]),
        0.5f * std::sin(q[0]) + 0.3f * std::sin(q[0] + q[1]), 0.0f };

    ExpectNear(graph.World(tool).translation, expected);
}

TEST_F(SceneGraphTest, ChildrenInheritTheirParentsRotation)
{
    graph.SetJointValue(shoulder, halfPi);

    ExpectNear(graph.World(elbow).translation, Vector3{ 0.0f, 0.5f, 0.0f });
    ExpectNear(graph.World(tool).ApplyDirection(Vector3{ 1.0f, 0.0f, 0.0f }), Vector3{ 0.0f, 1.0f, 0.0f });
}

TEST_F(SceneGraphTest, APrismaticJointSlidesAlongItsNormalisedAxis)
{
    const auto slide = graph.AddPrismaticJoint(NodeId{}, Transform3::Translation(Vector3{ 0.0f, 0.0f, 1.0f }), Vector3{ 0.0f, 3.0f, 0.0f });

    graph.SetJointValue(slide, 0.25f);

    ExpectNear(graph.World(slide).translation, Vector3{ 0.0f, 0.25f, 1.0f });
}

TEST_F(SceneGraphTest, JointValuesAreClampedToTheLimits)
{
    const auto limited = graph.AddRevoluteJoint(NodeId{}, Transform3{}, unitZ, -0.5f, 0.75f);

    EXPECT_NEAR(*graph.SetJointValue(limited, 2.0f), 0.75f, 1e-6f);
    EXPECT_NEAR(*graph.SetJointValue(limited, -2.0f), -0.5f, 1e-6f);
    EXPECT_NEAR(*graph.JointValue(limited), -0.5f, 1e-6f);
}

TEST_F(SceneGraphTest, ARangeExcludingZeroStartsAtTheNearestLimit)
{
    const auto raised = graph.AddPrismaticJoint(NodeId{}, Transform3{}, unitZ, 0.1f, 0.4f);

    EXPECT_NEAR(*graph.JointValue(raised), 0.1f, 1e-6f);
}

TEST_F(SceneGraphTest, AFixedFrameHasNoJointValue)
{
    EXPECT_FALSE(graph.SetJointValue(tool, 1.0f).has_value());
    EXPECT_FALSE(graph.JointValue(tool).has_value());
    EXPECT_EQ(graph.Joint(tool).type, JointType::Fixed);
}

TEST_F(SceneGraphTest, JointsAreNumberedInCreationOrder)
{
    ASSERT_EQ(graph.JointCount(), 2u);
    EXPECT_EQ(graph.JointAt(0), shoulder);
    EXPECT_EQ(graph.JointAt(1), elbow);
    EXPECT_FALSE(graph.JointAt(2).Valid());
}

TEST_F(SceneGraphTest, AMismatchedJointVectorIsRefused)
{
    const std::array<float, 3> tooMany{ 1.0f, 2.0f, 3.0f };

    EXPECT_FALSE(graph.SetJointValues(tooMany));
    EXPECT_NEAR(*graph.JointValue(shoulder), 0.0f, 1e-6f);
}

TEST_F(SceneGraphTest, AnUnknownParentIsRefused)
{
    EXPECT_FALSE(graph.AddFrame(NodeId{ 99 }).Valid());
    EXPECT_EQ(graph.Size(), 3u);
}

TEST_F(SceneGraphTest, UnknownNodesReadAsTheIdentity)
{
    EXPECT_EQ(graph.World(NodeId{ 42 }), Transform3{});
    EXPECT_FALSE(graph.Parent(NodeId{ 42 }).Valid());
    EXPECT_FALSE(graph.SetLocal(NodeId{ 42 }, Transform3{}));
}

TEST_F(SceneGraphTest, SettingALocalTransformMovesTheSubtree)
{
    ASSERT_TRUE(graph.SetLocal(shoulder, Transform3::Translation(Vector3{ 0.0f, 0.0f, 1.0f })));

    ExpectNear(graph.World(tool).translation, Vector3{ 0.8f, 0.0f, 1.0f });
    EXPECT_EQ(graph.Parent(tool), elbow);
}

TEST_F(SceneGraphTest, TheRevisionAdvancesOnEveryChange)
{
    const auto before = graph.Revision();

    graph.SetJointValue(shoulder, 0.1f);

    EXPECT_GT(graph.Revision(), before);
}

#pragma once

#include "ui/scene/Transform3.hpp"
#include "ui/stage/Ids.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace ui::stage
{
    using scene::Matrix3;
    using scene::Transform3;
    using scene::Vector3;

    enum class JointType : std::uint8_t
    {
        Fixed,
        Revolute,
        Prismatic
    };

    // Radians for a revolute joint, scene units for a prismatic one. The axis is in the joint's
    // own frame, after its origin transform.
    struct JointSpec
    {
        JointType type{ JointType::Fixed };
        Vector3 axis{ 0.0f, 0.0f, 1.0f };
        float minimum{ -std::numeric_limits<float>::infinity() };
        float maximum{ std::numeric_limits<float>::infinity() };
    };

    // Forward kinematics only. A node's world transform is parent * local * motion(joint, value),
    // and since a parent always precedes its children one linear pass settles the whole tree.
    // Adding nodes allocates; moving joints and reading transforms never does.
    class SceneGraph
    {
    public:
        // An invalid parent attaches to the world. An unknown parent is refused.
        NodeId AddFrame(NodeId parent, const Transform3& local = {});
        NodeId AddJoint(NodeId parent, const Transform3& origin, const JointSpec& joint);
        NodeId AddRevoluteJoint(NodeId parent, const Transform3& origin, Vector3 axis,
            float minimum = -std::numeric_limits<float>::infinity(), float maximum = std::numeric_limits<float>::infinity());
        NodeId AddPrismaticJoint(NodeId parent, const Transform3& origin, Vector3 axis,
            float minimum = -std::numeric_limits<float>::infinity(), float maximum = std::numeric_limits<float>::infinity());

        // Returns the value actually applied after clamping to the limits; nothing for an unknown
        // node or a fixed one.
        std::optional<float> SetJointValue(NodeId node, float value);
        [[nodiscard]] std::optional<float> JointValue(NodeId node) const;

        // In joint-creation order, so a robot's q vector maps straight across. Refused unless the
        // sizes match.
        bool SetJointValues(std::span<const float> values);
        [[nodiscard]] std::size_t JointCount() const;
        [[nodiscard]] NodeId JointAt(std::size_t index) const;
        [[nodiscard]] const JointSpec& Joint(NodeId node) const;

        // For consumers that run their own kinematics and just want the frames drawn.
        bool SetLocal(NodeId node, const Transform3& local);
        [[nodiscard]] const Transform3& Local(NodeId node) const;

        [[nodiscard]] const Transform3& World(NodeId node) const;
        [[nodiscard]] NodeId Parent(NodeId node) const;
        [[nodiscard]] bool Contains(NodeId node) const;
        [[nodiscard]] std::size_t Size() const;

        // Incremented whenever any world transform may have changed; cheap change detection.
        [[nodiscard]] std::uint32_t Revision() const;

    private:
        struct Node
        {
            NodeId parent;
            Transform3 local;
            JointSpec joint;
            float value{ 0.0f };
        };

        void Refresh() const;

        std::vector<Node> nodes;
        std::vector<NodeId> joints;
        mutable std::vector<Transform3> worlds;
        mutable bool dirty{ false };
        std::uint32_t revision{ 0 };
    };
}

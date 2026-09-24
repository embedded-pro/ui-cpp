#include "ui/stage/SceneGraph.hpp"
#include "ui/core/Geometry.hpp"

namespace ui::stage
{
    namespace
    {
        constexpr Transform3 identity{};
        constexpr JointSpec fixedJoint{};

        [[nodiscard]] Transform3 Motion(const JointSpec& joint, float value)
        {
            switch (joint.type)
            {
                case JointType::Revolute:
                    return Transform3::Rotation(Matrix3::AxisAngle(joint.axis, value));
                case JointType::Prismatic:
                    return Transform3::Translation(scene::Normalized(joint.axis) * value);
                default:
                    return identity;
            }
        }
    }

    NodeId SceneGraph::AddFrame(NodeId parent, const Transform3& local)
    {
        return AddJoint(parent, local, fixedJoint);
    }

    NodeId SceneGraph::AddJoint(NodeId parent, const Transform3& origin, const JointSpec& joint)
    {
        if ((parent.Valid() && !Contains(parent)) || nodes.size() >= NodeId::invalid)
            return NodeId{};

        const NodeId id{ static_cast<std::uint16_t>(nodes.size()) };
        auto& node = nodes.emplace_back();
        node.parent = parent;
        node.local = origin;
        node.joint = joint;
        node.value = Clamp(0.0f, joint.minimum, joint.maximum);
        worlds.emplace_back();

        if (joint.type != JointType::Fixed)
            joints.push_back(id);

        dirty = true;
        ++revision;
        return id;
    }

    NodeId SceneGraph::AddRevoluteJoint(NodeId parent, const Transform3& origin, Vector3 axis, float minimum, float maximum)
    {
        return AddJoint(parent, origin, JointSpec{ JointType::Revolute, axis, minimum, maximum });
    }

    NodeId SceneGraph::AddPrismaticJoint(NodeId parent, const Transform3& origin, Vector3 axis, float minimum, float maximum)
    {
        return AddJoint(parent, origin, JointSpec{ JointType::Prismatic, axis, minimum, maximum });
    }

    std::optional<float> SceneGraph::SetJointValue(NodeId node, float value)
    {
        if (!Contains(node) || nodes[node.value].joint.type == JointType::Fixed)
            return std::nullopt;

        auto& target = nodes[node.value];
        target.value = Clamp(value, target.joint.minimum, target.joint.maximum);
        dirty = true;
        ++revision;
        return target.value;
    }

    std::optional<float> SceneGraph::JointValue(NodeId node) const
    {
        if (!Contains(node) || nodes[node.value].joint.type == JointType::Fixed)
            return std::nullopt;

        return nodes[node.value].value;
    }

    bool SceneGraph::SetJointValues(std::span<const float> values)
    {
        if (values.size() != joints.size())
            return false;

        for (std::size_t i = 0; i < values.size(); ++i)
            SetJointValue(joints[i], values[i]);

        return true;
    }

    std::size_t SceneGraph::JointCount() const
    {
        return joints.size();
    }

    NodeId SceneGraph::JointAt(std::size_t index) const
    {
        return index < joints.size() ? joints[index] : NodeId{};
    }

    const JointSpec& SceneGraph::Joint(NodeId node) const
    {
        return Contains(node) ? nodes[node.value].joint : fixedJoint;
    }

    bool SceneGraph::SetLocal(NodeId node, const Transform3& local)
    {
        if (!Contains(node))
            return false;

        nodes[node.value].local = local;
        dirty = true;
        ++revision;
        return true;
    }

    const Transform3& SceneGraph::Local(NodeId node) const
    {
        return Contains(node) ? nodes[node.value].local : identity;
    }

    const Transform3& SceneGraph::World(NodeId node) const
    {
        if (!Contains(node))
            return identity;

        Refresh();
        return worlds[node.value];
    }

    NodeId SceneGraph::Parent(NodeId node) const
    {
        return Contains(node) ? nodes[node.value].parent : NodeId{};
    }

    bool SceneGraph::Contains(NodeId node) const
    {
        return node.Valid() && node.value < nodes.size();
    }

    std::size_t SceneGraph::Size() const
    {
        return nodes.size();
    }

    std::uint32_t SceneGraph::Revision() const
    {
        return revision;
    }

    void SceneGraph::Refresh() const
    {
        if (!dirty)
            return;

        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            const auto& node = nodes[i];
            const auto& parentWorld = node.parent.Valid() ? worlds[node.parent.value] : identity;

            worlds[i] = parentWorld * node.local * Motion(node.joint, node.value);
        }

        dirty = false;
    }
}

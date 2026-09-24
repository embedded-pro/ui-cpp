#include "ui/stage/StageBuilder.hpp"

namespace ui::stage
{
    PartId AddBox(Stage& stage, NodeId node, Vector3 size, MaterialId material, const Transform3& offset)
    {
        return AddMeshPart(stage, node, stage.BoxMesh(), material, offset, size);
    }

    PartId AddCylinder(Stage& stage, NodeId node, float radius, float length, MaterialId material, const Transform3& offset)
    {
        return AddMeshPart(stage, node, stage.CylinderMesh(), material, offset, Vector3{ radius, radius, length });
    }

    PartId AddCone(Stage& stage, NodeId node, float radius, float length, MaterialId material, const Transform3& offset)
    {
        return AddMeshPart(stage, node, stage.ConeMesh(), material, offset, Vector3{ radius, radius, length });
    }

    PartId AddSphere(Stage& stage, NodeId node, float radius, MaterialId material, const Transform3& offset)
    {
        return AddMeshPart(stage, node, stage.SphereMesh(), material, offset, Vector3{ radius, radius, radius });
    }

    PartId AddLink(Stage& stage, NodeId node, Vector3 from, Vector3 to, float radius, MaterialId material)
    {
        return AddCylinder(stage, node, radius, scene::Length(to - from), material, Transform3::AlignZ(from, to));
    }

    PartId AddJointHousing(Stage& stage, NodeId joint, float radius, float length, MaterialId material)
    {
        const auto axis = scene::Normalized(stage.Graph().Joint(joint).axis) * (0.5f * length);

        return AddLink(stage, joint, axis * -1.0f, axis, radius, material);
    }

    PartId AddMeshPart(Stage& stage, NodeId node, MeshId mesh, MaterialId material, const Transform3& offset, Vector3 scale)
    {
        return stage.AddPart(Part{ node, mesh, material, offset, scale, true, true });
    }
}

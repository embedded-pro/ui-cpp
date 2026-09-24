#pragma once

#include "ui/stage/Stage.hpp"

namespace ui::stage
{
    // Shorthands over Stage::AddPart using the stage's shared unit meshes. Every offset is in the
    // node's frame.

    // Centred on the offset origin.
    PartId AddBox(Stage& stage, NodeId node, Vector3 size, MaterialId material, const Transform3& offset = {});

    // Standing on the offset origin and reaching `length` along its +Z.
    PartId AddCylinder(Stage& stage, NodeId node, float radius, float length, MaterialId material, const Transform3& offset = {});
    PartId AddCone(Stage& stage, NodeId node, float radius, float length, MaterialId material, const Transform3& offset = {});

    PartId AddSphere(Stage& stage, NodeId node, float radius, MaterialId material, const Transform3& offset = {});

    // A round bar between two points: the usual robot link or machine rail.
    PartId AddLink(Stage& stage, NodeId node, Vector3 from, Vector3 to, float radius, MaterialId material);

    // A drum centred on the joint origin and lying along the joint axis.
    PartId AddJointHousing(Stage& stage, NodeId joint, float radius, float length, MaterialId material);

    PartId AddMeshPart(Stage& stage, NodeId node, MeshId mesh, MaterialId material, const Transform3& offset = {},
        Vector3 scale = Vector3{ 1.0f, 1.0f, 1.0f });
}

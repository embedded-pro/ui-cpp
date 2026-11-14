# 3D stage

`ui/stage` is a composable 3D view for robot arms, CNC machines and similar. You build a scene
from nodes, joints and parts, and it draws through the ordinary 2D `Canvas`. It is Tier 1: no Qt,
no GPU, and it is tested with `RecordingCanvas` like every other widget.

```text
ui/scene   Vector3, Matrix3, Transform3, near-plane clipping, ViewFrame, OrbitCamera, grid, triad
ui/stage   Mesh, primitives, lathe and extrusion, shading, STL import, SceneGraph, Material, Stage,
           StageRenderer, StageView
```

## Building a scene

A scene has four kinds of thing, each named by a small integer handle (`NodeId`, `PartId`, ...):

- **Nodes** are coordinate frames in a tree. A node can be a fixed frame, a revolute joint or a
  prismatic joint. Its world transform is `parent * local * motion(joint value)`.
- **Parts** are meshes hung off a node, each with an `offset` and a per-axis `scale`. One unit
  cylinder therefore serves every link.
- **Materials** are a colour or a theme role, plus ambient, diffuse and specular terms, opacity,
  and an optional outline or wireframe. The `materials::` presets are `Themed`, `Matte`,
  `Plastic`, `Metal`, `Aluminium`, `Steel`, `Translucent` and `Wireframe`.
- **Overlays** draw on top of the solids:
  - trails: a fixed-capacity polyline, such as an end-effector trail or a toolpath
  - labels: text anchored at a node
  - frame markers: an RGB axis triad at a node

```cpp
ui::stage::StageView view;
auto& stage = view.Scene();
auto& graph = stage.Graph();

const auto metal = stage.AddMaterial(ui::stage::materials::Aluminium());
const auto base = graph.AddFrame(ui::stage::NodeId{});
const auto shoulder = graph.AddRevoluteJoint(base, Transform3::Translation({ 0, 0, 0.1f }), { 0, 0, 1 }, -pi, pi);
AddJointHousing(stage, shoulder, 0.08f, 0.2f, metal);
AddLink(stage, shoulder, { 0, 0, 0 }, { 0, 0, 0.45f }, 0.05f, metal);

graph.SetJointValues(q);  // joint-creation order, so a robot's q vector maps straight across
```

`StageBuilder.hpp` provides `AddBox`, `AddCylinder`, `AddCone`, `AddSphere`, `AddLink`,
`AddJointHousing` and `AddMeshPart`. `Transform3::FromDh` builds a classic Denavit–Hartenberg
frame. `Transform3::FromRowMajor` takes a 3×4 pose from a consumer's own kinematics, and
`SceneGraph::SetLocal` applies it. Inverse kinematics stays in the consumer.

Imported geometry comes from `ParseStl` (binary or ASCII, from a byte span) or `LoadStlFile`. STL
carries no units, so set `StlOptions::scale` to `0.001f` for CAD files in millimetres.

### Shapes of your own

Two builders cover most machined parts, both in `Primitives.hpp`:

- **`MakeLathe`** turns a profile of `ProfilePoint{ radius, z }` about Z, from the top pole down to the bottom
  pole. Hubs, flanges, gearboxes, collars and chamfered discs are a handful of points each.
- **`MakeExtrusion`** sweeps a convex outline, counter-clockwise in XY, along Z and caps both ends. Plates,
  tubes, motor bodies and bolt heads come from `RoundedRectangle`, `Stadium` and `RegularPolygon`.

`Mesh::Append` merges a finished part under a rigid transform (`Transform3::AlignZ` puts a part's Z along
any axis), so a whole link can be one mesh; call `Finish` once all parts are in. Give rounded corners and
round ends enough points to stay under the feature angle (30° by default), or they outline as facets.

### Smooth shading for a GPU renderer

The painter flat-shades from face normals. A GPU renderer wants a normal per corner: `Shade(mesh,
creaseAngle)` returns a `ShadedMesh` of triangles with interleaved positions and normals and 32-bit
indices. Each corner averages the faces round its vertex that lie within the crease angle (35° by
default) of its own face, so a cylinder is round along its side and sharp at its rims, and a corner is
duplicated only where a crease splits it. It works the same for an imported STL. It allocates; call it at
setup, not per frame.

### Qt Quick 3D

`UI_BUILD_QUICK3D_BACKEND` adds the `Ui.Quick3D` QML module (`doc/portability.md`). C++ fills a
`MeshCatalog` with `ShadedMesh`es by key; QML draws one with a `ShadedMeshGeometry`:

```qml
import QtQuick3D
import Ui.Quick3D

Model {
    geometry: ShadedMeshGeometry { catalog: scene.meshes; key: "upper-arm/aluminium" }
    materials: PrincipledMaterial { baseColor: "#f2f4f7"; metalness: 0.45; roughness: 0.28 }
}
```

A geometry rebuilds only when its own key changes, and `empty` says whether there is anything to draw.
`PolylineGeometry` draws a line strip through `points` (one pixel wide in Qt Quick 3D).
`CheckGltf(path, triangleBudget)` counts a glTF file's triangles from its accessors and gives a URL for
`RuntimeLoader` only when the file is within the budget, otherwise the reason. A static consumer links
`ui.backend.quick3d_plugin`, writes `Q_IMPORT_QML_PLUGIN(UiQuick3DPlugin)` and adds `:/` to its QML
engine's import paths (Qt 6.4 does not search resources for modules by itself).

`StudioView` is the stage around a model, in Y-up centimetres: the studio light probe (`studio.hdr`,
written by `generate-studio-probe.py`: two soft boxes and a top light), a key light that casts soft
shadows, fill and rim lights, a floor with a grid, MSAA, ambient occlusion and filmic tone mapping. Its
children are scene nodes. `extent` sizes the camera: it orbits a point at `focusHeight` × `extent`
above the floor at `homeDistance` × `extent`. `frame()` goes home, `showPreset(i)` to one of `presets`
(front, side, top and iso by default), with keys F and 1–9 while `keyboardShortcuts` is on; a double tap
frames and a tap emits `picked` with the `View3D` pick result. Colours, light levels and the
floor, grid, shadow and ambient-occlusion switches are properties.

```qml
StudioView {
    extent: 60
    backgroundColor: theme.background
    Model { source: "#Sphere"; materials: PrincipledMaterial { baseColor: "#f28c28" } }
    AxisTriad { length: 8 }
    onPicked: (result) => select(result.objectHit)
}
```

`AxisTriad` draws x, y and z as red, green and blue cylinders of `length` and `thickness`.
`StatusBadge` is a 2-D label to place over the view: it shows `symbol` and `text` on `tone` while `text`
is not empty, and is announced as an alert.

`examples/stage` is a Qt demo of a six-axis arm and a three-axis gantry mill. It is built with
`-DUI_BUILD_QT_BACKEND=ON -DUI_BUILD_EXAMPLES=ON`.

## Interaction

| Input                            | Effect                                        |
|----------------------------------|-----------------------------------------------|
| Left drag                        | Orbit                                         |
| Right, middle or shift-left drag | Pan                                           |
| Wheel                            | Zoom                                          |
| Click (within `clickSlop`)       | Pick, select the node; fires `SetOnPick` hook |
| Double click                     | Frame the whole scene                         |
| Escape / Home                    | Clear selection / reset the camera            |

Camera and selection changes repaint on their own. Joint values and trail points are data: the
host's timer repaints, as it does for `ScopeCore`. Call `StageView::Refresh()` for a one-off change.

## Rendering

This is the painter's algorithm, not a depth buffer. For each frame the renderer:

1. transforms each visible part into view space once;
2. culls back faces (translucent and double-sided materials keep theirs);
3. clips at the near plane;
4. flat-shades each face with a key light, a weak fill from the eye and a Blinn–Phong highlight;
5. sorts the faces far to near by mean depth;
6. fills each face with one `DrawPolygon`.

Outlines draw only a mesh's hard edges, so a cylinder shows its rims and not every facet. Trails
are cut into 16-segment chunks that sort alongside the faces.

**Allocation:**

- Adding things allocates, and belongs to setup.
- Moving joints, pushing trail points, relabelling, recolouring and resizing never allocate.
- The renderer's scratch buffers grow only on the first paint after the stage has grown.
- `ui.stage_allocation_test` counts `operator new` to prove this.

## Limitations

- **Sorting artefacts.** Parts that interpenetrate, faces that overlap cyclically, and a small face
  next to a large one can sort wrongly. Split large planes, tessellate finer, or set `drawOnTop`
  on overlays that must stay visible.
- **Flat shading only.** There are no textures and no shadows; the painter ignores `Shade`, which is for GPU
  backends.
- **Translucency is sorted, not order-independent.**
- **Budget.** A few thousand front faces keep a QPainter backend interactive. Reduce a large STL
  before loading it, and cap it with `StlOptions::maximumTriangles`.
- **Units and axes.** The consumer chooses the units. Z is up. `OrbitLimits` must suit the scale:
  the default distance range of 1 to 15 fits a metre-sized scene.
- **Capacity.** Handles are `uint16_t`, so each kind is capped at 65 535.
- **Picking** reads what the last paint drew.

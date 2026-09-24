#include "ui/stage/StageBuilder.hpp"
#include "ui/stage/StageRenderer.hpp"
#include <atomic>
#include <cstdlib>
#include <gmock/gmock.h>
#include <new>

namespace
{
    std::atomic<std::size_t> allocations{ 0 };
}

void* operator new(std::size_t size)
{
    ++allocations;

    if (auto* memory = std::malloc(size == 0 ? 1 : size))
        return memory;

    throw std::bad_alloc{};
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

namespace
{
    using ui::scene::CameraPose;
    using ui::scene::ViewFrame;
    using ui::stage::NodeId;
    using ui::stage::Transform3;
    using ui::stage::Vector3;

    // RecordingCanvas allocates by design, so the steady-state check paints into nothing.
    class NullCanvas
        : public ui::Canvas
    {
    public:
        void Save() override
        {}

        void Restore() override
        {}

        void SetPen(const ui::Pen&) override
        {}

        void SetBrush(const ui::Brush&) override
        {}

        void SetFont(const ui::FontSpec&) override
        {}

        void SetAntialiasing(bool) override
        {}

        void SetClip(const ui::Rect&) override
        {}

        void ClearClip() override
        {}

        void Translate(ui::Point) override
        {}

        void Rotate(float) override
        {}

        void DrawLine(ui::Point, ui::Point) override
        {}

        void DrawPolyline(std::span<const ui::Point>) override
        {}

        void DrawPolygon(std::span<const ui::Point>) override
        {}

        void DrawRect(const ui::Rect&) override
        {}

        void FillRect(const ui::Rect&, ui::Color) override
        {}

        void DrawRoundedRect(const ui::Rect&, float, float) override
        {}

        void DrawEllipse(ui::Point, float, float) override
        {}

        void DrawText(ui::Point, std::string_view) override
        {}

        void DrawText(const ui::Rect&, ui::TextAlign, ui::TextVerticalAlign, std::string_view) override
        {}

        [[nodiscard]] ui::Size MeasureText(std::string_view) const override
        {
            return ui::Size{};
        }

        [[nodiscard]] float LineHeight() const override
        {
            return 0.0f;
        }
    };

    class StageAllocationTest
        : public ::testing::Test
    {
    protected:
        StageAllocationTest()
        {
            const auto metal = stage.AddMaterial(ui::stage::materials::Aluminium());
            const auto joint = stage.AddMaterial(ui::stage::materials::Themed(ui::theme::ColorRole::SceneJoint));
            const auto base = stage.Graph().AddFrame(NodeId{});

            shoulder = stage.Graph().AddRevoluteJoint(base, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.2f }), Vector3{ 0.0f, 0.0f, 1.0f });
            elbow = stage.Graph().AddRevoluteJoint(shoulder, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.5f }), Vector3{ 0.0f, 1.0f, 0.0f });
            tool = stage.Graph().AddFrame(elbow, Transform3::Translation(Vector3{ 0.4f, 0.0f, 0.0f }));

            ui::stage::AddCylinder(stage, base, 0.15f, 0.2f, metal);
            ui::stage::AddLink(stage, shoulder, Vector3{}, Vector3{ 0.0f, 0.0f, 0.5f }, 0.05f, metal);
            ui::stage::AddJointHousing(stage, elbow, 0.07f, 0.12f, joint);
            ui::stage::AddLink(stage, elbow, Vector3{}, Vector3{ 0.4f, 0.0f, 0.0f }, 0.04f, metal);
            ui::stage::AddSphere(stage, tool, 0.03f, joint);
            stage.AddMaterial(ui::stage::materials::Translucent(ui::colors::white));
            ui::stage::AddBox(stage, base, Vector3{ 1.0f, 1.0f, 0.02f }, stage.AddMaterial(ui::stage::materials::Wireframe(ui::colors::black)));

            label = stage.AddLabel(tool, Vector3{}, "TCP");
            trail = stage.AddTrail(256);
            stage.AddFrameMarker(tool);
        }

        void Paint(const ui::Rect& viewport, const CameraPose& pose)
        {
            renderer.Render(canvas, stage, ViewFrame{ pose, viewport, {} }, options);
        }

        ui::stage::Stage stage;
        NodeId shoulder;
        NodeId elbow;
        NodeId tool;
        ui::stage::LabelId label;
        ui::stage::TrailId trail;
        ui::stage::StageRenderer renderer;
        ui::stage::RenderOptions options;
        NullCanvas canvas;
    };
}

TEST_F(StageAllocationTest, PaintingAfterTheFirstFrameNeverAllocates)
{
    Paint(ui::Rect{ 0.0f, 0.0f, 800.0f, 600.0f }, CameraPose{});

    const auto before = allocations.load();

    for (auto frame = 0; frame < 200; ++frame)
    {
        const auto t = 0.03f * static_cast<float>(frame);

        stage.Graph().SetJointValue(shoulder, t);
        stage.Graph().SetJointValue(elbow, -0.5f * t);
        stage.FindTrail(trail)->Push(stage.Graph().World(tool).translation);
        stage.SetLabelText(label, frame % 2 == 0 ? "even" : "odd");
        options.selection = frame % 3 == 0 ? elbow : NodeId{};

        Paint(ui::Rect{ 0.0f, 0.0f, 400.0f + static_cast<float>(frame), 300.0f }, CameraPose{ t, 0.3f, 0.4f + 0.02f * static_cast<float>(frame), Vector3{} });
        (void)renderer.Pick(ui::Point{ 200.0f, 150.0f });
    }

    EXPECT_EQ(allocations.load(), before);
}

TEST_F(StageAllocationTest, AStageBuiltBeforeTheRendererCostsOneReservation)
{
    const auto before = allocations.load();

    Paint(ui::Rect{ 0.0f, 0.0f, 800.0f, 600.0f }, CameraPose{});
    const auto firstFrame = allocations.load() - before;

    Paint(ui::Rect{ 0.0f, 0.0f, 800.0f, 600.0f }, CameraPose{});

    EXPECT_LE(firstFrame, 4u);
    EXPECT_EQ(allocations.load() - before, firstFrame);
}

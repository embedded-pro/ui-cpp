#include "ui/backend/qt/QtPaintedWidget.hpp"
#include "ui/stage/StageBuilder.hpp"
#include "ui/stage/StageView.hpp"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
    using namespace ui::stage;

    constexpr float pi{ std::numbers::pi_v<float> };

    struct Arm
    {
        NodeId tool;
        TrailId trail;
    };

    struct Gantry
    {
        NodeId x;
        NodeId y;
        NodeId z;
        NodeId tip;
        TrailId cut;
    };

    // A six-axis arm built joint by joint: each joint is a frame, parts hang off the frames.
    Arm BuildArm(Stage& stage)
    {
        auto& graph = stage.Graph();
        const auto metal = stage.AddMaterial(materials::Aluminium());
        const auto housing = stage.AddMaterial(materials::Themed(ui::theme::ColorRole::SceneJoint));
        const auto dark = stage.AddMaterial(materials::Matte(ui::Color::Rgb(0x3A3A3A)));
        const auto guard = stage.AddMaterial(materials::Translucent(ui::Color::Rgb(0x88CCFF), 60));

        const auto base = graph.AddFrame(NodeId{});
        AddCylinder(stage, base, 0.16f, 0.1f, dark);
        AddBox(stage, base, Vector3{ 0.3f, 0.3f, 0.25f }, guard, Transform3::Translation(Vector3{ 0.55f, -0.3f, 0.125f }));

        const auto j1 = graph.AddRevoluteJoint(base, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.1f }), Vector3{ 0.0f, 0.0f, 1.0f }, -pi, pi);
        AddCylinder(stage, j1, 0.11f, 0.18f, metal);
        const auto j2 = graph.AddRevoluteJoint(j1, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.24f }), Vector3{ 0.0f, 1.0f, 0.0f }, -2.0f, 2.0f);
        AddJointHousing(stage, j2, 0.08f, 0.2f, housing);
        AddLink(stage, j2, Vector3{}, Vector3{ 0.0f, 0.0f, 0.45f }, 0.05f, metal);
        const auto j3 = graph.AddRevoluteJoint(j2, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.45f }), Vector3{ 0.0f, 1.0f, 0.0f }, -2.5f, 2.5f);
        AddJointHousing(stage, j3, 0.065f, 0.16f, housing);
        AddLink(stage, j3, Vector3{}, Vector3{ 0.36f, 0.0f, 0.0f }, 0.04f, metal);
        const auto j4 = graph.AddRevoluteJoint(j3, Transform3::Translation(Vector3{ 0.36f, 0.0f, 0.0f }), Vector3{ 1.0f, 0.0f, 0.0f });
        AddJointHousing(stage, j4, 0.045f, 0.09f, housing);
        const auto j5 = graph.AddRevoluteJoint(j4, Transform3::Translation(Vector3{ 0.08f, 0.0f, 0.0f }), Vector3{ 0.0f, 1.0f, 0.0f });
        AddJointHousing(stage, j5, 0.04f, 0.08f, housing);
        const auto j6 = graph.AddRevoluteJoint(j5, Transform3::Translation(Vector3{ 0.06f, 0.0f, 0.0f }), Vector3{ 1.0f, 0.0f, 0.0f });
        AddCylinder(stage, j6, 0.03f, 0.03f, dark, Transform3::AlignZ(Vector3{}, Vector3{ 1.0f, 0.0f, 0.0f }));
        const auto tool = graph.AddFrame(j6, Transform3::Translation(Vector3{ 0.1f, 0.0f, 0.0f }));
        AddCone(stage, j6, 0.025f, 0.1f, metal, Transform3::AlignZ(Vector3{ 0.03f, 0.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f }));

        stage.AddFrameMarker(tool, 0.08f);
        stage.AddLabel(tool, Vector3{}, "TCP");
        return Arm{ tool, stage.AddTrail(240) };
    }

    // A three-axis gantry mill: the bridge slides on X, the carriage on Y, the spindle on Z.
    Gantry BuildGantry(Stage& stage)
    {
        auto& graph = stage.Graph();
        const auto frame = stage.AddMaterial(materials::Steel());
        const auto rail = stage.AddMaterial(materials::Aluminium());
        const auto spindle = stage.AddMaterial(materials::Plastic(ui::Color::Rgb(0x2E86C1)));
        const auto stock = stage.AddMaterial(materials::Matte(ui::Color::Rgb(0xC8A165)));

        const auto base = graph.AddFrame(NodeId{});
        AddBox(stage, base, Vector3{ 1.2f, 0.8f, 0.08f }, frame, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.04f }));
        AddBox(stage, base, Vector3{ 0.5f, 0.35f, 0.06f }, stock, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.11f }));

        for (const auto y : { -0.38f, 0.38f })
            AddLink(stage, base, Vector3{ -0.6f, y, 0.1f }, Vector3{ 0.6f, y, 0.1f }, 0.02f, rail);

        const auto x = graph.AddPrismaticJoint(base, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.1f }), Vector3{ 1.0f, 0.0f, 0.0f }, -0.45f, 0.45f);
        for (const auto y : { -0.38f, 0.38f })
            AddBox(stage, x, Vector3{ 0.08f, 0.06f, 0.4f }, frame, Transform3::Translation(Vector3{ 0.0f, y, 0.2f }));
        AddBox(stage, x, Vector3{ 0.08f, 0.86f, 0.08f }, frame, Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.44f }));

        const auto y = graph.AddPrismaticJoint(x, Transform3::Translation(Vector3{ 0.07f, 0.0f, 0.44f }), Vector3{ 0.0f, 1.0f, 0.0f }, -0.3f, 0.3f);
        AddBox(stage, y, Vector3{ 0.06f, 0.14f, 0.16f }, rail);

        const auto z = graph.AddPrismaticJoint(y, Transform3::Translation(Vector3{ 0.06f, 0.0f, 0.0f }), Vector3{ 0.0f, 0.0f, 1.0f }, -0.28f, 0.0f);
        AddCylinder(stage, z, 0.035f, 0.18f, spindle, Transform3::Translation(Vector3{ 0.0f, 0.0f, -0.1f }));
        AddCone(stage, z, 0.012f, 0.05f, rail, Transform3{ ui::scene::Matrix3::RotationX(pi), Vector3{ 0.0f, 0.0f, -0.1f } });
        const auto tip = graph.AddFrame(z, Transform3::Translation(Vector3{ 0.0f, 0.0f, -0.15f }));

        TrailStyle preview;
        preview.role = ui::theme::ColorRole::Series4;
        preview.width = 1.0f;
        preview.drawOnTop = true;
        const auto path = stage.AddTrail(200, preview, base);

        for (auto i = 0; i < 200; ++i)
        {
            const auto t = 6.0f * pi * static_cast<float>(i) / 199.0f;
            const auto r = 0.03f + 0.12f * static_cast<float>(i) / 199.0f;
            stage.FindTrail(path)->Push(Vector3{ r * std::cos(t), r * std::sin(t), 0.141f });
        }

        stage.AddFrameMarker(tip, 0.06f);
        return Gantry{ x, y, z, tip, stage.AddTrail(400, {}, base) };
    }

    ui::stage::StageViewConfig Config(ui::scene::CameraPose pose)
    {
        StageViewConfig config;
        config.pose = pose;
        config.minimumSize = ui::Size{ 420.0f, 360.0f };
        config.limits.minimumDistance = 0.3f;
        return config;
    }
}

int main(int argc, char** argv)
{
    QApplication application{ argc, argv };
    ui::theme::SetCurrent(ui::theme::Instrument());

    StageView armView{ Config(ui::scene::CameraPose{ 2.3f, 0.35f, 1.6f, Vector3{ 0.15f, 0.15f, 0.35f } }) };
    StageView millView{ Config(ui::scene::CameraPose{ -0.7f, 0.55f, 1.6f, Vector3{ 0.0f, 0.0f, 0.2f } }) };
    const auto arm = BuildArm(armView.Scene());
    const auto mill = BuildGantry(millView.Scene());

    QWidget window;
    auto* layout = new QVBoxLayout{ &window };
    auto* views = new QHBoxLayout;
    auto* armWidget = new ui::backend::qt::QtPaintedWidget{ armView };
    auto* millWidget = new ui::backend::qt::QtPaintedWidget{ millView };
    auto* status = new QLabel{ "Drag to orbit, right-drag to pan, wheel to zoom, click a part to select it." };
    views->addWidget(armWidget);
    views->addWidget(millWidget);
    layout->addLayout(views, 1);
    layout->addWidget(status);

    const auto reportSelection = [status](NodeId node)
    {
        status->setText(node.Valid() ? QString{ "Selected node %1" }.arg(node.value) : QString{ "Nothing selected" });
    };
    armView.onSelectionChanged = reportSelection;
    millView.onSelectionChanged = reportSelection;

    auto time = 0.0f;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]
        {
            time += 0.03f;
            const std::array<float, 6> q{ 0.8f * std::sin(0.5f * time), 0.35f + 0.3f * std::sin(0.7f * time), -0.4f + 0.35f * std::cos(0.6f * time),
                0.6f * std::sin(time), 0.5f + 0.4f * std::sin(0.8f * time), time };
            armView.Scene().Graph().SetJointValues(q);
            armView.Scene().FindTrail(arm.trail)->Push(armView.Scene().Graph().World(arm.tool).translation);

            auto& graph = millView.Scene().Graph();
            const auto spiral = std::fmod(0.25f * time, 1.0f);
            const auto angle = 6.0f * pi * spiral;
            const auto radius = 0.03f + 0.12f * spiral;
            graph.SetJointValue(mill.x, radius * std::cos(angle) - 0.07f);
            graph.SetJointValue(mill.y, radius * std::sin(angle));
            graph.SetJointValue(mill.z, -0.2f);
            millView.Scene().FindTrail(mill.cut)->Push(graph.World(mill.tip).translation);

            armWidget->update();
            millWidget->update();
        });
    timer.start(30);

    window.resize(1000, 480);
    window.setWindowTitle("ui::stage demo");
    window.show();

    // `--snapshot file.png` renders a few seconds in and exits: a way to look at the demo on a
    // machine with no display (QT_QPA_PLATFORM=offscreen).
    const auto arguments = QApplication::arguments();
    const auto snapshot = arguments.indexOf("--snapshot");

    if (snapshot >= 0 && snapshot + 1 < arguments.size())
        QTimer::singleShot(3000, [&window, path = arguments[snapshot + 1]]
            {
                window.grab().save(path);
                QApplication::quit();
            });

    return QApplication::exec();
}

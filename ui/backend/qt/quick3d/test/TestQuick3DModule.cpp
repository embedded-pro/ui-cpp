#include "ui/backend/qt/quick3d/PolylineGeometry.hpp"
#include "ui/backend/qt/quick3d/ShadedMeshGeometry.hpp"
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVector3D>
#include <QtQml/QQmlExtensionPlugin>
#include <gmock/gmock.h>
#include <cmath>
#include <memory>

Q_IMPORT_QML_PLUGIN(UiQuick3DPlugin)

namespace
{
    class Quick3DModuleTest
        : public ::testing::Test
    {
    protected:
        Quick3DModuleTest()
        {
            engine.addImportPath(":/");
        }

        std::unique_ptr<QObject> Create(const QByteArray& source)
        {
            QQmlComponent component{ &engine };
            component.setData(source, QUrl{ "qrc:/test.qml" });
            std::unique_ptr<QObject> created{ component.create() };
            EXPECT_NE(created, nullptr) << component.errorString().toStdString();
            return created;
        }

        static double FitDistance(QObject& studio, const QVector3D& min, const QVector3D& max)
        {
            QVariant distance;
            const QVariant front{ QVector3D{} };
            EXPECT_TRUE(QMetaObject::invokeMethod(&studio, "fitDistance", Q_RETURN_ARG(QVariant, distance),
                Q_ARG(QVariant, QVariant{ min }), Q_ARG(QVariant, QVariant{ max }), Q_ARG(QVariant, front)));
            return distance.toDouble();
        }

        QQmlEngine engine;
    };
}

TEST_F(Quick3DModuleTest, QmlDeclaresTheGeometriesThroughTheUiQuick3DModule)
{
    const auto line = Create("import QtQuick\nimport QtQuick3D\nimport Ui.Quick3D\nPolylineGeometry { points: [Qt.vector3d(0, 0, 0), Qt.vector3d(1, 1, 1)] }\n");
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(qobject_cast<ui::backend::qt::PolylineGeometry*>(line.get())->vertexData().size(), 2 * 12);

    const auto mesh = Create("import QtQuick3D\nimport Ui.Quick3D\nShadedMeshGeometry { key: \"link\" }\n");
    ASSERT_NE(mesh, nullptr);
    EXPECT_TRUE(qobject_cast<ui::backend::qt::ShadedMeshGeometry*>(mesh.get())->Empty());
}

TEST_F(Quick3DModuleTest, StudioViewHoldsTheSceneContentAndShipsItsLightProbe)
{
    const auto studio = Create("import QtQuick3D\nimport Ui.Quick3D\nStudioView { extent: 50; Node { objectName: \"content\" } }\n");
    ASSERT_NE(studio, nullptr);

    EXPECT_NE(studio->findChild<QObject*>("content"), nullptr);
    EXPECT_NE(studio->findChild<QObject*>("studioFloor"), nullptr);
    EXPECT_EQ(studio->property("presets").toList().size(), 4);
    EXPECT_TRUE(QFile::exists(":" + studio->property("lightProbe").toUrl().path()));
}

TEST_F(Quick3DModuleTest, StudioViewFitsABoxBetweenItsMarginsAtAGivenViewDirection)
{
    const auto studio = Create("import QtQuick3D\nimport Ui.Quick3D\nStudioView { width: 400; height: 400 }\n");
    ASSERT_NE(studio, nullptr);

    const double distance{ FitDistance(*studio, QVector3D{ -10, -10, -10 }, QVector3D{ 10, 10, 10 }) };

    const double tanHalf{ std::tan(20.0 * M_PI / 180.0) * (1.0 - 2.0 * 0.08) };
    EXPECT_NEAR(distance, 10.0 + 10.0 / tanHalf, 1e-3);
}

TEST_F(Quick3DModuleTest, StudioViewFitsAWideBoxByItsWidthInANarrowView)
{
    const auto studio = Create("import QtQuick3D\nimport Ui.Quick3D\nStudioView { width: 200; height: 400 }\n");
    ASSERT_NE(studio, nullptr);

    const double distance{ FitDistance(*studio, QVector3D{ -40, 0, 0 }, QVector3D{ 40, 10, 0 }) };

    const double tanHalf{ std::tan(20.0 * M_PI / 180.0) * (1.0 - 2.0 * 0.08) };
    EXPECT_NEAR(distance, 40.0 / (tanHalf * 0.5), 1e-3);
}

TEST_F(Quick3DModuleTest, StudioViewSwitchesToAnOrthographicCameraWithTheSameScaleAtTheFocus)
{
    const auto studio = Create("import QtQuick3D\nimport Ui.Quick3D\nStudioView { width: 400; height: 300; extent: 50 }\n");
    ASSERT_NE(studio, nullptr);
    auto* view{ studio->property("view").value<QObject*>() };
    auto* perspective{ studio->property("camera").value<QObject*>() };
    auto* orthographic{ studio->property("orthographicCamera").value<QObject*>() };
    ASSERT_NE(view, nullptr);

    EXPECT_EQ(view->property("camera").value<QObject*>(), perspective);
    studio->setProperty("orthographic", true);
    EXPECT_EQ(view->property("camera").value<QObject*>(), orthographic);

    const double distance{ perspective->property("position").value<QVector3D>().z() };
    const double visibleHeight{ 2.0 * distance * std::tan(20.0 * M_PI / 180.0) };
    EXPECT_NEAR(orthographic->property("verticalMagnification").toDouble(), 300.0 / visibleHeight, 1e-3);
}

TEST_F(Quick3DModuleTest, StudioViewShowsAnOrientationGizmoWithBothEndsOfEachAxis)
{
    const auto studio = Create("import QtQuick3D\nimport Ui.Quick3D\nStudioView { width: 400; height: 300 }\n");
    ASSERT_NE(studio, nullptr);

    auto* gizmo{ studio->findChild<QObject*>("studioGizmo") };
    ASSERT_NE(gizmo, nullptr);
    EXPECT_EQ(gizmo->property("endCount").toInt(), 6);

    studio->setProperty("gizmoVisible", false);
    EXPECT_FALSE(gizmo->property("visible").toBool());
}

TEST_F(Quick3DModuleTest, AStatusBadgeShowsOnlyWhileItHasText)
{
    const auto badge = Create("import Ui.Quick3D\nStatusBadge {}\n");
    ASSERT_NE(badge, nullptr);
    EXPECT_FALSE(badge->property("shown").toBool());

    badge->setProperty("text", "Near a singularity");
    EXPECT_TRUE(badge->property("shown").toBool());

    EXPECT_NE(Create("import Ui.Quick3D\nAxisTriad { length: 12 }\n"), nullptr);
}

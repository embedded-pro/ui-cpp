#include "ui/backend/qt/quick3d/PolylineGeometry.hpp"
#include "ui/backend/qt/quick3d/ShadedMeshGeometry.hpp"
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <gmock/gmock.h>
#include <memory>

Q_IMPORT_QML_PLUGIN(UiQuick3DPlugin)

namespace
{
    class Quick3DModuleTest
        : public ::testing::Test
    {
    protected:
        std::unique_ptr<QObject> Create(const QByteArray& source)
        {
            QQmlComponent component{ &engine };
            component.setData(source, QUrl{ "qrc:/test.qml" });
            std::unique_ptr<QObject> created{ component.create() };
            EXPECT_NE(created, nullptr) << component.errorString().toStdString();
            return created;
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

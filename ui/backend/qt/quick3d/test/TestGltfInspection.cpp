#include "ui/backend/qt/quick3d/GltfInspection.hpp"
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>
#include <gmock/gmock.h>

namespace
{
    const QString box{ QStringLiteral(GLTF_TEST_DATA "/box.gltf") };

    QByteArray Gltf(int count, int mode)
    {
        return QString{ R"({ "asset": { "version": "2.0" }, "accessors": [ { "count": %1 } ],
            "meshes": [ { "primitives": [ { "attributes": { "POSITION": 0 }, "mode": %2 } ] } ] })" }
            .arg(count)
            .arg(mode)
            .toUtf8();
    }

    QByteArray Glb(const QByteArray& json)
    {
        auto padded = json;
        while (padded.size() % 4 != 0)
            padded.append(' ');
        QByteArray glb(20, '\0');
        qToLittleEndian<quint32>(0x46546C67, glb.data());
        qToLittleEndian<quint32>(2, glb.data() + 4);
        qToLittleEndian<quint32>(static_cast<quint32>(20 + padded.size()), glb.data() + 8);
        qToLittleEndian<quint32>(static_cast<quint32>(padded.size()), glb.data() + 12);
        qToLittleEndian<quint32>(0x4E4F534A, glb.data() + 16);
        return glb + padded;
    }

    class GltfInspectionTest
        : public ::testing::Test
    {
    protected:
        QString Write(const QString& name, const QByteArray& contents)
        {
            const auto path = directory.filePath(name);
            QFile file{ path };
            EXPECT_TRUE(file.open(QIODevice::WriteOnly));
            file.write(contents);
            return path;
        }

        QTemporaryDir directory;
    };
}

TEST_F(GltfInspectionTest, CountsTheTrianglesOfTextAndBinaryGltf)
{
    QFile file{ box };
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(ui::backend::qt::CountGltfTriangles(file.readAll()), 12u);

    EXPECT_EQ(ui::backend::qt::CountGltfTriangles(Glb(Gltf(300, 4))), 100u);
    EXPECT_EQ(ui::backend::qt::CountGltfTriangles(Gltf(100, 5)), 98u);
    EXPECT_EQ(ui::backend::qt::CountGltfTriangles(Gltf(100, 1)), 0u);
    EXPECT_FALSE(ui::backend::qt::CountGltfTriangles("solid stl").has_value());
}

TEST_F(GltfInspectionTest, RefusesAMissingForeignOrHeavyFileAndNamesIt)
{
    EXPECT_TRUE(ui::backend::qt::CheckGltf(directory.filePath("absent.glb"), 100).problem.startsWith("cannot open"));
    EXPECT_TRUE(ui::backend::qt::CheckGltf(Write("arm.glb", "solid stl"), 100).problem.endsWith("is not a glTF file"));

    const auto heavy = ui::backend::qt::CheckGltf(Write("heavy.gltf", Gltf(3 * 101, 4)), 100);
    EXPECT_TRUE(heavy.url.isEmpty());
    EXPECT_TRUE(heavy.problem.endsWith("has 101 triangles, more than 100")) << heavy.problem.toStdString();

    const auto fine = ui::backend::qt::CheckGltf(box, 100);
    EXPECT_TRUE(fine.problem.isEmpty());
    EXPECT_TRUE(fine.url.isLocalFile());
}

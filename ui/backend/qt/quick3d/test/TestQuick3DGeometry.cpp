#include "ui/backend/qt/quick3d/PolylineGeometry.hpp"
#include "ui/backend/qt/quick3d/ShadedMeshGeometry.hpp"
#include "ui/stage/Primitives.hpp"
#include <QVector3D>
#include <gmock/gmock.h>

namespace
{
    class Quick3DGeometryTest
        : public ::testing::Test
    {
    protected:
        ui::backend::qt::MeshCatalog catalog;
        ui::backend::qt::ShadedMeshGeometry geometry;
    };
}

TEST_F(Quick3DGeometryTest, AShadedMeshBecomesIndexedTrianglesWithNormals)
{
    catalog.Set("box", ui::stage::Shade(ui::stage::MakeBox()));
    geometry.SetCatalog(&catalog);
    geometry.SetKey("box");

    EXPECT_FALSE(geometry.Empty());
    EXPECT_EQ(geometry.primitiveType(), QQuick3DGeometry::PrimitiveType::Triangles);
    EXPECT_EQ(geometry.stride(), 24);
    EXPECT_EQ(geometry.attributeCount(), 3);
    EXPECT_EQ(geometry.vertexData().size(), 24 * 24);
    EXPECT_EQ(geometry.indexData().size(), 36 * 4);
    EXPECT_NEAR(geometry.boundsMax().x(), 0.5f, 1e-6f);
}

TEST_F(Quick3DGeometryTest, FollowsItsKeyAndIgnoresTheOthers)
{
    geometry.SetCatalog(&catalog);
    geometry.SetKey("link");
    EXPECT_TRUE(geometry.Empty());

    int emptied{ 0 };
    QObject::connect(&geometry, &ui::backend::qt::ShadedMeshGeometry::emptyChanged, [&emptied]()
        {
            ++emptied;
        });
    catalog.Set("other", ui::stage::Shade(ui::stage::MakeBox()));
    EXPECT_EQ(emptied, 0);

    catalog.Set("link", ui::stage::Shade(ui::stage::MakeCylinder()));
    EXPECT_FALSE(geometry.Empty());
    EXPECT_EQ(emptied, 1);

    catalog.Remove("link");
    EXPECT_TRUE(geometry.Empty());
    EXPECT_EQ(geometry.vertexData().size(), 0);

    catalog.Set("link", ui::stage::Shade(ui::stage::MakeBox()));
    catalog.Clear();
    EXPECT_TRUE(geometry.Empty());
}

TEST_F(Quick3DGeometryTest, APolylineIsALineStripThroughItsPoints)
{
    ui::backend::qt::PolylineGeometry line;
    EXPECT_EQ(line.vertexData().size(), 0);

    line.SetPoints({ QVector3D{ 0, 0, 0 }, QVector3D{ 1, 2, 3 }, QVector3D{ -1, 0, 1 } });

    EXPECT_EQ(line.primitiveType(), QQuick3DGeometry::PrimitiveType::LineStrip);
    EXPECT_EQ(line.vertexData().size(), 3 * 12);
    EXPECT_NEAR(line.boundsMin().x(), -1.0f, 1e-6f);
    EXPECT_NEAR(line.boundsMax().z(), 3.0f, 1e-6f);
    EXPECT_EQ(line.Points().size(), 3);
}

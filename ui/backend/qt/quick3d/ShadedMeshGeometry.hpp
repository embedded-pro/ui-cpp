#pragma once

#include "ui/backend/qt/quick3d/MeshCatalog.hpp"
#include <QPointer>
#include <QQuick3DGeometry>
#include <QtQml/qqmlregistration.h>

namespace ui::backend::qt
{
    // Draws the catalog's mesh under `key` as triangles with per-vertex normals, in the mesh's own
    // units: scale the model to the scene's.
    class ShadedMeshGeometry
        : public QQuick3DGeometry
    {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(ui::backend::qt::MeshCatalog* catalog READ Catalog WRITE SetCatalog NOTIFY catalogChanged)
        Q_PROPERTY(QString key READ Key WRITE SetKey NOTIFY keyChanged)
        Q_PROPERTY(bool empty READ Empty NOTIFY emptyChanged)

    public:
        explicit ShadedMeshGeometry(QQuick3DObject* parent = nullptr);

        [[nodiscard]] MeshCatalog* Catalog() const;
        void SetCatalog(MeshCatalog* newCatalog);
        [[nodiscard]] QString Key() const;
        void SetKey(const QString& newKey);
        [[nodiscard]] bool Empty() const;

    signals:
        void catalogChanged();
        void keyChanged();
        void emptyChanged();

    private:
        void Rebuild();

        QPointer<MeshCatalog> catalog;
        QMetaObject::Connection watching;
        QString key;
        bool empty{ true };
    };
}

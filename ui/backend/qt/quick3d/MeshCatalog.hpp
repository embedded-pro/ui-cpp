#pragma once

#include "ui/stage/Shading.hpp"
#include <QHash>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace ui::backend::qt
{
    // Shaded meshes by name, filled from C++ and drawn by ShadedMeshGeometry in QML. A key that changes
    // reaches only the geometries showing it.
    class MeshCatalog
        : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("A MeshCatalog is created in C++ and handed to QML.")

    public:
        explicit MeshCatalog(QObject* parent = nullptr);

        void Set(const QString& key, ui::stage::ShadedMesh mesh);
        void Remove(const QString& key);
        void Clear();
        [[nodiscard]] const ui::stage::ShadedMesh* Find(const QString& key) const;

    signals:
        void changed(const QString& key);

    private:
        QHash<QString, ui::stage::ShadedMesh> meshes;
    };
}

#pragma once

#include <QQuick3DGeometry>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace ui::backend::qt
{
    // A line strip through `points` (vector3d values), such as a tool trail. Qt Quick 3D draws it one
    // pixel wide.
    class PolylineGeometry
        : public QQuick3DGeometry
    {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QVariantList points READ Points WRITE SetPoints NOTIFY pointsChanged)

    public:
        explicit PolylineGeometry(QQuick3DObject* parent = nullptr);

        [[nodiscard]] QVariantList Points() const;
        void SetPoints(const QVariantList& newPoints);

    signals:
        void pointsChanged();

    private:
        void Rebuild();

        QVariantList points;
    };
}

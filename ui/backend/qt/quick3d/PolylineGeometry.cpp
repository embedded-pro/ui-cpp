#include "ui/backend/qt/quick3d/PolylineGeometry.hpp"
#include <QByteArray>
#include <QVector3D>
#include <algorithm>
#include <limits>

namespace ui::backend::qt
{
    PolylineGeometry::PolylineGeometry(QQuick3DObject* parent)
        : QQuick3DGeometry{ parent }
    {
        Rebuild();
    }

    QVariantList PolylineGeometry::Points() const
    {
        return points;
    }

    void PolylineGeometry::SetPoints(const QVariantList& newPoints)
    {
        points = newPoints;
        Rebuild();
        emit pointsChanged();
    }

    void PolylineGeometry::Rebuild()
    {
        clear();
        setPrimitiveType(PrimitiveType::LineStrip);
        setStride(3 * sizeof(float));
        addAttribute(Attribute::PositionSemantic, 0, Attribute::F32Type);

        QByteArray vertices(static_cast<qsizetype>(points.size() * 3 * sizeof(float)), Qt::Uninitialized);
        auto* data = reinterpret_cast<float*>(vertices.data());
        constexpr auto huge = std::numeric_limits<float>::max();
        QVector3D lowest{ huge, huge, huge };
        QVector3D highest{ -huge, -huge, -huge };
        for (const auto& value : points)
        {
            const auto point = value.value<QVector3D>();
            *data++ = point.x();
            *data++ = point.y();
            *data++ = point.z();
            lowest = QVector3D{ std::min(lowest.x(), point.x()), std::min(lowest.y(), point.y()), std::min(lowest.z(), point.z()) };
            highest = QVector3D{ std::max(highest.x(), point.x()), std::max(highest.y(), point.y()), std::max(highest.z(), point.z()) };
        }

        setVertexData(vertices);
        if (!points.isEmpty())
            setBounds(lowest, highest);
        update();
    }
}

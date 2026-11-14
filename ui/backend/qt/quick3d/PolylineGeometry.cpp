#include "ui/backend/qt/quick3d/PolylineGeometry.hpp"
#include <QByteArray>
#include <QVector3D>
#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

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

        std::vector<float> coordinates;
        coordinates.reserve(static_cast<std::size_t>(points.size()) * 3);
        constexpr auto huge = std::numeric_limits<float>::max();
        QVector3D lowest{ huge, huge, huge };
        QVector3D highest{ -huge, -huge, -huge };
        for (const auto& value : points)
        {
            const auto point = value.value<QVector3D>();
            coordinates.insert(coordinates.end(), { point.x(), point.y(), point.z() });
            lowest = QVector3D{ std::min(lowest.x(), point.x()), std::min(lowest.y(), point.y()), std::min(lowest.z(), point.z()) };
            highest = QVector3D{ std::max(highest.x(), point.x()), std::max(highest.y(), point.y()), std::max(highest.z(), point.z()) };
        }

        QByteArray vertices(static_cast<qsizetype>(coordinates.size() * sizeof(float)), Qt::Uninitialized);
        if (!coordinates.empty())
            std::memcpy(vertices.data(), coordinates.data(), static_cast<std::size_t>(vertices.size()));
        setVertexData(vertices);
        if (!points.isEmpty())
            setBounds(lowest, highest);
        update();
    }
}

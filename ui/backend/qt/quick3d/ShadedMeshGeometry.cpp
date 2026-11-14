#include "ui/backend/qt/quick3d/ShadedMeshGeometry.hpp"
#include <QByteArray>
#include <QVector3D>
#include <cstring>

namespace ui::backend::qt
{
    namespace
    {
        template<class T>
        QByteArray Bytes(const std::vector<T>& values)
        {
            QByteArray bytes(static_cast<qsizetype>(values.size() * sizeof(T)), Qt::Uninitialized);
            if (!values.empty())
                std::memcpy(bytes.data(), values.data(), static_cast<std::size_t>(bytes.size()));
            return bytes;
        }

        QVector3D ToQt(ui::scene::Vector3 value)
        {
            return QVector3D{ value.x, value.y, value.z };
        }
    }

    ShadedMeshGeometry::ShadedMeshGeometry(QQuick3DObject* parent)
        : QQuick3DGeometry{ parent }
    {
        Rebuild();
    }

    MeshCatalog* ShadedMeshGeometry::Catalog() const
    {
        return catalog;
    }

    void ShadedMeshGeometry::SetCatalog(MeshCatalog* newCatalog)
    {
        if (newCatalog == catalog)
            return;

        disconnect(watching);
        catalog = newCatalog;
        if (catalog != nullptr)
            watching = connect(catalog, &MeshCatalog::changed, this, [this](const QString& changedKey)
                {
                    if (changedKey == key)
                        Rebuild();
                });
        emit catalogChanged();
        Rebuild();
    }

    QString ShadedMeshGeometry::Key() const
    {
        return key;
    }

    void ShadedMeshGeometry::SetKey(const QString& newKey)
    {
        if (newKey == key)
            return;

        key = newKey;
        emit keyChanged();
        Rebuild();
    }

    bool ShadedMeshGeometry::Empty() const
    {
        return empty;
    }

    void ShadedMeshGeometry::Rebuild()
    {
        static_assert(sizeof(ui::stage::ShadedVertex) == 6 * sizeof(float));
        const auto* mesh = catalog != nullptr ? catalog->Find(key) : nullptr;

        clear();
        setPrimitiveType(PrimitiveType::Triangles);
        setStride(sizeof(ui::stage::ShadedVertex));
        addAttribute(Attribute::PositionSemantic, 0, Attribute::F32Type);
        addAttribute(Attribute::NormalSemantic, 3 * sizeof(float), Attribute::F32Type);
        addAttribute(Attribute::IndexSemantic, 0, Attribute::U32Type);

        if (mesh != nullptr)
        {
            setVertexData(Bytes(mesh->vertices));
            setIndexData(Bytes(mesh->indices));
            setBounds(ToQt(mesh->boundsMin), ToQt(mesh->boundsMax));
        }

        const auto nowEmpty = mesh == nullptr || mesh->indices.empty();
        update();
        if (nowEmpty != empty)
        {
            empty = nowEmpty;
            emit emptyChanged();
        }
    }
}

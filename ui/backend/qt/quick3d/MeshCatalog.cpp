#include "ui/backend/qt/quick3d/MeshCatalog.hpp"

namespace ui::backend::qt
{
    MeshCatalog::MeshCatalog(QObject* parent)
        : QObject{ parent }
    {}

    void MeshCatalog::Set(const QString& key, ui::stage::ShadedMesh mesh)
    {
        meshes.emplace(key, std::move(mesh));
        emit changed(key);
    }

    void MeshCatalog::Remove(const QString& key)
    {
        if (meshes.remove(key) != 0)
            emit changed(key);
    }

    void MeshCatalog::Clear()
    {
        const auto keys = meshes.keys();
        meshes.clear();
        for (const auto& key : keys)
            emit changed(key);
    }

    const ui::stage::ShadedMesh* MeshCatalog::Find(const QString& key) const
    {
        const auto found = meshes.constFind(key);
        return found == meshes.constEnd() ? nullptr : &found.value();
    }
}

#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <cstddef>
#include <optional>

namespace ui::backend::qt
{
    // Triangles in a .gltf or .glb, counted from its accessors without loading it: triangle lists,
    // strips and fans count, lines and points do not. Empty when the file is not glTF.
    [[nodiscard]] std::optional<std::size_t> CountGltfTriangles(const QByteArray& contents);

    struct GltfCheck
    {
        QUrl url;
        QString problem;
    };

    // A URL for RuntimeLoader when the file opens, is glTF and stays within the budget; otherwise the
    // reason, naming the file.
    [[nodiscard]] GltfCheck CheckGltf(const QString& path, std::size_t triangleBudget);
}

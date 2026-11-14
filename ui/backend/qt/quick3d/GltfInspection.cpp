#include "ui/backend/qt/quick3d/GltfInspection.hpp"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

namespace ui::backend::qt
{
    namespace
    {
        constexpr quint32 glbMagic{ 0x46546C67 };
        constexpr quint32 jsonChunk{ 0x4E4F534A };
        constexpr qsizetype glbHeader{ 20 };
        constexpr int trianglesMode{ 4 };
        constexpr int stripMode{ 5 };
        constexpr int fanMode{ 6 };

        std::optional<QByteArray> JsonOf(const QByteArray& contents)
        {
            if (contents.size() < glbHeader || qFromLittleEndian<quint32>(contents.constData()) != glbMagic)
                return contents;

            const auto chunkLength = qFromLittleEndian<quint32>(contents.constData() + 12);
            const auto chunkType = qFromLittleEndian<quint32>(contents.constData() + 16);
            if (chunkType != jsonChunk || static_cast<qsizetype>(chunkLength) > contents.size() - glbHeader)
                return std::nullopt;
            return contents.mid(glbHeader, static_cast<qsizetype>(chunkLength));
        }

        std::size_t Triangles(int mode, std::size_t vertices)
        {
            if (mode == trianglesMode)
                return vertices / 3;
            if ((mode == stripMode || mode == fanMode) && vertices >= 3)
                return vertices - 2;
            return 0;
        }
    }

    std::optional<std::size_t> CountGltfTriangles(const QByteArray& contents)
    {
        const auto json = JsonOf(contents);
        if (!json)
            return std::nullopt;

        const auto document = QJsonDocument::fromJson(*json);
        if (!document.isObject() || !document.object().contains("asset"))
            return std::nullopt;

        const auto accessors = document.object().value("accessors").toArray();
        std::size_t triangles{ 0 };
        for (const auto mesh : document.object().value("meshes").toArray())
            for (const auto primitive : mesh.toObject().value("primitives").toArray())
            {
                const auto object = primitive.toObject();
                const auto accessor = object.contains("indices") ? object.value("indices").toInt() : object.value("attributes").toObject().value("POSITION").toInt(-1);
                if (accessor < 0 || accessor >= accessors.size())
                    return std::nullopt;
                triangles += Triangles(object.value("mode").toInt(trianglesMode), static_cast<std::size_t>(accessors.at(accessor).toObject().value("count").toInteger()));
            }
        return triangles;
    }

    GltfCheck CheckGltf(const QString& path, std::size_t triangleBudget)
    {
        QFile file{ path };
        if (!file.open(QIODevice::ReadOnly))
            return { {}, QString{ "cannot open %1" }.arg(path) };

        const auto triangles = CountGltfTriangles(file.readAll());
        if (!triangles)
            return { {}, QString{ "%1 is not a glTF file" }.arg(path) };
        if (*triangles > triangleBudget)
            return { {}, QString{ "%1 has %2 triangles, more than %3" }.arg(path).arg(*triangles).arg(triangleBudget) };
        return { QUrl::fromLocalFile(QFileInfo{ path }.absoluteFilePath()), {} };
    }
}

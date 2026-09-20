#pragma once

#include <QColor>
#include <QImage>
#include <QRect>
#include <cstddef>

namespace ui::backend::qt::test
{
    [[nodiscard]] inline bool HasInk(const QImage& image, const QRect& region, QRgb background)
    {
        const auto clipped = region.intersected(image.rect());

        for (auto y = clipped.top(); y <= clipped.bottom(); ++y)
            for (auto x = clipped.left(); x <= clipped.right(); ++x)
                if (image.pixel(x, y) != background)
                    return true;

        return false;
    }

    [[nodiscard]] inline std::size_t InkCount(const QImage& image, const QRect& region, QRgb background)
    {
        const auto clipped = region.intersected(image.rect());
        std::size_t count{ 0 };

        for (auto y = clipped.top(); y <= clipped.bottom(); ++y)
            for (auto x = clipped.left(); x <= clipped.right(); ++x)
                if (image.pixel(x, y) != background)
                    ++count;

        return count;
    }
}

#pragma once

#include "ui/core/PaintedView.hpp"
#include <cstddef>

namespace ui::backend::recording
{
    class RecordingPaintedHost
        : public PaintedViewHost
    {
    public:
        explicit RecordingPaintedHost(PaintedView& hosted);

        void Invalidate() override;
        void OnViewDestroyed() override;

        [[nodiscard]] std::size_t InvalidateCount() const;
        [[nodiscard]] bool ViewDestroyed() const;

        void ClearInvalidateCount();

    private:
        std::size_t invalidateCount{ 0 };
        bool viewDestroyed{ false };
    };
}

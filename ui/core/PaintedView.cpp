#include "ui/core/PaintedView.hpp"

namespace ui
{
    Size PaintedView::MinimumSize() const
    {
        return Size{ 0.0f, 0.0f };
    }

    void PaintedView::RequestRepaint() const
    {
        if (onRepaintRequested)
            onRepaintRequested();
    }
}

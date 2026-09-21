#include "ui/core/PaintedViewHost.hpp"
#include "ui/core/PaintedView.hpp"

namespace ui
{
    PaintedViewHost::~PaintedViewHost()
    {
        if (view != nullptr)
            view->DetachHost(*this);
    }

    const PaintedView* PaintedViewHost::HostedView() const
    {
        return view;
    }
}

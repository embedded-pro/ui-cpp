#include "ui/core/PaintedView.hpp"

namespace ui
{
    PaintedView::~PaintedView()
    {
        if (host == nullptr)
            return;

        // Clear both ends before the notification: the host may destroy itself in response, and
        // ~PaintedViewHost would otherwise reach back into a view that is already unwinding.
        auto* departing = host;
        host = nullptr;
        departing->view = nullptr;
        departing->OnViewDestroyed();
    }

    Size PaintedView::MinimumSize() const
    {
        return Size{ 0.0f, 0.0f };
    }

    void PaintedView::AttachHost(PaintedViewHost& newHost)
    {
        if (host == &newHost)
            return;

        if (newHost.view != nullptr)
            newHost.view->DetachHost(newHost);

        if (host != nullptr)
            host->view = nullptr;

        host = &newHost;
        host->view = this;
    }

    void PaintedView::DetachHost(PaintedViewHost& formerHost)
    {
        if (host != &formerHost)
            return;

        host->view = nullptr;
        host = nullptr;
    }

    bool PaintedView::HasHost() const
    {
        return host != nullptr;
    }

    void PaintedView::RequestRepaint() const
    {
        if (host != nullptr)
            host->Invalidate();
    }
}

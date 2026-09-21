#pragma once

namespace ui
{
    class PaintedView;

    // The backend side of the painted-view link. A backend implements one of these per adapter;
    // the view calls Invalidate to ask for a repaint, and the link tells each side when the other
    // goes away. Clearing the link is this base's job rather than each implementation's, because a
    // host that forgot to do it would leave the view calling OnViewDestroyed on freed storage.
    class PaintedViewHost
    {
    public:
        PaintedViewHost() = default;
        PaintedViewHost(const PaintedViewHost& other) = delete;
        PaintedViewHost& operator=(const PaintedViewHost& other) = delete;
        virtual ~PaintedViewHost();

        virtual void Invalidate() = 0;
        virtual void OnViewDestroyed() = 0;

        [[nodiscard]] const PaintedView* HostedView() const;

    private:
        friend class PaintedView;

        PaintedView* view{ nullptr };
    };
}

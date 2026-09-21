#pragma once

namespace ui
{
    class PaintedView;

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

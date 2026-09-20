#pragma once

#include "ui/core/Callback.hpp"
#include "ui/core/Canvas.hpp"
#include "ui/core/Geometry.hpp"
#include "ui/core/Input.hpp"

namespace ui
{
    // Every custom-painted widget derives from this and is thereby backend-free and testable
    // without a display. A backend supplies one adapter that forwards its native paint and input
    // events here.
    class PaintedView
        : public InputHandler
    {
    public:
        virtual void Paint(Canvas& canvas, const Rect& bounds) = 0;

        [[nodiscard]] virtual Size MinimumSize() const;

        Callback<void()> onRepaintRequested;

    protected:
        void RequestRepaint() const;
    };
}

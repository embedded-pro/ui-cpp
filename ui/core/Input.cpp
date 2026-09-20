#include "ui/core/Input.hpp"

namespace ui
{
    void InputHandler::OnMousePress(const MouseEvent& event)
    {
        static_cast<void>(event);
    }

    void InputHandler::OnMouseMove(const MouseEvent& event)
    {
        static_cast<void>(event);
    }

    void InputHandler::OnMouseRelease(const MouseEvent& event)
    {
        static_cast<void>(event);
    }

    void InputHandler::OnMouseDoubleClick(const MouseEvent& event)
    {
        static_cast<void>(event);
    }

    void InputHandler::OnMouseLeave()
    {}

    void InputHandler::OnWheel(const WheelEvent& event)
    {
        static_cast<void>(event);
    }

    void InputHandler::OnKeyPress(const KeyEvent& event)
    {
        static_cast<void>(event);
    }
}

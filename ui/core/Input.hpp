#pragma once

#include "ui/core/Geometry.hpp"
#include <cstdint>

namespace ui
{
    enum class MouseButton : std::uint8_t
    {
        None,
        Left,
        Right,
        Middle
    };

    struct Modifiers
    {
        bool shift{ false };
        bool control{ false };
        bool alt{ false };
    };

    struct MouseEvent
    {
        Point position;
        MouseButton button{ MouseButton::None };
        Modifiers modifiers;
    };

    struct WheelEvent
    {
        Point position;
        float delta{ 0.0f };
        Modifiers modifiers;
    };

    // Wider than the current value set needs: a full key enumeration runs well past 255 entries,
    // and widening the base type later would be a silent ABI change.
    enum class Key : std::uint16_t
    {
        Unknown,
        Enter,
        Backspace,
        Tab,
        Escape,
        Delete,
        Home,
        End,
        PageUp,
        PageDown,
        Up,
        Down,
        Left,
        Right
    };

    struct KeyEvent
    {
        Key key{ Key::Unknown };
        char32_t codepoint{ 0 };
        Modifiers modifiers;
    };

    // Hover, double-click and leave have no equivalent on touch-first backends; widgets relying on
    // them must degrade rather than break. See doc/portability.md.
    class InputHandler
    {
    public:
        InputHandler() = default;
        InputHandler(const InputHandler& other) = delete;
        InputHandler& operator=(const InputHandler& other) = delete;
        virtual ~InputHandler() = default;

        virtual void OnMousePress(const MouseEvent& event);
        virtual void OnMouseMove(const MouseEvent& event);
        virtual void OnMouseRelease(const MouseEvent& event);
        virtual void OnMouseDoubleClick(const MouseEvent& event);
        virtual void OnMouseLeave();
        virtual void OnWheel(const WheelEvent& event);
        virtual void OnKeyPress(const KeyEvent& event);
    };
}

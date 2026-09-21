#pragma once

#include "ui/core/Color.hpp"
#include "ui/core/Font.hpp"
#include "ui/core/Geometry.hpp"
#include "ui/core/Input.hpp"
#include "ui/core/Pen.hpp"
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <Qt>
#include <string_view>

namespace ui::backend::qt
{
    [[nodiscard]] inline QColor ToQt(Color color)
    {
        return QColor{ color.red, color.green, color.blue, color.alpha };
    }

    [[nodiscard]] inline QPointF ToQt(Point point)
    {
        return QPointF{ point.x, point.y };
    }

    [[nodiscard]] inline QRectF ToQt(const Rect& rect)
    {
        return QRectF{ rect.x, rect.y, rect.width, rect.height };
    }

    [[nodiscard]] inline QSize ToQtSize(Size size)
    {
        return QSize{ static_cast<int>(size.width), static_cast<int>(size.height) };
    }

    // QString::fromUtf8 with an explicit length, never the const char* overload: these views come
    // from ui::FormatBuffer and are not null-terminated.
    [[nodiscard]] inline QString ToQt(std::string_view text)
    {
        return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
    }

    [[nodiscard]] inline ::Qt::PenStyle ToQt(LineStyle style)
    {
        switch (style)
        {
            case LineStyle::Dash:
                return ::Qt::DashLine;
            case LineStyle::Dot:
                return ::Qt::DotLine;
            case LineStyle::Solid:
            default:
                return ::Qt::SolidLine;
        }
    }

    [[nodiscard]] inline ::Qt::PenCapStyle ToQt(LineCap cap)
    {
        switch (cap)
        {
            case LineCap::Flat:
                return ::Qt::FlatCap;
            case LineCap::Round:
                return ::Qt::RoundCap;
            case LineCap::Square:
            default:
                return ::Qt::SquareCap;
        }
    }

    [[nodiscard]] inline QPen ToQt(const Pen& pen)
    {
        QPen converted{ ToQt(pen.color) };
        converted.setWidthF(pen.width);
        converted.setStyle(ToQt(pen.style));
        converted.setCapStyle(ToQt(pen.cap));

        return converted;
    }

    // A fully transparent colour becomes NoBrush rather than a transparent solid fill, so an
    // outline-only DrawRect behaves the same here as it does on a backend without alpha.
    [[nodiscard]] inline QBrush ToQt(const Brush& brush)
    {
        if (brush.color.alpha == 0)
            return QBrush{ ::Qt::NoBrush };

        return QBrush{ ToQt(brush.color) };
    }

    [[nodiscard]] inline QFont ToQt(const FontSpec& font)
    {
        QFont converted;

        if (font.family == FontFamily::Monospace)
        {
            converted.setFamily(QStringLiteral("Monospace"));
            converted.setStyleHint(QFont::Monospace);
        }
        else
            converted.setStyleHint(QFont::SansSerif);

        converted.setPointSize(font.pointSize);
        converted.setBold(font.bold);
        converted.setItalic(font.italic);

        return converted;
    }

    // Qt aligns text inside a rectangle by box, not by baseline, so Baseline maps to vertical
    // centring — the closest available behaviour. Callers needing a true baseline use the
    // Point overload of Canvas::DrawText.
    [[nodiscard]] inline ::Qt::Alignment ToQt(TextAlign align, TextVerticalAlign verticalAlign)
    {
        ::Qt::Alignment result;

        switch (align)
        {
            case TextAlign::Center:
                result |= ::Qt::AlignHCenter;
                break;
            case TextAlign::Right:
                result |= ::Qt::AlignRight;
                break;
            case TextAlign::Left:
            default:
                result |= ::Qt::AlignLeft;
                break;
        }

        switch (verticalAlign)
        {
            case TextVerticalAlign::Top:
                result |= ::Qt::AlignTop;
                break;
            case TextVerticalAlign::Bottom:
                result |= ::Qt::AlignBottom;
                break;
            case TextVerticalAlign::Middle:
            case TextVerticalAlign::Baseline:
            default:
                result |= ::Qt::AlignVCenter;
                break;
        }

        return result;
    }

    [[nodiscard]] inline Point ToUi(QPointF position)
    {
        return Point{ static_cast<float>(position.x()), static_cast<float>(position.y()) };
    }

    [[nodiscard]] inline Rect ToUi(const QRectF& rect)
    {
        return Rect{ static_cast<float>(rect.x()), static_cast<float>(rect.y()),
            static_cast<float>(rect.width()), static_cast<float>(rect.height()) };
    }

    [[nodiscard]] inline Size ToUi(QSizeF size)
    {
        return Size{ static_cast<float>(size.width()), static_cast<float>(size.height()) };
    }

    [[nodiscard]] inline MouseButton ToUi(::Qt::MouseButton button)
    {
        switch (button)
        {
            case ::Qt::LeftButton:
                return MouseButton::Left;
            case ::Qt::RightButton:
                return MouseButton::Right;
            case ::Qt::MiddleButton:
                return MouseButton::Middle;
            default:
                return MouseButton::None;
        }
    }

    [[nodiscard]] inline Modifiers ToUi(::Qt::KeyboardModifiers modifiers)
    {
        return Modifiers{
            modifiers.testFlag(::Qt::ShiftModifier),
            modifiers.testFlag(::Qt::ControlModifier),
            modifiers.testFlag(::Qt::AltModifier)
        };
    }

    [[nodiscard]] inline Key ToUiKey(int key)
    {
        switch (key)
        {
            case ::Qt::Key_Return:
            case ::Qt::Key_Enter:
                return Key::Enter;
            case ::Qt::Key_Backspace:
                return Key::Backspace;
            case ::Qt::Key_Tab:
                return Key::Tab;
            case ::Qt::Key_Escape:
                return Key::Escape;
            case ::Qt::Key_Delete:
                return Key::Delete;
            case ::Qt::Key_Home:
                return Key::Home;
            case ::Qt::Key_End:
                return Key::End;
            case ::Qt::Key_PageUp:
                return Key::PageUp;
            case ::Qt::Key_PageDown:
                return Key::PageDown;
            case ::Qt::Key_Up:
                return Key::Up;
            case ::Qt::Key_Down:
                return Key::Down;
            case ::Qt::Key_Left:
                return Key::Left;
            case ::Qt::Key_Right:
                return Key::Right;
            default:
                return Key::Unknown;
        }
    }
}

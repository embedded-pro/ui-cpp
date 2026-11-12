#include "ui/backend/qt/QtTheme.hpp"
#include "ui/backend/qt/QtConversions.hpp"
#include <QAbstractButton>
#include <QApplication>
#include <QWidget>

namespace ui::backend::qt
{
    namespace
    {
        theme::ColorRole FillFor(theme::ButtonRole role)
        {
            switch (role)
            {
                case theme::ButtonRole::Primary:
                    return theme::ColorRole::Accent;
                case theme::ButtonRole::Start:
                    return theme::ColorRole::Run;
                case theme::ButtonRole::Stop:
                    return theme::ColorRole::Stop;
                case theme::ButtonRole::EmergencyStop:
                    return theme::ColorRole::EmergencyStop;
                case theme::ButtonRole::Reset:
                    return theme::ColorRole::Neutral;
                case theme::ButtonRole::Default:
                default:
                    return theme::ColorRole::Surface;
            }
        }

        theme::ColorRole ColourFor(theme::StatusLevel level)
        {
            switch (level)
            {
                case theme::StatusLevel::Ok:
                    return theme::ColorRole::Ok;
                case theme::StatusLevel::Warning:
                    return theme::ColorRole::Warning;
                case theme::StatusLevel::Fault:
                    return theme::ColorRole::Fault;
                case theme::StatusLevel::Neutral:
                default:
                    return theme::ColorRole::Neutral;
            }
        }

        QString StatusRule(const QColor& text)
        {
            return QStringLiteral("color: %1; font-weight: bold;").arg(text.name(QColor::HexRgb));
        }

        QColor Mix(const QColor& from, const QColor& to, float amount)
        {
            const auto blend = [amount](int a, int b)
            {
                return static_cast<int>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * amount);
            };

            return QColor{ blend(from.red(), to.red()), blend(from.green(), to.green()), blend(from.blue(), to.blue()) };
        }

        // Hover and pressed states were inconsistent across the eighteen inline stylesheets they
        // replace; deriving them keeps every button behaving the same way.
        QString Rule(const QColor& fill, const QColor& text, const QString& border)
        {
            return QStringLiteral(
                "QPushButton, QToolButton { background-color: %1; color: %2; border: %7;"
                " border-radius: 4px; padding: 6px 14px; }"
                "QPushButton:hover, QToolButton:hover { background-color: %3; }"
                "QPushButton:pressed, QToolButton:pressed { background-color: %4; }"
                "QPushButton:disabled, QToolButton:disabled { background-color: %5; color: %6; }")
                .arg(fill.name(QColor::HexRgb), text.name(QColor::HexRgb),
                    fill.lighter(115).name(QColor::HexRgb), fill.darker(115).name(QColor::HexRgb),
                    fill.lighter(140).name(QColor::HexRgb), fill.lighter(160).name(QColor::HexRgb), border);
        }

        // A default button is filled with the panel's own colour, so on its own it disappears into
        // the panel; lifting it toward the text colour reads as raised in a light and a dark theme.
        QString DefaultRule(const theme::Theme& theme)
        {
            const auto surface = ToQt(theme.Get(theme::ColorRole::Surface));
            const auto text = ToQt(theme.Get(theme::ColorRole::Text));
            const auto fill = Mix(surface, text, 0.10f);
            const auto border = QStringLiteral("1px solid %1").arg(Mix(surface, text, 0.30f).name(QColor::HexRgb));

            return QStringLiteral(
                "QPushButton, QToolButton { background-color: %1; color: %2; border: %3;"
                " border-radius: 4px; padding: 6px 14px; }"
                "QPushButton:hover, QToolButton:hover { background-color: %4; }"
                "QPushButton:pressed, QToolButton:pressed { background-color: %5; }"
                "QPushButton:disabled, QToolButton:disabled { background-color: %6; color: %7; }")
                .arg(fill.name(QColor::HexRgb), text.name(QColor::HexRgb), border, Mix(surface, text, 0.18f).name(QColor::HexRgb),
                    Mix(surface, text, 0.04f).name(QColor::HexRgb), surface.name(QColor::HexRgb),
                    ToQt(theme.Get(theme::ColorRole::TextMuted)).name(QColor::HexRgb));
        }
    }

    QPalette ToQtPalette(const theme::Theme& theme)
    {
        QPalette palette;

        const auto background = ToQt(theme.Get(theme::ColorRole::Background));
        const auto surface = ToQt(theme.Get(theme::ColorRole::Surface));
        const auto text = ToQt(theme.Get(theme::ColorRole::Text));
        const auto muted = ToQt(theme.Get(theme::ColorRole::TextMuted));
        const auto accent = ToQt(theme.Get(theme::ColorRole::Accent));
        const auto inverse = ToQt(theme.Get(theme::ColorRole::TextInverse));

        palette.setColor(QPalette::Window, surface);
        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, background);
        palette.setColor(QPalette::AlternateBase, surface);
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::PlaceholderText, muted);
        palette.setColor(QPalette::Button, surface);
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, inverse);
        palette.setColor(QPalette::ToolTipBase, surface);
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
        palette.setColor(QPalette::Disabled, QPalette::Text, muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);

        return palette;
    }

    void ApplyTheme(const theme::Theme& theme)
    {
        theme::SetCurrent(theme);
        QApplication::setPalette(ToQtPalette(theme));
    }

    void StyleButton(QAbstractButton& button, theme::ButtonRole role)
    {
        StyleButton(button, role, theme::Current());
    }

    void StyleButton(QAbstractButton& button, theme::ButtonRole role, const theme::Theme& theme)
    {
        button.setStyleSheet(ButtonStyleSheet(role, theme));
    }

    QString ButtonStyleSheet(theme::ButtonRole role, const theme::Theme& theme)
    {
        if (role == theme::ButtonRole::Default)
            return DefaultRule(theme);

        return Rule(ToQt(theme.Get(FillFor(role))), ToQt(theme.Get(theme::ColorRole::TextInverse)), QStringLiteral("none"));
    }

    void StyleStatusLabel(QWidget& label, theme::StatusLevel level)
    {
        StyleStatusLabel(label, level, theme::Current());
    }

    void StyleStatusLabel(QWidget& label, theme::StatusLevel level, const theme::Theme& theme)
    {
        label.setStyleSheet(StatusStyleSheet(level, theme));
    }

    QString StatusStyleSheet(theme::StatusLevel level, const theme::Theme& theme)
    {
        return StatusRule(ToQt(theme.Get(ColourFor(level))));
    }
}

#pragma once

#include "ui/theme/Theme.hpp"
#include <QPalette>
#include <QString>

class QAbstractButton;
class QWidget;

namespace ui::backend::qt
{
    [[nodiscard]] QPalette ToQtPalette(const theme::Theme& theme);

    // Makes the given theme current for the portable layer and installs its palette on the
    // running QApplication, so Qt chrome and canvas-drawn content agree. Requires a constructed
    // QApplication.
    void ApplyTheme(const theme::Theme& theme);

    // The one place in the repository permitted to use setStyleSheet: every Run/Stop/Reset button
    // colour that used to be an inline stylesheet at its call site resolves here instead.
    void StyleButton(QAbstractButton& button, theme::ButtonRole role);
    void StyleButton(QAbstractButton& button, theme::ButtonRole role, const theme::Theme& theme);

    [[nodiscard]] QString ButtonStyleSheet(theme::ButtonRole role, const theme::Theme& theme);

    void StyleStatusLabel(QWidget& label, theme::StatusLevel level);
    void StyleStatusLabel(QWidget& label, theme::StatusLevel level, const theme::Theme& theme);

    [[nodiscard]] QString StatusStyleSheet(theme::StatusLevel level, const theme::Theme& theme);
}

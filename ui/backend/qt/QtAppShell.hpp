#pragma once

#include "ui/shell/AppShell.hpp"

class QMainWindow;
class QSplitter;
class QTabWidget;
class QWidget;

namespace ui::backend::qt
{
    // Composes an existing QMainWindow rather than subclassing one, so a consumer's window keeps
    // its own identity and only its constructor body changes.
    class QtAppShell
        : public shell::ShellView
    {
    public:
        QtAppShell(QMainWindow& window, const shell::ShellSpec& spec);

        void SetPanel(QWidget* panel);

        // Takes any QWidget, which is the escape hatch: a hand-written native page sits beside a
        // portable one with no wrapper type and no downcast.
        void SetPage(std::size_t index, QWidget* page);
        void SetContent(QWidget* content);

        void SetStatus(std::string_view message) override;
        void ShowAlert(std::string_view title, std::string_view message) override;
        void SelectPage(std::size_t index) override;

        [[nodiscard]] std::size_t PageCount() const override;
        [[nodiscard]] std::size_t CurrentPage() const override;

        [[nodiscard]] QSplitter* Splitter() const;
        [[nodiscard]] QTabWidget* Tabs() const;

    private:
        void ApplyStretch();

        QMainWindow* window;
        QSplitter* splitter{ nullptr };
        QTabWidget* tabs{ nullptr };
        float panelMaximumWidth{ 0.0f };
    };
}

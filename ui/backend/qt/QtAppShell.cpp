#include "ui/backend/qt/QtAppShell.hpp"
#include "ui/backend/qt/QtConversions.hpp"
#include <QMainWindow>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>

namespace ui::backend::qt
{
    QtAppShell::QtAppShell(QMainWindow& mainWindow, const shell::ShellSpec& spec)
        : window(&mainWindow)
        , panelMaximumWidth(spec.panelMaximumWidth)
    {
        window->setWindowTitle(ToQt(spec.windowTitle));
        window->resize(static_cast<int>(spec.initialSize.width), static_cast<int>(spec.initialSize.height));

        splitter = new QSplitter{ ::Qt::Horizontal, window };

        if (!spec.pages.empty())
        {
            tabs = new QTabWidget{ splitter };

            for (const auto& page : spec.pages)
                tabs->addTab(new QWidget{ tabs }, ToQt(page.title));

            splitter->addWidget(tabs);
        }

        window->setCentralWidget(splitter);
        window->statusBar()->showMessage(ToQt(spec.initialStatus));
    }

    // Applied whenever a widget lands rather than once, because a page-less window sets its panel
    // before its content and Qt rejects a stretch factor for an index that is not there yet.
    void QtAppShell::ApplyStretch()
    {
        if (splitter->count() < 2)
            return;

        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
    }

    void QtAppShell::SetPanel(QWidget* panel)
    {
        if (panel == nullptr)
            return;

        panel->setMaximumWidth(static_cast<int>(panelMaximumWidth));
        splitter->insertWidget(0, panel);
        ApplyStretch();
    }

    void QtAppShell::SetPage(std::size_t index, QWidget* page)
    {
        if (tabs == nullptr || page == nullptr || index >= static_cast<std::size_t>(tabs->count()))
            return;

        const auto title = tabs->tabText(static_cast<int>(index));
        auto* placeholder = tabs->widget(static_cast<int>(index));

        tabs->removeTab(static_cast<int>(index));
        tabs->insertTab(static_cast<int>(index), page, title);
        delete placeholder;

        tabs->setCurrentIndex(0);
    }

    void QtAppShell::SetContent(QWidget* content)
    {
        if (content == nullptr || tabs != nullptr)
            return;

        splitter->addWidget(content);
        ApplyStretch();
    }

    void QtAppShell::SetStatus(std::string_view message)
    {
        window->statusBar()->showMessage(ToQt(message));
    }

    void QtAppShell::ShowAlert(std::string_view title, std::string_view message)
    {
        QMessageBox::warning(window, ToQt(title), ToQt(message));
    }

    void QtAppShell::SelectPage(std::size_t index)
    {
        if (tabs != nullptr && index < static_cast<std::size_t>(tabs->count()))
            tabs->setCurrentIndex(static_cast<int>(index));
    }

    std::size_t QtAppShell::PageCount() const
    {
        return tabs == nullptr ? 0u : static_cast<std::size_t>(tabs->count());
    }

    std::size_t QtAppShell::CurrentPage() const
    {
        return tabs == nullptr || tabs->currentIndex() < 0 ? 0u : static_cast<std::size_t>(tabs->currentIndex());
    }

    QSplitter* QtAppShell::Splitter() const
    {
        return splitter;
    }

    QTabWidget* QtAppShell::Tabs() const
    {
        return tabs;
    }
}

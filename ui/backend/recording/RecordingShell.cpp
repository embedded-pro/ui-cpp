#include "ui/backend/recording/RecordingShell.hpp"

namespace ui::backend::recording
{
    RecordingShell::RecordingShell(const shell::ShellSpec& spec)
        : windowTitle(spec.windowTitle)
        , initialSize(spec.initialSize)
        , panelMaximumWidth(spec.panelMaximumWidth)
    {
        for (const auto& page : spec.pages)
            pageTitles.emplace_back(page.title);

        if (!spec.initialStatus.empty())
            statuses.emplace_back(spec.initialStatus);
    }

    void RecordingShell::SetStatus(std::string_view message)
    {
        statuses.emplace_back(message);
    }

    void RecordingShell::ShowAlert(std::string_view title, std::string_view message)
    {
        alerts.push_back(AlertRecord{ std::string{ title }, std::string{ message } });
    }

    void RecordingShell::SelectPage(std::size_t index)
    {
        if (index < pageTitles.size())
            currentPage = index;
    }

    std::size_t RecordingShell::PageCount() const
    {
        return pageTitles.size();
    }

    std::size_t RecordingShell::CurrentPage() const
    {
        return currentPage;
    }

    const std::vector<std::string>& RecordingShell::Statuses() const
    {
        return statuses;
    }

    const std::vector<AlertRecord>& RecordingShell::Alerts() const
    {
        return alerts;
    }

    const std::vector<std::string>& RecordingShell::PageTitles() const
    {
        return pageTitles;
    }

    std::string_view RecordingShell::WindowTitle() const
    {
        return windowTitle;
    }

    Size RecordingShell::InitialSize() const
    {
        return initialSize;
    }

    float RecordingShell::PanelMaximumWidth() const
    {
        return panelMaximumWidth;
    }
}

#pragma once

#include "ui/shell/AppShell.hpp"
#include <string>
#include <vector>

namespace ui::backend::recording
{
    struct AlertRecord
    {
        std::string title;
        std::string message;
    };

    // The second ShellView implementation, and the reason a window's composition can be asserted
    // without a display: an empty pages span, a status message and an alert are all observable here
    // exactly as they are through Qt.
    class RecordingShell
        : public shell::ShellView
    {
    public:
        explicit RecordingShell(const shell::ShellSpec& spec);

        void SetStatus(std::string_view message) override;
        void ShowAlert(std::string_view title, std::string_view message) override;
        void SelectPage(std::size_t index) override;

        [[nodiscard]] std::size_t PageCount() const override;
        [[nodiscard]] std::size_t CurrentPage() const override;

        [[nodiscard]] const std::vector<std::string>& Statuses() const;
        [[nodiscard]] const std::vector<AlertRecord>& Alerts() const;
        [[nodiscard]] const std::vector<std::string>& PageTitles() const;
        [[nodiscard]] std::string_view WindowTitle() const;
        [[nodiscard]] Size InitialSize() const;
        [[nodiscard]] float PanelMaximumWidth() const;

    private:
        std::string_view windowTitle;
        Size initialSize;
        float panelMaximumWidth;
        std::vector<std::string> pageTitles;
        std::vector<std::string> statuses;
        std::vector<AlertRecord> alerts;
        std::size_t currentPage{ 0 };
    };
}

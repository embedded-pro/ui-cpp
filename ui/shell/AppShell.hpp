#pragma once

#include "ui/core/Geometry.hpp"
#include <cstddef>
#include <span>
#include <string_view>

namespace ui::shell
{
    struct PageSpec
    {
        std::string_view title;
    };

    // What an application is made of, not how a toolkit arranges it. An empty pages span means the
    // content is shown directly rather than behind a page selector; that is one window shape in the
    // consumer repositories, not a special case.
    struct ShellSpec
    {
        std::string_view windowTitle;
        Size initialSize{ 1200.0f, 700.0f };
        float panelMaximumWidth{ 350.0f };
        std::span<const PageSpec> pages;
        std::string_view initialStatus;
    };

    class ShellView
    {
    public:
        ShellView() = default;
        ShellView(const ShellView& other) = delete;
        ShellView& operator=(const ShellView& other) = delete;
        virtual ~ShellView() = default;

        virtual void SetStatus(std::string_view message) = 0;
        virtual void ShowAlert(std::string_view title, std::string_view message) = 0;
        virtual void SelectPage(std::size_t index) = 0;

        [[nodiscard]] virtual std::size_t PageCount() const = 0;
        [[nodiscard]] virtual std::size_t CurrentPage() const = 0;
    };
}

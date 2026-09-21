#pragma once

#include "ui/terminal/TerminalTypes.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace ui::terminal
{
    class TerminalScreen;

    class TerminalCursorOperations
    {
    public:
        explicit TerminalCursorOperations(TerminalScreen& screen);

        void Up(int n);
        void Down(int n);
        void Forward(int n);
        void Backward(int n);
        void MoveTo(int row, int col);
        void MoveToColumn(int col);
        void Save();
        void Restore();

    private:
        TerminalScreen& screen_;
    };

    class TerminalTabStops
    {
    public:
        explicit TerminalTabStops(TerminalScreen& screen);

        void SetHere();
        void ClearHere();
        void ClearAll();

    private:
        TerminalScreen& screen_;
    };

    class TerminalScreen
    {
    public:
        explicit TerminalScreen(int rows = 24, int cols = 100);

        int Rows() const;
        int Cols() const;

        void Reset();
        void SoftReset();

        const Cell& At(int row, int col) const;
        const CursorPosition& Cursor() const;
        const Rendition& CurrentRendition() const;
        void SetRendition(const Rendition& rendition);

        const Modes& GetModes() const;
        Modes& GetModes();

        const std::deque<std::vector<Cell>>& History() const;
        void ClearHistory();

        void Write(char32_t ch);

        void CarriageReturn();
        void LineFeed();
        void Backspace();
        void HorizontalTab();

        void Index();
        void NextLine();
        void ReverseIndex();

        TerminalTabStops TabStops();

        TerminalCursorOperations CursorOperations();

        void EraseInDisplay(int mode);
        void EraseInLine(int mode);

        void SetScrollRegion(int top, int bottom);
        int ScrollTop() const;
        int ScrollBottom() const;

        std::string LineText(int row) const;

    private:
        friend class TerminalCursorOperations;
        friend class TerminalTabStops;

        void ScrollUpInRegion(int n);
        void ScrollDownInRegion(int n);
        std::vector<Cell> MakeBlankRow() const;
        Cell MakeBlankCell() const;
        void ClampCursor();

        int rows_{};
        int cols_{};
        std::vector<std::vector<Cell>> grid_;
        CursorPosition cursor_{};
        CursorPosition savedCursor_{};
        Rendition currentRendition_{};
        Rendition savedRendition_{};
        std::vector<uint8_t> tabStops_;
        int scrollTop_{ 0 };
        int scrollBottom_{ 0 };
        bool pendingWrap_{ false };
        Modes modes_{};
        std::deque<std::vector<Cell>> history_;
        std::size_t maxHistory_{ 1000 };
    };
}

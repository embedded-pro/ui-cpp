#include "ui/model/TableSpec.hpp"
#include <algorithm>

namespace ui::model
{
    TableModel::TableModel(const TableSpec& spec, std::span<double> cells)
        : spec(&spec)
        , cells(cells)
    {}

    const TableSpec& TableModel::Spec() const
    {
        return *spec;
    }

    std::size_t TableModel::RowCount() const
    {
        return rowCount;
    }

    std::size_t TableModel::ColumnCount() const
    {
        return spec->columns.size();
    }

    std::size_t TableModel::MaximumRows() const
    {
        return spec->maximumRows;
    }

    std::size_t TableModel::Capacity() const
    {
        const auto columns = ColumnCount();
        if (columns == 0)
            return 0;

        return std::min<std::size_t>(spec->maximumRows, cells.size() / columns);
    }

    std::span<const double> TableModel::Row(std::size_t row) const
    {
        if (row >= rowCount)
            return {};

        return cells.subspan(row * ColumnCount(), ColumnCount());
    }

    double TableModel::Cell(std::size_t row, std::size_t column) const
    {
        if (row >= rowCount || column >= ColumnCount())
            return 0.0;

        return cells[row * ColumnCount() + column];
    }

    void TableModel::SetCell(std::size_t row, std::size_t column, double value)
    {
        if (row >= rowCount || column >= ColumnCount())
            return;

        cells[row * ColumnCount() + column] = value;
    }

    bool TableModel::AddRow()
    {
        if (rowCount >= Capacity())
            return false;

        for (std::size_t column = 0; column < ColumnCount(); ++column)
            cells[rowCount * ColumnCount() + column] = spec->columns[column].number.initial;

        ++rowCount;
        return true;
    }

    bool TableModel::AddRow(std::span<const double> values)
    {
        if (rowCount >= Capacity())
            return false;

        for (std::size_t column = 0; column < ColumnCount(); ++column)
            cells[rowCount * ColumnCount() + column] = column < values.size() ? values[column] : spec->columns[column].number.initial;

        ++rowCount;
        return true;
    }

    bool TableModel::RemoveRow(std::size_t row)
    {
        if (row >= rowCount)
            return false;

        const auto columns = ColumnCount();
        for (auto target = row; target + 1 < rowCount; ++target)
            for (std::size_t column = 0; column < columns; ++column)
                cells[target * columns + column] = cells[(target + 1) * columns + column];

        --rowCount;
        return true;
    }

    bool TableModel::RemoveLastRow()
    {
        if (rowCount == 0)
            return false;

        --rowCount;
        return true;
    }

    void TableModel::Clear()
    {
        rowCount = 0;
    }

    void TableModel::Seed(std::span<const double> valuesRowMajor)
    {
        Clear();

        const auto columns = ColumnCount();
        if (columns == 0)
            return;

        for (std::size_t offset = 0; offset + columns <= valuesRowMajor.size(); offset += columns)
            if (!AddRow(valuesRowMajor.subspan(offset, columns)))
                return;
    }
}

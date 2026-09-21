#pragma once

#include "ui/model/FieldId.hpp"
#include "ui/model/FieldSpec.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ui::model
{
    struct ColumnSpec
    {
        std::string_view heading;
        NumberTraits number{};
    };

    struct TableSpec
    {
        GroupId group{ noGroup };
        std::span<const ColumnSpec> columns;
        std::uint16_t maximumRows{ 32 };
        std::string_view addLabel{ "Add" };
        std::string_view removeLabel{ "Remove" };
    };

    // Cells are row-major in storage the caller owns, sized maximumRows * columns.size(). The model
    // never grows it: adding past capacity fails rather than reallocating, which is the whole point
    // of declaring a maximum.
    class TableModel
    {
    public:
        TableModel(const TableSpec& spec, std::span<double> cells);

        [[nodiscard]] const TableSpec& Spec() const;
        [[nodiscard]] std::size_t RowCount() const;
        [[nodiscard]] std::size_t ColumnCount() const;
        [[nodiscard]] std::size_t MaximumRows() const;

        [[nodiscard]] std::span<const double> Row(std::size_t row) const;
        [[nodiscard]] double Cell(std::size_t row, std::size_t column) const;
        void SetCell(std::size_t row, std::size_t column, double value);

        bool AddRow();
        bool AddRow(std::span<const double> values);
        bool RemoveRow(std::size_t row);
        bool RemoveLastRow();
        void Clear();
        void Seed(std::span<const double> valuesRowMajor);

    private:
        [[nodiscard]] std::size_t Capacity() const;

        const TableSpec* spec;
        std::span<double> cells;
        std::size_t rowCount{ 0 };
    };
}

#include "ui/model/TableSpec.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::model::ColumnSpec;
    using ui::model::TableModel;
    using ui::model::TableSpec;

    constexpr std::array<ColumnSpec, 2> columns{
        ColumnSpec{ "Frequency (Hz)", { 0.0, 96000.0, 10.0, 1000.0, 1 } },
        ColumnSpec{ "Amplitude", { 0.0, 1.0, 0.01, 0.5, 3 } }
    };

    class TableModelTest
        : public ::testing::Test
    {
    protected:
        TableSpec spec{ ui::model::noGroup, columns, 4, "Add", "Remove" };
        std::array<double, 8> cells{};
        TableModel table{ spec, cells };
    };
}

TEST_F(TableModelTest, ANewTableIsEmptyButKnowsItsShape)
{
    EXPECT_EQ(table.RowCount(), 0u);
    EXPECT_EQ(table.ColumnCount(), 2u);
    EXPECT_EQ(table.MaximumRows(), 4u);
}

// The widget this replaces seeded new rows with hard-coded strings; the column's own default is
// where that belongs.
TEST_F(TableModelTest, AnAddedRowTakesEachColumnsDefault)
{
    ASSERT_TRUE(table.AddRow());

    EXPECT_NEAR(table.Cell(0, 0), 1000.0, 1e-9);
    EXPECT_NEAR(table.Cell(0, 1), 0.5, 1e-9);
}

TEST_F(TableModelTest, AddingPastTheMaximumFailsRatherThanGrowing)
{
    for (auto i = 0; i < 4; ++i)
        EXPECT_TRUE(table.AddRow());

    EXPECT_FALSE(table.AddRow());
    EXPECT_EQ(table.RowCount(), 4u);
}

TEST_F(TableModelTest, RemovingFromTheMiddleCompactsTheRowsAbove)
{
    table.AddRow(std::array<double, 2>{ 100.0, 0.1 });
    table.AddRow(std::array<double, 2>{ 200.0, 0.2 });
    table.AddRow(std::array<double, 2>{ 300.0, 0.3 });

    ASSERT_TRUE(table.RemoveRow(1));

    ASSERT_EQ(table.RowCount(), 2u);
    EXPECT_NEAR(table.Cell(0, 0), 100.0, 1e-9);
    EXPECT_NEAR(table.Cell(1, 0), 300.0, 1e-9);
}

TEST_F(TableModelTest, RemovingAnOutOfRangeRowFails)
{
    table.AddRow();

    EXPECT_FALSE(table.RemoveRow(3));
    EXPECT_EQ(table.RowCount(), 1u);
}

TEST_F(TableModelTest, RemovingTheLastRowOfAnEmptyTableFails)
{
    EXPECT_FALSE(table.RemoveLastRow());
}

TEST_F(TableModelTest, SeedingReplacesTheContents)
{
    table.AddRow();

    const std::array<double, 4> seed{ 200.0, 1.0, 2000.0, 0.5 };
    table.Seed(seed);

    ASSERT_EQ(table.RowCount(), 2u);
    EXPECT_NEAR(table.Cell(0, 0), 200.0, 1e-9);
    EXPECT_NEAR(table.Cell(1, 1), 0.5, 1e-9);
}

TEST_F(TableModelTest, SeedingStopsAtCapacityRatherThanOverrunning)
{
    const std::array<double, 12> seed{ 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 4.0, 4.0, 5.0, 5.0, 6.0, 6.0 };
    table.Seed(seed);

    EXPECT_EQ(table.RowCount(), 4u);
}

TEST_F(TableModelTest, ATrailingPartialRowIsNotSeeded)
{
    const std::array<double, 3> seed{ 100.0, 0.1, 200.0 };
    table.Seed(seed);

    EXPECT_EQ(table.RowCount(), 1u);
}

TEST_F(TableModelTest, RowExposesTheCellsOfThatRowOnly)
{
    table.AddRow(std::array<double, 2>{ 100.0, 0.1 });
    table.AddRow(std::array<double, 2>{ 200.0, 0.2 });

    const auto row = table.Row(1);

    ASSERT_EQ(row.size(), 2u);
    EXPECT_NEAR(row[0], 200.0, 1e-9);
    EXPECT_NEAR(row[1], 0.2, 1e-9);
}

TEST_F(TableModelTest, RowBeyondTheEndIsEmptyRatherThanOutOfBounds)
{
    EXPECT_TRUE(table.Row(0).empty());
}

TEST_F(TableModelTest, WritingAnOutOfRangeCellIsIgnored)
{
    table.AddRow();

    table.SetCell(5, 0, 42.0);
    table.SetCell(0, 9, 42.0);

    EXPECT_NEAR(table.Cell(0, 0), 1000.0, 1e-9);
}

// The cell storage is the caller's, so a span smaller than maximumRows caps the table rather than
// letting it write past the end.
TEST_F(TableModelTest, StorageSmallerThanTheDeclaredMaximumCapsTheTable)
{
    std::array<double, 4> smallCells{};
    TableModel small{ spec, smallCells };

    EXPECT_TRUE(small.AddRow());
    EXPECT_TRUE(small.AddRow());
    EXPECT_FALSE(small.AddRow());
}

#include "ui/backend/qt/QtFormTable.hpp"
#include "ui/backend/qt/test/FormTestSpec.hpp"
#include <QPushButton>
#include <QTableWidget>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::qt::QtFormTable;

    class QtFormTableTest
        : public ::testing::Test
    {
    protected:
        QtFormTableTest()
            : table(harness.Model(), 0)
        {}

        formspec::Harness harness;
        QtFormTable table;
    };
}

TEST_F(QtFormTableTest, TheColumnsTakeTheirHeadingsFromTheSpec)
{
    ASSERT_EQ(table.Grid()->columnCount(), 2);
    EXPECT_EQ(table.Grid()->horizontalHeaderItem(0)->text(), "Frequency (Hz)");
    EXPECT_EQ(table.Grid()->horizontalHeaderItem(1)->text(), "Amplitude");
}

TEST_F(QtFormTableTest, TheGridShowsTheRowsTheModelAlreadyHolds)
{
    EXPECT_EQ(table.Grid()->rowCount(), 1);
    EXPECT_NEAR(table.Grid()->item(0, 0)->data(::Qt::EditRole).toDouble(), 1000.0, 1e-9);
}

TEST_F(QtFormTableTest, AddingARowSeedsItFromTheColumnDefaults)
{
    table.AddButton()->click();

    ASSERT_EQ(table.Grid()->rowCount(), 2);
    EXPECT_NEAR(table.Grid()->item(1, 1)->data(::Qt::EditRole).toDouble(), 0.5, 1e-9);
    EXPECT_EQ(harness.Model().Table(0).RowCount(), 2u);
}

TEST_F(QtFormTableTest, AddingStopsAtTheDeclaredMaximum)
{
    for (auto i = 0; i < 5; ++i)
        table.AddButton()->click();

    EXPECT_EQ(harness.Model().Table(0).RowCount(), 4u);
    EXPECT_EQ(table.Grid()->rowCount(), 4);
}

// The widget this replaces removed the last row when nothing was selected; keeping that means a
// user who has not clicked into the grid still has a working Remove.
TEST_F(QtFormTableTest, RemovingWithNoSelectionDropsTheLastRow)
{
    table.AddButton()->click();
    harness.Model().Table(0).SetCell(0, 0, 4242.0);

    table.RemoveButton()->click();

    ASSERT_EQ(harness.Model().Table(0).RowCount(), 1u);
    EXPECT_NEAR(harness.Model().Table(0).Cell(0, 0), 4242.0, 1e-9);
}

TEST_F(QtFormTableTest, RemovingFromAnEmptyTableIsHarmless)
{
    table.RemoveButton()->click();
    table.RemoveButton()->click();

    EXPECT_EQ(harness.Model().Table(0).RowCount(), 0u);
    EXPECT_EQ(table.Grid()->rowCount(), 0);
}

TEST_F(QtFormTableTest, EditingACellReachesTheModel)
{
    table.Grid()->item(0, 0)->setData(::Qt::EditRole, 3300.0);

    EXPECT_NEAR(harness.Model().Table(0).Cell(0, 0), 3300.0, 1e-9);
}

TEST_F(QtFormTableTest, ChangingTheTableNotifiesItsSubscriber)
{
    auto notified = 0;
    harness.Model().onTableChanged = [&notified]
    {
        ++notified;
    };

    table.AddButton()->click();
    table.RemoveButton()->click();

    EXPECT_EQ(notified, 2);
}

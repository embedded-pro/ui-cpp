#include "ui/backend/qt/QtFormTable.hpp"
#include "ui/backend/qt/QtConversions.hpp"
#include "ui/backend/qt/QtTheme.hpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace ui::backend::qt
{
    QtFormTable::QtFormTable(model::FormModel& formModel, std::size_t index, QWidget* parent)
        : QWidget(parent)
        , model(&formModel)
        , tableIndex(index)
    {
        const auto& spec = formModel.Table(index).Spec();

        auto* layout = new QVBoxLayout{ this };
        layout->setContentsMargins(0, 0, 0, 0);

        grid = new QTableWidget{ 0, static_cast<int>(spec.columns.size()), this };

        QStringList headings;
        for (const auto& column : spec.columns)
            headings << ToQt(column.heading);

        grid->setHorizontalHeaderLabels(headings);
        grid->horizontalHeader()->setStretchLastSection(true);
        grid->verticalHeader()->setVisible(false);
        layout->addWidget(grid);

        auto* buttons = new QHBoxLayout{};
        addButton = new QPushButton{ ToQt(spec.addLabel), this };
        removeButton = new QPushButton{ ToQt(spec.removeLabel), this };
        StyleButton(*addButton, theme::ButtonRole::Default);
        StyleButton(*removeButton, theme::ButtonRole::Default);
        buttons->addWidget(addButton);
        buttons->addWidget(removeButton);
        layout->addLayout(buttons);

        connect(addButton, &QPushButton::clicked, this, &QtFormTable::OnAdd);
        connect(removeButton, &QPushButton::clicked, this, &QtFormTable::OnRemove);
        connect(grid, &QTableWidget::itemChanged, this, &QtFormTable::OnItemChanged);

        Refresh();
    }

    void QtFormTable::Refresh()
    {
        applying = true;

        const auto& table = model->Table(tableIndex);
        grid->setRowCount(static_cast<int>(table.RowCount()));

        for (std::size_t row = 0; row < table.RowCount(); ++row)
        {
            for (std::size_t column = 0; column < table.ColumnCount(); ++column)
            {
                auto* item = grid->item(static_cast<int>(row), static_cast<int>(column));
                if (item == nullptr)
                {
                    item = new QTableWidgetItem{};
                    grid->setItem(static_cast<int>(row), static_cast<int>(column), item);
                }

                // Numeric edit role rather than free text: the widget this replaces parsed strings
                // with toFloat(), which silently yielded zero for anything unparseable.
                item->setData(::Qt::EditRole, table.Cell(row, column));
            }
        }

        applying = false;
    }

    void QtFormTable::OnAdd()
    {
        if (model->Table(tableIndex).AddRow())
        {
            Refresh();
            model->NotifyTableChanged();
        }
    }

    void QtFormTable::OnRemove()
    {
        auto& table = model->Table(tableIndex);
        const auto selected = grid->selectionModel()->selectedRows();

        const auto removed = selected.isEmpty()
                                 ? table.RemoveLastRow()
                                 : table.RemoveRow(static_cast<std::size_t>(selected.first().row()));

        if (removed)
        {
            Refresh();
            model->NotifyTableChanged();
        }
    }

    void QtFormTable::OnItemChanged(QTableWidgetItem* item)
    {
        if (applying || item == nullptr)
            return;

        model->Table(tableIndex).SetCell(static_cast<std::size_t>(item->row()), static_cast<std::size_t>(item->column()), item->data(::Qt::EditRole).toDouble());
        model->NotifyTableChanged();
    }

    QTableWidget* QtFormTable::Grid() const
    {
        return grid;
    }

    QPushButton* QtFormTable::AddButton() const
    {
        return addButton;
    }

    QPushButton* QtFormTable::RemoveButton() const
    {
        return removeButton;
    }
}

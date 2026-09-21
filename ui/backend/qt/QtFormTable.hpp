#pragma once

#include "ui/model/FormModel.hpp"
#include <QWidget>

class QPushButton;
class QTableWidget;
class QTableWidgetItem;

namespace ui::backend::qt
{
    // A repeating group of numeric columns with add and remove. Deliberately not a table view: no
    // sorting, no selection model, no per-cell delegates, no mixed column types. Those make a
    // spreadsheet, and doc/portability.md puts one permanently out of scope.
    class QtFormTable
        : public QWidget
    {
        Q_OBJECT

    public:
        QtFormTable(model::FormModel& model, std::size_t tableIndex, QWidget* parent = nullptr);

        void Refresh();

        [[nodiscard]] QTableWidget* Grid() const;
        [[nodiscard]] QPushButton* AddButton() const;
        [[nodiscard]] QPushButton* RemoveButton() const;

    private:
        void OnAdd();
        void OnRemove();
        void OnItemChanged(QTableWidgetItem* item);

        model::FormModel* model;
        std::size_t tableIndex;
        QTableWidget* grid{ nullptr };
        QPushButton* addButton{ nullptr };
        QPushButton* removeButton{ nullptr };
        bool applying{ false };
    };
}

#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;

class UserManagePage : public QWidget
{
public:
    explicit UserManagePage(QWidget *parent = nullptr);
    void refresh();

private:
    void fillTable();
    QVariantMap selectedUser() const;
    void showNoSelection() const;

    void toggleFreeze();
    void showOrderHistory();

    QLineEdit *mSearchEdit = nullptr;
    QPushButton *mFreezeButton = nullptr;
    QTableWidget *mTable = nullptr;
};
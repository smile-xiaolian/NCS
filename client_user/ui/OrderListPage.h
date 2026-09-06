#ifndef ORDERLISTPAGE_H
#define ORDERLISTPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class OrderListPage : public QWidget
{
    Q_OBJECT
public:
    explicit OrderListPage(QWidget *parent = nullptr);
    void loadOrders(int userId);

signals:
    void goToSettleRequested();

private:
    int currentUserId = 0;
    QTableWidget *orderTable;
    QPushButton *viewDetailsBtn;

    void setupTable();
    void onViewDetails();
};

#endif // ORDERLISTPAGE_H
#ifndef ORDERLISTPAGE_H
#define ORDERLISTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QEvent>

class OrderListPage : public QWidget
{
    Q_OBJECT
public:
    explicit OrderListPage(QWidget *parent = nullptr);
    void loadOrders(int userId);

signals:
    void goToSettleRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    int currentUserId = 0;

    QScrollArea *scrollArea;
    QWidget *cardContainerWidget;
    QVBoxLayout *cardContainerLayout;

    QWidget* createOrderCard(const QVariantMap &orderMap);
    void handleOrderClick(int orderId, int status);
};

#endif // ORDERLISTPAGE_H

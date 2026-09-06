#ifndef ORDERTICKETDIALOG_H
#define ORDERTICKETDIALOG_H

#include <QDialog>
#include <QVariantMap>

class OrderTicketDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OrderTicketDialog(const QVariantMap &orderData, QWidget *parent = nullptr);

private:
    void setupUi(const QVariantMap &orderData);
};

#endif // ORDERTICKETDIALOG_H
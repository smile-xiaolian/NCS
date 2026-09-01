#ifndef USER_H
#define USER_H

#include <QString>

struct User {
    int id = 0;
    QString phone;
    QString nickname;
    QString avatar;
    double balance = 0.0;
    int status = 1;  // 1=正常, 0=冻结
    QString createdAt;
};

#endif // USER_H
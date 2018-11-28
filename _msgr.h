#ifndef MSGR_H
#define MSGR_H

#include <QObject>

class Msgr : public QObject
{
    Q_OBJECT

public:
    enum Type { Default, Busy, Info, Error, Critical, Permanent };
    Msgr() : QObject() {}

signals:
    void msg(const QString &msg, const Msgr::Type &msgType=Default);
};

#endif // MSGR_H

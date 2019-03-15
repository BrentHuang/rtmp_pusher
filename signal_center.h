#ifndef SIGNAL_CENTER_H
#define SIGNAL_CENTER_H

#include <QObject>
#include "singleton.h"

class SignalCenter : public QObject
{
    Q_OBJECT

public:
    SignalCenter();
    virtual ~SignalCenter();

signals:
    void StartStream();
    void StopStream();
};

#define SIGNAL_CENTER ((SignalCenter*) Singleton<SignalCenter>::Instance().get())

#endif // SIGNAL_CENTER_H

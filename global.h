#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWaitCondition>
#include <QMutex>
#include "singleton.h"
#include "config.h"

struct Global
{
    QMutex write_file_mutex;
    std::atomic_bool thread_exit;

    Config config;

    Global() : write_file_mutex(), config()
    {
        thread_exit = false;
    }

    ~Global() {}
};

#define GLOBAL static_cast<Global*>(Singleton<Global>::Instance().get())

#endif // GLOBAL_H

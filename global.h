#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWaitCondition>
#include <QMutex>
#include "singleton.h"
#include "config.h"

struct Global
{
    // 文件解析线程、播放线程之间的同步
    QWaitCondition file_parse_cond;
    QMutex file_parse_mutex;

    std::atomic_bool app_exit;

    Config config;

    Global() : config()
    {
        app_exit = false;
    }

    ~Global() {}
};

#define GLOBAL Singleton<Global>::Instance().get()

#endif // GLOBAL_H

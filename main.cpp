#include "main_window.h"
#include <QApplication>

#ifdef __cplusplus
extern "C" {
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

// avcl: A pointer to an arbitrary struct of which the first field is a pointer to an AVClass struct.
static void av_log_callback(void* avcl, int level, const char* fmt, va_list vl)
{
    (void) avcl;
    (void) level;
    (void) fmt;
    (void) vl;
    // TODO
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    av_log_set_level(AV_LOG_WARNING);
//    av_log_set_callback(av_log_callback);
    avdevice_register_all();

    MainWindow w;
    w.show();

    return a.exec();
}

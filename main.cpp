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

    // 遍历支持的封装格式
    AVInputFormat* input_format = av_iformat_next(nullptr);
    puts("-------------------------------Input--------------------------------");
    while (input_format != nullptr)
    {
        printf("%s ", input_format->name);
        input_format = input_format->next;
    }
    puts("\n--------------------------------------------------------------------");

    AVOutputFormat* output_format = av_oformat_next(nullptr);
    puts("-------------------------------Output-------------------------------");
    while (output_format != nullptr)
    {
        printf("%s ", output_format->name);
        output_format = output_format->next;
    }
    puts("\n--------------------------------------------------------------------");

    MainWindow w;
    w.show();

    return a.exec();
}

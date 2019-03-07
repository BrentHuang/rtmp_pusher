#include "main_window.h"
#include <QDebug>
#include "ui_main_window.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#endif
#if __cplusplus
}
#endif

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    AVFormatContext* pFmtCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "list_devices", "true", 0);
    AVInputFormat* iformat = av_find_input_format("dshow");
    printf("Device Info=============\n");
    avformat_open_input(&pFmtCtx, "video=dummy", iformat, &options);
    printf("========================\n");
}

MainWindow::~MainWindow()
{
    delete ui;
}

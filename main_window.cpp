#include "main_window.h"
#include <QDebug>

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

#include <chrono>
#include "ui_main_window.h"
#include "devices_dialog.h"
#include "signal_center.h"
#include "global.h"

MainWindow*           gpMainFrame = NULL;

static int64_t TimeNowMSec()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}


/* 在Windows下可以使用2种方式读取摄像头数据：
*  1.VFW: Video for Windows 屏幕捕捉设备。注意输入URL是设备的序号，
*          从0至9。
*  2.dshow: 使用Directshow。注意作者机器上的摄像头设备名称是
*         “Integrated Camera”，使用的时候需要改成自己电脑上摄像头设
*          备的名称。
* 在Linux下可以使用video4linux2读取摄像头设备。
* 在MacOS下可以使用avfoundation读取摄像头设备。
*/
void show_dshow_device()
{
//    {
//        AVFormatContext* pFormatCtx = avformat_alloc_context();
//        AVDictionary* options = NULL;
//        av_dict_set(&options, "list_devices", "true", 0);
//        AVInputFormat* iformat = av_find_input_format("dshow");
//        printf("Device Info=============\n");
//        avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
//        printf("========================\n");
//    }

//    {
//        AVFormatContext* pFormatCtx = avformat_alloc_context();
//        AVDictionary* options = NULL;
//        av_dict_set(&options, "list_devices", "true", 0);
//        AVInputFormat* iformat = av_find_input_format("dshow");
//        printf("Device Info=============\n");
//        avformat_open_input(&pFormatCtx, "audio=dummy", iformat, &options);
//        printf("========================\n");
//    }

//    {
//        AVFormatContext* pFormatCtx = avformat_alloc_context();
//        AVDictionary* options = NULL;
//        av_dict_set(&options, "list_options", "true", 0);
//        AVInputFormat* iformat = av_find_input_format("dshow");
//        printf("Device Option Info======\n");
//        avformat_open_input(&pFormatCtx, "video=ICT Camera", iformat, &options);
//        printf("========================\n");
//    }
}

//采集到的视频图像回调
static int VideoCaptureCallback(AVStream* input_st, AVPixelFormat pix_fmt, AVFrame* pframe, int64_t lTimeStamp)
{
//    if (gpMainFrame->IsPreview())
//    {
//        gpMainFrame->m_Painter.Play(input_st, pframe);
//    }

    gpMainFrame->output_stream_.write_video_frame(input_st, pix_fmt, pframe, lTimeStamp);
    return 0;
}

//采集到的音频数据回调
static int AudioCaptureCallback(AVStream* input_st, AVFrame* pframe, int64_t lTimeStamp)
{
    gpMainFrame->output_stream_.write_audio_frame(input_st, pframe, lTimeStamp);
    return 0;
}

int64_t  StartTime = 0;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    gpMainFrame = this;

    show_dshow_device();

    connect(SIGNAL_CENTER, &SignalCenter::StartStream, this, &MainWindow::OnStartStream);
    connect(SIGNAL_CENTER, &SignalCenter::StopStream, this, &MainWindow::OnStopStream);
}

MainWindow::~MainWindow()
{
    disconnect(SIGNAL_CENTER, &SignalCenter::StartStream, this, &MainWindow::OnStartStream);
    disconnect(SIGNAL_CENTER, &SignalCenter::StopStream, this, &MainWindow::OnStopStream);

    OnStopStream();
    delete ui;
}

void MainWindow::on_actionDevices_triggered()
{
    DevicesDialog devices_dialog;
    devices_dialog.exec(); // show(): 非模态  open(): 半模态  exec(): 模态
}

void MainWindow::OnStartStream()
{
//    input_stream_.SetVideoCaptureDevice(GLOBAL->config.GetVideoCaptureDevice());
    input_stream_.SetVideoCaptureDevice("/dev/video0");
    input_stream_.SetAudioCaptureDevice(GLOBAL->config.GetAudioCaptureDevice());

    // 设置视频和音频的数据回调函数。当采集开始时，视频和音频数据就会传递给相应的函数去处理，
    // 在该程序中，回调函数主要对图像或音频进行编码，然后封装成FFmpeg支持的容器（例如mkv/avi/mpg/ts/mp4）
    input_stream_.SetVideoCaptureCB(VideoCaptureCallback);
    input_stream_.SetAudioCaptureCB(AudioCaptureCallback);

    // 初始化采集设备
    if (input_stream_.Open() != 0)
    {
//        MessageBox(_T("打开采集设备失败"), _T("提示"), MB_OK | MB_ICONWARNING);
        input_stream_.Close();
        return;
    }

    // 初始化输出流.
    // 初始化输出流需要知道视频采集的分辨率，帧率，输出像素格式等信息，还有音频采集设备的采样率，声道数，Sample格式，而这些信息可通过CAVInputStream类的接口来获取到
    int cx, cy, fps;
    AVPixelFormat pixel_fmt;
    if (0 == input_stream_.GetVideoInputInfo(cx, cy, fps, pixel_fmt)) //获取视频采集源的信息
    {
        output_stream_.SetVideoCodecProp(AV_CODEC_ID_H264, fps, 500000, 100, cx, cy); //设置视频编码器属性
    }

    int sample_rate = 0, channels = 0;
    AVSampleFormat  sample_fmt;
    if (0 == input_stream_.GetAudioInputInfo(sample_fmt, sample_rate, channels)) //获取音频采集源的信息
    {
        output_stream_.SetAudioCodecProp(AV_CODEC_ID_AAC, sample_rate, channels, 32000); //设置音频编码器属性
    }

    // 打开编码器和录制的容器
    bool bRet  = output_stream_.OpenOutputStream("/home/hgc/a.mkv"); //GLOBAL->config.GetFilePath()); //设置输出路径
    if (!bRet)
    {
//        MessageBox(_T("初始化输出失败"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

//    m_Painter.SetVideoWindow(m_wndView.GetSafeHwnd()); //设置视频预览窗口
//    m_Painter.SetSrcFormat(cx, cy, 24, TRUE); //第4个参数表示是否翻转图像
//    m_Painter.SetStretch(TRUE); //是否缩放显示图像
//    m_Painter.Open();

    // 开始采集
    input_stream_.StartCapture();

    StartTime = TimeNowMSec();

    m_frmCount = 0;
    m_nFPS = 0;

    GLOBAL->config.SetStarted(true);
}

void MainWindow::OnStopStream()
{
    input_stream_.Close();
    output_stream_.CloseOutput();
//    m_Painter.Close();

    GLOBAL->config.SetStarted(false);
}

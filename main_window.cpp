﻿#include "main_window.h"
#include <QDebug>
#include <QSysInfo>
#include "ui_main_window.h"

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

#include <chrono>
#include "DS_AudioVideoDevices.h"
#include "MF_AudioVideoDevices.h"

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

    gpMainFrame->m_OutputStream.write_video_frame(input_st, pix_fmt, pframe, lTimeStamp);
    return 0;
}

//采集到的音频数据回调
static int AudioCaptureCallback(AVStream* input_st, AVFrame* pframe, int64_t lTimeStamp)
{
    gpMainFrame->m_OutputStream.write_audio_frame(input_st, pframe, lTimeStamp);
    return 0;
}

int64_t  StartTime = 0;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    gpMainFrame = this;

    avdevice_register_all();
    show_dshow_device();
}

MainWindow::~MainWindow()
{
    OnStopStream();
    delete ui;
}

void MainWindow::on_actionDevices_triggered()
{
    std::vector<TDeviceName> video_devices;
    std::vector<TDeviceName> audio_devices;
    HRESULT hr;


    if (QSysInfo::WV_WINDOWS10 == QSysInfo::windowsVersion())
    {
        hr = MF_GetAudioVideoInputDevices(video_devices, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (hr != S_OK)
        {
            qDebug() << "failed";
            return;
        }

        hr = MF_GetAudioVideoInputDevices(audio_devices, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
        if (hr != S_OK)
        {
            qDebug() << "failed";
            return;
        }
    }
    else
    {
        hr = DS_GetAudioVideoInputDevices(video_devices, CLSID_VideoInputDeviceCategory);
        if (hr != S_OK)
        {
            qDebug() << "failed";
            return;
        }

        hr = DS_GetAudioVideoInputDevices(audio_devices, CLSID_AudioInputDeviceCategory);
        if (hr != S_OK)
        {
            qDebug() << "failed";
            return;
        }
    }

    qDebug() << "video devices:";
    for (int i = 0; i < (int) video_devices.size(); ++i)
    {
        qDebug() << i << QString::fromWCharArray(video_devices[i].FriendlyName)
                 << QString::fromWCharArray(video_devices[i].MonikerName);
    }

    qDebug() << "audio devices:";
    for (int i = 0; i < (int) audio_devices.size(); ++i)
    {
        qDebug() << i << QString::fromWCharArray(audio_devices[i].FriendlyName)
                 << QString::fromWCharArray(audio_devices[i].MonikerName);
    }

// 开始采集
    m_InputStream.SetVideoCaptureDevice(QString::fromWCharArray(video_devices[1].FriendlyName).toStdString());
    m_InputStream.SetAudioCaptureDevice(QString::fromWCharArray(audio_devices[0].FriendlyName).toStdString());

    OnStartStream();

// 停止采集
//    OnStopStream();


}

void MainWindow::OnStartStream()
{
    // 首先设置了视频和音频的数据回调函数。当采集开始时，视频和音频数据就会传递给相应的函数去处理，在该程序中，回调函数主要对图像或音频进行编码，然后封装成FFmpeg支持的容器（例如mkv/avi/mpg/ts/mp4）
    m_InputStream.SetVideoCaptureCB(VideoCaptureCallback);
    m_InputStream.SetAudioCaptureCB(AudioCaptureCallback);

    // 打开输入设备
    bool bRet;
    bRet = m_InputStream.OpenInputStream(); //初始化采集设备
    if (!bRet)
    {
//        MessageBox(_T("打开采集设备失败"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

    // 初始化输出流.
    // 初始化输出流需要知道视频采集的分辨率，帧率，输出像素格式等信息，还有音频采集设备的采样率，声道数，Sample格式，而这些信息可通过CAVInputStream类的接口来获取到
    int cx, cy, fps;
    AVPixelFormat pixel_fmt;
    if (m_InputStream.GetVideoInputInfo(cx, cy, fps, pixel_fmt)) //获取视频采集源的信息
    {
        m_OutputStream.SetVideoCodecProp(AV_CODEC_ID_H264, fps, 500000, 100, cx, cy); //设置视频编码器属性
    }

    int sample_rate = 0, channels = 0;
    AVSampleFormat  sample_fmt;
    if (m_InputStream.GetAudioInputInfo(sample_fmt, sample_rate, channels)) //获取音频采集源的信息
    {
        m_OutputStream.SetAudioCodecProp(AV_CODEC_ID_AAC, sample_rate, channels, 32000); //设置音频编码器属性
    }

    // 打开编码器和录制的容器
    m_szFilePath = "D:\\mycamera.mkv";
    bRet  = m_OutputStream.OpenOutputStream(m_szFilePath.c_str()); //设置输出路径
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
    bRet = m_InputStream.StartCapture();

    StartTime = TimeNowMSec();

    m_frmCount = 0;
    m_nFPS = 0;
}

void MainWindow::OnStopStream()
{
    m_InputStream.CloseInputStream();
    m_OutputStream.CloseOutput();
//    m_Painter.Close();

//    TRACE("采集用时：%d 秒\n", (timeGetTime() - StartTime) / 1000);

    StartTime = 0;
}

int MainWindow::ShowDevices()
{
    //    ::CoInitialize(NULL); //调用DirectShow SDK的API需要用到COM库


    //    int iVideoCapDevNum = 0;
    //    int iAudioCapDevNum = 0;

    //    char* DevicesArray[20];
    //    for (int i = 0; i < 20; i++)
    //    {
    //        DevicesArray[i] = new char[256];
    //        memset(DevicesArray[i], 0, 256);
    //    }

    //    HRESULT hr;
    //    hr = EnumDevice(DSHOW_VIDEO_DEVICE, DevicesArray, sizeof(DevicesArray) / sizeof(DevicesArray[0]), iVideoCapDevNum);
    //    if (hr == S_OK)
    //    {
    //        for (int i = 0; i < iVideoCapDevNum; i++)
    //        {
    //            CString strDevName = DevicesArray[i];
    //            ((CComboBox*)GetDlgItem(IDC_COMBO_VIDEO_DEVICES))->AddString(strDevName);
    //        }
    //    }

    //    hr = EnumDevice(DSHOW_AUDIO_DEVICE, DevicesArray, sizeof(DevicesArray) / sizeof(DevicesArray[0]), iAudioCapDevNum);
    //    if (hr == S_OK)
    //    {
    //        for (int i = 0; i < iAudioCapDevNum; i++)
    //        {
    //            CString strDevName = DevicesArray[i];
    //            ((CComboBox*)GetDlgItem(IDC_COMBO_AUDIO_DEVICES))->AddString(strDevName);
    //        }
    //    }

    //    for (int i = 0; i < 20; i++)
    //    {
    //        delete DevicesArray[i];
    //        DevicesArray[i] = NULL;
    //    }

    //    if (iVideoCapDevNum > 0)
    //    {
    //        ((CButton*)GetDlgItem(IDC_CHECK_ENABLE_VIDEODEV))->SetCheck(TRUE);
    //    }

    //    if (iAudioCapDevNum > 0)
    //    {
    //        ((CButton*)GetDlgItem(IDC_CHECK_ENABLE_AUDIODEV))->SetCheck(TRUE);
    //    }

    //    CoUninitialize();

    return 0;
}
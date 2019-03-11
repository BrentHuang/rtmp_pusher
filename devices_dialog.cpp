#include "devices_dialog.h"
#include <QDebug>
#include <QOperatingSystemVersion>

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

#include "ui_devices_dialog.h"
#include "ds_av_devices.h"
#include "mf_av_devices.h"
#include "global.h"
#include "signal_center.h"

DevicesDialog::DevicesDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::DevicesDialog)
{
    ui->setupUi(this);

#if defined(Q_OS_WIN)
    std::vector<DeviceCtx> video_device_vec;
    std::vector<DeviceCtx> audio_device_vec;

    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
    {
        if (MFGetAVInputDevices(video_device_vec, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID) != 0)
        {
            qDebug() << "no video devices";
            return;
        }

        if (MFGetAVInputDevices(audio_device_vec, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID) != 0)
        {
            qDebug() << "no audio devices";
            return;
        }
    }
    else
    {
        if (DSGetAVInputDevices(video_device_vec, CLSID_VideoInputDeviceCategory) != 0)
        {
            qDebug() << "no video devices";
            return;
        }

        if (DSGetAVInputDevices(audio_device_vec, CLSID_AudioInputDeviceCategory) != 0)
        {
            qDebug() << "no audio devices";
            return;
        }
    }

    qDebug() << "video input devices:";
    for (int i = 0; i < (int) video_device_vec.size(); ++i)
    {
        qDebug() << i << QString::fromWCharArray(video_device_vec[i].FriendlyName)
                 << QString::fromWCharArray(video_device_vec[i].MonikerName);
        ui->comboBox_Video->addItem(QString::fromWCharArray(video_device_vec[i].FriendlyName));
    }

    qDebug() << "audio input devices:";
    for (int i = 0; i < (int) audio_device_vec.size(); ++i)
    {
        qDebug() << i << QString::fromWCharArray(audio_device_vec[i].FriendlyName)
                 << QString::fromWCharArray(audio_device_vec[i].MonikerName);
        ui->comboBox_Audio->addItem(QString::fromWCharArray(audio_device_vec[i].FriendlyName));
    }

    if (last_video_device_.length() > 0)
    {
        ui->comboBox_Video->setCurrentText(last_video_device_);
    }

    if (last_audio_device_.length() > 0)
    {
        ui->comboBox_Audio->setCurrentText(last_audio_device_);
    }

    if (GLOBAL->config.Started())
    {
        ui->pushButton_Start->setEnabled(false);
        ui->pushButton_Stop->setEnabled(true);
    }
    else
    {
        ui->pushButton_Start->setEnabled(true);
        ui->pushButton_Stop->setEnabled(false);
    }

#elif defined(Q_OS_LINUX)

#endif
}

DevicesDialog::~DevicesDialog()
{
    delete ui;
}

void DevicesDialog::on_comboBox_Video_currentIndexChanged(const QString& arg1)
{
    last_video_device_ = QString::fromStdString(GLOBAL->config.GetVideoCaptureDevice());
    GLOBAL->config.SetVideoCaptureDevice(arg1.toStdString());
}

void DevicesDialog::on_comboBox_Audio_currentIndexChanged(const QString& arg1)
{
    last_audio_device_ = QString::fromStdString(GLOBAL->config.GetAudioCaptureDevice());
    GLOBAL->config.SetAudioCaptureDevice(arg1.toStdString());
}

void DevicesDialog::on_pushButton_Start_clicked()
{
    close();
    emit SIGNAL_CENTER->StartStream();
}

void DevicesDialog::on_pushButton_Stop_clicked()
{
    close();
    emit SIGNAL_CENTER->StopStream();
}

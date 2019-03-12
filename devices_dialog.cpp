﻿#include "devices_dialog.h"
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QFileDialog>
#include <QMessageBox>

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
#elif defined(Q_OS_LINUX)
    ui->comboBox_Audio->addItem(QString::fromStdString("CX20751/2 Analog"));
    ui->comboBox_Video->addItem(QString::fromStdString("/dev/video0"));
#endif

    if (!last_video_device_.isEmpty())
    {
        ui->comboBox_Video->setCurrentText(last_video_device_);
    }

    if (!last_audio_device_.isEmpty())
    {
        ui->comboBox_Audio->setCurrentText(last_audio_device_);
    }

    ui->checkBox_Video->setChecked(GLOBAL->config.HasVideo());
    ui->checkBox_Audio->setChecked(GLOBAL->config.HasAudio());

    if (!GLOBAL->config.GetFilePath().empty())
    {
        ui->textBrowser_FilePath->setText(QString::fromStdString(GLOBAL->config.GetFilePath()));
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
    if (!ui->checkBox_Audio->isChecked() && !ui->checkBox_Video->isChecked())
    {
        QMessageBox::warning(this, "warning", QStringLiteral("音频和视频至少要有一个"));
        return;
    }

    if (GLOBAL->config.GetFilePath().empty())
    {
        QMessageBox::warning(this, "warning", QStringLiteral("录制路径未设置"));
        return;
    }

    close();
    emit SIGNAL_CENTER->StartStream();
}

void DevicesDialog::on_pushButton_Stop_clicked()
{
    close();
    emit SIGNAL_CENTER->StopStream();
}

void DevicesDialog::on_pushButton_Dir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择文件保存路径"));
    if (!dir.isEmpty())
    {
        GLOBAL->config.SetFilePath(dir.toStdString() + "/my.flv");
        ui->textBrowser_FilePath->setText(QString::fromStdString(GLOBAL->config.GetFilePath()));
    }
}

void DevicesDialog::on_checkBox_Audio_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetHasAudio(ui->checkBox_Audio->isChecked());
}

void DevicesDialog::on_checkBox_Video_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetHasVideo(ui->checkBox_Video->isChecked());
}

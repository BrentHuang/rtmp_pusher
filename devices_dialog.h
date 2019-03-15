#ifndef DEVICES_DIALOG_H
#define DEVICES_DIALOG_H

#include <QDialog>

namespace Ui
{
class DevicesDialog;
}

class DevicesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DevicesDialog(QWidget* parent = nullptr);
    ~DevicesDialog();

private slots:
    void on_comboBox_Video_currentIndexChanged(const QString& arg1);
    void on_comboBox_Audio_currentIndexChanged(const QString& arg1);
    void on_comboBox_AudioSpeaker_currentIndexChanged(const QString& arg1);

    void on_pushButton_Start_clicked();
    void on_pushButton_Stop_clicked();
    void on_pushButton_Dir_clicked();

    void on_checkBox_Audio_stateChanged(int arg1);
    void on_checkBox_Video_stateChanged(int arg1);
    void on_checkBox_AudioSystem_stateChanged(int arg1);
    void on_checkBox_VideoDesktop_stateChanged(int arg1);

private:
    Ui::DevicesDialog* ui;
    QString last_video_device_;
    QString last_audio_device_;
    QString last_audio2_device_;
};

#endif // DEVICES_DIALOG_H

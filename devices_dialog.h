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
    void on_comboBox_Microphone_currentIndexChanged(const QString& arg1);
    void on_comboBox_Speaker_currentIndexChanged(const QString& arg1);

    void on_pushButton_Start_clicked();
    void on_pushButton_Stop_clicked();
    void on_pushButton_Dir_clicked();

    void on_checkBox_Microphone_stateChanged(int arg1);
    void on_checkBox_Camera_stateChanged(int arg1);
    void on_checkBox_Speaker_stateChanged(int arg1);
    void on_checkBox_Desktop_stateChanged(int arg1);

private:
    Ui::DevicesDialog* ui;
    QString last_video_device_;
    QString last_microphone_;
    QString last_speaker_;
};

#endif // DEVICES_DIALOG_H

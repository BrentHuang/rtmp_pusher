#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "av_input_stream.h"
#include "av_output_stream.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:

    void on_actionDevices_triggered();
    void OnStartStream();
    void OnStopStream();

private:
    int ShowDevices();

public:
    AVInputStream    m_InputStream;
    AVOutputStream   m_OutputStream;

private:
    Ui::MainWindow* ui;
    std::string m_video_device;
    std::string m_audio_device;
    std::string m_szFilePath;
    int64_t              m_frmCount;
    int64_t              m_nFPS;
};

#endif // MAIN_WINDOW_H

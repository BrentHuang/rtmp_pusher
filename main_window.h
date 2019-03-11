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

public slots:
    void OnStartStream();
    void OnStopStream();

private slots:
    void on_actionDevices_triggered();

public:
    AVInputStream input_stream_;
    AVOutputStream output_stream_;

private:
    Ui::MainWindow* ui;

    std::string video_input_device_;
    std::string audio_input_device_;
    std::string record_file_path_;
    int64_t              m_frmCount;
    int64_t              m_nFPS;
};

#endif // MAIN_WINDOW_H

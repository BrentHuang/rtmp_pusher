#ifndef CAPTURE_DEVICE_H
#define CAPTURE_DEVICE_H

#include <string>
#include <thread>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

typedef int (*CaptureCB)(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp);

class CaptureDevice
{
public:
    CaptureDevice();
    ~CaptureDevice();

    void SetDeviceName(const std::string& fmt_name, const std::string& device_name, bool prefix);
    void SetCaptureCB(CaptureCB cb);

    int Open(bool video);
    void Close();

    int StartCapture(int64_t timestamp);

protected:
    static int CaptureThreadFunc(void* args);
    int ReadPackets();

protected:
    std::string fmt_name_;
    std::string device_name_;
    bool prefix_;
    AVInputFormat* input_fmt_;
    AVFormatContext* fmt_ctx_;
    int stream_idx_;
    std::thread* capture_thread_;
    int64_t start_time_;
    CaptureCB capture_cb_;
};

#endif // CAPTURE_DEVICE_H

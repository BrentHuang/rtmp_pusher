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

class CaptureDevice
{
public:
    CaptureDevice();
    ~CaptureDevice();

    void SetDeviceCtx(const std::string& fmt_name, const std::string& device_name, bool prefix);

protected:
    std::string fmt_name_;
    std::string device_name_;
    bool prefix_;
    AVInputFormat* input_fmt_;
    AVFormatContext* fmt_ctx_;
    int stream_idx_;
    std::thread* capture_thread_;
};

#endif // CAPTURE_DEVICE_H

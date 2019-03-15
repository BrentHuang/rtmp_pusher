#include "capture_device.h"

CaptureDevice::CaptureDevice() : fmt_name_(), device_name_()
{
    prefix_ = false;
    input_fmt_ = nullptr;
    fmt_ctx_ = nullptr;
    stream_idx_ = -1;
    capture_thread_ = nullptr;
}

CaptureDevice::~CaptureDevice()
{
}

void CaptureDevice::SetDeviceCtx(const std::string& fmt_name, const std::string& device_name, bool prefix)
{
    fmt_name_ = fmt_name;
    device_name_ = device_name;
    prefix_ = prefix;
}

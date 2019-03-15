#include "mf_av_devices.h"

#if defined(Q_OS_WIN)
#include <mfapi.h>

#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mf.lib")

int MFGetAVDevices(std::vector<DeviceCtx>& device_ctx_vec, REFGUID guid_value)
{
    DeviceCtx name;
    HRESULT hr;

    // 初始化
    device_ctx_vec.clear();

    // 初始化Media Foundation
    hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    if (FAILED(hr))
    {
        return -1;
    }

    // 创建属性搜索页
    int ret = -1;

    do
    {
        IMFAttributes* attributes = NULL;
        hr = MFCreateAttributes(&attributes, 1); // 要求Windows Vista
        if (FAILED(hr))
        {
            break;
        }

        // 设置搜索关键字-枚举音频视频设备
        hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, guid_value);
        if (SUCCEEDED(hr))
        {
            // 获取搜索结果
            IMFActivate** devices = NULL;
            UINT32 count = 0;

            hr = MFEnumDeviceSources(attributes, &devices, &count);  // 要求Windows 7
            if (FAILED(hr))
            {
                break;
            }

            if (0 == count)
            {
                // 没有找到
                hr = E_FAIL;
                break;
            }

            for (DWORD i = 0; i < count; ++i)
            {
                // 获取设备友好名
                devices[i]->GetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name.FriendlyName, MAX_FRIENDLY_NAME_LENGTH, nullptr);

                // 获取设备Moniker名
                devices[i]->GetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, name.MonikerName, MAX_MONIKER_NAME_LENGTH, NULL);

                // 加入列表
                device_ctx_vec.push_back(name);

                // 释放资源
                devices[i]->Release();
            }

            // 释放内存
            CoTaskMemFree(devices);
        }

        attributes->Release();
        ret = 0;
    } while (0);

    // 关闭Media Foundation
    MFShutdown();

    return ret;
}
#endif

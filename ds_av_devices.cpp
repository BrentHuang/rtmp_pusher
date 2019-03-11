#include "ds_av_devices.h"

#if defined(Q_OS_WIN)
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

int DSGetAVInputDevices(std::vector<DeviceCtx>& device_ctx_vec, REFGUID guid_value)
{
    DeviceCtx name;
    HRESULT hr;

    // 初始化
    device_ctx_vec.clear();

    // 初始化COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        return -1;
    }

    // 创建系统设备枚举器实例
    ICreateDevEnum* sys_dev_enum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**) &sys_dev_enum);
    if (FAILED(hr))
    {
        CoUninitialize();
        return -1;
    }

    // 获取设备类枚举器
    int ret = -1;

    do
    {
        IEnumMoniker* enum_cat = NULL;
        hr = sys_dev_enum->CreateClassEnumerator(guid_value, &enum_cat, 0);
        if (hr != S_OK)
        {
            break;
        }

        // 枚举设备名称
        IMoniker* moniker = NULL;
        ULONG fetched;
        while (enum_cat->Next(1, &moniker, &fetched) == S_OK)
        {
            IPropertyBag* prop_bag;
            hr = moniker->BindToStorage(NULL, NULL, IID_IPropertyBag, (void**) &prop_bag);
            if (SUCCEEDED(hr))
            {
                // 获取设备友好名
                VARIANT var_name;
                VariantInit(&var_name);

                hr = prop_bag->Read(L"FriendlyName", &var_name, NULL);
                if (SUCCEEDED(hr))
                {
                    StringCchCopy(name.FriendlyName, MAX_FRIENDLY_NAME_LENGTH, var_name.bstrVal);

                    // 获取设备Moniker名
                    LPOLESTR ole_display_name = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc(MAX_MONIKER_NAME_LENGTH * 2));
                    if (ole_display_name != NULL)
                    {
                        hr = moniker->GetDisplayName(NULL, NULL, &ole_display_name);
                        if (SUCCEEDED(hr))
                        {
                            StringCchCopy(name.MonikerName, MAX_MONIKER_NAME_LENGTH, ole_display_name);
                            device_ctx_vec.push_back(name);
                        }

                        CoTaskMemFree(ole_display_name);
                    }
                }

                VariantClear(&var_name);
                prop_bag->Release();
            }

            moniker->Release();
        } // End for While

        enum_cat->Release();
        ret = 0;
    } while (0);

    sys_dev_enum->Release();
    CoUninitialize();

    return ret;
}
#endif

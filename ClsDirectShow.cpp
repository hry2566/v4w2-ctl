#include "ClsDirectShow.h"

ClsDirectShow::ClsDirectShow()
{
    // ctor
}

ClsDirectShow::~ClsDirectShow()
{
    CoUninitialize();
}

IEnumMoniker *ClsDirectShow::video_init(ICreateDevEnum *pDevEnum)
{
    if (FAILED(CoInitialize(NULL)))
    {
        printf("error: COM init error");
        return NULL;
    }

    // Create the System Device Enumerator.
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        IEnumMoniker *pClassEnum = NULL;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
        if (hr == S_FALSE)
        {
            return NULL;
        }
        return pClassEnum;
    }
    return NULL;
}

void ClsDirectShow::get_devices_list(IEnumMoniker *pClassEnum)
{
    ULONG cFetched;
    IMoniker *pMoniker = NULL;
    int n = 0;
    pClassEnum->Reset();

    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK)
    {
        IPropertyBag *pP = NULL;
        VARIANT var;
        var.vt = VT_BSTR;
        BSTR device_name;
        BSTR device_path;

        pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pP);
        pP->Read(L"FriendlyName", &var, 0);
        device_name = var.bstrVal;
        VariantClear(&var);

        pP->Read(L"DevicePath", &var, 0);
        device_path = var.bstrVal;
        VariantClear(&var);

        std::wcout << device_name << ": (" << device_path << "):" << std::endl;
        std::wcout << "        /dev/video" << n << std::endl;
        n++;
    }
    std::wcout << std::endl;
    return;
}

void ClsDirectShow::get_videoformats_list(int device_num, IEnumMoniker *pClassEnum)
{
    ULONG cFetched;
    IMoniker *pMoniker = NULL;
    int n = 0;
    pClassEnum->Reset();

    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK)
    {
        if (device_num == n)
        {
            IBaseFilter *pbf = NULL;
            pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pbf);
            get_config(pbf);
            return;
        }
        n++;
    }
}

void ClsDirectShow::get_config(IBaseFilter *pbf)
{
    ICaptureGraphBuilder2 *pCapture = NULL;
    CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&pCapture);
    IAMStreamConfig *pConfig = NULL;
    HRESULT hr = pCapture->FindInterface(&PIN_CATEGORY_CAPTURE, 0, pbf, IID_IAMStreamConfig, (void **)&pConfig);

    int iCount = 0;
    int iSize = 0;
    hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
    std::vector<video_formats> format_list;
    video_formats ret;

    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
        for (int iFormat = 0; iFormat < iCount; iFormat++)
        {
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE *pmtConfig;
            hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE *)&scc);
            VIDEOINFOHEADER *pVih2;

            if ((SUCCEEDED(hr) && pmtConfig->majortype == MEDIATYPE_Video) && (pmtConfig->formattype == FORMAT_VideoInfo) && (pmtConfig->cbFormat >= sizeof(VIDEOINFOHEADER)) && (pmtConfig->pbFormat != NULL))
            {
                pVih2 = (VIDEOINFOHEADER *)pmtConfig->pbFormat;
                ret = get_format_type(pVih2);
                if (ret.flag)
                {
                    format_list.push_back(ret);
                }
            }
        }
    }
    pConfig->Release();
    pCapture->Release();

    if (int(format_list.size() > 0))
    {
        std::string type = "";
        int type_cnt = 0;
        for (int i = 0; i < int(format_list.size()); i++)
        {
            if (i == 0)
            {
                std::cout << "ioctl: VIDIOC_ENUM_FMT" << std::endl;
                std::cout << "	Type: Video Capture" << std::endl;
                std::cout << std::endl;
            }

            if (type != format_list[i].type)
            {
                type = format_list[i].type;
                std::cout << "	[" << type_cnt << "]: '" << type << "' (" << type << ")" << std::endl;
                type_cnt++;
            }

            std::cout << "		Size: Discrete " << format_list[i].width << "x" << format_list[i].height << std::endl;
            std::cout << std::fixed;
            std::cout << "		      Interval: Discrete " << std::setprecision(3) << 1 / format_list[i].fps << "s (" << std::setprecision(3) << format_list[i].fps << " fps)" << std::endl;
        }
        std::cout << std::endl;
    }
}

video_formats ClsDirectShow::get_format_type(VIDEOINFOHEADER *video)
{
    double ns = 100 * 1.0e-9;
    double frame = 1 / (double(video->AvgTimePerFrame) * ns);
    int width = video->bmiHeader.biWidth;
    int height = video->bmiHeader.biHeight;

    std::string format;
    std::vector<std::string> format_types;
    std::string line;
    std::string file_path;
    video_formats v_formats;

    char Path[MAX_PATH + 1];
    char drive[MAX_PATH + 1], dir[MAX_PATH + 1], fname[MAX_PATH + 1], ext[MAX_PATH + 1];

    GetModuleFileName(NULL, Path, MAX_PATH);
    _splitpath(Path, drive, dir, fname, ext); // パス名を構成要素に分解します
    // printf("完全パス : %s\n", Path);
    // printf("ドライブ : %s\n", drive);
    // printf("ディレクトリ パス : %s\n", dir);
    // printf("ベース ファイル名 (拡張子なし) : %s\n", fname);
    // printf("ファイル名の拡張子 : %s\n", ext);

    file_path = std::string(dir) + "\\format_types.txt";
    std::replace(file_path.begin(), file_path.end(), '\\', '/');

    std::ifstream input_file(file_path.c_str());
    while (getline(input_file, line))
    {
        format_types.push_back(std::string(line));
    }
    input_file.close();

    for (int i = 0; i < (int)format_types.size(); i++)
    {
        char c1 = (byte)format_types[i][0];
        char c2 = (byte)format_types[i][1];
        char c3 = (byte)format_types[i][2];
        char c4 = (byte)format_types[i][3];
        if (video->bmiHeader.biCompression == MAKEFOURCC(c1, c2, c3, c4))
        {
            format = format_types[i];
            break;
        }
    }

    v_formats.width = width;
    v_formats.height = height;
    v_formats.fps = frame;
    v_formats.flag = true;

    if (format != "")
    {
        v_formats.type = format;
    }
    else
    {
        v_formats.type = std::to_string(video->bmiHeader.biCompression);
    }
    return v_formats;
}

void ClsDirectShow::get_camera_settings(int device_num, IEnumMoniker *pClassEnum)
{
    ULONG cFetched;
    IMoniker *pMoniker = NULL;
    int n = 0;
    pClassEnum->Reset();

    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK)
    {
        if (device_num == n)
        {
            IBaseFilter *pbf = NULL;
            pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pbf);
            get_user_controls(pbf);
            get_camera_controls(pbf);
            return;
        }
        n++;
    }
}

void ClsDirectShow::get_user_controls(IBaseFilter *pbf)
{
    std::vector<std::string> proc_user = {"brightness",
                                          "contrast",
                                          "hue",
                                          "saturation",
                                          "sharpness",
                                          "gamma",
                                          "colorEnable",
                                          "whitebalance",
                                          "backlight-compensation",
                                          "gain"};

    IAMVideoProcAmp *pProcAmp = 0;
    HRESULT hr = pbf->QueryInterface(IID_IAMVideoProcAmp, (void **)&pProcAmp);

    if (FAILED(hr))
    {
        return;
    }

    std::cout << "User Controls" << std::endl;
    std::cout << std::endl;

    for (int proc_amp = 0; proc_amp < 10; proc_amp++)
    {
        long Min, Max, Step, Default, Flags, AutoFlags, Val;

        // Get the range and default value.
        hr = pProcAmp->GetRange(proc_amp, &Min, &Max, &Step, &Default, &AutoFlags);
        if (SUCCEEDED(hr))
        {
            // Get the current value.
            hr = pProcAmp->Get(proc_amp, &Val, &Flags);
        }
        if (SUCCEEDED(hr))
        {
            std::string str_proc = "";

            if (AutoFlags == 3)
            {
                int auto_val = Flags;
                if (Flags == 2)
                {
                    auto_val = 0;
                }
                str_proc = "";
                for (int i = int(proc_user[proc_amp].size()) + 10; i < 22; i++)
                {
                    str_proc += " ";
                }

                std::cout << "\t" << str_proc << proc_user[proc_amp] << "_automatic (bool)"
                          << "\t: default=" << 1 << " value=" << auto_val << std::endl;
            }

            str_proc = "";
            for (int i = int(proc_user[proc_amp].size()); i < 22; i++)
            {
                str_proc += " ";
            }

            std::cout << "\t" << str_proc << proc_user[proc_amp] << " (int) \t: min=" << Min << " max=" << Max << " step=" << Step << " default=" << Default << " value=" << Val << std::endl;
        }
    }

    std::cout << std::endl;
}

void ClsDirectShow::get_camera_controls(IBaseFilter *pbf)
{
    std::vector<std::string> proc_contrl = {"pan",
                                            "tilt",
                                            "roll",
                                            "zoom",
                                            "exposure",
                                            "iris",
                                            "focus"};

    IAMCameraControl *pCamCtl = 0;
    HRESULT hr = pbf->QueryInterface(IID_IAMCameraControl, (void **)&pCamCtl);

    if (FAILED(hr))
    {
        return;
    }

    std::cout << "Camera Controls" << std::endl;
    std::cout << std::endl;

    for (int camctl_num = 0; camctl_num < 7; camctl_num++)
    {
        long Min, Max, Step, Default, Flags, AutoFlags, Val;

        // Get the range and default value.
        hr = pCamCtl->GetRange(camctl_num, &Min, &Max, &Step, &Default, &AutoFlags);
        if (SUCCEEDED(hr))
        {
            // Get the current value.
            hr = pCamCtl->Get(camctl_num, &Val, &Flags);
        }
        if (SUCCEEDED(hr))
        {
            std::string str_proc = "";

            if (AutoFlags == 3)
            {
                int auto_val = Flags;
                if (Flags == 2)
                {
                    auto_val = 0;
                }
                str_proc = "";
                for (int i = int(proc_contrl[camctl_num].size()) + 10; i < 22; i++)
                {
                    str_proc += " ";
                }

                std::cout << "\t" << str_proc << proc_contrl[camctl_num] << "_automatic (bool)"
                          << "\t: default=" << 1 << " value=" << auto_val << std::endl;
            }

            str_proc = "";
            for (int i = int(proc_contrl[camctl_num].size()); i < 22; i++)
            {
                str_proc += " ";
            }

            std::cout << "\t" << str_proc << proc_contrl[camctl_num] << " (int) \t: min=" << Min << " max=" << Max << " step=" << Step << " default=" << Default << " value=" << Val << std::endl;
        }
    }
    std::cout << std::endl;
}

void ClsDirectShow::set_camera_settings(int device_num, IEnumMoniker *pClassEnum, std::string proc, int val)
{
    ULONG cFetched;
    IMoniker *pMoniker = NULL;
    int n = 0;
    pClassEnum->Reset();

    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK)
    {
        if (device_num == n)
        {
            IBaseFilter *pbf = NULL;
            pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pbf);
            set_user_controls(pbf, proc, val);
            // set_camera_controls(pbf);
            return;
        }
        n++;
    }
}

void ClsDirectShow::set_user_controls(IBaseFilter *pbf, std::string prop, int val)
{
    int prop_index = -1;
    bool auto_flag = false;

    if (prop.find("_automatic") != std::string::npos)
    {
        prop = replaceOtherStr(prop, "_automatic", "");
        auto_flag = true;
    }

    std::vector<std::string> proc_user = {"brightness",
                                          "contrast",
                                          "hue",
                                          "saturation",
                                          "sharpness",
                                          "gamma",
                                          "colorEnable",
                                          "whitebalance",
                                          "backlight-compensation",
                                          "gain"};

    for (int i = 0; i < int(proc_user.size()); i++)
    {
        if (proc_user[i] == prop)
        {
            prop_index = i;
            break;
        }
    }

    if (prop_index != -1)
    {
        long Val, Flags;
        IAMVideoProcAmp *pProcAmp = 0;
        HRESULT hr = pbf->QueryInterface(IID_IAMVideoProcAmp, (void **)&pProcAmp);

        if (FAILED(hr))
        {
            return;
        }

        if (auto_flag)
        {
            hr = pProcAmp->Get(prop_index, &Val, &Flags);

            if (val == 0)
            {
                Flags = VideoProcAmp_Flags_Manual;
            }
            else if (val == 1)
            {
                Flags = VideoProcAmp_Flags_Auto;
            }
            hr = pProcAmp->Set(prop_index, Val, Flags);
        }
        else
        {
            hr = pProcAmp->Get(prop_index, &Val, &Flags);
            hr = pProcAmp->Set(prop_index, val, Flags);
        }

        if (SUCCEEDED(hr))
        {
            printf("success");
        }
    }
    else
    {
        std::vector<std::string> proc_contrl = {"pan",
                                                "tilt",
                                                "roll",
                                                "zoom",
                                                "exposure",
                                                "iris",
                                                "focus"};

        for (int i = 0; i < int(proc_contrl.size()); i++)
        {
            if (proc_contrl[i] == prop)
            {
                prop_index = i;
                break;
            }
        }

        long Val, Flags;
        IAMCameraControl *pCamCtl = 0;
        HRESULT hr = pbf->QueryInterface(IID_IAMCameraControl, (void **)&pCamCtl);

        if (FAILED(hr))
        {
            return;
        }

        if (auto_flag)
        {
            hr = pCamCtl->Get(prop_index, &Val, &Flags);

            if (val == 0)
            {
                Flags = VideoProcAmp_Flags_Manual;
            }
            else if (val == 1)
            {
                Flags = VideoProcAmp_Flags_Auto;
            }
            hr = pCamCtl->Set(prop_index, Val, val);
        }
        else
        {
            hr = pCamCtl->Set(prop_index, val, Flags);
        }

        if (SUCCEEDED(hr))
        {
            printf("success");
        }
    }
}

std::string ClsDirectShow::replaceOtherStr(std::string &replacedStr, std::string from, std::string to)
{
    const unsigned int pos = replacedStr.find(from);
    const int len = from.length();

    if (pos == std::string::npos || from.empty())
    {
        return replacedStr;
    }

    return replacedStr.replace(pos, len, to);
}
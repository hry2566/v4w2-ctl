#include <iostream>
#include <dshow.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iomanip>

// #pragma comment(lib, "strmiids.lib")

struct video_formats
{
    int width;
    int height;
    double fps;
    std::string type;
    bool flag = false;
};

std::string replaceOtherStr(std::string &replacedStr, std::string from, std::string to);
IEnumMoniker *video_init(ICreateDevEnum *pDevEnum);
void get_devices_list(IEnumMoniker *pClassEnum);
void get_videoformats_list(int device_num, IEnumMoniker *pClassEnum);
void get_config(IBaseFilter *pbf);
video_formats get_format_type(VIDEOINFOHEADER *video);
void show_help();

int main(int argc, char *argv[])
{
    // debug用
    // ICreateDevEnum *pDevEnum = NULL;
    // IEnumMoniker *pClassEnum = NULL;
    // pClassEnum = video_init(pDevEnum);
    // get_videoformats_list(0, pClassEnum);
    // CoUninitialize();

    if (argc == 1)
    {
        show_help();
        return 0;
    }

    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pClassEnum = NULL;

    pClassEnum = video_init(pDevEnum);

    if (pClassEnum != NULL)
    {
        if (argc == 2)
        {
            std::string cmd1(argv[1]);
            if (cmd1 == "-h")
            {
                show_help();
            }
            else if (cmd1 == "--list-devices")
            {
                get_devices_list(pClassEnum);
            }
            else if (cmd1 == "--list-formats-ext")
            {
                get_videoformats_list(0, pClassEnum);
            }
        }
        else if (argc == 4)
        {
            std::string cmd1(argv[1]);
            std::string cmd2(argv[2]);
            std::string cmd3(argv[3]);
            int device_num = 0;

            if (cmd2.find("/dev/video") != std::string::npos)
            {
                cmd2 = replaceOtherStr(cmd2, "/dev/video", "");
            }

            if (cmd1 == "-d")
            {
                device_num = atoi(cmd2.c_str());
                // device_num = atoi(cmd2.c_str());
            }
            if (cmd3 == "--list-formats-ext")
            {
                get_videoformats_list(device_num, pClassEnum);
            }
        }
    }

    // pDevEnum->Release();
    // pClassEnum->Release();
    CoUninitialize();
    return 0;
}

std::string replaceOtherStr(std::string &replacedStr, std::string from, std::string to)
{
    const unsigned int pos = replacedStr.find(from);
    const int len = from.length();

    if (pos == std::string::npos || from.empty())
    {
        return replacedStr;
    }

    return replacedStr.replace(pos, len, to);
}

IEnumMoniker *video_init(ICreateDevEnum *pDevEnum)
{

    if (FAILED(CoInitialize(NULL)))
    {
        printf("error: COMの初期化に失敗しました");
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

void get_devices_list(IEnumMoniker *pClassEnum)
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

        // デバイス名列挙
        pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pP);
        pP->Read(L"FriendlyName", &var, 0);
        device_name = var.bstrVal;
        VariantClear(&var);

        pP->Read(L"DevicePath", &var, 0);
        device_path = var.bstrVal;
        VariantClear(&var);

        std::wcout << device_name << " (" << device_path << "):" << std::endl;
        std::wcout << "        /dev/video" << n << std::endl;
        n++;
    }
    return;
}

void get_videoformats_list(int device_num, IEnumMoniker *pClassEnum)
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
        }
        n++;
    }
    return;
}

void get_config(IBaseFilter *pbf)
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
            }

            if (type != format_list[i].type)
            {
                type = format_list[i].type;
                std::cout << "	Index       : " << type_cnt << std::endl;
                std::cout << "	Type        : Video Capture" << std::endl;
                std::cout << "	Pixel Format: '" << type << "'" << std::endl;
                std::cout << "	Name        : " << type << std::endl;
                type_cnt++;
            }

            std::cout << "		Size: Discrete " << format_list[i].width << "x" << format_list[i].height << std::endl;
            std::cout << std::fixed;
            std::cout << "			Interval: Discrete " << std::setprecision(3) << 1 / format_list[i].fps << "s (" << std::setprecision(3) << format_list[i].fps << " fps)" << std::endl;
        }
    }
}

video_formats get_format_type(VIDEOINFOHEADER *video)
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
    if (!input_file.is_open())
    {
        return v_formats;
    }

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

void show_help()
{
    std::vector<std::string> help;
    help.push_back("General/Common options:");
    help.push_back("    -d, --device=<dev> use device <dev> instead of /dev/video0");
    help.push_back("    -h, --help         display this help message");
    help.push_back("    --list-devices     list all v4w devices");
    help.push_back("    --list-formats-ext display supported video formats including frame sizes");
    help.push_back("");

    for (int i = 0; i < (int)help.size(); i++)
    {
        printf("%s\n", help[i].c_str());
    }
}

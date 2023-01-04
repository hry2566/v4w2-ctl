#include <iostream>
#include <dshow.h>
#include <vector>
#include <fstream>
#include <algorithm>

// #pragma comment(lib, "strmiids.lib")

IEnumMoniker* video_init(ICreateDevEnum* pDevEnum);
void get_devices_list(IEnumMoniker* pClassEnum);
void get_videoformats_list(int device_num, IEnumMoniker* pClassEnum);
void get_config(IBaseFilter *pbf);
void get_format_type(VIDEOINFOHEADER* video);
void show_help();

int main(int argc, char *argv[])
{
    if(argc==1){
        show_help();
        return 0;
    }

    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker* pClassEnum = NULL;

    pClassEnum = video_init(pDevEnum);

    if(pClassEnum!=NULL){
        if(argc==2){
            std::string cmd1(argv[1]);
            if(cmd1=="-h") {
                show_help();
            }else if(cmd1=="--list-devices"){
                get_devices_list(pClassEnum);
            }else if(cmd1=="--list-formats-ext"){
                get_videoformats_list(0, pClassEnum);
            }
        }else if(argc==4){
            std::string cmd1(argv[1]);
            std::string cmd2(argv[2]);
            std::string cmd3(argv[3]);
            int device_num = 0;

            if(cmd1=="-d"){
                device_num = atoi(cmd2.c_str());
            }
            if(cmd3=="--list-formats-ext"){
                get_videoformats_list(device_num, pClassEnum);
            }
        }
    }

    pDevEnum->Release();
    pClassEnum->Release();
    CoUninitialize();
    return 0;
}



IEnumMoniker* video_init(ICreateDevEnum* pDevEnum){

    if(FAILED(CoInitialize(NULL))){
        printf("error: COMの初期化に失敗しました");
        return NULL;
    }

    // Create the System Device Enumerator.
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr)){
        // Create an enumerator for the category.
        IEnumMoniker* pClassEnum = NULL;
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
        if (hr == S_FALSE){
            return NULL;
        }
        return pClassEnum;
    }
    return NULL;
}

void get_devices_list(IEnumMoniker* pClassEnum)
{
    ULONG cFetched;
    IMoniker* pMoniker = NULL;
    int n = 0;
    pClassEnum->Reset();

    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
        IPropertyBag* pP = NULL;
        VARIANT var;
        var.vt = VT_BSTR;
        BSTR device_name;
        BSTR device_path;

        // デバイス名列挙
        pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pP);
        pP->Read(L"FriendlyName", &var, 0);
        device_name = var.bstrVal;
        VariantClear(&var);

        pP->Read(L"DevicePath", &var, 0);
        device_path = var.bstrVal;
        VariantClear(&var);

        printf("[%i] %S %S\n", n, device_name, device_path);
        n++;
    }
    return;
}

void get_videoformats_list(int device_num, IEnumMoniker* pClassEnum){

    ULONG cFetched;
    IMoniker* pMoniker = NULL;
    int n = 0;
    pClassEnum->Reset();

    while (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
        if(device_num==n){
            IBaseFilter* pbf = NULL;
            pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pbf);
            get_config(pbf);
        }
        n++;
    }
    return;
}

void get_config(IBaseFilter *pbf){
    ICaptureGraphBuilder2 *pCapture = NULL;
    CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **) &pCapture);
    IAMStreamConfig *pConfig = NULL;
    HRESULT hr = pCapture->FindInterface(&PIN_CATEGORY_CAPTURE, 0, pbf, IID_IAMStreamConfig, (void**)&pConfig);

    int iCount=0;
    int iSize=0;
    hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
    if(iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)){
        for(int iFormat=0; iFormat<iCount; iFormat++){
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE *pmtConfig;
            hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
            VIDEOINFOHEADER *pVih2;

            if((SUCCEEDED(hr)
                && pmtConfig->majortype == MEDIATYPE_Video)
                && (pmtConfig->formattype == FORMAT_VideoInfo)
                && (pmtConfig->cbFormat >= sizeof(VIDEOINFOHEADER))
                && (pmtConfig->pbFormat != NULL)){

                pVih2 = (VIDEOINFOHEADER*)pmtConfig->pbFormat;
                get_format_type(pVih2);
            }
        }
    }
    pConfig->Release();
    pCapture->Release();
}


void get_format_type(VIDEOINFOHEADER* video){
    double ns = 100 * 1.0e-9;
    double frame = 1 / (double(video->AvgTimePerFrame)*ns);
    int width =video->bmiHeader.biWidth;
    int height = video->bmiHeader.biHeight;

    std::string format;
    std::vector<std::string> format_types;
    std::string line;
    std::string file_path;

    char cdir[255];
    GetCurrentDirectory(255,cdir);
    file_path = std::string(cdir) + "\\format_types.txt";
    std::replace(file_path.begin(), file_path.end(), '\\', '/');

    std::ifstream input_file(file_path.c_str());
    //std::ifstream input_file("./format_types.txt");
    if (!input_file.is_open()) {
        //printf("non file\n");
        //return;
    }

    while (getline(input_file, line)){
        format_types.push_back(std::string(line));
    }
    input_file.close();

    for(int i =0; i<(int)format_types.size(); i++)
    {
        char c1 = (byte)format_types[i][0];
        char c2 = (byte)format_types[i][1];
        char c3 = (byte)format_types[i][2];
        char c4 = (byte)format_types[i][3];
        if (video->bmiHeader.biCompression==MAKEFOURCC(c1,c2,c3,c4)){
            format =format_types[i];
            break;
        }
    }
    if (format != ""){
        printf("%dx%d %1.0lf %s\n", width, height, frame, format.c_str());
    }else{
        printf("%dx%d %1.0lf %lu\n", width, height, frame, video->bmiHeader.biCompression);
    }

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

    for(int i = 0; i<(int)help.size(); i++){
        printf("%s\n",help[i].c_str());
    }
}


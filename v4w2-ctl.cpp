#include <iostream>
#include <dshow.h>
#include <vector>
#include "ClsDirectShow.h"

// #pragma comment(lib, "strmiids.lib")

std::vector<std::string> split(std::string str, char del);
std::string replaceOtherStr(std::string &replacedStr, std::string from, std::string to);
void show_help();

int main(int argc, char *argv[])
{
    // debug用
    // ICreateDevEnum *pDevEnum = NULL;
    // IEnumMoniker *pClassEnum = NULL;
    // ClsDirectShow dshow;

    // pClassEnum = dshow.video_init(pDevEnum);
    // dshow.get_devices_list(pClassEnum);
    // dshow.get_videoformats_list(0, pClassEnum);
    // dshow.get_camera_settings(0, pClassEnum);
    // dshow.set_camera_settings(0, pClassEnum, "whiteBalance_automatic", 0);

    if (argc == 1)
    {
        show_help();
        return 0;
    }

    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pClassEnum = NULL;
    ClsDirectShow dshow;

    //    pClassEnum = video_init(pDevEnum);
    pClassEnum = dshow.video_init(pDevEnum);

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
                dshow.get_devices_list(pClassEnum);
            }
            else if (cmd1 == "--list-formats-ext")
            {
                dshow.get_videoformats_list(0, pClassEnum);
            }
            else if (cmd1 == "-L")
            {
                dshow.get_camera_settings(0, pClassEnum);
            }
        }
        else if (argc == 3)
        {
            std::string cmd1(argv[1]);
            std::string cmd2(argv[2]);

            if (cmd1.find("-c") != std::string::npos)
            {
                std::vector<std::string> spl = split(cmd2, '=');
                dshow.set_camera_settings(0, pClassEnum, spl[0], atoi(spl[1].c_str()));
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
            }
            if (cmd3 == "--list-formats-ext")
            {
                dshow.get_videoformats_list(device_num, pClassEnum);
            }
            else if (cmd3 == "-L")
            {
                dshow.get_camera_settings(device_num, pClassEnum);
            }
        }
        else if (argc == 5)
        {
            std::string cmd1(argv[1]);
            std::string cmd2(argv[2]);
            std::string cmd3(argv[3]);
            std::string cmd4(argv[4]);
            int device_num = 0;

            if (cmd2.find("/dev/video") != std::string::npos)
            {
                cmd2 = replaceOtherStr(cmd2, "/dev/video", "");
            }

            if (cmd1 == "-d")
            {
                device_num = atoi(cmd2.c_str());
            }

            if (cmd3.find("-c") != std::string::npos)
            {
                std::vector<std::string> spl = split(cmd4, '=');
                dshow.set_camera_settings(device_num, pClassEnum, spl[0], atoi(spl[1].c_str()));
            }
        }
    }
    //     pDevEnum->Release();
    //     pClassEnum->Release();
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

std::vector<std::string> split(std::string str, char del)
{
    int first = 0;
    int last = str.find_first_of(del);

    std::vector<std::string> result;

    while (first < int(str.size()))
    {
        std::string subStr(str, first, last - first);

        result.push_back(subStr);

        first = last + 1;
        last = str.find_first_of(del, first);

        if (last == int(std::string::npos))
        {
            last = str.size();
        }
    }

    return result;
}

void show_help()
{
    std::vector<std::string> help;
    help.push_back("General/Common options:");
    help.push_back("    -c, --set-ctrl <ctrl>=<val>[,<ctrl>=<val>...]");
    help.push_back("    -d, --device=<dev> use device <dev> instead of /dev/video0");
    help.push_back("    -h, --help         display this help message");
    help.push_back("    -L, --list-ctrls-menus");
    help.push_back("		     display all controls and their menus [VIDIOC_QUERYMENU]");
    help.push_back("    --list-devices     list all v4w devices");
    help.push_back("    --list-formats-ext display supported video formats including frame sizes");
    help.push_back("");

    for (int i = 0; i < (int)help.size(); i++)
    {
        printf("%s\n", help[i].c_str());
    }
}

#ifndef _MAINPAGE_H_
#define _MAINPAGE_H_

#include <thread>
#include <string>
#include <vector>
#include <string.h>
#include <mutex>

#include "MainPageUI.h"

using namespace std;

class MainPage;

class MainPage
{
public:
    struct AppInfo
    {
        string name;   // 应用程序名称
        string exec;   // 应用程序执行文件
        string argv;   // 应用程序参数
        string icon;   // 应用程序icon(bin)
        string config; // 应用程序配置文件(json)
    };

    struct SysConfig
    {
        int brightness;            // 保存亮度
        int volume;                // 保存音量
        string mainbgFile;         // 主界面壁纸文件
        string weatherKey;         // 天气key
        vector<AppInfo> appVector; // 欲安装的应用程序信息
    };

    struct SysConfigMem // 用于共享内存的系统配置
    {
        int magic;
        int brightness;
        int volume;
        char mainbgFile[32];
        char weatherKey[32];
        
        int appNum;
        struct AppInfoMem
        {
            char name[32];
            char config[32];
        } appInfo[0];
    };

    static constexpr int shmKey = 114486;

private:
    int shmId; // 共享内存id

    int tick; // 计数值
    mutex *uiMutex;
    string bgFile;       // 主界面背景图片文件名
    thread *pThread;     // 独立于UI的线程
    bool threadExitFlag; // 线程退出标志

    MainPageUI::Operations uiOpts; // ui回调函数

    SysConfig sysConfig;           // 配置信息
    SysConfigMem *configMem; // 共享内存配置信息

    int legalConfigAppNum;  // 配置文件有效的app个数

    bool readConfig(void);
    void saveConfig(void);
    bool getWeather(MainPageUI::WeatherInfo &weahter);
    void getSysTime(MainPageUI::TimeInfo &sysTime);
    void installApplications(vector<AppInfo> &appVector);
    void *createShareMem(size_t memSize);

    int mainThreadFunction(void);
    int bootThreadFunction(void);
    void runApplication(const char *exec, char *const argv[]);

    static char **stringToArgv(const char *exec, string &str);

public:
    MainPage(function<void(void)> exitCb, mutex &UIMutex);
    ~MainPage();
};

#endif
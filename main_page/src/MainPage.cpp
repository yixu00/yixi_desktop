#include "MainPage.h"
#include <unistd.h>
// #include "curl/curl.h"
#include <fstream>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <map>

#include "src/cJSON.h"
// #include "src/sys.h"

#define CONFIG_DIR "configs/"
#define CONFIG_FILE "sysconfig.json"

#define INSTALL_WIRELESS_CMD_1 "modprobe mac80211.ko"
#define INSTALL_WIRELESS_CMD_2 "modprobe rtl8xxxu.ko"
#define WLAN_UP_CMD "ifconfig wlan0 up"
#define WPA_SERVER_CMD "wpa_supplicant -Dwext -c /etc/wpa_supplicant.conf -i wlan0 -B"
#define WPA_UDHCCP "udhcpc -b -i wlan0 -q"

#define WEATHER_NOW_API "https://api.seniverse.com/v3/weather/now.json?key=%s&location=ip&language=zh-Hans&unit=c"
#define WEATHER_DAILY_API "https://api.seniverse.com/v3/weather/daily.json?key=%s&location=ip&language=zh-Hans&unit=c&start=0&days=1"

// static string curlHTTPGet(const char *url);
// static ssize_t curlWriteDataToString(void *buf, size_t size, size_t nmemb, void *arg);

static const char *configNumberItemName[] =
    {
        "brightness",
        "volume",
};

static const char *configStringItemName[] =
    {
        "mainbg",
        "weather_key",
};

static const char *appInfoItemName[] =
    {
        "name",
        "exec",
        "argv",
        "icon",
        "config",
};

static const char *weatherItemName[] =
    {
        "text",
        "code",
        "temperature",
};

static const char *weatherDailyItemName[] =
    {
        "wind_direction",
        "high",
        "low",
        "wind_scale",
        "rainfall",
        "humidity",
};

/***
 *@brief MainPage构造函数
 *@param exit_cb 退出进程回调函数
 */
MainPage::MainPage(function<void(void)> exitCb, mutex &UIMutex)
{
  threadExitFlag = false;

  uiOpts.exitCb = exitCb;
  uiOpts.runApp = std::bind(&MainPage::runApplication, this, std::placeholders::_1, std::placeholders::_2);

  tick = 0;
  legalConfigAppNum = 0;
  uiMutex = &UIMutex;

  // curl_global_init(CURL_GLOBAL_ALL); // CURL全局初始化

  pThread = new thread(&MainPage::mainThreadFunction, this); // 创建主UI执行线程
}

MainPage::~MainPage()
{
  threadExitFlag = true;
  pThread->join();
  delete pThread;

  // curl_global_cleanup(); // CURL全局释放

  MainPageUI::release();

  shmdt((void *)configMem);
  shmctl(shmId, IPC_RMID, 0); // 删除共享内存
}

/**
 *@brief 读取设置信息
 *@return true - 读取到了配置信息  false - 缺省值配置信息
 */
bool MainPage::readConfig(void)
{
  ifstream file;
  legalConfigAppNum = 0;

  file.open(CONFIG_DIR CONFIG_FILE, ios::in);

  if (file.is_open() != true)
  {
    sysConfig.brightness = 50; // 缺省值
    sysConfig.volume = 50;
    sysConfig.mainbgFile = "mainbg_1.bin";
    sysConfig.weatherKey = "";

    AppInfo info = {.name = "设置", .exec = "setting", .argv = "<null>", .icon = "setting_icon.bin", .config = ""};
    sysConfig.appVector.push_back(info);

    return false;
  }

  char *buf = new char[4096];
  memset(buf, 0, 4096);
  file.read(buf, 4096);
  file.close();

  cJSON *cjson = cJSON_Parse(buf); // 打包cJSON数据格式
  if (cjson != nullptr)
  {
    int *value[] = {&sysConfig.brightness, &sysConfig.volume}; // 数值参数
    for (int i = 0; i < sizeof(value) / sizeof(value[0]); i++)
    {
      cJSON *item = cJSON_GetObjectItem(cjson, configNumberItemName[i]);
      if (item != nullptr)
        *(value[i]) = item->valueint;
    }

    string *config_str[] = {&sysConfig.mainbgFile, &sysConfig.weatherKey}; // 字符串参数
    for (int i = 0; i < sizeof(config_str) / sizeof(config_str[0]); i++)
    {
      cJSON *item = cJSON_GetObjectItem(cjson, configStringItemName[i]);
      if (item != nullptr)
        *(config_str[i]) = string(item->valuestring);
    }

    printf("param sysConfig end\n");

    // 应用程序
    cJSON *applications = cJSON_GetObjectItem(cjson, "applications");
    int array_size = cJSON_GetArraySize(applications);
    for (int i = 0; i < array_size; i++)
    {
      cJSON *app_info = cJSON_GetArrayItem(applications, i);

      AppInfo info;
      string *app_str[] = {&info.name, &info.exec, &info.argv, &info.icon, &info.config};
      for (int j = 0; j < sizeof(app_str) / sizeof(app_str[0]); j++)
      {
        cJSON *item = cJSON_GetObjectItem(app_info, appInfoItemName[j]);
        if (item != nullptr && item->type != cJSON_NULL)
          *(app_str[j]) = string(item->valuestring);
      }

      if(info.config != "")
        ++legalConfigAppNum;

      sysConfig.appVector.push_back(info); // 向容器插入一个元素
    }
    printf("applications sysConfig end\n");

    cJSON_Delete(cjson);
  }
  delete[] buf;

  return true;
}

/**
 *@brief 保存设置信息
 *@param sysConfig 保存的设置信息
 */
void MainPage::saveConfig(void)
{
  cJSON *cjson = cJSON_CreateObject();

  const int *value[] = {&sysConfig.brightness, &sysConfig.volume};
  for (int i = 0; i < sizeof(value) / sizeof(value[0]); i++)
    cJSON_AddNumberToObject(cjson, configNumberItemName[i], *value[i]);

  const string *configString[] = {&sysConfig.mainbgFile, &sysConfig.weatherKey}; // 字符串参数
  for (int i = 0; i < sizeof(configString) / sizeof(configString[0]); i++)
    cJSON_AddStringToObject(cjson, configStringItemName[i], configString[i]->c_str());

  cJSON *applications = cJSON_CreateArray();

  for (AppInfo &info : sysConfig.appVector)
  {
    cJSON *appInfo = cJSON_CreateObject();

    cJSON_AddStringToObject(appInfo, appInfoItemName[0], info.name.c_str());
    cJSON_AddStringToObject(appInfo, appInfoItemName[1], info.exec.c_str());
    cJSON_AddStringToObject(appInfo, appInfoItemName[2], info.argv.c_str());
    cJSON_AddStringToObject(appInfo, appInfoItemName[3], info.icon.c_str());
    cJSON_AddStringToObject(appInfo, appInfoItemName[4], info.config.c_str());

    cJSON_AddItemToArray(applications, appInfo);
  }

  cJSON_AddItemToObject(cjson, "applications", applications);

  string jsonString(cJSON_Print(cjson));

  ofstream file;

  file.open(CONFIG_DIR CONFIG_FILE, ios::out); // 写方式打开文件

  file << jsonString << endl;

  file.close();

  cJSON_Delete(cjson);
}

/**
 *@brief 获取天气数据
 *@return 成功 - true  失败 - false
 */
// bool MainPage::getWeather(MainPageUI::WeatherInfo &weather)
// {
//   bool ret = false;
//   char *api = new char[256];
//   sprintf(api, WEATHER_NOW_API, sysConfig.weatherKey.c_str());

//   string str = curlHTTPGet(api); // 获取实况天气

//   cJSON *cjson = cJSON_Parse(str.c_str()); // 打包cJSON数据格式
//   if (cjson != nullptr)
//   {
//     cJSON *result = cJSON_GetObjectItem(cjson, "results");
//     cJSON *result0 = cJSON_GetArrayItem(result, 0);
//     cJSON *now = cJSON_GetObjectItem(result0, "now");
//     cJSON *location = cJSON_GetObjectItem(result0, "location");

//     cJSON *position = cJSON_GetObjectItem(location, "name");
//     if (position != nullptr)
//       strcpy(weather.position, position->valuestring);
//     else
//       strcpy(weather.position, "定位失败");

//     {
//       char *dataString[] = {weather.text};
//       int *dataValue[] = {&weather.code, &weather.temp};

//       for (int i = 0; i < 3; i++)
//       {
//         if (now != nullptr)
//         {
//           cJSON *data = cJSON_GetObjectItem(now, weatherItemName[i]);
//           if (data == nullptr)
//             continue;

//           if (i == 0)
//             strcpy(dataString[i], data->valuestring);
//           else
//             *(dataValue[i - 1]) = atoi(data->valuestring);
//         }
//       }
//     }
//     cJSON_Delete(cjson);
//   }

//   sprintf(api, WEATHER_DAILY_API, sysConfig.weatherKey.c_str());
//   str = curlHTTPGet(api);           // 获取今日天气
//   cjson = cJSON_Parse(str.c_str()); // 打包cJSON数据格式
//   if (cjson != nullptr)
//   {
//     cJSON *result = cJSON_GetObjectItem(cjson, "results");
//     cJSON *result_0 = cJSON_GetArrayItem(result, 0);
//     cJSON *daily = cJSON_GetObjectItem(result_0, "daily");
//     cJSON *today = cJSON_GetArrayItem(daily, 0);

//     char *dataString[] = {weather.windDir};
//     int *dataValue[] = {&weather.maxTemp, &weather.minTemp,
//                         &weather.windLevel, &weather.rainFall, &weather.humi};

//     for (int i = 0; i < 6; i++)
//     {
//       cJSON *data = cJSON_GetObjectItem(today, weatherDailyItemName[i]);
//       if (data == nullptr)
//         continue;

//       if (i == 0)
//         strcpy(dataString[i], data->valuestring);
//       else
//         *(dataValue[i - 1]) = atoi(data->valuestring);
//     }
//     cJSON_Delete(cjson);
//     weather.rainFall *= 100; // 扩大100倍

//     ret = true;
//   }

//   delete[] api;

//   return ret;
// }

/**
 *@brief 获取系统时间
 *@return 返回时间数据
 */
void MainPage::getSysTime(MainPageUI::TimeInfo &sysTime)
{
  time_t seconds = time(nullptr);

  struct tm *pstTIme = localtime(&seconds);
  sysTime.year = pstTIme->tm_year + 1900;
  sysTime.month = pstTIme->tm_mon + 1;
  sysTime.day = pstTIme->tm_mday;
  sysTime.hour = pstTIme->tm_hour;
  sysTime.min = pstTIme->tm_min;
  sysTime.sec = pstTIme->tm_sec;
  sysTime.week = pstTIme->tm_wday;
}

/**
 *@brief 安装应用程序
 *@param apps 应用程序表
 */
void MainPage::installApplications(vector<AppInfo> &appVector)
{
  for (AppInfo &info : appVector)
  {
    int execLen = info.exec.length();
    int iconLen = info.icon.length();

    char *exec = new char[execLen + 3];
    char *icon = new char[iconLen + 14];
    const char *name = info.name.c_str();
    char **argv;

    sprintf(exec, "./%s", info.exec.c_str());
    sprintf(icon, "S:./res/icon/%s", info.icon.c_str());
    argv = stringToArgv(exec, info.argv);

    MainPageUI::addApplication((name), exec, argv, icon); // 添加应用程序到UI

    delete[] icon;
    delete[] exec;
  }
}

void *MainPage::createShareMem(size_t memSize)
{
  void *pMem = nullptr;

  shmId = shmget((key_t)shmKey, memSize, 0666 | IPC_CREAT);
  if (shmId == -1)
  {
    printf("shmget failed\n");
    return nullptr;
  }

  pMem = shmat(shmId, (void *)0, 0); // 将共享内存连接到当前进程的地址空间
  if (pMem == (void *)-1)
  {
    printf("shmat failed\n");
    return nullptr;
  }

  return pMem;
}

/**
 *@brief 主UI线程
 *@param obj 主界面对象
 */
int MainPage::mainThreadFunction(void)
{
  usleep(50000);
  MainPageUI::TimeInfo timeInfo;
  // MainPageUI::WeatherInfo weatherInfo;
  bool isGetWeatherSucceed = false;

  // 启动操作
  thread bootThread(&MainPage::bootThreadFunction, this); // 创建启动线程
  bootThread.join();                                      // 等待启动线程返回

  printf("boot thread return\n");

  usleep(50000);

  printf("main running\n");

  while (!threadExitFlag)
  {
    if (tick % 5 == 0)
    {
      getSysTime(timeInfo); // 获取系统时间(500ms)
      uiMutex->lock();
      MainPageUI::updateDateTime(timeInfo); // 更新时间显示
      uiMutex->unlock();
    }

    // if (tick == 1 || isGetWeatherSucceed == false) // 5分钟更新一次天气，第一次失败则一直尝试直到成功
    // {
    //   isGetWeatherSucceed = getWeather(weatherInfo);
    //   uiMutex->lock();
    //   MainPageUI::updateWeather(weatherInfo); // 更新天气显示
    //   uiMutex->unlock();
    // }

    if (++tick >= 300) // 10分钟
      tick = 0;

    usleep(100000); // 1 tick = 100ms
  }

  return 0;
}

/**
 *@brief 启动UI线程
 */
int MainPage::bootThreadFunction(void)
{
  uiMutex->lock();
  MainPageUI::bootCreate(); // 创建启动界面UI,创建于top layer上覆盖主界面UI
  uiMutex->unlock();
  usleep(50000);

  // 加载配置文件
  uiMutex->lock();
  MainPageUI::bootUpdate("正在加载配置文件...");
  uiMutex->unlock();
  if (readConfig() != true) // 读取配置文件
    saveConfig();           // 写入缺省信息到配置文件

  bgFile = sysConfig.mainbgFile;            // 设置背景图片
  // sys_set_brightness(sysConfig.brightness); // 设置背光

  // 创建共享内存并写入配置
  configMem = (SysConfigMem *)createShareMem(sizeof(SysConfigMem) + sizeof(SysConfigMem::AppInfoMem) * legalConfigAppNum);
  if (configMem != nullptr)
  {
    configMem->magic = 0;
    configMem->brightness = sysConfig.brightness;
    configMem->volume = sysConfig.volume;
    configMem->appNum = legalConfigAppNum;
    strcpy(configMem->mainbgFile, sysConfig.mainbgFile.c_str());
    strcpy(configMem->weatherKey, sysConfig.weatherKey.c_str());

    int i = 0;
    for (AppInfo &info : sysConfig.appVector)
    {
      if(info.config == "")
        continue;

      sprintf(configMem->appInfo[i].name, "%s", info.name.c_str());
      sprintf(configMem->appInfo[i].config, "%s%s", CONFIG_DIR, info.config.c_str());

      ++i;
    }
  }

  // 加载网卡驱动
  uiMutex->lock();
  MainPageUI::bootUpdate("正在加载网卡驱动...");
  uiMutex->unlock();
  system(INSTALL_WIRELESS_CMD_1);
  sleep(1);
  system(INSTALL_WIRELESS_CMD_2);
  sleep(1);

  // 唤醒网卡
  uiMutex->lock();
  MainPageUI::bootUpdate("正在启动网卡...");
  uiMutex->unlock();
  system(WLAN_UP_CMD);
  sleep(1);

  // 启动wpa_supplicant服务
  uiMutex->lock();
  MainPageUI::bootUpdate("正在启动无线网络服务...");
  uiMutex->unlock();
  system(WPA_SERVER_CMD);
  sleep(1);
  system(WPA_UDHCCP); // 获取IP

  // 添加应用程序
  uiMutex->lock();
  MainPageUI::bootUpdate("正在安装应用程序...");
  uiMutex->unlock();
  sleep(1);

  uiMutex->lock();
  MainPageUI::create(uiOpts, bgFile.c_str()); // 初始化主界面UI
  installApplications(sysConfig.appVector);
  uiMutex->unlock();

  uiMutex->lock();
  MainPageUI::bootUpdate("欢迎使用");
  uiMutex->unlock();
  sleep(1);

  uiMutex->lock();
  MainPageUI::bootRelease(); // 释放启动界面UI
  uiMutex->unlock();

  return 0;
}

/**
 *@brief 运行app回调函数
 *@brief exec app执行文件
 *@param app的main函数参数
 *@note 由于回调函数被UI线程(主线程)执行，因此会阻塞UI线程
 */
void MainPage::runApplication(const char *exec, char *const argv[])
{
  if (exec == nullptr)
    return;

  pid_t pid = fork(); // 创建子进程

  if (pid == 0) // 子进程
  {
    int ret = execv(exec, argv);
    if (ret < 0)
    {
      printf("create %s failed\n", exec);
      exit(0); // 子进程退出
    }
  }

  wait(nullptr); // 阻塞等待子进程返回
}

/**
 *@brief 将字符串参数转为 char**
 *@return 带有应用程序执行路径的完整argv
 *@最大支持5个参数
 */
char **MainPage::stringToArgv(const char *exec, string &str)
{
  int i = 0;
  size_t dataStart = 0;
  size_t dataEnd = 0;
  string dataStr = "";

  char **argv = new char *[5];

  int len = strlen(exec) + 1;
  argv[0] = new char[len];
  sprintf(argv[i++], "%s", exec);

  do
  {
    dataStart = str.find('<', dataEnd); // 寻找 < 字符
    if (dataStart != string::npos)
    {
      dataStart += 1;
      dataEnd = str.find('>', dataStart); // 寻找 >
      if (dataEnd != string::npos)
      {
        dataStr = str.substr(dataStart, dataEnd - dataStart);
        if (dataStr != "null")
        {
          int len = dataStr.length() + 1;
          argv[i] = new char[len];
          sprintf(argv[i], "%s", dataStr.c_str());
        }
        else
        {
          argv[i] = nullptr;
          break;
        }
      }
    }
  } while (++i < 5);

  return argv;
}

/**
 *@brief curl发送http/https GET请求
 *@param url 请求的url
 *@return 返回接收的数据量,失败则返回""
 */
// static string curlHTTPGet(const char *url)
// {
//   CURL *curl;
//   CURLcode res;
//   string str = "";

//   curl = curl_easy_init(); // 初始化CURL
//   if (curl != nullptr)
//   {
//     curl_easy_setopt(curl, CURLOPT_URL, url);
//     curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");                 // 设置GET请求
//     curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);                    // 不验证本地证书
//     curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);                    // 不要验证主机证书
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteDataToString); // 设置接收回调函数
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&str);              // 设置接收回调函数参数
//     res = curl_easy_perform(curl);                                        // 执行请求
//     if (res != CURLE_OK)
//       str = "";
//     curl_easy_cleanup(curl);
//   }

//   return str;
// }

/**
 *@brief 将获取到的数据写入string
 *@param buf 待接收的数据
 *@param size 接收数据包个数
 *@param nmemb 数据包大小
 *@arg 使用CURLOPT_WRITEDATA设置的string地址
 *@return 返回接收的数据量
 */
// static ssize_t curlWriteDataToString(void *buf, size_t size, size_t nmemb, void *arg)
// {
//   int len = size * nmemb;
//   string *str = static_cast<string *>(arg);

//   str->append((char *)buf, len);

//   return len;
// }

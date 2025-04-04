/*
 * @Author: yixi 3283804330@qq.com
 * @Date: 2025-04-01 00:10:51
 * @LastEditors: Do not edit
 * @LastEditTime: 2025-04-04 20:29:16
 * @Description: 
 * @FilePath: /yixi_desktop/main_page/src/MainPageUI.h
 */
#ifndef _MAINPAGE_GUI_H_
#define _MAINPAGE_GUI_H_

#include "lvgl/lvgl.h"
#include <functional>

namespace MainPageUI
{

    struct TimeInfo
    {
        int year;
        int month;
        int day;
        int week;
        int hour;
        int min;
        int sec;
    };

    struct WeatherInfo
    {
        char position[32]; // 地点位置
        char text[32];     // 天气文字
        char windDir[16];  // 风向
        int code;          // 天气代码
        int temp;          // 温度
        int rainFall;      // 降水量mm,扩大100倍
        int humi;          // 相对湿度
        int windLevel;     // 风力等级
        int maxTemp;       // 最高气温
        int minTemp;       // 最低气温
    };

    using ExitCb = std::function<void(void)>;
    using RunApplicationCb = std::function<void(const char *, char *const *)>;

    struct Operations
    {
        ExitCb exitCb; // 退出进程回调函数
        RunApplicationCb runApp; // 运行应用程序回调函数, 参数为执行文件名称
    };

    void bootCreate(void);
    void bootUpdate(const char *text);
    void bootRelease(void);

    void create(Operations &opts, const char *bgFile);
    void release(void);

    void updateDateTime(TimeInfo &time);
    void updateWeather(WeatherInfo &weather);
    void addApplication(const char *name, const char *exec, char *const argv[], const char *icon);

};

#endif

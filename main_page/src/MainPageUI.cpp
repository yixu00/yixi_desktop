#include "MainPageUI.h"
#include "lvgl/lvgl.h"
#include "stdio.h"
#include "stdlib.h"
// #include "src/sys.h"

#define SystemFontFile "/usr/work/simsun.ttc"

static lv_ft_info_t font16;
static lv_ft_info_t font24;
static lv_ft_info_t font28;
static lv_ft_info_t font36;

struct WeatherObj
{
  lv_obj_t *cont;         // 天气容器
  lv_obj_t *img;          // 显示天气图片
  lv_obj_t *addressLabel; // 显示地址标签
  lv_obj_t *tempBar;      // 温度进度条
  lv_obj_t *humiBar;      // 湿度进度条
  lv_obj_t *miscLabel;    // 其他信息label
};

struct MainObj
{
  lv_obj_t *timeLabel;   // 显示时间标签
  lv_obj_t *dataLabel;   // 显示日期标签
  lv_obj_t *appCont;     // 应用程序容器
  WeatherObj weatherObj; // 天气小工具集合
};

static void application_click_event_cb(lv_event_t *e);

// LCD分辨率
static uint32_t lcdW = 1280;
static uint32_t lcdH = 720;

// UI回调函数集合
static MainPageUI::Operations uiOpts;

// 主界面背景img
static lv_obj_t *backImage;

// 启动界面背景图片
static lv_obj_t *bootImage;

// 主界面小工具集合
static MainObj mainObj;

// 星期文本
static const char *weekday[] =
    {
        "日",
        "一",
        "二",
        "三",
        "四",
        "五",
        "六",
};

/**
 *@brief 初始化界面UI
 *@param opts UI回调函数集合
 *@param bg_file 主界面壁纸文件名(.bin)
 */
void MainPageUI::create(Operations &opts, const char *bgFile)
{
  uiOpts = opts;

  // 创建背景图片
  backImage = lv_img_create(lv_scr_act());
  lv_obj_set_size(backImage, lcdW, lcdH);
  lv_obj_center(backImage);
  lv_obj_clear_flag(backImage, LV_OBJ_FLAG_SCROLLABLE); // 设置不可滚动
  char path[36];
  sprintf(path, "%s%s", "/usr/work/background/", bgFile);
  lv_img_set_src(backImage, path); // 设置背景图片

  // 创建主界面元素
  mainObj.timeLabel = lv_label_create(backImage);                                           // 创建时间label
  // lv_obj_set_style_text_font(mainObj.timeLabel, &lv_font_boli_72, LV_STATE_DEFAULT);        // 设置字体
  lv_obj_set_style_text_color(mainObj.timeLabel, lv_color_hex(0xececec), LV_STATE_DEFAULT); // 字体颜色
  lv_label_set_text(mainObj.timeLabel, "12:34");
  lv_obj_align(mainObj.timeLabel, LV_ALIGN_CENTER, 0, -150);

  mainObj.dataLabel = lv_label_create(backImage);                                           // 创建日期label
  lv_obj_set_style_text_font(mainObj.dataLabel, font28.font, LV_STATE_DEFAULT);             // 设置字体
  lv_obj_set_style_text_color(mainObj.dataLabel, lv_color_hex(0xececec), LV_STATE_DEFAULT); // 字体颜色
  lv_label_set_text(mainObj.dataLabel, "2月8日星期三");
  lv_obj_align_to(mainObj.dataLabel, mainObj.timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, -10);

  mainObj.appCont = lv_obj_create(backImage);
  lv_obj_set_size(mainObj.appCont, 700, 200);
  lv_obj_set_scroll_dir(mainObj.appCont, LV_DIR_HOR);                                   // 只能水平滚动
  lv_obj_set_scrollbar_mode(mainObj.appCont, LV_SCROLLBAR_MODE_OFF);                    // 不显示滚动条
  lv_obj_set_style_bg_color(mainObj.appCont, lv_color_hex(0xeddddd), LV_STATE_DEFAULT); // 背景颜色
  lv_obj_set_style_bg_opa(mainObj.appCont, LV_OPA_30, LV_STATE_DEFAULT);                // 背景透明度
  lv_obj_set_style_border_opa(mainObj.appCont, LV_OPA_TRANSP, LV_STATE_DEFAULT);        // 边框透明
  lv_obj_align(mainObj.appCont, LV_ALIGN_BOTTOM_MID, 0, -60);

  // // 创建天气相关元素
  // WeatherObj *weatherObj = &mainObj.weatherObj;

  // weatherObj->cont = lv_obj_create(backImage); // 创建天气容器
  // lv_obj_set_size(weatherObj->cont, 260, 160);
  // lv_obj_clear_flag(weatherObj->cont, LV_OBJ_FLAG_SCROLLABLE);                           // 禁止滚动
  // lv_obj_set_style_bg_color(weatherObj->cont, lv_color_hex(0xeddddd), LV_STATE_DEFAULT); // 背景颜色
  // lv_obj_set_style_bg_opa(weatherObj->cont, LV_OPA_40, LV_STATE_DEFAULT);                // 背景透明度
  // lv_obj_set_style_border_opa(weatherObj->cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);        // 边框透明
  // lv_obj_align(weatherObj->cont, LV_ALIGN_TOP_RIGHT, -20, 20);

  // weatherObj->img = lv_img_create(weatherObj->cont); // 创建天气icon
  // lv_img_set_src(weatherObj->img, WEATHER_ICON_DIR "0.bin");
  // lv_obj_align(weatherObj->img, LV_ALIGN_TOP_RIGHT, 20, -20);

  // weatherObj->addressLabel = lv_label_create(weatherObj->cont); // 创建地址label
  // lv_obj_set_style_text_font(weatherObj->addressLabel, font28.font, LV_STATE_DEFAULT);
  // lv_obj_set_style_text_color(weatherObj->addressLabel, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
  // lv_label_set_text(weatherObj->addressLabel, "定位失败");
  // lv_obj_align(weatherObj->addressLabel, LV_ALIGN_TOP_LEFT, 20, -10);

  // lv_obj_t **barArray[] = {&weatherObj->tempBar, &weatherObj->humiBar};
  // // const lv_img_dsc_t *bar_icon[] = {&temp_icon, &humi_icon};
  // lv_obj_t *lastImg;

  // for (int i = 0; i < 2; i++)
  // {
  //   lv_obj_t *img = lv_img_create(weatherObj->cont); // 创建温度湿度相关
  //   // lv_img_set_src(img, bar_icon[i]);
  //   if (i == 0)
  //     lv_obj_set_pos(img, -20, 30);
  //   else
  //     lv_obj_align_to(img, lastImg, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

  //   lv_obj_t *bar = lv_bar_create(weatherObj->cont);
  //   lv_obj_set_size(bar, 60, 12);
  //   lv_bar_set_range(bar, -50, 50); // 温度范围-50  ~ 50
  //   lv_bar_set_value(bar, 22, LV_ANIM_ON);
  //   lv_obj_set_style_border_width(bar, 2, LV_STATE_DEFAULT);
  //   lv_obj_set_style_border_color(bar, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
  //   lv_obj_set_style_bg_color(bar, lv_color_hex(0xce6767), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  //   lv_obj_set_style_pad_all(bar, 1, LV_STATE_DEFAULT);
  //   lv_obj_align_to(bar, img, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

  //   lv_obj_t *label = lv_label_create(weatherObj->cont);
  //   lv_obj_set_style_text_font(label, font16.font, LV_STATE_DEFAULT);
  //   lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
  //   lv_label_set_text(label, "22℃");
  //   lv_obj_align_to(label, bar, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

  //   lv_obj_set_user_data(bar, label);

  //   *(barArray[i]) = bar;
  //   lastImg = img;
  // }
  // lv_bar_set_range(weatherObj->humiBar, 0, 100); // 湿度范围0  ~ 100
  // lv_obj_set_style_bg_color(weatherObj->humiBar, lv_color_hex(0x1454cc), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  // lv_label_set_text((lv_obj_t *)lv_obj_get_user_data(weatherObj->humiBar), "50%");
  // lv_bar_set_value(weatherObj->humiBar, 50, LV_ANIM_ON);

  // weatherObj->miscLabel = lv_label_create(weatherObj->cont);
  // lv_obj_set_size(weatherObj->miscLabel, 220, 30);
  // lv_label_set_long_mode(weatherObj->miscLabel, LV_LABEL_LONG_SCROLL);
  // lv_obj_set_style_text_font(weatherObj->miscLabel, font24.font, LV_STATE_DEFAULT);
  // lv_obj_set_style_text_color(weatherObj->miscLabel, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
  // lv_label_set_text_fmt(weatherObj->miscLabel, WEATHER_MISC_FMT, 0, 0, 10, 20, "东", 1);
  // lv_obj_align(weatherObj->miscLabel, LV_ALIGN_BOTTOM_MID, 0, 20);
}

// /**
//  *@brief 更新主界面天气信息
//  */
// void MainPageUI::updateWeather(WeatherInfo &weatherInfo)
// {
//   WeatherObj *w = &mainObj.weatherObj;
//   lv_obj_t *bar_label;

//   lv_label_set_text(w->addressLabel, weatherInfo.position);
//   lv_bar_set_value(w->tempBar, weatherInfo.temp, LV_ANIM_ON);
//   lv_bar_set_value(w->humiBar, weatherInfo.humi, LV_ANIM_ON);

//   bar_label = (lv_obj_t *)lv_obj_get_user_data(w->tempBar);
//   lv_label_set_text_fmt(bar_label, "%d℃", weatherInfo.temp);
//   bar_label = (lv_obj_t *)lv_obj_get_user_data(w->humiBar);
//   lv_label_set_text_fmt(bar_label, "%d%%", weatherInfo.humi);

//   char path[32];
//   sprintf(path, WEATHER_ICON_DIR "%d.bin", weatherInfo.code);
//   lv_img_set_src(w->img, path);

//   lv_label_set_text_fmt(w->miscLabel, WEATHER_MISC_FMT, weatherInfo.rainFall / 100, weatherInfo.rainFall % 100,
//                         weatherInfo.minTemp, weatherInfo.maxTemp, weatherInfo.windDir, weatherInfo.windLevel);

//   lv_label_set_long_mode(w->miscLabel, LV_LABEL_LONG_SCROLL);
// }

/**
 *@brief 在主界面添加一个app icon
 *@param name 应用程序名称
 *@param exec 应用程序文件路径
 *@param argv 应用程序参数
 *@param icon 应用程序图标(lv_img)
 */
void MainPageUI::addApplication(const char *name, const char *exec, char *const argv[], void *icon)
{
  lv_obj_t *prevApp = lv_obj_get_child(mainObj.appCont, -1); // 获取最后创建的app

  int len = strlen(exec) + 1; // 保存app执行文件名称
  char *execFile = new char[len];
  strcpy(execFile, exec);

  lv_obj_t *cont = lv_obj_create(mainObj.appCont); // app容器充当背景
  lv_obj_set_size(cont, 160, 180);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                           // 禁止滚动
  lv_obj_set_style_bg_color(cont, lv_color_hex(0xffffff), LV_STATE_DEFAULT); // 背景颜色
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);            // 背景透明度(默认状态)
  lv_obj_set_style_bg_opa(cont, LV_OPA_50, LV_STATE_PRESSED);                // 背景透明度(按下状态)
  lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);        // 边框透明
  lv_obj_set_user_data(cont, execFile);
  lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE); // 设置可点击
  lv_obj_add_event_cb(cont, application_click_event_cb, LV_EVENT_SHORT_CLICKED, (void *)argv);

  if (prevApp != nullptr) // 设置对齐
    lv_obj_align_to(cont, prevApp, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  else
    lv_obj_align(cont, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t *img = lv_img_create(cont); // app图标
  lv_img_set_src(img, icon);           // 设置图标
  lv_obj_align(img, LV_ALIGN_TOP_MID, 0, -10);
  lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *label = lv_label_create(cont); // app名称标签
  lv_obj_set_style_text_font(label, font24.font, LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(label, lv_color_hex(0x454545), LV_STATE_DEFAULT); // 字体颜色
  lv_label_set_text(label, name);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 10);
}

/**
 *@brief 释放界面UI
 */
void MainPageUI::release(void)
{
  lv_obj_t *appCont;

  while ((appCont = lv_obj_get_child(mainObj.appCont, -1)) != nullptr)
  {
    char *execFile = (char *)lv_obj_get_user_data(appCont);

    delete[] execFile; // 释放保存的app数据

    lv_obj_del(appCont);
  }

  lv_obj_del(backImage);

  lv_ft_font_destroy(font16.font);
  lv_ft_font_destroy(font24.font);
  lv_ft_font_destroy(font28.font);
  lv_ft_font_destroy(font36.font);
}

/**
 *@brief 初始化启动界面UI
 */
void MainPageUI::bootCreate(void)
{
  lv_obj_t *par = lv_layer_top(); // 创建到top layer上以覆盖主界面UI

  // 初始化字体
  font16.name = SystemFontFile;
  font16.weight = 16;
  font16.style = FT_FONT_STYLE_NORMAL;
  font16.mem = nullptr;
  lv_ft_font_init(&font16);

  font24.name = SystemFontFile;
  font24.weight = 24;
  font24.style = FT_FONT_STYLE_NORMAL;
  font24.mem = nullptr;
  lv_ft_font_init(&font24);

  font28.name = SystemFontFile;
  font28.weight = 28;
  font28.style = FT_FONT_STYLE_NORMAL;
  font28.mem = nullptr;
  lv_ft_font_init(&font28);

  font36.name = SystemFontFile;
  font36.weight = 36;
  font36.style = FT_FONT_STYLE_NORMAL;
  font36.mem = nullptr;
  lv_ft_font_init(&font36);

  // 获取显示设备分辨率
  lcdW = lv_disp_get_hor_res(nullptr);
  lcdH = lv_disp_get_ver_res(nullptr);

  bootImage = lv_img_create(par);
  lv_obj_set_size(bootImage, lcdW, lcdH);
  lv_obj_center(bootImage);
  lv_obj_clear_flag(bootImage, LV_OBJ_FLAG_SCROLLABLE); // 设置不可滚动
  lv_img_set_src(bootImage, "/usr/work/background/bootbg.bin");

  lv_obj_t *bootLogo = lv_gif_create(bootImage); // 创建giflogo
  lv_gif_set_src(bootLogo, "/usr/work/background/bootlogo.gif");
  lv_obj_align(bootLogo, LV_ALIGN_CENTER, 0, -50);

  lv_obj_t *bootSpinner = lv_spinner_create(bootImage, 1000, 60); // 创建旋转加载器
  lv_obj_set_size(bootSpinner, 100, 100);
  lv_obj_set_style_arc_opa(bootSpinner, LV_OPA_TRANSP, LV_STATE_DEFAULT);
  lv_obj_set_style_arc_color(bootSpinner, lv_color_hex(0xffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_align_to(bootSpinner, bootLogo, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);

  lv_obj_t *bootLabel = lv_label_create(bootImage);
  lv_obj_set_style_text_font(bootLabel, font36.font, LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(bootLabel, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
  lv_obj_align(bootLabel, LV_ALIGN_BOTTOM_MID, 0, -80);
  lv_label_set_text(bootLabel, "系统初始化");

  lv_obj_set_user_data(bootImage, bootLabel);
  lv_obj_set_user_data(bootLabel, bootSpinner);
}

/**
 *@brief 更新启动界面UI
 *@param progress 启动进度百分比 0~100
 *@param text 启动进度提示文本
 */
void MainPageUI::bootUpdate(const char *text)
{
  lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(bootImage);

  lv_label_set_text(label, text);

  if (strcmp(text, "欢迎使用") == 0)
  {
    lv_obj_t *spinner = (lv_obj_t *)lv_obj_get_user_data(label);
    if (spinner != nullptr)
      lv_obj_del_async(spinner);
  }

  lv_obj_invalidate(lv_scr_act()); // 重绘屏幕
}

/**
 *@brief 释放启动界面UI
 */
void MainPageUI::bootRelease(void)
{
  lv_obj_del_async(bootImage);
}

/**
 *@brief 更新时间显示
 */
void MainPageUI::updateDateTime(TimeInfo &time)
{
  char buf[32];

  memset(buf, 0, 32);
  buf[0] = time.hour / 10 + '0';
  buf[1] = time.hour % 10 + '0';
  buf[2] = ':';
  buf[3] = time.min / 10 + '0';
  buf[4] = time.min % 10 + '0';

  lv_label_set_text(mainObj.timeLabel, buf); // 设置时间文本

  sprintf(buf, "%d月%d日 星期%s", time.month, time.day, weekday[time.week]);
  lv_label_set_text(mainObj.dataLabel, buf); // 设置日期文本
}

/**
 *@brief 应用程序icon点击事件回调函数
 */
static void application_click_event_cb(lv_event_t *e)
{
  lv_obj_t *cont = lv_event_get_target(e);

  const char *exec = (const char *)lv_obj_get_user_data(cont);
  char *const *argv = (char *const *)lv_event_get_user_data(e);

  printf("click event cb, exec:%s,argv:", exec);

  for (int i = 0; argv[i] != nullptr; i++)
    printf("%s ", argv[i]);

  printf("\n");

  if (exec != nullptr && argv != nullptr)
  {
    if (uiOpts.runApp != nullptr)
    {
      uiOpts.runApp(exec, argv);       // 运行应用程序,应用程序退出前阻塞在此
      lv_obj_invalidate(lv_scr_act()); // 重绘屏幕
    }
  }
}

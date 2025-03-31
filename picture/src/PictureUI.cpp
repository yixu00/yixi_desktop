#include "PictureUI.hpp"
#include "lvgl/lvgl.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
// #include "sys.h"

#define SystemFontFile "/mnt/desk/font/simsun.ttc"

LV_IMG_DECLARE(bqb_night)
LV_IMG_DECLARE(picture_exit)

#define COLUMN_NUM (sizeof(columnDsc) / sizeof(columnDsc[0]) - 1)
#define ROW_NUM (sizeof(rowDsc) / sizeof(rowDsc[0]) - 1)

#define TITLE_HEIGHT 40

struct TitleObj
{
    lv_obj_t *cont;
    lv_obj_t *label;
    lv_obj_t *prevBtn;
    lv_obj_t *nextBtn;
};

static void clearImageList(void);
static void changeImage(int tag, bool dir);
static lv_obj_t *createImage(int x, int y, PictureUI::ImgInfo &info);
static lv_img_dsc_t *createImgDsc(PictureUI::ImgInfo &info);
static void gridDscInit(void);
static void imgDel(lv_obj_t *img);
static uint8_t *rgb888Toargb888(const uint8_t *src, int len);

static void drag_event_handler(lv_event_t *e);
static void img_list_click_event_handler(lv_event_t *e);
static void img_click_event_handler(lv_event_t *e);
static void screen_gusture_event_cb(lv_event_t *e);
static void title_btn_click_event_handler(lv_event_t *e);
static void img_anim_ready_handler(lv_anim_t *anim);

static lv_ft_info_t font28;
static lv_ft_info_t font32;

// gui回调函数集
static PictureUI::Operations uiOpts;

// LCD分辨率
static uint32_t lcdW = 1280;
static uint32_t lcdH = 720;

// 图片界面容器
static lv_obj_t *cont = nullptr;

// 背景图片
static lv_obj_t *backImage = nullptr;

// 上方标题栏
static TitleObj titleObj;

// 退出按钮
static lv_obj_t *exitBtn = nullptr;

// 保存上一张图片对象
static lv_obj_t *openedImage = nullptr;

// 保存上一张图片tag
static int openedTag = 0;

// 网格布局列cell
static lv_coord_t columnDsc[4 + 1] = {0};

// 网格布局行cell
static lv_coord_t rowDsc[6 + 1] = {0};

// 网格布局cell行索引
static lv_coord_t rowIndex = 0;

// 网格布局cell列索引
static lv_coord_t columnIndex = 0;

// 图片列表当前活动页索引
static int activePage = 1;

// 图片列表总页数
static int totalPage = 1;
/**
 *@brief 初始化picture界面
 */
void PictureUI::create(Operations &opts)
{
    lv_obj_t *act = lv_scr_act();

    uiOpts = opts;

    // 初始化字体
    font28.name = SystemFontFile;
    font28.weight = 28;
    font28.style = FT_FONT_STYLE_NORMAL;
    // font28.mem = nullptr;
    lv_ft_font_init(&font28);

    font32.name = SystemFontFile;
    font32.weight = 32;
    font32.style = FT_FONT_STYLE_NORMAL;
    // font32.mem = nullptr;
    lv_ft_font_init(&font32);

    // 获取显示设备分辨率
    lcdW = lv_disp_get_hor_res(nullptr);
    lcdH = lv_disp_get_ver_res(nullptr);

    // 初始化网格布局
    gridDscInit();

    // 创建背景图片
    backImage = lv_img_create(act);
    lv_obj_clear_flag(backImage, LV_OBJ_FLAG_SCROLLABLE);
    lv_img_set_src(backImage, &bqb_night);
    

    // 创建标题栏
    titleObj.cont = lv_obj_create(backImage);
    lv_obj_set_size(titleObj.cont, lcdW, TITLE_HEIGHT);
    lv_obj_set_style_radius(titleObj.cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(titleObj.cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(titleObj.cont, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(titleObj.cont, LV_OPA_70, LV_STATE_DEFAULT);
    lv_obj_clear_flag(titleObj.cont, LV_OBJ_FLAG_SCROLLABLE); // 禁用滚动

    titleObj.label = lv_label_create(titleObj.cont);
    lv_obj_set_height(titleObj.label, TITLE_HEIGHT);
    lv_label_set_text(titleObj.label, "图片(0/0)");
    lv_obj_set_style_text_color(titleObj.label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(titleObj.label, font32.font, LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(titleObj.label, LV_OPA_80, LV_STATE_DEFAULT);
    lv_obj_center(titleObj.label);

    for (int i = 0; i < 2; i++)
    {
        lv_obj_t *btn = lv_btn_create(titleObj.cont);
        const char *string;

        if (i == 0)
        {
            titleObj.prevBtn = btn;
            string = "上一页";
        }
        else
        {
            titleObj.nextBtn = btn;
            string = "下一页";
        }
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE); // 设置无法触摸
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);      // 隐藏按钮
        lv_obj_set_size(btn, 100, TITLE_HEIGHT - 4);
        lv_obj_add_event_cb(btn, title_btn_click_event_handler, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, string);
        lv_obj_set_style_text_color(label, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, font28.font, LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_STATE_DEFAULT);
        lv_obj_center(label);
    }

    lv_obj_align(titleObj.prevBtn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_align(titleObj.nextBtn, LV_ALIGN_RIGHT_MID, -10, 0);

    // 创建背景容器
    cont = lv_obj_create(backImage);
    lv_obj_set_size(cont, lcdW, lcdH - TITLE_HEIGHT);
    lv_obj_align_to(cont, titleObj.cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_grid_dsc_array(cont, columnDsc, rowDsc);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_all(cont, 8, LV_STATE_DEFAULT);                // 设置网格cell四周填充宽度
    lv_obj_set_style_outline_width(cont, 0, LV_STATE_DEFAULT);          // 设置网格cell四周外部线条宽度
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT); // 设置边框透明
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);     // 设置背景透明

    // 创建退出按钮
    exitBtn = lv_obj_create(backImage);
    lv_obj_set_size(exitBtn, 70, 70);
    lv_obj_set_pos(exitBtn, 1100, 360);
    lv_obj_clear_flag(exitBtn, LV_OBJ_FLAG_SCROLLABLE); // 禁止滚动
    lv_obj_set_style_radius(exitBtn, 35, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(exitBtn, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(exitBtn, LV_OPA_TRANSP, LV_STATE_DEFAULT); // A边框透明
    lv_obj_set_style_bg_opa(exitBtn, LV_OPA_50, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(exitBtn, LV_OPA_90, LV_STATE_PRESSED);

    lv_obj_t *img = lv_img_create(exitBtn);
    lv_img_set_src(img, &picture_exit);
    // lv_img_set_sec(img,"./picture_exit.png");
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_add_event_cb(exitBtn, drag_event_handler, LV_EVENT_PRESSING, nullptr);
    if (uiOpts.exitCb != nullptr)
        lv_obj_add_event_cb(
            exitBtn,
            [](lv_event_t *e)
            {
                uiOpts.exitCb();
            },
            LV_EVENT_SHORT_CLICKED, nullptr);
}

/**
 *@brief  设置图片页数
 *@param total 图片总数
 */
void PictureUI::setImageListPageNum(int total)
{
    totalPage = total / (24 + 1) + 1;

    lv_label_set_text_fmt(titleObj.label, "图片(%d/%d)", activePage, totalPage);

    if (totalPage > 1)
    {
        lv_obj_clear_flag(titleObj.prevBtn, LV_OBJ_FLAG_HIDDEN); // 显示按钮
        lv_obj_clear_flag(titleObj.nextBtn, LV_OBJ_FLAG_HIDDEN); // 显示按钮
    }
}

/**
 *@brief 在图片列表添加一个img对象显示图片缩略图
 *@param info 图片信息
 *@param tag 图片标签
 *@param get_image 获取图像原始数据回调
 */
const uint8_t *PictureUI::addImageList(ImgInfo info, int tag)
{
    lv_img_dsc_t *src = createImgDsc(info);

    ImgData *data = new ImgData;

    data->src = src;
    data->tag = tag;

    if (src != nullptr)
    {
        lv_obj_t *cell = lv_img_create(cont);
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, columnIndex, 1, LV_GRID_ALIGN_STRETCH, rowIndex, 1);
        lv_obj_set_style_bg_color(cell, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(cell, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_color(cell, lv_color_hex(0xffff00ff), LV_STATE_FOCUSED);
        lv_obj_set_style_outline_pad(cell, 6, LV_STATE_FOCUSED);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *img = lv_img_create(cell);
        lv_img_set_src(img, src);
        lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE |
                                 LV_OBJ_FLAG_CLICK_FOCUSABLE |
                                 LV_OBJ_FLAG_CLICKABLE); // 设置事件同时传播到父对象cell
        lv_obj_center(img);

        lv_obj_set_user_data(img, data);
        lv_obj_add_event_cb(img, img_list_click_event_handler, LV_EVENT_SHORT_CLICKED, nullptr);

        if (++columnIndex == COLUMN_NUM)
        {
            columnIndex = 0;
            rowIndex++;
        }
    }

    return src->data;
}

/**
 *@brief 完成列表页面切换完成调用函数
 */
void PictureUI::setListChangeReady(void)
{
    lv_obj_add_flag(titleObj.prevBtn, LV_OBJ_FLAG_CLICKABLE); // 设置按钮可触摸
    lv_obj_add_flag(titleObj.nextBtn, LV_OBJ_FLAG_CLICKABLE);
}

/**
 *@brief 清空picture界面
 */
void PictureUI::release()
{
    clearImageList(); // 清空图片列表

    lv_obj_del(backImage);

    lv_ft_font_destroy(font28.font);
    lv_ft_font_destroy(font32.font);
}

/**
 *@brief 清空图片列表
 */
static void clearImageList(void)
{
    lv_obj_t *cell = nullptr;

    do
    {
        cell = lv_obj_get_child(cont, -1);
        if (cell != nullptr)
        {
            lv_obj_t *img = lv_obj_get_child(cell, -1);
            if (img != nullptr)
            {
                PictureUI::ImgData *imgData = (PictureUI::ImgData *)lv_obj_get_user_data(img); // 释放图像数据
                delete[] imgData->src->data;
                delete imgData->src;
                delete imgData;
            }

            lv_obj_del(cell);
        }
    } while (cell != nullptr);

    columnIndex = 0; // 清除行列索引
    rowIndex = 0;
}

/**
 *@brief 切换图片显示
 *@param tag 切换的图片的tag
 *@param dir 切换的方向，true-正序，false-逆序
 */
static void changeImage(int tag, bool dir)
{
    if (uiOpts.getImageCb != nullptr)
    {
        PictureUI::ImgInfo imginfo;
        uiOpts.getImageCb(tag, &imginfo);

        printf("img create, tag:%d\n", tag);

        int startX = dir ? lcdW : -lcdW;

        lv_obj_t *img = createImage(startX, 0, imginfo);
        if (img != nullptr)
        {
            // 添加动画，删除上一个图片对象及数据
            lv_anim_t anim;

            lv_anim_init(&anim);
            lv_anim_set_var(&anim, img);
            lv_anim_set_values(&anim, startX, 0);
            lv_anim_set_time(&anim, 500);
            lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
            lv_anim_set_path_cb(&anim, lv_anim_path_ease_out); // 动画轨迹：最后变慢
            lv_anim_set_ready_cb(&anim, img_anim_ready_handler);
            lv_anim_set_user_data(&anim, openedImage);
            lv_anim_start(&anim); // 开启动画

            openedImage = img; // 保存图片对象
            openedTag = tag;   // 保存图片tag
        }
    }
}

/**
 *@brief 在指定位置创建一个img
 *@param w 图片宽度(像素)
 *@param h 图片高度(像素)
 *@param img 创建的img对象
 */
static lv_obj_t *createImage(int x, int y, PictureUI::ImgInfo &info)
{
    lv_img_dsc_t *src = createImgDsc(info);
    lv_obj_t *img = nullptr;

    if (src != nullptr)
    {
        img = lv_img_create(backImage);
        lv_obj_set_pos(img, x, y);
        lv_img_set_src(img, src);
        lv_obj_center(img);

        lv_obj_set_user_data(img, src);
    }

    return img;
}

/**
 *@brief 根据图像数据构建一个img_dsc
 */
static lv_img_dsc_t *createImgDsc(PictureUI::ImgInfo &info)
{
    lv_img_dsc_t *src = nullptr;
    const uint8_t *map = info.imgMap;

    src = new lv_img_dsc_t;
    memset(src, 0, sizeof(lv_img_dsc_t));

    if (src != nullptr)
    {
        if (info.bpp == 24)
        {
            map = rgb888Toargb888(info.imgMap, info.w * info.h);
            delete[] info.imgMap;
        }
        src->header.always_zero = 0;
        src->header.w = info.w;
        src->header.h = info.h;
        src->data_size = info.w * info.h * LV_COLOR_SIZE / 8;
        src->header.cf = LV_IMG_CF_TRUE_COLOR;
        src->data = map;
    }

    return src;
}

/**
 *@brief 初始化picture界面网格布局cell
 */
static void gridDscInit(void)
{ 
    int i;

    for (i = 0; i < COLUMN_NUM; i++)
        columnDsc[i] = CELL_W;
    columnDsc[i] = LV_GRID_TEMPLATE_LAST;

    for (i = 0; i < ROW_NUM; i++)
        rowDsc[i] = CELL_H;
    rowDsc[i] = LV_GRID_TEMPLATE_LAST;
}

/**
 *@brief 删除img对象及其图像数据
 */
static void imgDel(lv_obj_t *img)
{
    if (img == nullptr)
        return;

    lv_img_dsc_t *src = (lv_img_dsc_t *)lv_obj_get_user_data(img);
    if (src != nullptr)
    {
        delete[] src->data;
        delete src;
    }

    lv_obj_del_async(img);
}

static uint8_t *rgb888Toargb888(const uint8_t *src, int len)
{
    uint8_t *argb888 = new uint8_t[len * 4];

    for (int i = 0; i < len; i++)
    {
        argb888[4 * i] = src[3 * i];
        argb888[4 * i + 1] = src[3 * i + 1];
        argb888[4 * i + 2] = src[3 * i + 2];
        argb888[4 * i + 3] = 0xff;
    }

    return argb888;
}

/**
 *@brief 长按拖动控件事件函数
 */
static void drag_event_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == nullptr)
        return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t x = lv_obj_get_x(obj) + vect.x;
    lv_coord_t y = lv_obj_get_y(obj) + vect.y;
    lv_obj_set_pos(obj, x, y);
}

/**
 *@brief 图片列表图标点击事件函数
 */
static void img_list_click_event_handler(lv_event_t *e)
{
    lv_obj_t *img = lv_event_get_target(e);

    PictureUI::ImgData *data = (PictureUI::ImgData *)lv_obj_get_user_data(img);

    if (data != nullptr)
    {
        changeImage(data->tag, true); // 创建图片

        // 为屏幕创建手势事件回调函数
        lv_obj_add_flag(lv_scr_act(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);                               // 禁止滚动
        lv_obj_add_event_cb(lv_scr_act(), screen_gusture_event_cb, LV_EVENT_GESTURE, nullptr); // 屏幕手势事件，用于切换图片
    }
}

/**
 *@brief 图片单击事件函数
 */
static void img_click_event_handler(lv_event_t *e)
{
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_event_cb(lv_scr_act(), screen_gusture_event_cb); // 移除屏幕手势事件

    lv_obj_t *img = lv_event_get_target(e);
    imgDel(img);

    openedTag = 0;
    openedImage = nullptr;
}

/**
 * @brief 屏幕手势滑动事件(用于切换图片)图片
 */
static void screen_gusture_event_cb(lv_event_t *e)
{
    lv_indev_wait_release(lv_indev_get_act());

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

    switch (dir)
    {
    case LV_DIR_RIGHT:
        if (uiOpts.getPrevTagCb != nullptr)
        {
            int tag = uiOpts.getPrevTagCb(openedTag);
            changeImage(tag, false); // 创建图片
        }
        break;
    case LV_DIR_LEFT:
        if (uiOpts.getNextTagCb != nullptr)
        {
            int tag = uiOpts.getNextTagCb(openedTag);
            changeImage(tag, true); // 创建图片
        }
        break;
    default:
        return;
    }
}

/**
 *@brief 标题栏按钮点击事件回调函数
 */
static void title_btn_click_event_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    clearImageList();

    if (btn == titleObj.prevBtn)
        activePage = activePage > 1 ? activePage - 1 : totalPage;
    else if (btn == titleObj.nextBtn)
        activePage = activePage < totalPage ? activePage + 1 : 1;

    lv_label_set_text_fmt(titleObj.label, "图片(%d/%d)", activePage, totalPage);

    lv_obj_clear_flag(titleObj.prevBtn, LV_OBJ_FLAG_CLICKABLE); // 设置无法触摸
    lv_obj_clear_flag(titleObj.nextBtn, LV_OBJ_FLAG_CLICKABLE);

    if (uiOpts.changeListPage != nullptr)
        uiOpts.changeListPage(activePage);
}

/**
 *@brief 图片动画结束回调函数
 */
static void img_anim_ready_handler(lv_anim_t *anim)
{
    lv_obj_t *old_img = (lv_obj_t *)lv_anim_get_user_data(anim);

    if (old_img != nullptr)
        imgDel(old_img);

    lv_obj_t *img = (lv_obj_t *)anim->var;

    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_CLICKABLE); // 设置可触摸
    lv_obj_add_event_cb(img, img_click_event_handler, LV_EVENT_SHORT_CLICKED, nullptr);
}

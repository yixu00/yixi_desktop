/*
 * @Author: yixi 3283804330@qq.com
 * @Date: 2024-11-25 22:50:14
 * @LastEditors: Do not edit
 * @LastEditTime: 2025-03-31 21:48:07
 * @FilePath: /lv_port_linux_frame_buffer/src/PictureUI.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _PICTURE_GUI_HPP_
#define _PICTURE_GUI_HPP_

#include "lvgl/lvgl.h"
#include <functional>

#define CELL_W 300
#define CELL_H 160

namespace PictureUI
{

    struct ImgInfo
    {
        int w;
        int h;
        const uint8_t *imgMap;
        int bpp;
    };

    struct ImgData
    {
        int tag;
        lv_img_dsc_t *src;
    };

    using GetImageCb = std::function<void(int, ImgInfo *)>;
    using ExitCb = std::function<void(void)>;
    using GetImageTagCb = std::function<int(int)>;
    using ChangeListCb = std::function<void(int)>;

    struct Operations
    {
        GetImageCb getImageCb;       // 获取图片数据回调函数
        ExitCb exitCb;               // 退出程序回调函数
        GetImageTagCb getPrevTagCb;    // 切换上一张图片的tag回调函数
        GetImageTagCb getNextTagCb;    // 切换下一张图片的tag回调函数
        ChangeListCb changeListPage; // 切换页面,参数为列表索引
    };

    void create(Operations &gui_opts);
    const uint8_t *addImageList(ImgInfo info, int tag);
    void setImageListPageNum(int total);
    void setListChangeReady(void);
    void release(void);
};

#endif

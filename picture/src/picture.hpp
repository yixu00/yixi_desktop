/*
 * @Author: yixi 3283804330@qq.com
 * @Date: 2024-11-22 16:55:22
 * @LastEditors: yixi 3283804330@qq.com
 * @LastEditTime: 2024-11-27 22:20:23
 * @FilePath: \yixidesk_imx\application\picture\src\picture.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _PICTURE_HPP_
#define _PICTURE_HPP_

#include <thread>
#include <mutex>
#include <map>
#include "PictureUI.hpp"

class Picture
{
public:
private:
    std::thread *pthread; // 数据处理线程
    bool threadExitFlag;  // 指定线程退出标记
    std::mutex *uiMutex;

    std::map<int, std::string> imageTable; // 图像映射表
    std::mutex updateMutex;

    int destpage = 1; // 目标页面


    int threadFunction(void);
    int searchImage(std::string &path, int listMax);
    void updateImageListPage(int page);
    void getImage(int tag, PictureUI::ImgInfo *info);
    int getNextTag(int curTag);
    int getPrevTag(int curTag);
    void changeImageListPage(int page);

public:
    Picture(std::function<void(void)> exitCb, std::mutex &uiMutex);
    ~Picture();

    static unsigned char *bmpImageDecode(std::string &file, int &wight, int &height, int &bitPerPixel);
    static unsigned char *jpegImageDecode(std::string &file, int &wight, int &height, int &bitPerPixel);
    static unsigned char *pngImageDecode(std::string &file, int &wight, int &height, int &bitPerPixel);
};

#endif

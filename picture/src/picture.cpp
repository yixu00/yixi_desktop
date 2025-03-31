#include "picture.hpp"
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "BitImage.h"
#include "JpegImage.h"
#include "PngImage.h"

#include <iostream>

#define IMAGE_DIR "/usr/wifi/bin/"




Picture::Picture(std::function<void(void)> exitCb, std::mutex &UIMutex)
{
    threadExitFlag = false;
    uiMutex = &UIMutex;

    PictureUI::Operations uiOpts;

    uiOpts.exitCb = exitCb;
    uiOpts.getImageCb = std::bind(&Picture::getImage, this, std::placeholders::_1, std::placeholders::_2);
    uiOpts.getNextTagCb = std::bind(&Picture::getNextTag, this, std::placeholders::_1);
    uiOpts.getPrevTagCb = std::bind(&Picture::getPrevTag, this, std::placeholders::_1);
    uiOpts.changeListPage = std::bind(&Picture::changeImageListPage, this, std::placeholders::_1);

    PictureUI::create(uiOpts);

    pthread = new std::thread(&Picture::threadFunction, this);
}

Picture::~Picture()
{
    threadExitFlag = true;
    pthread->join();
    delete pthread;

    PictureUI::release();
}

int Picture::threadFunction(void)
{
    usleep(50000);

    std::string imagePath = IMAGE_DIR;

    int img_total = searchImage(imagePath, 300);
    PictureUI::setImageListPageNum(img_total);

    printf("search image complete! total:%d\n", img_total);

    while (!threadExitFlag)
    {
        if(updateMutex.try_lock() == true)
        {
            updateImageListPage(destpage);
            PictureUI::setListChangeReady();
        }
        
        usleep(10000);
    }

    return 0;
}

int Picture::searchImage(std::string &path, int listMax)
{
    // printf("I am search\n");
    int cnt_r = 0;
    int i;
    bool legal_img = false;
    std::string file_path;

    struct dirent *ent;
    DIR *dir = opendir(path.c_str());
    // if (dir != nullptr) {
    //     std::cout << "Successfully opened directory: " << path << std::endl;
    //     // 这里可以进行后续的目录读取操作
    //     // ...

    //     // 完成后关闭目录流
    //     closedir(dir);
    // } else {
    //     // 打开目录失败，打印错误信息
    //     std::cout << "Failed to open directory: " << path << std::endl;
    //     // 可以进一步使用 perror 来打印错误原因
    //     // perror("opendir");
    // }
   
    if (dir != nullptr) 
    {
    for (i = 0; i < listMax; i++)
    {
        ent = readdir(dir);
        if (ent == NULL)
            break;

        if (ent->d_type == DT_REG)
        {
            const char *pfile = strrchr(ent->d_name, '.');
            if (pfile != NULL)
            {
                file_path = path + std::string(ent->d_name);

                if (strcasecmp(pfile, ".bmp") == 0)
                {
                    printf("bmp file\n");
                    legal_img = true;
                }
                else if (strcasecmp(pfile, ".jpg") == 0 || strcasecmp(pfile, ".jpeg") == 0)
                {
                    printf("jpg/jpeg file\n");
                    legal_img = true;
                }
                else if (strcasecmp(pfile, ".png") == 0)
                {
                    printf("png file\n");
                    legal_img = true;
                }
            }
        }
        if (legal_img == true)
        {
            legal_img = false;
            imageTable.insert({cnt_r, file_path}); // 将图像索引和文件名插入map
            cnt_r++;
        }

        usleep(50000);
    }

    closedir(dir);
    }

    return cnt_r;
}

void Picture::updateImageListPage(int page)
{
    int tag = (page - 1) * 24;
    int num = imageTable.size() - tag;

    if (num > 24)
        num = 24;

    for (int i = 0; i < num; i++)
    {
        std::string file_path = imageTable[tag + i];
        int w, h, bpp;
        unsigned char *image = NULL;

        const char *pfile = strrchr(file_path.c_str(), '.');
        if (strcasecmp(pfile, ".bmp") == 0)
        {
            printf("bmp file\n");
            image = bmpImageDecode(file_path, w, h, bpp);
        }
        else if (strcasecmp(pfile, ".jpg") == 0 || strcasecmp(pfile, ".jpeg") == 0)
        {
            printf("jpg/jpeg file\n");
            image = jpegImageDecode(file_path, w, h, bpp);
        }
        else if (strcasecmp(pfile, ".png") == 0)
        {
            printf("png file\n");
            image = pngImageDecode(file_path, w, h, bpp);
        }

        if (image != NULL)
        {
            unsigned char *zoomimge = BitImage::BitImageZoom(w, h, image, CELL_W, CELL_H, bpp);
            delete[] image;

            uiMutex->lock();
            PictureUI::addImageList((PictureUI::ImgInfo){CELL_W, CELL_H, zoomimge, bpp}, tag + i);
            uiMutex->unlock();
        }

        usleep(50000);
    }
}

unsigned char *Picture::bmpImageDecode(std::string &file, int &wight, int &height, int &bitPerPixel)
{
    BitImage *bmp = new BitImage(file);
    int w = bmp->GetInfo().width;
    int h = bmp->GetInfo().height;
    int bpp = bmp->GetInfo().bpp;

    printf("bmp image width:%d\n", w);
    printf("bmp image height:%d\n", h);
    printf("bmp image bpp:%d\n", bpp);

    unsigned char *buf = new unsigned char[w * h * bpp / 8];

    bmp->GetImage(buf, h);

    wight = w;
    height = h;
    bitPerPixel = bpp;

    delete bmp;

    return buf;
}

unsigned char *Picture::jpegImageDecode(std::string &file, int &wight, int &height, int &bitPerPixel)
{
    JpegImage *jpeg = new JpegImage(file);
    int w = jpeg->GetInfo().output_width;
    int h = jpeg->GetInfo().output_height;
    int bpp = jpeg->GetInfo().output_components * 8;

    printf("jpeg image width:%d\n", w);
    printf("jpeg image height:%d\n", h);
    printf("jpeg image bpp:%d\n", bpp);

    unsigned char *buf = new unsigned char[w * h * bpp / 8];

    jpeg->GetImage(buf, h);

    wight = w;
    height = h;
    bitPerPixel = bpp;

    delete jpeg;

    return buf;
}

unsigned char *Picture::pngImageDecode(std::string &file, int &wight, int &height, int &bitPerPixel)
{
    PngImage *png = new PngImage(file);
    int w = png->GetInfo().width;
    int h = png->GetInfo().height;
    int bpp = png->GetInfo().bpp;

    printf("png image width:%d\n", w);
    printf("png image height:%d\n", h);
    printf("png image bpp:%d\n", bpp);

    unsigned char *buf = new unsigned char[w * h * bpp / 8];

    png->GetImage(buf, h);

    wight = w;
    height = h;
    bitPerPixel = bpp;

    delete png;

    return buf;
}

/**
 *@brief 根据索引值获取已知图像数据
 *@param tag 图片索引
 *@param info 保存图像信息
 */
void Picture::getImage(int tag, PictureUI::ImgInfo *info)
{
    std::string path = imageTable[tag];
    unsigned char *image = NULL;

    const char *pfile = strrchr(path.c_str(), '.');
    if (pfile != NULL)
    {
        int w, h, bpp;

        if (strcasecmp(pfile, ".bmp") == 0)
        {
            printf("bmp file\n");
            image = bmpImageDecode(path, w, h, bpp);
        }
        else if (strcasecmp(pfile, ".jpg") == 0 || strcasecmp(pfile, ".jpeg") == 0)
        {
            printf("jpg/jpeg file\n");
            image = jpegImageDecode(path, w, h, bpp);
        }
        else if (strcasecmp(pfile, ".png") == 0)
        {
            printf("png file\n");
            image = pngImageDecode(path, w, h, bpp);
        }

        info->w = w;
        info->h = h;
        info->bpp = bpp;
        info->imgMap = image;
    }
}

int Picture::getNextTag(int curTag)
{
    int tag = curTag + 1;

    int img_total = imageTable.size();

    if (tag == img_total)
        tag = 0;

    return tag;
}

int Picture::getPrevTag(int curTag)
{
    int tag = curTag - 1;

    int img_total = imageTable.size();

    if (tag < 0)
        tag = img_total - 1;

    return tag;
}

void Picture::changeImageListPage(int page)
{
    destpage = page;

    updateMutex.unlock(); // 释放互斥量
}

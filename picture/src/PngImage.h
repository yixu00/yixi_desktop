#ifndef _PNGIMAGE_H_
#define _PNGIMAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include <string>
#include <string.h>
#include <stdio.h>
#include <png.h>


class PngImage;

class PngImage
{
public:

    typedef struct _ImageInfo
    {
        int width;
        int height;
        int bpp;
    }ImageInfo;

    typedef enum
    {
        IMAGE_NOT_INIT = -1,
        IMAGE_OK = 0,
    } ImageStatus;

private:
    png_structp png_ptr;
    png_infop info_ptr;
    FILE *file;
    ImageInfo image_info;
    unsigned long line_bytes; // 一行的字节数
    ImageStatus status;

public:
    PngImage(std::string path);
    ~PngImage();

    void GetImage(unsigned char *buf, unsigned long line);

    const ImageInfo &GetInfo() const { return image_info; }

};





#endif

#ifndef _JPEGIMAGE_H_
#define _JPEGIMAGE_H_

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
#include <jpeglib.h>

class JpegImage;

class JpegImage
{
public:
    typedef enum
    {
        IMAGE_NOT_INIT = -1,
        IMAGE_OK = 0,
    } ImageStatus;

private:
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *file;
    unsigned long line_bytes; // 一行的字节数
    ImageStatus status;

public:
    JpegImage(std::string path);
    ~JpegImage();

    void GetImage(unsigned char *buf, unsigned long line);

    const jpeg_decompress_struct &GetInfo() const { return cinfo; }

};

#endif

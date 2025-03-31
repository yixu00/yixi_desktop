/*
 * @Author: yixi 3283804330@qq.com
 * @Date: 2024-11-22 21:12:33
 * @LastEditors: yixi 3283804330@qq.com
 * @LastEditTime: 2024-11-22 21:20:58
 * @FilePath: \yixidesk_imx\application\picture\src\JpegImage.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "JpegImage.h"

JpegImage::JpegImage(std::string path)
{
    status = IMAGE_NOT_INIT;

    do
    { 
        file = fopen(path.c_str(), "r");
        if (file == NULL)
        {
            perror("open error");
            break;
        }

        cinfo.err = jpeg_std_error(&jerr);      //绑定默认错误处理函数

        jpeg_create_decompress(&cinfo);     //创建JPEG解码器
        jpeg_stdio_src(&cinfo, file);               //指定解码图片文件
        int ret = jpeg_read_header(&cinfo, TRUE);     //读取图像信息
        if(ret != JPEG_HEADER_OK)
        {
            printf("%s: read jpeg header failed\n", path.c_str());
            fclose(file);
            file = NULL;
            break;
        }
        // cinfo.out_color_space = JCS_EXT_BGRA;        //输出颜色格式为RGB888

        jpeg_start_decompress(&cinfo);      //开始解码

        status = IMAGE_OK;
        line_bytes = cinfo.output_components * cinfo.output_width;

    } while (0);
}

JpegImage::~JpegImage()
{
    jpeg_finish_decompress(&cinfo);     //完成解码
    jpeg_destroy_decompress(&cinfo);        //销毁解码器

    if(file != NULL)
        fclose(file);
}

void JpegImage::GetImage(unsigned char *buf, unsigned long line)
{
    if (buf == NULL || line <= 0 || status == IMAGE_NOT_INIT)
        return;

    unsigned char *line_buf = new unsigned char[line_bytes];

    for(int i = 0; i < line; i++)
    {
        jpeg_read_scanlines(&cinfo, &line_buf, 1);       //每次读取一行

        unsigned char *p = buf + line_bytes * i;

        memcpy(p, line_buf, line_bytes);
    }

    delete[] line_buf;
}


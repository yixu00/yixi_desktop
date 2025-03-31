#include "PngImage.h"

PngImage::PngImage(std::string path)
{
    status = IMAGE_NOT_INIT;
    png_ptr = NULL;
    info_ptr = NULL;

    do
    {
        file = fopen(path.c_str(), "r");
        if (file == NULL)
        {
            perror("open error");
            break;
        }

        /* 创建png_ptr  info_ptr */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr == NULL)
        {
            printf("png decoder create failed\n");
            fclose(file);
            file = NULL;
            break;
        }

        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == NULL)
        {
            printf("png decoder create failed\n");
            png_destroy_read_struct(&png_ptr, NULL, NULL);
            fclose(file);
            file = NULL;
            break;
        }

        /* 设置png错误跳转点 */
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            printf("png error\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(file);
            file = NULL;
            break;
        }

        png_init_io(png_ptr, file); // 指定png数据源

        png_read_info(png_ptr, info_ptr); // 获取png图像数据
        image_info.width = png_get_image_width(png_ptr, info_ptr);
        image_info.height = png_get_image_height(png_ptr, info_ptr);
        image_info.bpp = png_get_bit_depth(png_ptr, info_ptr) * 4;
        line_bytes = png_get_rowbytes(png_ptr, info_ptr);

        unsigned char color_type = png_get_color_type(png_ptr, info_ptr);

        if (color_type == PNG_COLOR_TYPE_RGB)
            png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER); // 添加alpha通道转argb8888

        png_set_bgr(png_ptr);    

        png_read_update_info(png_ptr, info_ptr);

        status = IMAGE_OK;

    } while (0);
}

PngImage::~PngImage()
{
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    if (file != NULL)
        fclose(file);
}

void PngImage::GetImage(unsigned char *buf, unsigned long line)
{
    if (buf == NULL || line <= 0 || status == IMAGE_NOT_INIT)
        return;

    /* 设置png错误跳转点 */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        printf("png error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(file);
        file = NULL;
        return;
    }

    unsigned char *line_buf = new unsigned char[line_bytes];

    for (int i = 0; i < line; i++)
    {
        png_read_rows(png_ptr, &line_buf, NULL, 1); // 每次读取一行

        unsigned char *p = buf + line_bytes * i;

        memcpy(p, line_buf, line_bytes);
    }

    png_read_end(png_ptr, info_ptr);

    delete[] line_buf;
}
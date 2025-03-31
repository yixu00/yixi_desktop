#ifndef _SYS_H_
#define _SYS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SystemFontFile "res/fonts/systemFont.ttf"

void sys_init(void);
uint32_t sys_tick(void);
void lv_port_disp_init(void);
void lv_port_indev_init(void);
void lv_port_fs_init(void);
int sys_set_brightness(int value);
void sys_exit(void);

#ifdef __cplusplus
}
#endif





#endif

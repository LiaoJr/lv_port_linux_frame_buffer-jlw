
#ifndef _MYGUI_H_
#define _MYGUI_H_

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>



/*Add click event to a button*/
void lv_mybtn(void);

void show_png(int16_t x, int16_t y);

void lv_example_imgbtn_my(void);

void show_font(int x, int y, char *msg);

/*主菜单界面显示和事件注册*/
void main_menu(void);




#endif

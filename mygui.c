//                           .-"``"-.
//                          /______; }
//                         {_______}\|
//                         (/ a a \)(_)
//                         (.-.).-.)
//   ________________ooo__(    ^    )____________________
//  /                      '-.___.-'                     |
// |                                                      |
// |                       JW影音                          |
// |                          by ljw                      |
//  \_________________________________ooo________________/
//                         |_  |  _|  jgs
//                         \___|___/
//                         {___|___}
//                          |_ | _|
//                          /-'Y'-|
//                         (__/ \__)
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include "mygui.h"

#define HIDDEN_WIN 0

static lv_obj_t *main_win = NULL;  //主界面窗口
static lv_obj_t *music_win = NULL; //music player窗口
static lv_obj_t *video_win = NULL; //video player窗口

static char video_name[8][512] = { 0 };
static char music_name[8][512] = { 0 };
static char album_name[8][512] = { 0 };
static char lrc_name[8][512] = { 0 };
static char lrc_lines[256][256] = { 0 };
static char name[512] = { 0 };
static char songname[128] = { 0 };
static char artist[128] = { 0 };
static char album[128] = { 0 };
static lv_obj_t *list1 = NULL;//video list
static lv_obj_t *list2 = NULL;//music list

static lv_obj_t *play_obj = NULL;
static lv_obj_t *slider_obj = NULL;
static lv_obj_t *slider_volume = NULL;
static lv_obj_t *img_volume = NULL;
static char time_pos[32] = { 0 };
static lv_obj_t *time_obj = NULL;
static char percent_pos[32] = { 0 };
// static const lv_font_t *font_small;
static lv_obj_t *play_info_obj = NULL;
static lv_obj_t *imgbtn_album = NULL;
static lv_anim_t a;  //专辑封面旋转动画
static int32_t angle_current = 0;
static lv_obj_t *label_song = NULL;
static lv_obj_t *label_singer = NULL;
static lv_obj_t *label_album = NULL;
static lv_obj_t *label_lrc = NULL;

pthread_t tid = 0;
int taskquit = 0;

FILE *fp = NULL;

int32_t slider_value = 0;
extern pthread_mutex_t mutex;


static lv_obj_t *show_music_icon(char *pic_path);
static void enter_music_player(lv_event_t *e);
static lv_obj_t *show_video_icon(char *pic_path);
static void enter_video_player(lv_event_t *e);

static void show_video_win(void);
static void show_video_back_btn(void);
static void video_back(lv_event_t *e);
static void show_video_play_list(char *dp);
static void video_play(lv_event_t *e);
void *play_video_task(void *arg);
void get_play_info(int arg);
static lv_obj_t *create_ctrl_box(lv_obj_t *parent);

/*video ctrl box事件*/
static void play_event_click_cb(lv_event_t *e);
static void prev_click_event_cb(lv_event_t *e);
static void next_click_event_cb(lv_event_t *e);
static void slider_release_event(lv_event_t *e);
static void slider_volume_release_event(lv_event_t *e);

static void show_music_win(void);
static void show_music_back_btn(void);
static void music_back(lv_event_t *e);
static void show_music_play_list(char *dp);
static void music_play(lv_event_t *e);
static void *play_music_task(void *arg);
static void show_music_play_info(char *pic_path, char *song_name, char *singer, char *album_name);
static void anim_angle_cb(void *var, int32_t v);
static void music_lyric_save(char *lyric_file);
static size_t freadline(FILE *f, char *fline, int len);
static void get_str1(char *str);
static void get_str2(char *str);
static void get_album(char *str);

bool check_process(const char *process_name)//判断视频是否播放
{
    char command[1024];
    snprintf(command, sizeof(command), "pgrep %s >/dev/null 2>&1", process_name);
    return system(command) == 0;
}

/*读取文件夹中的文件名并添加到数组中*/
/*参数: dp文件夹路径，keyword文件名关键词，head存放的二位数组*/
char **SaveFilePath(const char *dp, const char *keyword, char head[][512])
{
    if(NULL == head)
    {
        printf("Save file failed.\n");
        return NULL;
    }

    DIR *d = opendir(dp);
    if(NULL == d)
    {
        perror("opendir failed.\n");
        return NULL;
    }

    int n = 0;
    while(1)
    {
        /*循环读取文件夹中的内容*/
        struct dirent *dc = readdir(d);
        if(NULL == dc)
        {
            // printf("the end of the directory stream is reached.\n");
            break;
        }

        /*跳过./..和隐藏目录*/
        if(dc->d_name[0] == '.')
        {
            continue;
        }
        else if(dc->d_type == DT_DIR)
        {
            printf(">>>>>>>>>>>>skip the subdirectory: %s.\n", dc->d_name);
            // head = SaveFilePath(path, head);
            continue;
        }
        else if(strstr(dc->d_name, keyword) != NULL)
        {
            char path[512] = { 0 };
            sprintf(path, "%s/%s", dp, dc->d_name);
            char(*p)[512] = (void *)head;
            strcpy(*(p + n), dc->d_name);
            // printf("path = %s\n", *(p+n));
            n++;
        }
    }

    int cn = closedir(d);
    if(cn != 0)
    {
        perror("closedir failed.");
        return NULL;
    }
    return NULL;
}



/**************************************************主界面相关函数******************************************************/
/*主菜单界面显示和事件注册*/
void main_menu(void)
{
    main_win = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_win, 800, 480);
    /*显示music图标*/
    lv_obj_t *music = show_music_icon("S:/root/project2/img/music.png");
    /*注册music图标事件*/
    lv_obj_add_event_cb(music, enter_music_player, LV_EVENT_CLICKED, NULL);

    /*显示video图标*/
    lv_obj_t *video = show_video_icon("S:/root/project2/img/video.png");
    /*注册video图标事件*/
    lv_obj_add_event_cb(video, enter_video_player, LV_EVENT_CLICKED, NULL);
}

/*显示Music图标*/
static lv_obj_t *show_music_icon(char *pic_path)
{

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr, lv_color_black());
    // lv_style_set_transform_width(&style_pr, 20);

    /*Create an image button*/
    lv_obj_t *imgbtn1 = lv_imgbtn_create(main_win);
    lv_imgbtn_set_src(imgbtn1, LV_IMGBTN_STATE_RELEASED, NULL, pic_path, NULL);
    lv_obj_set_size(imgbtn1, 200, 200);
    // lv_obj_add_style(imgbtn1, &style_def, 0);
    lv_obj_add_style(imgbtn1, &style_pr, LV_STATE_PRESSED);

    lv_obj_align(imgbtn1, LV_ALIGN_CENTER, -150, 0);//图标相对屏幕的位置

    /*Create a label on the image button*/
    lv_obj_t *label1 = lv_label_create(imgbtn1);
    lv_label_set_text(label1, "Music");
    lv_obj_align_to(label1, imgbtn1, LV_ALIGN_OUT_BOTTOM_MID, 0, -30);

    return imgbtn1;
}

/*点即music按钮后事件，进入音乐播放器*/
static void enter_music_player(lv_event_t *e)
{
    // 获取事件号
    lv_event_code_t code = lv_event_get_code(e);

    // 单击事件
    if(code == LV_EVENT_CLICKED)
    {
#if HIDDEN_WIN
        // 给窗口1的容器添加隐藏属性，清除窗口2的隐藏属性
        lv_obj_add_flag(win1_contanier, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(win2_contanier, LV_OBJ_FLAG_HIDDEN);
#else
        // 删除主界面
        lv_obj_del(main_win);

        // 显示音乐界面
        show_music_win();
#endif
    }
}

/*显示video player图标*/
static lv_obj_t *show_video_icon(char *pic_path)
{
    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr, lv_color_black());

    /*Create an image button*/
    lv_obj_t *imgbtn1 = lv_imgbtn_create(main_win);
    lv_imgbtn_set_src(imgbtn1, LV_IMGBTN_STATE_RELEASED, NULL, pic_path, NULL);
    lv_obj_set_size(imgbtn1, 200, 200);
    // lv_obj_add_style(imgbtn1, &style_def, 0);
    lv_obj_add_style(imgbtn1, &style_pr, LV_STATE_PRESSED);

    lv_obj_align(imgbtn1, LV_ALIGN_CENTER, 150, 0);//图标相对屏幕的位置

    /*Create a label on the image button*/
    lv_obj_t *label1 = lv_label_create(imgbtn1);
    lv_label_set_text(label1, "Video");
    lv_obj_align_to(label1, imgbtn1, LV_ALIGN_OUT_BOTTOM_MID, 0, -30);

    return imgbtn1;
}

/*进入video player界面*/
static void enter_video_player(lv_event_t *e)
{
    // 获取事件号
    lv_event_code_t code = lv_event_get_code(e);

    // 单击事件
    if(code == LV_EVENT_CLICKED)
    {
#if HIDDEN_WIN
        // 给窗口1的容器添加隐藏属性，清除窗口2的隐藏属性
        lv_obj_add_flag(win1_contanier, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(win2_contanier, LV_OBJ_FLAG_HIDDEN);
#else
        // 删除主界面
        lv_obj_del(main_win);

        // 显示视频界面
        show_video_win();

#endif
    }
}



//================================================================================================================

/***************************************************video播放器界面*************************************************/
/*显示视频播放器界面，并注册相关函数*/
static void show_video_win(void)
{
    video_win = lv_obj_create(lv_scr_act());
    lv_obj_set_size(video_win, 800, 480);

    // 在视频界面中创建一个按钮
    lv_obj_t *label = lv_label_create(video_win);

    // 设置标签的内容
    lv_label_set_text(label, "Video Player");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, -10);

    /*视频播放器返回按钮*/
    show_video_back_btn();

    /*读取video文件夹里的视频并显示play list*/
    show_video_play_list("/root/project2/video");

    // lv_obj_t * cont = create_cont(video_win);
    create_ctrl_box(video_win);
}

/*读取music目录下歌词文件名并将它存入数组*/

/*显示视屏播放器返回按钮, 并注册事件*/
static void show_video_back_btn(void)
{
    // 创建一个返回按钮
    lv_obj_t *btn_video_back = lv_btn_create(video_win);
    // 设置按钮的大小
    lv_obj_set_size(btn_video_back, 80, 50);
    // 把按钮放入左上角
    lv_obj_align(btn_video_back, LV_ALIGN_TOP_LEFT, -20, -20);

    // 添加按钮的事件 ，当按钮 被点击后就会触发  event_cb 函数
    lv_obj_add_event_cb(btn_video_back, video_back, LV_EVENT_CLICKED, NULL);

    // 创建一个标签对象  ，在按钮上
    lv_obj_t *label1 = lv_label_create(btn_video_back);
    // 设置标签的文字
    lv_label_set_text(label1, "< back");
    // 把标签放入按钮的中央
    lv_obj_center(label1);
}

/*从video player返回触发的事件*/
static void video_back(lv_event_t *e)
{
    // 获取事件号
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        if(fp != NULL)
        {
            if(check_process("mplayer"))
                printf("errno = %d\n", errno);
            taskquit = 1;
            system("killall -9 mplayer");
            {
                // system("echo stop > /root/project2/mpipe");
                // system("echo quit 0 > /root/project2/mpipe");

                printf("errno = %d\n", errno);
                pthread_cancel(tid);
                printf("等待回收线程\n");
                printf("errno = %d\n", errno);
                pthread_join(tid, NULL);
                tid = 0;
                printf("视频播放线程回收完成\n");
            }

            taskquit = 1;
        }
        lv_obj_del(video_win);
        main_menu();  //显示主界面
    }
}

/*读取video文件夹里的视频并显示play list*/
static void show_video_play_list(char *dp)
{
    SaveFilePath(dp, ".avi", video_name);

    /*Create a list*/
    list1 = lv_list_create(video_win);
    lv_obj_set_size(list1, 190, 280);
    lv_obj_align(list1, LV_ALIGN_LEFT_MID, -20, -42);

    /*Add buttons to the list*/
    lv_obj_t *btn;

    lv_list_add_text(list1, "video list");
    for(int i = 0;i < 8;i++)
    {
        if(video_name[i][0] == 0)
        {
            break;
        }
        // printf("path = %s\n", video_name[i]);
        btn = lv_list_add_btn(list1, LV_SYMBOL_VIDEO, video_name[i]);
        lv_obj_add_event_cb(btn, video_play, LV_EVENT_CLICKED, NULL);
    }
}

/*视屏播放事件处理*/
static void video_play(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED)
    {
        bzero(name, sizeof(name));
        strcpy(name, lv_list_get_btn_text(list1, obj));
        LV_LOG_USER("Clicked: %s", name);
        for(int i = 0;i < 8;i++)
        {
            if(video_name[i][0] == 0)
            {
                break;
            }

            if(strcmp(video_name[i], name) == 0)
            {
                if(fp != NULL && tid != 0)
                {
                    taskquit = 1;
                    printf("errno = %d\n", errno);
                    pthread_cancel(tid);
                    printf("errno = %d\n", errno);
                    printf("等待回收线程\n");
                    pthread_join(tid, NULL);
                    tid = 0;
                    printf("视频播放线程回收完成\n");

                    system("echo stop > /root/project2/mpipe");
                    system("echo quit 0 > /root/project2/mpipe");
                    // system("killall -9 mplayer");
                }

                LV_IMG_DECLARE(img_lv_demo_music_btn_play);
                LV_IMG_DECLARE(img_lv_demo_music_btn_pause);
                lv_obj_del(play_obj);
                play_obj = lv_imgbtn_create(video_win);
                lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);
                lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL);
                lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
                lv_obj_align(play_obj, LV_ALIGN_BOTTOM_MID, 0, 20);
                // lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

                lv_obj_add_event_cb(play_obj, play_event_click_cb, LV_EVENT_CLICKED, NULL);
                lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);
                // if(play_obj != NULL)
                // {
                //     lv_obj_clear_state(play_obj, LV_STATE_CHECKED);
                // }

                // 开始播放新的视频
                taskquit = 0;
                int ret = pthread_create(&tid, NULL, play_video_task, NULL);
                printf("创建新线程\n");
                if(ret < 0)
                {
                    perror("pthread_create failed");
                    return;
                }
                // break;
            }
        }
    }
}

void *play_video_task(void *arg)
{
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, 18);
    sigdelset(&set, 19);
    // sigdelset(&set, 2);
    int ret = sigprocmask(SIG_BLOCK, &set, NULL);  //屏蔽信号
    if(ret == 0)
    {
        printf("sigprocmask success\n");
    }
    else
    {
        printf("sigprocmask failed\n");
    }

    signal(SIGALRM, get_play_info);  //注册一个定时信号
    alarm(1);

    // 注册一个取消处理函数
    // pthread_cleanup_push(clean_func, NULL);

    // 加载mplayer 播放器进程
    char cmd[1024] = { 0 };
    sprintf(cmd, "mplayer -slave -quiet -input file=/root/project2/mpipe  -geometry 200:30 -zoom -x 582 -y 333  /root/project2/video/%s", name);
    fp = popen(cmd, "r");
    system("echo volume 10 1 > /root/project2/mpipe");

    pthread_mutex_lock(&mutex);
    lv_slider_set_value(slider_volume, (10), LV_ANIM_ON);  //设置进度条位置
    pthread_mutex_unlock(&mutex);

    while(1)
    {
        printf("------>errno = %d\n", errno);
        char buf[1024] = { 0 };
        char *p = fgets(buf, 1024, fp); // 读取播放器返回的信息
        if(p == NULL)
        {
            break;
        }
        printf("msg=%s\n", p);
        if(strncmp(buf, "ANS_TIME", 8) == 0)
        {
            strcpy(time_pos, buf + 18);
            double timeplay = atof(time_pos);
            int time_int = (int)timeplay;
            // printf("timeplay = %f\n", timeplay);
            printf("time_int = %u\n", time_int);

            pthread_mutex_lock(&mutex);
            if(time_obj != NULL)
            {
                lv_label_set_text_fmt(time_obj, "%"LV_PRIu32":%02"LV_PRIu32, time_int / 60, time_int % 60);
            }
            else
            {
                printf("你们不要再打啦\n");
            }
            pthread_mutex_unlock(&mutex);
        }
        else if(strncmp(buf, "ANS_PERCENT", 11) == 0)
        {
            strcpy(percent_pos, buf + 21);

            pthread_mutex_lock(&mutex);
            if(slider_obj != NULL)
            {
                lv_slider_set_value(slider_obj, atoi(percent_pos), LV_ANIM_ON);  //设置进度条位置
            }
            else
            {
                printf("你们也不要再打啦\n");
            }
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_mutex_unlock(&mutex);
    pclose(fp);
    fp = NULL;
    printf("播放线程退出了\n");
    // 注销取消处理函数
    // pthread_cleanup_pop(0);
    return NULL;
}

void get_play_info(int arg)
{
    if(fp == NULL)
    {
        printf("pipe have no reader\n");
        // system("killall -2 demo");
        return;
    }
    else
    {
        if(taskquit == 1)
        {
            printf("不再定时\n");
            taskquit = 0;
            return;
        }
        system("echo get_time_pos > /root/project2/mpipe");

        usleep(100);
        system("echo get_percent_pos > /root/project2/mpipe");
        alarm(1);
    }
}


static lv_obj_t *create_ctrl_box(lv_obj_t *parent)
{
    /*Create the control box*/
    // lv_obj_t * cont = lv_obj_create(parent);
    // static const lv_coord_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    // static const lv_coord_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    // lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    // LV_IMG_DECLARE(img_lv_demo_music_btn_loop);
    // LV_IMG_DECLARE(img_lv_demo_music_btn_rnd);
    LV_IMG_DECLARE(img_lv_demo_music_btn_next);
    LV_IMG_DECLARE(img_lv_demo_music_btn_prev);
    LV_IMG_DECLARE(img_lv_demo_music_btn_play);
    LV_IMG_DECLARE(img_lv_demo_music_btn_pause);

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_img_recolor_opa(&style1, LV_OPA_30);
    lv_style_set_img_recolor(&style1, lv_color_black());

    lv_obj_t *icon;
    // icon = lv_img_create(parent);
    // lv_img_set_src(icon, &img_lv_demo_music_btn_rnd);
    // lv_obj_align(icon, LV_ALIGN_BOTTOM_LEFT, 150, 0);
    // lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // icon = lv_img_create(parent);
    // lv_img_set_src(icon, &img_lv_demo_music_btn_loop);
    // lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    // lv_obj_align(icon, LV_ALIGN_BOTTOM_RIGHT, -150, 0);

    icon = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(icon, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_prev, NULL);
    lv_obj_set_size(icon, img_lv_demo_music_btn_prev.header.w, img_lv_demo_music_btn_prev.header.h);
    // lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(icon, LV_ALIGN_BOTTOM_MID, -100, 15);
    lv_obj_add_style(icon, &style1, LV_STATE_PRESSED);
    // lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, prev_click_event_cb, LV_EVENT_CLICKED, NULL);

    play_obj = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);
    lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align(play_obj, LV_ALIGN_BOTTOM_MID, 0, 20);
    // lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_add_event_cb(play_obj, play_event_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);

    icon = lv_imgbtn_create(parent);
    lv_imgbtn_set_src(icon, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_next, NULL);
    lv_obj_set_size(icon, img_lv_demo_music_btn_next.header.w, img_lv_demo_music_btn_next.header.h);
    lv_obj_align(icon, LV_ALIGN_BOTTOM_MID, 100, 15);
    lv_obj_add_style(icon, &style1, LV_STATE_PRESSED);
    // lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, next_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    LV_IMG_DECLARE(img_lv_demo_music_slider_knob);
    slider_obj = lv_slider_create(parent);
    lv_obj_set_style_anim_time(slider_obj, 100, -15);
    lv_obj_add_flag(slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/

    lv_obj_set_width(slider_obj, 600);
    lv_obj_set_height(slider_obj, 5);
    // lv_slider_set_value(slider_obj, 80, LV_ANIM_ON);  //设置进度条位置
    // lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 1, 4, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_align(slider_obj, LV_ALIGN_BOTTOM_MID, 0, -65);
    lv_obj_add_event_cb(slider_obj, slider_release_event, LV_EVENT_RELEASED, NULL);

    lv_obj_set_style_bg_img_src(slider_obj, &img_lv_demo_music_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_obj, 20, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_obj, 0, 0);

    slider_volume = lv_slider_create(parent);
    lv_obj_set_style_anim_time(slider_volume, 100, -15);
    lv_obj_add_flag(slider_volume, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/

    lv_obj_set_width(slider_volume, 150);
    lv_obj_set_height(slider_volume, 5);
    lv_slider_set_value(slider_volume, 10, LV_ANIM_ON);  //设置进度条位置
    // lv_obj_set_grid_cell(slider_volume, LV_GRID_ALIGN_STRETCH, 1, 4, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_align(slider_volume, LV_ALIGN_BOTTOM_LEFT, 80, -15);
    lv_obj_add_event_cb(slider_volume, slider_volume_release_event, LV_EVENT_RELEASED, NULL);

    lv_obj_set_style_bg_img_src(slider_volume, &img_lv_demo_music_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider_volume, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_volume, 20, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_volume, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_volume, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_volume, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_volume, 0, 0);

    img_volume = lv_img_create(parent);
    lv_img_set_src(img_volume, "S:/root/project2/img/volume.png");
    lv_obj_set_size(img_volume, 60, 60);
    lv_obj_align_to(img_volume, slider_volume, LV_ALIGN_OUT_LEFT_MID, 10, 6);

    time_obj = lv_label_create(parent);
    // lv_obj_set_style_text_font(time_obj, font_small, 0);
    lv_obj_set_style_text_color(time_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_obj, "0:00");
    lv_obj_align(time_obj, LV_ALIGN_BOTTOM_RIGHT, -30, -58);
    // lv_obj_set_grid_cell(time_obj, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    return parent;
}

static void play_event_click_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    if(lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
        printf(">>>>>>>>>>>>>>>>>>>暂停\n");
        system("killall -19 mplayer");  //SIGSTOP
        lv_anim_del(&a, NULL);
        // system("echo pause > /root/project2/mpipe");

        // lv_anim_init(&a);
        lv_img_set_angle(imgbtn_album, angle_current);
        lv_anim_set_time(&a, 0);
        lv_anim_set_var(&a, imgbtn_album);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a, lv_anim_path_linear);

        lv_anim_set_exec_cb(&a, anim_angle_cb);
        lv_anim_set_values(&a, angle_current, 3600);
        lv_anim_start(&a);
    }
    else
    {
        printf(">>>>>>>>>>>>>>>>>>>继续\n");
        system("killall -18 mplayer");  //SIGCONT
        // system("echo pause > /root/project2/mpipe");

        // lv_anim_init(&a);
        lv_anim_set_time(&a, 6000);
        lv_anim_set_var(&a, imgbtn_album);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a, lv_anim_path_linear);

        lv_anim_set_exec_cb(&a, anim_angle_cb);
        lv_anim_set_values(&a, 0, 3600);
        lv_anim_start(&a);
    }
}

static void prev_click_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        if(fp != NULL)
        {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>后退\n");
            system("echo seek -10 > /root/project2/mpipe");
        }
    }
}

static void next_click_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        if(fp != NULL)
        {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>快进\n");
            system("echo seek +10 > /root/project2/mpipe");
        }
    }
}

static void slider_release_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_RELEASED)
    {
        slider_value = lv_slider_get_value(slider_obj);
        if(fp != NULL && (slider_value - atoi(percent_pos) < -2 || slider_value - atoi(percent_pos) > 2))
        {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>重设时间\n");
            char cmd[512] = { 0 };
            sprintf(cmd, "echo seek %d 1 > /root/project2/mpipe", slider_value);
            system(cmd);
        }
    }
}

static void slider_volume_release_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_RELEASED)
    {
        int32_t slider_volume_value = lv_slider_get_value(slider_volume);
        if(fp != NULL)
        {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>设置音量\n");
            char cmd[512] = { 0 };
            sprintf(cmd, "echo volume %d 1 > /root/project2/mpipe", slider_volume_value);
            system(cmd);
        }
    }
}







//================================================================================================================




/***************************************************music播放器界面*************************************************/
/*显示音乐播放窗口，相关事件注册*/
static void show_music_win(void)
{
    SaveFilePath("/root/project2/music", ".lrc", lrc_name);//将lrc文件名存入数组中

    music_win = lv_obj_create(lv_scr_act());  //创建music player窗口
    lv_obj_set_size(music_win, 800, 480);

    // 在音乐界面中创建一个按钮
    lv_obj_t *label = lv_label_create(music_win);

    // 设置标签的内容
    lv_label_set_text(label, "Music Player");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, -10);

    /*读取music文件夹里的音乐并显示play list*/
    show_music_play_list("/root/project2/music");

    show_music_back_btn();

    create_ctrl_box(music_win); //显示相关控制按钮并注册

    /*Create an image button*/
    // lv_obj_t *imgbtn1 = lv_imgbtn_create(music_win);
    // lv_imgbtn_set_src(imgbtn1, LV_IMGBTN_STATE_RELEASED, NULL, "S:/root/project2/music/Night Visions - Imagine Dragons.png", NULL);//专辑封面图
    //                                                             // S:/root/project2/music/Night Visions - Imagine Dragons.png
    // lv_obj_set_size(imgbtn1, 220, 220);
    // lv_obj_align(imgbtn1, LV_ALIGN_CENTER, -30, -50);//图标相对屏幕的位置
}


/*显示返回按钮, 并注册事件*/
static void show_music_back_btn(void)
{
    // 创建一个返回按钮
    lv_obj_t *btn_music_back = lv_btn_create(music_win);
    // 设置按钮的大小
    lv_obj_set_size(btn_music_back, 80, 50);
    // 把按钮放入左上角
    lv_obj_align(btn_music_back, LV_ALIGN_TOP_LEFT, -20, -20);

    // 添加按钮的事件 ，当按钮 被点击后就会触发  event_cb 函数
    lv_obj_add_event_cb(btn_music_back, music_back, LV_EVENT_CLICKED, NULL);

    // 创建一个标签对象  ，在按钮上
    lv_obj_t *label1 = lv_label_create(btn_music_back);
    // 设置标签的文字
    lv_label_set_text(label1, "< back");
    // 把标签放入按钮的中央
    lv_obj_center(label1);
}

/*从music player返回触发的事件*/
static void music_back(lv_event_t *e)
{
    // 获取事件号
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        if(fp != NULL && tid != 0)
        {
            if(check_process("mplayer"))
            {
                taskquit = 1;
                system("killall -9 mplayer");
                usleep(1000);

                pthread_cancel(tid);
                printf("等待回收线程\n");
                printf("errno = %d\n", errno);
                pthread_join(tid, NULL);
                tid = 0;
                printf("音乐播放线程回收完成\n");

                // system("echo stop > /root/project2/mpipe");
                // system("echo quit 0 > /root/project2/mpipe");
            }

        }
        if(music_win != NULL)
        {
            lv_obj_del(music_win);
            music_win = NULL;
        }
        main_menu();  //显示主界面
    }
}



/*读取music文件夹里的音乐并显示play list*/
static void show_music_play_list(char *dp)
{
    SaveFilePath(dp, ".mp3", music_name);
    SaveFilePath(dp, ".png", album_name);

    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = "/usr/share/fonts/DroidSansFallback.ttf";
    info.weight = 15;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info))
    {
        LV_LOG_ERROR("create failed.");
    }
    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, info.font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a list*/
    list2 = lv_list_create(music_win);
    lv_obj_set_size(list2, 230, 280);
    lv_obj_align(list2, LV_ALIGN_LEFT_MID, -20, -42);
    // lv_obj_add_style(list2, &style, 0);

    /*Add buttons to the list*/
    lv_obj_t *btn;

    lv_list_add_text(list2, "music list");
    for(int i = 0;i < 8;i++)
    {
        if(music_name[i][0] == 0)
        {
            break;
        }

        btn = lv_list_add_btn(list2, LV_SYMBOL_AUDIO, music_name[i]);
        // lv_obj_add_style(btn, &style, 0);  //添加中文样式
        lv_obj_add_event_cb(btn, music_play, LV_EVENT_CLICKED, NULL);
    }
}


/*音乐播放事件处理*/
static void music_play(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED)
    {
        bzero(name, sizeof(name));
        strcpy(name, lv_list_get_btn_text(list1, obj));  //获取按钮对应的文本信息 song_name - singer.mp3
        LV_LOG_USER("Clicked: %s", name);
        for(int i = 0;i < 8;i++)
        {
            if(music_name[i][0] == 0)
            {
                break;
            }

            if(strcmp(music_name[i], name) == 0)
            {
                if(fp != NULL && tid != 0)
                {
                    taskquit = 1;
                    usleep(1000);
                    system("killall -9 mplayer");
                    // system("echo stop > /root/project2/mpipe");
                    // system("echo quit 0 > /root/project2/mpipe");

                    pthread_cancel(tid);
                    printf("等待回收线程\n");
                    printf("errno = %d\n", errno);
                    pthread_join(tid, NULL);
                    tid = 0;
                    // printf("音乐播放线程回收完成\n");
                }

                LV_IMG_DECLARE(img_lv_demo_music_btn_play);
                LV_IMG_DECLARE(img_lv_demo_music_btn_pause);
                lv_obj_del(play_obj);
                play_obj = lv_imgbtn_create(music_win);
                lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);
                lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL);
                lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
                lv_obj_align(play_obj, LV_ALIGN_BOTTOM_MID, 0, 20);
                // lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

                lv_obj_add_event_cb(play_obj, play_event_click_cb, LV_EVENT_CLICKED, NULL);
                lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);
                // if(play_obj != NULL)
                // {
                //     lv_obj_clear_state(play_obj, LV_STATE_CHECKED);
                // }
                if(play_info_obj != NULL)
                {
                    lv_obj_del(play_info_obj);
                    play_info_obj = NULL;
                }

                /*打印一下歌曲路径是否正确*/
                char song_path[512] = { 0 };
                sprintf(song_path, "/root/project2/music/%s", name);
                printf("song_path = %s\n", song_path);

                bzero(songname, sizeof(songname));
                get_str1(name);
                printf("songname = %s\n", songname);
                bzero(artist, sizeof(artist));
                get_str2(name);
                printf("artist = %s\n", artist);

                int j = 0;
                for(j = 0;j < 8;j++)
                {
                    if(strstr(album_name[j], artist) != NULL)
                    {
                        bzero(album, sizeof(album));
                        get_album(album_name[j]);
                        printf("album = %s\n", album);
                        break;
                    }
                }

                char path[512] = { 0 };
                sprintf(path, "S:/root/project2/music/%s", album_name[j]);
                show_music_play_info(path, songname, artist, album);

                /*将lrc文件中的歌词读入数组*/
                char *pAddr = strstr(name, ".mp3");
                int length = (int)pAddr - (int)name;
                // (char *)name;
                for(int i = 0;i < 8;i++)
                {
                    if(lrc_name[i][0] == 0)
                    {
                        break;
                    }
                    else if(strncmp(lrc_name[i], name, length) == 0)
                    {
                        music_lyric_save(lrc_name[i]);//拿到歌词
                    }
                }

                // 开始播放新的音乐
                taskquit = 0;
                int ret = pthread_create(&tid, NULL, play_music_task, NULL);
                printf("创建新线程\n");
                if(ret < 0)
                {
                    perror("pthread_create failed");
                    return;
                }
                break;
            }
        }
    }
}

static void *play_music_task(void *arg)
{
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, 18);
    sigdelset(&set, 19);
    // sigdelset(&set, 2);
    int ret = sigprocmask(SIG_BLOCK, &set, NULL);  //屏蔽信号
    if(ret == 0)
    {
        printf("sigprocmask success\n");
    }
    else
    {
        printf("sigprocmask failed\n");
    }

    signal(SIGALRM, get_play_info);  //注册一个定时信号
    alarm(1);

    // 注册一个取消处理函数
    // pthread_cleanup_push(clean_func, NULL);

    // 加载mplayer 播放器进程
    char cmd[1024] = { 0 };
    sprintf(cmd, "mplayer -slave -quiet -input file=/root/project2/mpipe '/root/project2/music/%s'", name);
    printf("cmd = %s\n", cmd);
    fp = popen(cmd, "r");
    system("echo volume 10 1 > /root/project2/mpipe");
    system("echo get_time_pos > /root/project2/mpipe");

    pthread_mutex_lock(&mutex);
    lv_slider_set_value(slider_volume, (10), LV_ANIM_ON);  //设置进度条位置
    pthread_mutex_unlock(&mutex);

    while(1)
    {
        if(errno == EAGAIN)
        {
            printf("球球你了\n");
            break;
        }
        char buf[1024] = { 0 };
        char *p = fgets(buf, 1024, fp); // 读取播放器返回的信息
        if(p == NULL)
        {
            break;
        }

        // printf("msg=%s\n", p);
        if(strncmp(buf, "ANS_TIME", 8) == 0)
        {
            strcpy(time_pos, buf + 18);
            double timeplay = atof(time_pos);
            int time_int = (int)timeplay;//获取到播放的秒数time_int
            // printf("timeplay = %f\n", timeplay);
            printf("time_int = %u\n", time_int);

            pthread_mutex_lock(&mutex);
            if(time_obj != NULL)
            {
                lv_label_set_text_fmt(time_obj, "%"LV_PRIu32":%02"LV_PRIu32, time_int / 60, time_int % 60);
            }
            pthread_mutex_unlock(&mutex);

            int time_lrc = 0;
            for(int i = 0;i < 256;i++)
            {
                if(lrc_lines[i][0] == 0)
                {
                    break;
                }
                time_lrc = (lrc_lines[i][2] - 48) * 60 + (lrc_lines[i][4] - 48) * 10 + (lrc_lines[i][5] - 48);
                if(time_int == time_lrc)
                {
                    pthread_mutex_lock(&mutex);
                    printf("lrc line:%s\n", lrc_lines[i]);
                    lv_label_set_text_fmt(label_lrc, "#AE86FC %s#", lrc_lines[i] + 10);
                    pthread_mutex_unlock(&mutex);
                    break;
                }
            }
        }
        else if(strncmp(buf, "ANS_PERCENT", 11) == 0)
        {
            strcpy(percent_pos, buf + 21);

            pthread_mutex_lock(&mutex);
            if(slider_obj != NULL)
            {
                lv_slider_set_value(slider_obj, atoi(percent_pos), LV_ANIM_ON);  //设置进度条位置
            }
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            // pthread_mutex_lock(&mutex);
            // if(slider_obj != NULL)
            // {
            //     slider_value = lv_slider_get_value(slider_obj);  //获取进度条百分比
            // }
            // pthread_mutex_unlock(&mutex);
        }

        // if(taskquit == 1)
        // {
        //     taskquit = 0;
        //     break;
        // }
    }

    pthread_mutex_lock(&mutex);
    lv_img_set_angle(imgbtn_album, angle_current);
    lv_anim_set_time(&a, 0);
    lv_anim_set_var(&a, imgbtn_album);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_exec_cb(&a, anim_angle_cb);
    lv_anim_set_values(&a, angle_current, 3600);
    lv_anim_start(&a);
    pthread_mutex_unlock(&mutex);  //退出确保解锁

    pclose(fp);
    fp = NULL;
    printf("music播放线程退出了\n");
    // 注销取消处理函数
    // pthread_cleanup_pop(0);
    return NULL;
}

static void show_music_play_info(char *pic_path, char *song_name, char *singer, char *album_name)
{

    /*Darken the button when pressed and make it wider*/
    // static lv_style_t style_pr;
    // lv_style_init(&style_pr);
    // lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    // lv_style_set_img_recolor(&style_pr, lv_color_black());


    play_info_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(play_info_obj, 520, 300);
    lv_obj_set_pos(play_info_obj, 250, 40);

    /*Create an image button*/
    printf("pic_path:%s\n", pic_path);
    imgbtn_album = lv_img_create(play_info_obj);
    // lv_obj_set_style_radius(imgbtn_album, 110, LV_STATE_DEFAULT);

    if(strstr(pic_path, "Jane Zhang") != NULL)
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/The One - Jane Zhang.png");
    }
    else if(strstr(pic_path, "Imagine Dragons") != NULL)
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/Night Visions - Imagine Dragons.png");
    }
    else if(strstr(pic_path, "Martin Garrix, Bebe Rexha") != NULL)
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/In The Name Of Love - Martin Garrix, Bebe Rexha.png");
    }
    else if(strstr(pic_path, "Easton Corbin") != NULL)
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/All Over The Road - Easton Corbin.png");
    }
    else if(strstr(pic_path, "Al Rocco") != NULL)
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/All On Me - Al Rocco.png");
    }
    else if(strstr(pic_path, "The Weeknd ; Daft Punk") != NULL)
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/I Feel It Coming - The Weeknd ; Daft Punk.png");
    }
    else
    {
        lv_img_set_src(imgbtn_album, "S:/root/project2/music/I Feel It Coming - The Weeknd ; Daft Punk.png");
    }
    lv_obj_set_size(imgbtn_album, 220, 220);
    lv_obj_align(imgbtn_album, LV_ALIGN_LEFT_MID, 0, -30);
    lv_img_set_pivot(imgbtn_album, 110, 110);

    lv_anim_init(&a);
    lv_anim_set_time(&a, 6000);
    lv_anim_set_var(&a, imgbtn_album);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);

    lv_anim_set_exec_cb(&a, anim_angle_cb);
    lv_anim_set_values(&a, 0, 3600);
    lv_anim_start(&a);

    /*显示歌曲名称*/
    printf("song_name:%s\n", song_name);
    char buf[512] = { 0 };
    sprintf(buf, "name:   %s", song_name);
    label_song = lv_label_create(play_info_obj);
    lv_label_set_text(label_song, buf);
    lv_obj_align_to(label_song, imgbtn_album, LV_ALIGN_OUT_RIGHT_MID, 25, -80);

    /*显示歌手*/
    printf("singer:%s\n", singer);
    bzero(buf, sizeof(buf));
    sprintf(buf, "singer:  %s", singer);
    label_singer = lv_label_create(play_info_obj);
    lv_label_set_text(label_singer, buf);
    lv_obj_align_to(label_singer, imgbtn_album, LV_ALIGN_OUT_RIGHT_MID, 20, -50);

    /*显示专辑名称*/
    printf("album_name:%s\n", album_name);
    bzero(buf, sizeof(buf));
    sprintf(buf, "album:  %s", album_name);
    label_album = lv_label_create(play_info_obj);
    lv_label_set_text(label_album, buf);
    lv_obj_align_to(label_album, imgbtn_album, LV_ALIGN_OUT_RIGHT_MID, 20, -20);

    /*显示歌词*/
    label_lrc = lv_label_create(play_info_obj);
    lv_label_set_long_mode(label_lrc, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
    lv_label_set_text(label_lrc, "Welcome to Music");
    lv_obj_align_to(label_lrc, play_info_obj, LV_ALIGN_BOTTOM_LEFT, 20, 0);
    lv_label_set_recolor(label_lrc, true);                      /*Enable re-coloring by commands in the text*/
    lv_obj_set_width(label_lrc, 380);
    lv_obj_set_height(label_lrc, 200);
}

static void anim_angle_cb(void *var, int32_t v)
{
    angle_current = v;
    lv_img_set_angle(var, v);
}

static void music_lyric_save(char *lyric_file)
{
    bzero(lrc_lines, 256 * 256);
    int line_num = 256; //sizeof(lrc)/sizeof(*lrc);

    char fp[256] = { 0 };
    sprintf(fp, "/root/project2/music/%s", lyric_file);
    FILE *f = fopen(fp, "r");
    if(NULL == f)
    {
        perror("fopen failed");
        return;
    }

    for(int i = 0;i < line_num;i++)
    {
        int line_len = freadline(f, lrc_lines[i], 256);
        if(line_len == 0)
        {
            break;
        }
        // printf("line%d = %s\n", i, lrc_lines[i]);
    }
}

/*读取文件中的一行存入数组fline中，数组长度为len(必须要足够大，否则截断)
*成功返回从该行读取到的字节数(空行时为0), 失败返回-1*/
static size_t freadline(FILE *f, char *fline, int len)
{
    bzero(fline, len);
    size_t size = 0;
    char c = 0;
    int ret = 0;
    for(int i = 0;c != '\n';i++)
    {
        ret = fread(&c, 1, 1, f);
        if(ret == 0)
        {
            printf("read a empty line\n");
            return 0;
        }

        if(i >= len)
        {
            printf("buf have not enough capacity\n");
            return -1;
        }

        *(fline + i) = c;
        size++;
    }

    return size;
}


static void get_album(char *str)
{
    char *p = strstr(str, "-");
    if(NULL == p)
    {
        perror("invalid string");
        return;
    }

    int addr1 = 0, len = 0;
    addr1 = (int)str;
    len = (int)p - 1 - addr1;  //len为str1的长度
    strncpy(album, (char *)str, len);
}

/*获取str1 - str2.mp3的str1, 参数str为name(这个是针对全局变量name用的)*/
static void get_str1(char *str)
{
    char *p = strstr(str, "-");
    if(NULL == p)
    {
        perror("invalid string");
        return;
    }

    int addr1 = 0, len = 0;
    addr1 = (int)str;
    len = (int)p - 1 - addr1;  //len为str1的长度
    strncpy(songname, (char *)str, len);
}

static void get_str2(char *str)
{
    char *p = strstr(str, "-");
    if(NULL == p)
    {
        perror("invalid string");
        return;
    }

    int addr1 = 0, len = 0;
    addr1 = (int)p + 2;
    char *point = strstr(str, ".mp3");
    if(NULL == point)
    {
        perror("invalid string");
        return;
    }
    len = (int)point - addr1;
    strncpy(artist, (char *)addr1, len);
}








//=======================================================================================================================
/*label使用示例*/
void lv_example_label_my_name(void)
{
    lv_obj_t *label2 = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/

    lv_label_set_recolor(label2, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_text(label2, "#0000ff Liao# #ff00ff Jie# #ff0000 Wen #, It's nice to see you!");
    lv_obj_set_width(label2, 200);
    lv_obj_set_height(label2, 200);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 40);
}


// 按钮事件函数，
static void event_cb(lv_event_t *e)
{
    LV_LOG_USER("Clicked");

    static uint32_t cnt = 1;
    // 获取当前触发事件的对象
    lv_obj_t *btn = lv_event_get_target(e);
    // 获取当前触发事件的子对象
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    // 设置标签的文本格式
    lv_label_set_text_fmt(label, "%" LV_PRIu32, cnt);
    cnt++;
}

/**
 * Add click event to a button
 */
void lv_mybtn(void)
{

    // 创建一个按钮对象
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    // 设置按钮的大小
    lv_obj_set_size(btn, 200, 200);
    // 把按钮放入中央
    lv_obj_center(btn);
    // lv_obj_set_pos(btn, 100, 100);

    //  添加按钮的事件 ，当按钮 被点击后就会触发  event_cb 函数
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *img;
    // 在按钮上创建可乐图片
    img = lv_img_create(btn);
    /* Assuming a File system is attached to letter 'A'
     * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
    lv_img_set_src(img, "S:/1.png");
    // lv_obj_align(img, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_center(img);

    // 创建一个标签对象  ，在可乐图片上
    lv_obj_t *label = lv_label_create(img);
    // 设置标签的文字
    lv_label_set_text(label, "Coke 3 RMB");
    // 把标签放入按钮的中央
    lv_obj_center(label);
}

void show_png(int16_t x, int16_t y)
{

    lv_obj_t *img;
    img = lv_img_create(lv_scr_act());
    /* Assuming a File system is attached to letter 'A'

     * E.g. set LV_USE_FS_STDIO 'A' in lv_conf.h */
    lv_img_set_src(img, "S:/root/project2/img/music.png");  //S:/1.png

    // lv_obj_align(img, LV_ALIGN_RIGHT_MID, -20, 0);
    // lv_obj_center(img);
    lv_obj_set_pos(img, x, y);
}


void lv_example_imgbtn_my(void)
{
    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr, lv_color_black());
    lv_style_set_transform_height(&style_pr, 20);

    /*Create an image button*/
    lv_obj_t *imgbtn1 = lv_imgbtn_create(lv_scr_act());
    lv_obj_set_size(imgbtn1, 200, 200);
    lv_imgbtn_set_src(imgbtn1, LV_IMGBTN_STATE_RELEASED, NULL, "S:/root/project2/img/cafe.png", NULL);
    // lv_obj_add_style(imgbtn1, &style_def, 0);
    lv_obj_add_style(imgbtn1, &style_pr, LV_STATE_PRESSED);
    lv_obj_align(imgbtn1, LV_ALIGN_CENTER, 0, 0);

    /*Create a label on the image button*/
    lv_obj_t *label = lv_label_create(imgbtn1);
    lv_label_set_text(label, "cafe 3");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -50);
}

void show_font(int x, int y, char *msg)
{
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = "/usr/share/fonts/DroidSansFallback.ttf";
    info.weight = 32;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info))
    {
        LV_LOG_ERROR("create failed.");
    }

    /*Create style with the new font*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, info.font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

    /*Create a label with the new style*/
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text(label, msg);
    // lv_obj_center(label);
    lv_obj_set_pos(label, x, y);
}



#include "../lv_examples.h"
#if LV_BUILD_EXAMPLES && LV_USE_SWITCH

//按钮事件函数，
static void event_cb(lv_event_t * e)
{
    LV_LOG_USER("Clicked");

    static uint32_t cnt = 1;
    //获取当前触发事件的对象 
    lv_obj_t * btn      = lv_event_get_target(e);
    //获取当前触发事件的子对象
    lv_obj_t * label    = lv_obj_get_child(btn, 0);
    //设置标签的文本格式
    lv_label_set_text_fmt(label, "%" LV_PRIu32, cnt);
    cnt++;
}

/**
 * Add click event to a button
 */
void lv_example_event_1(void)
{
    // 创建一个按钮对象
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    // 设置按钮的大小
    lv_obj_set_size(btn, 100, 50);
    // 把按钮放入中央
    lv_obj_center(btn);
    // 添加按钮的事件 ，当按钮 被点击后就会触发  event_cb 函数
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    // 创建一个标签对象  ，在按钮上
    lv_obj_t * label = lv_label_create(btn);
    // 设置标签的文字
    lv_label_set_text(label, "Click me!");
    // 把标签放入按钮的中央
    lv_obj_center(label);
}

#endif

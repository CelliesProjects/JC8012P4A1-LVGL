#pragma GCC push_options
#pragma GCC optimize("O3")

#include <Arduino.h>
#include "lvgl.h"
#include "pins_config.h"
#include "lcd/jd9365_lcd.h"
#include "touch/gsl3680_touch.h"

jd9365_lcd lcd = jd9365_lcd(LCD_RST);
gsl3680_touch touch = gsl3680_touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

lv_display_t *disp_drv;
static uint32_t *buf;
static uint32_t *buf1;

static lv_obj_t *label;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map)
{
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    lcd.lcd_draw_bitmap(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data)
{
    bool touched;
    uint16_t touchX, touchY;

    touched = touch.getTouch(&touchX, &touchY);

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        data->point.x = touchX;
        data->point.y = touchY;
        Serial.printf("x=%d,y=%d \r\n", touchX, touchY);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ESP32P4 MIPI DSI LVGL");
    lcd.begin();
    touch.begin();

    lv_init();
    uint32_t buffer_size = LCD_H_RES * LCD_V_RES;

    buf = (uint32_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    buf1 = (uint32_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    assert(buf);
    assert(buf1);

    disp_drv = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp_drv, my_disp_flush);
    lv_display_set_buffers(disp_drv, buf, buf1, buffer_size * sizeof(uint32_t), LV_DISPLAY_RENDER_MODE_FULL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    constexpr int BIT_DEPTH = 14;
    constexpr int MAX_PWM_VAL = 1 << BIT_DEPTH;
    if (!ledcAttachChannel(LCD_LED, 1220, BIT_DEPTH, 0))
    {
        log_e("Error setting ledc pin %i. system halted", LCD_LED);
        while (1)
            delay(1000);
    }

    if (!ledcWrite(LCD_LED, MAX_PWM_VAL >> 4))
    {
        log_e("Error setting ledc value. system halted");
        while (1)
            delay(1000);
    }

    lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_main(LV_PALETTE_AMBER), 0);

    // create a style
    static lv_style_t style_big;
    lv_style_init(&style_big);
    lv_style_set_text_font(&style_big, &lv_font_montserrat_48);

    // create label
    label = lv_label_create(lv_scr_act());
    // apply style to label
    lv_obj_add_style(label, &style_big, 0);
    lv_label_set_text(label, "Hello World!");
    lv_obj_center(label);
}

void loop()
{
    lv_timer_handler();

    lv_indev_t *indev = lv_indev_get_next(nullptr);
    if (indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER)
    {
        lv_point_t point;
        lv_indev_get_point(indev, &point); // gets current or last point
        lv_indev_state_t state = lv_indev_get_state(indev);

        static lv_point_t last = {-1, -1};
        static bool last_pressed = false;

        if (state == LV_INDEV_STATE_PRESSED &&
            (point.x != last.x || point.y != last.y || !last_pressed))
        {
            last = point;
            last_pressed = true;

            char buf[64];
            snprintf(buf, sizeof(buf), "Touch: %d, %d", point.x, point.y);
            lv_label_set_text(label, buf);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_BLUE), 0);
        }
        else if (state == LV_INDEV_STATE_RELEASED && last_pressed)
        {
            last_pressed = false;
            lv_label_set_text(label, "JC8012P4A1 ESP32-P4");
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_NONE), 0);
        }
    }

    delay(5);
    lv_tick_inc(5);
}
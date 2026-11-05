#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "tft.h"
#define UI_SCENE_MAX_ELEMENTS 12
enum { UI_SCENE_RECT=1, UI_SCENE_TEXT=2, UI_SCENE_CREST=3 };
typedef struct {
    uint8_t type, font_size;
    int16_t x,y,w,h;
    uint32_t color;
    char text[49];
} ui_scene_element_t;
typedef struct {
    uint32_t version, background;
    uint8_t count;
    bool animate;
    ui_scene_element_t elements[UI_SCENE_MAX_ELEMENTS];
} ui_scene_t;
typedef struct {
    unsigned page;
    bool demo, stopwatch_running, clock_set;
    uint64_t uptime_ms, utc_epoch, stopwatch_ms;
    int timezone_minutes;
    unsigned rpm, speed, coolant;
    unsigned battery_mv; /* SIMULATED vehicle battery; not band/Pico ADC */
    unsigned rtos_free, rtos_min, lv_free, lv_max_used;
    unsigned stack_min[4];
    bool tasks_healthy;
    tft_config_t display;
    ui_scene_t scene;
} ui_snapshot_t;
bool ui_init(void);
void ui_boot(const char *title);
void ui_show_logo(void);
void ui_show_grid(void);
void ui_show_animation(void);
void ui_render(const ui_snapshot_t *state);
void ui_poll(void);
void ui_memory(unsigned *free_bytes,unsigned *max_used);
void ui_resize(void);
unsigned ui_padded_flush_count(void);
unsigned ui_rendered_frames(void);

#include "ui.h"
#include "lvgl.h"
#include "pico/stdlib.h"
#include "build_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static lv_display_t *display;
static lv_obj_t *title,*primary,*details,*footer;
static lv_obj_t *logo_layer;
static lv_obj_t *grid_layer;
static lv_obj_t *logo_crest,*scene_layer;
static lv_obj_t *scene_crests[UI_SCENE_MAX_ELEMENTS];
static unsigned scene_crest_count;
static uint32_t scene_version=UINT32_MAX;
static bool logo_animating,scene_animating;
static unsigned padded_flush_count;
static unsigned rendered_frames;
extern const lv_image_dsc_t cadillac_crest_image;
static uint16_t draw_a[160*10] __attribute__((aligned(4)));
static uint16_t draw_b[160*10] __attribute__((aligned(4)));
static uint32_t milliseconds(void) { return (uint32_t)(time_us_64()/1000); }
static void crest_opacity(void *object,int32_t value) { lv_obj_set_style_opa(object,(lv_opa_t)value,0); }
static void crest_y(void *object,int32_t value) { lv_obj_set_y(object,value); }
static void pulse(lv_obj_t *object) {
    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a,object); lv_anim_set_exec_cb(&a,crest_opacity);
    lv_anim_set_values(&a,170,255); lv_anim_set_duration(&a,1800);
    lv_anim_set_playback_duration(&a,1800); lv_anim_set_repeat_count(&a,LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a,lv_anim_path_ease_in_out); lv_anim_start(&a);
}
static void stop_logo_animation(void) {
    if(!logo_animating) return;
    lv_anim_delete(logo_crest,crest_opacity); lv_anim_delete(logo_crest,crest_y);
    lv_obj_set_style_opa(logo_crest,LV_OPA_COVER,0); lv_obj_set_y(logo_crest,8);
    logo_animating=false;
}
static void stop_scene_animation(void) {
    if(!scene_animating) return;
    for(unsigned i=0;i<scene_crest_count;i++) {
        lv_anim_delete(scene_crests[i],crest_opacity);
        lv_obj_set_style_opa(scene_crests[i],LV_OPA_COVER,0);
    }
    scene_animating=false;
}
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *map) {
    lv_draw_buf_t *buffer=lv_display_get_buf_active(d);
    unsigned packed_stride=(unsigned)(a->x2-a->x1+1)*2;
    unsigned stride=buffer?buffer->header.stride:packed_stride;
    if(stride!=packed_stride) padded_flush_count++;
    tft_flush_strided(a->x1,a->y1,a->x2,a->y2,(const uint16_t *)map,stride);
    if(lv_display_flush_is_last(d)) rendered_frames++;
    lv_display_flush_ready(d);
}
static lv_obj_t *label(int y,int h,const lv_font_t *font) {
    lv_obj_t *o=lv_label_create(lv_screen_active());
    lv_obj_set_pos(o,4,y); lv_obj_set_size(o,tft_usable_width()-8,h);
    lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(o,font,0);
    lv_obj_set_style_text_color(o,lv_color_white(),0);
    return o;
}
void ui_resize(void) {
    stop_logo_animation(); stop_scene_animation();
    lv_display_set_resolution(display,tft_width(),tft_height());
    lv_obj_t *screen=lv_screen_active(); lv_obj_clean(screen);
    grid_layer=NULL; scene_layer=NULL; scene_crest_count=0; scene_version=UINT32_MAX;
    lv_obj_set_style_bg_color(screen,lv_color_black(),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_set_style_pad_all(screen,0,0);
    lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    title=label(2,12,&lv_font_montserrat_10);
    primary=label(18,24,&lv_font_montserrat_20);
    details=label(43,tft_usable_height()-56,&lv_font_montserrat_10);
    footer=label(tft_usable_height()-12,11,&lv_font_montserrat_10);
    lv_obj_set_style_text_color(footer,lv_color_hex(0x999999),0);
    lv_obj_t *accent=lv_obj_create(screen);
    lv_obj_set_pos(accent,0,15); lv_obj_set_size(accent,tft_usable_width(),1);
    lv_obj_set_style_bg_color(accent,lv_color_hex(0xa51626),0);
    lv_obj_set_style_border_width(accent,0,0);
    lv_obj_set_style_radius(accent,0,0);
    logo_layer=lv_obj_create(screen);
    lv_obj_set_pos(logo_layer,0,0); lv_obj_set_size(logo_layer,tft_usable_width(),tft_usable_height());
    lv_obj_set_style_bg_color(logo_layer,lv_color_black(),0);
    lv_obj_set_style_bg_opa(logo_layer,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(logo_layer,0,0);
    lv_obj_set_style_radius(logo_layer,0,0);
    lv_obj_set_style_pad_all(logo_layer,0,0);
    lv_obj_remove_flag(logo_layer,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *crest=lv_image_create(logo_layer);
    logo_crest=crest;
    lv_image_set_src(crest,&cadillac_crest_image);
    if(tft_usable_width()<112) lv_image_set_scale(crest,128);
    lv_obj_set_pos(crest,((int)tft_usable_width()-112)/2,8);
    lv_obj_t *wordmark=lv_label_create(logo_layer);
    lv_label_set_text(wordmark,"CADILLAC");
    lv_obj_set_style_text_font(wordmark,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_color(wordmark,lv_color_white(),0);
    lv_obj_align(wordmark,LV_ALIGN_BOTTOM_MID,0,-2);
    lv_obj_add_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
}
bool ui_init(void) {
    lv_init(); lv_tick_set_cb(milliseconds);
    display=lv_display_create(tft_width(),tft_height());
    if(!display) return false;
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,draw_a,draw_b,sizeof(draw_a),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    ui_resize();
    printf("LVGL initialized: RGB565 PARTIAL, 2x%u-byte buffers, single UI owner\n",(unsigned)sizeof(draw_a));
    return true;
}
void ui_boot(const char *text) {
    stop_logo_animation(); stop_scene_animation();
    if(scene_layer) lv_obj_add_flag(scene_layer,LV_OBJ_FLAG_HIDDEN);
    if(grid_layer) lv_obj_add_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(title,"M6 REBORN");
    lv_label_set_text(primary,text);
    lv_label_set_text(details,"Version " PICOOS_VERSION "\nRP2040 embedded environment");
    lv_label_set_text(footer,"PICO OS");
    lv_obj_set_style_text_color(primary,lv_color_white(),0);
    lv_refr_now(display);
}
void ui_show_logo(void) {
    stop_logo_animation(); stop_scene_animation();
    if(scene_layer) lv_obj_add_flag(scene_layer,LV_OBJ_FLAG_HIDDEN);
    if(grid_layer) lv_obj_add_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(display);
}
void ui_show_animation(void) {
    stop_scene_animation();
    if(grid_layer) lv_obj_add_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    if(scene_layer) lv_obj_add_flag(scene_layer,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
    if(!logo_animating) {
        /* Keep brightness constant: opacity animation looks like flashing on this panel. */
        lv_obj_set_style_opa(logo_crest,LV_OPA_COVER,0);
        lv_anim_t a; lv_anim_init(&a);
        lv_anim_set_var(&a,logo_crest); lv_anim_set_exec_cb(&a,crest_y);
        lv_anim_set_values(&a,3,18); lv_anim_set_duration(&a,1000);
        lv_anim_set_playback_duration(&a,1000); lv_anim_set_repeat_count(&a,LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a,lv_anim_path_ease_in_out); lv_anim_start(&a);
        logo_animating=true;
    }
}
void ui_show_grid(void) {
    stop_logo_animation(); stop_scene_animation();
    if(scene_layer) lv_obj_add_flag(scene_layer,LV_OBJ_FLAG_HIDDEN);
    bool changed=grid_layer==NULL || lv_obj_has_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    if(!grid_layer) {
        grid_layer=lv_obj_create(lv_screen_active());
        lv_obj_set_pos(grid_layer,0,0);
        lv_obj_set_size(grid_layer,tft_width(),tft_height());
        lv_obj_set_style_bg_color(grid_layer,lv_color_black(),0);
        lv_obj_set_style_bg_opa(grid_layer,LV_OPA_COVER,0);
        lv_obj_set_style_border_width(grid_layer,0,0);
        lv_obj_set_style_radius(grid_layer,0,0);
        lv_obj_set_style_pad_all(grid_layer,0,0);
        lv_obj_remove_flag(grid_layer,LV_OBJ_FLAG_SCROLLABLE);
        unsigned number=1;
        for(unsigned y=0;y<tft_height();y+=20) {
            for(unsigned x=0;x<tft_width();x+=20) {
                unsigned cell_number=number++;
                if(x>=tft_usable_width() || y>=tft_usable_height()) continue;
                lv_obj_t *cell=lv_obj_create(grid_layer);
                lv_obj_set_pos(cell,x,y); lv_obj_set_size(cell,20,20);
                lv_obj_set_style_bg_color(cell,lv_color_hex(((x+y)/20)&1?0x101010:0x000000),0);
                lv_obj_set_style_bg_opa(cell,LV_OPA_COVER,0);
                lv_obj_set_style_border_color(cell,lv_color_hex(0xaaaaaa),0);
                lv_obj_set_style_border_width(cell,1,0);
                lv_obj_set_style_radius(cell,0,0);
                lv_obj_set_style_pad_all(cell,0,0);
                lv_obj_remove_flag(cell,LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_t *text=lv_label_create(cell);
                char digits[8]; snprintf(digits,sizeof digits,"%u",cell_number);
                lv_label_set_text(text,digits);
                lv_obj_set_style_text_color(text,lv_color_white(),0);
                lv_obj_set_style_text_font(text,&lv_font_montserrat_12,0);
                lv_obj_center(text);
            }
        }
    }
    lv_obj_add_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    if(changed) lv_refr_now(display);
}
static void show_scene(const ui_scene_t *scene) {
    stop_logo_animation();
    if(grid_layer) lv_obj_add_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
    if(!scene_layer) {
        scene_layer=lv_obj_create(lv_screen_active());
        lv_obj_set_pos(scene_layer,0,0); lv_obj_set_size(scene_layer,tft_usable_width(),tft_usable_height());
        lv_obj_set_style_bg_opa(scene_layer,LV_OPA_COVER,0);
        lv_obj_set_style_border_width(scene_layer,0,0); lv_obj_set_style_pad_all(scene_layer,0,0);
        lv_obj_set_style_radius(scene_layer,0,0);
        lv_obj_remove_flag(scene_layer,LV_OBJ_FLAG_SCROLLABLE);
    }
    if(scene_version!=scene->version) {
        stop_scene_animation(); lv_obj_clean(scene_layer); scene_crest_count=0;
        lv_obj_set_style_bg_color(scene_layer,lv_color_hex(scene->background),0);
        for(unsigned i=0;i<scene->count && i<UI_SCENE_MAX_ELEMENTS;i++) {
            const ui_scene_element_t *e=&scene->elements[i];
            lv_obj_t *object;
            if(e->type==UI_SCENE_RECT) {
                object=lv_obj_create(scene_layer);
                lv_obj_set_style_bg_color(object,lv_color_hex(e->color),0);
                lv_obj_set_style_bg_opa(object,LV_OPA_COVER,0);
                lv_obj_set_style_border_width(object,0,0); lv_obj_set_style_radius(object,0,0);
                lv_obj_set_style_pad_all(object,0,0); lv_obj_remove_flag(object,LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_size(object,e->w,e->h);
            } else if(e->type==UI_SCENE_TEXT) {
                object=lv_label_create(scene_layer); lv_label_set_text(object,e->text);
                lv_label_set_long_mode(object,LV_LABEL_LONG_CLIP);
                lv_obj_set_size(object,e->w,e->h);
                const lv_font_t *font=e->font_size==20?&lv_font_montserrat_20:
                                      e->font_size==12?&lv_font_montserrat_12:&lv_font_montserrat_10;
                lv_obj_set_style_text_font(object,font,0);
                lv_obj_set_style_text_color(object,lv_color_hex(e->color),0);
            } else if(e->type==UI_SCENE_CREST) {
                object=lv_image_create(scene_layer); lv_image_set_src(object,&cadillac_crest_image);
                lv_image_set_pivot(object,0,0); lv_image_set_scale(object,(e->w*256+56)/112);
                scene_crests[scene_crest_count++]=object;
            } else continue;
            lv_obj_set_pos(object,e->x,e->y);
        }
        scene_version=scene->version;
    }
    lv_obj_remove_flag(scene_layer,LV_OBJ_FLAG_HIDDEN);
    if(scene->animate && !scene_animating) {
        for(unsigned i=0;i<scene_crest_count;i++) pulse(scene_crests[i]);
        scene_animating=true;
    } else if(!scene->animate) stop_scene_animation();
}
void ui_render(const ui_snapshot_t *s) {
    if(s->page==9) { ui_show_animation(); return; }
    if(s->page==10) { show_scene(&s->scene); return; }
    stop_logo_animation(); stop_scene_animation();
    if(scene_layer) lv_obj_add_flag(scene_layer,LV_OBJ_FLAG_HIDDEN);
    if(s->page==8) { ui_show_grid(); return; }
    if(grid_layer) lv_obj_add_flag(grid_layer,LV_OBJ_FLAG_HIDDEN);
    if(s->page==7) {
        lv_obj_remove_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_add_flag(logo_layer,LV_OBJ_FLAG_HIDDEN);
    char heading[64],large[80],body[192],bottom[64];
    static const char *pages[]={"HOME","CLOCK","STOPWATCH","CTS 2009 / SIM","DIAGNOSTICS","SETTINGS","SYSTEM INFO"};
    unsigned page=s->page%7;
    snprintf(heading,sizeof heading,"%s",pages[page]);
    snprintf(bottom,sizeof bottom,"%s | USB: n / p",s->demo?"DEMO":"MANUAL");
    const lv_font_t *font=&lv_font_montserrat_20;
    bool warning=false;
    switch(page) {
    case 0:
        font=&lv_font_montserrat_12;
        snprintf(large,sizeof large,"PicoOS Cadillac");
        snprintf(body,sizeof body,"Clock  Stopwatch\nDashboard  Apps: USB");
        break;
    case 1: {
        time_t epoch=(time_t)(s->utc_epoch+s->timezone_minutes*60);
        struct tm date; gmtime_r(&epoch,&date);
        if(s->clock_set) {
            snprintf(large,sizeof large,"%02d:%02d:%02d",date.tm_hour,date.tm_min,date.tm_sec);
            snprintf(body,sizeof body,"%04d-%02d-%02d UTC%+d\nUSB clock sync",date.tm_year+1900,date.tm_mon+1,date.tm_mday,
                     s->timezone_minutes/60);
        } else { snprintf(large,sizeof large,"--:--:--"); snprintf(body,sizeof body,"Set UTC time over USB\nNo battery-backed RTC"); }
        break;
    }
    case 2: {
        uint64_t cs=s->stopwatch_ms/10;
        snprintf(large,sizeof large,"%02llu:%02llu.%02llu",(unsigned long long)(cs/6000),
                 (unsigned long long)((cs/100)%60),(unsigned long long)(cs%100));
        snprintf(body,sizeof body,"%s\nUSB: start / stop / reset",s->stopwatch_running?"RUNNING":"STOPPED");
        break;
    }
    case 3:
        font=&lv_font_montserrat_12;
        snprintf(large,sizeof large,"%u km/h",s->speed);
        warning=s->coolant>105 || s->battery_mv<12000;
        snprintf(body,sizeof body,"%u RPM  %u C\n%u.%u V  %s",s->rpm,s->coolant,s->battery_mv/1000,(s->battery_mv%1000)/100,
                 warning?"SIM WARN":"SIMULATED");
        break;
    case 4:
        font=&lv_font_montserrat_12;
        snprintf(large,sizeof large,"RTOS free %u KB",s->rtos_free/1024);
        snprintf(body,sizeof body,"LVGL free %u KB\nTasks %s | Uptime %llu s",s->lv_free/1024,
                 s->tasks_healthy?"OK":"WAIT",(unsigned long long)(s->uptime_ms/1000));
        warning=!s->tasks_healthy;
        break;
    case 5:
        font=&lv_font_montserrat_12;
        snprintf(large,sizeof large,"Profile %u  Rot %u",s->display.profile,s->display.rotation);
        snprintf(body,sizeof body,"Offset %d,%d  SPI %lu MHz\nSettings via USB; RAM only",s->display.x_offset,s->display.y_offset,
                 (unsigned long)(s->display.spi_hz/1000000));
        break;
    default:
        font=&lv_font_montserrat_12;
        snprintf(large,sizeof large,"PICO OS  v%s",PICOOS_VERSION);
        snprintf(body,sizeof body,"RP2040 / FreeRTOS\nLVGL / Cadillac");
        break;
    }
    lv_label_set_text(title,heading);
    lv_obj_set_style_text_font(primary,font,0);
    lv_obj_set_style_text_color(primary,warning?lv_color_hex(0xff3648):lv_color_white(),0);
    lv_label_set_text(primary,large); lv_label_set_text(details,body); lv_label_set_text(footer,bottom);
}
void ui_poll(void) { lv_timer_handler(); }
void ui_memory(unsigned *free_bytes,unsigned *max_used) {
    lv_mem_monitor_t m; lv_mem_monitor(&m); *free_bytes=m.free_size; *max_used=m.max_used;
}
unsigned ui_padded_flush_count(void) { return padded_flush_count; }
unsigned ui_rendered_frames(void) { return rendered_frames; }

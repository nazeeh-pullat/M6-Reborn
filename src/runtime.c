#include "runtime.h"
#include "ui.h"
#include "build_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "designer_scene.h"
#ifndef PICOOS_HEADLESS
#define PICOOS_HEADLESS 0
#endif

static SemaphoreHandle_t model_lock;
static TaskHandle_t handles[4];
static ui_snapshot_t model;
static uint64_t beats[3];
static unsigned config_version=1;
static bool request_test;
static bool logging=true;
static ui_scene_t staged_scene;
static bool scene_staging;
static uint64_t clock_base, clock_base_ms;
static uint64_t now_ms(void) { return time_us_64()/1000; }
static void lock(void) { configASSERT(xSemaphoreTake(model_lock,portMAX_DELAY)==pdTRUE); }
static void unlock(void) { xSemaphoreGive(model_lock); }
void picoos_assert(const char *file,int line) {
    printf("FATAL assertion %s:%d; watchdog will reset\n",file,line);
    watchdog_enable(2000,true); while(true) tight_loop_contents();
}
void vApplicationMallocFailedHook(void) { picoos_assert("RTOS malloc failure",0); }
void vApplicationStackOverflowHook(TaskHandle_t t,char *name) {
    (void)t; printf("FATAL stack overflow %s\n",name); picoos_assert("stack overflow",0);
}
static void help(void) {
    puts("Commands: animate | grid | logo | n | p | app 0..10 | demo 0/1 | start | stop | reset");
    puts("time UTC_UNIX_SECONDS | tz MINUTES | profile 0..2 | rotation 0..3 | offset X Y");
    puts("invert 0/1 | bgr 0/1 | spi HZ | test | status | logging 0/1 | help");
    puts("All settings are RAM-only. Clock defaults to build timestamp; no battery-backed RTC.");
    puts("Designer protocol: design caps | begin HEX ANIMATE | rect X Y W H HEX | text X Y FONT HEX TEXT | crest X Y W | commit | cancel | show");
}
static bool scene_bounds(int x,int y,int w,int h) {
    return x>=0 && y>=0 && w>0 && h>0 && w<=120 && h<=80 && x<=120-w && y<=80-h;
}
static void design_command(const char *line) {
    if(!strcmp(line,"design caps")) {
        puts("DESIGN_CAPS version=1 width=120 height=80 max_elements=12"); return;
    }
    unsigned color=0,animate=0,font=0;
    int x=0,y=0,w=0,h=0,consumed=0;
    bool ok=false;
    lock();
    if(sscanf(line,"design begin %x %u",&color,&animate)==2 && color<=0xffffff && animate<=1) {
        memset(&staged_scene,0,sizeof staged_scene);
        staged_scene.background=color; staged_scene.animate=animate; scene_staging=true; ok=true;
    } else if(!strcmp(line,"design cancel")) { scene_staging=false; ok=true; }
    else if(!strcmp(line,"design show")) { model.page=10; model.demo=false; ok=true; }
    else if(!strcmp(line,"design commit") && scene_staging) {
        staged_scene.version=model.scene.version+1;
        model.scene=staged_scene; model.page=10; model.demo=false; scene_staging=false; ok=true;
    } else if(scene_staging && staged_scene.count<UI_SCENE_MAX_ELEMENTS) {
        ui_scene_element_t element={0};
        if(sscanf(line,"design rect %d %d %d %d %x",&x,&y,&w,&h,&color)==5 && color<=0xffffff && scene_bounds(x,y,w,h)) {
            element.type=UI_SCENE_RECT; element.color=color; ok=true;
        } else if(sscanf(line,"design text %d %d %u %x %n",&x,&y,&font,&color,&consumed)==4 && consumed>0 &&
                  color<=0xffffff && (font==10 || font==12 || font==20) && x>=0 && x<120 && y>=0 && y<80 &&
                  strlen(line+consumed)>0 && strlen(line+consumed)<=48) {
            element.type=UI_SCENE_TEXT; element.color=color; element.font_size=font;
            w=120-x; h=80-y;
            snprintf(element.text,sizeof element.text,"%s",line+consumed); ok=true;
        } else if(sscanf(line,"design crest %d %d %d",&x,&y,&w)==3 && w>=8 && w<=112) {
            h=(w*45+56)/112;
            if(scene_bounds(x,y,w,h)) { element.type=UI_SCENE_CREST; ok=true; }
        }
        if(ok) {
            element.x=x; element.y=y; element.w=w; element.h=h;
            staged_scene.elements[staged_scene.count++]=element;
        }
    }
    unlock(); puts(ok?"OK":"ERROR invalid design command, bounds, staging state or element limit");
}
static void command(const char *line) {
    unsigned v; int x,y; unsigned long long epoch;
    if(!strcmp(line,"help") || !strcmp(line,"?")) { help(); return; }
    if(!strncmp(line,"design ",7)) { design_command(line); return; }
    if(!strcmp(line,"status")) {
        lock(); ui_snapshot_t s=model; unlock();
        printf("STATUS uptime=%llu heap=%u min_heap=%u LVGL_free=%u page=%u demo=%u tasks_ok=%u profile=%u\n",
               (unsigned long long)s.uptime_ms,s.rtos_free,s.rtos_min,s.lv_free,s.page,s.demo,s.tasks_healthy,s.display.profile);
        printf("VIEWPORT x=0 y=0 width=%u height=%u unused=BLACK\n",tft_usable_width(),tft_usable_height());
        printf("RENDER padded_flushes=%u frames=%u row_stride=aware\n",ui_padded_flush_count(),ui_rendered_frames());
        printf("DESIGN version=%lu elements=%u animate=%u\n",(unsigned long)s.scene.version,s.scene.count,s.scene.animate);
        if(s.page==9) puts("ANIMATION active=1 kind=crest_float constant_brightness=1 bounds=120x80 refresh_ms=33");
        if(s.page==8) puts("GRID columns=8 rows=4 cell_px=20 visible_columns=6 origin=top-left numbering=row-major");
        printf("CLOCK utc=%llu tz_minutes=%d set=%u\n",(unsigned long long)s.utc_epoch,s.timezone_minutes,s.clock_set);
        printf("STOPWATCH ms=%llu running=%u\n",(unsigned long long)s.stopwatch_ms,s.stopwatch_running);
        printf("SIM rpm=%u speed=%u coolant=%u battery_mv=%u\n",s.rpm,s.speed,s.coolant,s.battery_mv);
        return;
    }
    lock(); bool ok=true;
    if(!strcmp(line,"animate")) { model.page=9; model.demo=false; }
    else if(!strcmp(line,"grid")) { model.page=8; model.demo=false; }
    else if(!strcmp(line,"logo")) { model.page=7; model.demo=false; }
    else if(!strcmp(line,"n")) { model.page=(model.page+1)%11; model.demo=false; }
    else if(!strcmp(line,"p")) { model.page=(model.page+10)%11; model.demo=false; }
    else if(sscanf(line,"app %u",&v)==1 && v<11) { model.page=v; model.demo=false; }
    else if(sscanf(line,"demo %u",&v)==1 && v<2) model.demo=v;
    else if(!strcmp(line,"start")) model.stopwatch_running=true;
    else if(!strcmp(line,"stop")) model.stopwatch_running=false;
    else if(!strcmp(line,"reset")) model.stopwatch_ms=0;
    else if(sscanf(line,"time %llu",&epoch)==1 && epoch>=946684800ULL && epoch<=4102444799ULL) {
        clock_base=epoch; clock_base_ms=now_ms(); model.clock_set=true;
    }
    else if(sscanf(line,"tz %d",&x)==1 && x>=-720 && x<=840) model.timezone_minutes=x;
    else if(sscanf(line,"profile %u",&v)==1 && v<3) { model.display=tft_default_config(v,model.display.rotation); config_version++; }
    else if(sscanf(line,"rotation %u",&v)==1 && v<4) { model.display=tft_default_config(model.display.profile,v); config_version++; }
    else if(sscanf(line,"offset %d %d",&x,&y)==2 && x>=0 && x<=80 && y>=0 && y<=80) {
        model.display.x_offset=x; model.display.y_offset=y; config_version++;
    }
    else if(sscanf(line,"invert %u",&v)==1 && v<2) { model.display.invert=v; config_version++; }
    else if(sscanf(line,"bgr %u",&v)==1 && v<2) { model.display.bgr=v; config_version++; }
    else if(sscanf(line,"spi %u",&v)==1 && v>=100000 && v<=16000000) { model.display.spi_hz=v; config_version++; }
    else if(sscanf(line,"logging %u",&v)==1 && v<2) logging=v;
    else if(!strcmp(line,"test")) request_test=true;
    else ok=false;
    unlock(); puts(ok?"OK":"ERROR invalid command/range; type help");
}
static void ui_task(void *unused) {
    (void)unused; unsigned seen_version; tft_config_t config;
    lock(); config=model.display; seen_version=config_version; unlock();
    if(!PICOOS_HEADLESS) configASSERT(tft_init(&config));
    else puts("HEADLESS RTOS/LVGL test: display GPIO untouched; flushes discarded");
    configASSERT(ui_init());
    if(PICOOS_BOOT_DESIGN) { lock(); ui_snapshot_t initial=model; unlock(); ui_render(&initial); }
    else ui_show_animation();
    uint64_t boot_until=now_ms()+2000,last_render=0;
    while(true) {
        uint64_t now=now_ms(); lock();
        beats[0]=now; ui_snapshot_t snapshot=model;
        bool change=seen_version!=config_version;
        if(change) { seen_version=config_version; config=model.display; }
        bool test=request_test; request_test=false; unlock();
        if(change) { if(!PICOOS_HEADLESS) configASSERT(tft_init(&config)); ui_resize(); }
        if(test) {
            const uint16_t colors[]={0xf800,0x07e0,0x001f};
            for(unsigned i=0;i<3;i++) {
                tft_fill(colors[i]); lock(); beats[0]=now_ms(); unlock();
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            ui_resize(); ui_boot("PICO OS");
        }
        if(now>=boot_until && now-last_render>=250) { ui_render(&snapshot); last_render=now; }
        ui_poll(); unsigned free_bytes,max_used; ui_memory(&free_bytes,&max_used);
        lock(); model.lv_free=free_bytes; model.lv_max_used=max_used; unlock();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
static void app_task(void *unused) {
    (void)unused; char line[128]; unsigned length=0; bool discard=false;
    uint64_t previous=now_ms(),demo_at=previous;
    help();
    while(true) {
        uint64_t now=now_ms(),delta=now-previous; previous=now;
        lock(); beats[1]=now; model.uptime_ms=now;
        model.utc_epoch=clock_base+(now-clock_base_ms)/1000;
        if(model.stopwatch_running) model.stopwatch_ms+=delta;
        if(model.demo && now-demo_at>=8000) { model.page=(model.page+1)%11; demo_at=now; }
        if(!model.demo) demo_at=now;
        unlock();
        for(unsigned i=0;i<32;i++) {
            int c=getchar_timeout_us(0); if(c==PICO_ERROR_TIMEOUT) break;
            if(c=='\r' || c=='\n') {
                if(!discard && length) { line[length]=0; command(line); }
                length=0; discard=false;
            } else if(c>=32 && c<=126 && !discard) {
                if(length<sizeof(line)-1) line[length++]=(char)c;
                else { discard=true; puts("ERROR line too long; discarded"); }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
static void telemetry_task(void *unused) {
    (void)unused;
    while(true) {
        uint64_t now=now_ms(); unsigned phase=(now/250)%240;
        unsigned ramp=phase<=120?phase:240-phase;
        lock(); beats[2]=now; model.speed=ramp; model.rpm=800+ramp*45;
        model.coolant=80+(now/2000)%32; model.battery_mv=13600+(ramp%9)*100; unlock();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
static void monitor_task(void *unused) {
    (void)unused; uint64_t last_log=0; bool led=false;
    gpio_init(PICO_DEFAULT_LED_PIN); gpio_set_dir(PICO_DEFAULT_LED_PIN,GPIO_OUT);
    watchdog_enable(5000,true);
    while(true) {
        uint64_t now=now_ms(); unsigned stacks[4];
        for(unsigned i=0;i<4;i++) stacks[i]=(unsigned)uxTaskGetStackHighWaterMark(handles[i]);
        lock(); bool healthy=true;
        for(unsigned i=0;i<3;i++) if(now>=beats[i] && now-beats[i]>3000) healthy=false;
        model.rtos_free=xPortGetFreeHeapSize(); model.rtos_min=xPortGetMinimumEverFreeHeapSize();
        for(unsigned i=0;i<4;i++) model.stack_min[i]=stacks[i];
        model.tasks_healthy=healthy; ui_snapshot_t s=model; bool verbose=logging; unlock();
        if(healthy) watchdog_update();
        led=!led; gpio_put(PICO_DEFAULT_LED_PIN,led);
        if(now-last_log>=5000) {
            if(verbose) printf("MONITOR uptime=%llu heap=%u min=%u LVGL_free=%u LVGL_peak=%u stack_words(UI,APP,SIM,MON)=%u,%u,%u,%u tasks=%s\n",
                       (unsigned long long)(now/1000),s.rtos_free,s.rtos_min,s.lv_free,s.lv_max_used,
                       stacks[0],stacks[1],stacks[2],stacks[3],healthy?"OK":"STALE");
            last_log=now;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
void picoos_start(void) {
    model_lock=xSemaphoreCreateMutex(); configASSERT(model_lock);
    model.page=9; model.demo=false; model.timezone_minutes=180;
    if(PICOOS_BOOT_DESIGN) { model.scene=designer_boot_scene; model.scene.version=1; model.page=10; }
    model.display=tft_default_config(PICOOS_PROFILE,PICOOS_ROTATION);
    clock_base=PICOOS_BUILD_EPOCH; clock_base_ms=now_ms(); model.clock_set=clock_base!=0;
    for(unsigned i=0;i<3;i++) beats[i]=now_ms();
    char id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES*2+1]; pico_get_unique_board_id_string(id,sizeof id);
    printf("BOARD_ID %s RTOS_heap=%u LVGL_heap=49152 buffers=6400 core0_only\n",id,configTOTAL_HEAP_SIZE);
    const TaskFunction_t fns[]={ui_task,app_task,telemetry_task,monitor_task};
    const char *names[]={"UI","AppLogic","SimTelemetry","SystemMonitor"};
    const unsigned stacks[]={2048,1536,512,1024};
    for(unsigned i=0;i<4;i++) {
        BaseType_t ok=xTaskCreate(fns[i],names[i],stacks[i],NULL,i==0?2:1,&handles[i]);
        printf("TASK %s create=%s stack_words=%u priority=%u\n",names[i],ok==pdPASS?"OK":"FAILED",stacks[i],i==0?2:1);
        configASSERT(ok==pdPASS);
    }
    puts("RTOS starting; watchdog requires fresh UI/AppLogic/SimTelemetry heartbeats");
    vTaskStartScheduler(); picoos_assert("scheduler returned",0);
}

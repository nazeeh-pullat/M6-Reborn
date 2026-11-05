#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "tft.h"
#include "ui.h"
#ifndef PICOOS_PROFILE
#define PICOOS_PROFILE 0
#define PICOOS_ROTATION 0
#endif
static void test_pattern(const tft_config_t *config) {
    if(!tft_init(config)) { puts("ERROR: invalid display config"); return; }
    puts("TEST 1 RED"); tft_fill(0xf800); sleep_ms(800);
    puts("TEST 2 GREEN"); tft_fill(0x07e0); sleep_ms(800);
    puts("TEST 3 BLUE"); tft_fill(0x001f); sleep_ms(800);
    puts("TEST 4 RECTANGLES"); tft_fill(0);
    tft_rect(0,0,40,40,0xf800); tft_rect(40,0,40,40,0x07e0);
    tft_rect(80,0,40,40,0x001f); tft_rect(120,0,40,40,0xffff); sleep_ms(1500);
    ui_resize(); ui_boot("PICO OS"); puts("TEST 5 PICO OS text / boot screen commands sent");
}
int main(void) {
    stdio_init_all(); sleep_ms(1500);
    puts("PICOOS_DISPLAY_TEST: candidate controller; backlight never driven by firmware");
    tft_config_t config=tft_default_config(PICOOS_PROFILE,PICOOS_ROTATION);
    if(!tft_init(&config) || !ui_init()) { puts("FATAL display/LVGL init failed"); while(true) tight_loop_contents(); }
    test_pattern(&config);
    puts("Commands: profile 0..2 | rotation 0..3 | offset X Y | invert 0/1 | bgr 0/1 | spi HZ | test | release");
    char line[96]; unsigned n=0;
    while(true) {
        int c=getchar_timeout_us(0);
        if(c=='\n' || c=='\r') {
            if(n) {
                line[n]=0; n=0; unsigned v; int x,y; bool changed=false;
                if(sscanf(line,"profile %u",&v)==1 && v<3) { config=tft_default_config(v,config.rotation); changed=true; }
                else if(sscanf(line,"rotation %u",&v)==1 && v<4) { config=tft_default_config(config.profile,v); changed=true; }
                else if(sscanf(line,"offset %d %d",&x,&y)==2 && x>=0 && x<=80 && y>=0 && y<=80) { config.x_offset=x; config.y_offset=y; changed=true; }
                else if(sscanf(line,"invert %u",&v)==1 && v<2) { config.invert=v; changed=true; }
                else if(sscanf(line,"bgr %u",&v)==1 && v<2) { config.bgr=v; changed=true; }
                else if(sscanf(line,"spi %u",&v)==1 && v>=100000 && v<=16000000) { config.spi_hz=v; changed=true; }
                else if(!strcmp(line,"test")) changed=true;
                else if(!strcmp(line,"release")) { tft_release(); puts("Display signal pins released"); }
                else puts("ERROR unknown command or invalid range");
                if(changed) test_pattern(&config);
            }
        } else if(c>=32 && c<=126) {
            if(n<sizeof(line)-1) line[n++]=(char)c;
            else { n=0; puts("ERROR input too long"); }
        }
        ui_poll(); sleep_ms(5);
    }
}

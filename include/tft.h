#pragma once
#include <stdbool.h>
#include <stdint.h>
enum { TFT_SCK=18, TFT_MOSI=19, TFT_CS=17, TFT_DC=20, TFT_RST=21 };
typedef struct {
    unsigned profile, rotation;
    int16_t x_offset, y_offset;
    bool invert, bgr;
    uint32_t spi_hz;
} tft_config_t;
tft_config_t tft_default_config(unsigned profile, unsigned rotation);
const char *tft_profile_name(unsigned profile);
bool tft_init(const tft_config_t *config);
void tft_release(void);
unsigned tft_width(void);
unsigned tft_height(void);
unsigned tft_usable_width(void);
unsigned tft_usable_height(void);
void tft_flush(int x1,int y1,int x2,int y2,const uint16_t *pixels);
void tft_flush_strided(int x1,int y1,int x2,int y2,const uint16_t *pixels,unsigned stride_bytes);
bool tft_stride_selftest(void);
void tft_fill(uint16_t color);
void tft_rect(int x,int y,int width,int height,uint16_t color);

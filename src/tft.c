#include "tft.h"
#include "display_area.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

static tft_config_t current;
static unsigned width=160,height=80;
static bool ready=false;
static void command(uint8_t cmd,const uint8_t *data,size_t size) {
    gpio_put(TFT_CS,0); gpio_put(TFT_DC,0);
    spi_write_blocking(spi0,&cmd,1);
    if(size) { gpio_put(TFT_DC,1); spi_write_blocking(spi0,data,size); }
    gpio_put(TFT_CS,1);
}
static void byte_command(uint8_t cmd,uint8_t value) { command(cmd,&value,1); }
const char *tft_profile_name(unsigned id) {
    static const char *names[]={"M6 minimal candidate","ST7735S candidate","Generic ST7735 candidate"};
    return id<3 ? names[id] : "INVALID";
}
tft_config_t tft_default_config(unsigned profile,unsigned rotation) {
    tft_config_t c={.profile=profile,.rotation=rotation,.spi_hz=2000000,
                    .invert=profile!=2,.bgr=profile==1};
    if(profile!=2) { c.x_offset=(rotation&1)?26:1; c.y_offset=(rotation&1)?1:26; }
    return c;
}
void tft_release(void) {
    spi_deinit(spi0);
    const unsigned pins[]={TFT_SCK,TFT_MOSI,TFT_CS,TFT_DC,TFT_RST};
    for(unsigned i=0;i<5;i++) { gpio_init(pins[i]); gpio_set_dir(pins[i],GPIO_IN); gpio_disable_pulls(pins[i]); }
    ready=false;
}
bool tft_init(const tft_config_t *c) {
    if(!tft_stride_selftest()) { puts("ERROR RGB565 stride/mask self-test failed"); return false; }
    if(c->profile>2 || c->rotation>3 || c->spi_hz<100000 || c->spi_hz>16000000 ||
       c->x_offset<0 || c->y_offset<0 || c->x_offset>80 || c->y_offset>80) return false;
    current=*c; width=(c->rotation&1)?80:160; height=(c->rotation&1)?160:80;
    printf("DISPLAY candidate=%s profile=%u rotation=%u offset=%d,%d inversion=%u BGR=%u\n",
           tft_profile_name(c->profile),c->profile,c->rotation,c->x_offset,c->y_offset,c->invert,c->bgr);
    spi_init(spi0,c->spi_hz);
    spi_set_format(spi0,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
    gpio_set_function(TFT_SCK,GPIO_FUNC_SPI); gpio_set_function(TFT_MOSI,GPIO_FUNC_SPI);
    const unsigned pins[]={TFT_CS,TFT_DC,TFT_RST};
    for(unsigned i=0;i<3;i++) { gpio_init(pins[i]); gpio_put(pins[i],1); gpio_set_dir(pins[i],GPIO_OUT); }
    printf("SPI0 SCK=GP18 MOSI=GP19 CS=GP17 DC=GP20 RST=GP21 hz=%lu; backlight untouched\n",(unsigned long)c->spi_hz);
    gpio_put(TFT_RST,0); sleep_ms(20); gpio_put(TFT_RST,1); sleep_ms(120);
    command(0x01,NULL,0); sleep_ms(150); command(0x11,NULL,0); sleep_ms(120);
    if(c->profile==1) {
        const uint8_t frame[]={1,0x2c,0x2d}, frame3[]={1,0x2c,0x2d,1,0x2c,0x2d};
        const uint8_t p0[]={0xa2,2,0x84},p2[]={0x0a,0},p3[]={0x8a,0x2a},p4[]={0x8a,0xee};
        const uint8_t gamma0[]={2,0x1c,7,0x12,0x37,0x32,0x29,0x2d,0x29,0x25,0x2b,0x39,0,1,3,0x10};
        const uint8_t gamma1[]={3,0x1d,7,6,0x2e,0x2c,0x29,0x2d,0x2e,0x2e,0x37,0x3f,0,0,2,0x10};
        command(0xb1,frame,3); command(0xb2,frame,3); command(0xb3,frame3,6);
        byte_command(0xb4,7); command(0xc0,p0,3); byte_command(0xc1,0xc5);
        command(0xc2,p2,2); command(0xc3,p3,2); command(0xc4,p4,2); byte_command(0xc5,0x0e);
        command(0xe0,gamma0,16); command(0xe1,gamma1,16);
    }
    static const uint8_t modes[]={0x60,0x80,0xa0,0x40};
    byte_command(0x36,modes[c->rotation] | (c->bgr?8:0));
    byte_command(0x3a,5); command(c->invert?0x21:0x20,NULL,0);
    command(0x13,NULL,0); command(0x29,NULL,0); sleep_ms(100);
    ready=true;
    tft_fill(0); /* Clear the complete panel, including unused/damaged columns. */
    puts("DISPLAY init commands sent; visible output NOT VERIFIED");
    return true;
}
unsigned tft_width(void) { return width; }
unsigned tft_height(void) { return height; }
unsigned tft_usable_width(void) { return width<PICOOS_USABLE_WIDTH?width:PICOOS_USABLE_WIDTH; }
unsigned tft_usable_height(void) { return height<PICOOS_USABLE_HEIGHT?height:PICOOS_USABLE_HEIGHT; }
static void window(int x1,int y1,int x2,int y2) {
    x1+=current.x_offset; x2+=current.x_offset; y1+=current.y_offset; y2+=current.y_offset;
    uint8_t a[]={x1>>8,x1,x2>>8,x2},b[]={y1>>8,y1,y2>>8,y2};
    command(0x2a,a,4); command(0x2b,b,4); command(0x2c,NULL,0);
}
static void encode_row(uint8_t *bytes,const uint16_t *row,unsigned x,unsigned y,unsigned count) {
    const unsigned usable_width=tft_usable_width(),usable_height=tft_usable_height();
    for(unsigned i=0;i<count;i++) {
        uint16_t value=(x+i<usable_width && y<usable_height)?row[i]:0;
        bytes[2*i]=value>>8; bytes[2*i+1]=value&255;
    }
}
bool tft_stride_selftest(void) {
    /* Two 3-pixel rows padded to 8 bytes. Padding must never become a pixel. */
    const uint16_t source[]={0xf800,0x07e0,0x001f,0xdead,0xffff,0x0000,0x8410,0xbeef};
    const uint8_t expected[]={0xf8,0,0x07,0xe0,0,0x1f,0xff,0xff,0,0,0x84,0x10};
    uint8_t result[12];
    for(unsigned row=0;row<2;row++) {
        const uint16_t *pixels=(const uint16_t *)((const uint8_t *)source+row*8);
        encode_row(result+row*6,pixels,0,row,3);
    }
    for(unsigned i=0;i<12;i++) if(result[i]!=expected[i]) return false;
    uint8_t edge[6]; encode_row(edge,source,tft_usable_width()-1,0,3);
    return edge[0]==0xf8 && edge[1]==0 && edge[2]==0 && edge[3]==0 && edge[4]==0 && edge[5]==0;
}
void tft_flush_strided(int x1,int y1,int x2,int y2,const uint16_t *pixels,unsigned stride_bytes) {
    if(!ready || x1<0 || y1<0 || x2>=(int)width || y2>=(int)height || x2<x1 || y2<y1) return;
    unsigned row_pixels=(unsigned)(x2-x1+1);
    if(stride_bytes<row_pixels*2 || stride_bytes%2) return;
    window(x1,y1,x2,y2);
    gpio_put(TFT_CS,0); gpio_put(TFT_DC,1);
    uint8_t bytes[320];
    for(int y=y1;y<=y2;y++) {
        const uint16_t *row=(const uint16_t *)((const uint8_t *)pixels+(y-y1)*stride_bytes);
        encode_row(bytes,row,(unsigned)x1,(unsigned)y,row_pixels);
        spi_write_blocking(spi0,bytes,row_pixels*2);
    }
    gpio_put(TFT_CS,1);
}
void tft_flush(int x1,int y1,int x2,int y2,const uint16_t *pixels) {
    if(x2<x1) return;
    tft_flush_strided(x1,y1,x2,y2,pixels,(unsigned)(x2-x1+1)*2);
}
void tft_rect(int x,int y,int w,int h,uint16_t color) {
    if(x<0 || y<0 || w<1 || h<1 || x+w>(int)width || y+h>(int)height) return;
    uint16_t line[160]; for(int i=0;i<w;i++) line[i]=color;
    for(int row=0;row<h;row++) tft_flush(x,y+row,x+w-1,y+row,line);
}
void tft_fill(uint16_t color) { tft_rect(0,0,width,height,color); }

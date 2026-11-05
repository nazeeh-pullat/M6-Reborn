#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "build_info.h"

int main(void) {
    stdio_init_all();
    // Only onboard GP25 is configured. Display/backlight pins remain untouched.
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN,GPIO_OUT);
    watchdog_enable(5000,true);
    unsigned long count=0;
    bool on=false;
    while(true) {
        on=!on;
        gpio_put(PICO_DEFAULT_LED_PIN,on);
        printf("PICOOS_SMOKE v%s RP2040 LED=%u heartbeat=%lu uptime_ms=%llu DISPLAY_GPIO_UNTOUCHED\n",
               PICOOS_VERSION,on,count++,(unsigned long long)(time_us_64()/1000));
        int c=getchar_timeout_us(0);
        if(c=='?') puts("SMOKE_OK board=pico chip=RP2040 display=NOT_INITIALIZED");
        watchdog_update();
        sleep_ms(500);
    }
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "runtime.h"
#include "build_info.h"
int main(void) {
    stdio_init_all(); sleep_ms(1500);
    printf("BOOT M6 Reborn %s board=pico chip=RP2040 watchdog_reset=%u\n",PICOOS_VERSION,watchdog_caused_reboot());
    puts(PICOOS_BUILD_DESCRIPTION);
    puts("No vehicle interface. Telemetry is simulated. Backlight is external hardware only.");
    picoos_start();
    while(true) tight_loop_contents();
}

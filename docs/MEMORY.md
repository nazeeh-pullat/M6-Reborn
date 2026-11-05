# Memory and rendering

Current exact linker sizes are printed by scripts/Build.ps1 and recorded locally in logs/*-memory.txt. The full prototype uses about 249 KB flash, 133 KB main RAM and 2 KB scratch X, comfortably within the RP2040 Pico's 2 MB flash / 264 KB SRAM.

FreeRTOS heap is 64 KB; LVGL heap is 48 KB. The UI owns two 3200-byte partial buffers (160×10 RGB565 pixels each), not a full-screen double framebuffer. The 112×45 RGB565 example asset is a flash-resident constant of 10080 pixel bytes. Up to 12 custom elements are held in fixed-size staging/model structures.

Monitor observations during live design/animation reported about 40 KB free RTOS heap and 43 KB free LVGL heap with healthy tasks. These are short runtime observations, not guaranteed high-water marks for every possible custom scene. Monitor logs report task stack minima and heap statistics; allocation/stack faults trigger watchdog recovery.

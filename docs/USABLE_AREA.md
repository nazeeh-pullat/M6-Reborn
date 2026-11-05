# Working display area

The controller layout is 160×80 at rotation 0, offsets (1,26). A diagnostic 8×4 grid uses 20×20 cells numbered left-to-right, top-to-bottom. Operator-visible cells were 1–6, 9–14, 17–22 and 25–30.

These are the first six columns of every row, giving usable x=0..119 and y=0..79. The remaining x=120..159 pixels are forced RGB565 black in the transfer path. The logical UI/scene container is 120×80; the physical panel window stays 160×80. See config/display_area.h.

For cell n: column=(n-1)%8, row=(n-1)/8 using integer division. The cell spans 20*column..20*column+19 and 20*row..20*row+19. The example crest is 112×45, horizontally centered with four-pixel margins.

Grid selects page 8; static crest page 7; moving crest page 9; custom designer page 10. The driver honors LVGL padded row strides, avoiding slanted/zigzag partial redraws. Other orientations are not a new measurement of physical damage and may require a different layout.

"""Convert Cadillac's published crest to a flash-resident RGB565 LVGL asset."""
from pathlib import Path
import hashlib
import json
from PIL import Image, ImageDraw

root=Path(__file__).resolve().parent.parent
source=root/'assets/cadillac-source.tif'
image=Image.open(source).convert('RGB')
# Source is 1200x1000: crest above the script wordmark on a white field.
# Keep the crest/chrome and remove the exterior white field with its outline.
mask=Image.new('L',image.size,0)
ImageDraw.Draw(mask).polygon([(136,193),(1076,193),(1020,256),(1006,410),
                              (992,460),(606,558),(238,462),(218,435),(197,262)],fill=255)
crest=Image.composite(image,Image.new('RGB',image.size,'black'),mask)
crest=crest.crop((125,182,1088,568)).resize((112,45),Image.Resampling.LANCZOS)
crest.save(root/'assets/cadillac-crest.png')
values=[((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b in crest.getdata()]
lines=['#include "lvgl.h"','#include <stdint.h>',
       '/* Cadillac media crest, RGB565, stored in flash; see assets/SOURCE.md. */',
       'static const uint16_t cadillac_crest_pixels[112*45] = {']
for start in range(0,len(values),16):
    lines.append('    '+','.join(f'0x{v:04x}' for v in values[start:start+16])+',')
lines.extend(['};','const lv_image_dsc_t cadillac_crest_image = {',
              '    .header = {.magic=LV_IMAGE_HEADER_MAGIC, .cf=LV_COLOR_FORMAT_RGB565,',
              '               .flags=0, .w=112, .h=45, .stride=224},',
              '    .data_size=sizeof(cadillac_crest_pixels),',
              '    .data=(const uint8_t *)cadillac_crest_pixels,',
              '};',''])
(root/'src/cadillac_logo.c').write_text('\n'.join(lines))
metadata={'source_url':'https://media.cadillac.com/content/dam/Media/images/Shared/Logos/Cadillac/logo-cadillac.1200x1000.tif',
          'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
          'width':112,'height':45,'pixel_format':'RGB565','flash_pixel_bytes':len(values)*2,
          'runtime_asset_heap_bytes':0}
(root/'assets/cadillac-crest.json').write_text(json.dumps(metadata,indent=2))
print(json.dumps(metadata,indent=2))

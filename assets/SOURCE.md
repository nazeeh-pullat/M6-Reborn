# Cadillac crest artwork

Source: Cadillac's published media gallery:
https://media.cadillac.com/Pages/galleries/intl/en/cadillac/logos/logo.html

Original downloaded asset:
https://media.cadillac.com/content/dam/Media/images/Shared/Logos/Cadillac/logo-cadillac.1200x1000.tif

Cadillac/GM artwork is retained as the requested brand logo. No new ownership or license is claimed. `scripts/Make-Cadillac-Asset.py` crops the crest, masks the exterior white field, scales it to112x45 for the confirmed usable area, and emits `src/cadillac_logo.c` as constant RGB565 data in flash. Pillow was already installed; it is needed only to regenerate this asset, not to build the checked-in C source.


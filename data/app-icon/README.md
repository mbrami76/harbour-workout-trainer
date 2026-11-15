# Application icon

`app-icon.svg` is the launcher icon of the app, drawn by the project author in Inkscape and released under CC0 1.0 with the rest of the project. The PNG files in `app/icons/<size>x<size>/` (86, 108, 128 and 172 pixels square) are rendered from it:

```
rsvg-convert -w 86 -h 86 app-icon.svg -o app/icons/86x86/<package>.png
```

`app-icon-mono.svg` is a single-colour silhouette of the icon on a transparent background, also CC0 1.0: the drop shape is solid, the ring, the figure and the stripes are cut out. `app-icon-mono.png` (512 x 512 pixels) is rendered from it and installed with the app; the cover tints it with the primary colour of the ambience at low opacity (R-UI-17):

```
inkscape app-icon-mono.svg -w 512 -h 512 -o app-icon-mono.png
```

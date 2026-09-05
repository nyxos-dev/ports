# sdl

## Simple DirectMedia Library 

Minimal framebuffer graphics: pixel plot and fill-rect, rendered to a PPM image.

sdl WxH CMD...

## Functions

- **put_pixel(x, y, color)** — sets a single pixel to `color`. Out-of-bounds coordinates are silently ignored.
- **fill_rect(x, y, w, h, color)** — fills a `w` by `h` rectangle starting at `(x, y)` with `color`, one pixel at a time via `put_pixel`.
- **write_ppm(out)** — writes the current canvas to `out` as a binary PPM (P6) image.

## CLI commands

Each argument after `WxH` maps to one of the two drawing functions:

- `px X,Y,RRGGBB` → `put_pixel`
- `rect X,Y,W,H,RRGGBB` → `fill_rect`

Commands run in order onto a black canvas, then the result is written to stdout as a PPM.

## Example

sdl 64x32 rect 0,0,64,32,000000 px 10,10,ff0000 > out.ppm

Draws a black canvas, then one red pixel, and saves it as `out.ppm`.
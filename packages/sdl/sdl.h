/* sdl.h — minimal framebuffer graphics: pixel plot and fill-rect.
 *
 * Declarations for the sdl library. See sdl.c for the CLI tool that
 * uses these to build a PPM image from argv commands.
 */

#ifndef SDL_H
#define SDL_H

#include <stdint.h>
#include <stdio.h>

void canvas_create(int width, int height);
void canvas_free(void);

void put_pixel(int x, int y, uint32_t color);
void fill_rect(int x, int y, int w, int h, uint32_t color);

void write_ppm(FILE *out);

#endif /* SDL_H */
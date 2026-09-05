/* sdl — minimal framebuffer graphics: pixel plot and fill-rect, rendered to a PPM image.
 *
 *   sdl WxH CMD...
 *
 * WxH sets the canvas size, e.g. 64x32. Each CMD is one of:
 *
 *   px X,Y,RRGGBB          plot a single pixel
 *   rect X,Y,W,H,RRGGBB    fill a rectangle
 *
 * Commands are applied in order onto a black canvas, then the result is written
 * to stdout as a binary PPM (P6) image. Pure argv + stdout, deterministic, built
 * INSIDE the OS by `cc` — installs via `xbm install sdl`.
 *
 * Example:
 *   sdl 64x32 rect 0,0,64,32,000000 px 10,10,ff0000 > out.ppm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int canvas_w;
static int canvas_h;
static uint32_t *canvas;

static void put_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 || x >= canvas_w || y >= canvas_h)
        return;

    canvas[y * canvas_w + x] = color;
}

static void fill_rect(int x, int y, int w, int h, uint32_t color)
{
    int row, col;

    for (row = y; row < y + h; row++)
    {
        for (col = x; col < x + w; col++)
        {
            put_pixel(col, row, color);
        }
    }
}

static void write_ppm(FILE *out)
{
    int i;

    fprintf(out, "P6\n%d %d\n255\n", canvas_w, canvas_h);

    for (i = 0; i < canvas_w * canvas_h; i++)
    {
        unsigned char rgb[3];
        rgb[0] = (canvas[i] >> 16) & 0xff;
        rgb[1] = (canvas[i] >> 8) & 0xff;
        rgb[2] = canvas[i] & 0xff;
        fwrite(rgb, 1, 3, out);
    }
}

static int parse_size(const char *arg)
{
    if (sscanf(arg, "%dx%d", &canvas_w, &canvas_h) != 2)
        return -1;

    if (canvas_w <= 0 || canvas_h <= 0)
        return -1;

    return 0;
}

static int run_command(const char *arg)
{
    int x, y, w, h;
    unsigned int color;

    if (sscanf(arg, "%d,%d,%x", &x, &y, &color) == 3)
    {
        put_pixel(x, y, color);
        return 0;
    }

    if (sscanf(arg, "%d,%d,%d,%d,%x", &x, &y, &w, &h, &color) == 5)
    {
        fill_rect(x, y, w, h, color);
        return 0;
    }

    return -1;
}

int main(int argc, char **argv)
{
    int i;

    if (argc < 2)
    {
        fprintf(stderr, "usage: sdl WxH CMD...\n");
        return 1;
    }

    if (parse_size(argv[1]) != 0)
    {
        fprintf(stderr, "sdl: bad size '%s', expected WxH\n", argv[1]);
        return 1;
    }

    canvas = calloc((size_t)canvas_w * (size_t)canvas_h, sizeof(uint32_t));
    if (canvas == NULL)
    {
        fprintf(stderr, "sdl: out of memory\n");
        return 1;
    }

    for (i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "px") == 0 || strcmp(argv[i], "rect") == 0)
            continue; /* keyword is just a label for readability */

        if (run_command(argv[i]) != 0)
        {
            fprintf(stderr, "sdl: bad command '%s'\n", argv[i]);
            free(canvas);
            return 1;
        }
    }

    write_ppm(stdout);
    free(canvas);
    return 0;
}
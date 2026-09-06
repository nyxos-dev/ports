#include "libc.h"

/* column -t — read whitespace-separated rows from a file (or stdin) and print them as an
 * aligned table: every column is padded to its widest field and columns are separated by
 * two spaces. Matches util-linux `column -t`: fields split on runs of spaces/tabs, the
 * LAST column is never padded, a short line still pads through the intervening columns
 * (so it can carry trailing spaces), and blank lines produce no row. Table mode only;
 * `-t` is accepted and is the default. Reads up to 64 KiB of input. */

#define MAXBUF 65536
#define MAXCOLS 512

static char buf[MAXBUF];
static int  colw[MAXCOLS];
static const char* tk[MAXCOLS];
static int  tl[MAXCOLS];

/* Next whitespace-delimited token in [*pp, end): returns 1 and sets *tok/*len, advancing
 * *pp past it; 0 when the line has no more tokens. */
static int next_tok(const char** pp, const char* end, const char** tok, int* len) {
    const char* s = *pp;
    while (s < end && (*s == ' ' || *s == '\t')) s++;
    if (s >= end) { *pp = s; return 0; }
    const char* t = s;
    while (s < end && *s != ' ' && *s != '\t') s++;
    *tok = t; *len = (int)(s - t); *pp = s;
    return 1;
}

static void emit(const char* s, int len) { for (int i = 0; i < len; i++) putchar(s[i]); }

int main(int argc, char** argv) {
    const char* file = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) continue;       /* table mode: our only + default mode */
        if (argv[i][0] == '-' && argv[i][1]) continue;  /* ignore other flags for now */
        file = argv[i];
    }
    FILE* f = stdin;
    if (file) {
        f = fopen(file, "r");
        if (!f) { fprintf(stderr, "column: cannot open %s\n", file); return 1; }
    }
    int n = (int)fread(buf, 1, MAXBUF - 1, f);
    if (n < 0) n = 0;
    buf[n] = '\0';
    if (file) fclose(f);

    const char* end = buf + n;

    /* pass 1: per-column max width + the max column count */
    int maxcols = 0;
    const char* p = buf;
    while (p < end) {
        const char* le = p;
        while (le < end && *le != '\n') le++;           /* line = [p, le) */
        const char* lp = p; const char* tok; int len; int c = 0;
        while (next_tok(&lp, le, &tok, &len)) {
            if (c < MAXCOLS && len > colw[c]) colw[c] = len;
            c++;
        }
        if (c > maxcols) maxcols = c;
        p = (le < end) ? le + 1 : le;
    }
    if (maxcols > MAXCOLS) maxcols = MAXCOLS;

    /* pass 2: emit each row aligned (a blank line yields no row) */
    p = buf;
    while (p < end) {
        const char* le = p;
        while (le < end && *le != '\n') le++;
        const char* lp = p; const char* tok; int len; int c = 0;
        while (next_tok(&lp, le, &tok, &len)) {
            if (c < MAXCOLS) { tk[c] = tok; tl[c] = len; }
            c++;
        }
        if (c > MAXCOLS) c = MAXCOLS;
        if (c > 0) {
            for (int j = 0; j < maxcols; j++) {
                int present = (j < c);
                int flen = present ? tl[j] : 0;
                if (j == maxcols - 1) {
                    if (present) emit(tk[j], flen);         /* last column: no padding */
                } else {
                    if (present) emit(tk[j], flen);
                    for (int s = flen; s < colw[j]; s++) putchar(' ');   /* pad to column width */
                    putchar(' '); putchar(' ');                          /* 2-space gap */
                }
            }
            putchar('\n');
        }
        p = (le < end) ? le + 1 : le;
    }
    return 0;
}

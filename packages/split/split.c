#include "libc.h"

/* split — break FILE (or stdin when FILE is "-"/omitted) into pieces. `-l N` puts N lines per
 * piece (the default, N=1000); `-b N[K|M|G]` puts N bytes per piece (K/M/G are 1024-based).
 * Pieces are written to PREFIX + a two-letter suffix aa, ab, …, zz (PREFIX defaults to "x").
 * Matches GNU split for these modes. Options come before the operands (POSIX getopt order).
 * A trailing partial line stays with its piece; empty input creates no pieces; more than
 * 26*26 pieces is an error (as GNU does without -a). */

static const char SUF[] = "abcdefghijklmnopqrstuvwxyz";

/* i-th two-letter suffix: 0->aa, 1->ab, …, 25->az, 26->ba, …, 675->zz. */
static void suffix2(int i, char* out) {
    out[0] = SUF[(i / 26) % 26];
    out[1] = SUF[i % 26];
    out[2] = '\0';
}

/* Parse a non-negative count with an optional K/M/G suffix (1024-based, like GNU -b); -1 on error. */
static long parse_size(const char* s) {
    if (!s || !*s) return -1;
    long v = 0; const char* p = s;
    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
    if (p == s) return -1;
    long mul = 1;
    if      (*p == 'k' || *p == 'K') mul = 1024L;
    else if (*p == 'm' || *p == 'M') mul = 1024L * 1024;
    else if (*p == 'g' || *p == 'G') mul = 1024L * 1024 * 1024;
    else if (*p) return -1;                 /* unknown suffix */
    if (*p && p[1]) return -1;              /* junk after the unit letter */
    return v * mul;
}

int main(int argc, char** argv) {
    long lines = 1000, bytes = 0;          /* bytes > 0 selects byte mode, else line mode */
    int o;
    while ((o = getopt(argc, argv, "l:b:")) != -1) {
        if (o == 'l')      { lines = parse_size(optarg); if (lines <= 0) { fprintf(stderr, "split: invalid -l\n"); return 1; } bytes = 0; }
        else if (o == 'b') { bytes = parse_size(optarg); if (bytes <= 0) { fprintf(stderr, "split: invalid -b\n"); return 1; } }
        else return 1;                     /* getopt already reported the bad option */
    }

    const char* path   = (optind < argc) ? argv[optind++] : "-";
    const char* prefix = (optind < argc) ? argv[optind++] : "x";

    FILE* in = strcmp(path, "-") == 0 ? stdin : fopen(path, "rb");
    if (!in) { fprintf(stderr, "split: cannot open %s\n", path); return 1; }

    int idx = 0;                           /* current piece index */
    FILE* out = 0;
    long count = 0;                        /* lines or bytes in the current piece */
    char name[256], sfx[4];
    int rc = 0, c;

    while ((c = fgetc(in)) != EOF) {
        if (!out) {                        /* need a new piece */
            if (idx >= 26 * 26) { fprintf(stderr, "split: output file suffixes exhausted\n"); rc = 1; break; }
            suffix2(idx++, sfx);
            snprintf(name, sizeof name, "%s%s", prefix, sfx);
            out = fopen(name, "wb");
            if (!out) { fprintf(stderr, "split: cannot write %s\n", name); rc = 1; break; }
            count = 0;
        }
        fputc(c, out);
        if (bytes > 0) {                   /* byte mode: close after `bytes` bytes */
            if (++count >= bytes) { fclose(out); out = 0; }
        } else if (c == '\n') {            /* line mode: close after `lines` newlines */
            if (++count >= lines) { fclose(out); out = 0; }
        }
    }

    if (out) fclose(out);
    if (in != stdin) fclose(in);
    return rc;
}

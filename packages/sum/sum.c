#include "libc.h"
#include "syscall.h"

/* sum — the classic BSD (default, -r) and System V (-s) file checksums.
 *
 *   sum [-r|-s] [file...]
 *
 * BSD (default, -r): a 16-bit checksum that rotates right by one bit before adding each
 * byte; prints a 5-digit zero-padded checksum then the number of 1024-byte blocks (rounded
 * up).  System V (-s): adds the bytes, folds the carries into 16 bits twice; prints the
 * checksum then the number of 512-byte blocks.  With no file (or "-") it reads standard
 * input and omits the name column; with named files it appends the filename, matching GNU
 * sum byte-for-byte.  Standard algorithm, no upstream code; installs via `xbm install sum`.
 */

typedef struct {
    int sysv;             /* 0 = BSD (rotate), 1 = System V (fold) */
    unsigned int bsd;     /* running 16-bit BSD checksum */
    unsigned int sv;      /* running System V byte sum */
    unsigned long total;  /* bytes seen */
} sum_ctx;

static void sum_init(sum_ctx* c, int sysv) { c->sysv = sysv; c->bsd = 0; c->sv = 0; c->total = 0; }

static void sum_update(sum_ctx* c, const unsigned char* d, unsigned long n) {
    c->total += n;
    if (c->sysv) {
        for (unsigned long i = 0; i < n; i++) c->sv += d[i];
    } else {
        unsigned int s = c->bsd;
        for (unsigned long i = 0; i < n; i++) {
            s = (s >> 1) + ((s & 1) << 15);   /* rotate right one bit within 16 bits */
            s = (s + d[i]) & 0xffff;
        }
        c->bsd = s;
    }
}

static void sum_final(const sum_ctx* c, unsigned int* ck, unsigned long* blocks) {
    if (c->sysv) {
        unsigned int r = (c->sv & 0xffff) + (c->sv >> 16);   /* fold twice into 16 bits */
        *ck = (r & 0xffff) + (r >> 16);
        *blocks = (c->total + 511) / 512;
    } else {
        *ck = c->bsd;
        *blocks = (c->total + 1023) / 1024;
    }
}
/* ---- end pure core ---- */

static void sum_print(int sysv, unsigned int ck, unsigned long blocks, const char* name) {
    if (sysv) {
        if (name) printf("%d %d %s\n", ck, (unsigned int)blocks, name);
        else      printf("%d %d\n", ck, (unsigned int)blocks);
    } else {
        if (name) printf("%05d %5d %s\n", ck, (unsigned int)blocks, name);
        else      printf("%05d %5d\n", ck, (unsigned int)blocks);
    }
}

static void sum_file(FILE* f, int sysv, const char* name) {
    sum_ctx c; sum_init(&c, sysv);
    unsigned char buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) sum_update(&c, buf, (unsigned long)n);
    unsigned int ck; unsigned long blocks; sum_final(&c, &ck, &blocks);
    sum_print(sysv, ck, blocks, name);
}

int main(int argc, char** argv) {
    int sysv = 0, i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
        const char* p = argv[i] + 1; int ok = 1;
        for (; *p; p++) { if (*p == 's') sysv = 1; else if (*p == 'r') sysv = 0; else { ok = 0; break; } }
        if (!ok) break;
        i++;
    }
    if (i >= argc) { sum_file(stdin, sysv, 0); return 0; }
    int rc = 0;
    for (; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '\0') { sum_file(stdin, sysv, 0); continue; }
        FILE* f = fopen(argv[i], "rb");
        if (!f) { fprintf(stderr, "sum: %s: cannot open\n", argv[i]); rc = 1; continue; }
        sum_file(f, sysv, argv[i]);
        fclose(f);
    }
    return rc;
}

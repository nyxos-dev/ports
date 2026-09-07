#include "libc.h"

/* numfmt — convert between plain integers and human-readable SI (k/M/G/…, base 1000) or
 * IEC (K/M/G/…, base 1024) forms, matching GNU/uutils `numfmt`. `--to=si`/`--to=iec`
 * formats an integer (1500 -> 1.5k); `--from=si`/`--from=iec` parses a human number back to
 * an integer (1.5K -> 1500 / 1536). With both, it parses then reformats; with neither the
 * number is reprinted. Reads non-negative values from the operands, or one per line from
 * stdin when there are none. Rounds toward +inf (GNU's "from-zero"): formatting shows one
 * decimal below 10 of a unit else an integer and rescales at the divisor (999999 -> 1.0M);
 * parsing takes the ceiling of mantissa*unit (1.71K -> 1752). Integer-only (ring-3 has no %f)
 * and the mathematically-exact ceiling throughout — so it matches GNU/uutils on the vast
 * majority of inputs and stays exact on the minority where their f64 rounding drifts (a
 * suffixless decimal is reprinted verbatim rather than float-rounded). */

static const char* const U_SI[]  = { "", "k", "M", "G", "T", "P", "E" };
static const char* const U_IEC[] = { "", "K", "M", "G", "T", "P", "E" };

/* Format non-negative v with divisor `div` and unit table `units` into out[cap]. */
static void fmt_human(long v, long div, const char* const* units, char* out, int cap) {
    if (v < div) { snprintf(out, cap, "%ld", v); return; }   /* below the first unit: as-is */
    int u = 0; long scale = 1;
    while (u < 6 && v >= scale * div) { scale *= div; u++; }  /* scale = div^u, v in [scale, scale*div) */
    long q = v / scale, r = v % scale;
    long tenths = q * 10 + (r * 10 + scale - 1) / scale;      /* ceil(v*10/scale), no v*10 overflow */
    if (tenths >= 100) {                                     /* mantissa >= 10.0 -> integer form */
        long ival = q + (r ? 1 : 0);                         /* ceil(v/scale) */
        if (ival >= div && u < 6) snprintf(out, cap, "1.0%s", units[u + 1]);   /* rounded up a whole unit */
        else                      snprintf(out, cap, "%ld%s", ival, units[u]);
    } else {                                                 /* mantissa < 10 -> one decimal */
        snprintf(out, cap, "%ld.%ld%s", tenths / 10, tenths % 10, units[u]);
    }
}

/* Unit-suffix letter -> power (k/m/g/t/p/e, case-insensitive); -1 if not a unit letter. */
static int suffix_power(char c) {
    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    switch (c) {
        case 'k': return 1; case 'm': return 2; case 'g': return 3;
        case 't': return 4; case 'p': return 5; case 'e': return 6;
        default:  return -1;
    }
}

/* Parse a plain non-negative decimal integer (all-digits); -1 on empty/non-digit. */
static long parse_num(const char* s) {
    if (!s || !*s) return -1;
    long v = 0;
    for (const char* p = s; *p; p++) {
        if (*p < '0' || *p > '9') return -1;
        v = v * 10 + (*p - '0');
    }
    return v;
}

static void emit(long v, int to_mode) {
    char out[48];
    if (to_mode == 1)      fmt_human(v, 1000, U_SI,  out, sizeof out);
    else if (to_mode == 2) fmt_human(v, 1024, U_IEC, out, sizeof out);
    else                   snprintf(out, sizeof out, "%ld", v);
    printf("%s\n", out);
}

/* --from: parse "[digits][.digits][suffix]" with divisor `div`. A number with NO suffix is
 * already in base units, so it is reprinted verbatim (matching GNU — no scaling, no rounding);
 * with a suffix the value = ceil(mantissa * unit / 10^frac) is emitted (reformatted if to_mode).
 * Malformed tokens (no digits, unknown suffix, trailing junk) are skipped. */
static void do_from(const char* tok, long div, int to_mode) {
    const char* p = tok;
    unsigned long long mant = 0, denom = 1; int any = 0;
    while (*p >= '0' && *p <= '9') { mant = mant * 10 + (unsigned)(*p - '0'); p++; any = 1; }
    if (*p == '.') { p++; while (*p >= '0' && *p <= '9') { mant = mant * 10 + (unsigned)(*p - '0'); denom *= 10; p++; any = 1; } }
    if (!any) return;
    if (*p == '\0') { printf("%s\n", tok); return; }         /* no suffix: reprint verbatim */
    int pw = suffix_power(*p);
    if (pw < 0) return;
    p++;
    if (*p) return;                                          /* junk after the suffix (e.g. the 'i' in "Ki") */
    unsigned long long unit = 1;
    for (int i = 0; i < pw; i++) unit *= (unsigned long long)div;
    /* ceil(mant*unit/denom), split into integer + fractional parts so mant*unit can't overflow
     * within the exact range (each term stays <= the result). */
    unsigned long long ip = mant / denom, fp = mant % denom;
    unsigned long long val = ip * unit + (fp * unit + denom - 1) / denom;
    emit((long)val, to_mode);
}

int main(int argc, char** argv) {
    int to_mode = 0, from_mode = 0;
    long from_div = 1000;
    int nops = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--to=si")   == 0) { to_mode = 1; continue; }
        if (strcmp(argv[i], "--to=iec")  == 0) { to_mode = 2; continue; }
        if (strcmp(argv[i], "--from=si") == 0) { from_mode = 1; from_div = 1000; continue; }
        if (strcmp(argv[i], "--from=iec")== 0) { from_mode = 2; from_div = 1024; continue; }
        if (argv[i][0] == '-' && argv[i][1]) continue;       /* ignore other flags */
        nops++;
        if (from_mode) do_from(argv[i], from_div, to_mode);
        else { long v = parse_num(argv[i]); if (v >= 0) emit(v, to_mode); }
    }
    if (nops == 0) {                                         /* no operands: read stdin, one number per line */
        char line[64];
        while (fgets(line, sizeof line, stdin)) {
            int n = 0; while (line[n] && line[n] != '\n' && line[n] != '\r') n++;
            line[n] = '\0';
            if (line[0] == '\0') continue;
            if (from_mode) do_from(line, from_div, to_mode);
            else { long v = parse_num(line); if (v >= 0) emit(v, to_mode); }
        }
    }
    return 0;
}

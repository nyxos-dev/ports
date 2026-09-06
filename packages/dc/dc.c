#include "libc.h"

/* dc — the classic reverse-Polish (RPN) desk calculator.
 *
 *   dc <program...>     e.g.  dc "3 4 + 5 * p"   ->  35
 *
 * A stack machine: digits push a number; an operator pops the top two (b then a, computed b OP a)
 * and pushes the result; a command prints or reshuffles the stack. Integer arithmetic (64-bit),
 * the GNU dc idiom of a leading '_' for a negative literal, and the everyday command set:
 *   +  -  *  /  %      arithmetic (/, % truncate toward zero; divide-by-zero is skipped)
 *   p                  print the top of the stack (a newline after), leaving it
 *   n                  print the top WITHOUT a newline, and pop it
 *   f                  print the whole stack, top first, one per line
 *   d  r  c            duplicate top / swap top two / clear the stack
 * Whitespace separates tokens; any other character is ignored (as dc does after a warning).
 * Not the full dc (no arbitrary precision, registers or macros) — the RPN core people reach for.
 * Standard semantics, no upstream code; installs via `xbm install dc`.
 */

#define DC_STACK 256

/* Append the decimal form of a signed 64-bit value to out[*o] (bounded by cap), no printf %lld. */
static void dc_emit_num(char* out, int* o, int cap, long long v) {
    char t[24]; int n = 0;
    unsigned long long u = (v < 0) ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;  /* |v| without INT64_MIN overflow */
    if (v < 0 && *o < cap - 1) out[(*o)++] = '-';
    if (u == 0) t[n++] = '0';
    while (u) { t[n++] = (char)('0' + (int)(u % 10)); u /= 10; }
    for (int i = n - 1; i >= 0; i--) if (*o < cap - 1) out[(*o)++] = t[i];
}

/* Pure RPN evaluator: run program s, writing all printed output into out (NUL-terminated). */
void dc_run(const char* s, char* out, int cap) {
    long long st[DC_STACK]; int sp = 0; int o = 0;
    for (int i = 0; s[i]; ) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { i++; continue; }
        if ((c >= '0' && c <= '9') || (c == '_' && s[i + 1] >= '0' && s[i + 1] <= '9')) {
            int neg = 0; if (c == '_') { neg = 1; i++; }
            long long v = 0;
            while (s[i] >= '0' && s[i] <= '9') { v = v * 10 + (s[i] - '0'); i++; }
            if (neg) v = -v;
            if (sp < DC_STACK) st[sp++] = v;
            continue;
        }
        switch (c) {
            case '+': case '-': case '*': case '/': case '%':
                if (sp >= 2) {
                    long long a = st[sp - 1], b = st[sp - 2], r = 0; int ok = 1;
                    if ((c == '/' || c == '%') && a == 0) ok = 0;                 /* divide by zero: leave stack */
                    else if (c == '+') r = b + a; else if (c == '-') r = b - a;
                    else if (c == '*') r = b * a; else if (c == '/') r = b / a; else r = b % a;
                    if (ok) { sp -= 2; st[sp++] = r; }
                }
                break;
            case 'p': if (sp > 0) { dc_emit_num(out, &o, cap, st[sp - 1]); if (o < cap - 1) out[o++] = '\n'; } break;
            case 'n': if (sp > 0) { dc_emit_num(out, &o, cap, st[sp - 1]); sp--; } break;
            case 'f': for (int k = sp - 1; k >= 0; k--) { dc_emit_num(out, &o, cap, st[k]); if (o < cap - 1) out[o++] = '\n'; } break;
            case 'd': if (sp > 0 && sp < DC_STACK) { st[sp] = st[sp - 1]; sp++; } break;
            case 'r': if (sp >= 2) { long long t = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = t; } break;
            case 'c': sp = 0; break;
            default: break;                                                       /* ignore unknown */
        }
        i++;
    }
    if (o < cap) out[o] = '\0'; else if (cap > 0) out[cap - 1] = '\0';
}
/* ---- end pure core ---- */

int main(int argc, char** argv) {
    if (argc < 2) { printf("Usage: dc <RPN program>   e.g. dc \"3 4 + p\"\n"); return 1; }
    static char prog[4096]; int n = 0;
    for (int a = 1; a < argc; a++) {
        if (a > 1 && n < (int)sizeof(prog) - 1) prog[n++] = ' ';
        for (const char* p = argv[a]; *p && n < (int)sizeof(prog) - 1; p++) prog[n++] = *p;
    }
    prog[n] = '\0';
    static char out[8192];
    dc_run(prog, out, (int)sizeof(out));
    printf("%s", out);
    return 0;
}

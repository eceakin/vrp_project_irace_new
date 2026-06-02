/* =============================================================================
 *  vrp_rng.c : xorshift64 tabanli tekrar-uretebilir RNG
 * =============================================================================*/

#include "vrp_rng.h"

void rng_seed(RNG *r, unsigned long long s) {
    *r = s ^ 0x123456789ABCDEF0ULL;
    if (*r == 0) *r = 1;   /* xorshift'te 0 durumu degismez; onu onle */
}

double rng_double(RNG *r) {
    *r ^= *r >> 12;
    *r ^= *r << 25;
    *r ^= *r >> 27;
    return (double)((*r) * 2685821657736338717ULL) / 18446744073709551616.0;
}

int rng_int(RNG *r, int n) {
    if (n <= 0) return 0;
    return (int)(rng_double(r) * n);
}

int rng_range(RNG *r, int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + rng_int(r, hi - lo + 1);
}

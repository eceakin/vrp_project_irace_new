#ifndef VRP_RNG_H
#define VRP_RNG_H

/* =============================================================================
 *  vrp_rng.h : Basit tekrar-uretebilir rastgele sayi ureteci (xorshift64)
 * =============================================================================*/

#include "vrp_types.h"

void   rng_seed  (RNG *r, unsigned long long s);
double rng_double(RNG *r);
int    rng_int   (RNG *r, int n);
int    rng_range (RNG *r, int lo, int hi);

#endif /* VRP_RNG_H */

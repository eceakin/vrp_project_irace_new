#ifndef VRP_MUTATION_H
#define VRP_MUTATION_H

/* =============================================================================
 *  vrp_mutation.h : Mutasyon operatorleri
 *
 *  Icerik:
 *    - swap_mutate()     : Iki rastgele genin degerini degistir
 *                          [Baker & Ayechew, 2003]
 *    - or_opt_mutate()   : 1-3 musteriden olusan segmenti baska araca tasi
 *    - mutate()          : MUTATION_OR_OPT_PROB olasiligi ile birini sec
 * =============================================================================*/

#include "vrp_types.h"
#include "vrp_rng.h"

/* Iki rastgele pozisyonun arac atamasini degistir */
void swap_mutate(const Chromosome in_chrom, int n,
                 RNG *rng, Chromosome out_chrom);

/* 1-3 musteriden olusan segmenti en iyi baska araca tasi */
void or_opt_mutate(const Chromosome in_chrom, int n,
                   const VRPInstance *inst,
                   RNG *rng, int two_opt_iter, Chromosome out_chrom);

void mutate(const Chromosome in_chrom, int n,
            const VRPInstance *inst, RNG *rng,
            double mut_prob, int two_opt_iter,
            Chromosome out_chrom);

#endif /* VRP_MUTATION_H */

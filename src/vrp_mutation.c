/* =============================================================================
 *  vrp_mutation.c : Mutasyon operatorleri
 *
 *  Referanslar:
 *    [1] Baker & Ayechew (2003) — swap mutasyon
 *    [2] Or (1976)              — or-opt fikri (segment tasima)
 *
 *  Icerik:
 *    swap_mutate()    : Iki rastgele genin arac atamasini degistir
 *    or_opt_mutate()  : 1-3 musteriden olusan segmenti en iyi araca tasi
 *    mutate()         : Olasiliga gore swap veya or-opt sec
 * =============================================================================*/

#include <string.h>
#include "vrp_mutation.h"
#include "vrp_population.h"   /* decode_chromosome icin */

/* =============================================================================
 * Swap Mutasyon  [Baker & Ayechew, 2003]
 *
 * Rastgele secilen iki pozisyonun arac atamasini birbiriyle degistir.
 * Ornek: chrom = [1,2,1,3,2] → [1,3,1,2,2]  (pozisyon 1 ve 3 takas edildi)
 * =============================================================================*/
void swap_mutate(const Chromosome in_chrom, int n,
                 RNG *rng, Chromosome out_chrom) {
    memcpy(out_chrom, in_chrom, sizeof(int) * n);

    int i = rng_int(rng, n);
    int j;
    do { j = rng_int(rng, n); } while (j == i);

    int tmp      = out_chrom[i];
    out_chrom[i] = out_chrom[j];
    out_chrom[j] = tmp;
}

/* =============================================================================
 * Or-opt Mutasyon
 *
 * 1-3 uzunlugunda rastgele bir segment secilir.
 * Segment, fitness + unfitness'i en cok iyilestiren baska bir araca tasinir.
 * Hicbir arac iyilestirme saglamazsa kromozon degistirilmez.
 *
 * delta = (yeni_unfitness - eski_unfitness) * 1e6
 *        + (yeni_fitness  - eski_fitness)
 * (unfitness onceligi var: once feasibility kazan, sonra mesafeyi azalt)
 * =============================================================================*/
void or_opt_mutate(const Chromosome in_chrom, int n,
                   const VRPInstance *inst,
                   RNG *rng,int two_opt_iter,  Chromosome out_chrom) {
    memcpy(out_chrom, in_chrom, sizeof(int) * n);
    int nv = inst->n_vehicles;

    /* Segment uzunlugu: 1, 2 veya 3 musteri */
    int seg_size = 1 + rng_int(rng, 3);
    if (seg_size > n) seg_size = 1;

    /* Rastgele baslangic noktasi */
    int seg_start = rng_int(rng, n - seg_size + 1);

    /* Mevcut fitness */
    FitPair base = decode_chromosome(out_chrom, inst, two_opt_iter);

    double best_delta = 0.0;   /* sadece iyilesme varsa guncelle */
    int    best_v     = -1;

    for (int tv = 1; tv <= nv; tv++) {
        /* Segmentin tamami zaten bu aractaysa tasimaya gerek yok */
        int all_same = 1;
        for (int k = 0; k < seg_size; k++)
            if (out_chrom[seg_start + k] != tv) { all_same = 0; break; }
        if (all_same) continue;

        /* Deneme: segmenti tv'ye tasi */
        Chromosome trial;
        memcpy(trial, out_chrom, sizeof(int) * n);
        for (int k = 0; k < seg_size; k++)
            trial[seg_start + k] = tv;

        FitPair tp = decode_chromosome(trial, inst, two_opt_iter);

        double delta = (tp.unfitness - base.unfitness) * 1e6
                     + (tp.fitness   - base.fitness);

        if (delta < best_delta) {
            best_delta = delta;
            best_v     = tv;
        }
    }

    /* En iyi tasisma buloinduysa uygula */
    if (best_v >= 0)
        for (int k = 0; k < seg_size; k++)
            out_chrom[seg_start + k] = best_v;
}

/* =============================================================================
 * Birlesik Mutasyon
 *
 * MUTATION_OR_OPT_PROB olasiligi ile or-opt, kalan olasilkla swap sec.
 * vrp_types.h icinde tanimli: #define MUTATION_OR_OPT_PROB 0.6
 * =============================================================================*/
void mutate(const Chromosome in_chrom, int n,
            const VRPInstance *inst, RNG *rng,double mut_prob, int two_opt_iter,
            Chromosome out_chrom) {
    if (rng_double(rng) < MUTATION_OR_OPT_PROB)
        or_opt_mutate(in_chrom, n, inst, rng, two_opt_iter, out_chrom);
    else
        swap_mutate(in_chrom, n, rng, out_chrom);
}

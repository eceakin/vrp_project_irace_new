#ifndef VRP_LOCAL_H
#define VRP_LOCAL_H

/* =============================================================================
 *  vrp_local.h : Lokal arama operatorleri
 *
 *  Icerik:
 *    - two_opt()              : Intra-route 2-opt [Lin, 1965]
 *    - reassign_infeasible()  : Infeasible kromozonlarda ihlal azaltma
 *    - lambda_interchange()   : Inter-route relocation + swap [Baker & Ayechew, 2003]
 *    - ranking_replacement()  : Populasyona kabul karari [Baker & Ayechew, 2003]
 * =============================================================================*/

#include "vrp_types.h"
#include "vrp_rng.h"

/*
 * 2-opt intra-route local search  [Lin, 1965]
 * Tek bir rota icindeki musteri siralamasi iyilestirilir.
 * route[0..len-1]: 1-tabanli musteri id'leri (in-place degisir)
 * Donus: iyilesme olduysa 1, olmadiysa 0
 */int two_opt(int *route, int len, const VRPInstance *inst, int max_iter);

void reassign_infeasible(Chromosome chrom,
                          const VRPInstance *inst, RNG *rng,
                          int two_opt_iter);

void lambda_interchange(Chromosome chrom, const VRPInstance *inst,
                         int two_opt_iter);
/*
 * Ranking replacement: child'i populasyona ekle (S1 > S2 > S3 kurallari)
 * Donus: ekleme yapildiysa 1, yapilmadiysa 0
 */
int ranking_replacement(Chromosome population[], FitPair fitnesses[],
                         int pop_size,
                         const Chromosome child, FitPair child_fit);

#endif /* VRP_LOCAL_H */

/* Binary tournament secim: iki rastgele bireyden daha iyisini sec */
int binary_tournament(const FitPair fitnesses[], int pop_size, RNG *rng);

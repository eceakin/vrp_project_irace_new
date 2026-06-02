#ifndef VRP_CROSSOVER_H
#define VRP_CROSSOVER_H

/* =============================================================================
 *  vrp_crossover.h : Crossover operatorleri
 *
 *  Icerik:
 *    - two_point_crossover()  : Standart 2-nokta crossover
 *                               [Baker & Ayechew, 2003]
 *    - optimised_crossover()  : En pahali araci hedefleyen crossover
 *                               [Nazif & Lee, 2012]
 * =============================================================================*/

#include "vrp_types.h"
#include "vrp_rng.h"

/* Standart 2-nokta crossover: [a,b] arasindaki genler esler arasi degisir */
void two_point_crossover(const Chromosome p1, const Chromosome p2,
                          int n, RNG *rng,
                          Chromosome o1, Chromosome o2);

/* Optimised crossover: P1'in en pahali aracini P2'deki en uygun esle */
void optimised_crossover(const Chromosome p1, const Chromosome p2,
                          int n, const VRPInstance *inst, RNG *rng,
                          int two_opt_iter,
                          Chromosome o1, Chromosome o2);

#endif /* VRP_CROSSOVER_H */

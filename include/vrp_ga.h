#ifndef VRP_GA_H
#define VRP_GA_H

/* =============================================================================
 *  vrp_ga.h : Ana GA dongusu
 *
 *  Icerik:
 *    - run_ga() : Secim → Crossover → Mutasyon → Lokal Arama → Replacement
 *
 *  Diger GA bilesenleri ayri modullerden gelir:
 *    vrp_population.h  → decode_chromosome, generate_initial_population
 *    vrp_crossover.h   → two_point_crossover, optimised_crossover
 *    vrp_mutation.h    → swap_mutate, or_opt_mutate, mutate
 *    vrp_local.h       → two_opt, reassign_infeasible, lambda_interchange,
 *                         ranking_replacement
 * =============================================================================*/

#include "vrp_types.h"
#include "vrp_rng.h"
#include "vrp_population.h"
#include "vrp_crossover.h"
#include "vrp_mutation.h"
#include "vrp_local.h"

/*
 * Ana GA dongusu.
 *
 * Parametreler:
 *   inst                   : Problem ornegi
 *   pop_size               : Populasyon buyuklugu          [irace: --pop]
 *   max_generations        : Maksimum nesil sayisi
 *   local_search_interval  : Her kac nesilde bir lambda-interchange uygulanir
 *                            [irace: --local-interval]
 *   no_improve_limit       : Bu kadar nesil iyilesme yoksa erken dur
 *                            [irace: --no-improve]
 *   use_optimised_crossover: 1 = Nazif&Lee, 0 = 2-Point   [irace: --crossover]
 *   mut_prob               : Or-opt mutasyon olasiligi     [irace: --mut-prob]
 *   two_opt_iter           : 2-opt max iterasyon sayisi    [irace: --two-opt-iter]
 *   seed                   : Rastgele sayi tohumu (tekrar-uretilebilirlik)
 *   verbose                : 1 = nesil bazli loglama ac
 */
GAResult run_ga(const VRPInstance *inst,
                int    pop_size,
                int    max_generations,
                int    local_search_interval,
                int    no_improve_limit,
                int    use_optimised_crossover,
                double mut_prob,
                int    two_opt_iter,
                unsigned long long seed,
                int    verbose);

#endif /* VRP_GA_H */
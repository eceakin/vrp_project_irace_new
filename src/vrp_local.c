/* =============================================================================
 *  vrp_local.c : Lokal arama operatorleri ve replacement
 *
 *  Referanslar:
 *    [1] Lin (1965)             — 2-opt
 *    [2] Baker & Ayechew (2003) — reassign_infeasible, lambda_interchange,
 *                                 ranking_replacement
 * =============================================================================*/

#include <string.h>
#include <float.h>
#include "vrp_local.h"
#include "vrp_population.h"

/* =============================================================================
 * 2-opt Intra-route Local Search  [Lin, 1965]
 * =============================================================================*/
int two_opt(int *route, int len, const VRPInstance *inst, int max_iter) {
    if (len < 4)      return 0;
    if (max_iter <= 0) return 0;

    int improved_ever = 0;
    int improved;
    int iter = 0;

    do {
        improved = 0;
        iter++;

        for (int i = 0; i < len - 1; i++) {
            int node_a = (i == 0) ? 0 : route[i - 1];
            int node_b = route[i];

            for (int j = i + 2; j < len; j++) {
                int node_c = route[j];
                int node_d = (j + 1 < len) ? route[j + 1] : 0;

                double delta = (inst->dist[node_a][node_c] +
                                inst->dist[node_b][node_d])
                             - (inst->dist[node_a][node_b] +
                                inst->dist[node_c][node_d]);

                if (delta < -1e-10) {
                    int left  = i;
                    int right = j;
                    while (left < right) {
                        int tmp      = route[left];
                        route[left]  = route[right];
                        route[right] = tmp;
                        left++;
                        right--;
                    }
                    improved      = 1;
                    improved_ever = 1;
                    break;
                }
            }
            if (improved) break;
        }
    } while (improved && iter < max_iter);

    return improved_ever;
}

/* =============================================================================
 * Infeasible Kromozon Duzeltme  [Baker & Ayechew, 2003]
 * =============================================================================*/
void reassign_infeasible(Chromosome chrom,
                          const VRPInstance *inst, RNG *rng,
                          int two_opt_iter) {
    (void)rng;
    int n  = inst->n_customers;
    int nv = inst->n_vehicles;

    FitPair cur = decode_chromosome(chrom, inst, two_opt_iter);
    if (cur.unfitness == 0.0) return;

    int improved = 1;
    while (improved) {
        improved = 0;
        cur = decode_chromosome(chrom, inst, two_opt_iter);

        for (int cid = 1; cid <= n; cid++) {
            int cur_veh  = chrom[cid - 1];
            int neighbors[2];
            neighbors[0] = (cur_veh % nv) + 1;
            neighbors[1] = ((cur_veh - 2 + nv) % nv) + 1;

            double best_u   = cur.unfitness;
            int    best_veh = cur_veh;

            for (int ni = 0; ni < 2; ni++) {
                int nv_id = neighbors[ni];
                if (nv_id == cur_veh) continue;

                int old_v = chrom[cid - 1];
                chrom[cid - 1] = nv_id;
                FitPair trial  = decode_chromosome(chrom, inst, two_opt_iter);
                chrom[cid - 1] = old_v;

                if (trial.unfitness < best_u) {
                    best_u   = trial.unfitness;
                    best_veh = nv_id;
                }
            }

            if (best_veh != cur_veh) {
                chrom[cid - 1] = best_veh;
                improved = 1;
                FitPair after = decode_chromosome(chrom, inst, two_opt_iter);
                if (after.unfitness == 0.0) return;
                cur = after;
            }
        }
    }
}

/* =============================================================================
 * Lambda-Interchange (lambda=1)  [Baker & Ayechew, 2003]
 * =============================================================================*/
void lambda_interchange(Chromosome chrom, const VRPInstance *inst,
                         int two_opt_iter) {
    int n  = inst->n_customers;
    int nv = inst->n_vehicles;

    FitPair best = decode_chromosome(chrom, inst, two_opt_iter);
    int improved = 1;

    while (improved) {
        improved = 0;

        for (int c1 = 1; c1 <= n; c1++) {
            int v1 = chrom[c1 - 1];
            int v2_cands[2];
            v2_cands[0] = (v1 % nv) + 1;
            v2_cands[1] = ((v1 - 2 + nv) % nv) + 1;

            for (int ci = 0; ci < 2; ci++) {
                int v2 = v2_cands[ci];
                if (v2 == v1) continue;

                /* Relocation */
                {
                    int old_v = chrom[c1 - 1];
                    chrom[c1 - 1] = v2;
                    FitPair trial = decode_chromosome(chrom, inst, two_opt_iter);
                    if (trial.unfitness < best.unfitness ||
                        (trial.unfitness == best.unfitness &&
                         trial.fitness   < best.fitness)) {
                        best     = trial;
                        improved = 1;
                    } else {
                        chrom[c1 - 1] = old_v;
                    }
                }

                /* Swap */
                for (int c2 = 1; c2 <= n; c2++) {
                    if (chrom[c2 - 1] != v2) continue;
                    int ov1 = chrom[c1 - 1];
                    int ov2 = chrom[c2 - 1];
                    chrom[c1 - 1] = v2;
                    chrom[c2 - 1] = v1;
                    FitPair trial2 = decode_chromosome(chrom, inst, two_opt_iter);
                    if (trial2.unfitness < best.unfitness ||
                        (trial2.unfitness == best.unfitness &&
                         trial2.fitness   < best.fitness)) {
                        best     = trial2;
                        improved = 1;
                    } else {
                        chrom[c1 - 1] = ov1;
                        chrom[c2 - 1] = ov2;
                    }
                }
            }
        }
    }
}

/* =============================================================================
 * Ranking Replacement  [Baker & Ayechew, 2003]
 * =============================================================================*/
int ranking_replacement(Chromosome population[], FitPair fitnesses[],
                         int pop_size,
                         const Chromosome child, FitPair child_fit) {
    double fc = child_fit.fitness;
    double uc = child_fit.unfitness;

    int    target_idx = -1;
    double best_u = -1.0, best_f = -1.0;

#define FIND_WORST(cond_f, cond_u)                                   \
    for (int i = 0; i < pop_size; i++) {                             \
        if ((cond_f) && (cond_u)) {                                  \
            if (fitnesses[i].unfitness > best_u ||                   \
                (fitnesses[i].unfitness == best_u &&                 \
                 fitnesses[i].fitness   > best_f)) {                 \
                best_u     = fitnesses[i].unfitness;                 \
                best_f     = fitnesses[i].fitness;                   \
                target_idx = i;                                      \
            }                                                        \
        }                                                            \
    }

    FIND_WORST(fitnesses[i].fitness > fc, fitnesses[i].unfitness > uc)
    if (target_idx >= 0) goto do_replace;

    best_u = best_f = -1.0;
    FIND_WORST(fitnesses[i].fitness < fc, fitnesses[i].unfitness > uc)
    if (target_idx >= 0) goto do_replace;

    best_u = best_f = -1.0;
    FIND_WORST(fitnesses[i].fitness > fc, fitnesses[i].unfitness < uc)
    if (target_idx < 0) return 0;

#undef FIND_WORST

do_replace:
    memcpy(population[target_idx], child, sizeof(int) * MAX_CUSTOMERS);
    fitnesses[target_idx] = child_fit;
    return 1;
}

/* =============================================================================
 * Binary Tournament Secim  [Baker & Ayechew, 2003]
 * =============================================================================*/
int binary_tournament(const FitPair fitnesses[], int pop_size, RNG *rng) {
    int i = rng_int(rng, pop_size);
    int j;
    do { j = rng_int(rng, pop_size); } while (j == i);

    double fi = fitnesses[i].fitness, ui = fitnesses[i].unfitness;
    double fj = fitnesses[j].fitness, uj = fitnesses[j].unfitness;

    if (ui == 0.0 && uj > 0.0) return i;
    if (uj == 0.0 && ui > 0.0) return j;
    return (fi <= fj) ? i : j;
}
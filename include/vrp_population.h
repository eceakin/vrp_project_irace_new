#ifndef VRP_POPULATION_H
#define VRP_POPULATION_H

/* =============================================================================
 *  vrp_population.h : Kromozon cozumleme ve baslangic populasyonu
 *
 *  Icerik:
 *    - route_distance()            : Tek rota toplam mesafesi
 *    - nearest_neighbour_tsp()     : Greedy rota siralama (NN heuristik)
 *    - decode_chromosome()         : Kromozon → fitness + unfitness
 *    - generate_initial_population(): Sweep + rastgele baslangic populasyonu
 * =============================================================================*/

#include "vrp_types.h"
#include "vrp_rng.h"

/* Tek rota toplam mesafesi (depo → musteriler → depo + drop_allowance) */
double route_distance(const int *route, int len, const VRPInstance *inst);

/* Nearest-Neighbour greedy rota siralama */
void nearest_neighbour_tsp(const int *customers, int n,
                            const VRPInstance *inst, int *out);

/* Kromozon cozumleme: fitness (toplam mesafe) ve unfitness (ihlal) hesapla */
FitPair decode_chromosome(const Chromosome chrom, const VRPInstance *inst, int two_opt_iter);
                        
/* Sweep + rastgele karisimi ile baslangic populasyonu olustur */
void generate_initial_population(const VRPInstance *inst,
                                  int pop_size,
                                  RNG *rng,
                                  Chromosome population[]);

#endif /* VRP_POPULATION_H */

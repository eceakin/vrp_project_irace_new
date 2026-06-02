#ifndef VRP_TYPES_H
#define VRP_TYPES_H

/* =============================================================================
 *  VRP Genetic Algorithm — v2 + 2-opt Local Search
 *  vrp_types.h : Ortak veri yapilari ve sabitler
 * =============================================================================*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* Windows / MinGW'de M_PI tanimsiz olabilir */
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

/* ── Derleme zamani sabitler ──────────────────────────────────────────────── */
#define MAX_CUSTOMERS   300
#define MAX_VEHICLES     50
#define MAX_POP         200

/* ── Veri yapilari ──────────────────────────────────────────────────────── */

typedef struct {
    int    id;
    double x, y;
    double demand;
    double drop_allowance;
} Customer;

typedef struct {
    int      n_customers;
    int      n_vehicles;
    double   capacity;
    double   max_distance;
    double   depot_x, depot_y;
    Customer customers[MAX_CUSTOMERS];
    double   dist[MAX_CUSTOMERS + 1][MAX_CUSTOMERS + 1];
    double   total_demand;
} VRPInstance;

/* Kromozon: chrom[i] = arac numarasi (1-tabanli) musteri i+1 icin */
typedef int Chromosome[MAX_CUSTOMERS];

typedef struct {
    double fitness;    /* toplam mesafe */
    double unfitness;  /* ihlal miktari (0 = gecerli) */
} FitPair;

typedef struct {
    double best_distance;
    double best_unfitness;
    int    generations_run;
    double cpu_seconds;
    int    best_chrom[MAX_CUSTOMERS];
} GAResult;

/* ── Rastgele sayi ureteci ──────────────────────────────────────────────── */
typedef unsigned long long RNG;

/* ── GA parametreleri ───────────────────────────────────────────────────── */
#define MUTATION_OR_OPT_PROB  0.6   /* or-opt mutasyon olasiligi            */
#define GA_POP_SIZE           30    /* populasyon buyuklugu                  */
#define GA_MAX_GENERATIONS    30000 /* maksimum nesil sayisi                 */
#define GA_LOCAL_INTERVAL     3000  /* periyodik lambda-interchange araligi  */
#define GA_NO_IMPROVE_LIMIT   5000  /* erken durdurma: iyilesme olmayan nesil*/
#define GA_USE_OPT_CROSSOVER  1     /* 1 = optimised crossover, 0 = 2-point  */
#define N_RUNS                30    /* her problem icin bagimsiz calistirma  */

/* ── 2-opt parametreleri ────────────────────────────────────────────────── */
#define TWO_OPT_ENABLED       1     /* 1 = 2-opt aktif, 0 = devre disi       */
#define TWO_OPT_MAX_ITER      5     /* decode'da max 2-opt iterasyon sayisi  */

#endif /* VRP_TYPES_H */
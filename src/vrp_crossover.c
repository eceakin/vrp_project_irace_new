/* =============================================================================
 *  vrp_crossover.c : Crossover operatorleri
 *
 *  Referanslar:
 *    [1] Baker & Ayechew (2003) — 2-nokta crossover
 *    [2] Nazif & Lee (2012)     — Optimised crossover
 *
 *  Icerik:
 *    two_point_crossover() : Klasik 2-nokta crossover
 *    avg_min_dist()        : Arac kume benzerlik olcusu (static, yardimci)
 *    optimised_crossover() : En pahali araci hedefleyen crossover
 * =============================================================================*/

#include <string.h>
#include <float.h>
#include "vrp_crossover.h"
#include "vrp_population.h"   /* nearest_neighbour_tsp, route_distance icin */
#include "vrp_local.h"        /* two_opt icin */

/* =============================================================================
 * 2-Nokta Crossover  [Baker & Ayechew, 2003]
 *
 * Rastgele iki kesim noktasi (a, b) belirlenir.
 * o1 = p1[0..a-1] | p2[a..b] | p1[b+1..n-1]
 * o2 = p2[0..a-1] | p1[a..b] | p2[b+1..n-1]
 * =============================================================================*/
void two_point_crossover(const Chromosome p1, const Chromosome p2,
                          int n, RNG *rng,
                          Chromosome o1, Chromosome o2) {
    int a = rng_int(rng, n);
    int b;
    do { b = rng_int(rng, n); } while (b == a);
    if (a > b) { int t = a; a = b; b = t; }

    for (int i = 0; i < n; i++) {
        if (i >= a && i <= b) { o1[i] = p2[i]; o2[i] = p1[i]; }
        else                  { o1[i] = p1[i]; o2[i] = p2[i]; }
    }
}

/* =============================================================================
 * Yardimci: Ortalama minimum mesafe (arac kume benzerligi)
 *
 * custs_a[0..na-1] ve custs_b[0..nb-1]: 1-tabanli musteri id'leri
 * Her a musterisi icin b kumesindeki en yakin musteriye olan mesafeyi bul,
 * ortalamalarini dondur. Dusuk deger = kumeler birbirine yakin = iyi eslesme.
 * =============================================================================*/
static double avg_min_dist(const int *custs_a, int na,
                            const int *custs_b, int nb,
                            const VRPInstance *inst) {
    if (na == 0 || nb == 0) return 1e18;

    double total = 0.0;
    for (int i = 0; i < na; i++) {
        double best = 1e18;
        for (int j = 0; j < nb; j++) {
            double d = inst->dist[custs_a[i]][custs_b[j]];
            if (d < best) best = d;
        }
        total += best;
    }
    return total / na;
}

/* =============================================================================
 * Optimised Crossover  [Nazif & Lee, 2012]
 *
 * Adimlar:
 *   1. P1'deki en pahali (en uzun) araci bul → worst_v
 *   2. P2'deki araclar arasinda worst_v'ye en cok benzeyen araci bul
 *      (avg_min_dist ile olculur) → best_match
 *   3. Gen degisimi:
 *      o1 = P1 tabanli; yalnizca worst_v pozisyonlarinda P2'den gen al
 *      o2 = P2 tabanli; yalnizca best_match pozisyonlarinda P1'den gen al
 *
 * Fallback: worst_v bos ise standart 2-nokta crossover'a don.
 * =============================================================================*/
void optimised_crossover(const Chromosome p1, const Chromosome p2,
                          int n, const VRPInstance *inst, RNG *rng,int two_opt_iter,
                          Chromosome o1, Chromosome o2) {
    int nv = inst->n_vehicles;

    /* P1 arac-musteri gruplari */
    int vcusts1[MAX_VEHICLES][MAX_CUSTOMERS];
    int vcnt1[MAX_VEHICLES];
    memset(vcnt1, 0, sizeof(vcnt1));
    for (int i = 0; i < n; i++)
        vcusts1[p1[i] - 1][vcnt1[p1[i] - 1]++] = i + 1;

    /* P1'de en pahali araci bul */
    int    worst_v    = 0;
    double worst_cost = -1.0;
    for (int v = 0; v < nv; v++) {
        if (vcnt1[v] == 0) continue;
        int route[MAX_CUSTOMERS];
        nearest_neighbour_tsp(vcusts1[v], vcnt1[v], inst, route);
#if TWO_OPT_ENABLED
        two_opt(route, vcnt1[v], inst, two_opt_iter);
#endif
        double cost = route_distance(route, vcnt1[v], inst);
        if (cost > worst_cost) { worst_cost = cost; worst_v = v; }
    }

    /* worst_v bos ise fallback */
    if (vcnt1[worst_v] == 0) {
        two_point_crossover(p1, p2, n, rng, o1, o2);
        return;
    }

    /* P2 arac-musteri gruplari */
    int vcusts2[MAX_VEHICLES][MAX_CUSTOMERS];
    int vcnt2[MAX_VEHICLES];
    memset(vcnt2, 0, sizeof(vcnt2));
    for (int i = 0; i < n; i++)
        vcusts2[p2[i] - 1][vcnt2[p2[i] - 1]++] = i + 1;

    /* P2'de worst_v'ye en cok benzeyen araci bul */
    int    best_match   = -1;
    double best_match_s = 1e18;
    for (int v = 0; v < nv; v++) {
        if (vcnt2[v] == 0) continue;
        double s = avg_min_dist(vcusts1[worst_v], vcnt1[worst_v],
                                vcusts2[v],       vcnt2[v], inst);
        if (s < best_match_s) { best_match_s = s; best_match = v; }
    }

    /* Gecerli eslesme bulunamadiysa fallback */
    if (best_match < 0) {
        two_point_crossover(p1, p2, n, rng, o1, o2);
        return;
    }

    /* Gen alimi:
     *   o1 = P1 tabanli; worst_v pozisyonlarinda P2'nin geni kullan
     *   o2 = P2 tabanli; best_match pozisyonlarinda P1'in geni kullan */
    memcpy(o1, p1, sizeof(int) * n);
    memcpy(o2, p2, sizeof(int) * n);

    for (int i = 0; i < n; i++)
        if (p1[i] - 1 == worst_v)    o1[i] = p2[i];

    for (int i = 0; i < n; i++)
        if (p2[i] - 1 == best_match) o2[i] = p1[i];
}

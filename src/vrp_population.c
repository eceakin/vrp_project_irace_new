/* =============================================================================
 *  vrp_population.c : Kromozon cozumleme ve baslangic populasyonu
 *
 *  Referans: Baker & Ayechew (2003), Bolum 2-3
 *
 *  Icerik:
 *    route_distance()             : Depo→musteriler→depo toplam mesafesi
 *    nearest_neighbour_tsp()      : Greedy rota siralama
 *    decode_chromosome()          : Kromozon → fitness + unfitness
 *    sort_by_polar_angle()        : Sweep icin polar aci siralaması (static)
 *    sweep_chromosome()           : Tek sweep kromozon uretimi (static)
 *    generate_initial_population(): Populasyon baslatma
 * =============================================================================*/

#include <string.h>
#include <float.h>
#include "vrp_population.h"
#include "vrp_local.h"        /* two_opt icin */
#include "vrp_instance.h"     /* polar_angle icin */

/* =============================================================================
 * Rota mesafesi
 * route[0..len-1]: 1-tabanli musteri id'leri
 * Toplam = depo→c1 + c1→c2 + ... + cn→depo + sum(drop_allowance)
 * =============================================================================*/
double route_distance(const int *route, int len, const VRPInstance *inst) {
    if (len == 0) return 0.0;

    double d = inst->dist[0][route[0]];
    for (int i = 0; i < len - 1; i++)
        d += inst->dist[route[i]][route[i + 1]];
    d += inst->dist[route[len - 1]][0];

    for (int i = 0; i < len; i++)
        d += inst->customers[route[i] - 1].drop_allowance;

    return d;
}

/* =============================================================================
 * Nearest-Neighbour TSP
 * Depodan (node 0) baslar, her adimda en yakin ziyaret edilmemis musteri secilir.
 * customers[0..n-1]: 1-tabanli musteri id'leri (kaynak, degistirilmez)
 * out[0..n-1]      : NN sirasina gore yeniden duzenlenmis id'ler
 * =============================================================================*/
void nearest_neighbour_tsp(const int *customers, int n,
                            const VRPInstance *inst, int *out) {
    if (n == 0) return;

    int visited[MAX_CUSTOMERS];
    memset(visited, 0, sizeof(int) * (size_t)n);

    int current = 0; /* depo */
    for (int step = 0; step < n; step++) {
        double best_d = DBL_MAX;
        int    best_i = -1;

        for (int i = 0; i < n; i++) {
            if (!visited[i]) {
                double d = inst->dist[current][customers[i]];
                if (d < best_d) { best_d = d; best_i = i; }
            }
        }

        visited[best_i] = 1;
        out[step]       = customers[best_i];
        current         = customers[best_i];
    }
}

/* =============================================================================
 * Kromozon cozumleme  (Baker & Ayechew §2)
 *
 * Her arac icin:
 *   1. Atanan musterileri topla
 *   2. NN-TSP ile rotayi sirala
 *   3. 2-opt ile intra-route iyilestir  [TWO_OPT_ENABLED = 1 ise]
 *   4. Kapasite ve mesafe ihlallerini hesapla
 * =============================================================================*/
FitPair decode_chromosome(const Chromosome chrom, const VRPInstance *inst, int two_opt_iter) {
    int n  = inst->n_customers;
    int nv = inst->n_vehicles;

    int vcusts[MAX_VEHICLES][MAX_CUSTOMERS];
    int vcnt[MAX_VEHICLES];
    memset(vcnt, 0, sizeof(vcnt));

    /* Her musteri hangi araca atanmis? */
    for (int i = 0; i < n; i++) {
        int v = chrom[i] - 1;          /* 0-tabanli arac indisi */
        vcusts[v][vcnt[v]++] = i + 1; /* 1-tabanli musteri id  */
    }

    double total_dist      = 0.0;
    double total_unfitness = 0.0;

    for (int v = 0; v < nv; v++) {
        int cnt = vcnt[v];
        if (cnt == 0) continue;

        /* 1. NN-TSP ile baslangic rota sirasi */
        int route[MAX_CUSTOMERS];
        nearest_neighbour_tsp(vcusts[v], cnt, inst, route);

        /* 2. 2-opt intra-route iyilestirme */
#if TWO_OPT_ENABLED
        two_opt(route, cnt, inst, two_opt_iter);
#endif

        /* 3. Rota mesafesi */
        total_dist += route_distance(route, cnt, inst);

        /* 4. Kapasite ihlali: (asim / kapasite) */
        double load = 0.0;
        for (int i = 0; i < cnt; i++)
            load += inst->customers[vcusts[v][i] - 1].demand;
        if (load > inst->capacity)
            total_unfitness += (load - inst->capacity) / inst->capacity;

        /* 5. Mesafe kisiti ihlali (sadece max_distance > 0 problemler) */
        if (inst->max_distance > 0.0) {
            double rd = route_distance(route, cnt, inst);
            if (rd > inst->max_distance)
                total_unfitness += (rd - inst->max_distance) / inst->max_distance;
        }
    }

    FitPair fp = { total_dist, total_unfitness };
    return fp;
}

/* =============================================================================
 * Sweep algoritmasiyla musteri siralaması  (Baker & Ayechew §3)
 * Musterileri depo cevresindeki polar aciya gore siralar (artan).
 * =============================================================================*/
static void sort_by_polar_angle(const VRPInstance *inst,
                                  int *sorted_ids, int n) {
    for (int i = 0; i < n; i++) sorted_ids[i] = i + 1;

    /* Kabarcik siralama — n<=300 icin yeterli performans */
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            double a1 = polar_angle(
                inst->customers[sorted_ids[j]     - 1].x,
                inst->customers[sorted_ids[j]     - 1].y,
                inst->depot_x, inst->depot_y);
            double a2 = polar_angle(
                inst->customers[sorted_ids[j + 1] - 1].x,
                inst->customers[sorted_ids[j + 1] - 1].y,
                inst->depot_x, inst->depot_y);
            if (a1 > a2) {
                int tmp            = sorted_ids[j];
                sorted_ids[j]      = sorted_ids[j + 1];
                sorted_ids[j + 1]  = tmp;
            }
        }
    }
}

/* =============================================================================
 * Sweep kromozon uretimi
 *
 * sorted_ids listesi ustunde rastgele bir baslangic noktasindan taranir.
 * Kapasite veya mesafe dolunca yeni arac acilir.
 * Tightness yuksekse kucuk ihlallere izin verilir (keep_threshold).
 * =============================================================================*/
static void sweep_chromosome(const int *sorted_ids, int n,
                               const VRPInstance *inst,
                               RNG *rng, Chromosome out_chrom) {
    int chrom_by_cid[MAX_CUSTOMERS + 1];
    memset(chrom_by_cid, 0, sizeof(chrom_by_cid));

    /* Rastgele baslangic ofseti: farkli sweeplar -> cesitli kromozonlar */
    int start = rng_int(rng, n);

    double tightness      = inst->total_demand /
                            ((double)inst->n_vehicles * inst->capacity);
    double keep_threshold = (tightness >= 0.95) ? 0.75 : 0.9;

    int    vehicle   = 1;
    double load      = 0.0;
    double dist_used = 0.0;
    int    prev      = 0; /* depo */

    for (int s = 0; s < n; s++) {
        int    cid  = sorted_ids[(start + s) % n];
        int    cidx = cid - 1;
        double dem  = inst->customers[cidx].demand;
        double drop = inst->customers[cidx].drop_allowance;
        double d_to   = inst->dist[prev][cid];
        double d_back = inst->dist[cid][0];

        int cap_ok  = (load + dem <= inst->capacity);
        int dist_ok = (inst->max_distance == 0.0 ||
                       dist_used + d_to + d_back + drop <= inst->max_distance);

        if (cap_ok && dist_ok) {
            /* Kalan kapasiteye sigmis: ayni araca ekle */
            chrom_by_cid[cid]  = vehicle;
            load               += dem;
            dist_used          += d_to + drop;
            prev                = cid;
        } else {
            /* Sigmadi: kalan kapasite orani kontrol et */
            double rc = (dem > 0.0) ? (inst->capacity - load) / dem : 1.0;
            if (rc >= keep_threshold) {
                /* Kucuk asim kabul edilebilir, ayni araca ekle */
                chrom_by_cid[cid] = vehicle;
                load             += dem;
            } else {
                /* Yeni arac ac */
                vehicle = (vehicle < inst->n_vehicles) ? vehicle + 1
                                                        : inst->n_vehicles;
                chrom_by_cid[cid] = vehicle;
                load              = dem;
                dist_used         = 0.0;
                prev              = 0;
            }
        }
    }

    /* Edge case: atanmamis musteri kalmissa rastgele ata */
    for (int cid = 1; cid <= n; cid++) {
        if (chrom_by_cid[cid] == 0)
            chrom_by_cid[cid] = rng_range(rng, 1, inst->n_vehicles);
    }

    /* Diziye donustur: out_chrom[i] = musteri (i+1)'in araci */
    for (int i = 0; i < n; i++)
        out_chrom[i] = chrom_by_cid[i + 1];
}

/* =============================================================================
 * Baslangic populasyonu olustur  (Baker & Ayechew §3)
 *
 * Ilk 2/3: Sweep tabanli yapilandirilmis kromozonlar
 * Son 1/3: Tamamen rastgele kromozonlar (cesitlilik icin)
 * =============================================================================*/
void generate_initial_population(const VRPInstance *inst,
                                   int pop_size,
                                   RNG *rng,
                                   Chromosome population[]) {
    int n = inst->n_customers;
    int sorted_ids[MAX_CUSTOMERS];
    sort_by_polar_angle(inst, sorted_ids, n);

    for (int p = 0; p < pop_size; p++) {
        if (p < pop_size * 2 / 3) {
            /* Sweep: her seferinde farkli baslangic noktasi -> cesitlilik */
            sweep_chromosome(sorted_ids, n, inst, rng, population[p]);
        } else {
            /* Tamamen rastgele: [1, n_vehicles] araliginda */
            for (int i = 0; i < n; i++)
                population[p][i] = rng_range(rng, 1, inst->n_vehicles);
        }
    }
}
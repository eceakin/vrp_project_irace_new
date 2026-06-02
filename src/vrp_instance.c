/* =============================================================================
 *  vrp_instance.c : Problem ornegi yukleme, mesafe matrisi, demo uretici
 * =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "vrp_instance.h"
#include "vrp_rng.h"

/* ─────────────────────────────────────────────────────────────────────────────
 * Polar aci (sweep icin)
 * ───────────────────────────────────────────────────────────────────────────*/
double polar_angle(double cx, double cy, double depot_x, double depot_y) {
    double a = atan2(cy - depot_y, cx - depot_x);
    if (a < 0.0) a += 2.0 * M_PI;
    return a;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Euclidean mesafe matrisi
 * node 0 = depo, node i = musteri i (1-tabanli)
 * ───────────────────────────────────────────────────────────────────────────*/
void build_distance_matrix(VRPInstance *inst) {
    int n = inst->n_customers;

    double nx[MAX_CUSTOMERS + 1], ny[MAX_CUSTOMERS + 1];
    nx[0] = inst->depot_x;
    ny[0] = inst->depot_y;
    for (int i = 1; i <= n; i++) {
        nx[i] = inst->customers[i - 1].x;
        ny[i] = inst->customers[i - 1].y;
    }

    for (int i = 0; i <= n; i++) {
        inst->dist[i][i] = 0.0;
        for (int j = i + 1; j <= n; j++) {
            double dx = nx[i] - nx[j];
            double dy = ny[i] - ny[j];
            double d  = sqrt(dx * dx + dy * dy);
            inst->dist[i][j] = inst->dist[j][i] = d;
        }
    }

    inst->total_demand = 0.0;
    for (int i = 0; i < n; i++)
        inst->total_demand += inst->customers[i].demand;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * OR-Library CMT formati dosya okuyucu
 *
 * Satir 1: n_customers  capacity  max_distance  [drop_allowance_global]
 *          max_distance >= 999999 → sinir yok (0.0 olarak saklanir)
 * Satir 2: depot_x  depot_y
 * Geri kalan satirlar: cust_x  cust_y  demand
 * ───────────────────────────────────────────────────────────────────────────*/
int parse_or_library(const char *filepath, VRPInstance *inst) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Dosya acilamadi: %s\n", filepath);
        return 0;
    }

    int    n_cust;
    double cap, max_d, drop_global, dx, dy;

    if (fscanf(fp, "%d %lf %lf", &n_cust, &cap, &max_d) != 3) {
        fclose(fp);
        return 0;
    }

    /* Opsiyonel 4. sutun: global drop_allowance */
    drop_global = 0.0;
    {
        long pos = ftell(fp);
        if (fscanf(fp, "%lf", &drop_global) != 1) {
            fseek(fp, pos, SEEK_SET);
            drop_global = 0.0;
        }
    }

    if (fscanf(fp, "%lf %lf", &dx, &dy) != 2) {
        fclose(fp);
        return 0;
    }

    inst->capacity     = cap;
    inst->max_distance = (max_d >= 999999.0) ? 0.0 : max_d;
    inst->depot_x      = dx;
    inst->depot_y      = dy;

    int    cid = 0;
    double x, y, dem;
    while (cid < n_cust && fscanf(fp, "%lf %lf %lf", &x, &y, &dem) == 3) {
        inst->customers[cid].id             = cid + 1;
        inst->customers[cid].x              = x;
        inst->customers[cid].y              = y;
        inst->customers[cid].demand         = dem;
        inst->customers[cid].drop_allowance = drop_global;
        cid++;
    }
    fclose(fp);
    inst->n_customers = cid;

    /* n_vehicles: ceil(toplam_talep / kapasite), en az 1 */
    double total_dem = 0.0;
    for (int i = 0; i < cid; i++)
        total_dem += inst->customers[i].demand;

    inst->n_vehicles = (int)ceil(total_dem / cap);
    if (inst->n_vehicles < 1)            inst->n_vehicles = 1;
    if (inst->n_vehicles > MAX_VEHICLES) inst->n_vehicles = MAX_VEHICLES;

    return 1;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Demo ornegi olusturucu (test / gecerlilik dogrulamasi icin)
 * ───────────────────────────────────────────────────────────────────────────*/
void build_demo_instance(int n_customers, unsigned long long seed,
                          VRPInstance *inst) {
    RNG rng;
    rng_seed(&rng, seed);

    inst->n_customers  = n_customers;
    inst->n_vehicles   = 5;
    inst->capacity     = 160.0;
    inst->max_distance = 0.0;
    inst->depot_x      = 50.0;
    inst->depot_y      = 50.0;

    for (int i = 0; i < n_customers; i++) {
        inst->customers[i].id             = i + 1;
        inst->customers[i].x              = rng_double(&rng) * 100.0;
        inst->customers[i].y              = rng_double(&rng) * 100.0;
        inst->customers[i].demand         = 1.0 + rng_double(&rng) * 14.0;
        inst->customers[i].drop_allowance = 0.0;
    }
}

#ifndef VRP_INSTANCE_H
#define VRP_INSTANCE_H

/* =============================================================================
 *  vrp_instance.h : Problem ornegi yukleme ve mesafe matrisi
 * =============================================================================*/

#include "vrp_types.h"

/* OR-Library CMT formati dosya okuyucu */
int    parse_or_library   (const char *filepath, VRPInstance *inst);

/* Demo ornegi olustur (test amacli) */
void   build_demo_instance(int n_customers, unsigned long long seed,
                            VRPInstance *inst);

/* Euclidean mesafe matrisini hesapla (bir kez cagrilir) */
void   build_distance_matrix(VRPInstance *inst);

/* Polar aci: sweep tabanli baslangic populasyonu icin */
double polar_angle(double cx, double cy, double depot_x, double depot_y);

#endif /* VRP_INSTANCE_H */

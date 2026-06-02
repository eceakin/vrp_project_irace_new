/* =============================================================================
 *  vrp_report.c : Iterasyon bazli raporlama + CSV cikti
 * =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vrp_report.h"

/* ── Ekran baslik satirı ─────────────────────────────────────────────────── */
void report_header(const char *problem_name) {
    printf("\n");
    for (int i = 0; i < 82; i++) printf("=");
    printf("\n  Problem: %-20s  |  %d bagimsiz calistirma\n",
           problem_name, N_RUNS);
    for (int i = 0; i < 82; i++) printf("=");
    printf("\n");

    printf("  %-4s  %-18s  %-12s  %-10s  %-10s  %-8s  %s\n",
           "Run", "Seed", "Mesafe", "Unfitness", "Nesil", "Sure(s)", "Durum");
    printf("  ");
    for (int i = 0; i < 76; i++) printf("-");
    printf("\n");
}

/* ── Tek run satirı ────────────────────────────────────────────────────────── */
void report_run(const RunRecord *rec) {
    printf("  %-4d  %-18llu  %-12.4f  %-10.4f  %-10d  %-8.2f  %s\n",
           rec->run_id,
           rec->seed,
           rec->distance,
           rec->unfitness,
           rec->generations_run,
           rec->cpu_seconds,
           rec->feasible ? "OK" : "INFEASIBLE");
}

/* ── Ozet istatistik tablosu ─────────────────────────────────────────────── */
void report_summary(const RunRecord *records, int n, const char *problem_name) {
    double min_d = 1e18, max_d = -1e18, sum_d = 0.0, sum_sq = 0.0;
    int    feasible_cnt = 0;
    double min_feas = 1e18, max_feas = -1e18, sum_feas = 0.0;

    for (int i = 0; i < n; i++) {
        double d = records[i].distance;
        if (d < min_d) min_d = d;
        if (d > max_d) max_d = d;
        sum_d  += d;
        sum_sq += d * d;
        if (records[i].feasible) {
            feasible_cnt++;
            if (d < min_feas) min_feas = d;
            if (d > max_feas) max_feas = d;
            sum_feas += d;
        }
    }

    double avg_d = sum_d / n;
    double var   = (n > 1) ? (sum_sq / n - avg_d * avg_d) : 0.0;
    double std_d = (var > 0.0) ? sqrt(var) : 0.0;

    printf("\n");
    for (int i = 0; i < 82; i++) printf("-");
    printf("\n  OZET — %s\n", problem_name);
    for (int i = 0; i < 82; i++) printf("-");
    printf("\n");

    printf("  Toplam calistirma       : %d\n", n);
    printf("  Gecerli (feasible)      : %d / %d  (%.1f%%)\n",
           feasible_cnt, n, 100.0 * feasible_cnt / n);

    printf("\n  [ Tum calistirmalar ]\n");
    printf("  En iyi mesafe           : %.4f\n", min_d);
    printf("  En kotu mesafe          : %.4f\n", max_d);
    printf("  Ortalama                : %.4f\n", avg_d);
    printf("  Standart sapma          : %.4f\n", std_d);

    if (feasible_cnt > 0) {
        double avg_feas = sum_feas / feasible_cnt;
        printf("\n  [ Yalnizca feasible calistirmalar ]\n");
        printf("  En iyi mesafe           : %.4f\n", min_feas);
        printf("  En kotu mesafe          : %.4f\n", max_feas);
        printf("  Ortalama                : %.4f\n", avg_feas);
    }

    for (int i = 0; i < 82; i++) printf("=");
    printf("\n\n");
}

/* ── CSV baslik (bir kez yaz) ─────────────────────────────────────────────── */
void report_csv_header(const char *csv_path) {
    FILE *fp = fopen(csv_path, "w");
    if (!fp) { perror("CSV baslik yazilamadi"); return; }
    fprintf(fp,
        "problem,run_id,seed,distance,unfitness,"
        "generations_run,cpu_seconds,feasible\n");
    fclose(fp);
}

/* ── CSV'ye satirlar ekle ─────────────────────────────────────────────────── */
void report_csv_append(const char *csv_path,
                        const char *problem_name,
                        const RunRecord *records,
                        int n) {
    FILE *fp = fopen(csv_path, "a");
    if (!fp) { perror("CSV acilamadi"); return; }
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%s,%d,%llu,%.6f,%.6f,%d,%.3f,%d\n",
                problem_name,
                records[i].run_id,
                records[i].seed,
                records[i].distance,
                records[i].unfitness,
                records[i].generations_run,
                records[i].cpu_seconds,
                records[i].feasible);
    }
    fclose(fp);
}

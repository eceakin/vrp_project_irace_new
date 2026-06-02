#ifndef VRP_REPORT_H
#define VRP_REPORT_H

/* =============================================================================
 *  vrp_report.h : Iterasyon ve ozet raporlama
 * =============================================================================*/

#include "vrp_types.h"

/* Tek run sonucunu sakla */
typedef struct {
    int                run_id;
    double             distance;
    double             unfitness;
    int                generations_run;
    double             cpu_seconds;
    unsigned long long seed;
    int                feasible;   /* 1 = gecerli cozum, 0 = infeasible */
} RunRecord;

/* Ozet istatistikler */
typedef struct {
    double min_dist;
    double max_dist;
    double avg_dist;
    double std_dist;
    int    feasible_count;
    int    n_runs;
} RunSummary;

/* Baslik satirini ekrana yaz */
void report_header (const char *problem_name);

/* Tek run satirini ekrana yaz */
void report_run    (const RunRecord *rec);

/* Ozet tabloyu ekrana yaz */
void report_summary(const RunRecord *records, int n, const char *problem_name);

/* CSV dosyasinin basligini olustur (program basinda bir kez cagrilir) */
void report_csv_header(const char *csv_path);

/* CSV'ye run kayitlarini ekle */
void report_csv_append(const char *csv_path,
                        const char *problem_name,
                        const RunRecord *records,
                        int n);

#endif /* VRP_REPORT_H */

/* =============================================================================
 *  main.c : VRP GA — 14 problem x 30 bagimsiz calistirma
 *
 *  Derleme : make
 *  Kullanim (normal mod):
 *    ./vrp_ga                              → tum 14 problem
 *    ./vrp_ga data/vrpnc1.txt             → tek dosya
 *    ./vrp_ga data/vrpnc1.txt -v          → tek dosya + verbose log
 *
 *  Kullanim (irace modu):
 *    ./vrp_ga --irace <instance_id> <seed> <instance_file>
 *             [--pop <int>]
 *             [--no-improve <int>]
 *             [--mut-prob <float>]
 *             [--crossover <0|1>]
 *             [--local-interval <int>]
 *             [--two-opt-iter <int>]
 *
 *    Ornek:
 *    ./vrp_ga --irace 1 42 data/vrpnc1.txt \
 *             --pop 40 --no-improve 3000 --mut-prob 0.7 \
 *             --crossover 1 --local-interval 2000 --two-opt-iter 3
 *
 *    irace modunda program yalnizca tek bir sayisal deger yazdirir (stdout):
 *      - Feasible run bulunduysa: en iyi feasible mesafe
 *      - Hic feasible bulunamazsa: mesafe + 1e6 * unfitness (penalti)
 *
 *  Seed stratejisi (normal mod):
 *    Run i icin seed = BASE_SEED + i * SEED_STEP
 *    Asal SEED_STEP sayesinde ardisik seedler korelasyonsuzdur.
 * =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vrp_types.h"
#include "vrp_instance.h"
#include "vrp_ga.h"
#include "vrp_report.h"

/* ── Seed stratejisi ─────────────────────────────────────────────────────── */
#define BASE_SEED   1000ULL
#define SEED_STEP     97ULL

/* ── Varsayilan problem dosyalari ─────────────────────────────────────────── */
static const char *DEFAULT_PROBLEMS[] = {
    "data/vrpnc1.txt",  "data/vrpnc2.txt",  "data/vrpnc3.txt",
    "data/vrpnc4.txt",  "data/vrpnc5.txt",  "data/vrpnc6.txt",
    "data/vrpnc7.txt",  "data/vrpnc8.txt",  "data/vrpnc9.txt",
    "data/vrpnc10.txt", "data/vrpnc11.txt", "data/vrpnc12.txt",
    "data/vrpnc13.txt", "data/vrpnc14.txt",
    NULL
};

/* ── GA parametre yapisi ──────────────────────────────────────────────────── */
typedef struct {
    int    pop_size;
    int    max_generations;
    int    local_interval;
    int    no_improve_limit;
    int    use_opt_crossover;
    double mut_prob;
    int    two_opt_iter;
} GAParams;

/* Varsayilan degerler (vrp_types.h ile ayni) */
static GAParams default_params(void) {
    GAParams p;
    p.pop_size          = GA_POP_SIZE;
    p.max_generations   = GA_MAX_GENERATIONS;
    p.local_interval    = GA_LOCAL_INTERVAL;
    p.no_improve_limit  = GA_NO_IMPROVE_LIMIT;
    p.use_opt_crossover = GA_USE_OPT_CROSSOVER;
    p.mut_prob          = MUTATION_OR_OPT_PROB;
    p.two_opt_iter      = TWO_OPT_MAX_ITER;
    return p;
}

/* ── Yardimci: dosya yolundan kisa problem adi cikar ─────────────────────── */
static void extract_problem_name(const char *filepath,
                                  char *out, int maxlen) {
    const char *slash = strrchr(filepath, '/');
    const char *base  = slash ? slash + 1 : filepath;
    const char *dot   = strrchr(base, '.');
    int len = dot ? (int)(dot - base) : (int)strlen(base);
    if (len >= maxlen) len = maxlen - 1;
    strncpy(out, base, (size_t)len);
    out[len] = '\0';
}

/* ── Normal mod: tek problem, N_RUNS kez calistir ───────────────────────── */
static void run_problem(const char *filepath,
                         int verbose,
                         const char *csv_path,
                         const GAParams *p) {
    static VRPInstance inst;

    char prob_name[64];
    extract_problem_name(filepath, prob_name, sizeof(prob_name));

    if (!parse_or_library(filepath, &inst)) {
        fprintf(stderr, "HATA: %s okunamadi, atlaniyor.\n", filepath);
        return;
    }
    build_distance_matrix(&inst);

    printf("\n  [%s]  %d musteri | %d arac | kapasite=%.0f | max_dist=%.0f\n",
           prob_name, inst.n_customers, inst.n_vehicles,
           inst.capacity, inst.max_distance);

    printf("  Parametreler: pop=%d  no_improve=%d  mut_prob=%.2f"
           "  crossover=%d  local_interval=%d  two_opt_iter=%d\n",
           p->pop_size, p->no_improve_limit, p->mut_prob,
           p->use_opt_crossover, p->local_interval, p->two_opt_iter);

    RunRecord records[N_RUNS];
    report_header(prob_name);

    for (int run = 0; run < N_RUNS; run++) {
        unsigned long long seed = BASE_SEED + (unsigned long long)run * SEED_STEP;

        GAResult res = run_ga(&inst,
                              p->pop_size,
                              p->max_generations,
                              p->local_interval,
                              p->no_improve_limit,
                              p->use_opt_crossover,
                              p->mut_prob,
                              p->two_opt_iter,
                              seed,
                              verbose);

        records[run].run_id          = run + 1;
        records[run].seed            = seed;
        records[run].distance        = res.best_distance;
        records[run].unfitness       = res.best_unfitness;
        records[run].generations_run = res.generations_run;
        records[run].cpu_seconds     = res.cpu_seconds;
        records[run].feasible        = (res.best_unfitness == 0.0) ? 1 : 0;

        report_run(&records[run]);
        fflush(stdout);
    }

    report_summary(records, N_RUNS, prob_name);

    if (csv_path)
        report_csv_append(csv_path, prob_name, records, N_RUNS);
}

/* ── irace modu: tek run, tek sayisal cikti ─────────────────────────────── */
static int run_irace_mode(int argc, char *argv[]) {
    /*
     * irace cagrisi:
     *   ./vrp_ga --irace <instance_id> <seed> <instance_file> [parametreler...]
     *
     * argv indeksleri:
     *   argv[0] = ./vrp_ga
     *   argv[1] = --irace
     *   argv[2] = instance_id   (irace tarafindan atanir, biz kullanmiyoruz)
     *   argv[3] = seed           (irace tarafindan atanir)
     *   argv[4] = instance_file  (problem dosyasi)
     *   argv[5..] = --param deger ciftleri
     */
    if (argc < 5) {
        fprintf(stderr,
            "irace modu kullanimi:\n"
            "  ./vrp_ga --irace <id> <seed> <instance> [--param deger ...]\n");
        return 1;
    }

    /* instance_id kullanilmiyor ama irace her zaman gonder */
    /* int instance_id = atoi(argv[2]); */
    unsigned long long seed          = (unsigned long long)atoll(argv[3]);
    const char        *instance_file = argv[4];

    /* Varsayilan parametrelerle basla */
    GAParams p = default_params();

    /* argv[5]'ten itibaren --anahtar deger ciftlerini oku */
    for (int a = 5; a < argc - 1; a += 2) {
        const char *key = argv[a];
        const char *val = argv[a + 1];

        if      (strcmp(key, "--pop")            == 0) p.pop_size          = atoi(val);
        else if (strcmp(key, "--no-improve")     == 0) p.no_improve_limit  = atoi(val);
        else if (strcmp(key, "--mut-prob")       == 0) p.mut_prob          = atof(val);
        else if (strcmp(key, "--crossover")      == 0) p.use_opt_crossover = atoi(val);
        else if (strcmp(key, "--local-interval") == 0) p.local_interval    = atoi(val);
        else if (strcmp(key, "--two-opt-iter")   == 0) p.two_opt_iter      = atoi(val);
        else {
            fprintf(stderr, "Bilinmeyen parametre: %s\n", key);
        }
    }

    /* Instance dosyasini yukle */
    static VRPInstance inst;
    if (!parse_or_library(instance_file, &inst)) {
        fprintf(stderr, "HATA: %s okunamadi.\n", instance_file);
        return 1;
    }
    build_distance_matrix(&inst);

    /* Tek run */
    GAResult res = run_ga(&inst,
                          p.pop_size,
                          p.max_generations,
                          p.local_interval,
                          p.no_improve_limit,
                          p.use_opt_crossover,
                          p.mut_prob,
                          p.two_opt_iter,
                          seed,
                          0 /* verbose kapali */);

    /*
     * irace tek bir sayisal deger bekler (kucuk = iyi).
     * Feasible ise mesafeyi, degilse agir penalti ekle.
     */
    double cost;
    if (res.best_unfitness == 0.0) {
        cost = res.best_distance;
    } else {
        cost = res.best_distance + 1e6 * res.best_unfitness;
    }

    printf("%.6f\n", cost);
    return 0;
}

/* ── Parametre yardim mesaji ─────────────────────────────────────────────── */
static void print_usage(const char *prog) {
    printf("\nKullanim:\n");
    printf("  %s                          → tum 14 problem (varsayilan)\n", prog);
    printf("  %s <dosya.txt>              → tek problem, 30 run\n", prog);
    printf("  %s <dosya.txt> -v           → tek problem + verbose\n", prog);
    printf("\nParametre gecersayma (normal mod):\n");
    printf("  --pop            <int>    Populasyon buyuklugu     (varsayilan: %d)\n", GA_POP_SIZE);
    printf("  --no-improve     <int>    Erken durdurma esigi     (varsayilan: %d)\n", GA_NO_IMPROVE_LIMIT);
    printf("  --mut-prob       <float>  Or-opt mutasyon olasiligi(varsayilan: %.2f)\n", MUTATION_OR_OPT_PROB);
    printf("  --crossover      <0|1>    Crossover tipi           (varsayilan: %d)\n", GA_USE_OPT_CROSSOVER);
    printf("  --local-interval <int>    Lambda-interchange period(varsayilan: %d)\n", GA_LOCAL_INTERVAL);
    printf("  --two-opt-iter   <int>    2-opt max iterasyon      (varsayilan: %d)\n", TWO_OPT_MAX_ITER);
    printf("\nirace modu:\n");
    printf("  %s --irace <id> <seed> <dosya.txt> [parametreler...]\n\n", prog);
}

/* ── main ─────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {

    /* irace modu: ilk arguman --irace ise hemen irace akisina gec */
    if (argc >= 2 && strcmp(argv[1], "--irace") == 0)
        return run_irace_mode(argc, argv);

    /* Yardim */
    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 ||
                      strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }

    /* ── Normal mod ──────────────────────────────────────────────────────── */
    printf("\n");
    for (int i = 0; i < 82; i++) printf("=");
    printf("\n  VRP Genetic Algorithm — Baker & Ayechew (2003)\n");
    printf("  + Optimised Crossover  [Nazif & Lee, 2012]\n");
    printf("  + 2-opt Local Search   [Lin, 1965]\n");
    printf("  Her problem icin %d BAGIMSIZ calistirma\n", N_RUNS);
    for (int i = 0; i < 82; i++) printf("=");
    printf("\n");

    printf("\n  Seed stratejisi: seed(run i) = %llu + i x %llu\n",
           BASE_SEED, SEED_STEP);
    printf("  Ornek: Run1=%llu  Run2=%llu  Run3=%llu  ...\n\n",
           BASE_SEED, BASE_SEED + SEED_STEP, BASE_SEED + 2 * SEED_STEP);

    /* Varsayilan parametrelerle basla */
    GAParams params = default_params();
    int      verbose   = 0;
    int      has_file  = 0;

    /* CSV hazirla */
    const char *csv_path = "results/vrp_results.csv";
    report_csv_header(csv_path);
    printf("  CSV cikti: %s\n", csv_path);

    /* argv tarama: once parametreleri topla, sonra dosyalari isle */
    const char *file_args[64];
    int         file_count = 0;

    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "-v") == 0 ||
            strcmp(argv[a], "--verbose") == 0) {
            verbose = 1;

        } else if (strcmp(argv[a], "--pop") == 0 && a + 1 < argc) {
            params.pop_size = atoi(argv[++a]);

        } else if (strcmp(argv[a], "--no-improve") == 0 && a + 1 < argc) {
            params.no_improve_limit = atoi(argv[++a]);

        } else if (strcmp(argv[a], "--mut-prob") == 0 && a + 1 < argc) {
            params.mut_prob = atof(argv[++a]);

        } else if (strcmp(argv[a], "--crossover") == 0 && a + 1 < argc) {
            params.use_opt_crossover = atoi(argv[++a]);

        } else if (strcmp(argv[a], "--local-interval") == 0 && a + 1 < argc) {
            params.local_interval = atoi(argv[++a]);

        } else if (strcmp(argv[a], "--two-opt-iter") == 0 && a + 1 < argc) {
            params.two_opt_iter = atoi(argv[++a]);

        } else if (argv[a][0] != '-') {
            /* - ile baslamayan arguman = dosya yolu */
            if (file_count < 64)
                file_args[file_count++] = argv[a];
            has_file = 1;
        }
    }

    if (!has_file) {
        /* Dosya verilmedi → tum varsayilan problemler */
        printf("  Mod: Tum problemler (vrpnc1 – vrpnc14)\n");
        for (int i = 0; DEFAULT_PROBLEMS[i] != NULL; i++)
            run_problem(DEFAULT_PROBLEMS[i], verbose, csv_path, &params);
    } else {
        for (int i = 0; i < file_count; i++)
            run_problem(file_args[i], verbose, csv_path, &params);
    }

    /* Kapanis */
    printf("\n");
    for (int i = 0; i < 82; i++) printf("=");
    printf("\n  Tum calistirmalar tamamlandi.\n");
    printf("  Sonuclar: %s\n", csv_path);
    for (int i = 0; i < 82; i++) printf("=");
    printf("\n\n");

    return 0;
}
/* =============================================================================
 *  vrp_ga.c : Ana Genetik Algoritma dongusu
 *
 *  Referans: Baker & Ayechew (2003)
 *
 *  Bu dosya yalnizca run_ga() fonksiyonunu icerir.
 *  Tum diger GA bilesenleri ayri modullerde:
 *    vrp_population.c  -> kromozon cozumleme, baslangic populasyonu
 *    vrp_crossover.c   -> 2-nokta ve optimised crossover
 *    vrp_mutation.c    -> swap ve or-opt mutasyon
 *    vrp_local.c       -> 2-opt, reassign, lambda-interchange, replacement
 *
 *  run_ga() akisi:
 *    1. Baslangic populasyonu olustur
 *    2. Lokal arama ile populasyonu iyilestir
 *    3. Her nesilde:
 *       a. Binary tournament ile ebeveyn sec
 *       b. Crossover (optimised veya 2-nokta)
 *       c. Mutasyon (or-opt veya swap)
 *       d. Infeasibility duzeltme
 *       e. Daha iyi ofspringe ranking replacement uygula
 *    4. Periyodik lambda-interchange
 *    5. Iyilesme olmazsa erken dur
 * =============================================================================*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vrp_ga.h"

GAResult run_ga(const VRPInstance *inst,
                int    pop_size,
                int    max_generations,
                int    local_search_interval,
                int    no_improve_limit,
                int    use_optimised_crossover,
                double mut_prob,
                int    two_opt_iter,
                unsigned long long seed,
                int    verbose) {

    RNG rng;
    rng_seed(&rng, seed);

    int n = inst->n_customers;

    static Chromosome population[MAX_POP];
    static FitPair    fitnesses[MAX_POP];

    /* 1. Baslangic populasyonu */
    generate_initial_population(inst, pop_size, &rng, population);
    for (int i = 0; i < pop_size; i++)
        fitnesses[i] = decode_chromosome(population[i], inst, two_opt_iter);

    /* 2. Baslangic lokal aramasi */
    for (int i = 0; i < pop_size; i++) {
        for (int r = 0; r < 5; r++)
            reassign_infeasible(population[i], inst, &rng, two_opt_iter);
        lambda_interchange(population[i], inst, two_opt_iter);
        fitnesses[i] = decode_chromosome(population[i], inst, two_opt_iter);
    }

    /* En iyi bireyi belirle */
    int best_idx = 0;
    for (int i = 1; i < pop_size; i++) {
        if (fitnesses[i].unfitness < fitnesses[best_idx].unfitness ||
            (fitnesses[i].unfitness == fitnesses[best_idx].unfitness &&
             fitnesses[i].fitness   < fitnesses[best_idx].fitness))
            best_idx = i;
    }
    double best_f = fitnesses[best_idx].fitness;
    double best_u = fitnesses[best_idx].unfitness;

    static Chromosome best_chrom;
    memcpy(best_chrom, population[best_idx], sizeof(int) * n);

    /* Verbose baslik */
    if (verbose) {
        const char *cx_name = use_optimised_crossover
            ? "Optimised Crossover [Nazif & Lee, 2012] + 2-opt"
            : "2-Point Crossover   [Baker & Ayechew, 2003] + 2-opt";
        printf("\n%s\n", "============================================================");
        printf("  Crossover    : %s\n", cx_name);
        printf("  Pop: %d  |  Max nesil: %d  |  Seed: %llu\n",
               pop_size, max_generations, seed);
        printf("  mut_prob: %.2f  |  two_opt_iter: %d  |  local_interval: %d\n",
               mut_prob, two_opt_iter, local_search_interval);
        printf("%s\n", "============================================================");
        printf("  %8s  %14s  %10s  %8s\n",
               "Nesil", "En iyi mesafe", "Unfitness", "Sure(s)");
        printf("  %s\n", "----------------------------------------------");
    }

    int     no_improve = 0;
    int     log_every  = (max_generations / 10 > 0) ? max_generations / 10 : 1;
    clock_t t_start    = clock();
    int     gen;

    static Chromosome c1_buf, c2_buf;

    /* 3. Ana evrim dongusu */
    for (gen = 1; gen <= max_generations; gen++) {

        /* a. Secim */
        int i1 = binary_tournament(fitnesses, pop_size, &rng);
        int i2 = binary_tournament(fitnesses, pop_size, &rng);

        /* b. Crossover */
        if (use_optimised_crossover)
            optimised_crossover(population[i1], population[i2],
                                n, inst, &rng, two_opt_iter, c1_buf, c2_buf);
        else
            two_point_crossover(population[i1], population[i2],
                                n, &rng, c1_buf, c2_buf);

        /* c. Mutasyon */
        mutate(c1_buf, n, inst, &rng, mut_prob, two_opt_iter, c1_buf);
        mutate(c2_buf, n, inst, &rng, mut_prob, two_opt_iter, c2_buf);

        /* d. Infeasibility duzeltme */
        reassign_infeasible(c1_buf, inst, &rng, two_opt_iter);
        reassign_infeasible(c2_buf, inst, &rng, two_opt_iter);

        /* Daha iyi ofspringe sec */
        FitPair fp1 = decode_chromosome(c1_buf, inst, two_opt_iter);
        FitPair fp2 = decode_chromosome(c2_buf, inst, two_opt_iter);
        Chromosome *child;
        FitPair     child_fit;
        if (fp1.unfitness < fp2.unfitness ||
            (fp1.unfitness == fp2.unfitness && fp1.fitness <= fp2.fitness)) {
            child = &c1_buf; child_fit = fp1;
        } else {
            child = &c2_buf; child_fit = fp2;
        }

        /* e. Replacement */
        int entered = ranking_replacement(population, fitnesses, pop_size,
                                          *child, child_fit);

        /* En iyi guncelle */
        if (entered && (child_fit.unfitness < best_u ||
            (child_fit.unfitness == best_u && child_fit.fitness < best_f))) {
            best_f = child_fit.fitness;
            best_u = child_fit.unfitness;
            memcpy(best_chrom, *child, sizeof(int) * n);
            no_improve = 0;
        } else {
            no_improve++;
        }

        /* 4. Periyodik lambda-interchange */
        if (gen % local_search_interval == 0) {
            for (int i = 0; i < pop_size; i++) {
                lambda_interchange(population[i], inst, two_opt_iter);
                fitnesses[i] = decode_chromosome(population[i], inst, two_opt_iter);
                if (fitnesses[i].unfitness < best_u ||
                    (fitnesses[i].unfitness == best_u &&
                     fitnesses[i].fitness   < best_f)) {
                    best_f = fitnesses[i].fitness;
                    best_u = fitnesses[i].unfitness;
                    memcpy(best_chrom, population[i], sizeof(int) * n);
                    no_improve = 0;
                }
            }
        }

        /* Verbose loglama */
        if (verbose && gen % log_every == 0) {
            double elapsed = (double)(clock() - t_start) / CLOCKS_PER_SEC;
            printf("  %8d  %14.2f  %10.4f  %8.1f  %s\n",
                   gen, best_f, best_u, elapsed,
                   best_u == 0.0 ? "OK" : "--");
        }

        /* 5. Erken durdurma */
        if (no_improve >= no_improve_limit) {
            if (verbose)
                printf("\n  [Erken durdurma: %d nesil iyilesme yok]\n",
                       no_improve_limit);
            break;
        }
    }

    double cpu_sec = (double)(clock() - t_start) / CLOCKS_PER_SEC;

    if (verbose) {
        printf("\n%s\n", "============================================================");
        printf("  SONUC  |  Mesafe: %.4f  |  Unfitness: %.4f  |  Sure: %.1fs\n",
               best_f, best_u, cpu_sec);
        printf("%s\n\n", "============================================================");
    }

    GAResult res;
    res.best_distance   = best_f;
    res.best_unfitness  = best_u;
    res.generations_run = gen - 1;
    res.cpu_seconds     = cpu_sec;
    memcpy(res.best_chrom, best_chrom, sizeof(int) * n);
    return res;
}
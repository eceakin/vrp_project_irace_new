# Optimised Crossover in a Hybrid Genetic Algorithm for the CVRP

A C11 implementation of a hybrid Genetic Algorithm (GA) for the **Capacitated Vehicle Routing Problem (CVRP)**, built on the framework of Baker & Ayechew (2003) and extended with the **Optimised Crossover (OX)** of Nazif & Lee (2012) and an **Or-opt mutation** operator. Hyperparameters are tuned automatically with [**irace**](https://mlopez-ibanez.github.io/irace/), and the algorithm is evaluated on the 14 classic OR-Library benchmark instances (`vrpnc1`–`vrpnc14`).

This repository accompanies the paper:

> **Optimised Crossover Operator in a Genetic Algorithm for the Capacitated Vehicle Routing Problem**
> Ece Akın, Betül Sarı — İzmir Katip Çelebi Üniversitesi

---

## Contents

- [Overview](#overview)
- [Algorithm](#algorithm)
- [Repository structure](#repository-structure)
- [Getting started](#getting-started)
- [Usage](#usage)
- [Parameter tuning with irace](#parameter-tuning-with-irace)
- [Analysing results](#analysing-results)
- [Results](#results)
- [Limitations and future work](#limitations-and-future-work)
- [References](#references)

---

## Overview

In the CVRP, a fleet of identical vehicles with capacity *Q* leaves a single depot to serve customers with known demands. Every customer must be visited exactly once, no route may exceed the vehicle capacity, and the goal is to minimise the total distance travelled.

Baker & Ayechew's hybrid GA reaches results within 0.50% of best-known values, but its two-point crossover picks cut points at random and has no notion of which routes are expensive. This project asks a focused question: **does directing recombination at the costliest route improve that framework?**

To keep the comparison clean, only the crossover and mutation operators are changed; every other component follows the original design.

**Contributions of this work**

- **Optimised Crossover (OX)** integrated as a drop-in replacement for two-point crossover
- **Or-opt mutation** (segment relocation, *s* ∈ {1, 2, 3}) alongside swap mutation
- **xorshift64** random number generator with deterministic per-run seeds for full reproducibility
- **Nearest Neighbour** heuristic for intra-route ordering
- Automated hyperparameter tuning with **irace** and a statistical comparison (Wilcoxon signed-rank test) against default settings

## Algorithm

Each solution is a **vehicle-indexed chromosome** of length *n*: gene *i* holds the vehicle assigned to customer *i*. The visit order inside each route is not stored; it is rebuilt with Nearest Neighbour and refined with 2-opt.

```
Initialise RNG (seed = BASE_SEED + r × 97) and pre-compute distance matrix
Build initial population with sweep heuristic → NN ordering → 2-opt + λ-interchange
repeat
    Select two parents by binary tournament
    Crossover: two-point (baseline) or OX (worst route of P1, centroid-aligned P2)
    Mutation:  swap + Or-opt
    Repair infeasible offspring with greedy reassignment
    Insert offspring via ranking replacement
    Every `local_interval` generations: 2-opt + λ-interchange on whole population
until no improvement for `no_improve` consecutive offspring
return best feasible solution
```

| Component | Origin |
|---|---|
| xorshift64 RNG | This work |
| Euclidean distance matrix | This work |
| Sweep initialisation | Baker & Ayechew [1] |
| Nearest Neighbour routing | This work |
| Binary tournament selection | Baker & Ayechew [1] |
| Two-point crossover (baseline) | Baker & Ayechew [1] |
| Optimised Crossover (OX) | Nazif & Lee [2], integrated here |
| Swap mutation | Baker & Ayechew [1] |
| Or-opt mutation | This work |
| Ranking replacement | Baker & Ayechew [1] |
| Reassignment procedure | Baker & Ayechew [1] |
| 2-opt local search | Baker & Ayechew [1] |
| λ-interchange (λ = 1) | Baker & Ayechew [1] |

Fitness (total distance) and unfitness (total capacity violation) are kept as two separate values: a feasible solution always beats an infeasible one, feasible solutions are compared by distance, and infeasible ones by unfitness.

## Repository structure

```
.
├── src/                    # C source files
│   ├── main.c              # Entry point: batch mode and irace mode
│   ├── vrp_ga.c            # Main GA loop
│   ├── vrp_population.c    # Sweep initialisation, selection, ranking replacement
│   ├── vrp_crossover.c     # Two-point crossover and Optimised Crossover (OX)
│   ├── vrp_mutation.c      # Swap and Or-opt mutation
│   ├── vrp_local.c         # 2-opt, λ-interchange, reassignment
│   ├── vrp_instance.c      # OR-Library parser and distance matrix
│   ├── vrp_rng.c           # xorshift64 RNG
│   └── vrp_report.c        # Console tables and CSV output
├── include/                # Headers; default parameters live in vrp_types.h
├── data/                   # OR-Library instances vrpnc1–vrpnc14
├── results/                # Per-run CSV outputs (default and irace-tuned)
├── scenario.txt            # irace scenario
├── parameters.txt          # irace parameter space
├── instances.txt           # irace training instances
├── target-runner           # Optional wrapper script for irace
├── analyze.py              # Statistics and figures from the CSV results
├── comparison_table.csv    # Default vs. irace comparison with Wilcoxon p-values
└── Makefile
```

## Getting started

**Requirements**

- GCC with C11 support (MinGW-w64 on Windows)
- `make` (`mingw32-make` on Windows)
- *Optional:* R with the `irace` package for tuning
- *Optional:* Python 3 with `numpy`, `pandas`, `matplotlib`, `seaborn` and `scipy` for analysis

**Build**

```bash
# Linux / macOS
make

# Windows (MinGW)
mingw32-make
```

This produces `vrp_ga` (or `vrp_ga.exe` on Windows). Run `make clean` to remove build files.

## Usage

### Batch mode

Each instance is solved in **30 independent runs** with seeds `1000 + i × 97`, so every run is reproducible.

```bash
./vrp_ga                         # all 14 instances
./vrp_ga data/vrpnc1.txt         # a single instance
./vrp_ga data/vrpnc1.txt -v      # single instance with verbose log
./vrp_ga --help                  # list options and defaults
```

Shortcuts are also available: `make run` (all instances) and `make run1` (only `vrpnc1`).

Results are printed as a table per instance and written to `results/vrp_results.csv` with the columns:

```
problem, run_id, seed, distance, unfitness, generations_run, cpu_seconds, feasible
```

### Parameters

Any default can be overridden on the command line:

| Flag | Description | Default | irace-tuned |
|---|---|---|---|
| `--pop` | Population size | 30 | **87** |
| `--no-improve` | Early stopping: offspring without improvement | 5000 | **3000** |
| `--mut-prob` | Or-opt mutation probability | 0.60 | **0.47** |
| `--crossover` | `0` = two-point, `1` = Optimised Crossover | 1 | **0** |
| `--local-interval` | Generations between λ-interchange passes | 3000 | **2791** |
| `--two-opt-iter` | Max 2-opt iterations per call | 5 | **10** |

To reproduce the tuned configuration from the paper:

```bash
./vrp_ga --pop 87 --no-improve 3000 --mut-prob 0.47 --crossover 0 \
         --local-interval 2791 --two-opt-iter 10
```

Compile-time defaults (and limits such as `MAX_CUSTOMERS`) are defined in `include/vrp_types.h`.

### irace mode

In irace mode the program performs a single run and prints one number to stdout: the best feasible distance, or `distance + 1e6 × unfitness` if no feasible solution was found.

```bash
./vrp_ga --irace <instance_id> <seed> <instance_file> [--param value ...]

# example
./vrp_ga --irace 1 42 data/vrpnc1.txt --pop 87 --crossover 0
```

## Parameter tuning with irace

The tuning setup consists of three files:

- `parameters.txt`: the six parameters and their ranges
- `scenario.txt`: budget of 1,000 runs, seed 42, calls the binary directly via `targetCmdline`
- `instances.txt`: the training instances

```bash
# from the repository root, with irace installed in R
irace --scenario scenario.txt
```

`scenario.txt` points to `vrp_ga.exe`; on Linux/macOS change `targetRunner` to `"./vrp_ga"`. The bundled `irace.log` and `irace_output.txt` are the logs from the run reported in the paper.

## Analysing results

```bash
python analyze.py                          # reads results/vrp_results.csv
python analyze.py results/irace_results.csv
```

The script writes a summary table to `reports/summary_stats.csv` and eight figures to `figures/`: summary table, distance box plots, feasibility rate, CPU time, best vs. standard deviation, run-to-run variation, convergence proxy and a normalised distance heatmap.

## Results

Experiments: 30 runs per instance, Intel Core i5-10300H, 8 GB RAM, Windows 10, single-threaded.

**irace vs. default parameters.** The tuned configuration matches or beats the defaults on capacity-only instances, with statistically significant improvements (Wilcoxon, *p* < 0.05) on `vrpnc1`–`vrpnc4`, `vrpnc11` and `vrpnc12`. The largest mean gains are on `vrpnc11` (−32.97) and `vrpnc12` (−24.78).

**Best solutions on capacity-only instances**

| Instance | Best known | This work | Gap |
|---|---|---|---|
| vrpnc1 | 524.61 | 535.35 | +2.05% |
| vrpnc2 | 835.26 | 862.08 | +3.21% |
| vrpnc3 | 826.14 | 845.58 | +2.35% |
| vrpnc4 | 1028.42 | 1061.37 | +3.20% |
| vrpnc5 | 1291.45 | 1439.17 | +11.44% |
| vrpnc11 | 1042.11 | 1209.08 | +16.02% |
| vrpnc12 | 819.56 | 831.38 | +1.44% |
| **Average** | | | **5.67%** |

For reference, the reported average gaps are 0.68% for Baker & Ayechew, 0.09% for Nazif & Lee and 0.46% for Szeto et al. The larger gap here is mainly due to the tighter early-stopping threshold chosen by irace, which trades solution quality for speed.

**Key finding.** Although OX was introduced to concentrate search on the costliest route, irace selected **two-point crossover** (`crossover = 0`). Within this hybrid framework, two-point crossover appears to preserve population diversity better, while the more exploitative OX combined with aggressive λ-interchange may cause premature convergence.

Full per-run data is in `results/`, and the default vs. irace comparison is in `comparison_table.csv`.

## Limitations and future work

- **Route-length constraint:** on instances with a maximum route length (`vrpnc6`–`vrpnc10`, `vrpnc13`) the algorithm does not find feasible solutions; `vrpnc14` is feasible but far from best-known. Adding explicit route-length repair is the main next step.
- **Adaptive operator selection:** weighting OX and two-point crossover dynamically based on population diversity.
- **Problem variants:** extending to VRPTW or multi-depot VRP.
- **Scalability:** testing on larger benchmark sets (e.g. Uchoa et al.) and parallelising fitness evaluation and local search.

## References

1. B. M. Baker and M. A. Ayechew, "A genetic algorithm for the vehicle routing problem," *Computers & Operations Research*, 30(5), 787–800, 2003.
2. H. Nazif and L. S. Lee, "Optimised crossover genetic algorithm for capacitated vehicle routing problem," *Applied Mathematical Modelling*, 36(5), 2110–2117, 2012.
3. W. Y. Szeto, Y. Wu and S. C. Ho, "An artificial bee colony algorithm for the capacitated vehicle routing problem," *European Journal of Operational Research*, 215(1), 126–135, 2011.
4. M. López-Ibáñez, J. Dubois-Lacoste, L. Pérez Cáceres, M. Birattari and T. Stützle, "The irace package: Iterated racing for automatic algorithm configuration," *Operations Research Perspectives*, 3, 43–58, 2016.
5. N. Christofides, A. Mingozzi and P. Toth, "The vehicle routing problem," in *Combinatorial Optimization*, Wiley, 1979. (OR-Library instances `vrpnc1`–`vrpnc14`)

## Authors

- **Ece Akın** — [@eceakin](https://github.com/eceakin)
- **Betül Sarı** — [@betlsari](https://github.com/betlsari)

İzmir Katip Çelebi Üniversitesi, İzmir, Türkiye

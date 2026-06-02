"""
================================================================================
  VRP Genetic Algorithm — Python Analiz Scripti
  Baker & Ayechew (2003) + Nazif & Lee (2012) + 2-opt Local Search

  Kullanim:
    python analyze.py                        # results/vrp_results.csv okur
    python analyze.py results/benim.csv      # ozel dosya

  Cikti:
    figures/01_summary_table.png        — problem bazli ozet tablo
    figures/02_boxplot_distance.png     — mesafe dagilimi (box plot)
    figures/03_feasibility_rate.png     — feasibility oran grafigi
    figures/04_cpu_time.png             — CPU suresi karsilastirmasi
    figures/05_best_vs_std.png          — en iyi mesafe vs std sapma
    figures/06_run_variation.png        — run-run degiskenlik haritasi
    figures/07_convergence_proxy.png    — nesil sayisi (convergence proxy)
    figures/08_distance_heatmap.png     — mesafe normalize heatmap
    reports/summary_stats.csv          — tam istatistik tablosu (CSV)
================================================================================
"""

import sys
import os
import warnings
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")                 # ekransiz ortamda render
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import seaborn as sns
from scipy import stats

warnings.filterwarnings("ignore")

# ── Genel stil ayarlari ──────────────────────────────────────────────────────
plt.rcParams.update({
    "figure.dpi":        150,
    "figure.facecolor":  "white",
    "axes.facecolor":    "#f8f9fa",
    "axes.grid":         True,
    "grid.alpha":        0.4,
    "grid.linestyle":    "--",
    "font.family":       "DejaVu Sans",
    "axes.titlesize":    13,
    "axes.labelsize":    11,
    "xtick.labelsize":   9,
    "ytick.labelsize":   9,
    "legend.fontsize":   9,
    "legend.framealpha": 0.85,
})

# Renk paleti (renk körü dostu)
PALETTE = [
    "#2196F3", "#4CAF50", "#FF5722", "#9C27B0",
    "#FF9800", "#00BCD4", "#795548", "#607D8B",
    "#E91E63", "#3F51B5", "#009688", "#CDDC39",
    "#F44336", "#03A9F4",
]


# ════════════════════════════════════════════════════════════════════════════════
# 1. VERİ YUKLEME VE TEMIZLEME
# ════════════════════════════════════════════════════════════════════════════════

def load_data(csv_path: str) -> pd.DataFrame:
    """CSV'yi yukle, temel dogrulama yap, turetilmis sutunlari ekle."""
    if not os.path.exists(csv_path):
        sys.exit(f"[HATA] CSV dosyasi bulunamadi: {csv_path}")

    df = pd.read_csv(csv_path)
    required = {"problem", "run_id", "distance", "unfitness",
                "generations_run", "cpu_seconds", "feasible"}
    missing = required - set(df.columns)
    if missing:
        sys.exit(f"[HATA] CSV'de eksik sutunlar: {missing}")

    if df.empty:
        sys.exit("[HATA] CSV bos — once './vrp_ga' calistirip veri uretiniz.")

    # Problem sirasi: vrpnc1 … vrpnc14
    def _sort_key(name):
        digits = "".join(c for c in name if c.isdigit())
        return int(digits) if digits else 999

    ordered = sorted(df["problem"].unique(), key=_sort_key)
    df["problem"] = pd.Categorical(df["problem"], categories=ordered, ordered=True)
    df = df.sort_values(["problem", "run_id"]).reset_index(drop=True)

    # Turetilmis sutunlar
    df["is_feasible"] = df["feasible"].astype(bool)

    print(f"  Yuklendi : {csv_path}")
    print(f"  Problem  : {df['problem'].nunique()}  |  "
          f"Run/problem: {df.groupby('problem').size().iloc[0]}  |  "
          f"Toplam   : {len(df)} satir\n")
    return df


# ════════════════════════════════════════════════════════════════════════════════
# 2. ISTATISTIK TABLOSU
# ════════════════════════════════════════════════════════════════════════════════

def compute_stats(df: pd.DataFrame) -> pd.DataFrame:
    """Her problem icin tam istatistik tablosu."""
    records = []
    for prob, grp in df.groupby("problem", observed=True):
        feas   = grp[grp["is_feasible"]]
        n_runs = len(grp)

        row = {
            "Problem"         : prob,
            "N_Runs"          : n_runs,
            "Feasible"        : int(grp["is_feasible"].sum()),
            "Feasibility_%"   : round(grp["is_feasible"].mean() * 100, 1),
            # Tum runlar
            "All_Best"        : round(grp["distance"].min(), 4),
            "All_Worst"       : round(grp["distance"].max(), 4),
            "All_Mean"        : round(grp["distance"].mean(), 4),
            "All_Std"         : round(grp["distance"].std(ddof=1), 4),
            "All_CV_%"        : round(grp["distance"].std(ddof=1) /
                                      grp["distance"].mean() * 100, 2),
            # Yalnizca feasible
            "Feas_Best"       : round(feas["distance"].min(), 4) if len(feas) > 0 else float("nan"),
            "Feas_Mean"       : round(feas["distance"].mean(), 4) if len(feas) > 0 else float("nan"),
            "Feas_Std"        : round(feas["distance"].std(ddof=1), 4) if len(feas) > 1 else float("nan"),
            # Convergence proxy
            "Mean_Gens"       : round(grp["generations_run"].mean(), 1),
            "Min_Gens"        : int(grp["generations_run"].min()),
            "Max_Gens"        : int(grp["generations_run"].max()),
            # CPU
            "Mean_CPU_s"      : round(grp["cpu_seconds"].mean(), 2),
            "Total_CPU_s"     : round(grp["cpu_seconds"].sum(), 2),
        }
        records.append(row)

    return pd.DataFrame(records)


# ════════════════════════════════════════════════════════════════════════════════
# 3. GRAFIK FONKSİYONLARI
# ════════════════════════════════════════════════════════════════════════════════

def _save(fig, path: str):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    fig.savefig(path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Kaydedildi: {path}")


# ── 3.1 Ozet Tablo Gorseli ──────────────────────────────────────────────────
def plot_summary_table(stats_df: pd.DataFrame, out_path: str):
    cols = ["Problem", "N_Runs", "Feasibility_%",
            "All_Best", "All_Mean", "All_Std", "All_CV_%", "Mean_CPU_s"]
    tbl  = stats_df[cols].copy()
    tbl.columns = ["Problem", "Runs", "Feasible\n(%)",
                   "Best\nDist", "Mean\nDist", "Std\nDev", "CV\n(%)", "Mean\nCPU(s)"]

    fig, ax = plt.subplots(figsize=(14, max(4, len(tbl) * 0.55 + 1.5)))
    ax.axis("off")

    table = ax.table(
        cellText  = tbl.values,
        colLabels = tbl.columns,
        cellLoc   = "center",
        loc       = "center",
    )
    table.auto_set_font_size(False)
    table.set_fontsize(9)
    table.scale(1, 1.6)

    # Baslik satiri rengi
    for j in range(len(tbl.columns)):
        table[0, j].set_facecolor("#1565C0")
        table[0, j].set_text_props(color="white", fontweight="bold")

    # Satirlari sira rengi
    for i in range(1, len(tbl) + 1):
        color = "#E3F2FD" if i % 2 == 0 else "white"
        for j in range(len(tbl.columns)):
            table[i, j].set_facecolor(color)

    ax.set_title("VRP GA — Problem Bazli Ozet Istatistikler",
                 fontsize=13, fontweight="bold", pad=12)
    _save(fig, out_path)


# ── 3.2 Box Plot — Mesafe Dagilimi ──────────────────────────────────────────
def plot_boxplot_distance(df: pd.DataFrame, out_path: str):
    problems = list(df["problem"].cat.categories)
    n = len(problems)
    colors = PALETTE[:n]

    fig, ax = plt.subplots(figsize=(max(10, n * 1.1), 6))

    data_by_prob = [df[df["problem"] == p]["distance"].values for p in problems]

    bp = ax.boxplot(
        data_by_prob,
        patch_artist   = True,
        notch          = False,
        vert           = True,
        showfliers     = True,
        flierprops     = dict(marker="o", markersize=4, alpha=0.5),
        medianprops    = dict(color="black", linewidth=2),
        whiskerprops   = dict(linewidth=1.2),
        capprops       = dict(linewidth=1.2),
    )
    for patch, color in zip(bp["boxes"], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.75)

    ax.set_xticks(range(1, n + 1))
    ax.set_xticklabels(problems, rotation=35, ha="right")
    ax.set_xlabel("Problem")
    ax.set_ylabel("Toplam Rota Mesafesi")
    ax.set_title("Problem Bazli Mesafe Dagilimi (Box Plot)\n"
                 "30 Bagimsiz Calistirma")

    # Her kutunun altina min degerini yaz
    for i, vals in enumerate(data_by_prob, start=1):
        ax.text(i, min(vals) - (max(vals) - min(vals)) * 0.08,
                f"{min(vals):.1f}", ha="center", va="top",
                fontsize=7, color="#555555")

    _save(fig, out_path)


# ── 3.3 Feasibility Oran Grafigi ────────────────────────────────────────────
def plot_feasibility(stats_df: pd.DataFrame, out_path: str):
    problems = stats_df["Problem"].tolist()
    rates    = stats_df["Feasibility_%"].tolist()
    n = len(problems)

    fig, ax = plt.subplots(figsize=(max(10, n * 1.1), 5))

    bars = ax.bar(problems, rates,
                  color=[PALETTE[i % len(PALETTE)] for i in range(n)],
                  edgecolor="white", linewidth=0.8, alpha=0.85)

    # %100 cizgisi
    ax.axhline(100, color="#E53935", linestyle="--", linewidth=1.4,
               label="%100 Feasible")

    # Deger etiketleri
    for bar, rate in zip(bars, rates):
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 1.5,
                f"{rate:.0f}%",
                ha="center", va="bottom", fontsize=8, fontweight="bold")

    ax.set_ylim(0, 115)
    ax.set_xlabel("Problem")
    ax.set_ylabel("Feasible Oran (%)")
    ax.set_title("Problem Bazli Feasibility Orani\n(30 Bagimsiz Calistirma)")
    ax.set_xticks(range(n))
    ax.set_xticklabels(problems, rotation=35, ha="right")
    ax.legend()

    _save(fig, out_path)


# ── 3.4 CPU Suresi ──────────────────────────────────────────────────────────
def plot_cpu_time(stats_df: pd.DataFrame, out_path: str):
    problems = stats_df["Problem"].tolist()
    means    = stats_df["Mean_CPU_s"].tolist()
    totals   = stats_df["Total_CPU_s"].tolist()
    n        = len(problems)

    x   = np.arange(n)
    w   = 0.38

    fig, ax1 = plt.subplots(figsize=(max(10, n * 1.1), 5))
    ax2 = ax1.twinx()

    b1 = ax1.bar(x - w / 2, means,  w, label="Ortalama CPU (s)",
                 color="#42A5F5", alpha=0.85, edgecolor="white")
    b2 = ax2.bar(x + w / 2, totals, w, label="Toplam CPU (s)",
                 color="#FF7043", alpha=0.85, edgecolor="white")

    ax1.set_ylabel("Ortalama CPU / Run (sn)", color="#1565C0")
    ax2.set_ylabel("Toplam CPU 30 Run (sn)",  color="#BF360C")
    ax1.tick_params(axis="y", labelcolor="#1565C0")
    ax2.tick_params(axis="y", labelcolor="#BF360C")

    ax1.set_xticks(x)
    ax1.set_xticklabels(problems, rotation=35, ha="right")
    ax1.set_xlabel("Problem")
    ax1.set_title("Problem Bazli CPU Suresi Analizi")

    lines = [b1, b2]
    labels = [b.get_label() for b in lines]
    ax1.legend(lines, labels, loc="upper left")

    _save(fig, out_path)


# ── 3.5 En Iyi Mesafe vs Std Sapma (scatter) ────────────────────────────────
def plot_best_vs_std(stats_df: pd.DataFrame, out_path: str):
    fig, ax = plt.subplots(figsize=(8, 6))

    for i, row in stats_df.iterrows():
        color = PALETTE[i % len(PALETTE)]
        ax.scatter(row["All_Best"], row["All_Std"],
                   s=120, color=color, zorder=3,
                   edgecolors="white", linewidths=0.8)
        ax.annotate(row["Problem"],
                    (row["All_Best"], row["All_Std"]),
                    textcoords="offset points", xytext=(6, 4),
                    fontsize=8, color=color)

    ax.set_xlabel("En Iyi Mesafe (Best Distance)")
    ax.set_ylabel("Standart Sapma (Std Dev)")
    ax.set_title("En Iyi Mesafe vs Standart Sapma\n"
                 "(Sol-alt kose: iyi sonuc + tutarli algoritma)")

    # Referans cizgileri
    ax.axvline(stats_df["All_Best"].median(),
               color="gray", linestyle=":", linewidth=1, alpha=0.6)
    ax.axhline(stats_df["All_Std"].median(),
               color="gray", linestyle=":", linewidth=1, alpha=0.6)

    _save(fig, out_path)


# ── 3.6 Run-Run Degiskenlik Haritasi (heatmap) ──────────────────────────────
def plot_run_variation(df: pd.DataFrame, out_path: str):
    """Her (problem, run) icin normalize mesafe heatmap'i."""
    problems = list(df["problem"].cat.categories)
    n_runs   = df.groupby("problem", observed=True).size().max()

    matrix = np.full((len(problems), n_runs), np.nan)
    for i, prob in enumerate(problems):
        grp = df[df["problem"] == prob].sort_values("run_id")
        vals = grp["distance"].values
        if len(vals) > 0:
            mn, mx = vals.min(), vals.max()
            if mx > mn:
                matrix[i, :len(vals)] = (vals - mn) / (mx - mn)
            else:
                matrix[i, :len(vals)] = 0.0

    fig, ax = plt.subplots(figsize=(max(12, n_runs * 0.45), max(5, len(problems) * 0.6)))
    sns.heatmap(
        matrix,
        ax         = ax,
        cmap       = "RdYlGn_r",
        vmin       = 0, vmax=1,
        xticklabels= [f"R{i+1}" for i in range(n_runs)],
        yticklabels= problems,
        linewidths = 0.3,
        linecolor  = "white",
        cbar_kws   = {"label": "Normalize Mesafe (0=en iyi, 1=en kotu)"},
    )
    ax.set_xlabel("Run Numarasi")
    ax.set_ylabel("Problem")
    ax.set_title("Run-Run Mesafe Degiskenligi\n"
                 "(Her satir kendi icinde normalize edilmistir)")
    ax.tick_params(axis="x", labelsize=7, rotation=90)

    _save(fig, out_path)


# ── 3.7 Convergence Proxy (Nesil Sayisi) ────────────────────────────────────
def plot_convergence_proxy(df: pd.DataFrame, out_path: str):
    """
    Dogrudan convergence egrisi kaydetmedigimiz icin,
    'kac nesilde duruldu?' sorusunu gorsellestirir.
    Az nesil = hizli buldu = iyi convergence.
    """
    problems = list(df["problem"].cat.categories)
    n = len(problems)

    fig, ax = plt.subplots(figsize=(max(10, n * 1.1), 5))

    data_by_prob = [
        df[df["problem"] == p]["generations_run"].values
        for p in problems
    ]

    bp = ax.boxplot(
        data_by_prob,
        patch_artist=True,
        notch=False,
        medianprops=dict(color="black", linewidth=2),
    )
    colors = PALETTE[:n]
    for patch, color in zip(bp["boxes"], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.75)

    ax.set_xticks(range(1, n + 1))
    ax.set_xticklabels(problems, rotation=35, ha="right")
    ax.set_xlabel("Problem")
    ax.set_ylabel("Durdugumdaki Nesil Sayisi")
    ax.set_title("Convergence Proxy: Erken Durdurma Nesil Sayisi\n"
                 "(Dusuk = algoritma daha hizli yakinlasti)")

    _save(fig, out_path)


# ── 3.8 Normalize Mesafe Heatmap (ozet) ─────────────────────────────────────
def plot_distance_heatmap(stats_df: pd.DataFrame, out_path: str):
    """
    Best / Mean / Std / CV degerlerini problem bazli normalize heatmap.
    Farkli olceklerdeki problemleri karsilastirmayi kolaylastirir.
    """
    metrics = ["All_Best", "All_Mean", "All_Std", "All_CV_%", "Mean_CPU_s"]
    labels  = ["Best\nDist", "Mean\nDist", "Std\nDev", "CV\n(%)", "Mean\nCPU"]

    matrix = stats_df[metrics].values.astype(float)

    # Her sutunu [0,1] araligina normalize et (min-max)
    norm = np.zeros_like(matrix)
    for j in range(matrix.shape[1]):
        col = matrix[:, j]
        mn, mx = col.min(), col.max()
        norm[:, j] = (col - mn) / (mx - mn) if mx > mn else 0.0

    fig, ax = plt.subplots(figsize=(9, max(5, len(stats_df) * 0.6)))
    sns.heatmap(
        norm,
        ax          = ax,
        cmap        = "RdYlGn_r",
        vmin=0, vmax=1,
        xticklabels = labels,
        yticklabels = stats_df["Problem"].tolist(),
        annot       = True,
        fmt         = ".2f",
        linewidths  = 0.4,
        linecolor   = "white",
        cbar_kws    = {"label": "Normalize Deger (0=en iyi, 1=en kotu)"},
        annot_kws   = {"size": 8},
    )
    ax.set_title("Problem Bazli Metrik Karsilastirmasi (Normalize)\n"
                 "Her sutun kendi icinde normalize edilmistir")
    ax.tick_params(axis="y", labelsize=9)

    _save(fig, out_path)


# ════════════════════════════════════════════════════════════════════════════════
# 4. EKSTRA: ISTATISTIKSEL TESTLER
# ════════════════════════════════════════════════════════════════════════════════

def print_statistical_notes(df: pd.DataFrame, stats_df: pd.DataFrame):
    """
    Makale icin notlar:
      - Normallik (Shapiro-Wilk)
      - Varyasyon katsayisi
    """
    print("\n" + "=" * 70)
    print("  ISTATISTIKSEL NOTLAR  (Makale icin)")
    print("=" * 70)

    for _, row in stats_df.iterrows():
        prob = row["Problem"]
        grp  = df[df["problem"] == prob]["distance"].values

        sw_stat, sw_p = stats.shapiro(grp) if len(grp) >= 3 else (float("nan"), float("nan"))
        normal = "NORMAL" if sw_p > 0.05 else "NORMAL DEGIL"

        print(f"\n  {prob:12s}  |  "
              f"Best={row['All_Best']:.2f}  "
              f"Mean={row['All_Mean']:.2f}  "
              f"Std={row['All_Std']:.2f}  "
              f"CV={row['All_CV_%']:.1f}%  |  "
              f"Shapiro-Wilk p={sw_p:.3f} → {normal}")

    print()


# ════════════════════════════════════════════════════════════════════════════════
# 5. ANA AKIS
# ════════════════════════════════════════════════════════════════════════════════

def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "results/vrp_results.csv"

    print("\n" + "=" * 70)
    print("  VRP GA — Python Analiz Scripti")
    print("=" * 70 + "\n")

    # Veri yukle
    df = load_data(csv_path)

    # Istatistik tablosu
    stats_df = compute_stats(df)

    # Rapor klasoru
    os.makedirs("reports",  exist_ok=True)
    os.makedirs("figures",  exist_ok=True)

    # CSV olarak kaydet
    stats_path = "reports/summary_stats.csv"
    stats_df.to_csv(stats_path, index=False)
    print(f"  Istatistik tablosu: {stats_path}\n")

    # ── Grafikleri uret ──────────────────────────────────────────────────────
    print("  Grafikler uretiliyor...")

    plot_summary_table(
        stats_df,
        "figures/01_summary_table.png"
    )
    plot_boxplot_distance(
        df,
        "figures/02_boxplot_distance.png"
    )
    plot_feasibility(
        stats_df,
        "figures/03_feasibility_rate.png"
    )
    plot_cpu_time(
        stats_df,
        "figures/04_cpu_time.png"
    )
    plot_best_vs_std(
        stats_df,
        "figures/05_best_vs_std.png"
    )
    plot_run_variation(
        df,
        "figures/06_run_variation.png"
    )
    plot_convergence_proxy(
        df,
        "figures/07_convergence_proxy.png"
    )
    plot_distance_heatmap(
        stats_df,
        "figures/08_distance_heatmap.png"
    )

    # Istatistiksel notlar
    print_statistical_notes(df, stats_df)

    # ── Ozet yazdir ─────────────────────────────────────────────────────────
    print("=" * 70)
    print("  OZET")
    print("=" * 70)
    print(f"  Toplam problem      : {stats_df.shape[0]}")
    feas_100 = (stats_df["Feasibility_%"] == 100).sum()
    print(f"  %100 Feasible prob  : {feas_100} / {stats_df.shape[0]}")
    print(f"  Genel mean mesafe   : {stats_df['All_Mean'].mean():.4f}")
    print(f"  Genel mean CPU/run  : {stats_df['Mean_CPU_s'].mean():.2f} sn")
    print(f"\n  Tum grafikler       : figures/")
    print(f"  Istatistik CSV      : {stats_path}")
    print("=" * 70 + "\n")


if __name__ == "__main__":
    main()

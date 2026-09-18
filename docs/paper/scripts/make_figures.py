"""Figures and LaTeX tables for the TV denoising paper.

Reads the CSV files and PNG images written by experiments/PaperExperiments
and writes vector PDF figures and table bodies to ../figures.

    python make_figures.py
"""
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "data"
FIG = ROOT / "figures"
FIG.mkdir(exist_ok=True)

COL = 3.45
WIDE = 7.0

GD = "#DC143C"       # application colours: Crimson
NEWTON = "#008080"   #                     Teal
GRAY = "#7f7f7f"
IMAGES = ["cameraman", "barbara", "ape"]
TITLES = {"cameraman": "Cameraman", "barbara": "Barbara", "ape": "Ape"}
STYLES = {"cameraman": "-", "barbara": "--", "ape": ":"}
MARK = {"cameraman": "o", "barbara": "s", "ape": "^"}

plt.rcParams.update({
    "font.family": "serif",
    "font.serif": ["Times New Roman", "Times", "DejaVu Serif"],
    "mathtext.fontset": "stix",
    "font.size": 8,
    "axes.titlesize": 8,
    "axes.labelsize": 8,
    "legend.fontsize": 6.5,
    "xtick.labelsize": 7,
    "ytick.labelsize": 7,
    "lines.linewidth": 1.1,
    "lines.markersize": 3.0,
    "axes.grid": True,
    "grid.linewidth": 0.3,
    "grid.alpha": 0.5,
    "legend.frameon": False,
    "savefig.bbox": "tight",
    "savefig.pad_inches": 0.02,
    "pdf.fonttype": 42,
})

conv = pd.read_csv(DATA / "convergence.csv")
summary = pd.read_csv(DATA / "comparison_summary.csv")
lam = pd.read_csv(DATA / "lambda_study.csv")
eps = pd.read_csv(DATA / "epsilon_study.csv")
sigma = pd.read_csv(DATA / "sigma_study.csv")
scaling = pd.read_csv(DATA / "scaling.csv")


def save(fig, name):
    fig.savefig(FIG / f"{name}.pdf", dpi=300)
    plt.close(fig)
    print("wrote", name)


def method_legend(ax, loc="best", images=True):
    from matplotlib.lines import Line2D
    handles = [Line2D([], [], color=GD, label="gradient descent"),
               Line2D([], [], color=NEWTON, label="Newton-type (IRLS)")]
    if images:
        handles += [Line2D([], [], color="k", ls=STYLES[i], marker=MARK[i], label=TITLES[i]) for i in IMAGES]
    ax.legend(handles=handles, loc=loc)


# ----------------------------------------------------------------------
def fig_gallery():
    cols = [("original", "Original"), ("noisy", "Noisy"), ("gd", "Gradient descent, 300 it."),
            ("newton", "Newton-type, 30 it.")]
    fig, axes = plt.subplots(3, 4, figsize=(WIDE, 5.6))
    for r, img in enumerate(IMAGES):
        s = summary[summary.image == img]
        psnr = {"original": None, "noisy": s.noisy_psnr.iloc[0],
                "gd": s[s.method == "GD"].final_psnr.iloc[0], "newton": s[s.method == "Newton"].final_psnr.iloc[0]}
        for c, (key, title) in enumerate(cols):
            ax = axes[r, c]
            ax.imshow(np.asarray(Image.open(DATA / "images" / f"{img}_{key}.png").convert("L")), cmap="gray",
                      vmin=0, vmax=255, interpolation="nearest")
            ax.set_xticks([])
            ax.set_yticks([])
            ax.grid(False)
            if r == 0:
                ax.set_title(title, fontsize=8, pad=3)
            if psnr[key] is not None:
                ax.set_xlabel(f"{psnr[key]:.2f} dB", fontsize=7.5, labelpad=1.5)
            if c == 0:
                ax.set_ylabel(TITLES[img])
    fig.tight_layout(h_pad=0.3, w_pad=0.2)
    save(fig, "gallery")


# ----------------------------------------------------------------------
def top_legend(fig, images=True, y=1.16):
    from matplotlib.lines import Line2D
    handles = [Line2D([], [], color=GD, label="gradient descent"),
               Line2D([], [], color=NEWTON, label="Newton-type")]
    if images:
        handles += [Line2D([], [], color="k", ls=STYLES[i], label=TITLES[i]) for i in IMAGES]
    fig.legend(handles=handles, loc="upper center", ncol=5 if images else 2, bbox_to_anchor=(0.52, y),
               handlelength=1.5, columnspacing=0.55, handletextpad=0.4, fontsize=6.2)


def fig_convergence():
    fig, axes = plt.subplots(1, 2, figsize=(COL, 1.75))
    for img in IMAGES:
        c = conv[conv.image == img]
        emin = c.energy.min()
        for method, color in (("GD", GD), ("Newton", NEWTON)):
            m = c[(c.method == method) & (c.k >= 1)]
            axes[0].semilogx(m.k, m.psnr, color=color, ls=STYLES[img])
            axes[1].semilogx(m.time_ms, m.energy / emin, color=color, ls=STYLES[img])
    axes[0].set_xlabel("iteration $k$")
    axes[0].set_ylabel("PSNR (dB)")
    axes[0].set_title("(a) quality")
    axes[1].set_ylim(0.99, 1.25)
    axes[1].set_xlabel("wall time (ms)")
    axes[1].set_ylabel(r"$E(u_k)/E_{\min}$")
    axes[1].set_title("(b) exact TV energy")
    fig.tight_layout(w_pad=0.8)
    top_legend(fig)
    save(fig, "convergence")


# ----------------------------------------------------------------------
def fig_rates():
    fig, axes = plt.subplots(1, 2, figsize=(COL, 1.75))
    c = conv[(conv.image == "cameraman") & (conv.k >= 1)]
    for method, color in (("GD", GD), ("Newton", NEWTON)):
        m = c[c.method == method]
        axes[0].semilogy(m.k, m.rel_change, color=color, label="gradient descent" if method == "GD" else "Newton-type")
    axes[0].set_xlim(0, 60)
    axes[0].set_xlabel("iteration $k$")
    axes[0].set_ylabel(r"$\|u_k-u_{k-1}\|/\|u_{k-1}\|$")
    axes[0].set_title("(a) Cameraman, $\\varepsilon=10^{-2}$")
    axes[0].legend(loc="upper right")

    e = eps[(eps.image == "cameraman") & (eps.k >= 1)]
    values = sorted(e.epsilon.unique())
    cmap = plt.get_cmap("viridis")
    for i, v in enumerate(values):
        m = e[np.isclose(e.epsilon, v)]
        axes[1].semilogy(m.k, m.rel_change, color=cmap(i / (len(values) - 1)))
        if i in (0, len(values) - 1):
            axes[1].text(m.k.iloc[-1] + 0.6, m.rel_change.iloc[-1], f"$\\varepsilon={v:g}$", fontsize=6.3,
                         ha="left", va="center")
    axes[1].set_xlim(0, 36)
    axes[1].set_xticks([0, 10, 20])
    axes[1].set_xlabel("iteration $k$")
    axes[1].set_title("(b) Newton-type, varying $\\varepsilon$")
    fig.tight_layout(w_pad=0.8)
    save(fig, "rates")


# ----------------------------------------------------------------------
def fig_epsilon():
    fig, axes = plt.subplots(1, 2, figsize=(COL, 1.7))
    last = eps.sort_values("k").groupby(["image", "epsilon"]).tail(1)
    for img in IMAGES:
        m = last[last.image == img].sort_values("epsilon")
        axes[0].semilogx(m.epsilon, m.psnr, color=NEWTON, ls=STYLES[img], marker=MARK[img], markersize=2.4)
        axes[1].semilogx(m.epsilon, m.energy / m.energy.min(), color=NEWTON, ls=STYLES[img], marker=MARK[img],
                         markersize=2.4, label=TITLES[img])
    axes[0].set_xlabel(r"$\varepsilon$")
    axes[0].set_ylabel("PSNR (dB) after 25 it.")
    axes[0].set_title("(a) quality")
    axes[1].set_xlabel(r"$\varepsilon$")
    axes[1].set_ylabel(r"$E(u)/\min_\varepsilon E(u)$")
    axes[1].set_title("(b) exact TV energy")
    axes[1].legend(loc="upper left")
    fig.tight_layout(w_pad=0.8)
    save(fig, "epsilon")


# ----------------------------------------------------------------------
def fig_lambda():
    fig, axes = plt.subplots(1, 2, figsize=(COL, 1.8))
    for img in IMAGES:
        for method, color in (("GD", GD), ("Newton", NEWTON)):
            m = lam[(lam.image == img) & (lam.method == method)].sort_values("lambda")
            axes[0].semilogx(m["lambda"], m.psnr, color=color, ls=STYLES[img], marker=MARK[img], markersize=2.2)
            b = m.loc[m.psnr.idxmax()]
            axes[0].plot(b["lambda"], b.psnr, marker="*", color="k", markersize=5, zorder=6)
    m = lam[(lam.image == "cameraman")].sort_values("lambda")
    for method, color in (("GD", GD), ("Newton", NEWTON)):
        x = m[m.method == method]
        axes[1].loglog(x.tv, x.fidelity, color=color, marker="o", markersize=2.2)
    x = m[m.method == "Newton"]
    for idx, off in ((0, (7, -9)), (7, (4, 3)), (14, (4, -2))):
        row = x.iloc[idx]
        axes[1].annotate(f"$\\lambda={row['lambda']:.2g}$", (row.tv, row.fidelity), fontsize=6.3,
                         xytext=off, textcoords="offset points")
    axes[0].set_xlabel(r"$\lambda$")
    axes[0].set_ylabel("PSNR (dB)")
    axes[0].set_title("(a) PSNR, star: best $\\lambda$")
    axes[1].set_xlabel(r"$\mathrm{TV}(u_\lambda)$")
    axes[1].set_ylabel(r"$\frac{1}{2}\|u_\lambda-f\|^2$")
    axes[1].set_title("(b) L-curve, Cameraman")
    fig.tight_layout(w_pad=0.8)
    top_legend(fig)
    save(fig, "lambda")


# ----------------------------------------------------------------------
def fig_scaling():
    fig, ax = plt.subplots(figsize=(COL, 1.9))
    g = scaling[scaling.method == "GD"]
    d = scaling[scaling.linear_solver == "dense LU"]
    s = scaling[scaling.linear_solver == "sparse LDLT"]
    ax.loglog(g.N, g.ms_per_iteration, color=GD, marker="o", label="gradient descent")
    ax.loglog(d.N, d.ms_per_iteration, color=NEWTON, marker="s", label="Newton-type, dense LU")
    ax.loglog(s.N, s.ms_per_iteration, color=NEWTON, marker="^", ls="--", label="Newton-type, sparse LDLT")
    n = np.array([256.0, 4096.0])
    ax.loglog(n, 0.58 * (n / 256) ** 3, color=GRAY, ls=":", lw=0.8)
    ax.text(330, 25, "$\\propto N^3$", color=GRAY, fontsize=7)
    n = np.array([5184.0, 202500.0])
    ax.loglog(n, 11 * (n / 5184), color=GRAY, ls=":", lw=0.8)
    ax.text(6e4, 30, "$\\propto N$", color=GRAY, fontsize=7)
    ax.axvline(4096, color="k", lw=0.6, ls="-.")
    ax.text(5000, 150, "dense limit\n$N=4096$", fontsize=6.3)
    ax.set_xlabel("number of pixels $N$")
    ax.set_ylabel("time per iteration (ms)")
    ax.legend(loc="lower right")
    save(fig, "scaling")


# ----------------------------------------------------------------------
def table_comparison():
    rows = []
    for img in IMAGES:
        c = conv[conv.image == img]
        cells = [TITLES[img], f"{summary[summary.image == img].noisy_psnr.iloc[0]:.2f}"]
        for method, final_k in (("GD", 300), ("Newton", 30)):
            m = c[(c.method == method) & (c.k >= 1)]
            best = m.loc[m.psnr.idxmax()]
            k10 = m[m.k == 10].iloc[0]
            fin = m[m.k == final_k].iloc[0]
            cells += [f"{best.psnr:.2f} ({int(best.k)})", f"{k10.psnr:.2f}", f"{fin.psnr:.2f}", f"{fin.energy:.1f}",
                      f"{fin.time_ms / final_k:.1f}"]
        rows.append(" & ".join(cells) + " \\\\")
    (FIG / "table_comparison.tex").write_text("\n".join(rows) + "\n", encoding="utf-8")
    print("wrote table_comparison.tex")


def table_sigma():
    best = sigma.loc[sigma.groupby(["image", "sigma", "method"]).psnr.idxmax()]
    rows = []
    for sg in sorted(sigma.sigma.unique()):
        cells = [f"{sg:.2f}"]
        for img in IMAGES:
            b = best[(best.image == img) & np.isclose(best.sigma, sg)]
            gd = b[b.method == "GD"].iloc[0]
            nt = b[b.method == "Newton"].iloc[0]
            lam_text = f"{gd['lambda']:.3f}" if np.isclose(gd["lambda"], nt["lambda"]) else \
                f"{gd['lambda']:.3f}/{nt['lambda']:.3f}"
            cells += [f"{gd.noisy_psnr:.2f}", lam_text, f"{gd.psnr:.2f}", f"{nt.psnr:.2f}"]
        rows.append(" & ".join(cells) + " \\\\")
    (FIG / "table_sigma.tex").write_text("\n".join(rows) + "\n", encoding="utf-8")
    print("wrote table_sigma.tex")


if __name__ == "__main__":
    fig_gallery()
    fig_convergence()
    fig_rates()
    fig_epsilon()
    fig_lambda()
    fig_scaling()
    table_comparison()
    table_sigma()

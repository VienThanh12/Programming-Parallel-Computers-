# Programming Parallel Computers

Personal study notes and exercise solutions for:
- **CS-E4580** — Aalto University ([course page](https://ppc.cs.aalto.fi))
- **CSM14212** — University of Helsinki, HY 2025–2026

---

## What This Course Is About

All modern computers are massively parallel processors. A normal desktop CPU can have **hundreds of arithmetic operations in progress simultaneously** through:

- Multiple CPU cores
- Multiple execution units per core
- Wide vector (SIMD) operations
- Pipelined execution units

Yet most programs use **less than 1%** of the theoretical computing power available. This course teaches you how to exploit that power — on both CPUs and GPUs.

### The Problem in Numbers

On a modern 4-core Intel CPU, a naive baseline solution (`V0`) uses only **0.6% of the theoretical multi-core performance**. A well-optimized solution (`V7`) is **42× faster on a single core** and **151× faster across all 4 cores**.

> *Good performance on a modern CPU is much more than simply using multiple threads.*

---

## Why Parallelism?

### Moore's Law → Still Running, But Changed Direction

Transistor counts continue doubling every ~2 years. However, since ~2000, **clock speeds stopped improving** and remain in the 2–4 GHz range:

| Year | Transistors | Clock Speed | CPU |
|------|-------------|-------------|-----|
| 1975 | 3,000 | 1 MHz | 6502 |
| 1985 | 300,000 | 20 MHz | 386 |
| 2000 | 40,000,000 | 2,000 MHz | Pentium 4 |
| 2008 | 700,000,000 | 3,000 MHz | 8-core Nehalem |
| 2017 | 20,000,000,000 | 3,000 MHz | 32-core AMD Epyc |
| 2023 | 90,000,000,000 | 4,000 MHz | M3 Max |

A single float multiply still takes ~2 ns today — same as year 2000.

### Latency vs. Throughput

| Concept | Definition |
|---------|------------|
| **Latency** | Time to complete one operation (start → finish) |
| **Throughput** | Operations completed per unit time (steady state) |

Modern CPUs improve **throughput, not latency** — by having many pipelined parallel units running simultaneously. A modern 4-core CPU can sustain **~200 billion FP multiplies/second** at a throughput of 64 operations/cycle.

**Old sequential code cannot benefit from this. New parallel-aware code can.**

---

## Prerequisites

- Solid understanding of algorithms and data structures
- Working knowledge of C or C++
- No prior knowledge of parallel computing, multi-threading, or GPUs required

---

## Repository Structure

```
.
├── ch01-role-of-parallelism/     # Why we need parallelism; latency vs throughput
│   ├── notes/
│   └── exercises/
├── ch02-cpu-programming/         # SIMD, vectorization, multi-threading
│   ├── notes/
│   └── exercises/
├── ch03-gpu-programming/         # CUDA / GPU concepts and coding
│   ├── notes/
│   └── exercises/
├── ch04-performance-engineering/ # Assembly analysis, performance prediction
│   ├── notes/
│   └── exercises/
├── exercises/                    # Graded course exercises (organized by problem set)
│   ├── pre/    # Prerequisite test
│   ├── cp/     # Correlated Pairs
│   ├── is/     # Image Segmentation
│   ├── mf/     # Median Filter
│   ├── so/     # Sorting
│   ├── llm/    # Large Language Model
│   └── x/      # Extra
├── resources/                    # Reference material, links, cheat sheets
└── solutions/                    # Polished / final solutions
```

---

## Exercises & Progress

Course: **CSM14212 — HY 2025–2026** · Deadline: **2026-08-31**

Grading thresholds: ≥ 69 pts → **Grade 5** · ≥ 62 → **4** · ≥ 54 → **3** · Total available: **104 + 2 bonus**

> **M** = mandatory (must score ≥ 1 pt to pass)  
> Rating: ★ = easy · ★★ = medium · ★★★ = research-level

### Pre — Prerequisite Test

| Task | Points | Rating | Mandatory | Status |
|------|--------|--------|-----------|--------|
| Pre0 | 1 | ★ | | ⬜ |

### CP — Correlated Pairs

| Task | Points | Rating | Mandatory | Status |
|------|--------|--------|-----------|--------|
| CP1 | 5 | ★ | M | ⬜ |
| CP2a | 3 | ★ | | ⬜ |
| CP2b | 3 | ★ | | ⬜ |
| CP2c | 3 | ★ | | ⬜ |
| CP3a | 5 | ★★ | M | ⬜ |
| CP3b | 5 | ★★ | | ⬜ |
| CP4 | 5 | ★ | | ⬜ |
| CP5 | 10 | ★★ | M | ⬜ |
| CP9a | 5 | ★★★ | | ⬜ |

### IS — Image Segmentation

| Task | Points | Rating | Mandatory | Status |
|------|--------|--------|-----------|--------|
| IS2 | 5 | ★★ | | ⬜ |
| IS4 | 5 | ★★ | | ⬜ |
| IS6a | 5 | ★★★ | | ⬜ |
| IS6b | 5 | ★★ | | ⬜ |
| IS9a | 5 | ★★★ | | ⬜ |

### MF — Median Filter

| Task | Points | Rating | Mandatory | Status |
|------|--------|--------|-----------|--------|
| MF1 | 5 | ★ | | ⬜ |
| MF2 | 3 | ★ | M | ⬜ |
| MF9a | 5 | ★★★ | | ⬜ |

### SO — Sorting

| Task | Points | Rating | Mandatory | Status |
|------|--------|--------|-----------|--------|
| SO4 | 5 | ★+ | M | ⬜ |
| SO5 | 5 | ★+ | | ⬜ |
| SO6 | 5 | ★★★ | | ⬜ |

### LLM — Large Language Model

| Task | Points | Rating | Mandatory | Status |
|------|--------|--------|-----------|--------|
| LLM9a | 5 + 2 bonus | ★★ | | ⬜ |

### X — Extra

| Task | Points | Rating | Status |
|------|--------|--------|--------|
| X0a | 0 | — | ⬜ |
| X0b | 0 | — | ⬜ |
| X9a | 1 | + | ⬜ |

### Score Tracker

| Category | Earned | Max |
|----------|--------|-----|
| Pre | 0 | 1 |
| CP | 0 | 44 |
| IS | 0 | 25 |
| MF | 0 | 13 |
| SO | 0 | 15 |
| LLM | 0 | 5 (+2 bonus) |
| X | 0 | 1 |
| **Total** | **0** | **104 (+2 bonus)** |

---

## Key Concepts (Quick Reference)

| Concept | One-liner |
|---------|-----------|
| **Pipelining** | A new instruction can start before the previous one finishes |
| **SIMD / Vectorization** | One instruction operates on multiple data values at once |
| **Multi-threading** | Use all CPU cores in parallel |
| **Throughput vs Latency** | Modern CPUs optimize throughput — so must your code |
| **Independent operations** | Required for any parallelism; avoid data-dependency chains |
| **GPU** | Massively parallel; requires explicit GPU code to use |

---

## Resources

- [Course website (ppc.cs.aalto.fi)](https://ppc.cs.aalto.fi)
- [HY course page](https://studies.helsinki.fi/courses/course-unit/otm-141a5e07-08d5-46cc-b6de-5aabd8b7d4c7)
- [Discord channel](https://discord.gg/ppc) *(check course page for current link)*
- Original paper: Moore, G.E. (1965). *Cramming more components onto integrated circuits.* Electronics Magazine. Reprinted in Proc. IEEE, vol. 86, no. 1, 1998.

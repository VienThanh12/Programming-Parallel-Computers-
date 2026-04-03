# Chapter 1 — Why Parallelism?

> Source: CS-E4580 Lecture Notes, Aalto University

## Key Insight

Modern CPUs are massively parallel but most programs use less than 1% of their theoretical compute power.

**Sources of CPU parallelism:**
- Multiple CPU cores
- Multiple execution units per core (can run simultaneously)
- Wide vector (SIMD) operations per execution unit
- Pipelined execution units (no need to wait for prior instruction to finish)

---

## Moore's Law & The Performance Shift

| Year | Transistors | Clock Speed | CPU Model |
|------|-------------|-------------|-----------|
| 1975 | 3,000 | 1 MHz | 6502 |
| 1979 | 30,000 | 5 MHz | 8088 |
| 1985 | 300,000 | 20 MHz | 386 |
| 1989 | 1,000,000 | 20 MHz | 486 |
| 1995 | 6,000,000 | 200 MHz | Pentium Pro |
| 2000 | 40,000,000 | 2,000 MHz | Pentium 4 |
| 2005 | 100,000,000 | 3,000 MHz | 2-core Pentium D |
| 2008 | 700,000,000 | 3,000 MHz | 8-core Nehalem |
| 2014 | 6,000,000,000 | 2,000 MHz | 18-core Haswell |
| 2017 | 20,000,000,000 | 3,000 MHz | 32-core AMD Epyc |
| 2023 | 90,000,000,000 | 4,000 MHz | M3 Max |

**Observation:** After ~2000, clock speeds stopped growing. Transistor count kept growing but performance per single instruction did not.

### FMUL (80-bit float multiply) latency over time:

| Year | Clock Cycles | CPU |
|------|-------------|-----|
| 1980 | 100 | 8087 |
| 1987 | 50 | 387 |
| 1993 | 3 | Pentium |
| 2018 | 5 | Coffee Lake |

**A single float multiply still takes ~2 ns in 2025 — same as year 2000.**

---

## Latency vs. Throughput

| Term | Definition |
|------|------------|
| **Latency** | Time to complete one operation start-to-finish |
| **Throughput** | Operations completed per unit time (long run) |

**University analogy:** A Master's degree has a *latency* of 2 years. Aalto's *throughput* is ~1960 degrees/year — because thousands of students are in the pipeline simultaneously.

Modern CPUs work the same way with **pipelining**: even if one multiplication takes 5 clock cycles, a new one can be *started* every clock cycle.

### Pipelining illustration:

| Time | Started | In Progress | Finished |
|------|---------|-------------|---------|
| 0 | A | — | — |
| 1 | B | A | — |
| 2 | C | B, A | — |
| 5 | F | E, D, C, B | A |
| 6 | G | F, E, D, C | B |

A typical modern CPU core has **16 parallel single-precision FP multiply units**.  
→ A 4-core CPU can do **64 FMUL/cycle ≈ 200 billion ops/second**.

**Old code does not benefit from this. This course teaches you to write new code that does.**

---

## Section Reference
- Course page: <https://ppc.cs.aalto.fi>
- Chapter 1: Why? / How?

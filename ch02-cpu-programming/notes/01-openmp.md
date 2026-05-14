# Chapter 2 — OpenMP

> Source: CS-E4580 Lecture Notes, Aalto University

The obvious drawback of the baseline implementation is that it only uses one thread, and hence only one CPU core. To exploit all CPU cores, we must somehow create multiple threads of execution.

## About threads

Any modern operating system makes it possible to create threads. If we create, for example, 4 threads, and we run the code on a computer with 4 CPU cores, we can usually count on the operating system to do the sensible thing: it will typically assign each thread to a separate CPU core, and hence, if all goes well, we can do (almost) 4 times as much useful work per second as previously.

There are many ways of creating threads:

- **pthreads** — a low-level interface in Unix-like operating systems.
- **C++11 thread support library** — a higher-level interface on top of OS threading primitives.

However, using such libraries directly to speed up computations would take a nontrivial amount of code. In this course we will use a convenient high-level interface called **OpenMP**.

## OpenMP: multithreading made easy

OpenMP is an extension of the C, C++, and Fortran programming languages. It is standardized and widely supported (e.g., GCC has built-in support). To enable OpenMP support, use the `-fopenmp` flag both to compile and link:

```bash
# Without OpenMP:
g++ -g -O3 -march=native -std=c++17

# With OpenMP:
g++ -fopenmp -g -O3 -march=native -std=c++17
```

The basic idea: add `#pragma omp` directives in the source code to tell OpenMP how to divide work among multiple threads.

## OpenMP parallel for loops

The key directive is `#pragma omp parallel for`, used right before a `for` loop you want to parallelize:

```cpp
#pragma omp parallel for
for (int i = 0; i < 10; ++i) {
    c(i);
}
```

Without the `#pragma`, the loop runs sequentially in one thread:

```
Thread 1: c(0); c(1); c(2); c(3); c(4); c(5); c(6); c(7); c(8); c(9);
```

With the `#pragma`, OpenMP splits iterations across threads (e.g., 4 threads on a 4-core machine):

```
Thread 1: c(0); c(1); c(2);
Thread 2: c(3); c(4); c(5);
Thread 3: c(6); c(7);
Thread 4: c(8); c(9);
```

OpenMP maintains a thread pool and handles synchronization — the program does not continue past the loop until all threads have finished their iterations.

## Scheduling directives

You can control how iterations are split with the `schedule` directive:

```cpp
#pragma omp parallel for schedule(static,1)
for (int i = 0; i < 10; ++i) {
    c(i);
}
```

Now iterations are interleaved across threads:

```
Thread 1: c(0); c(4); c(8);
Thread 2: c(1); c(5); c(9);
Thread 3: c(2); c(6);
Thread 4: c(3); c(7);
```

This is useful when the cost of `c(i)` depends on `i`, so each thread gets a mix of small and large jobs.

## Warning! Stay safe!

When you ask OpenMP to parallelize a `for` loop, **it is your responsibility to make sure it is safe**.

Two operations X and Y can be executed in parallel only if:

1. There must not be any shared data element that is read by X and written by Y.
2. There must not be any shared data element that is written by X and written by Y.

A "data element" here is, for example, a scalar variable or an array element.

**Cannot parallelize** — iteration 0 writes `x[1]`, iteration 1 reads `x[1]`:

```cpp
for (int i = 0; i < 10; ++i) {
    x[i + 1] = f(x[i]);
}
```

**Cannot parallelize** — iteration 0 and 1 both write `y[0]`:

```cpp
for (int i = 0; i < 10; ++i) {
    y[0] = f(x[i]);
}
```

**Safe to parallelize** (if `x` and `y` point to distinct arrays and `f` has no side effects):

```cpp
#pragma omp parallel for
for (int i = 0; i < 10; ++i) {
    y[i] = f(x[i]);
}
```

## Back to our application

Recall the sequential baseline `step`:

```cpp
void step(float* r, const float* d, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float v = std::numeric_limits<float>::infinity();
            for (int k = 0; k < n; ++k) {
                float x = d[n*i + k];
                float y = d[n*k + j];
                float z = x + y;
                v = std::min(v, z);
            }
            r[n*i + j] = v;
        }
    }
}
```

The outermost loop is safe to parallelize:

- `j` is local (declared inside the loop), not shared.
- `n` is read-only.
- `d` is read-only.
- `r` is write-only, and different iterations write to different elements.

Adding the pragma is all that is needed:

```cpp
void step(float* r, const float* d, int n) {
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float v = std::numeric_limits<float>::infinity();
            for (int k = 0; k < n; ++k) {
                float x = d[n*i + k];
                float y = d[n*k + j];
                float z = x + y;
                v = std::min(v, z);
            }
            r[n*i + j] = v;
        }
    }
}
```

Compile with `-fopenmp` and the program uses all CPU cores. For `n = 4000` on a 4-core machine, each thread handles 1000 iterations of the outer loop.

### Results

| Version            | Time (s) | Speedup |
| ------------------ | -------- | ------- |
| Baseline (1 core)  | 99       | 1.0×    |
| OpenMP (4 cores)   | 25       | 3.9×    |

But we are definitely not done yet!

---

# Version 1: Linear reading

What is the bottleneck in V0 — getting data from memory to the CPU, or doing arithmetic? Performance varies dramatically with input size:

- For `n = 1000`: input is ~4 MB, fits in the 6 MB last-level cache.
- For `n = 1600`: input is ~10 MB, exceeds cache.

Performance drops sharply once the input no longer fits in cache. **Memory is the bottleneck for large `n`.**

## Source of the problem

In the innermost loop:

```cpp
for (int k = 0; k < n; ++k) {
    // ...
    float y = d[n*k + j];
    // ...
}
```

Access pattern (for `n = 10`):

```
d[0], d[10], d[20], ..., d[90],
d[1], d[11], d[21], ..., d[91],
...
```

We scan `d` in a non-linear (column-wise) order, repeatedly. When `d[0]` is read, it is cached — but by the time we want it again, it has been evicted to make room for other elements. We pay for an expensive main-memory fetch every time.

### Why was macOS slower? [advanced]

Memory addresses in a program are **virtual addresses**. The CPU translates them to physical addresses using a page table; this translation is cached in the **TLB**.

- **Linux** (with Transparent Hugepage Support) typically uses 2 MB pages for large allocations.
- **macOS** defaults to 4 KB pages.

For `n = 4000`, increasing `k` by 1 shifts the address by 16 KB. With 4 KB pages we cross page boundaries constantly, the TLB cannot hold all the relevant entries, and we pay extra page-table walks. With 2 MB pages we usually stay within the same page.

The same slowdown can be reproduced on Linux by disabling Transparent Hugepage Support.

## How to fix it

A good rule of thumb for modern computers: **read memory in linear order**.

Two reasons:

1. Memory is transferred in **64-byte cache lines**. Reading `d[0]` (4 bytes) also brings `d[1]..d[15]` into cache for free.
2. **Hardware prefetching** kicks in when the CPU detects sequential access — data is fetched before the program asks for it.

The fix: build the **transpose** `t` of `d` once, then both `d` and `t` are scanned by rows (linear) in the inner loop.

```cpp
void step(float* r, const float* d, int n) {
    std::vector<float> t(n*n);
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            t[n*j + i] = d[n*i + j];
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float v = std::numeric_limits<float>::infinity();
            for (int k = 0; k < n; ++k) {
                float x = d[n*i + k];
                float y = t[n*j + k];
                float z = x + y;
                v = std::min(v, z);
            }
            r[n*i + j] = v;
        }
    }
}
```

### Results

Now performance is roughly the same on small and large inputs — memory is no longer the limiting factor. The speedup over V0 on Linux for `n = 4000` is modest, though, because the next bottleneck has appeared.

## Version 1: Assembly code [advanced]

The innermost loop compiles to just 6 instructions:

```asm
LOOP:
    vmovss     (%rsi,%rax,4), %xmm0
    vaddss     (%rdx,%rax,4), %xmm0, %xmm0
    addq       $1, %rax
    cmpl       %eax, %ebx
    vminss     %xmm1, %xmm0, %xmm1
    jg         LOOP
```

`%xmm0` holds `x`, `%xmm1` accumulates `v`:

| C++                          | Assembly                              |
| ---------------------------- | ------------------------------------- |
| `x = d[n*i + k];`            | `vmovss (%rsi,%rax,4), %xmm0`         |
| `x = t[n*j + k] + x;`        | `vaddss (%rdx,%rax,4), %xmm0, %xmm0`  |
| `v = std::min(v, x);`        | `vminss %xmm1, %xmm0, %xmm1`          |

### Latency vs. throughput

From Agner Fog's instruction tables (Intel Skylake):

| Instruction    | Latency | Reciprocal throughput |
| -------------- | ------- | --------------------- |
| `ADDSS v,v,v`  | 4       | 0.5                   |
| `MINSS v,v,v`  | 4       | 0.5                   |

Reciprocal throughput 0.5 = **2 operations per clock cycle**. But our benchmark shows only **0.50 ops/cycle** — far below the throughput limit.

### Dependency chain

The bottleneck is a **dependency chain** on `%xmm1`:

```asm
... vminss %xmm1, %xmm0, %xmm1
... vminss %xmm1, %xmm0, %xmm1
... vminss %xmm1, %xmm0, %xmm1
```

Each `vminss` consumes the result of the previous one. The same chain exists in C++:

```cpp
v = std::min(v, z);
v = std::min(v, z);
v = std::min(v, z);
```

So each iteration must wait for the latency of `vminss`, which is **4 cycles**. In 4 cycles we do 1 add + 1 min = 2 useful ops → at most **0.5 useful ops/cycle**. This matches our benchmarks almost exactly.

Memory is no longer the bottleneck — the **sequential dependency chain in `min`** is. We need to break it.

---

# Version 2: Instruction-level parallelism

The innermost loop is sequential because each `min` depends on the previous:

```cpp
v = std::min(v, z0);
v = std::min(v, z1);
v = std::min(v, z2);
v = std::min(v, z3);
```

## Independent operations

Accumulate **two** (or more) minimums in interleaved fashion, then combine at the end:

```cpp
v0 = std::min(v0, z0);
v1 = std::min(v1, z1);
v0 = std::min(v0, z2);
v1 = std::min(v1, z3);
// ...
v = std::min(v0, v1);
```

Now operations 1 & 2 can execute in parallel, then 3 & 4 in parallel, etc. Extending to 4 independent accumulators gives even more parallelism.

## Instruction-level parallelism is automatic

How do we tell the CPU these instructions can run in parallel?

> **The answer: we don't — it happens automatically.**

The CPU looks ahead in the instruction stream, performs branch prediction, and identifies instructions whose operands are ready. It dispatches them to execution units **out of order**, fully in hardware. The programmer's only job: provide enough independent instructions.

## Implementation

V1 was latency-limited at 0.5 ops/cycle, while throughput allows 2 ops/cycle — so a **4× speedup** is possible by running 4 independent accumulators.

Two ways to handle `n` not being a multiple of 4:

1. Pad the data so the width is a multiple of 4.
2. Do `n/4` blocks plus a tail loop for the remaining `n % 4` elements.

We choose option 1 (padding):

```cpp
constexpr float infty = std::numeric_limits<float>::infinity();

void step(float* r, const float* d_, int n) {
    constexpr int nb = 4;
    int na = (n + nb - 1) / nb;
    int nab = na * nb;

    // input data, padded
    std::vector<float> d(n*nab, infty);
    // input data, transposed, padded
    std::vector<float> t(n*nab, infty);

    #pragma omp parallel for
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            d[nab*j + i] = d_[n*j + i];
            t[nab*j + i] = d_[n*i + j];
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            // vv[0]..vv[3] accumulate minimums of k % 4 == 0..3
            float vv[nb];
            for (int kb = 0; kb < nb; ++kb) {
                vv[kb] = infty;
            }
            for (int ka = 0; ka < na; ++ka) {
                for (int kb = 0; kb < nb; ++kb) {
                    float x = d[nab*i + ka*nb + kb];
                    float y = t[nab*j + ka*nb + kb];
                    float z = x + y;
                    vv[kb] = std::min(vv[kb], z);
                }
            }
            float v = infty;
            for (int kb = 0; kb < nb; ++kb) {
                v = std::min(vv[kb], v);
            }
            r[n*i + j] = v;
        }
    }
}
```

`nab` is `n` rounded up to a multiple of 4. Padding cells are filled with `+infty`, so they never affect the minimum. The result is **bit-for-bit identical** to V1, just computed in a different order.

## Notes

The added inner `for (int kb = 0; kb < nb; ++kb)` and `vv[]` array might look expensive. They are not, with `-O3`:

- `nb` is a `constexpr`, so the inner loop is fully **unrolled**.
- After unrolling, all `vv[kb]` uses have constant indices, so `vv[0..3]` are kept in **registers**, not memory.

Using an array + loop instead of 4 named variables keeps the code easy to re-tune (try `nb = 2, 8, ...`). But always check the assembly to confirm the compiler did the right thing.

### Results

| Version            | Single-thread | Multi-thread |
| ------------------ | ------------- | ------------ |
| V0 baseline        | 99 s          | —            |
| V1 (transposed)    | —             | —            |
| V2 (ILP, 4 accs)   | 3.3× over V1  | 3.2× over V1 |

Total: **99 s → 6 s**. We now achieve **1.6 useful ops/cycle** per core — each core really is a superscalar processor.

## Version 2: Assembly code [advanced]

The compiler unrolls the `kb` loop and assigns `vv[0..3]` to `%xmm1, %xmm4, %xmm3, %xmm2`:

```asm
LOOP:
    vmovss     (%rax), %xmm0
    addq       $16, %rax
    addq       $16, %rdx
    vaddss     -16(%rdx), %xmm0, %xmm0
    vminss     %xmm1, %xmm0, %xmm1
    vmovss     -12(%rax), %xmm0
    vaddss     -12(%rdx), %xmm0, %xmm0
    vminss     %xmm4, %xmm0, %xmm4
    vmovss     -8(%rax), %xmm0
    vaddss     -8(%rdx), %xmm0, %xmm0
    vminss     %xmm3, %xmm0, %xmm3
    vmovss     -4(%rax), %xmm0
    vaddss     -4(%rdx), %xmm0, %xmm0
    cmpq       %rsi, %rax
    vminss     %xmm2, %xmm0, %xmm2
    jne        LOOP
```

There is no special "parallel" annotation in the machine code. The CPU recognizes that the four `vminss` instructions are independent and dispatches them in parallel.

### But what about reusing `%xmm0`?

It looks like `%xmm0` is reused on every add and min — wouldn't this serialize execution? **No**, thanks to **register renaming**. The CPU maps the architectural `%xmm0` to different physical registers (`%xmm0a, %xmm0b, %xmm0c, %xmm0d`), as if we had written:

```cpp
xa = d[...0]; xa = t[...0] + xa; vv[0] = std::min(vv[0], xa);
xb = d[...1]; xb = t[...1] + xb; vv[1] = std::min(vv[1], xb);
xc = d[...2]; xc = t[...2] + xc; vv[2] = std::min(vv[2], xc);
xd = d[...3]; xd = t[...3] + xd; vv[3] = std::min(vv[3], xd);
```

After renaming, the four `vmovss`, four `vaddss`, and four `vminss` form three groups of independent instructions that can issue in parallel.

The CPU also looks across iterations of the outer loop, so at any moment it might be running:

- memory lookups for iteration `ka + 2`,
- `vaddss` for iteration `ka + 1`,
- `vminss` for iteration `ka`.

By the time `vminss` needs the result of `vaddss`, it is already done.

## Analysis

`vaddss` and `vminss` together can issue **2 per cycle** (ports 0 and 1 on Skylake handle them). So the theoretical max is 2 useful ops/cycle/core.

We measure **1.6 useful ops/cycle**, plus overhead from memory loads, loop counters, comparisons, and jumps — about **3.3 instructions/cycle/core** total (visible via `perf stat`). Already close to the practical limit for a superscalar core (~3–4 instructions/cycle).

To go further, each instruction must do **more useful work** — that is the job of **vector instructions**, the topic of Version 3.

### Things to try in Compiler Explorer

- Increase `nb` slightly — does ILP improve?
- Remove `constexpr` from `nb` — what happens? Try also with `-O2`.
- Move the declaration of `nb` inside `step` (without `constexpr`).
- Increase `nb` a lot, e.g., to 20 — the compiler may auto-vectorize.

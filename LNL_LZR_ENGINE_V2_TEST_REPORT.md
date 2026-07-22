# LNL/LZR Engine V2 optimization and test report

Version: LNL-LZR-CE-2  
Date: 2026-07-20  
Copyright (c) 2026 Mike Lange. All rights reserved.

## Outcome

The active-limb representation and exact completed-result cache passed all
correctness tests. The native V2 `engine-fast` build used 48.09 percent less
time than the freshly compiled portable V1 reference in the three-run median
333-bit benchmark. That corresponds to 1.926x throughput.

## Fair V1 versus V2 comparison

Workload: 20,000 deterministic 333-bit starts, block 20260728, cache capacity
500,000. V1 and portable V2 were compiled by the same compiler with the same
`-O3 -std=c++20` base flags. Values below are medians of three runs.

| Build and mode | Median time |
| --- | ---: |
| V1 portable engine-fast | 279.348190 ms |
| V2 portable engine-fast | 155.922420 ms |
| V2 native engine-fast | 145.014876 ms |
| V2 native exact-reuse cold | 150.890837 ms |
| V2 native exact-reuse warm | 2.410174 ms |
| V2 native trajectory-cache warm | 7.356848 ms |

- V1 portable to V2 native engine-fast: 1.926x throughput.
- Portable V2 to native V2: 1.075x throughput, or 7.00 percent less time.
- Exact-reuse cold to warm: 62.606x speedup and zero new transitions.
- Exact root reuse versus warm trajectory cache: 3.052x faster.

## PGO decision

The tested PGO build was not selected. Its median engine-fast time was
152.230457 ms, 4.98 percent slower than the native build. Its warm exact-cache
time was 4.092279 ms, 69.79 percent slower than native. This experiment remains
documented as a rejected optimization rather than being presented as a gain.

## Correctness gates

- Embedded suffix rows checked: 100,000.
- Embedded suffix failures: 0.
- Deterministic widths checked: 128, 333, 500, 768 and 900 bits.
- Cross-mode verification failures: 0.
- Warm exact-reuse evaluated transitions: 0.
- Bounded FIFO replacement test with 257 entries: 0 failures.
- Address/undefined sanitizer run: 0 reported program errors; leak detection
  was disabled because it is unsupported in the container environment.
- V1/V2 comparison of 25,000 block-344 starts: identical candidate sequence,
  strongest start, peak, peak step and total stopping time.

## Exactness rule

An entry is inserted only after `engine-fast` has completed the whole suffix to
1 or reached the independently verified embedded table. Reuse requires full
U1024 equality. Rest classes such as mod 32, 64, 128, 256, 512 or 1024 may be
stored as classifications, but they are never sufficient for an exact cache
hit.

## Scientific status

These results establish reproducible software behavior for the tested inputs.
They do not prove the Collatz conjecture and do not establish a global peak
record.

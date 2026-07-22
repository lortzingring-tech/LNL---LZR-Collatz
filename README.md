# LNL/LZR 1024-bit Collatz Search Engine V2

An exact, deterministic 1024-bit Collatz search engine developed as part of the
**Lange Number Line Project (LNL)**, called **Langer Zahlenstrahl (LZS)** in
German. **LZR** denotes the residue and search engine component.

**Version:** `LNL-LZR-CE-2`  
**Original author:** Mike Lange  
**Release date:** 2026-07-20

> Scientific status: This software reproduces exact calculations for tested
> inputs. It is not a proof of the Collatz conjecture, and sampled peaks are not
> automatically global records.

## Highlights

- Exact unsigned 1024-bit arithmetic using sixteen 64-bit limbs.
- Active-limb metadata avoids work on unused high limbs.
- Embedded table containing 100,000 independently verified suffix results.
- Three calculation modes for different workloads.
- Exact completed-result reuse: cache hits require equality of the complete
  1024-bit start value.
- Portable and CPU-native C++20 builds.
- Deterministic verification and benchmark data included.

Residue classes such as modulo 32, 64, 128, 256, 512, or 1024 are
classifications only. They are never treated as sufficient evidence for an
identical trajectory or an exact cache hit.

## Verified V2 performance

Primary benchmark: 20,000 deterministic 333-bit starts, median of three runs
on an AMD EPYC 9V74 host.

| Build and mode | Median time |
| --- | ---: |
| V1 portable `engine-fast` | 279.348 ms |
| V2 portable `engine-fast` | 155.922 ms |
| V2 native `engine-fast` | 145.015 ms |
| V2 native `engine-reuse`, cold | 150.891 ms |
| V2 native `engine-reuse`, warm | 2.410 ms |

V2 native `engine-fast` required 48.09% less time than the portable V1
reference, corresponding to 1.926x throughput. Warm exact reuse was 62.606x
faster than its cold run.

## Requirements

- Linux or a compatible POSIX environment
- A C++20 compiler such as GCC or Clang
- Bash

The prebuilt native binary is CPU-family specific. Build or use the portable
binary on unknown hardware.

## Build

```bash
chmod +x build_lnl_lzr_v2.sh verify_lnl_lzr_v2.sh
./build_lnl_lzr_v2.sh portable
./build_lnl_lzr_v2.sh native
```

## Verify

```bash
./verify_lnl_lzr_v2.sh
```

The verification covers:

- all 100,000 embedded suffix rows;
- deterministic inputs at 128, 333, 500, 768, and 900 bits;
- all calculation modes;
- exact warm-result reuse;
- bounded FIFO replacement behavior.

The recorded release tests reported zero mathematical verification failures.

## Run

```bash
./search_lnl_1024_engine_native search SAMPLES BITS BLOCK engine-fast 0
./search_lnl_1024_engine_native search SAMPLES BITS BLOCK engine-reuse CACHE_ENTRIES
./search_lnl_1024_engine_native search SAMPLES BITS BLOCK engine-cache CACHE_ENTRIES
```

### Modes

- `engine-fast`: recommended for new, non-repeating searches.
- `engine-reuse`: optimized for repeated complete start values.
- `engine-cache`: useful when different starts merge into previously evaluated
  exact intermediate states.

## Repository contents

- `search_lnl_1024_engine.cpp` - main C++20 source.
- `lnl_opening_book_100k.hpp` - verified embedded suffix table.
- `build_lnl_lzr_v2.sh` - portable and native build script.
- `verify_lnl_lzr_v2.sh` - deterministic verification script.
- `LNL_LZR_ENGINE_V2_TEST_REPORT.md` - optimization and correctness report.
- `LNL_LZR_ENGINE_V2_BENCHMARK.json` - machine-readable benchmark results.
- `LNL_LZR_PROJECT_DIARY_2026-07-20.md` - development record for this release.
- `RELEASE_NOTES.md` - GitHub release summary.
- `CITATION.cff` - citation metadata preserving original authorship.

## Authorship and license

Copyright (c) 2026 Mike Lange.

The software is released under the MIT License. Use, modification, and
redistribution are permitted, but the copyright and permission notice must be
preserved. See `LICENSE` and `CITATION.cff`.


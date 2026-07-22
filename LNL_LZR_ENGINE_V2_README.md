# Lange Number Line Project: LNL/LZR Engine V2

Version: LNL-LZR-CE-2  
Date: 2026-07-20  
Copyright (c) 2026 Mike Lange. All rights reserved.

## Scope and naming

The main project is the Lange Number Line Project (LNL), called Langer
Zahlenstrahl (LZS) in German. LZR identifies the 1024-bit residue and Collatz
search engine used here.

This version transfers tested engineering lessons from PeakQueen back into the
Collatz software. It does not run chess logic and does not use Collatz residue
similarity as proof of an identical trajectory.

## New architecture

1. Active-limb U1024 arithmetic
   - A 1024-bit value still has 16 exact 64-bit limbs.
   - It additionally records how many high limbs are actually occupied.
   - Comparisons, hashing, `3n+1`, shifts and decimal conversion touch only the
     active range.

2. Exact completed-result reuse
   - `engine-reuse` stores only a fully completed result.
   - A hit requires equality of the complete 1024-bit start value.
   - Hash or residue equality alone can never produce a hit.
   - The FIFO cache is bounded and uses exact-key collision checks.

3. Existing trajectory reuse retained
   - `engine-cache` remains available for merging trajectories through exact
     intermediate states.
   - `engine-fast` remains the recommended mode for one-pass exploration.

4. Evidence-based builds
   - Portable and native build scripts are supplied.
   - The native build was faster on the tested AMD EPYC host.
   - The tested PGO build was slower than native and was therefore rejected as
     the recommended build.

## Build

```text
./build_lnl_lzr_v2.sh portable
./build_lnl_lzr_v2.sh native
```

The native binary is specific to the CPU family on which it was built. Use the
portable binary when moving the program to an unknown computer.

## Verification

```text
./verify_lnl_lzr_v2.sh
```

The verification covers all 100,000 embedded suffixes, deterministic widths of
128, 333, 500, 768 and 900 bits, all calculation modes, warm exact reuse and a
small-cache replacement test.

## Modes

```text
./search_lnl_1024_engine_native search SAMPLES BITS BLOCK engine-fast 0
./search_lnl_1024_engine_native search SAMPLES BITS BLOCK engine-reuse CACHE_ENTRIES
./search_lnl_1024_engine_native search SAMPLES BITS BLOCK engine-cache CACHE_ENTRIES
```

- `engine-fast`: best for new non-repeating samples.
- `engine-reuse`: best when complete start values will be requested again.
- `engine-cache`: useful when different starts often merge into previously
  calculated intermediate states.

The exact reuse cache must be large enough for the repeated working set. A tiny
FIFO cache can legitimately miss every repeated query; a miss always falls back
to exact calculation.

## Scientific limit

This engine performs exact deterministic calculations for tested inputs. It is
not a proof of the Collatz conjecture, and a sampled peak is not automatically a
global record.

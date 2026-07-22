# Release notes: LNL-LZR-CE-2

Release date: 2026-07-20  
Original author: Mike Lange

## Added

- Active-limb metadata for the fixed sixteen-limb U1024 representation.
- Exact completed-result cache mode, `engine-reuse`.
- Full-key collision checks and bounded FIFO cache behavior.
- Portable and CPU-native build targets.
- Cross-width verification from 128 through 900 bits.

## Retained

- `engine-fast` as the recommended mode for new non-repeating searches.
- `engine-cache` for exact intermediate trajectory merging.
- The independently verified 100,000-row embedded suffix table.

## Performance

- 48.09% lower median runtime for V2 native `engine-fast` versus the portable
  V1 reference on the recorded 333-bit workload.
- 1.926x throughput on the same comparison.
- 62.606x cold-to-warm speedup for exact completed-result reuse.

## Verification

- 100,000 embedded rows checked with zero failures.
- Zero cross-mode failures at 128, 333, 500, 768, and 900 bits.
- Identical V1/V2 candidate sequence and strongest result for 25,000 block-344
  starts.
- No reported address or undefined-behavior sanitizer errors in the recorded
  test environment.

## Rejected experiment

The tested profile-guided optimization build was slower than the native build
and is therefore documented but not recommended.

## Scientific limitation

This release establishes reproducible behavior for the tested inputs. It does
not prove the Collatz conjecture and does not establish a global peak record.


# LNL/LZR project diary

Copyright (c) 2026 Mike Lange. All rights reserved.

## 2026-07-20: lessons transferred from PeakQueen

- Preserved the established LNL/LZS project and treated LZR as its 1024-bit
  residue/search component.
- Applied the PeakQueen rule that only completed, exactly matching states may
  be reused.
- Kept residue-family classifications separate from exact cache identity.
- Added active-limb metadata to the fixed U1024 representation so arithmetic
  touches only occupied words.
- Added bounded FIFO exact-result reuse as `engine-reuse`.
- Kept `engine-fast` for new searches and the existing `engine-cache` for
  intermediate trajectory merging.
- Compiled V1 and V2 with equal portable flags for a fair A/B test.
- Accepted native CPU compilation after a measured improvement.
- Rejected the tested PGO build because it was slower than native.
- Verified all 100,000 embedded results and deterministic inputs from 128
  through 900 bits with zero failures.
- Confirmed V1 and V2 return the same block-344 candidate sequence and final
  strongest result for 25,000 starts.

## Next gate

Run V2 on the larger existing block corpus, record peak candidates and hashes
in the external reproducible database, and compare total throughput per block.
Only after those results should V2 replace V1 for new high-range searches.

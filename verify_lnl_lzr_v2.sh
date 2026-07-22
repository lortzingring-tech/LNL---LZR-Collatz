#!/usr/bin/env bash
# Copyright (c) 2026 Mike Lange. All rights reserved.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
engine="${1:-$root/search_lnl_1024_engine_native}"

if [[ ! -x "$engine" ]]; then
  engine="$($root/build_lnl_lzr_v2.sh native)"
fi

book="$($engine verify-book)"
grep -Fq '"opening_rows":100000' <<<"$book"
grep -Fq '"failures":0' <<<"$book"

for spec in "10000 128 20260731" "10000 333 20260728" "5000 500 20260731" \
            "2000 768 20260731" "1000 900 20260731"; do
  read -r samples bits block <<<"$spec"
  result="$($engine benchmark "$samples" "$bits" "$block" 500000)"
  grep -Fq '"verification_failures": 0' <<<"$result"
  grep -Fq '"exact_reuse_warm"' <<<"$result"
  grep -Fq '"evaluated_transitions": 0' <<<"$result"
done

replacement="$($engine benchmark 5000 333 20260728 257)"
grep -Fq '"verification_failures": 0' <<<"$replacement"

echo "LNL/LZR V2 verification passed: book=100000, widths=128/333/500/768/900, bounded-cache replacement=ok"

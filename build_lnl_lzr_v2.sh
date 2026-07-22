#!/usr/bin/env bash
# Copyright (c) 2026 Mike Lange. All rights reserved.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mode="${1:-portable}"

common=(-O3 -std=c++20 -Wall -Wextra -Wconversion -Wshadow)
case "$mode" in
  portable)
    output="$root/search_lnl_1024_engine_portable"
    extra=()
    ;;
  native)
    output="$root/search_lnl_1024_engine_native"
    extra=(-march=native -mtune=native -flto)
    ;;
  *)
    echo "usage: $0 [portable|native]" >&2
    exit 2
    ;;
esac

g++ "${common[@]}" "${extra[@]}" "$root/search_lnl_1024_engine.cpp" -o "$output"
echo "$output"

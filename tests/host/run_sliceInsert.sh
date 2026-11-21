#!/usr/bin/env bash
# Extracts insertSlicesAtPositions() from the real source and runs test_sliceInsert.cpp against it.
set -e
cd "$(dirname "$0")/../.."
tmp=$(mktemp -d)
awk '/^uint8_t mtSliceManager::insertSlicesAtPositions/{p=1} p{print} p&&/^}/{exit}' core/source/mtSliceManager.cpp > "$tmp/extracted_insert.inc"
g++ -std=c++17 -Wall -I"$tmp" tests/host/test_sliceInsert.cpp -o "$tmp/t" && "$tmp/t"

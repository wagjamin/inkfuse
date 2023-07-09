#!/bin/bash

if [[ $# -lt 1 ]]; then
  echo "Run as ./reproduce_inkfuse.sh <scale_factor> [<write_perf_events>]"
  exit 1
fi

# Change soft limit for open file descriptors.
# For background compilation in the hybrid backend we open a lot of those.
ulimit -n 100000

# Generate TPC-H data
./generate_tpch.sh $1

if [[ ! -f inkfuse_bench ]]; then
  # Build inkfuse_bench and copy into current directory
  cd ..
  mkdir tmp_build
  cd tmp_build
  cmake .. -DCMAKE_CXX_COMPILER=clang++-14 -DCMAKE_C_COMPILER=clang-14 -DCMAKE_BUILD_TYPE=Release
  make -j 16 inkfuse_bench
  cd ..
  cp tmp_build/inkfuse_bench reproduce
  rm -rf tmp_build
  cd reproduce
fi

if [[ $# -eq 1 ]]; then
    ./inkfuse_bench -scale_factor $1
fi
if [[ $# -eq 2 ]]; then
    ./inkfuse_bench -scale_factor $1 -perf_events $2
fi

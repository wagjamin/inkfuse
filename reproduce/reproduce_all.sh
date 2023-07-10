#!/bin/bash

# Install requirements.
pip3 install -r requirements.txt

# Benchmark Umbra
echo "Reproducing Umbra"
./reproduce_umbra.sh 0.01
./reproduce_umbra.sh 0.1
./reproduce_umbra.sh 1
./reproduce_umbra.sh 10

# Benchmark DuckDB
echo "Reproducing DuckDB"
./reproduce_duckdb.py 0.01
./reproduce_duckdb.py 0.1
./reproduce_duckdb.py 1
./reproduce_duckdb.py 10

# Benchmark InkFuse
echo "Reproducing InkFuse"
./reproduce_inkfuse.sh 0.01
./reproduce_inkfuse.sh 0.1
./reproduce_inkfuse.sh 1
./reproduce_inkfuse.sh 10

# Feel free to take a look if you want to, but not used in the experimental
# section of our paper.
# ./reproduce_hyperapi 0.01
# ./reproduce_hyperapi 0.1
# ./reproduce_hyperapi 1
# ./reproduce_hyperapi 10

# Generate Plots
echo "Generating Plots"
./plot.py

echo "Result plots can be found in `plots`"

echo "Performing Microbenchmark for CPU Counters"
./reproduce_inkfuse.sh 1 true
./analyze_microbenchmark.py

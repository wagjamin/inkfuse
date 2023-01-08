#!/bin/bash

# Install requirements.
pip3 install -r requirements.txt

# Benchmark DuckDB
echo "Reproducing DuckDB"
./reproduce_duckdb.py 0.01
./reproduce_duckdb.py 0.1
./reproduce_duckdb.py 1

# Benchmark InkFuse
echo "Reproducing InkFuse"
./reproduce_inkfuse.sh 0.01
./reproduce_inkfuse.sh 0.1
./reproduce_inkfuse.sh 1

# Feel free to take a look if you want to, but not used in the experimental
# section of our paper.
# ./reproduce_hyperapi 0.01
# ./reproduce_hyperapi 0.1
# ./reproduce_hyperapi 1

# Generate Plots
echo "Generating Plots"
./plot.py

echo "Result plots can be found in `plots`"

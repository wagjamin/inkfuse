# Benchmarking Inkfuse

This directory contains the required scripts to benchmark InkFuse on TPC-H, as well as run it against other systems.

As InkFuse does not support all SQL constructs for the raw TPC-H queries, the directory contains a simplified schema and slightly simplified queries that match the physical plans run by InkFuse in the `sql` directory.

## Reproducing Everything
To reproduce the experiments in the paper, simply run `./reproduce_all.sh` from this directory.
This creates:
- `results_<configuration>.csv` results files for the different engines and scale factors.
- `plots/main.pdf` containing a result figure.

## Reproducing Individual Systems
There are individual reproduction-scripts for all systems. These usually perform the following steps:
- Download the TPC-H `dbgen` tool and create correctly formatted data at the target scale factor
- Load the data into the target system 
- Run the queries and create result files

### InkFuse
To measure InkFuse at a given scale factor `<sf>`, simply run:
```
# Run at the given scale factor
./reproduce_inkfuse.sh <sf>
```
Note: you need to have `clang-14` and `clang++-14` installed for this script to run!
The script compiles the `inkfuse_bench` target from the source tree in the parent directory.
Note that InkFuse has no SQL interface at the moment, but only supports hard-coded physical plans.
The `inkfuse_bench` binary runs all supported TPC-H queries and creates result files containing the measurements.

### DuckDB
To measure DuckDB at a given scale factor `<sf>`, simply run:
```
# Install requirements
pip3 install -r requirements.txt
# Run at the given scale factor
./reproduce_duckdb.py <sf>
```

### Umbra
While part of the evaluation section of the paper, Umbra reproducibility infrastructure is currently missing from this repository.




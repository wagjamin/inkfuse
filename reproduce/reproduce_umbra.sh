#!/bin/bash

if [[ $# -lt 1 ]]; then
  echo "Run as ./reproduce_umbra.sh <scale_factor>"
  exit 1
fi

# All umbra data ends up here
mkdir umbra_data

# Maybe install umbra locally
if [[ ! -f umbra_data/umbra/bin/sql ]]; then
    cd umbra_data
    curl -L 'http://db.in.tum.de/~fent/umbra-2021-12-13.tar.xz' -o umbra.tar.xz
    tar -xf umbra.tar.xz
    rm umbra.tar.xz
    cd ..
fi

UMBRA_BIN=./umbra_data/umbra/bin/sql

# Setup database if it doesn't exist yet
if [[ ! -f umbra_data/tpch_${1} ]]; then
    # Generate TPC-H data for Umbra
    ./generate_tpch.sh $1 umbra
    echo "\i sql/schema.sql" | ${UMBRA_BIN} -createdb umbra_data/tpch_${1}
    echo "\i sql/load.sql" | ${UMBRA_BIN} umbra_data/tpch_${1}
fi

# Run queries
for q in 'q1' 'q3' 'q4' 'q5' 'q6' 'q13' 'q14' 'q18' 'q19' 'q_bigjoin'
do
    # LLVM JIT Compilation
    COMPILATIONMODE=o ${UMBRA_BIN} umbra_data/tpch_${1} `for i in $(seq 1 10); do echo sql/${q}.sql; done` | grep 'execution' > umbra_data/${q}_o_res_${1}.csv
    # Adaptive execution using the flying start backend
    COMPILATIONMODE=a ${UMBRA_BIN} umbra_data/tpch_${1} `for i in $(seq 1 10); do echo sql/${q}.sql; done` | grep 'execution' > umbra_data/${q}_a_res_${1}.csv
done

# Post-process umbra dumps
./postprocess_umbra.py


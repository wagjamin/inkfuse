#!/bin/bash

if [[ $# < 1 ]]; then
  echo "Run as ./generate_tpch.sh <scale factor> <system={duckdb|hyper|inkfuse|umbra}>"
  exit 1
fi

file_shuffle () {
  shuf $1.tbl > $1_shuffled.tbl
  mv $1_shuffled.tbl $1.tbl
}

# Clone TPC-H dbgen tool
git clone https://github.com/electrum/tpch-dbgen.git
cd tpch-dbgen
# Build it
make
# Generate data
./dbgen -s $1
# Move over .tbl files into separate directory
cd -
mkdir -p data
mv -f tpch-dbgen/*.tbl data
# Drop tpch-dbgen again
rm -rf tpch-dbgen

# Shuffle data.
cd data 
echo "Shuffling Generated Data"
file_shuffle "lineitem"
file_shuffle "orders"
file_shuffle "partsupp"
file_shuffle "part"
file_shuffle "customer"
cd -

# Normalize data for system ingest
if [[ $# -ge 2 ]]; then
  if [[ $2 == "duckdb" ]]; then
    echo "Cleaning up .tbl for DuckDB"
    # Remove trailing '|' which make this invalid CSV
    sed -i 's/.$//' data/*.tbl
  fi
  if [[ $2 == "umbra" ]]; then
    echo "Cleaning up .tbl for Umbra"
    # Remove trailing '|' which make this invalid CSV
    sed -i 's/.$//' data/*.tbl
  fi
  if [[ $2 == "hyper" ]]; then
    echo "Cleaning up .tbl for Hyper"
    # Remove trailing '|' which make this invalid CSV
    sed -i 's/.$//' data/*.tbl
  fi
fi

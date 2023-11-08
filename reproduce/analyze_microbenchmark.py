#! /usr/bin/python3
"""
Analyze the microbenchmark results that were created by the inkfuse_bench
target. Target creates per-query observations which are aggreagted here.

Normalization factors assume that everything was run at SF1.
"""

import argparse
import duckdb
import os
import pandas as pd


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Get a detailed report of the microbenchmark results.')
    parser.add_argument('scale_factor', type=str, nargs='?', default='1', help='The scale factor the microbenchmark was performed at.')
    args = parser.parse_args()
    
    f_name = f'inkfuse_bench_perf_result_{args.scale_factor}.csv'

    con = duckdb.connect(database=':memory:')
    con.execute(f"CREATE TABLE results AS SELECT * FROM read_csv_auto('{f_name}', header=True);")

    con.execute(
        f'CREATE TABLE aggregated AS SELECT backend, query, sf::INT as sf,' \
                ' avg(cycles) as cycles,' \
                ' avg(instructions) as instructions,' \
                ' avg("L1-misses") as l1_misses,' \
                ' avg("LLC-misses") as llc_misses,' \
                ' avg("branch-misses") as branch_misses,' \
                ' avg("IPC") as ipc,' \
        f"FROM results " \
        f"GROUP BY backend, query, sf " \
        f"ORDER BY query")

    con.execute("CREATE TABLE normalizer(query text, tuples bigint);")

    # Normalization Factor Calculations - Taking Card. Esimates from Umbra at SF1, and summing
    # up cardinality per relational algebra operator.
    # Q1: 6 Million table scan 
    # Q3: 8 Million table scan
    # Q4: 7.5 Million table scan
    # Q6: 6 Million table scan
    # Q14: 6.2 Million table scan 
    con.execute(f"INSERT INTO normalizer VALUES ('q1', {args.scale_factor} * 600000), ('q3', {args.scale_factor} * 800000), ('q4', {args.scale_factor} * 750000), ('q6', {args.scale_factor} * 600000), ('q14', {args.scale_factor} * 620000);")

    res = con.execute(
            "SELECT ltrim(backend) as backend, n.query as query, " \
            " round(cycles/(r.sf*tuples), 2) as cycles, "
            " round(instructions/(r.sf*tuples), 2) as instructions, " \
            " round(l1_misses/(r.sf*tuples), 2) as l1_misses, " \
            " round(llc_misses/(r.sf*tuples), 2) as llc_misses, " \
            " round(branch_misses/(r.sf*tuples), 2) as branch_misses, " \
            " round(ipc, 2) as ipc " \
            " FROM aggregated r INNER JOIN normalizer n ON (ltrim(r.query) = n.query) "  \
            " WHERE ltrim(backend) != 'hybrid'" \
            " ORDER BY n.query, ltrim(backend)"
        ).fetchdf()

    print(res.to_csv())


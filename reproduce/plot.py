#! /usr/bin/python3

import duckdb
import os
import numpy as np
import matplotlib.pyplot as plt

systems = ['duckdb', 'umbra_adaptive', 'umbra_optimized', 'inkfuse_fused', 'inkfuse_interpreted', 'inkfuse_hybrid']
label_map = {
    'duckdb': 'DuckDB',
    'umbra_adaptive': 'Umbra (Hybrid)',
    'umbra_optimized': 'Umbra (LLVM)',
    'inkfuse_fused': 'Inkfuse (Compiling)',
    'inkfuse_interpreted': 'Inkfuse (Vectorized)',
    'inkfuse_hybrid': 'Inkfuse (Hybrid)',
}
scale_factors = ['0.01', '0.1', '1', '10']

if __name__ == '__main__':
    con = duckdb.connect(database=':memory:')
    con.execute("CREATE TABLE results (engine text, query text, sf text, latency int);")

    # Insert engine results
    for engine in systems:
        for sf in scale_factors:
            f_name = f'result_{engine}_{sf}.csv'
            con.execute(f"INSERT INTO results SELECT * FROM read_csv_auto('{f_name}', header=False)")
            con.execute(f"DELETE FROM results WHERE query in ('l_count', 'l_point')")

    queries = ['Q1', 'Q3', 'Q4', 'Q6']

    # Plot 1: Backends at Different Scale Factors
    fig, axs = plt.subplots(1, 4)
    fig.set_size_inches(20, 5)
    x_vals = np.array([0, 1.5, 3, 4.5])
    for idx, sf in enumerate(scale_factors):
        offset = -0.5
        for engine in systems:
            res = con.execute(
                f"SELECT query, engine, min(latency) as latency "
                f"FROM results WHERE sf = '{sf}' and engine = '{engine}' "
                f"GROUP BY query, engine "
                f"ORDER BY query").fetchnumpy()
            # If we completely cut the x bars it looks like there is no data.
            # Some queries on sf0.01 do take 0 ms (rounded). Put them at 0.5 nevertheless to show there is data.
            # Note that xmax on that plot lies around 50, so this is still representative.
            res['latency'] = np.maximum(res['latency'], [0.5])
            axs[idx].bar(x_vals + offset, res['latency'], width=0.2, label=label_map[engine] if idx == 0 else "")
            axs[idx].set_xticks(x_vals, queries)
            axs[idx].set_ylabel('Latency [ms]')
            axs[idx].set_xlabel('TPC-H Query')
            axs[idx].set_title(f'Scale Factor {sf}')
            offset += 0.2
    fig.legend(loc='upper center', ncol=len(systems), fancybox=True)
    # plt.show()
    os.makedirs('plots', exist_ok=True)
    plt.savefig('plots/main.pdf', dpi=300)

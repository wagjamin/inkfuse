#! /usr/bin/python3

import duckdb
import os
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

matplotlib.rcParams.update({'font.size': 24})

systems = ['inkfuse_fused', 'inkfuse_rof', 'inkfuse_interpreted', 'inkfuse_hybrid']
# Colors of the different systems 
# DuckDB orange
# colors = ['#df6f00', '#c5d31f', '#8d9511','#daa6f5','#b056d0', '#782993']
# DuckDB blue
colors = ['#daa6f5','#b056d0', '#782993', '#531c66']
global_hatch = '///'

# Legend labels of the different systems
label_map = {
    'inkfuse_fused': 'Compiling',
    'inkfuse_interpreted': 'Vectorized',
    'inkfuse_hybrid': 'Hybrid',
    'inkfuse_rof': 'ROF',
}

scale_factors = ['100']

if __name__ == '__main__':
    con = duckdb.connect(database=':memory:')
    con.execute("CREATE TABLE results (engine text, query text, sf text, latency int, codegen_stalled int);")

    # Insert engine results
    for engine in systems:
        for sf in scale_factors:
            f_name = f'result_{engine}_{sf}.csv'
            con.execute(f"INSERT INTO results SELECT * FROM read_csv_auto('{f_name}', header=False)")
            if 'inkfuse' in engine:
                # TMP hack to allow reading from multiple inkfuse runs
                con.execute(f"INSERT INTO results SELECT * FROM read_csv_auto('res_inkfuse/{f_name}', header=False)")
            con.execute(f"DELETE FROM results WHERE query in ('l_count', 'l_point', 'q_bigjoin')")
            # Kinda nasty, change ordering so Q1x come last.
            con.execute(f"UPDATE results SET query = case when query = 'q13' then 'q87' else query end")
            con.execute(f"UPDATE results SET query = case when query = 'q14' then 'q88' else query end")
            con.execute(f"UPDATE results SET query = case when query = 'q18' then 'q98' else query end")
            con.execute(f"UPDATE results SET query = case when query = 'q19' then 'q99' else query end")

    queries = ['Q1', 'Q3', 'Q4', 'Q5', 'Q6', 'Q13', 'Q14', 'Q19']

    # Plot 1: Backends at Different Scale Factors
    fig, axs = plt.subplots(1, 1)
    fig.set_size_inches(16, 5.5)
    plt.axhline(y=1.0, color='black', linestyle='dashed', linewidth=2.0)
    plt.set_cmap('Set3')
    axs.set_ylim(bottom=0.6)
    axs.set_ylim(top=1.5)
    x_vals = np.array([0, 1.5, 3, 4.5, 6, 7.5, 9, 10.5])
    for idx, sf in enumerate(scale_factors):
        offset = -0.25
        for engine_idx, engine in enumerate(systems):
            # Get the queries with minimal latency for the different queries and engines.
            res_baseline = con.execute(
                f"SELECT query, engine, first(latency) as latency, first(codegen_stalled) as codegen_stalled "
                f"FROM results o WHERE sf = '{sf}' and engine = 'inkfuse_interpreted' "
                f"  and latency = (SELECT min(latency) FROM results i WHERE sf = '{sf}' and engine = 'inkfuse_interpreted' and i.query = o.query)"
                f"GROUP BY query, engine "
                f"ORDER BY query").fetchnumpy()
            res = con.execute(
                f"SELECT query, engine, first(latency) as latency, first(codegen_stalled) as codegen_stalled "
                f"FROM results o WHERE sf = '{sf}' and engine = '{engine}' "
                f"  and latency = (SELECT min(latency) FROM results i WHERE sf = '{sf}' and engine = '{engine}' and i.query = o.query)"
                f"GROUP BY query, engine "
                f"ORDER BY query").fetchnumpy()
            if len(res['latency']) != 8:
                print(f'missing {engine}, {sf}')
            bar_color = len(queries) * [colors[engine_idx]]
            non_stalled = 1 / ((res['latency'] - res['codegen_stalled'] / 1000) / (res_baseline['latency'] - res_baseline['codegen_stalled'] / 1000))
            # Seconds
            # axs.bar(x_vals + offset, stalled, width=0.2, color=bar_color, edgecolor= 'black', hatch = global_hatch)
            axs.bar(x_vals + offset, non_stalled, width=0.2, label=label_map[engine] if idx == 0 else "", color=bar_color, edgecolor = 'black')
            axs.set_xticks(x_vals, queries)
            axs.set_ylabel('Normalized Throughput', fontsize=24)
            axs.set_title(f'TPC-H Scale Factor {sf}', y=0.88)
            offset += 0.2
    # HACK - Add empty bar to just get the legend to contain compliation latency
    # axs.bar(x_vals + offset - 0.19, [0, 0, 0, 0, 0, 0, 0, 0], bottom=[0, 0, 0, 0, 0, 0, 0, 0], width=0.00001, label='Compilation Latency', hatch = global_hatch, color='white', edgecolor = 'black')
    fig.legend(loc='upper center', ncol=len(systems) + 1, fancybox=True, labelspacing=0.1, handlelength=1.8, handletextpad=0.4, columnspacing=1.0)
    # 4. Space legend (actually okay)
    plt.subplots_adjust(top=0.83)
    # plt.show()
    os.makedirs('plots', exist_ok=True)
    plt.savefig('plots/main_inkfuse.pdf', bbox_inches='tight', dpi=300)

#! /usr/bin/python3

import duckdb
import os
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

matplotlib.rcParams.update({'font.size': 20})

systems = ['duckdb', 'umbra_optimized', 'umbra_adaptive', 'inkfuse_fused', 'inkfuse_rof', 'inkfuse_interpreted', 'inkfuse_hybrid']
# Colors of the different systems 
# DuckDB orange
# colors = ['#df6f00', '#c5d31f', '#8d9511','#daa6f5','#b056d0', '#782993']
# DuckDB blue
colors = ['#04718a', '#c5d31f', '#8d9511','#daa6f5','#b056d0', '#782993', '#531c66']
global_hatch = '///'

# Legend labels of the different systems
label_map = {
    'duckdb': 'DuckDB',
    'umbra_adaptive': 'Umbra (Hybrid)',
    'umbra_optimized': 'Umbra (LLVM)',
    'inkfuse_fused': 'Inkfuse (Compiling)',
    'inkfuse_interpreted': 'Ink (Vectorized)',
    'inkfuse_hybrid': 'Ink (Hybrid)',
    'inkfuse_rof': 'Ink (ROF)',
}

# If you're running up to SF 10
# scale_factors = ['0.01', '0.1', '1', '10']
# If you're running up to SF 100
scale_factors = ['0.1', '1', '10', '100']

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
            con.execute(f"DELETE FROM results WHERE query in ('l_count', 'l_point', 'q_bigjoin', 'q18')")
            # Kinda nasty, change ordering so Q1x come last.
            con.execute(f"UPDATE results SET query = case when query = 'q13' then 'q87' else query end")
            con.execute(f"UPDATE results SET query = case when query = 'q14' then 'q88' else query end")
            con.execute(f"UPDATE results SET query = case when query = 'q19' then 'q99' else query end")

    queries = ['Q1', 'Q3', 'Q4', 'Q5', 'Q6', 'Q13', 'Q14', 'Q19']

    # Plot 1: Backends at Different Scale Factors
    gridspec = dict(height_ratios=[3, 3])
    fig, axs = plt.subplots(2, 2, sharex=True, gridspec_kw=gridspec)
    fig.set_size_inches(22, 7.5)
    plt.set_cmap('Set3')
    x_vals = np.array([0, 1.5, 3, 4.5, 6, 7.5, 9, 10.5])
    for idx, sf in enumerate(scale_factors):
        pos_y = idx % 2
        pos_x = idx // 2
        offset = - (3.5 * 0.18)
        for engine_idx, engine in enumerate(systems):
            # Get the queries with minimal latency for the different queries and engines.
            res = con.execute(
                f"SELECT query, engine, first(latency) as latency, first(codegen_stalled) as codegen_stalled "
                f"FROM results o WHERE sf = '{sf}' and engine = '{engine}' "
                f"  and latency = (SELECT min(latency) FROM results i WHERE sf = '{sf}' and engine = '{engine}' and i.query = o.query)"
                f"GROUP BY query, engine "
                f"ORDER BY query").fetchnumpy()
            if len(res['latency']) != 8:
                print(f'missing {engine}, {sf}')
            print(res['query'])
            # If we completely cut the x bars it looks like there is no data.
            # Some queries on sf0.01 do take 0 ms (rounded). Put them at 1 nevertheless to show there is data.
            # Note that xmax on that plot lies around 50, so this is still representative.
            res['latency'] = np.maximum(res['latency'], [1.5])
            # Set the color of the current engine
            bar_color = len(queries) * [colors[engine_idx]]
            if (pos_x == 0):
                # Milliseconds
                stalled = res['codegen_stalled'] / 1000
                non_stalled = res['latency'] - stalled
                axs[pos_x, pos_y].bar(x_vals + offset, stalled, width=0.18, color=bar_color, edgecolor= 'black', hatch = global_hatch)
                axs[pos_x, pos_y].bar(x_vals + offset, non_stalled, bottom=stalled, width=0.18, label=label_map[engine] if idx == 0 else "", color=bar_color, edgecolor= 'black')
            else:
                stalled = res['codegen_stalled'] / 1000000
                non_stalled = res['latency'] / 1000 - stalled
                # Seconds
                axs[pos_x, pos_y].bar(x_vals + offset, stalled, width=0.18, color=bar_color, edgecolor= 'black', hatch = global_hatch)
                axs[pos_x, pos_y].bar(x_vals + offset, non_stalled, bottom=stalled, width=0.18, label=label_map[engine] if idx == 0 else "", color=bar_color, edgecolor = 'black')
            axs[pos_x, pos_y].set_xticks(x_vals, queries)
            if (pos_y == 0 and pos_x == 0):
                axs[pos_x, pos_y].set_ylabel('Latency [ms]')
            if (pos_y == 0 and pos_x == 1):
                axs[pos_x, pos_y].set_ylabel('Latency [s]')
            # if (pos_x == 1):
                # axs[pos_x, pos_y].set_xlabel('TPC-H Query')
            axs[pos_x, pos_y].set_title(f'  TPC-H Scale Factor {sf}', loc='left', y=0.85)
            axs[pos_x, pos_y].locator_params(axis='y', nbins=6) 
            offset += 0.18
    # HACK - Add empty bar to just get the legend to contain compliation latency
    axs[0, 0].bar(x_vals + offset - 0.19, [0, 0, 0, 0, 0, 0, 0, 0], bottom=[0, 0, 0, 0, 0, 0, 0, 0], width=0.00001, label='Compilation Latency', hatch = global_hatch, color='white', edgecolor = 'black')
    fig.legend(loc='upper center', ncol=len(systems) + 1, fancybox=True, labelspacing=0.1, handlelength=1.8, handletextpad=0.4, columnspacing=1.0)
    # Different style things
    # 1. Aligned y labels
    fig.align_ylabels(axs[:, 0])
    # 2. Better figure spacing
    plt.subplots_adjust(wspace=0.065, hspace=0.09)
    # 3. Nicer colors
    # 4. Space legend
    plt.subplots_adjust(top=0.90)
    plt.subplots_adjust(left=0.01, right=0.99)
    # plt.show()
    os.makedirs('plots', exist_ok=True)
    plt.savefig('plots/main.pdf', bbox_inches='tight', dpi=300)

#! /usr/bin/python3

import os
import re

backends = {'a': 'adaptive', 'o': 'optimized'}

for file in os.listdir('umbra_data'):
    if '_res_' in file:
        print(f'Postprocessing {file}')
        matched = re.search('^([q0-9]*)_([ao])_res_([0-9.]*).csv', file)
        q_name = matched.group(1)
        mode = matched.group(2)
        sf = matched.group(3).replace('_', '.')
        print(f'{q_name} {mode} {sf}')
        with open(f'umbra_data/{file}') as f:
            with open(f'result_umbra_{backends[mode]}_{sf}.csv', 'a') as out:
                for line in f:
                    times = re.search('execution: \(([0-9.]*).*compilation: \(([0-9.]*)', line)
                    # Seconds -> Milliseconds
                    time = int(1000 * (float(times.group(1)) + float(times.group(2))))
                    # Seconds -> Microseconds
                    stalled = 1000 * 1000 * float(times.group(2))
                    if sf == '0.1' and mode == 'a':
                        print(time)
                    # print(f'{q_name} {time}')
                    out.write(f'umbra_{backends[mode]},{q_name},{sf},{time},{stalled}\n')

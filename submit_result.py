#!/usr/bin/env python3
""" Leaderboard result submitter

Leaderboard is located here https://leaderboard.db.in.tum.de/

This script is called by gitlab-ci and executed on a dedicated machine.
It executes your program, finds result in the output using regexp and
sends results to the leaderboard.

Feel free to edit regexp expressions if necessary.
The system is easy to cheat, but we hope that you respect a fair competition.
Your program have to accept path to tpcc_5w dir as a command line argument.

Uncomment the task you want to execute in the bottom of the file.
You can test if it works like this
`python3 submit_result.py <path to dir containing tpcc_5w>`
"""
import http.client
import json
import os
import sys
import subprocess
import re


def send_result(competition_id, secret, result):
    data = {
        'competition_slug': competition_id,
        'secret': secret,
        'commit_hash': os.environ.get('CI_COMMIT_SHA', ''),
        'result': result,
        'user_name': os.environ.get('GITLAB_USER_EMAIL', ''),
        'gitlab_user': os.environ.get('GITLAB_USER_EMAIL', ''),
    }
    if data['gitlab_user'] == '':
        print('Local run mode. Results are not sent to the leaderboard.')
        return

    host = 'leaderboard.db.in.tum.de'
    conn = http.client.HTTPSConnection(host=host)
    conn.request(
        method='POST',
        url='/api/submission/',
        headers={'Content-type': 'application/json'},
        body=json.dumps(data))
    response = conn.getresponse()
    if response.status != 201:
        sys.stderr.write('Leaderboard server returned error:\n')
        sys.stderr.write(response.read().decode('utf8'))
        exit(1)
    conn.close()
    print('Result accepted by leaderboard server')


def imlab_1_task(data_path):
    parameters = ['./imlabdb', data_path]
    command = ' '.join(parameters)
    print(f'Run benchmark with the command "{command}"')
    process = subprocess.run(parameters, capture_output=True)
    assert process.returncode == 0, f'Error code {process.returncode} returned'
    output = process.stdout.decode('utf8')

    # Find transactions/second using the regexp (change if necessary)
    m = re.search(r'TPS: ([\d,\.,e,\+]+)', output)
    assert m is not None, 'Unexpected output:\n' + output
    result = float(m.group(1))
    print('Result', result)
    send_result('imlab21-task1', 'd2HU3TnGJM', result)


def imlab_3_task(data_path):
    parameters = ['./imlabdb', data_path]
    command = ' '.join(parameters)
    print(f'Run benchmark with the command "{command}"')
    process = subprocess.run(parameters, capture_output=True)
    assert process.returncode == 0, f'Error code {process.returncode} returned'
    output = process.stdout.decode('utf8')

    # Find OLAP run time using the regexp (change if necessary)
    m = re.search(r'join duration: ([\d,\.,e,\+]+)', output)
    assert m is not None, 'Unexpected output:\n' + output
    result = float(m.group(1))
    print('Result', result, 'TPS')
    send_result('imlab21-task3-2', 'd2HU3TnGJM', result)

    # Find transactions/second using the regexp (change if necessary)
    m = re.search(r'TPS: ([\d,\.,e,\+]+)', output)
    assert m is not None, 'Unexpected output:\n' + output
    result = float(m.group(1))
    print('Result', result, 'TPS')
    send_result('imlab21-task3-1', 'd2HU3TnGJM', result)


def imlab_6_task(data_path):
    parameters = ['./imlabdb', data_path]
    command = ' '.join(parameters)
    print(f'Run benchmark with the command "{command}"')
    process = subprocess.run(parameters, capture_output=True)
    assert process.returncode == 0, f'Error code {process.returncode} returned'
    output = process.stdout.decode('utf8')

    # Find queries/second using the regexp (change if necessary)
    m = re.search(r'QPS: ([\d,\.,e,\+]+)', output)
    assert m is not None, 'Unexpected output:\n' + output
    result = float(m.group(1))
    print('Result', result)
    send_result('imlab21-task6', 'd2HU3TnGJM', result)

if __name__ == "__main__":
    assert len(sys.argv) == 2, 'Pass a path to the data as a single parameter'
    base_dir = sys.argv[1]
    # Uncomment the task you want to bench and send to the leaderboard
    # imlab_1_task(base_dir + '/tpcc_5w')
    imlab_3_task(base_dir + '/tpcc_5w')
    # imlab_6_task(base_dir + '/tpcc_5w')

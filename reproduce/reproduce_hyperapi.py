#! /usr/bin/python3

from tableauhyperapi import HyperProcess, Connection, TableDefinition, SqlType, Telemetry, Inserter, CreateMode, \
    NOT_NULLABLE
import argparse
import subprocess
import os
import time

# Reproducibility script running our sample workload on the Hyper API.
# For anyone who wants to play around with a really, really fast compiling system,
# this should be a great starting point.
# Sadly it's not super easy to use the HyperAPI for measurements in a paper.
# For the InkFuse evaluation, we need a compiling system where we can configure
# worker threads as well as the backend that's used.

# Didn't find how to do a CREATE TABLE with the Hyper API
table_lineitem = TableDefinition(
    table_name='lineitem',
    columns=[
        TableDefinition.Column("l_orderkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("l_partkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("l_suppkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("l_linenumber", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("l_quantity", SqlType.double(), NOT_NULLABLE),
        TableDefinition.Column("l_extendedprice", SqlType.double(), NOT_NULLABLE),
        TableDefinition.Column("l_discount", SqlType.double(), NOT_NULLABLE),
        TableDefinition.Column("l_tax", SqlType.double(), NOT_NULLABLE),
        TableDefinition.Column("l_returnflag", SqlType.char(1), NOT_NULLABLE),
        TableDefinition.Column("l_linestatus", SqlType.char(1), NOT_NULLABLE),
        TableDefinition.Column("l_shipdate", SqlType.date(), NOT_NULLABLE),
        TableDefinition.Column("l_commitdate", SqlType.date(), NOT_NULLABLE),
        TableDefinition.Column("l_receiptdate", SqlType.date(), NOT_NULLABLE),
        TableDefinition.Column("l_shipinstruct", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("l_shipmode", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("l_comment", SqlType.text(), NOT_NULLABLE),
    ]
)

table_customer = TableDefinition(
    table_name='customer',
    columns=[
        TableDefinition.Column("c_custkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("c_name", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("c_address", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("c_nationkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("c_phone", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("c_acctbal", SqlType.double(), NOT_NULLABLE),
        TableDefinition.Column("c_mktsegment", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("c_comment", SqlType.text(), NOT_NULLABLE),
    ]
)

table_orders = TableDefinition(
    table_name='orders',
    columns=[
        TableDefinition.Column("o_orderkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("o_custkey", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("o_orderstatus", SqlType.char(1), NOT_NULLABLE),
        TableDefinition.Column("o_totalprice", SqlType.double(), NOT_NULLABLE),
        TableDefinition.Column("o_orderdate", SqlType.date(), NOT_NULLABLE),
        TableDefinition.Column("o_orderpriority", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("o_clerk", SqlType.text(), NOT_NULLABLE),
        TableDefinition.Column("o_shippriority", SqlType.int(), NOT_NULLABLE),
        TableDefinition.Column("o_comment", SqlType.text(), NOT_NULLABLE),
    ]
)


def load_query(q_name):
    with open(f'sql/{q_name}.sql') as query:
        return query.read()


def set_up_schema(con: Connection):
    print('Setting up schema')
    con.catalog.create_table(table_definition=table_lineitem)
    con.catalog.create_table(table_definition=table_orders)
    con.catalog.create_table(table_definition=table_customer)


def load_data(con: Connection):
    tables = ['lineitem', 'orders', 'customer']
    for table in tables:
        print(f'Loading {table}')
        con.execute_command(f"COPY {table} FROM 'data/{table}.tbl' WITH (format csv, delimiter '|')")


def run_query(con: Connection, query):
    query = load_query(query)
    t_start = time.time()
    result = con.execute_query(query)
    result.close()
    t_end = time.time()
    return int(1000 * (t_end - t_start))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Benchmark HyperAPI')
    parser.add_argument('scale_factor', type=str, help='The factor to run the experiment on')
    parser.add_argument('--repeat', type=int, default=10, help='How often should each query be run?')
    parser.add_argument('--no-regen', dest='regen', action='store_false', help="Don't regenerate data")
    parser.set_defaults(regen=True)
    args = parser.parse_args()

    if args.regen:
        # Generate the data. Trailing | generated by dbgen are stripped.
        ret = subprocess.run(['bash', 'generate_tpch.sh', args.scale_factor, 'hyper'])
        assert ret.returncode == 0

    # Set up the local Hyper process
    with HyperProcess(telemetry=Telemetry.SEND_USAGE_DATA_TO_TABLEAU) as hyper:
        # Connect to the Hyper endpoint
        with Connection(endpoint=hyper.endpoint,
                        database='tpch.hyper',
                        create_mode=CreateMode.CREATE_AND_REPLACE) as connection:

            # Set up the schema.
            set_up_schema(connection)
            load_data(connection)

            with open(f'result_hyper_{args.scale_factor}.csv', 'w') as results:
                # Run the queries.
                for file in os.listdir('sql'):
                    if file != 'schema.sql':
                        q_name = file[:-4]
                        print(f'Running query {q_name}')
                        for _ in range(args.repeat):
                            dur = run_query(connection, q_name)
                            results.write(f'hyper,{q_name},{args.scale_factor},{dur}\n')

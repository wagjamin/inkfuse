# IMLAB

You can call my imlabdb executable as follows:
````
./imlabdb --include_dir <path_to_dbimpl-tasks_include_dir>
````
We need the include_dir argument in this case because otherwise the generated code does not know
where to find the imlab headers needed for code generation.

You then get a nice REPL in which you can send queries.
If you want to look at the generated code, it is put into `/tmp/imlab_query_<idx>.[cc|h]`. Enjoy!

Final note: lint jobs in the CI are failing, but I decided to accept this.
These are pending TODOs and other minor things which are IMO not important.


## Task 1 - Performance Evaluation
This README contains a quick performance breakdown of the numbers we are seeing for the NewOrder transactions.
I decided to go with a column-store style engine. 

I'm still in Seattle running on a Laptop with an AMD Ryzen 7 PRO 4750U.
Not a great CPU, it's okay and has 16 threads, but every core is not so fast.
So we shouldn't expect any world-breaking numbers.

When running 10k transactions here's a perf stat breakdown:
````
 Performance counter stats for './imlabdb':

         34.269,05 msec task-clock                #    0,999 CPUs utilized
             3.670      context-switches          #    0,107 K/sec
                49      cpu-migrations            #    0,001 K/sec
           156.066      page-faults               #    0,005 M/sec
   138.354.680.144      cycles                    #    4,037 GHz                      (83,32%)
       602.832.969      stalled-cycles-frontend   #    0,44% frontend cycles idle     (83,34%)
    14.184.137.391      stalled-cycles-backend    #   10,25% backend cycles idle      (83,34%)
   570.622.240.423      instructions              #    4,12  insn per cycle
                                                  #    0,02  stalled cycles per insn  (83,34%)
   169.261.179.967      branches                  # 4939,184 M/sec                    (83,33%)
         6.447.946      branch-misses             #    0,00% of all branches          (83,33%)

      34,312612350 seconds time elapsed

      34,063539000 seconds user
       0,195698000 seconds sys
````
This puts at roughly 3.4 milliseconds per transaction. Considering that the scans have to read through quite some data
this actually seems like a fine performance number.
In 3.4 ms we can fetch about 130 MB of data (considering 40GB/s throughput) from main memory, which seems quite reasonable here.
The tables are quite big and large tables like stock must be scanned multiple times.

We can nicely see that we also get a very high IPC count which is great, we are able to use our CPU pipeline almost perfectly.
Almost no frontend stalls because of branch misses etc. A few backend cycles lost in cases where we are memory bound I assume.

Honestly I still find the performance a bit depressing, I thought we would be way faster.
But my back-of-the-envelope calculations yield that that's probably as good as it gets for this scenario on my laptop ...

Results after a run of 10k queries:
````
uples in table district: 50
Tuples in table order: 150000
Tuples in table neworder: 45000
Tuples in table stock: 500000
Tuples in table orderline: 1425564
-------------- Running -----------------
-------------- Done-----------------
10 Thousand transactions took 33607 milliseconds
Resulting in the following tps 2975.57
Tuples in updated table district: 50
Tuples in updated table order: 160000
Tuples in updated table neworder: 55000
Tuples in updated table stock: 500000
Tuples in updated table orderline: 1525774
````
We can see that the numbers look fine.
Inserts seem to be progressing correctly.

Going to a million queries now is just a waiting game, it would probably take way longer than 10x since
the tables grow so much larger ...

## Task 2 - Result
The generated schema for TPC-C can be found in `include/gen/tpcc.h` and `src/gen/tpcc.cc`.
When running the one million transactions we get the following results:

````
Tuples in table district: 50
Tuples in table order: 150000
Tuples in table neworder: 45000
Tuples in table stock: 500000
Tuples in table orderline: 1425564
-------------- Running -----------------
-------------- Done-----------------
1 Million transactions took 47382 milliseconds
TPS: 21105.1
Tuples in updated table district: 50
Tuples in updated table order: 1150000
Tuples in updated table neworder: 1045000
Tuples in updated table stock: 500000
Tuples in updated table orderline: 11426102
````

Why is this so much faster than last week? 
That's beacuse I now have secondary primary-key indexes speeding up the workload significantly.

## Task 3 - Result
I boosted my bad implementation from last week by a bit and got to around 38k new_order TPS.
I did this by changing some of the excessive hash table lookups I did to resolve column names.
Rather than going through #col hash table lookups to re-construct a row I now go directly into the backing vector.
This was an oversight last week I didn't think of initially and then had no time to fix.

Then I found a bug and had to see the TPS drop to about 23k again :-(
I wasn't constructing the secondary indices correctly leading to wrong resuls.

Why is this so much worse than the others on the leaderboard? The core reason is the fact that my column store
leads to excessive cache misses, for every random row read I incur #columns cache misses.
This leads to way worse overall performance because I have to reconstruct the tuples in an inefficient way.
In the future I want to switch over to a row-store layout and see if I can beat the others.

I then implemented the delivery transaction, with the mix outlined in the task I get a TPS of about 12k.
So the delivery transactions in the mix tend to be heavier than the neworder ones.

I then implemented the OLAP query and verified the results - all worked well.
The concurrent OLAP queries added to the mix don't change a lot about the TPS since they are executed on a second
thread and my machine is able to scale well in this case.

What if we switch to the lazy hash table?
````
-------------- Running Raw Join Performance -----------------
STL Join OLAP Result: 1367872001.25
Total join STL duration: 0.33
LHT Join OLAP Result: 1367872001.25
Total join duration: 0.212
-------------- Done-----------------
````

We can see that the join performance with the lazy hash table is improved by more than 50%.

## Task 5 - Result

I spent tons of time on this week and overall I must say I am quite happy with the code I wrote.
I think my codegen logic is quite nice and the code is quite clean.

The speed might not be perfect, but regarding clarity I think this is fine considering it was the first time I built something like this.

## Task 6 - Result

I started implementing the TBB solution for this assignment.
However, there is one subtle bug I was not yet able to resolve entirely.
During multi-threaded printing for longer character strings, my implementation starts making mistakes and producing weird artifacts.
I will try fixing them over the next week just for my own sake.

Code generation for multi-threaded stuff can be turned off using the USE_TBB flag in Settings.h. 
If you want to see my generated tbb code, just enable the flag.
If you want to see my single-threaded code, just disable the flag.

Overall I am happy with the code I generated and I also fixed the IU declaration in the global scope andre pointed out in the tutorial.
Thanks for the fun learning experience building a small code-generated engine! 

## Task 1

* Implement the TPC-C [New Order](/data/new_order.sql) transaction and storage for all [relations](/data/schema.sql) as a standalone C++ program.
* Implement the [load procedures](/include/imlab/database.h) to populate the database with the data provided in the [\*.tbl](https://db.in.tum.de/teaching/ws2122/imlab/tpcc_5w.tar.gz) files.
* Translate the *New Order* transaction to C++ using the [mapping](/include/imlab/infra/types.h) from SQL to C data types.
    * **integer**: int32_t
    * **char**, **varchar**: length plus character array (do not use pointers or std::string)
    * **timestamp**: uint64_t (seconds since 01.01.1970)
    * **numeric(x, y)**: int64_t<br/>
    (x is the total number of digits, y is number of digits after the decimal separator, for example numeric(10,2) x=12.3 is represented as 1230)
* Implement the [New Order](/data/new_order.sql) transaction as [C++ function](/include/imlab/database.h).
* Execute the *New Order* transaction 1 million times with [random input](/tools/imlabdb.cc).
* Print the number of tuples in the tables before and after executing the transactions.

Useful files:
* [CMake configuration (CMakeLists.txt)](/CMakeLists.txt)
* [Type header (include/imlab/infra/types.h)](/include/imlab/infra/types.h)
* [Key hashing (include/imlab/infra/key.h)](/include/imlab/infra/hash.h)
* [Database header (include/imlab/database.h)](/include/imlab/database.h)
* [Database defs (src/database.cc)](/src/database.cc)
* [Database tests (test/database_test.cc)](/test/database_test.cc)
* [Executable (tools/imlabdb.cc)](/tools/imlabdb.cc)

Useful links:
* [TPC-C website](http://www.tpc.org/tpcc/)
* [TPC-C specification](http://www.tpc.org/tpc_documents_current_versions/pdf/tpc-c_v5.11.0.pdf)
* [TPC-C data (5 warehouses)](https://db.in.tum.de/teaching/ws2122/imlab/tpcc_5w.tar.gz)

## Task 2

* Use *bison* and *flex* to write a parser for SQL schema files.
    * You can assume, that all attributes are declared NOT NULL.
    * You must support primary keys.
    * You can ignore secondary indexes (for now).
* Write a schema compiler, that generates C++ code from the [schema representation](include/imlab/schemac/schema_parse_context.h).
    * You must implement the **C**reate, **R**ead, **U**pdate, **D**elete operations for every relation.
    * It must further be possible to lookup a tuple by its primary key (if any).
    * Use the schema compiler to generate code for the [schema](data/schema.sql) and add it to your repository.
* Re-implement the [New Order](data/new_order.sql) transaction as [C++ function](include/imlab/database.h) using your generated code.

The project skeleton for this task contains a grammar & scanner for a toy format called **FOO**, which looks as follows:

```
foo bar1 {
    a integer,
    b char{20}
};

foo bar2 {
    c integer
};
```

You can use the executable `schemac` to parse a **FOO** file with the following command:
```
./schemac --in ~/Desktop/example.foo --out_cc /tmp/example.cc --out_h /tmp/example.h
```

First learn how bison & flex work together and how they are used in the following files:
* [FOO grammar](tools/schemac/schema_parser.y)
* [FOO scanner](tools/schemac/schema_scanner.l)
* [CMake integration](tools/schemac/local.cmake)
* [Parse context H](include/imlab/schemac/schema_parse_context.h)
* [Parse context CC](tools/schemac/schema_parse_context.cc)
* [Parser tests](tools/schemac/tester.cc)

Then use the parsed schema to generate C++ code with a schema compiler:
* [Schema compiler H](include/imlab/schemac/schema_compiler.h)
* [Schema compiler CC](tools/schemac/schema_compiler.cc)

## Task 3

* Implement the [Delivery](data/delivery.sql) transaction.
* Execute 1 million transactions [composed of a mix](tools/imlabdb.cc) of [New Order](data/new_order.sql) (90%) and [Delivery](data/delivery.sql) (10%) transactions.
* Implement [this analytical query](data/olap.sql) using STL datastructures for the join implementation.<br/>
    * Do not use any existing existing index structures.
    * Follow [this interface](include/imlab/database.h).
    * The result of the query [our dataset](https://db.in.tum.de/teaching/ws2122/imlab/tpcc_5w.tar.gz) should be *1367872001.25*.
* Execute OLTP and OLAP queries concurrently.
    * Execute 1 million transactions of the NewOrder/Delivery mix.
    * Run the analytical concurrently 10 times using the **fork** system call (in [imlabdb.cc](tools/imlabdb.cc)).
    * Whenever an analytical query is finished, create a new snapshot using fork, such that exactly one snapshot and query is active at any given time.
    * [This example](data/fork_example.cc) for using fork may be helpful.
* Implement your own lazy hash table using [this interface](include/imlab/infra/hash_table.h).
* Measure the runtimes of the analytical query with the STL datastructure and your hash table.

## Task 4

* Implement query compilation of relational algebra trees to C++ code. Use the produce/consume (push) model.
* You need to support the following operators: [table scan](include/imlab/algebra/table_scan.h), [selection](include/imlab/algebra/selection.h), [print](include/imlab/algebra/print.h) and [inner join](include/imlab/algebra/inner_join.h).
* Test your code to make sure, that the [IUs](include/imlab/algebra/iu.h) are correctly propagated through the operators.
* Manually build an algebra tree for [this SQL query](data/queryc_1.sql).
* Write a [query compiler](tools/queryc/queryc.cc) that generates C++ code for the static algebra tree.
* Add the generated C++ code to your repository and execute it in [imlabdb.cc](tools/imlabdb.cc) using the print operator.

## Task 5

* Implement a simple parser for the following subset of SQL:
    * The select clause contains one or more attribute names.
    * The from clause contains one or more relation names.
    * The where clause is optional and can contain one or more selections (connected by "and").
    * You only need to support selections of the form "attribute = attribute" and "attribute = constant".
    * You can assume that each relation is referenced only once, and that all attribute names are unique.

* Implement a semantic analysis, which takes the parsed SQL statement and the schema as input and computes an algebra tree that can be used as input to your code generator.
    * Report errors when non-existing attributes or tables are referenced.
    * You should report an error if a table has no join condition (i.e., a cross product would be necessary).
    * Build left-deep join trees based on the order of the relations in the from clause.

* Implement a REPL ("read-eval-print-loop") for your database system:
    * Read one line from standard input.
    * Analyze it (report any errors and abort if necessary).
    * Generate C++ code and store it in a temporary file.
    * Call a C++ compiler and generate a temporary shared library (.so file).
    * Using `dlopen`, `dlsym`, and `dlclose` you can now load the shared library and execute the query in it (have a look at [this example](data/dlopen)), you man need to compile with `-fPIC` and `-rdynamic`.
    * Measure and report the compilation and the execution times separately.

* Test your database with [this query](data/queryc_2.sql).

## Task 6

* Implement morsel-driven parallelism in your query engine.
    * Use `parallel_for` in your table scans. [TBB](https://www.threadingbuildingblocks.org/docs/help/tbb_userguide/parallel_for.html)
    * Make your hash table of task 3 thread-safe and build it multi-threaded with `enumerable_thread_specific`. [TBB](https://www.threadingbuildingblocks.org/docs/help/reference/thread_local_storage/enumerable_thread_specific_cls.html)

How many transactions per second can your implementation execute?

Make sure your builds are not failing! <br/>
*Left Sidebar > CI /CD > Pipelines*

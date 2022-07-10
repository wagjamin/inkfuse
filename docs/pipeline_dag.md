# PipelineDAG 
This doc gives a quick overview of how we reason about dataflow within InkFuse.

## A Primer on Suboperators
In InkFuse, computation is done through suboperators.
A suboperator represents a simple primitive performing some computation.
An example might be the an arithmetic operation on two columns.

Suboperators must have a limited degree of freedom, which allows them to serve
as building blocks of both compiled and vectorized execution.
What does this mean? It means that there must be a fixed (reasonably small) set of potential runtime configurations of the suboperator.
Let's look at the arithmetic operation on two columns.
This suboperator is parametrized over the first column type, the second column type, and the opcode of the arithmethic operation.

This is awesome, because it means that we can easily enumerate all the above combinations of valid parameters and generate vectorized primitives for them.
This is for example done in `src/interpreter/ExpressionFragmentizer`.

## Simple Computational DAGs
A query can be represented through a set of pipelines.
A pipeline is the maximum flow from one pipeline-breaking operator to the next.
Every pipeline is a DAG of suboperators.

Queries are broken down into pipelines of suboperator DAGs. The basic flow thus looks as follows:
- A query enters the system, it is transformed into a traditional relational algebra tree of operators (think aggregation, join, table scan)
- Every relational operator implements a `decay` function which lowers it to another IR. Namely, the relational operator takes the current query
pipeline and adds suboperators to the pipeline that compute the higher-level operator
- Once all relational algebra operators were decayed, we are left with a DAG of suboperators that represent the original query

There are retained in a topologically sorted ordering and then either interpreted one-by-one,
or fused into larger pieces of generated code for a specific query.



## Added Complexity
Sadly, the above picture only works well for scans, expressions and filters.
Building a database requires more than that - for example we need support for joins and aggregations.
There are two things that complicate the above picture.

### Runtime Parameters
We said that a suboperator must have a fixed set of potential runtime configurations.
But in SQL we can formulate queries such as `SELECT a + 42 FROM table`. 
How can we build suboperators for a query like this? We need some primitive that can perform the addition of a and 42.
In InkFuse this is a special operator allowing an arithmetic operation with a constant.

Our initial idea might be to add the constant as one of the parameters of the sub-operator.
This however would mean that we cannot generate all possible configurations for vectorized interpretation.
After all, SQL can be arbitrary and we would have to generate ~4 billion primitives just for 32-bit signed integers.

We thus define a second set of parameters within InkFuse, these are called "runtime parameters".
These types of parameters are dynamically evaluated at operator runtime through the runtime state passed to a suboperator.
That way, the arithmetic operation with a constant is still only parametrized over opcode and types.
The actual value is loaded dynamically during runtime.

In reality, InkFuse is a bit more fancy. 
If a suboperator is compiled in the context of a specific query (here the parameter is always known),
it will actually hard-code the value to generate more efficient code.

### Control-Flow in DAGs 
An additional complexity arises from complex operators like aggregations or joins.
Here, our model of a flag DAG of suboperators breaks down.
The core complexity is that we need to be able to resolve hash collisions in a correct way.
How does an aggregation operator work in a code-generating system? A common approach looks as-follows:
- For a given row, the aggregation columns are packed together and then hashed
- Based on the hash value, we perform a lookup into a hash table exposed by the runtime system
- We get back an iterator for that hash value, and iterate over the potential matches
- For every potential match, we generate custom key-checking logic based on the aggregation column that check whether
the given hash table entry has the same group-by columns
- One we found the match in the hash-table, update the aggregation state 

The ability to generate custom key-checking logic for that query makes our life relatively easy.
Vectorized systems have to jump through more hoops here. [Take a look](https://github.com/duckdb/duckdb/blob/1751116044c5e89eb121276e867c1f3531291295/src/execution/aggregate_hashtable.cpp#L426) at how DuckDB implements the aggregation flow.

- Based on our input chunk, we hash and get the initial matches for these hash values.
- We then run key-checking logic over every block.
- This returns a new selection mask indicating which rows did not find the suitable keys yet.
- We update the iterator by one on these rows.
- We repeat this until we found a suitable match for every row (this is the outer while loop)

This flow is relatively different from the code-generating world.
Namely, we run the vectorized primitives in a loop until we found all matches.

The goal of InkFuse is to allow vectorized and compiled execution to co-exist without much added complexity.
As such, we need to find clean abstractions that allow for the two above flows to use the same building blocks.
We allow this through embedding control-flow blocks within a single Pipeline.
Every node in a pipeline can either be a suboperator, or a control-flow structure that contains another PipelineDAG.
Currently, we allow a maximum nesting of one.

The control-flow structure is parametrized over a boolean IU.
For every row, we loop through the control-flow suboperator-IUs until it is true.

Assume we do a `SELECT sum(c) FROM table GROUP BY a, b`, where `a` is an integer column and `b` is a string column,
the sub-operator DAG would look (roughly) as follows:
```
// Outer DAG:
a -> hash \
           ---> hash_combine -> ht_lookup \   
b -> hash /                                ----> control_flow \ 
                              const_false /                    ----> agg_update_sum                   
                                                            c /

// Control-Flow DAG:
Inputs: ht_lookup, const_false
Loop IU: the IU created by const_false

             /---> key_equals_int(offset 0)    ---\
ht_lookup ---                                      ----> bool_and -> advance_it_if_false -> update_loop_var
             \---> key_equals_string(offset 4) ---/

```

Let's understand how this unifies both compiled and vectorized execution.

For vectorization, the interpreter loops through the pipeline DAG until the boolean IU is true for every row in the current chunk.
After that, we leave the control-flow structure and resume execution in the outer DAG.
Note that this pretty much exactly corresponds to the above flow in DuckDB.

For code-generation, we would roughly generate code that looks (roughly) as follows:
```python
for [a, b, c] in table:
    a_hash = hash(a)
    b_hash = hash(b)
    final_hash = hash_combine(a_hash, b_hash)
    ht_it = ht_lookup(final_hash)
    control_flow = False
    while !control_flow:
        eq_a = (*(ht_it + 0) == a)
        eq_b = (*(ht_it + 4) == b)
        control_flow = eq_a && eq_b
        if !control_flow:
            ht_advance(ht_it)
    agg_update_sum(ht_it, c, 0)   
```

As such, the suboperator model above also allows realtively natural code-generation.
Now the cool thing here is that we can 100% reuse the primitive operators for vectorization and compilation.

The only thing that is different that for compilation the control-flow operator gets baked into the `while (!control_flow)`
and in the other case we need to unroll the loop in the interpreter.

A final side-note here: the execution model of InkFuse is in principle flexible enough to allow the execution logic
to only generate optimized code for the raw lookup and key-checking logic that could then be run as a single vectorized primitive.

#### Getting This Right for Hash Joins

The above logic is also applied for hash joins.
At first glance, hash joins are even more tricky than aggregations.
This is because for a hash join, every row on the probe side can have multiple matches on the build side.
For aggregations, we know there will only be one match into which we aggregate.

Now the nice thing for hash joins is that this complexity can be hidden in the value representation of the backing hash table.
For InkFuse, hash joins materialize the rows in a set of blocks and then use the backing hash table as follows:
- For every join key, a single entry is created in the hash table
- The value stored in the hash table is a vector of pointers into the underlying blocks

This is similar to the concept of a `RowRefList` in [ClickHouse](https://github.com/ClickHouse/ClickHouse/blob/6d1154b50572b97bd7d7d32f375095644d72d69a/src/Interpreters/RowRefs.h#L36).

This allows InkFuse to use 100% the same logic for resolving collisions in joins and aggregations.
After the collision resolution we have dedicated sub-operators for the join that resolve all output rows in the hash table payload.
This has its own complexities as this operation can produce more output rows than input rows, but this approach decouples these complexities.

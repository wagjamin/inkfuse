# Incremental Fusion

This repository contains InkFuse - a prototype database for an Incremental Fusion database combining code generation and vectorization.

As a research prototype, InkFuse is exclusively focused on showing how to build an Incremental Fusion engine. It does not contain 
components deemed essential for modern production databases like a SQL parser, optimizer or concurrency control mechanisms.

Rather, the interface to InkFuse are hand-crafted physical operator trees.

## Overview

The repository effectively contains an Incremental Fusion runtime built from scratch.
As such, it does *not* support arbitrary relational operators at the moment. We only support some types of joins,
aggregations and expressions. However, this prototype is deemed sufficient to prove the feasibility of this novel
type of engine.

The repository is build to provide a clear overview of the concepts underpinning the runtime. We tried to write
well-documented and tested code which should allow everyone who is interested to take a peek under the hood and
see how such an engine can be constructed.

Let's take a look at the main directories:
- `algebra`:
- `codegen`:
- `exec`:
- `runtime`:
- `sotrage`: Backing storage engine containing relations of certain types. A table is effectively a collection of vectors containing raw data. This part of InkFuse probably received the least amount of love. It's all about being functional.

## Configuring Inkfuse 
Inkfuse comes with the following CMake settings that can be configured:
- `WITH_COVERAGE: Bool`: Should the build have coverage information attached? Default `OFF`.
- `JIT_CLANG_11: Bool`: Should the default `clang-11` be used to compile generated code? If `OFF`, then the compiler is read at runtime from the `CUSTOM_JIT` env variable. Default `ON`.

Inkfuse also listens to the following environment variables:
- `CUSTOM_JIT`: If `JIT_CLANG_11` is set to `OFF`, then this flag controls which C compiler to use.

## Contributing

If you want to learn more about modern code generating database engines, feel free to contribute. Some ideas:
- Better runtime adaptivity, i.e. smarter fusing of operators in pipelines or taking a look at operator reordering. 
- Adding new operators. You can start simple with some expression to see how the fragment generation works. If you want something more challenging, maybe try to implement a new aggregation function or data type.

Feel free to reach out to the maintainers of this repository if you want to know where to get started.

However, if you want to contribute to open source in a way which serves the overall community, we recommend you try
your luck with one of the amazing full-blown open-source database systems out there. Examples might be NoisePage (JIT Compiled),
ClickHouse (Vectorized) or DuckDB (Vectorized).


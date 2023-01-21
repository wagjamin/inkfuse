-- TPC-H Query 18: Simplified Aggregate
-- If the engine can infer o_orderkey is unique, it can replace
-- group by keys with any() aggregate function. This plan leads
-- to much more consistent plans.
select
    o_orderkey,
    sum(l_quantity)
from
    customer,
    orders,
    lineitem
where
        o_orderkey in (
        select
            l_orderkey
        from
            lineitem
        group by
            l_orderkey having
                sum(l_quantity) > 300
    )
  and c_custkey = o_custkey
  and o_orderkey = l_orderkey
group by
    o_orderkey
limit
    100

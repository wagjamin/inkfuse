-- TPC-H Query 14 - Simplified final projection significantly.
select
    sum(l_extendedprice * (1 - l_discount)) as promo_revenue
from
    lineitem,
    part
where l_partkey = p_partkey
  and l_shipdate >= date '1995-09-01'
  and l_shipdate < date '1995-09-01' + interval '1' month

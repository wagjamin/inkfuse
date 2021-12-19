select sum(ol_quantity*ol_amount-c_balance*o_ol_cnt)
from customer, "order", orderline
where o_w_id = c_w_id
and o_d_id = c_d_id
and o_c_id = c_id
and o_w_id = ol_w_id
and o_d_id = ol_d_id
and o_id = ol_o_id
and c_last like 'B%'


select c_first, c_last, o_all_local, ol_amount 
from customer, "order", orderline
where o_w_id = c_w_id
and o_d_id = c_d_id
and o_c_id = c_id
and o_w_id = ol_w_id
and o_d_id = ol_d_id
and o_id = ol_o_id
and c_id = 322
and c_w_id = 1
and c_d_id = 1


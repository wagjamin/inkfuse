select w_id from warehouse;
select c_id, c_first, c_middle, c_last from customer where c_last = 'BARBARBAR';
select d_name from district, order where d_w_id = o_w_id and o_d_id = d_id and o_id = 7;
select o_id, ol_dist_info from order, orderline where o_id = ol_o_id and ol_d_id = o_d_id and o_w_id = ol_w_id and ol_number = 1 and ol_o_id = 100;
select o_id, ol_dist_info from orderline, order where o_id = ol_o_id and ol_d_id = o_d_id and o_w_id = ol_w_id and ol_number = 1 and ol_o_id = 100;
select c_last, o_id, i_id, ol_dist_info from customer, order, orderline, item where c_id = o_c_id and c_d_id = o_d_id and c_w_id = o_w_id and o_id = ol_o_id and ol_d_id = o_d_id and o_w_id = ol_w_id and ol_number = 1 and ol_o_id = 100 and ol_i_id = i_id;
select c_last, o_id, ol_dist_info from customer, order, orderline where c_id = o_c_id and o_id = ol_o_id and ol_d_id = o_d_id and o_w_id = ol_w_id and ol_number = 1 and ol_o_id = 100;


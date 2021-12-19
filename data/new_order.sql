create transaction newOrder ( integer w_id, integer d_id, integer c_id, integer items, array(15) integer supware, array(15) integer itemid, array(15) integer qty, timestamp datetime)
{
    select w_tax from warehouse w where w.w_id=w_id;
    select c_discount from customer c where c_w_id=w_id and c_d_id=d_id and c.c_id=c_id;
    select d_next_o_id as o_id,d_tax from district d where d_w_id=w_id and d.d_id=d_id;
    update district set d_next_o_id=o_id+1 where d_w_id=w_id and district.d_id=d_id;

    var integer all_local = 1;
    forsequence (index between 0 and items-1) {
        if (w_id<>supware[index])
            all_local=0;
    }

    insert into "order" values (o_id,d_id,w_id,c_id,datetime,0,items,all_local);
    insert into neworder values (o_id,d_id,w_id);

    forsequence (index between 0 and items-1) {
        select i_price from item where i_id=itemid[index];

        select s_quantity,s_remote_cnt,s_order_cnt,case d_id when 1 then s_dist_01 when 2 then s_dist_02 when 3 then s_dist_03 when 4 then s_dist_04 when 5 then s_dist_05 when 6 then s_dist_06 when 7 then s_dist_07 when 8 then s_dist_08 when 9 then s_dist_09 when 10 then s_dist_10 end as s_dist
        from stock
        where s_w_id=supware[index] and s_i_id=itemid[index];

        if (s_quantity>qty[index]) {
            update stock set s_quantity=s_quantity-qty[index] where s_w_id=supware[index] and s_i_id=itemid[index];
        } else {
            update stock set s_quantity=s_quantity+91-qty[index] where s_w_id=supware[index] and s_i_id=itemid[index];
        }

        if (supware[index]<>w_id) {
            update stock set s_remote_cnt=s_remote_cnt+1 where s_w_id=w_id and s_i_id=itemid[index];
        } else {
            update stock set s_order_cnt=s_order_cnt+1 where s_w_id=w_id and s_i_id=itemid[index];
        }

        var numeric(6,2) ol_amount=qty[index]*i_price*(1.0+w_tax+d_tax)*(1.0-c_discount);
        insert into orderline values (o_id,d_id,w_id,index+1,itemid[index],supware[index],0,qty[index],ol_amount,s_dist);
    }

    commit;
};

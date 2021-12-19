create transaction delivery(integer w_id, integer o_carrier_id, timestamp datetime)
{
   forsequence (d_id between 1 and 10) {
      select min(no_o_id) as o_id from neworder where no_w_id=w_id and no_d_id=d_id order by no_o_id else { continue; } -- ignore this district if no row found
      delete from neworder where no_w_id=w_id and no_d_id=d_id and no_o_id=o_id;

      select o_ol_cnt,o_c_id from "order" o where o_w_id=w_id and o_d_id=d_id and o.o_id=o_id;
      update "order" set o_carrier_id=o_carrier_id where o_w_id=w_id and o_d_id=d_id and "order".o_id=o_id;

      var numeric(6,2) ol_total=0;
      forsequence (ol_number between 1 and o_ol_cnt) {
         select ol_amount from orderline where ol_w_id=w_id and ol_d_id=d_id and ol_o_id=o_id and orderline.ol_number=ol_number;
         ol_total=ol_total+ol_amount;
         update orderline set ol_delivery_d=datetime where ol_w_id=w_id and ol_d_id=d_id and ol_o_id=o_id and orderline.ol_number=ol_number;
      }

      update customer set c_balance=c_balance+ol_total where c_w_id=w_id and c_d_id=d_id and c_id=o_c_id;
   }

   commit;
};


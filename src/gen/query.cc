

#include "gen/query.h"

namespace imlab

{

void GeneratedQuery::executeOLAPQuery(Database& db) 
    {
        LazyMultiMap<Key<Integer,Integer,Integer>, std::tuple<Varchar<16>,Varchar<16>>> ht_0x4ae740;
        LazyMultiMap<Key<Integer,Integer,Integer>, std::tuple<Numeric<1,0>,Varchar<16>,Varchar<16>>> ht_0x4ae960;
        std::mutex print_mutex_0x4aeb50;
        tbb::parallel_for(tbb::blocked_range<int>(0, db.customer.getSize()), [&](tbb::blocked_range<int> r)
        {
            Integer iu_0x4adbc0;
            Integer iu_0x4adbe0;
            Integer iu_0x4adc00;
            Varchar<16> iu_0x4adc20;
            Varchar<16> iu_0x4adc60;
            for(size_t scan_0x4ad6f0_tid= r.begin(); scan_0x4ad6f0_tid < r.end();scan_0x4ad6f0_tid++)
            {
                auto row_0x4ad6f0 = db.customer.Read(scan_0x4ad6f0_tid);
                iu_0x4adbc0 = std::get<0>(row_0x4ad6f0);
                iu_0x4adbe0 = std::get<1>(row_0x4ad6f0);
                iu_0x4adc00 = std::get<2>(row_0x4ad6f0);
                iu_0x4adc20 = std::get<3>(row_0x4ad6f0);
                iu_0x4adc60 = std::get<5>(row_0x4ad6f0);
                Integer iu_0x4a64a8 = Integer{322};
                bool iu_0x4ae3d8 = (iu_0x4adbc0 == iu_0x4a64a8);
                Integer iu_0x4ae138 = Integer{1};
                bool iu_0x4ae448 = (iu_0x4adbe0 == iu_0x4ae138);
                bool iu_0x4ae568 = (iu_0x4ae3d8 && iu_0x4ae448);
                Integer iu_0x4ae1c8 = Integer{1};
                bool iu_0x4ae4d8 = (iu_0x4adc00 == iu_0x4ae1c8);
                bool iu_0x4ae5f8 = (iu_0x4ae568 && iu_0x4ae4d8);
                if (iu_0x4ae5f8)
                {
                    Key<Integer,Integer,Integer> packed_key_0x4ae6d0;
                    std::get<0>(packed_key_0x4ae6d0.components) = iu_0x4adbc0;
                    std::get<1>(packed_key_0x4ae6d0.components) = iu_0x4adbe0;
                    std::get<2>(packed_key_0x4ae6d0.components) = iu_0x4adc00;
                    std::tuple<Varchar<16>,Varchar<16>> packed_value_0x4ae6d0;
                    std::get<0>(packed_value_0x4ae6d0) = iu_0x4adc20;
                    std::get<1>(packed_value_0x4ae6d0) = iu_0x4adc60;
                    ht_0x4ae740.insert(packed_key_0x4ae6d0, packed_value_0x4ae6d0);
                }
            }
        }
        );
        ht_0x4ae740.finalize();
        tbb::parallel_for(tbb::blocked_range<int>(0, db.order.getSize()), [&](tbb::blocked_range<int> r)
        {
            Integer iu_0x4ad8a0;
            Integer iu_0x4ad8c0;
            Integer iu_0x4ad8e0;
            Numeric<1,0> iu_0x4ad980;
            for(size_t scan_0x4ad770_tid= r.begin(); scan_0x4ad770_tid < r.end();scan_0x4ad770_tid++)
            {
                auto row_0x4ad770 = db.order.Read(scan_0x4ad770_tid);
                iu_0x4ad8a0 = std::get<0>(row_0x4ad770);
                iu_0x4ad8c0 = std::get<1>(row_0x4ad770);
                iu_0x4ad8e0 = std::get<2>(row_0x4ad770);
                iu_0x4ad980 = std::get<7>(row_0x4ad770);
                Key<Integer,Integer,Integer>  packed_key_right_0x4ae6d0;
                std::get<0>( packed_key_right_0x4ae6d0.components) = iu_0x4ad8a0;
                std::get<1>( packed_key_right_0x4ae6d0.components) = iu_0x4ad8c0;
                std::get<2>( packed_key_right_0x4ae6d0.components) = iu_0x4ad8e0;
                auto [it_start_0x4ae6d0, it_end_0x4ae6d0] = ht_0x4ae740.equal_range( packed_key_right_0x4ae6d0);
                for(;it_start_0x4ae6d0 != it_end_0x4ae6d0;++it_start_0x4ae6d0)
                {
                    std::tuple<Varchar<16>,Varchar<16>>& packed_tuple_read_0x4ae6d0 = *it_start_0x4ae6d0;
                    Varchar<16> iu_0x4adc20 = std::get<0>(packed_tuple_read_0x4ae6d0);
                    Varchar<16> iu_0x4adc60 = std::get<1>(packed_tuple_read_0x4ae6d0);
                    Key<Integer,Integer,Integer> packed_key_0x4ae8f0;
                    std::get<0>(packed_key_0x4ae8f0.components) = iu_0x4ad8a0;
                    std::get<1>(packed_key_0x4ae8f0.components) = iu_0x4ad8c0;
                    std::get<2>(packed_key_0x4ae8f0.components) = iu_0x4ad8e0;
                    std::tuple<Numeric<1,0>,Varchar<16>,Varchar<16>> packed_value_0x4ae8f0;
                    std::get<0>(packed_value_0x4ae8f0) = iu_0x4ad980;
                    std::get<1>(packed_value_0x4ae8f0) = iu_0x4adc20;
                    std::get<2>(packed_value_0x4ae8f0) = iu_0x4adc60;
                    ht_0x4ae960.insert(packed_key_0x4ae8f0, packed_value_0x4ae8f0);
                }
            }
        }
        );
        ht_0x4ae960.finalize();
        tbb::parallel_for(tbb::blocked_range<int>(0, db.orderline.getSize()), [&](tbb::blocked_range<int> r)
        {
            Integer iu_0x4ad9b0;
            Integer iu_0x4ad9d0;
            Integer iu_0x4ad9f0;
            Numeric<6,2> iu_0x4adab0;
            for(size_t scan_0x4ad7f0_tid= r.begin(); scan_0x4ad7f0_tid < r.end();scan_0x4ad7f0_tid++)
            {
                auto row_0x4ad7f0 = db.orderline.Read(scan_0x4ad7f0_tid);
                iu_0x4ad9b0 = std::get<0>(row_0x4ad7f0);
                iu_0x4ad9d0 = std::get<1>(row_0x4ad7f0);
                iu_0x4ad9f0 = std::get<2>(row_0x4ad7f0);
                iu_0x4adab0 = std::get<8>(row_0x4ad7f0);
                Key<Integer,Integer,Integer>  packed_key_right_0x4ae8f0;
                std::get<0>( packed_key_right_0x4ae8f0.components) = iu_0x4ad9b0;
                std::get<1>( packed_key_right_0x4ae8f0.components) = iu_0x4ad9d0;
                std::get<2>( packed_key_right_0x4ae8f0.components) = iu_0x4ad9f0;
                auto [it_start_0x4ae8f0, it_end_0x4ae8f0] = ht_0x4ae960.equal_range( packed_key_right_0x4ae8f0);
                for(;it_start_0x4ae8f0 != it_end_0x4ae8f0;++it_start_0x4ae8f0)
                {
                    std::tuple<Numeric<1,0>,Varchar<16>,Varchar<16>>& packed_tuple_read_0x4ae8f0 = *it_start_0x4ae8f0;
                    Numeric<1,0> iu_0x4ad980 = std::get<0>(packed_tuple_read_0x4ae8f0);
                    Varchar<16> iu_0x4adc20 = std::get<1>(packed_tuple_read_0x4ae8f0);
                    Varchar<16> iu_0x4adc60 = std::get<2>(packed_tuple_read_0x4ae8f0);
                    std::lock_guard<std::mutex> lock_0x4aeb50(print_mutex_0x4aeb50);
                    std::stringstream print_stream;
                    print_stream << iu_0x4ad980 << ',';
                    print_stream << iu_0x4adab0 << ',';
                    print_stream << iu_0x4adc20 << ',';
                    print_stream << iu_0x4adc60 << std::endl;
                    std::cout << print_stream.str();
                }
            }
        }
        );
    }
}

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "/tmp/global_runtime.c"

uint8_t execute(void** global_state)
{
    uint64_t iu_scan_idx;
    uint64_t iu_scan_0xffffb6361940_end;
    iu_scan_idx = (*(((struct LoopDriverState*) ((*(global_state + 0)))))).start;
    iu_scan_0xffffb6361940_end = (*(((struct LoopDriverState*) ((*(global_state + 0)))))).end;
    int32_t* iu_scan_0xffffb6586220_start;
    iu_scan_0xffffb6586220_start = ((int32_t*) ((*(((struct IndexedIUProviderState*) ((*(global_state + 7)))))).start));
    char* iu_scan_0xffffb6585f20_start;
    iu_scan_0xffffb6585f20_start = ((char*) ((*(((struct IndexedIUProviderState*) ((*(global_state + 1)))))).start));
    double* iu_scan_0xffffb6586020_start;
    iu_scan_0xffffb6586020_start = ((double*) ((*(((struct IndexedIUProviderState*) ((*(global_state + 3)))))).start));
    while (iu_scan_idx < iu_scan_0xffffb6361940_end)
    {
        int32_t iu_l_shipdate;
        iu_l_shipdate = (*(iu_scan_0xffffb6586220_start + iu_scan_idx));
        bool iu_0xffffb692b260;
        iu_0xffffb692b260 = iu_l_shipdate <= 10471;
        if (iu_0xffffb692b260)
        {
            char iu_l_returnflag;
            iu_l_returnflag = (*(iu_scan_0xffffb6585f20_start + iu_scan_idx));
            char iu_0xffffb730ea40;
            iu_0xffffb730ea40 = iu_l_returnflag;
            char* iu_0xffffb6101d28;
            iu_0xffffb6101d28 = ht_sk_lookup_or_insert((*(((struct RuntimeFunctionSubopState*) ((*(global_state + 17)))))).this_object, ((char*) ((&(iu_0xffffb730ea40)))));
            double iu_l_quantity;
            iu_l_quantity = (*(iu_scan_0xffffb6586020_start + iu_scan_idx));
            double iu_0xffffb730eaa0;
            iu_0xffffb730eaa0 = iu_l_quantity;
            char* iu_agg_0xffffb65862a0_packed_state;
            iu_agg_0xffffb65862a0_packed_state = iu_0xffffb6101d28 + 1;
            (*(((int64_t*) (iu_agg_0xffffb65862a0_packed_state)))) = 1 + (*(((int64_t*) (iu_agg_0xffffb65862a0_packed_state))));
        }
        iu_scan_idx = iu_scan_idx + 1;
    }
    return 0;
}


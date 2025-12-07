#ifndef HW_COUNTERS_H
#define HW_COUNTERS_H

#include <stdint.h>

/* Minimal HW counter helper API for DTEK-V (RV32)
   - hwc_init() will program a recommended set of mhpmeventX
   - hwc_set_event(n, code) allows customizing mhpmevent3..9
   - hwc_clear() zeros counters (mcycle, minstret, mhpmcounter3..9)
   - hwc_snapshot(out) fills out[0..8] with mcycle, minstret, mhpm3..9
*/

void hwc_init(void);
void hwc_set_event(unsigned mhpmevent_index /* 3..9 */, uint32_t event_code);
void hwc_clear(void);
uint32_t hwc_read_mcycle(void);
uint32_t hwc_read_minstret(void);
uint32_t hwc_read_mhpm(unsigned index /* 3..9 */);
void hwc_snapshot(uint32_t out[9]);

/* Common event codes (from intro_hw_counters_v11.pdf) */
#define HWEV_NONE               0x00
#define HWEV_ICACHE_MISS        0x01
#define HWEV_ITLB_MISS          0x02
#define HWEV_IFETCH_STALL       0x03
#define HWEV_BRANCH_RESOLVE_STALL 0x04

#define HWEV_LOAD_USE_STALL     0x05
#define HWEV_LOAD_RETIRED       0x06
#define HWEV_STORE_RETIRED      0x07
#define HWEV_DCACHE_MISS        0x08
#define HWEV_DTLB_MISS          0x09

#define HWEV_ALU_RETIRED        0x0A
#define HWEV_MULT_RETIRED       0x0B
#define HWEV_DIV_RETIRED        0x0C
#define HWEV_EXEC_STALL         0x0D

#define HWEV_BRANCH_RETIRED     0x0E
#define HWEV_BRANCH_MISPRED     0x0F

#endif /* HW_COUNTERS_H */

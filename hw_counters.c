/* hw_counters.c
   Minimal helpers to program/read RV32 performance CSRs on DTEK-V.
   Uses mcycle, minstret, mhpmcounter3..9 and mhpmevent3..9.
*/

#include "hw_counters.h"

void hwc_init(void) {
    /* Program a recommended set of events into mhpmevent3..9.
       You can change these later with hwc_set_event(). */

    /* Recommended mapping for your experiments (these choices are explained below):
       mhpmcounter3 -> instruction-cache misses
       mhpmcounter4 -> data-cache misses
       mhpmcounter5 -> load-use stalls (data hazards)
       mhpmcounter6 -> branch mispredictions
       mhpmcounter7 -> execution (ALU) stalls
       mhpmcounter8 -> instruction-fetch stalls
       mhpmcounter9 -> branch retired (optional)
    */
    hwc_set_event(3, HWEV_ICACHE_MISS);
    hwc_set_event(4, HWEV_DCACHE_MISS);
    hwc_set_event(5, HWEV_LOAD_USE_STALL);
    hwc_set_event(6, HWEV_BRANCH_MISPRED);
    hwc_set_event(7, HWEV_EXEC_STALL);
    hwc_set_event(8, HWEV_IFETCH_STALL);
    hwc_set_event(9, HWEV_BRANCH_RETIRED);
}

void hwc_set_event(unsigned mhpmevent_index, uint32_t event_code) {
    switch (mhpmevent_index) {
    case 3: __asm__ volatile ("csrw mhpmevent3, %0" :: "r"(event_code)); break;
    case 4: __asm__ volatile ("csrw mhpmevent4, %0" :: "r"(event_code)); break;
    case 5: __asm__ volatile ("csrw mhpmevent5, %0" :: "r"(event_code)); break;
    case 6: __asm__ volatile ("csrw mhpmevent6, %0" :: "r"(event_code)); break;
    case 7: __asm__ volatile ("csrw mhpmevent7, %0" :: "r"(event_code)); break;
    case 8: __asm__ volatile ("csrw mhpmevent8, %0" :: "r"(event_code)); break;
    case 9: __asm__ volatile ("csrw mhpmevent9, %0" :: "r"(event_code)); break;
    default: break;
    }
}

/* Clear counters. On DTEK-V (bare-metal M-mode) these writes are allowed.
   If your platform forbids writing mcycle/minstret, use baseline-read-and-subtract
   (I provide instructions below to switch to that mode if needed). */
void hwc_clear(void) {
    __asm__ volatile (
        "csrw mcycle, x0\n\t"
        "csrw minstret, x0\n\t"
        "csrw mhpmcounter3, x0\n\t"
        "csrw mhpmcounter4, x0\n\t"
        "csrw mhpmcounter5, x0\n\t"
        "csrw mhpmcounter6, x0\n\t"
        "csrw mhpmcounter7, x0\n\t"
        "csrw mhpmcounter8, x0\n\t"
        "csrw mhpmcounter9, x0\n\t"
        ::: "memory");
}

uint32_t hwc_read_mcycle(void) {
    uint32_t v;
    __asm__ volatile("csrr %0, mcycle" : "=r"(v));
    return v;
}
uint32_t hwc_read_minstret(void) {
    uint32_t v;
    __asm__ volatile("csrr %0, minstret" : "=r"(v));
    return v;
}
uint32_t hwc_read_mhpm(unsigned idx) {
    uint32_t v = 0;
    switch (idx) {
      case 3: __asm__ volatile("csrr %0, mhpmcounter3" : "=r"(v)); break;
      case 4: __asm__ volatile("csrr %0, mhpmcounter4" : "=r"(v)); break;
      case 5: __asm__ volatile("csrr %0, mhpmcounter5" : "=r"(v)); break;
      case 6: __asm__ volatile("csrr %0, mhpmcounter6" : "=r"(v)); break;
      case 7: __asm__ volatile("csrr %0, mhpmcounter7" : "=r"(v)); break;
      case 8: __asm__ volatile("csrr %0, mhpmcounter8" : "=r"(v)); break;
      case 9: __asm__ volatile("csrr %0, mhpmcounter9" : "=r"(v)); break;
      default: v = 0; break;
    }
    return v;
}

void hwc_snapshot(uint32_t out[9]) {
    if (!out) return;
    out[0] = hwc_read_mcycle();
    out[1] = hwc_read_minstret();
    for (unsigned i = 0; i < 7; ++i) out[2 + i] = hwc_read_mhpm(3 + i);
}

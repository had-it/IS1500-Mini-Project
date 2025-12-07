/* Fractal Visualizer
Date: 2025-12-xx
Authors:
*/

#include "dtekv-lib.h"
#include <stdint.h>

// Functions from fractals.c
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256], int palette);
extern uint8_t iter_to_index(int iter, int max_iter);

// Functions from panels.c
extern void draw_menu_panel(int selected_right, int menu_state, uint32_t bb_addr, uint32_t fb_addr);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50   // keep same value used when building palette / testing

// VGA
#define VGA      ((volatile uint8_t *)0x08000000UL)
#define VGA_CTRL ((volatile uint32_t *)0x04000100UL)

// VGA DMA
#define DMA_SWAP        VGA_CTRL[0]
#define DMA_BACKBUFFER  VGA_CTRL[1]
#define DMA_STATUS      VGA_CTRL[3]

// Two framebuffers inside the VGA frame-memory region
#define FB_ADDR      (0x08000000u)
#define FB2_ADDR     (FB_ADDR + (W * H))

// SWITCH
#define SWITCH   ((volatile uint32_t *)0x04000010UL)

// BUTTON
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)
#define BUTTON_EDGE         ((volatile int*) 0x040000dc)
#define BUTTON_INTERRUPT    ((volatile int*) 0x040000d8)

// GLOBAL VARIABLES
volatile int menu_state = 0;
volatile int fractal_type = 0;
static uint8_t palette[256];
static uint8_t *current_palette; // pointer for chosen palette
static volatile int last_btn = 0;

// Initial center of the Fractals
int32_t center_x = -32768;   // -0.5 * (1 << 16)
int32_t center_y = 0;
int32_t scale = 5 * (1 << 16);  // 5.0 in fixed point format (Q16.16)
int32_t pixel;

// current front and back buffer addresses
static uint32_t bb_addr = FB2_ADDR;
static uint32_t fb_addr = FB_ADDR;

// Getting switch and button
static int get_sw(void) {
    return (*SWITCH);
}
static int get_btn(void) {
    return (*BUTTON);
}

// Clearing the whole VGA buffer by setting all pixels to black (0)
static void clearScreen(void){
    volatile uint8_t *fb = VGA;
    for (int i = 0; i < W*H; ++i) {
        fb[i] = 0;
    }
}

void buffer_swap(uint32_t bb_addr_local) {
    DMA_BACKBUFFER = bb_addr_local; // set backbuffer address
    DMA_SWAP = 0;  // This triggers the swap

    // wait for swap to complete
    while (DMA_STATUS & 0x1) {
        continue;
    }
}

/* Draw using functions from fractals.c */
static void draw_fractal_to_fb_once(int fractal_type_local, uint8_t palette_local[256], int32_t scale_local, int32_t center_x_local, int32_t center_y_local) {
    uint8_t *bb = (uint8_t *) bb_addr; // Pointer to the backbuffer
    int32_t pixel_local = (int32_t)(((int64_t)scale_local) / W);

    int half_w = W / 2;
    int half_h = H / 2;

    // Choose fractal outside the loop for speedoptimization
    int (*fractal_func)(int32_t, int32_t, int);
    if (fractal_type_local == 0){
        fractal_func = mandelbrot;
    } else {
        fractal_func = burningship;
    }

    for (int py = 0; py < H; ++py) {
        int32_t cy = center_y_local + (int32_t)(((int64_t)(py - half_h) * pixel_local));
        uint8_t *row = &bb[py * W];
        int32_t cx = center_x_local + (int32_t)(((int64_t)(- half_w) * pixel_local));
        for (int px = 0; px < W; ++px) {
            int iter = fractal_func(cx, cy, MAX_ITER);
            cx += pixel_local;
            uint8_t idx = iter_to_index(iter, MAX_ITER);
            row[px] = palette_local[idx];
        }
    }

    /* swap to the buffer we just wrote, and flip for next frame */
    buffer_swap(bb_addr);

    // Swap front and back buffer addresses
    uint32_t tmp = bb_addr;
    bb_addr = fb_addr;
    fb_addr = tmp;
}

/* ----------------- CSR reads for perf counters ----------------- */
static inline uint32_t read_mcycle(void) {
    uint32_t v;
    asm volatile ("csrr %0, mcycle" : "=r"(v));
    return v;
}
static inline uint32_t read_minstret(void) {
    uint32_t v;
    asm volatile ("csrr %0, minstret" : "=r"(v));
    return v;
}
static inline uint32_t read_mhpm3(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter3" : "=r"(v)); return v; }
static inline uint32_t read_mhpm4(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter4" : "=r"(v)); return v; }
static inline uint32_t read_mhpm5(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter5" : "=r"(v)); return v; }
static inline uint32_t read_mhpm6(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter6" : "=r"(v)); return v; }
static inline uint32_t read_mhpm7(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter7" : "=r"(v)); return v; }
static inline uint32_t read_mhpm8(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter8" : "=r"(v)); return v; }
static inline uint32_t read_mhpm9(void)  { uint32_t v; asm volatile ("csrr %0, mhpmcounter9" : "=r"(v)); return v; }

/* ----------------- small printing helpers ----------------- */
/* Print 3 decimal digits padded (for fraction) */
static void print_padded3(uint32_t v) {
    char buf[4];
    buf[3] = '\0';
    buf[2] = '0' + (v % 10); v /= 10;
    buf[1] = '0' + (v % 10); v /= 10;
    buf[0] = '0' + (v % 10);
    print(buf);
}

/* Print CSV header (one-time) */
static void print_csv_header(void) {
    print("cycles,insts,mhpm3,mhpm4,mhpm5,mhpm6,mhpm7,mhpm8,mhpm9\n");
}

/* Compute integer part and 3 fractional digits of insts / cycles without 64-bit div.
   Returns ip_int in *ip_int and fractional 3-digit value in *ip_frac (0..999).
   Assumes cycles != 0. Uses repeated long-division for digits.
*/
static void compute_ipc_3digits(uint32_t insts, uint32_t cycles, uint32_t *ip_int, uint32_t *ip_frac) {
    if (cycles == 0) { *ip_int = 0; *ip_frac = 0; return; }
    uint32_t intpart = insts / cycles;
    uint32_t rem = insts - intpart * cycles;
    uint32_t frac = 0;
    for (int d = 0; d < 3; ++d) {
        // rem = rem * 10; careful about overflow - in practice rem < cycles and cycles << 2^32
        rem = rem * 10U;
        uint32_t digit = 0;
        if (rem >= cycles) {
            digit = rem / cycles;    // 32-bit div: safe
            rem = rem - digit * cycles;
        }
        frac = frac * 10U + digit;
    }
    *ip_int = intpart;
    *ip_frac = frac;
}

/* Measure N draws: read counters before and after each call, print deltas as CSV */
static void measure_draws_n(int fractal_t, uint8_t pal[256], int32_t scl, int32_t cx, int32_t cy, int n) {
    print("BEGIN_MEASURE\n");
    print_csv_header();

    for (int i = 0; i < n; ++i) {
        uint32_t b_cycles = read_mcycle();
        uint32_t b_insts  = read_minstret();
        uint32_t b3 = read_mhpm3();
        uint32_t b4 = read_mhpm4();
        uint32_t b5 = read_mhpm5();
        uint32_t b6 = read_mhpm6();
        uint32_t b7 = read_mhpm7();
        uint32_t b8 = read_mhpm8();
        uint32_t b9 = read_mhpm9();

        draw_fractal_to_fb_once(fractal_t, pal, scl, cx, cy);

        uint32_t a_cycles = read_mcycle();
        uint32_t a_insts  = read_minstret();
        uint32_t a3 = read_mhpm3();
        uint32_t a4 = read_mhpm4();
        uint32_t a5 = read_mhpm5();
        uint32_t a6 = read_mhpm6();
        uint32_t a7 = read_mhpm7();
        uint32_t a8 = read_mhpm8();
        uint32_t a9 = read_mhpm9();

        uint32_t d_cycles = a_cycles - b_cycles;
        uint32_t d_insts  = a_insts  - b_insts;
        uint32_t d3 = a3 - b3;
        uint32_t d4 = a4 - b4;
        uint32_t d5 = a5 - b5;
        uint32_t d6 = a6 - b6;
        uint32_t d7 = a7 - b7;
        uint32_t d8 = a8 - b8;
        uint32_t d9 = a9 - b9;

        // CSV row
        print_dec(d_cycles); print(","); print_dec(d_insts); print(",");
        print_dec(d3); print(","); print_dec(d4); print(","); print_dec(d5); print(",");
        print_dec(d6); print(","); print_dec(d7); print(","); print_dec(d8); print(",");
        print_dec(d9); print("\n");

        // human-readable IPC: compute integer and 3-digit fraction
        if (d_cycles != 0) {
            uint32_t ip_int, ip_frac;
            compute_ipc_3digits(d_insts, d_cycles, &ip_int, &ip_frac);
            print("IPC   : ");
            print_dec(ip_int);
            print(".");
            print_padded3(ip_frac);
            print("\n");
        } else {
            print("IPC   : - (zero cycles)\n");
        }
    }

    print("END_MEASURE\n");
}

/* keep compatibility wrapper */
static void draw_fractal_to_fb(int fractal_type_local, uint8_t palette_local[256], int32_t scale_local, int32_t center_x_local, int32_t center_y_local) {
    draw_fractal_to_fb_once(fractal_type_local, palette_local, scale_local, center_x_local, center_y_local);
}

void handle_interrupt(unsigned cause) {

    *BUTTON_EDGE = 0; // Reset the edge button
    int btn = get_btn() & 1;

    // Return if no rising edge detected
    if (btn){
        last_btn = 1;
        return;
    }

    last_btn = 0;
    int sw = get_sw();

    /* ------------- 1. PALETTE MENU -------------*/
    if (menu_state == 0){
        if((sw & (1u << 0)) == 0) {
            build_palette(palette, 0); // Palette 1 - fire
            current_palette = palette;
            menu_state = 1;
            return;
        }
        // Switch 1
        if (sw & (1u << 0)) {
            build_palette(palette, 1); // Palette 2 - sea
            current_palette = palette;
            menu_state = 1;
            return;
        }
        return;
    }
    /* ------------- 2. FRACTAL MENU -------------*/
    else if (menu_state == 1){

        if ((sw & (1u << 0)) == 0) {// If switch 0 is off
                fractal_type = 0; // Mandelbrot
                measure_draws_n(fractal_type, current_palette, scale, center_x, center_y, 10);
                menu_state = 2;
                return;
            }
            // Switch 1
        if (sw & (1u << 0))  { // If switch 0 is on
                fractal_type = 1; // Burning Ship
                measure_draws_n(fractal_type, current_palette, scale, center_x, center_y, 10);
                menu_state = 2;
                return;
            }
            return;
    }
    /* ------------- 3. NAVIGATION STATE -------------*/
    else if (menu_state == 2){
        if (sw & (1u << 0)) {
                    center_y += pixel;
                } else if (sw & (1u << 1)) {
                    center_y -= pixel;
                } else if (sw & (1u << 2)) {
                    center_x += pixel;
                } else if (sw & (1u << 3)) {
                    center_x -= pixel;
                } else if (sw & (1u << 4)) {
                    scale -= (pixel*10);
                } else if (sw & (1u << 5)) {
                    scale += (pixel*10);
                }
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                return;
        }
}

void labinit(void) {

  // Button
  *BUTTON_EDGE = 0; //resets edgecapture to 0
  *BUTTON_INTERRUPT = 0x1; // 1 on bit0 enables interrupt
  asm volatile ("csrsi mstatus,3"); // mstatus = machine status control register. Enable interrupts
  asm volatile ("csrsi mie,18"); // machine interrupt enable control register. Accept interrupts from Switches
}

int main(void) {
    labinit();

    pixel = (int32_t)(((int64_t)scale) / W);

    while (1) {
        if (menu_state == 0) {
            static int last_sw0 = -1;
            int sw0 = (get_sw() & 1) ? 1 : 0;
            if (sw0 != last_sw0) {
                draw_menu_panel(sw0, menu_state, bb_addr, fb_addr);
                last_sw0 = sw0;
            }
            for (volatile int d = 0; d < 20000; ++d) ;
        } else if (menu_state == 1) {
            /* keep chooser visible and update when SW0 toggles */
            static int last_sw0 = -1;
            int sw0 = (get_sw() & 1) ? 1 : 0;
            if (sw0 != last_sw0) {
                draw_menu_panel(sw0, menu_state, bb_addr, fb_addr);
                last_sw0 = sw0;
            }
            for (volatile int d = 0; d < 20000; ++d) ;
        } else {
            asm volatile ("wfi");
        }
    }
    return 0; // Does not reach here
}

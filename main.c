/* 
   Fractal Visualizer

   Authors:
   Eliza Anna Kizowska
   Hadia Abdulova

   Date:
   2025-12-07
*/

#include <stdint.h>

// Functions from fractals.c and panels.c
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256], int palette);
extern uint8_t iter_to_index(int iter, int max_iter);
extern void draw_menu_panel(int selected_right, int menu_state, uint32_t bb_addr, uint32_t fb_addr);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50

// VGA
#define VGA             ((volatile uint8_t *)0x08000000)  
#define VGA_CTRL        ((volatile uint32_t *)0x04000100) 
#define DMA_BUFFER      VGA_CTRL[0]
#define DMA_BACKBUFFER  VGA_CTRL[1]
#define DMA_STATUS      VGA_CTRL[3]
#define FB_ADDR         (0x08000000u)   // Base address of framebuffer region
#define FB2_ADDR        (FB_ADDR + (W * H))   // Second framebuffer address. difference between two fbs is that one frambuffer is after another in memory

// BUTTON AND SWITCHES
#define SWITCH   ((volatile uint32_t *)0x04000010)
#define BUTTON   ((volatile uint32_t *)0x040000D0)
#define BUTTON_EDGE         ((volatile int*) 0x040000dc)
#define BUTTON_INTERRUPT    ((volatile int*) 0x040000d8)

// GLOBAL VARIABLES
volatile int menu_state = 0;
volatile int fractal_type = 0;
static uint8_t palette[256];
static uint8_t *current_palette; // pointer for choosed pallette, needs for draw_fractal
int32_t center_x = -32768;   // -0.5 * (1 << 16)
int32_t center_y = 0;
int32_t scale = 5 * (1 << 16);  // 5.0 in fixed point format (Q16.16)
int32_t pixel;

// Current front and back buffer addresses
static uint32_t bb_addr = FB2_ADDR;
static uint32_t fb_addr = FB_ADDR;

// Getting switch and button
static int get_sw(void) {
    return (*SWITCH);
}
static int get_btn(void) {
    return (*BUTTON);
}

// Swaps frame- and backbuffers
void buffer_swap(void) {
    DMA_BACKBUFFER = bb_addr; // set backbuffer address
    DMA_BUFFER = 0;  // trigger the swap 
    
    // wait for swap to complete
    while (DMA_STATUS & 0x1) {    
        continue;
    }
    // swap front and back buffer addresses
    uint32_t tmp = bb_addr; 
    bb_addr = fb_addr; 
    fb_addr = tmp;
}

// Draw fractals
static void draw_fractal_to_fb(int fractal_type, uint8_t palette[256], int32_t scale, int32_t center_x, int32_t center_y) {
    uint8_t *bb = (uint8_t *) bb_addr; // pointer to backbuffer
    int32_t pixel = scale / W;

    int half_w = W / 2;
    int half_h = H / 2;

    // Create a pointer function for fractals
    int (*fractal_func)(int32_t, int32_t, int);
    if (fractal_type == 0){
        fractal_func = mandelbrot;
    } else {
        fractal_func = burningship;
    }

    // draw fractal with palette to pixel
    for (int py = 0; py < H; ++py) {  // loop pixels in y direction
        int32_t cy = center_y + (int32_t)(((int64_t)(py - half_h) * pixel));
        uint8_t *row = &bb[py * W];
        int32_t cx = center_x + (int32_t)(((int64_t)(- half_w) * pixel));
        for (int px = 0; px < W; ++px) {  // loop pixels in x direction
            int iter = fractal_func(cx, cy, MAX_ITER);
            cx += pixel;
            uint8_t idx = iter_to_index(iter, MAX_ITER);
            row[px] = palette[idx];
        }
    }

    buffer_swap(); // swapping framebuffer and backbuffer
}

// Handles interrupt from button in all the panel states
void handle_interrupt(unsigned cause) {
    *BUTTON_EDGE = 0; // reset the edge button
    int btn = get_btn() & 1;

    // return if no rising edge detected 
    if (btn){
        return;
    }
    int sw0 = get_sw();
/* ------------- 1. PALETTE MENU -------------*/
    if (menu_state == 0){
        if(!(sw0 & 0x1)) { // if switch 0 is 0
            build_palette(palette, 0); // Palette 1 - fire
            current_palette = palette;
            menu_state = 1;
            return; 
        }
        if (sw0 & 0x1) { // if switch 0 is 1
            build_palette(palette, 1); // Palette 2 - sea
            current_palette = palette;
            menu_state = 1;
            return; 
        } 
        return;
    }

    /* ------------- 2. FRACTAL MENU -------------*/
    else if (menu_state == 1){
        if (!(sw0 & 0x1)) {// If switch 0 is off
                fractal_type = 0; // Mandelbrot
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                menu_state = 2;
                return; 
            }
            // Switch 1
        if (sw0 & 0x1)  { // If switch 0 is on
                fractal_type = 1; // Burning Ship
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                menu_state = 2;
                return; 
            }
            return;
    }
    /* ------------- 3. NAVIGATION STATE -------------*/
    else if (menu_state == 2){
        if (get_sw() & 0x1) { // If switch 1 is on, we go up
            center_y += pixel; // Move the center up 
        } else if (get_sw() & 0x2) { // If switch 2 is on, we go down
            center_y -= pixel; // Move the center down
        } else if (get_sw() & 0x4) { // If switch 3 is on, we go right
            center_x += pixel; // Move the center right
        } else if (get_sw() & 0x8) { // If switch 4 is on, we go left
            center_x -= pixel; // Move the center left
        } else if (get_sw() & 0x10) { // If switch 5 is on, we zoom in
            scale -= (pixel*10); // Zoom in by reducing scale
        } else if (get_sw() & 0x20) { // If switch 6 is on, we zoom out
            scale += (pixel*10); // Zoom out by increasing scale
        }
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                return;
        }
}

void labinit(void) {
  *BUTTON_EDGE = 0; //resets edgecapture to 0
  *BUTTON_INTERRUPT = 0x1; // 1 on bit0 enables interrupt
  asm volatile ("csrsi mstatus,3"); // mstatus = machine status control register. Enable interrupts
  asm volatile ("csrsi mie,18"); // machine interrupt enable control register. Accept interrupts from buttons
}

int main(void) {
    labinit();
    pixel = scale / W;

    // Where the main part of the code happens
    // We check buttons, switches and menu states in order to interchange between panels
    while (1) {
        static int last_sw0 = -1;
        static int last_menu_state = -1;
        int sw0 = get_sw() & 1;
        
        if (menu_state == 0 || menu_state == 1) {
            if (sw0 != last_sw0 || menu_state != last_menu_state) {
                draw_menu_panel(sw0, menu_state, bb_addr, fb_addr);
                last_sw0 = sw0;
                last_menu_state = menu_state;
            }} else {
                asm volatile ("wfi"); // "wait for interrupt"
            }
        }
    return 0; // Does not reach here
}
/* Contributions: Both worked on basic functions( e.g. button, switches, VGA).
Eliza focused mostly on interrupts and Hadia on backbuffer */